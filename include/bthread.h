#ifndef BTHREAD_H_
#define BTHREAD_H_

#define BTHREAD_THREADS_MAX     16      /* Maximum number of threads per process. */
#define BTHREAD_STACK_SIZE      1024    /* Thread stack size in bytes. */
#define BTHREAD_CTXBUF_SIZE     10      /* Number of registers saved in the context buffer. */
#define BTHREAD_TQUANTUM        10      /* Quantum of each thread (in system ticks) */

/* bthread context offsets */
#define BTHREAD_CTXBUF_EDI      0
#define BTHREAD_CTXBUF_ESI      4
#define BTHREAD_CTXBUF_EBP      8
#define BTHREAD_CTXBUF_EBX      12
#define BTHREAD_CTXBUF_EDX      16
#define BTHREAD_CTXBUF_ECX      20
#define BTHREAD_CTXBUF_EAX      24
#define BTHREAD_CTXBUF_EFLAGS   28
#define BTHREAD_CTXBUF_EIP      32
#define BTHREAD_CTXBUF_ESP      36

/* bthread tcb offsets */
#define BTHREAD_TCB_TID         0
#define BTHREAD_TCB_CTX         4
#define BTHREAD_TCB_DETACH      44
#define BTHREAD_TCB_STATE       48
#define BTHREAD_TCB_STACK       52
#define BTHREAD_TCB_ROUTINE     56
#define BTHREAD_TCB_ARG         60
#define BTHREAD_TCB_RET         64

#ifndef _ASM_FILE_

#define DEBUG_PRINT 0

typedef int bthread_t;
struct bt_mutex;

/*
 * @brief Creates a new thread in the calling process.
 * @param thread A pointer to a `bthread_t` variable.
 * @param start_routine The function to be executed by the created thread.
 * @param arg The only argument passed to `start_routine()`.
 * @return If sucessful `bthread_create()` shall return zero, otherwise `EAGAIN` shall be returned if there are no thread resources avaiable i.e. the calling process has already `BTHREAD_THREADS_MAX` simoutaneus thread instances. 
 */
extern int bthread_create(bthread_t *thread, void *(*start_routine)(), void *arg);

/*
 * @brief Suspends the calling thread execution while until the target `thread` terminates,
 * unless the target `thread` has already terminated. A thread can be joined only once.
 * @param thread The target thread to be joined.
 * @param thread_return The return value from the joined thread.
 * @return If sucessful `bthread_join()` shall return zero, otherwise `EINVAL` if the target `thread` is not joinable or `ESRCH` if the target `thread` could not be found.
 */
extern int bthread_join(bthread_t thread, void **thread_return);

/*
 * @brief Releases the target `thread` resources after its termination or if target `thread` is finished instantly releases it.
 * @param thread The target thread to be detached.
 * @return If sucessful `bthread_join()` shall return zero,  otherwise `ESRCH` if the target `thread` could not be found.
 */
extern int bthread_detach(bthread_t thread);

/*
 * @brief Yeilds execution by the calling thread.
 */
extern void bthread_yield(void);

/*
 * @brief Returns a `bthread_t` reffering to the calling thread.
 */
extern bthread_t bthread_self(void);

/*
 * @brief Terminates the calling thread and releases its resources.
 * @return If sucessful `bthread_cancel()` shall return zero, 
 * otherwise `ESRCH` if the target `thread` could not be found.
 */
extern int bthread_cancel(bthread_t thread);

/*
 * @brief Initializes a mutex object.
 * @return A pointer to a `bt_mutext` variable, or `NULL` if `malloc()` fails.
 */
extern struct bt_mutex *bthread_mutex_init(void);

/*
 * @brief Locks a mutex object.
 * @param mutex A pointer to a `bt_mutext` variable.
 * @return If sucessful `bthread_mutex_lock()` shall return zero,
 * otherwise `EINVAL` if the mutex is already locked or is `NULL`.
 */
extern int bthread_mutex_lock(struct bt_mutex *mutex);

/*
 * @brief Unlocks a mutex object.
 * @param mutex A pointer to a `bt_mutext` variable.
 * @return If sucessful `bthread_mutex_unlock()` shall return zero,
 * otherwise `EINVAL` if the mutex is not locked by the calling thread or is `NULL`.
 */
extern int bthread_mutex_unlock(struct bt_mutex *mutex);

/*
 * @brief Destroys a mutex object.
 * @param mutex A pointer to a `bt_mutext` variable.
 * @return If sucessful `bthread_mutex_destroy()` shall return zero,
 * otherwise `EBUSY` if the mutex is locked or `EINVAL` if mutext is `NULL`.
 */
extern int bthread_mutex_destroy(struct bt_mutex *mutex);

#endif /* _ASM_FILE_ */
#endif /* BTHREAD_H_ */
