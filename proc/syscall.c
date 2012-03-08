/*
 * System calls.
 *
 * Copyright (C) 2003 Juha Aatrokoski, Timo Lilja, Leena Salmela,
 *   Teemu Takanen, Aleksi Virtanen, Troels Henriksen.
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
#include "drivers/device.h"
#include "drivers/gcd.h"
#include "kernel/interrupt.h"
#include "kernel/cswitch.h"
#include "proc/syscall.h"
#include "kernel/halt.h"
#include "kernel/panic.h"
#include "lib/libc.h"
#include "kernel/assert.h"
#include "proc/process.h"
#include "fs/vfs.h"

void syscall_exit(int retval)
{
    process_finish(retval);
}

uint32_t syscall_write(uint32_t fd, char* s, int len)
{
    int count;
    gcd_t *gcd;
    device_t *dev;
    if (fd == FILEHANDLE_STDOUT) {
        dev = device_get(YAMS_TYPECODE_TTY, 0);
        KERNEL_ASSERT(dev != NULL);
        KERNEL_ASSERT(dev->generic_device != NULL);
        gcd = (gcd_t *)dev->generic_device;
        count = gcd->write(gcd, s, len);
        return count;
    } else {
     return vfs_write(fd, s, len);
    }
}

uint32_t syscall_read(uint32_t fd, char* s, int len)
{
    int count = 0;
    gcd_t *gcd;
    device_t *dev;
    if (fd == FILEHANDLE_STDIN) {
        dev = device_get(YAMS_TYPECODE_TTY, 0);
        KERNEL_ASSERT(dev != NULL);
        KERNEL_ASSERT(dev->generic_device != NULL);
        gcd = (gcd_t *)dev->generic_device;
        count = gcd->read(gcd, s, len);
        return count;
    } else {
        return vfs_read(fd, s, len);
    }
}

uint32_t syscall_join(process_id_t pid)
{
    return process_join(pid);
}

process_id_t syscall_exec(char* filename)
{
    process_id_t child = process_spawn(filename);
    return child;
}

int syscall_fork(void (*func)(int), int arg)
{
    if (process_fork(func, arg) >= 0) {
        return 0;
    } else {
        return -1;
    }
}

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
    switch(user_context->cpu_regs[MIPS_REGISTER_A0]) {
    case SYSCALL_HALT:
        halt_kernel();
        break;
    case SYSCALL_EXIT:
        syscall_exit(user_context->cpu_regs[MIPS_REGISTER_A1]);
        break;
    case SYSCALL_WRITE:
        user_context->cpu_regs[MIPS_REGISTER_V0] =
            syscall_write(user_context->cpu_regs[MIPS_REGISTER_A1],
                          (char*)user_context->cpu_regs[MIPS_REGISTER_A2],
                          (user_context->cpu_regs[MIPS_REGISTER_A3]));
        break;
    case SYSCALL_READ:
        user_context->cpu_regs[MIPS_REGISTER_V0] =
            syscall_read(user_context->cpu_regs[MIPS_REGISTER_A1],
                         (char*)user_context->cpu_regs[MIPS_REGISTER_A2],
                         (user_context->cpu_regs[MIPS_REGISTER_A3]));
        break;
    case SYSCALL_JOIN:
        user_context->cpu_regs[MIPS_REGISTER_V0] =
          syscall_join(user_context->cpu_regs[MIPS_REGISTER_A1]);
        break;
    case SYSCALL_EXEC:
        user_context->cpu_regs[MIPS_REGISTER_V0] =
            syscall_exec((char*)user_context->cpu_regs[MIPS_REGISTER_A1]);
        break;
    case SYSCALL_FORK:
        user_context->cpu_regs[MIPS_REGISTER_V0] =
            syscall_fork((void (*)(int))user_context->cpu_regs[MIPS_REGISTER_A1],
                         user_context->cpu_regs[MIPS_REGISTER_A2]);
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
