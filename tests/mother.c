#include "tests/lib.h"

int main(void)
{
    int child = syscall_exec("[fyamsHDD]kidd");
    int childret = syscall_join(child);
    if (childret == 42) {
        puts("Child returned the right value! hooray!\n");
    } else {
        puts("Child returned wrong value, noooooes!\n");
    }
    return 0;
}
