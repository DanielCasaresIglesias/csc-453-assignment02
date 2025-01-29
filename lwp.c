#include "lwp.h"
#include <stdlib.h>
#include <stdio.h>
#include <ucontext.h>

static tid_t next_tid = 1;  // Unique thread ID counter
static thread current_thread = NULL;
static scheduler current_sched = NULL;

// Creates a new lightweight process
// Returns the thread ID or NO_THREAD if creation fails
tid_t lwp_create(lwpfun function, void *argument) {
    if (!function) return NO_THREAD;
    
    thread new_thread = malloc(sizeof(struct thread));
    if (!new_thread) return NO_THREAD;
    
    new_thread->tid = next_tid++;
    new_thread->stack = mmap(NULL, STACK_SIZE, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    if (new_thread->stack == MAP_FAILED) {
        free(new_thread);
        return NO_THREAD;
    }
    
    getcontext(&new_thread->context);
    new_thread->context.uc_stack.ss_sp = new_thread->stack;
    new_thread->context.uc_stack.ss_size = STACK_SIZE;
    new_thread->context.uc_link = NULL;
    makecontext(&new_thread->context, (void (*)(void))function, 1, argument);
    
    if (current_sched) current_sched->admit(new_thread);
    return new_thread->tid;
}

// Starts the LWP system
void lwp_start(void) {
    if (!current_sched || !(current_thread = current_sched->next())) return;
    swapcontext(&current_thread->context, &current_thread->context);
}

// Yields control to another LWP
void lwp_yield(void) {
    thread prev = current_thread;
    if (current_sched) {
        current_sched->admit(current_thread);
        current_thread = current_sched->next();
    }
    if (current_thread && prev != current_thread)
        swapcontext(&prev->context, &current_thread->context);
}

// Exits the current LWP
void lwp_exit(int exitval) {
    thread to_free = current_thread;
    if (current_sched) current_sched->remove(to_free);
    
    munmap(to_free->stack, STACK_SIZE);
    free(to_free);
    
    current_thread = current_sched ? current_sched->next() : NULL;
    if (current_thread)
        setcontext(&current_thread->context);
    else
        exit(exitval);
}

// Waits for a thread to terminate
tid_t lwp_wait(int *status) {
    // Simple implementation, might need an explicit wait queue
    return NO_THREAD;
}

// Returns the thread ID of the calling LWP
tid_t lwp_gettid(void) {
    return current_thread ? current_thread->tid : NO_THREAD;
}

// Converts tid to thread structure
thread tid2thread(tid_t tid) {
    // Implementation depends on thread storage, assuming scheduler manages threads
    return NULL;
}

// Sets a new scheduler
void lwp_set_scheduler(scheduler sched) {
    current_sched = sched;
}

// Gets the current scheduler
scheduler lwp_get_scheduler(void) {
    return current_sched;
}
