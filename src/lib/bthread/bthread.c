#include <bthread.h>
#include <stdlib.h>
#include <nanvix/syscall.h>
#include <errno.h>
#include <stdio.h>

/* Quick bool implementation */
#define bool char
#define true 1
#define false 0

#define MAIN_THREAD 0

typedef struct {
    unsigned long edi;
    unsigned long esi;
    unsigned long ebp;
    unsigned long ebx;
    unsigned long edx;
    unsigned long ecx;
    unsigned long eax;
    unsigned long eflags;
    unsigned long eip;
    unsigned long esp;
} context_t[1];

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

static struct tcb threadtab[BTHREAD_THREADS_MAX];
static unsigned thd_count = 0;
static struct tcb *curr_thread = &(threadtab[MAIN_THREAD]);

static char *sched_stack[BTHREAD_STACK_SIZE];

/* extern asm functions */

/*
 * @brief Saves thread context, either at stack or in the registers.
 * @param ctx The `context_t` buffer of the thread.
 * @param mode The mode wich `savectx()` will operate, 
 * `0` to save from stack (after alarm handler call) 
 * or `1` to save from registers (at `bthread_yield()` call).
 */
extern void savectx(context_t ctx, int mode);

/*
 * @brief Loads thread context from target buffer `ctx`.
 * @param ctx The `context_t` buffer of the thread.
 */
extern void loadctx(context_t ctx);

/*
 * @brief Sets stacks registers (`%esp` & `%ebp`) to the target `stack`,
 * then calls the target non returning `noreturn_routine()`.
 * @param stack The buffer of the new stack.
 * @param noreturn_routine The target non returning routine to be called in the new stack.
 */
extern void set_stack(void *stack, void (*noreturn_routine)());

/* alarm helper functions */
static inline void set_sigalrm_handler(void);
static unsigned bthread_alarm(void);
static unsigned turnoff_alarm(void);

/* thread system functions */
static void handler(void);
static void btrestorer(void);
static void scheduler(void);

/* debug functions */


#if DEBUG_PRINT
static void print_curr_thread()
{
    static char *state[] = {
        "AVAILABLE",
        "CREATED",
        "FINISHED",
        "READY",
        "RUNNING",
        "WAITING" 
    };

    printf("curr_thread: { tid: %d, ctx: { edi: %x, esi: %x, ebx: %x, ebp: %x, ebx: %x, edx: %x, ecx: %x, eax: %x, eflags: %x, eip: %x, esp: %x } ", 
        curr_thread->tid, curr_thread->ctx->edi, curr_thread->ctx->esi, curr_thread->ctx->ebp, curr_thread->ctx->ebx, curr_thread->ctx->edx, 
        curr_thread->ctx->ecx, curr_thread->ctx->eax, curr_thread->ctx->eflags, curr_thread->ctx->eip, curr_thread->ctx->esp
    );
    printf(", state %s, stack: %x, routine: %x, arg: %x, ret: %x }\n", 
        state[curr_thread->state], curr_thread->stack, curr_thread->routine, curr_thread->arg, curr_thread->ret
    ); 
}
#endif

/*----------------------------------------------------------------------------*
 *                           ALARM HELPER FUNCTIONS                           *
 *----------------------------------------------------------------------------*/

/*
 * @brief Sets the `SIGALRM` handler function.
 */
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
        fprintf(stderr, "signal failed\n");
    }
}

/*
 * @brief Sets a `SIGALRM` for the next `BTHREAD_QUATUM`.
 */
static unsigned bthread_alarm(void)
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

/*
 * @brief Turns alarm off.
 */
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

/*----------------------------------------------------------------------------*
 *                             THREAD QUEUE SYSTEM                            *
 *----------------------------------------------------------------------------*/

#define TQ_EMPTY -1
static bthread_t tqueue[BTHREAD_THREADS_MAX];
static unsigned tq_end = 0;

/*
 * @brief Enqueues the thread id in the ready queue.
 */
static void enqueue(bthread_t tid)
{
    tqueue[tq_end++] = tid;
}

/*
 * @brief Remove the next tid from the queue,
 * then sets `curr_thread` to the next thread.
 * @return `0` for success, 
 * `TQ_EMPTY` if there's only `main()` in the queue.
 */
static int dequeue(void)
{
    unsigned next = tqueue[0];
    curr_thread = &(threadtab[next]);
    if (tq_end == 1) {
        return TQ_EMPTY;
    }
    for (unsigned i = 0; i < tq_end - 1; i++) {
        tqueue[i] = tqueue[i + 1];
    }
    tq_end--;
    return 0;
}

/*----------------------------------------------------------------------------*
 *                          THREAD SYSTEM FUNCTIONS                           *
 *----------------------------------------------------------------------------*/

/*
 * @brief Makes the first call to the thread routine, 
 * finishes the thread when the routine returns.
 */
static void routine_caller(void)
{
    struct tcb *this_thd = curr_thread;
    bthread_alarm();
    this_thd->ret = this_thd->routine(this_thd->arg);
    turnoff_alarm();
    this_thd->state = FINISHED;
    set_stack(sched_stack, scheduler);
}

/*
 * @brief Called when `SIGALRM` is raised, 
 * sets the next alarm signal, 
 * then returns to `btrestorer()`.
 */
static void handler()
{
    set_sigalrm_handler();
    /* return to btretorer */
}

/*
 * @brief Called when `handler()` returns, 
 * saves the context of the current thread, 
 * then calls `scheduler()`.
 */
static void btrestorer(void)
{
    savectx(curr_thread->ctx, 0);
    set_stack(sched_stack, scheduler);
    /* no return */
}

/*
 * @brief Schedules when each thread shall run.
 */
static void scheduler(void)
{
    if (curr_thread->state == RUNNING) {
        curr_thread->state = READY;
        enqueue(curr_thread->tid);
    } else if (curr_thread->state == FINISHED) {
        free(curr_thread->stack);
        curr_thread->stack = NULL;
        if (curr_thread->is_detached) {
            curr_thread->state = AVAILABLE;
        }
    }

    if (dequeue() == TQ_EMPTY) {
        loadctx(curr_thread->ctx);
        /* no return */
    } else if (curr_thread->state == CREATED) {
        curr_thread->state = RUNNING;
        set_stack(curr_thread->stack, routine_caller);
    } else {
        curr_thread->state = RUNNING;
        bthread_alarm();
        loadctx(curr_thread->ctx);
        /* no return */
    }
}

/*----------------------------------------------------------------------------*
 *                          USER AVAIABLE FUNCTIONS                           *
 *----------------------------------------------------------------------------*/

extern int bthread_create(bthread_t *thread, void *(*start_routine)(), void *arg)
{
    if (thd_count == 0) {
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

extern void bthread_yield(void)
{
    savectx(curr_thread->ctx, 1);
    turnoff_alarm();
    set_stack(sched_stack, scheduler);
}

extern int bthread_detach(bthread_t thread)
{
    if (thread < 1 || thread > BTHREAD_THREADS_MAX) {
        return ESRCH;
    } else if (!threadtab[thread].is_detached) {
        threadtab[thread].is_detached = true;
        if (threadtab[thread].state == FINISHED) {
            curr_thread->state = AVAILABLE;
        }
    }
    return 0;
}

extern int bthread_join(bthread_t thread, void **thread_return)
{
    if (thread < 1 || thread > BTHREAD_THREADS_MAX 
    || threadtab[thread].state == AVAILABLE) {
        return ESRCH;
    } else if (threadtab[thread].is_detached) {
        return EINVAL;
    }

    while (1) {
        if (threadtab[thread].state != FINISHED)
            bthread_yield();
        else break;
    }
    
    if (thread_return != NULL) {
        *thread_return = threadtab[thread].ret;
    }

    threadtab[thread].state = AVAILABLE;
    return 0;
}

extern bthread_t bthread_self(void)
{
    return curr_thread->tid;
}