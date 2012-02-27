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
    char buffer[user_context->cpu_regs[MIPS_REGISTER_A3] + 1];
    int len;


    switch(user_context->cpu_regs[MIPS_REGISTER_A0]) {
    case SYSCALL_HALT:
        halt_kernel();
        break;
    case SYSCALL_EXEC:
        pid = process_spawn((char*)user_context->cpu_regs[MIPS_REGISTER_A1]);
        if (pid < 0) KERNEL_PANIC("Out of processes\n");
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
        dev = device_get(YAMS_TYPECODE_TTY, 0);
        KERNEL_ASSERT(dev != NULL);
        gcd = (gcd_t*)dev->generic_device;
        KERNEL_ASSERT(gcd != NULL);
        break;
    case SYSCALL_WRITE:
        /* Find system console (first tty) */
        kwrite("getting device\n");
        dev = device_get(YAMS_TYPECODE_TTY, 0);
        KERNEL_ASSERT(dev != NULL);
        kwrite("Getting generic device\n");
        gcd = (gcd_t *)dev->generic_device;
        KERNEL_ASSERT(gcd != NULL);
        kwrite("Calling snprintf\n");
        len = snprintf(buffer, user_context->cpu_regs[MIPS_REGISTER_A3], (char*)user_context->cpu_regs[MIPS_REGISTER_A2]);
        kwrite("Writing!\n");
        gcd->write(gcd, buffer, len);
        kwrite("Breaking!\n");
        break;
    default:
        KERNEL_PANIC("Unhandled system call\n");
    }

    /* Move to next instruction after system call */
    user_context->pc += 4;
}
