#include "kernel/lock_cond.h"
#include "kernel/interrupt.h"
#include "kernel/sleepq.h"
#include "kernel/config.h"

/* Initialize an already allocated lock t structure such that it can be acquired 
 * and released afterwards. The function should return 0 on success and a
 * negative number on failure. */
int lock_reset(lock_t *lock) {
    interrupt_status_t intr_status;
    int val;

    intr_status = _interrupt_disable();
    spinlock_acquire(&lock->slock);

    lock->tid = -1;

    if((lock->taken = 0) == 0) {
        val = 0;
    } else {
        val = -1;
    }

    spinlock_release(&lock->slock);
    _interrupt_set_state(intr_status);

    return val;
}

/* Acquire the lock. Use the sleep queue to let kernel threads wait. */
void lock_acquire(lock_t *lock) {
    interrupt_status_t intr_status;

    while (1) {
        intr_status = _interrupt_disable();
        spinlock_acquire(&lock->slock);

        if (lock->taken == 0) break;

        sleepq_add(lock);
        spinlock_release(&lock->slock);
        _interrupt_set_state(intr_status);
        thread_switch();
    }

    lock->taken = 1;
    lock->tid = thread_get_current_thread();
    spinlock_release(&lock->slock);
    _interrupt_set_state(intr_status);
}

/* Releases the lock. */
void lock_release(lock_t *lock) {
    interrupt_status_t intr_status;

    intr_status = _interrupt_disable();
    spinlock_acquire(&lock->slock);

    if (thread_get_current_thread() == lock->tid) {
        lock->taken = 0;
        sleepq_wake(lock);
    }

    spinlock_release(&lock->slock);
    _interrupt_set_state(intr_status);
}

/* Init a condition  */
void condition_init(cond_t *cond) {
    *cond = 0;
}
/* Wait until condition is true, based on a lock. It is assumed, that the lock
 * has been acquired, and that interrupts have been disabled. */
void condition_wait(cond_t *cond, lock_t *lock) {
    sleepq_add(cond);
    lock_release(lock);
    thread_switch();
    lock_acquire(lock);
}

/* wakes first in sleepq. It is assumed that interrupts have been disabled  */
void condition_signal(cond_t *cond) {
    sleepq_wake(cond);
}

/* wakes all. It is assumed that interrupts have been disabled  */
void condition_broadcast(cond_t *cond) {
    sleepq_wake_all(cond);
}
