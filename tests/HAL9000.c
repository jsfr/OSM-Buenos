#include "tests/lib.h"

void execute(char *in) {
    char *cmd, *arg0, *arg1;
    int cmdi = 0, arg0i = 0
    for(int i = 0; in[i] != '\n'; i++) {
        if (!cmdi) {
            if (in[i] == ' ') {
                cmdi = 1;
            } else {
                *cmd++ = in[i];
            }
        } else if (!arg0i) {
            if (in[i] == ' ') {
                arg0i = 1;
            } else {
                *arg0++ = in[i];
            }
        } else {
            *arg1i++ = in[i];
        }
    }
}

int main() {
    puts("Hello Dave.\n");
    char *in;
    int len = 0;
    len = readline(in,255);

    execute(in);

    return 0;
}
