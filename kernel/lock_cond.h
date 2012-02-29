#ifndef BUENOS_KERNEL_LOCK_COND_H
#define BUENOS_KERNEL_LOCK_COND_H

#include "lib/types.h"
#include "kernel/spinlock.h"
#include "kernel/config.h"
#include "kernel/thread.h"

typedef struct lock_t {
    // 1 is free, 0 is taken. All else is error :'(
    TID_t tid;
    int taken;
    spinlock_t slock;
    // TODO Me... 
} lock_t;

typedef struct cond_t {
    // TODO me...  DO ME! ;D
} cond_t;

/* Initialize an already allocated lock t structure such that it can be acquired 
 * and released afterwards. The function should return 0 on success and a 
 * negative number on failure. */
int lock_reset(lock_t *lock);

/* Acquire the lock. Use the sleep queue to let kernel threads wait. */
void lock_acquire(lock_t *lock);

/* Releases the lock. */
void lock_release(lock_t *lock);

/* Init a condition  */
void condition_init(cond_t *cond);

/* Wait until condition is true, based on a lock */
void condition_wait(cond_t *cond, lock_t *lock);

/* Signals that a condition is true.  */
void condition_signal(cond_t *cond);

/* Same mayhabs?  */
void condition_broadcast(cond_t *cond);


#endif
