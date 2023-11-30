#include <stdio.h>
#include <sys/types.h>
#define IMPL_TESTS
#include <bthread.h>
#include <unistd.h>
#include <stdlib.h>

#define T1_DURATION 0x10
#define T2_DURATION 0x10

static unsigned long i = 0;
static unsigned long j = 0;

static long shared = 0;

static struct bt_mutex *mutex;

void *thd1()
{
    for (i = 0; i < T1_DURATION; i++) {
        bthread_mutex_lock(mutex);
        shared++;
        bthread_mutex_unlock(mutex);
    }

    return NULL;
}

void *thd2()
{
    int *val = malloc(sizeof(int));
    
    for (j = 0; j < T2_DURATION; j++) {
        bthread_mutex_lock(mutex);
        shared--;
        bthread_mutex_unlock(mutex);
    }
        
    *val = shared;
    return val;
}

int main(/*int argc, char *const argv[]*/)
{
    bthread_t t1, t2;
    mutex = bthread_mutex_init();

    bthread_create(&t1, thd1, NULL);
    bthread_create(&t2, thd2, NULL);
    
    int *t2_ret = NULL;

    bthread_detach(t1);
    bthread_join(t2, (void *)&t2_ret);
    printf("t2 ret: %d\n", *t2_ret);

    return 0;
}