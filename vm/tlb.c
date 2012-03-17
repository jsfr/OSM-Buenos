/*
 * TLB handling
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
 * $Id: tlb.c,v 1.6 2004/04/16 10:54:29 ttakanen Exp $
 *
 */

#include "kernel/panic.h"
#include "kernel/assert.h"
#include "vm/tlb.h"
#include "vm/pagetable.h"
#include "vm/pagepool.h"
#include "kernel/thread.h"
#include "kernel/interrupt.h"

void tlb_modified_exception(void)
{
    kwrite("modified exception\n");
    tlb_exception_state_t *state = NULL;
    thread_table_t *thread;
    _tlb_get_exception_state(state);
    thread = thread_get_current_thread_entry();
    if(thread->user_context->status & USERLAND_ENABLE_BIT){
        // Acces violation? HOWTO!
        kwrite("Access violation\n");
        thread_finish();
    }    
    KERNEL_PANIC("Unhandled TLB modified exception");
}

void tlb_load_exception(void)
{
    kwrite("is in load\n");
    tlb_exception_state_t state;
    thread_table_t *thread;
    tlb_entry_t *entry;
    uint32_t vpn2;
    uint32_t asid;
    uint32_t evenodd;

    state = state;
    _tlb_get_exception_state(&state);
    thread = thread_get_current_thread_entry();
    vpn2 = state.badvpn2;
    asid = state.asid;
    evenodd = (state.badvaddr>>12) & 1;

    for(int i = 0; i < PAGETABLE_ENTRIES; i++) {
        entry = &(thread->pagetable->entries[i]);
        uint32_t valid = evenodd ? entry->V0 : entry->V1;
        
        if(vpn2 == entry->VPN2 && asid == entry->ASID && valid) {
            int index = _tlb_probe(entry);
            if (index == -1) {
                _tlb_write_random(entry);
                return;
            } else {
                _tlb_write(entry, index, 1);
                return;
            }
        }
    }
    if(thread->user_context->status & USERLAND_ENABLE_BIT){
        // Acces violation? HOWTO!
        kwrite("Access violation\n");
        thread_finish();
        }
    KERNEL_PANIC("Unhandled TLB load exception");
}

void tlb_store_exception(void)
{
    kwrite("is in store\n");
    tlb_exception_state_t *state;
    thread_table_t *thread;
    tlb_entry_t *entry;
    uint32_t vpn2;
    uint32_t asid;
    uint32_t evenodd;

    state = (tlb_exception_state_t*)pagepool_get_phys_page();
    state = (tlb_exception_state_t*)ADDR_PHYS_TO_KERNEL((uint32_t)state);
    
    kprintf("State addr: %d\n",(uint32_t) state);
    _tlb_get_exception_state(state);
    kwrite("DO I LIVE?\n");
    thread = thread_get_current_thread_entry();
    vpn2 = state->badvpn2;
    asid = state->asid;
    evenodd = (state->badvaddr>>12) & 1;
    kprintf("addr: %d\n",asid);

    for(int i = 0; i < PAGETABLE_ENTRIES; i++) {
        entry = &(thread->pagetable->entries[i]);
        uint32_t valid = evenodd ? entry->V1 : entry->V0;
        kprintf("i is in loop------VALID:%d\n",valid);
        kprintf("VPN2: %d\nEntryVPN: %d\n",vpn2,entry->VPN2);
        if(vpn2 == entry->VPN2 && asid == entry->ASID && valid) {
            kwrite("is in if!\n");
            int index = _tlb_probe(entry);
            if (index == -1) {
                _tlb_write_random(entry);
                kprintf("Write random\n");
                return;
            } else {
                _tlb_write(entry, index, 1);
                kprintf("Replacing old\n");
                return;
            }
        }
    }
    if(thread->user_context->status & USERLAND_ENABLE_BIT){
        // Acces violation? HOWTO!
        kwrite("Access violation\n");
        thread_finish();
        }
        KERNEL_PANIC("Unhandled TLB store exception");
}

/**
 * Fill TLB with given pagetable. This function is used to set memory
 * mappings in CP0's TLB before we have a proper TLB handling system.
 * This approach limits the maximum mapping size to 128kB.
 *
 * @param pagetable Mappings to write to TLB.
 *
 */

void tlb_fill(pagetable_t *pagetable)
{
    if(pagetable == NULL)
	return;

    /* Check that the pagetable can fit into TLB. This is needed until
     we have proper VM system, because the whole pagetable must fit
     into TLB. */
    KERNEL_ASSERT(pagetable->valid_count <= (_tlb_get_maxindex()+1));

    _tlb_write(pagetable->entries, 0, pagetable->valid_count);

    /* Set ASID field in Co-Processor 0 to match thread ID so that
       only entries with the ASID of the current thread will match in
       the TLB hardware. */
    _tlb_set_asid(pagetable->ASID);
}
