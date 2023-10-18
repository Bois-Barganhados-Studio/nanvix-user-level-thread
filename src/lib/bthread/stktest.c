#define IMPL_TESTS
#include <bthread.h>
#include <stdlib.h>
#include <stdio.h>

static void *foo(void *arr_size)
{
    int *arr = malloc(sizeof(int) * *(int *)arr_size);
    for (int i = 0; i < *(int *)arr_size; i++) {
        arr[i] = i ^ 0x1234;
        printf("arr[%d]: %d\n", i, arr[i]);
    }
    return arr;
}

static void create_thdstk(void *(*func)(void *), void *farg, void **fret)
{
    unsigned long *thd_stack = malloc(BTHREAD_STACK_SIZE);
    __asm__ volatile (
        "movl %%ebp, 1016(%%eax)\n\t"
        "movl %%esp, 1020(%%eax)\n\t"
        "lea 1020(%%eax), %%esp\n\t"
        "lea 1020(%%eax), %%ebp\n\t"
        "subl $4, %%esp\n\t"
        "push %%ecx\n\t"
        "call *%%edx\n\t"
        "addl $4, %%esp\n\t"
        "pop %%ebp\n\t"
        "movl 0(%%esp), %%ecx\n\t"
        "movl %%ecx, %%esp\n\t"
        : "=a" (*fret)
        : "a" (thd_stack),
          "c" (farg),
          "d" (func)
    );
    free(thd_stack);
}

extern void stk_test()
{
    int *arr_len = malloc(sizeof(int));
    *arr_len = 100;
    int *arr = NULL;
    create_thdstk(foo, arr_len, (void **)&arr);
    free(arr_len);
    printf("arr[%d]: %d\n", 0, arr[0]);
    free(arr);
}