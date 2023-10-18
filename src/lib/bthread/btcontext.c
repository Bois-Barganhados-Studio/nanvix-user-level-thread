#include <bthread.h>

typedef unsigned long bcontext_t[9];

//static bcontext_t ctxtab[BTHREAD_THREADS_MAX];

static unsigned long var = 15;

void set_static_var(unsigned long val)
{
    var = val;
}

unsigned long get_static_var()
{
    return var;
}