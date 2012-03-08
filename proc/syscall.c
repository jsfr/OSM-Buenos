/*
 * System calls.
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
 * $Id: syscall.c,v 1.3 2004/01/13 11:10:05 ttakanen Exp $
 *
 */
#include "kernel/cswitch.h"
#include "proc/syscall.h"
#include "kernel/halt.h"
#include "kernel/panic.h"
#include "lib/libc.h"
#include "kernel/assert.h"
#include "proc/process.h"
#include "vm/vm.h"
#include "kernel/thread.h"
#include "drivers/device.h"
#include "drivers/gcd.h"
#include "fs/vfs.h"

/**
 * Handle system calls. Interrupts are enabled when this function is
 * called.
 *
 * @param user_context The userland context (CPU registers as they
 * where when system call instruction was called in userland)
 */
void syscall_handle(context_t *user_context)
{
    /* When a syscall is executed in userland, register a0 contains
     * the number of the syscall. Registers a1, a2 and a3 contain the
     * arguments of the syscall. The userland code expects that after
     * returning from the syscall instruction the return value of the
     * syscall is found in register v0. Before entering this function
     * the userland context has been saved to user_context and after
     * returning from this function the userland context will be
     * restored from user_context.
     */
    thread_table_t *thr;
    process_id_t pid;
    device_t *dev;
    gcd_t *gcd;
    char *wbuffer, *rbuffer;
    int wlen, rlen, rlen2;


    switch(user_context->cpu_regs[MIPS_REGISTER_A0]) {
    case SYSCALL_HALT:
        halt_kernel();
        break;

    case SYSCALL_EXEC:
        pid = process_spawn((char*)user_context->cpu_regs[MIPS_REGISTER_A1]);
        if (pid < 0) KERNEL_PANIC("Out of processes\n");
        user_context->cpu_regs[MIPS_REGISTER_V0] = pid;
        break;

    case SYSCALL_EXIT:
        thr = thread_get_current_thread_entry();
        process_finish(user_context->cpu_regs[MIPS_REGISTER_A1]);
        vm_destroy_pagetable(thr->pagetable);
        thr->pagetable = NULL;
        thread_finish();
        break;

    case SYSCALL_JOIN:
        user_context->cpu_regs[MIPS_REGISTER_V0] = process_join(user_context->cpu_regs[MIPS_REGISTER_A1]);
        break;

    case SYSCALL_READ:
        rbuffer = (char*) user_context->cpu_regs[MIPS_REGISTER_A2];
        rlen = user_context->cpu_regs[MIPS_REGISTER_A3];

        if (user_context->cpu_regs[MIPS_REGISTER_A1] == FILEHANDLE_STDIN) {
            dev = device_get(YAMS_TYPECODE_TTY, 0);
            KERNEL_ASSERT(dev != NULL);
            gcd = (gcd_t*)dev->generic_device;
            KERNEL_ASSERT(gcd != NULL);

            rlen2 = gcd->read(gcd,rbuffer,rlen);
            
            KERNEL_ASSERT(rlen2 >= 0);
            rbuffer[rlen2 + 1] = '\0';
            user_context->cpu_regs[MIPS_REGISTER_V0] = rlen2;
        } else {
            user_context->cpu_regs[MIPS_REGISTER_V0] = 
                vfs_read(user_context->cpu_regs[MIPS_REGISTER_A1],rbuffer,rlen);
        }
        break;

    case SYSCALL_WRITE:
        wbuffer = (char*) user_context->cpu_regs[MIPS_REGISTER_A2];
        wlen = user_context->cpu_regs[MIPS_REGISTER_A3];

        if (user_context->cpu_regs[MIPS_REGISTER_A1] == FILEHANDLE_STDOUT) {
            /* Find system console (first tty) */
            dev = device_get(YAMS_TYPECODE_TTY, 0);
            gcd = (gcd_t *)dev->generic_device;
            KERNEL_ASSERT(gcd != NULL);

            gcd->write(gcd, wbuffer, wlen);
            user_context->cpu_regs[MIPS_REGISTER_V0] = wlen;
        } else {
            user_context->cpu_regs[MIPS_REGISTER_V0] = 
                vfs_write(user_context->cpu_regs[MIPS_REGISTER_A1],wbuffer,wlen);
        }
        break;

    case SYSCALL_SEEK:
        user_context->cpu_regs[MIPS_REGISTER_V0] =
            vfs_seek(user_context->cpu_regs[MIPS_REGISTER_A1],
                     user_context->cpu_regs[MIPS_REGISTER_A2]);
        break;
        
    case SYSCALL_OPEN:
        user_context->cpu_regs[MIPS_REGISTER_V0] =
            vfs_open((char*) user_context->cpu_regs[MIPS_REGISTER_A1]);
        break;
        
    case SYSCALL_CLOSE:
        user_context->cpu_regs[MIPS_REGISTER_V0] =
            vfs_close(user_context->cpu_regs[MIPS_REGISTER_A1]);
        break;

    case SYSCALL_CREATE:
        user_context->cpu_regs[MIPS_REGISTER_V0] =
            vfs_create((char*) user_context->cpu_regs[MIPS_REGISTER_A1],
                       user_context->cpu_regs[MIPS_REGISTER_A2]);
        break;

    case SYSCALL_DELETE:
        user_context->cpu_regs[MIPS_REGISTER_V0] =
            vfs_remove((char*) user_context->cpu_regs[MIPS_REGISTER_A1]);
        break;

    default:
        KERNEL_PANIC("Unhandled system call\n");
    }

    /* Move to next instruction after system call */
    user_context->pc += 4;
}
