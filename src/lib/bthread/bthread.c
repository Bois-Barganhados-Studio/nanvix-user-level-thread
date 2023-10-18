#include <bthread.h>
#include <stdlib.h>
#include <signal.h>
#include <nanvix/syscall.h>

/* Quick bool implementation */
#define bool char
#define true 1
#define false 0

#define MAIN_THREAD 0

typedef unsigned long context_t[BTHREAD_CTXBUF_SIZE];

enum tstate
{
    AVAILABLE,
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

static struct tcb threadtab[BTHREAD_THREADS_MAX];
static unsigned thd_count = 0;

static inline void set_sigalrm_handler();

static int bthread_alarm(void)
{
    return -1;
}

static void restorer(void)
{

}

static void handler()
{
    set_sigalrm_handler();
}

static inline void set_sigalrm_handler()
{
    unsigned long ret;
    __asm__ volatile ( 
		"int $0x80" 
		: "=a" (ret)
		: "0" (NR_signal), 
		  "b" (SIGALRM), 
		  "c" (handler), 
		  "d" (restorer) 
	);
}

static void scheduler(void)
{

}

extern int bthread_create(bthread_t *thread, void *(*start_routine)(), void *arg)
{
    if (thd_count == 0) {
        scheduler();
        threadtab[MAIN_THREAD].tid = 0;
        threadtab[MAIN_THREAD].state = RUNNING;
        for (int i = 1; i < BTHREAD_THREADS_MAX; i++) {
            threadtab[i].state = AVAILABLE;
        }
    }
    struct tcb *this_thd = NULL;
    for (int i = 1; i < BTHREAD_THREADS_MAX; i++) {
        if (threadtab[i].state == AVAILABLE) {
            this_thd = &(threadtab[i]);
            break;
        }
    }
    if (this_thd == NULL) {
        // TODO - errno
        return -1;
    }
    *thread = thd_count;
    this_thd->tid = thd_count;
    this_thd->is_detached = false;
    this_thd->state = READY;
    this_thd->stack = malloc(BTHREAD_STACK_SIZE);
    this_thd->routine = start_routine;
    this_thd->arg = arg;
    this_thd->ret = NULL;
    if (thd_count == 0) {
        set_sigalrm_handler();
        bthread_alarm();
    }
    return 0;
}

