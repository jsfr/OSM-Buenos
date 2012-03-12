#include "tests/lib.h"

char* substring(char *in, char *out, int start, int stop);
void execute(char *in);

/* The main loop of the shell */
int main() {
    puts("Hello Dave\n");
    char in[255];
    int len = 0;
    while(1) {
        puts("[shell]$ ");
        len = readline(in,255);
        execute(in);
        len = len;
    }
    return 0;
}

/* Takes a substring from in
 * from start to stop and stores it in out */
char *substring(char *in, char *out, int start, int stop) {
    int i;
    for(i = 0 ; i <= stop - start ; i++) {
        out[i] = in[start+i];
    }
    out[i] = '\0';
    return out;
}

/* Checks wheter the command is known and if so executes it */
void execute(char *in) {
    int cmdend = 0, arg0end = 0, arg1end = 0;
    int step = 0;

    for (int i = 0 ; i <= (int)strlen(in) ; i++) {
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
                puts("Unknown number of parameters/commands\n");
                return;
            }
        }
    }

    char cmd[10], arg0[50], arg1[50];
    substring(in, cmd, 0, cmdend);
    substring(in, arg0, cmdend+2, arg0end);
    substring(in, arg1, arg0end+2, arg1end);

    puts("Doing: "); puts(cmd); putc('\n');
    if (!strcmp(cmd,"ls")) {
        char *buffer[100];
        int read = syscall_list(arg0, buffer, 100);
        puts("SYSCALL DONE\n");

        for (int i = 0 ; i < read ; i++) {
            puts("File ");
            puts(buffer[i]);
            putc('\n');
        }
    }
    if (!strcmp(cmd,"cp")) {
        // do cp action.
    }
    if (!strcmp(cmd,"show")) {
        // do show action.
        int handle = syscall_open(arg0);
        if( handle > -1) {
            char buffer[255];
            int res = syscall_read(handle,buffer,255);
            puts(buffer);
            while (res == 255) {
                res = syscall_read(handle,buffer,255);
                puts(buffer);
            }
            puts(buffer);
            putc('\n');
        }
        else {
            puts("Error while reading file: ");
            puts(arg0);
            putc('\n');
        }
        syscall_close(handle);
        return;

    }
    if (!strcmp(cmd,"rm")) {
        // do rm action.
        int res = syscall_delete(arg0);
        if(res == 0) {
            return;
        } else {
            puts("Could not delete file: ");
            puts(arg0);
            putc('\n');
            return;
        }
    }
    if (!strcmp(cmd,"touch")) {
        // do touch action.
        int res = syscall_create(arg0,0);
        if(res == 0) {
            return;
        } else {
            puts("Could not create file: ");
            puts(arg0);
            putc('\n');
            return;
        }

    }
    if (!strcmp(cmd,"exit")) {
        puts("I've turned off the oxygen, Dave.\n");
        syscall_exit(0);
    }

    return;
}
