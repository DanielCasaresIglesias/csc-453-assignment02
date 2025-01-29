#include "lwp.h"
#include <stdlib.h>
#include <stdio.h>
#include <ucontext.h>
#include <sys/mman.h>
#include <sys/resource.h>

static tid_t next_tid = 1;  // Unique thread ID counter
static thread current_thread = NULL;
static scheduler current_sched = NULL;


size_t get_stack_size() {
    struct rlimit limit;
    if (getrlimit(RLIMIT_STACK, &limit) == 0) {
        return limit.rlim_cur;  // Soft limit
    }
    return 8 * 1024;  // Fallback to 8KB if getrlimit fails
}


void lwp_wrapper(lwpfun function, void *argument) {
    function(argument);
    lwp_exit(0);  // Ensure thread exits properly
}


// Creates a new lightweight process
// Returns the thread ID or NO_THREAD if creation fails
tid_t lwp_create(lwpfun function, void *argument) {
    /*
       Creates a new thread and admits it to the current scheduler. The
       thread’s resources will consist of a context and stack, both initialized
       so that when the scheduler chooses this thread and its context is loaded
       via swap rfiles) it will run the given function. This may be called by
       any thread.
    */

    if (!function) {
        return NO_THREAD;
    }
    
    thread new_thread = malloc(sizeof(struct threadinfo_st));
    if (!new_thread) {
        return NO_THREAD;
    }
    
    // Allocate stack
    size_t stack_size = get_stack_size();
    new_thread->stack = mmap(
        NULL,
        stack_size,
        PROT_READ | PROT_WRITE,
        MAP_ANONYMOUS | MAP_PRIVATE,
        -1,
        0
    );
    if (new_thread->stack == MAP_FAILED) {
        free(new_thread);
        return NO_THREAD;
    }

    // Assign thread ID
    new_thread->tid = next_tid++;

    // Align stack pointer to 16-byte boundary
    unsigned long *stack_top = (unsigned long *)(
        new_thread->stack + stack_size / sizeof(unsigned long)
    );
    stack_top--;  // Space for return address
    *stack_top = (unsigned long) lwp_wrapper;  // Fake return address

    // Setup registers
    new_thread->state.rsp = (unsigned long) stack_top;
    new_thread->state.rdi = (unsigned long) function;  // First argument
    new_thread->state.rsi = (unsigned long) argument;  // Second argument
    new_thread->state.rbp = 0;  // Clear base pointer (no local variables for now) TODO

    // The final cleanup in the wrapper will handle calling the function and exiting
    // Admit the new thread to the scheduler
    return new_thread->tid;
}

// Starts the LWP system
void lwp_start(void) {
    /*
        Transform the calling thread into a lightweight process (LWP), and
        yield control to the scheduler to allow the system to run this thread
        as a part of the LWP system.
    */

    // Check if we're already in a valid state (if not, exit)
    if (current_thread == NULL) {
        return;
    }

    // Create context for the calling thread without allocating a new stack
    thread current = current_thread;

    // Set up the context as if this thread had just been created
    // The return address should go to lwp_exit() when the thread finishes
    unsigned long *stack_top = (unsigned long *)(current->state.rsp); // Use the current stack pointer

    // Set up a fake return address (this will be where the thread "returns" when finished)
    stack_top--;  // Space for return address
    *stack_top = (unsigned long) lwp_exit;  // Fake return address (exit when done)

    // Update the stack pointer in the context
    current->state.rsp = (unsigned long)stack_top;

    // Set up the thread's registers (using rdi and rsi for the function and argument)
    current->state.rdi = 0;  // No function argument
    current->state.rsi = 0;  // No additional argument

    // We don't deallocate the current stack, just mark it as valid
    current->state.rbp = 0;  // Clear base pointer for simplicity

    // Admit the thread to the scheduler
    if (current_sched) {
        current_sched->admit(current);
    }

    // Yield control to the scheduler (this function will return when the scheduler decides)
    lwp_yield();
}

// Yields control to another LWP
void lwp_yield(void) {
}

// Exits the current LWP
void lwp_exit(int exitval) {
    
}

// Waits for a thread to terminate
tid_t lwp_wait(int *status) {
    
}

// Returns the thread ID of the calling LWP
tid_t lwp_gettid(void) {
    
}

// Converts tid to thread structure
thread tid2thread(tid_t tid) {
    
}

// Sets a new scheduler
void lwp_set_scheduler(scheduler sched) {
}

// Gets the current scheduler
scheduler lwp_get_scheduler(void) {

}
