/*
 * Process startup.
 *
 * Copyright (C) 2003 Juha Aatrokoski, Timo Lilja,
 *   Leena Salmela, Teemu Takanen, Aleksi Virtanen.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials provided
 *    with the distribution.
 * 3. The name of the author may not be used to endorse or promote
 *    products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * $Id: process.h,v 1.4 2003/05/16 10:13:55 ttakanen Exp $
 *
 */

#ifndef BUENOS_PROC_PROCESS
#define BUENOS_PROC_PROCESS

//#include "kernel/thread.h"
#include "lib/types.h"
#include "kernel/cswitch.h"
//#include "kernel/thread.h"

typedef enum {
    PROC_FREE,
    PROC_NEW,
    PROC_READY,
    PROC_RUNNING,
    PROC_WAITING,
    PROC_TERMINATED
} process_state_t;

typedef int TID_t; //TODO bør nok snarere komme fra en include... DILLER!
typedef int process_id_t;

typedef struct process_table_t {
    char *executable;
    process_id_t pid;
    process_state_t state;
    TID_t thread;
} process_table_t;

void process_start(const char *executable);

/* Run process in new thread , returns PID of new process */
process_id_t process_spawn( const char *executable );

/* Run process in this thread , only returns if there is an error */
int process_run( const char *executable ) ;

process_id_t process_get_current_process( void ) ;

/* Stop the current process and the kernel thread in which it runs */
void process_finish( int retval );

/* Wait for the given process to terminate , returning its return value,
 * and marking the process table entry as free */
uint32_t process_join( process_id_t pid ) ;

/* Initialize process table. Should be called before any other process-related calls */
void process_init ( void ) ;


#define USERLAND_STACK_TOP 0x7fffeffc

#endif
