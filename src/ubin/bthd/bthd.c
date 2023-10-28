#include <stdio.h>
#include <sys/types.h>
#define IMPL_TESTS
#include <bthread.h>
#include <unistd.h>
#include <stdlib.h>

#define T1_DURATION 0x40000000
#define T2_DURATION 0x40000000
#define T3_DURATION 0x40000000

static unsigned long MAIN = 0;
static unsigned long i = 0;
static unsigned long j = 0;
static unsigned long k = 0;
static unsigned thd_finish = 0;


void *thd1()
{
    for (i = 0; i < T1_DURATION; i++)
        ;
    puts("t1 operation done");
    thd_finish = 1;
    return NULL;
}

void *thd2()
{
    for (j = 0; j < T2_DURATION; j++)
        ;
    puts("t2 operation done");
    thd_finish = 2;
    return NULL;
}

// void *thd3()
// {
//     for (k = 0; k < T3_DURATION; k++)
//         ;
//     puts("t3 operation done");
//     return NULL;
// }


int main(/*int argc, char *const argv[]*/)
{
    bthread_t t1, t2/*, t3*/;

    bthread_create(&t1, thd1, NULL);
    bthread_create(&t2, thd2, NULL);
    //bthread_create(&t3, thd3, NULL);
    
    //void *t1_ret = NULL;

    //bthread_join(t1, &t1_ret);

    while (i != T1_DURATION || j != T2_DURATION /*|| k != T2_DURATION*/) {
        printf("i: %d\tj: %d\tk: %d\n", i, j, k);
        for (MAIN = 0; MAIN < 0x8000000; MAIN++)
            ;
    }

    return 0;
}