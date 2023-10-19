#define IMPL_TESTS
#include <bthread.h>
#include <stdio.h>
#include <setjmp.h>
#include <signal.h>
#include <unistd.h>

#define NUM_THDS 2
#define T1_DURATION 0x40000000
#define T2_DURATION 0x40000000

#define SET_ALARM_HANDLER() \
    __asm__ volatile ( \
		"int $0x80" \
		: "=a" (ret) \
		: "0" (20), \
		  "b" (SIGALRM), \
		  "c" (handler), \
		  "d" (bthd_restorer) \
	)

static jmp_buf buf;
static unsigned long ctx[NUM_THDS][9];
static unsigned long count = 0;
static unsigned long ret = 0;
static unsigned long finished[NUM_THDS];

static unsigned long i = 0;
static unsigned long j = 0;

static void thread1();
static void thread2();
static void scheduler();
static void bthd_restorer();
static void handler();
extern void loadctx(unsigned long *ctx);

extern void round_robin_test()
{
    SET_ALARM_HANDLER();
    scheduler();
}

static void thread1()
{
    for (i = 0; i < T1_DURATION; i++)
        /* eat time */;
    finished[0] = 1;
}

static void thread2()
{
    for (j = 0; j < T2_DURATION; j++)
        /* eat time */;
    finished[1] = 1;
}

static void scheduler()
{
    if (!setjmp(buf)) {
        alarm(1);
        thread1();
        if (count <= 1)
            goto t2;
    } else if (count == 1) {
t2:
        alarm(1);
        thread2();
    }
    alarm(0);
    int done = 0;
    for (int i = 0; i < NUM_THDS; i++) {
        if (finished[i])
            done++;
        if (finished[count % NUM_THDS])
            count++;
    }
    if (done != NUM_THDS) {
        alarm(1);
        loadctx(ctx[count % NUM_THDS]);
    }
    alarm(0);
}

static void bthd_restorer()
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
        "push $1\n\t"
        "push %%edx\n\t"
        "call longjmp\n\t"
        : "=a" (ret)
        : "a" (ctx[count++ % NUM_THDS]),
          "d" (buf)
    );
}

static void handler()
{
    printf("i: %d\tj: %d\n", i, j);
    SET_ALARM_HANDLER();
}