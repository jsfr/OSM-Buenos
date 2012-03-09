#include "tests/lib.h"

char *substring(char *in, char *out, int start, int stop) {
    int i;
    for(i = 0 ; i <= stop - start ; i++) {
        //puts("Adding: ");
        putc(in[start+i]);
        putc('\n');
        out[i] = in[start+i];
    }
    out[i] = '\0';
    return out;
}

void execute(char *in) {
    int cmdend, arg0end, arg1end;
    int step = 0;
    
    for (int i = 0 ; i <= (int) strlen(in) ; i++) {
        if(in[i] == ' ' || in[i] == '\0') {
            switch (step) {
            case 0:
                cmdend = i-1;
                step++;
                break;

            case 1:
                arg0end = i-1;
                step++;
                break;

            case 2:
                arg1end = i-1;
                step++;
                break;

            default:
                puts("I've turned off the oxygen, Dave.\n");
                syscall_exit(1);
                break;
            }
        }
    }

    char cmd[cmdend];
    substring(in, cmd, 0, cmdend);
    char arg0[arg0end - (cmdend+2)];
    substring(in, arg0, cmdend+2, arg0end);
    char arg1[arg1end - (arg0end+2)];
    substring(in, arg1, arg0end+2, arg1end);
    
    puts(cmd);
    puts(arg0);
    puts(arg1);

    cmdend = arg0end + arg1end;
    arg0end = cmdend;
    return;
}

int main() {
    puts("Hello Dave.\n");
    char in[255];
    int len = 0;
    len = readline(in,255);
    execute(in);

    len = len;
    return 0;
}
