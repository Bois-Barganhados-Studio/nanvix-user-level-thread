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
    void *ret;
};

static bthread_t tqueue[BTHREAD_THREADS_MAX];
static unsigned tq_end = 0;

static struct tcb threadtab[BTHREAD_THREADS_MAX];
static unsigned thd_count = 0;
static struct tcb *curr_thread = &(threadtab[MAIN_THREAD]);

//static char *sched_stack[BTHREAD_STACK_SIZE];
//static jmp_buf sched_ctx, first_create;

//extern void bt_do_restore(context_t thread_ctx, jmp_buf sched_ctx);
extern void savectx2(context_t ctx);
//extern void savectx(context_t ctx, jmp_buf sched_ctx);
extern void loadctx(context_t ctx);
extern void build_stack(void *stack, void (*routine_caller)());
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

    set_sigalrm_handler();

	__asm__ volatile (
		"int $0x80"
		: "=a" (ret)
		: "0" (NR_btalarm),
          "b" (BTHREAD_TQUANTUM)
	);

    /* Time left on old alarm */
    return ret;
}

// static void btrestorer(void)
// {
//     //bt_do_restore(curr_thread->ctx, sched_ctx);
//     __asm__ volatile (
//         "pop %%ebp\n\t"
//         "add $4, %%esp\n\t"
//         "pop 0(%%eax)\n\t"
//         "pop 4(%%eax)\n\t"
//         "pop 8(%%eax)\n\t"
//         "pop 12(%%eax)\n\t"
//         "pop 16(%%eax)\n\t"
//         "pop 20(%%eax)\n\t"
//         "pop 24(%%eax)\n\t"
//         "pop 28(%%eax)\n\t"
//         "pop 32(%%eax)\n\t"
//         "lea 0(%%esp), %%ecx\n\t"
//         "movl %%ecx, 36(%%eax)\n\t"
//         "push $1\n\t"
//         "push %%edx\n\t"
//         "call longjmp\n\t"
//         : /* No output operands */
//         : "a" (curr_thread->ctx),
//           "d" (sched_ctx)
//     );
// }

static void scheduler2(void);

static void routine_caller(void)
{
    struct tcb *this_thd = curr_thread;
    bthread_alarm();
    this_thd->ret = this_thd->routine(this_thd->arg);
    turnoff_alarm();
    this_thd->state = FINISHED;
    free(this_thd->stack);
    this_thd->stack = NULL;
    if (this_thd->is_detached) {
        this_thd->state = AVAILABLE;
    }
    printf("finished t%d, back to sched\n", this_thd->tid);
    scheduler2();
}

static void enqueue2(bthread_t tid)
{
    enqueue(tid);
}

static void dequeue2(void)
{
    unsigned next = 0;
    dequeue();
    curr_thread = &(threadtab[next]);
}


static void scheduler2(void)
{
    if (curr_thread->state == RUNNING) {
        savectx2(curr_thread->ctx);
        curr_thread->state = READY;
        enqueue2(curr_thread->tid);
    }
    dequeue2();
    if (curr_thread->state == CREATED) {
        curr_thread->state = RUNNING;
        build_stack(curr_thread->stack, routine_caller);
    } else {
        curr_thread->state = RUNNING;
        bthread_alarm();
        loadctx(curr_thread->ctx);
    }
}

// static void handler()
// {
//     puts("handler called");
//     set_sigalrm_handler();
// }

static inline void set_sigalrm_handler()
{
    sighandler_t ret;
    __asm__ volatile (
		"int $0x80" 
		: "=a" (ret)
		: "0" (NR_signal), 
		  "b" (SIGALRM), 
		  "c" (scheduler2), 
		  "d" (NULL) 
	);
    if (ret == SIG_ERR) {
        fprintf(stderr, "signal failed\n");
    }
}

// static void scheduler(void)
// {
//     register int val = setjmp(sched_ctx);
//     if (val == 0) {
//         longjmp(first_create, 1);
//     } else if (val == 1) {
//         register unsigned next = 0;
//         if (curr_thread->state == RUNNING) {
//             curr_thread->state = READY;
//             enqueue(curr_thread->tid);
//         }
//         dequeue();
//         curr_thread = &(threadtab[next]);
//         if (curr_thread->state == CREATED) {
//             printf("first run tid: %d\n", curr_thread->tid);
//             curr_thread->state = RUNNING;
//             start_routine(bthread_alarm, sched_ctx, turnoff_alarm, curr_thread);
//         } else {
//             printf("swaping to tid: %d\n", curr_thread->tid);
//             curr_thread->state = RUNNING;
//             bthread_alarm();
//             loadctx(curr_thread->ctx);
//         }
//     } else if (val == 2) {
//         printf("finishing tid: %d\n", curr_thread->tid);
//         curr_thread->state = FINISHED;
//         free(curr_thread->stack);
//         curr_thread->stack = NULL;
//         if (curr_thread->is_detached) {
//             curr_thread->state = AVAILABLE;
//         }
//         longjmp(sched_ctx, 1);
//     }
// spin:
//     goto spin;
// }

extern int bthread_create(bthread_t *thread, void *(*start_routine)(), void *arg)
{
    if (thd_count == 0) {
        // if (!setjmp(first_create)) {
        //     __asm__ volatile (
        //         "lea 1016(%%eax), %%esp\n\t"
        //         "lea 1020(%%eax), %%ebp\n\t"
        //         "jmp *%%edx\n\t"
        //         : /* No output operands */
        //         : "a" (sched_stack),
        //           "d" (scheduler)
        //     );
        // }
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

// extern void bthread_yield(void)
// {
//     //savectx(curr_thread->ctx, sched_ctx);
//     turnoff_alarm();
//     longjmp(sched_ctx, 1);
// }

// extern int bthread_detach(bthread_t thread)
// {
//     if (thread < 1 || thread > BTHREAD_THREADS_MAX) {
//         return ESRCH;
//     } else if (!threadtab[thread].is_detached) {
//         threadtab[thread].is_detached = true;
//         if (threadtab[thread].state == FINISHED) {
//             curr_thread->state = AVAILABLE;
//         }
//     }
//     return 0;
// }

// extern int bthread_join(bthread_t thread, void **thread_return)
// {
//     if (thread < 1 || thread > BTHREAD_THREADS_MAX 
//     || threadtab[thread].state == AVAILABLE) {
//         return ESRCH;
//     }
    
//     while (1) {
//         if (threadtab[thread].state != FINISHED) {
//             savectx(curr_thread->ctx, sched_ctx);
//         } else break;
//     }
//     if (thread_return) {
        
//     }
//     // *thread_return = threadtab[thread].ret;
//     // threadtab[thread].state = AVAILABLE;
//     puts("*join worked");
//     return 0;
// }

extern bthread_t bthread_self(void)
{
    return curr_thread->tid;
}