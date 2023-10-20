#include <bthread.h>
#include <stdlib.h>
#include <signal.h>
#include <setjmp.h>
#include <nanvix/syscall.h>
#include <errno.h>
#include <stdio.h>

/* Quick bool implementation */
#define bool char
#define true 1
#define false 0
#define MAIN_THREAD 0

#define enqueue(t) \
    tqueue[tq_end++] = t;

#define dequeue() \
    next = tqueue[0]; \
    for (unsigned i = 0; i < BTHREAD_THREADS_MAX - 1; i++) { \
        tqueue[i] = tqueue[i + 1]; \
    } \
    tq_end-- 

typedef unsigned long context_t[BTHREAD_CTXBUF_SIZE];

enum tstate
{
    AVAILABLE,
    CREATED,
    FINISHED,
    READY,
    RUNNING,
    WAITING
};

struct tcb
{
    bthread_t tid;
    context_t ctx;
    bool is_detached;
    enum tstate state;
    void *stack;
    void *(*routine)();
    void *arg;
    void **ret;
};

static bthread_t tqueue[BTHREAD_THREADS_MAX];
static unsigned tq_end = 0;

static struct tcb threadtab[BTHREAD_THREADS_MAX];
static unsigned thd_count = 0;
static struct tcb *curr_thread = &(threadtab[MAIN_THREAD]);

static char *sched_stack[BTHREAD_STACK_SIZE];
static jmp_buf sched_ctx, first_create;

extern void loadctx(unsigned long *ctx);
extern void start_routine(unsigned (*alarm)(), jmp_buf sched_ctx, unsigned (*turnoff_alarm)(), struct tcb *thread);
static inline void set_sigalrm_handler();

static unsigned turnoff_alarm(void)
{
    unsigned ret;

	__asm__ volatile (
		"int $0x80"
		: "=a" (ret)
		: "0" (NR_btalarm),
          "b" (0)
	);

    /* Time left on old alarm */
    return ret;
}

static unsigned bthread_alarm()
{
    unsigned ret;

	__asm__ volatile (
		"int $0x80"
		: "=a" (ret)
		: "0" (NR_btalarm),
          "b" (BTHREAD_TQUANTUM)
	);

    /* Time left on old alarm */
    return ret;
}

static void btrestorer(void)
{
    __asm__ volatile (
        "pop %%ebp\n\t"
        "add $4, %%esp\n\t"
        "pop 0(%%eax)\n\t"
        "pop 4(%%eax)\n\t"
        "pop 8(%%eax)\n\t"
        "pop 12(%%eax)\n\t"
        "pop 16(%%eax)\n\t"
        "pop 20(%%eax)\n\t"
        "pop 24(%%eax)\n\t"
        "pop 28(%%eax)\n\t"
        "pop 32(%%eax)\n\t"
        "lea 0(%%esp), %%ecx\n\t"
        "movl %%ecx, 36(%%eax)\n\t"
        "push $1\n\t"
        "push %%edx\n\t"
        "call longjmp\n\t"
        : /* No output operands */
        : "a" (curr_thread->ctx),
          "d" (sched_ctx)
    );
}

static void handler()
{
    set_sigalrm_handler();
}

static inline void set_sigalrm_handler()
{
    sighandler_t ret;
    __asm__ volatile (
		"int $0x80" 
		: "=a" (ret)
		: "0" (NR_signal), 
		  "b" (SIGALRM), 
		  "c" (handler), 
		  "d" (btrestorer) 
	);
    if (ret == SIG_ERR) {
        printf("signal failed\n");
    }
}

static void scheduler(void)
{
    register int val = setjmp(sched_ctx);
    if (val == 0) {
        longjmp(first_create, 1);
    } else if (val == 1) {
        register unsigned next = 0;
        if (curr_thread->state == RUNNING) {
            curr_thread->state = READY;
            enqueue(curr_thread->tid);
        }
        dequeue();
        curr_thread = &(threadtab[next]);
        if (curr_thread->state == CREATED) {
            printf("first run tid: %d\n", curr_thread->tid);
            curr_thread->state = RUNNING;
            start_routine(bthread_alarm, sched_ctx, turnoff_alarm, curr_thread);
        } else {
            printf("swaping to tid: %d\n", curr_thread->tid);
            curr_thread->state = RUNNING;
            bthread_alarm();
            loadctx(curr_thread->ctx);
        }
    } else if (val == 2) {
        printf("finishing tid: %d\n", curr_thread->tid);
        curr_thread->state = FINISHED;
        free(curr_thread->stack);
        curr_thread->stack = NULL;
        if (curr_thread->is_detached) {
            curr_thread->state = AVAILABLE;
        }
        longjmp(sched_ctx, 1);
    }
spin:
    goto spin;
}

extern int bthread_create(bthread_t *thread, void *(*start_routine)(), void *arg)
{
    if (thd_count == 0) {
        if (!setjmp(first_create)) {
            __asm__ volatile (
                "lea 1016(%%eax), %%esp\n\t"
                "lea 1020(%%eax), %%ebp\n\t"
                "jmp *%%edx\n\t"
                : /* No output operands */
                : "a" (sched_stack),
                  "d" (scheduler)
            );
        }
        threadtab[MAIN_THREAD].tid = 0;
        threadtab[MAIN_THREAD].state = RUNNING;
        for (int i = 1; i < BTHREAD_THREADS_MAX; i++) {
            threadtab[i].state = AVAILABLE;
        }
    }
    struct tcb *this_thd = NULL;
    for (unsigned i = 1; i < BTHREAD_THREADS_MAX; i++) {
        if (threadtab[i].state == AVAILABLE) {
            this_thd = &(threadtab[i]);
            this_thd->tid = i;
            break;
        }
    }
    if (this_thd == NULL) {
        return EAGAIN;
    }
    *thread = this_thd->tid;
    this_thd->is_detached = false;
    this_thd->state = CREATED;
    this_thd->stack = malloc(BTHREAD_STACK_SIZE);
    this_thd->routine = start_routine;
    this_thd->arg = arg;
    this_thd->ret = NULL;
    enqueue(this_thd->tid);
    if (thd_count++ == 0) {
        set_sigalrm_handler();
        bthread_alarm();
    }
    return 0;
}

