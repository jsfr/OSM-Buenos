#include "tests/lib.h"

/* OLEKS -2: Eksempelprogram mht. TLB exception. */

int main(void)
{
    syscall_exit(27);
    return 0;
}
