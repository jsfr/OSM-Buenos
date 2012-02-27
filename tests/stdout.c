#include "tests/lib.h"

int main(void)
{
    char buffer[100];
    puts("Martin likes this TTY!\n\n\n");
    readline(buffer, 100);
    printf("[%d] = %s\n",strlen(buffer),buffer);
    syscall_halt();


    return 0;
}
