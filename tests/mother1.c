#include "tests/lib.h"

int main(void)
{
    int child = syscall_exec("[fyamsHDD]kidd");
    int child2 = syscall_exec("[fyamsHDD]kidd2");
    int childret = syscall_join(child);
    int childret2 = syscall_join(child2);
    if (childret + childret2 == 69) {
        puts("Child returned the right value! hooray!\n");
    } else {
        puts("Child returned wrong value, noooooes!\n");
    }
    return 0;
}
