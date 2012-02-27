/*
 * Process startup.
 *
 * Copyright (C) 2003-2005 Juha Aatrokoski, Timo Lilja,
 *       Leena Salmela, Teemu Takanen, Aleksi Virtanen.
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
 * $Id: process.c,v 1.11 2007/03/07 18:12:00 ttakanen Exp $
 *
 */

#include "proc/process.h"
#include "proc/elf.h"
#include "kernel/thread.h"
#include "kernel/assert.h"
#include "kernel/interrupt.h"
#include "kernel/config.h"
#include "fs/vfs.h"
#include "drivers/yams.h"
#include "vm/vm.h"
#include "vm/pagepool.h"
#include "kernel/sleepq.h"

// The process table holds every process
process_table_t process_table;

// Semaphore used for ensuring atomicity in the process functions
semaphore_t process_table_semaphore;

/** @name Process startup
 *
 * This module contains a function to start a userland process.
 */

/**
 * Starts one userland process. The thread calling this function will
 * be used to run the process and will therefore never return from
 * this function. This function asserts that no errors occur in
 * process startup (the executable file exists and is a valid ecoff
 * file, enough memory is available, file operations succeed...).
 * Therefore this function is not suitable to allow startup of
 * arbitrary processes.
 *
 * @executable The name of the executable to be run in the userland
 * process
 */
//void process_start(const char *executable)
void process_start(uint32_t exec)
{
    const char *executable = (char*) exec;
    thread_table_t *my_entry;
    pagetable_t *pagetable;
    uint32_t phys_page;
    context_t user_context;
    uint32_t stack_bottom;
    elf_info_t elf;
    openfile_t file;

    int i;

    interrupt_status_t intr_status;

    my_entry = thread_get_current_thread_entry();

    /* If the pagetable of this thread is not NULL, we are trying to
       run a userland process for a second time in the same thread.
       This is not possible. */
    KERNEL_ASSERT(my_entry->pagetable == NULL);

    pagetable = vm_create_pagetable(thread_get_current_thread());
    KERNEL_ASSERT(pagetable != NULL);

    intr_status = _interrupt_disable();
    my_entry->pagetable = pagetable;
    _interrupt_set_state(intr_status);

    file = vfs_open((char *)executable);
    /* Make sure the file existed and was a valid ELF file */
    KERNEL_ASSERT(file >= 0);
    KERNEL_ASSERT(elf_parse_header(&elf, file));

    /* Trivial and naive sanity check for entry point: */
    KERNEL_ASSERT(elf.entry_point >= PAGE_SIZE);

    /* Calculate the number of pages needed by the whole process
       (including userland stack). Since we don't have proper tlb
       handling code, all these pages must fit into TLB. */
    KERNEL_ASSERT(elf.ro_pages + elf.rw_pages + CONFIG_USERLAND_STACK_SIZE
		  <= _tlb_get_maxindex() + 1);

    /* Allocate and map stack */
    for(i = 0; i < CONFIG_USERLAND_STACK_SIZE; i++) {
        phys_page = pagepool_get_phys_page();
        KERNEL_ASSERT(phys_page != 0);
        vm_map(my_entry->pagetable, phys_page,
               (USERLAND_STACK_TOP & PAGE_SIZE_MASK) - i*PAGE_SIZE, 1);
    }

    /* Allocate and map pages for the segments. We assume that
       segments begin at page boundary. (The linker script in tests
       directory creates this kind of segments) */
    for(i = 0; i < (int)elf.ro_pages; i++) {
        phys_page = pagepool_get_phys_page();
        KERNEL_ASSERT(phys_page != 0);
        vm_map(my_entry->pagetable, phys_page,
               elf.ro_vaddr + i*PAGE_SIZE, 1);
    }

    for(i = 0; i < (int)elf.rw_pages; i++) {
        phys_page = pagepool_get_phys_page();
        KERNEL_ASSERT(phys_page != 0);
        vm_map(my_entry->pagetable, phys_page,
               elf.rw_vaddr + i*PAGE_SIZE, 1);
    }

    /* Put the mapped pages into TLB. Here we again assume that the
       pages fit into the TLB. After writing proper TLB exception
       handling this call should be skipped. */
    intr_status = _interrupt_disable();
    tlb_fill(my_entry->pagetable);
    _interrupt_set_state(intr_status);

    /* Now we may use the virtual addresses of the segments. */

    /* Zero the pages. */
    memoryset((void *)elf.ro_vaddr, 0, elf.ro_pages*PAGE_SIZE);
    memoryset((void *)elf.rw_vaddr, 0, elf.rw_pages*PAGE_SIZE);

    stack_bottom = (USERLAND_STACK_TOP & PAGE_SIZE_MASK) -
        (CONFIG_USERLAND_STACK_SIZE-1)*PAGE_SIZE;
    memoryset((void *)stack_bottom, 0, CONFIG_USERLAND_STACK_SIZE*PAGE_SIZE);

    /* Copy segments */

    if (elf.ro_size > 0) {
	/* Make sure that the segment is in proper place. */
        KERNEL_ASSERT(elf.ro_vaddr >= PAGE_SIZE);
        KERNEL_ASSERT(vfs_seek(file, elf.ro_location) == VFS_OK);
        KERNEL_ASSERT(vfs_read(file, (void *)elf.ro_vaddr, elf.ro_size)
		      == (int)elf.ro_size);
    }

    if (elf.rw_size > 0) {
	/* Make sure that the segment is in proper place. */
        KERNEL_ASSERT(elf.rw_vaddr >= PAGE_SIZE);
        KERNEL_ASSERT(vfs_seek(file, elf.rw_location) == VFS_OK);
        KERNEL_ASSERT(vfs_read(file, (void *)elf.rw_vaddr, elf.rw_size)
		      == (int)elf.rw_size);
    }


    /* Set the dirty bit to zero (read-only) on read-only pages. */
    for(i = 0; i < (int)elf.ro_pages; i++) {
        vm_set_dirty(my_entry->pagetable, elf.ro_vaddr + i*PAGE_SIZE, 0);
    }

    /* Insert page mappings again to TLB to take read-only bits into use */
    intr_status = _interrupt_disable();
    tlb_fill(my_entry->pagetable);
    _interrupt_set_state(intr_status);

    /* Initialize the user context. (Status register is handled by
       thread_goto_userland) */
    memoryset(&user_context, 0, sizeof(user_context));
    user_context.cpu_regs[MIPS_REGISTER_SP] = USERLAND_STACK_TOP;
    user_context.pc = elf.entry_point;

    /* Add the pid to the thread */
    for (int i = 0; i < CONFIG_MAX_PROCESSES; i++) {
        if (process_table.table[i].executable == executable) {
            my_entry->process_id = process_table.table[i].pid;
            break;
        }
    }

    thread_goto_userland(&user_context);

    KERNEL_PANIC("thread_goto_userland failed.");
}

/* Run process in new thread , returns PID of new process */
process_id_t process_spawn( const char *executable ) {
    semaphore_P(&process_table_semaphore);
    TID_t thr;
    for(int i = 0; i < CONFIG_MAX_PROCESSES; i++) {
        if (process_table.table[i].state == PROC_FREE) {
            process_table.table[i].executable = executable;
            process_table.table[i].pid        = process_table.new_pid++;
            process_table.table[i].state      = PROC_RUNNING;
            process_table.table[i].retval     = -1;
            process_table.table[i].parent     = process_get_current_process();
            semaphore_V(&process_table_semaphore);
            thr = thread_create(&process_start, (uint32_t)executable);
            thread_run(thr);
            return process_table.table[i].pid;
        }
    }
    return -1;
}

/* Run process in this thread , only returns if there is an error */
int process_run( const char *executable ) {
    semaphore_P(&process_table_semaphore);
    for(int i = 0; i < CONFIG_MAX_PROCESSES; i++) {
        if (process_table.table[i].state == PROC_FREE) {
            process_table.table[i].executable = executable;
            process_table.table[i].pid        = process_table.new_pid++;
            process_table.table[i].state      = PROC_RUNNING;
            process_table.table[i].retval     = -1;
            process_table.table[i].parent     = -1;
            semaphore_V(&process_table_semaphore);
            process_start((uint32_t)executable);
        }
    }
    return -1;
}

process_id_t process_get_current_process( void ) {
    //semaphore_P(&process_table_semaphore);
    return thread_get_current_thread_entry()->process_id;
    /*
    for(int i = 0; i < CONFIG_MAX_PROCESSES; i++) {
        if (process_table.table[i].state == PROC_RUNNING) {
            semaphore_V(&process_table_semaphore);
            return process_table.table[i].pid;
        }
    }
    semaphore_V(&process_table_semaphore);
    return -1;*/
}

/* Stop the current process and the kernel thread in which it runs */
void process_finish( int retval ) {
    process_id_t pid = thread_get_current_thread_entry()->process_id;
    semaphore_P(&process_table_semaphore);
    for(int i = 0; i < CONFIG_MAX_PROCESSES; i++) {
        if (process_table.table[i].pid == pid) {
            process_table.table[i].state  = PROC_TERMINATED;
            process_table.table[i].retval = retval;
            semaphore_V(&process_table_semaphore);
            sleepq_wake(&process_table.table[i]);
            return;
        }
    }
    semaphore_V(&process_table_semaphore);
}

/* Wait for the given process to terminate , returning its return value,
 * and marking the process table entry as free */
uint32_t process_join( process_id_t pid ) {
    semaphore_P(&process_table_semaphore);
    for(int i = 0; i < CONFIG_MAX_PROCESSES; i++) {
        if (process_table.table[i].pid == pid) {
            while (1) {
                if(process_table.table[i].state == PROC_TERMINATED) {
                    uint32_t retval = process_table.table[i].retval;
                    process_table.table[i].executable = "nil";
                    process_table.table[i].pid        = -1;
                    process_table.table[i].state      = PROC_FREE;
                    process_table.table[i].retval     = -1;
                    process_table.table[i].parent     = -1;
                    semaphore_V(&process_table_semaphore);
                    return retval;
                } else {
                    semaphore_V(&process_table_semaphore);
                    sleepq_add(&process_table.table[i]);
                    semaphore_P(&process_table_semaphore);
                }
            }
        }
    }
    semaphore_V(&process_table_semaphore);
    return 0;
}

/* Initialize process table. Should be called before any other process-related calls */
void process_init ( void ) {
    for (int i = 0; i < CONFIG_MAX_PROCESSES; i++) {
        process_table.table[i].executable = "nil";
        process_table.table[i].pid        = -1;
        process_table.table[i].state      = PROC_FREE;
        process_table.table[i].retval     = -1;
        process_table.table[i].parent     = -1;
    }
    // intitialized to 1 since we use 0 as an error-code for process_join
    process_table.new_pid                 = 1;
    process_table_semaphore               = *semaphore_create(1);
}
/** @} */
