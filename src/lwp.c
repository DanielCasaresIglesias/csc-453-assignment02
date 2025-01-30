#include "lwp.h"
#include <stdlib.h>
#include <stdio.h>
#include <ucontext.h>
#include <sys/mman.h>
#include <sys/resource.h>

#define MAX_QUEUE_SIZE 100

static tid_t next_tid = 1;  // Unique thread ID counter
static thread current_thread = NULL;
static scheduler current_sched = NULL;

// Simple queue to manage blocked threads
typedef struct thread_queue {
    thread *threads;  // Array or list of threads
    int front;        // Index of the front of the queue
    int rear;         // Index of the rear of the queue
    int capacity;     // Maximum capacity of the queue
    int size;         // Current size of the queue
} thread_queue;

int queue_empty(thread_queue *queue) {
    return (queue == NULL || queue->size == 0);
}

// Initialize the waiting queue
thread_queue waiting_queue = {NULL, 0, 0, MAX_QUEUE_SIZE, 0};


typedef struct thread_node {
    thread t;
    struct thread_node *next, *prev;
} thread_node;

static thread_node *head = NULL; // Circular queue head
static int queue_length = 0;

// Round Robin Scheduler Functions
void rr_admit(thread new) {
    thread_node *node = malloc(sizeof(thread_node));
    if (!node) return;
    node->t = new;
    
    if (!head) {  // First thread
        node->next = node->prev = node;
        head = node;
    } else {  // Insert at end
        thread_node *tail = head->prev;
        tail->next = node;
        node->prev = tail;
        node->next = head;
        head->prev = node;
    }
    queue_length++;
}


void rr_remove(thread victim) {
    if (!head) return;

    thread_node *current = head;
    do {
        if (current->t == victim) {
            if (current == head && current->next == head) {
                head = NULL;  // Only one thread in the queue
            } else {
                current->prev->next = current->next;
                current->next->prev = current->prev;
                if (head == current) head = current->next;
            }
            free(current);
            queue_length--;
            return;
        }
        current = current->next;
    } while (current != head);
}


thread rr_next(void) {
    if (!head) return NULL;
    head = head->next; // Move to the next thread in the queue
    return head->t;
}


int rr_qlen(void) {
    return queue_length;
}


struct scheduler rr_publish = {
    NULL, NULL, rr_admit, rr_remove, rr_next, rr_qlen
};
scheduler RoundRobin = &rr_publish;


// Add a thread to the waiting queue
void enqueue(thread t) {
    if (!waiting_queue.threads) {
        waiting_queue.threads =  malloc(MAX_QUEUE_SIZE * sizeof(thread));
        if (!waiting_queue.threads) {
            return;
        }
    }
    if (waiting_queue.size < waiting_queue.capacity) {
        waiting_queue.threads[waiting_queue.rear] = t;
        waiting_queue.rear = (waiting_queue.rear + 1) % waiting_queue.capacity;
        waiting_queue.size++;
    }
}

// Remove a thread from the front of the waiting queue
thread dequeue(void) {
    if (waiting_queue.size > 0) {
        thread t = waiting_queue.threads[waiting_queue.front];
        waiting_queue.front = (waiting_queue.front + 1)%waiting_queue.capacity;
        waiting_queue.size--;
        return t;
    }
    return NULL;  // If the queue is empty
}

// Check if the queue is empty
int is_empty(void) {
    return waiting_queue.size == 0;
}

size_t get_stack_size() {
    struct rlimit limit;
    if (getrlimit(RLIMIT_STACK, &limit) == 0) {
        return limit.rlim_cur;  // Soft limit
    }
    return 8 * 1024;  // Fallback to 8KB if getrlimit fails
}


void lwp_wrapper(lwpfun function, void *argument) {
    /* Call the given lwpfunction with the given argument.
        Calls lwp exit() with its return value
    */
    int rval;
    rval=function(argument);
    lwp_exit(rval);
}


// Creates a new lightweight process
// Returns the thread ID or NO_THREAD if creation fails
tid_t lwp_create(lwpfun function, void *argument) {
    /*
       Creates a new thread and admits it to the current scheduler. The
       thread’s resources will consist of a context and stack, both
       initialized so that when the scheduler chooses this thread and its
       context is loaded via swap rfiles) it will run the given function. This
       may be called by any thread.
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
    new_thread->state.rbp = 0;  // Clear base pointer

    // Final cleanup in the wrapper will handle calling the function & exiting
    // Admit the new thread to the scheduler
    if (current_sched) {
        current_sched->admit(new_thread);
    }
    return new_thread->tid;
}

// Yields control to another LWP
void lwp_yield(void) {
    /*
       Yields control to another thread, saving the current thread's context,
       selecting the next thread from the scheduler, restoring its context,
       and returning to it. If no next thread is available, the program exits.
    */

    // Step 1: Save the current thread's context
    if (current_thread != NULL) {
        // Save the current thread's context (i.e., registers, stack pointer)
        swap_rfiles(&current_thread->state, NULL);
    }

    // Step 2: Pick the next thread from the scheduler
    thread next_thread = current_sched->next();
    if (next_thread == NULL) {
        // No threads to run, so terminate the program
        exit(3);
    }

    // Step 3: Restore the next thread's context
    current_thread = next_thread;
    swap_rfiles(NULL, &current_thread->state);
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
        // Initialize current thread if NULL
        current_thread = malloc(sizeof(struct threadinfo_st));
        if (!current_thread) {
            return;
        }
    }

    // Create context for the calling thread without allocating a new stack
    thread current = current_thread;

    // Set up the context as if this thread had just been created
    // The return address should go to lwp_exit() when the thread finishes
    unsigned long *stack_top = (unsigned long *)(current->state.rsp);

    // Set up a fake return address
    stack_top--;  // Space for return address
    *stack_top = (unsigned long) lwp_exit;  // Fake return address

    // Update the stack pointer in the context
    current->state.rsp = (unsigned long)stack_top;

    // Set up the thread's registers
    current->state.rdi = 0;  // No function argument
    current->state.rsi = 0;  // No additional argument

    // We don't deallocate the current stack, just mark it as valid
    current->state.rbp = 0;  // Clear base pointer for simplicity

    // Admit the thread to the scheduler
    if (current_sched) {
        current_sched->admit(current);
    }

    // Yield control to the scheduler
    lwp_yield();
}

// Exits the current LWP
void lwp_exit(int exitval) {
    /*
       Terminates the calling thread. The exit value's low 8 bits are combined
       with the terminated status to create a termination status. The thread
       will yield control to the next runnable thread. The thread's resources
       will be deallocated when it's waited for.
    */
    
    if (current_thread != NULL) {
        // Set the thread's status to terminated, using MKTERMSTAT to combine
        // the status and exit code
        current_thread->status = MKTERMSTAT(LWP_TERM, exitval & 0xFF);
        current_sched->remove(current_thread);

        // Step 1: Yield control to the next thread
        lwp_yield();
    }
}


// Handles waking up blocked threads when a thread exits
void lwp_exit_blocked(thread_queue *waiting_queue) {
    if (waiting_queue == NULL || queue_empty(waiting_queue)) {
        return;  // No blocked threads to wake up
    }

    // Dequeue the first blocked thread
    thread unblocked_thread = dequeue();
    if (unblocked_thread != NULL) {
        current_sched->admit(unblocked_thread);  // Re-add it to the scheduler
    }
}


// Waits for a thread to terminate
tid_t lwp_wait(int *status) {
    // Check if there are terminated threads in the scheduler
    if (current_sched->qlen() > 1) {
        // Find the oldest terminated thread (FIFO order)
        thread terminated_thread = NULL;
          // Assuming next() gives the next thread in the scheduler
        thread temp_thread = current_sched->next();

        while (temp_thread != NULL) {
            if (temp_thread->status == LWP_TERM) {
                terminated_thread = temp_thread;
                break;  // Get the oldest terminated thread (FIFO)
            }
            // Traverse through the scheduler's linked list of threads
            temp_thread = temp_thread->sched_one;
        }

        if (terminated_thread != NULL) {
            // If status is non-NULL, populate it with the termination status
            if (status != NULL) {
                *status = LWPTERMSTAT(terminated_thread->status);
            }

            // Deallocate resources associated with the terminated thread, but
            // don't free the stack of the system thread
            // Assuming tid 1 is the system thread
            if (terminated_thread->tid != 1) {
                free(terminated_thread->stack);
            }

            // Remove the terminated thread from the scheduler
            current_sched->remove(terminated_thread);
            
            // Return the terminated thread's ID
            return terminated_thread->tid;
        }
    }

    // If no terminated threads, block the current thread
    // First, check if there are no more threads that can be terminated
    if (current_sched->qlen() <= 1) {
        return NO_THREAD;  // No more threads to wait for
    }

    // Block the current thread (remove it from the scheduler)
    current_sched->remove(current_thread);

    // Add it to the waiting queue
    enqueue(current_thread);

    // Block until another thread exits
    while (1) {
        // If there are terminated threads, unblock and process
        if (current_sched->qlen() > 1) {
            lwp_exit_blocked(&waiting_queue);
        }
    }
}


// Returns the thread ID of the calling LWP
tid_t lwp_gettid(void) {
    // Check if there is a valid current thread (i.e., an LWP is running)
    if (current_thread != NULL) {
        return current_thread->tid;  // Return the thread ID of the current LWP
    } else {
        return NO_THREAD;  // If there is no current thread, return NO_THREAD
    }
}


// Converts tid to thread structure
thread tid2thread(tid_t tid) {
    // Start with the first thread in the scheduler
    thread temp_thread = current_sched->next();
    
    // Iterate through the scheduler to find the thread with the matching tid
    while (temp_thread != NULL) {
        if (temp_thread->tid == tid) {
            return temp_thread;  // Return the thread if the tid matches
        }
        // Move to the next thread in the scheduler
        temp_thread = temp_thread->sched_one;
    }
    
    return NULL;  // Return NULL if no matching thread is found
}


// Sets a new scheduler
void lwp_set_scheduler(scheduler sched) {
    if (sched == NULL) {
        // If NULL, reset to round-robin scheduler
        current_sched = RoundRobin; 
        return;
    }

    // Otherwise, transfer all threads to the new scheduler
    thread temp_thread = current_sched->next();  // Get the first thread
    while (temp_thread != NULL) {
        // Store the old scheduler for removal
        scheduler old_sched = current_sched;
        // Remove the thread from the old scheduler
        current_sched->remove(temp_thread);
        // Add it to the new scheduler
        sched->admit(temp_thread);
        // Move to the next thread
        temp_thread = old_sched->next();
    }

    // Now set the current scheduler to the new scheduler
    current_sched = sched;
}


// Gets the current scheduler
scheduler lwp_get_scheduler(void) {
    return current_sched;
}
