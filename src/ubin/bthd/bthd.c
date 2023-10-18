#include <stdio.h>
#include <sys/types.h>
#define IMPL_TESTS
#include <bthread.h>
#include <unistd.h>
#include <stdlib.h>

int main(/*int argc, char *const argv[]*/)
{
    round_robin_test();
    //stk_test();
    // pid_t p = fork();
    // if (p < 0) {
    //     puts("fork fail");
    //     exit(1);
    // } else if (p == 0) {
    //     printf("child static var: %d\n", get_static_var());
    // } else {
    //     set_static_var(420);
    //     printf("parent static var: %d\n", get_static_var());
    // }
    return 0;
}