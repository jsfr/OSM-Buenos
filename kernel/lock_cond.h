#ifndef BUENOS_KERNEL_LOCK_COND_H
#define BUENOS_KERNEL_LOCK_COND_H

#include "lib/types.h"
#include "kernel/spinlock.h"
#include "kernel/config.h"
#include "kernel/thread.h"

typedef struct lock_t {
    // 1 is free, 0 is taken. Any other value is an error
    int taken;
    TID_t tid;
    spinlock_t slock;
} lock_t;

typedef int cond_t;

/* Initialize an already allocated lock_t structure such that it can be acquired 
 * and released afterwards. The function should return 0 on success and a
 * negative number on failure. */
int lock_reset(lock_t *lock);

/* Acquire the lock. Use the sleep queue to let kernel threads wait. */
void lock_acquire(lock_t *lock);

/* Releases the lock. */
void lock_release(lock_t *lock);

/* Init a condition  */
void condition_init(cond_t *cond);

/* Wait until condition is true */
void condition_wait(cond_t *cond, lock_t *lock);

/* Signals to first waiting (if any) that a condition is true */
void condition_signal(cond_t *cond);

/* Signals to all waiting (if any) that a condition is true */
void condition_broadcast(cond_t *cond);

#endif
