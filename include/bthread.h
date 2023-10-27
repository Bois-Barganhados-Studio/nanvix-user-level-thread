#ifndef BTHREAD_H_
#define BTHREAD_H_

#define BTHREAD_THREADS_MAX 16      /* Maximum number of threads per process. */
#define BTHREAD_STACK_SIZE 1024     /* Thread stack size in bytes. */
#define BTHREAD_CTXBUF_SIZE 10      /* Number of registers saved in the context buffer. */
#define BTHREAD_TQUANTUM 10

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

#ifndef _ASM_FILE_

typedef unsigned bthread_t;

extern int bthread_create(bthread_t *thread, void *(*start_routine)(), void *arg);
extern void bthread_exit(void *retval);
extern int bthread_join(bthread_t thread, void **thread_return);
extern int bthread_cancel(bthread_t thread);
extern int bthread_detach(bthread_t thread);
extern void bthread_yield(void);
extern bthread_t bthread_self(void);
// Maybe do mutex stuff too.



#ifdef IMPL_TESTS
    void set_static_var(unsigned long val);
    unsigned long get_static_var();
    extern void round_robin_test();
    extern void stk_test();
#endif /* IMPL_TESTS */

#endif /* _ASM_FILE_ */
#endif /* BTHREAD_H_ */