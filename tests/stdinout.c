#include "tests/lib.h"

int main(void)
{
    char buffer[100];
    int len;
    puts("====================\n");
    puts("==syscall_write OK==\n");
    puts("====================\n");
    puts("Enter between 1 and 100 characters:\n");
    len = readline(buffer, 100);
    printf("You entered %d chars: %s\n",len,buffer);
    printf("Shutting down\n");
    syscall_halt();
    return 0;
}
