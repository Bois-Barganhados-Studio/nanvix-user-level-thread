#include <stdio.h>
#include <sys/types.h>
#define IMPL_TESTS
#include <bthread.h>
#include <unistd.h>
#include <stdlib.h>

#define T1_DURATION 0x40000000
#define T2_DURATION 0x40000000

static unsigned long i = 0;
static unsigned long j = 0;

void *thd1()
{
    for (i = 0; i < T1_DURATION; i++)
        ;
    return NULL;
}

void *thd2()
{
    for (j = 0; j < T2_DURATION; j++)
        ;
    return NULL;
}

int main(/*int argc, char *const argv[]*/)
{
    bthread_t t1, t2;
    bthread_create(&t1, thd1, NULL);
    bthread_create(&t2, thd2, NULL);
    while (i != T1_DURATION && j != T2_DURATION) {
        printf("i: %d\tj: %d\n", i, j);
        for (int k = 0; k < 0x2800000; k++)
            ;
    }
    return 0;
}