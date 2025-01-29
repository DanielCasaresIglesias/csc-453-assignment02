#ifndef LWPH
#define LWPH
#include <sys/types.h>
#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif
#if defined( x86 64)
#include <fp.h>
typedef struct attribute ((aligned(16))) attribute ((packed))
registers {
    unsigned long rax; /* the sixteen architecturally–visible regs. */
    unsigned long rbx;
    unsigned long rcx;
    unsigned long rdx;
    unsigned long rsi;
    unsigned long rdi;
    unsigned long rbp;
    unsigned long rsp;
    unsigned long r8;
    unsigned long r9;
    unsigned long r10;
    unsigned long r11;
    unsigned long r12;
    unsigned long r13;
    unsigned long r14;
    unsigned long r15;
    struct fxsave fxsave; /* space to save floating point state */
} rfile;
#else
#error "This only works on x86_64 for now"
#endif
typedef unsigned long tid t;
#define NO THREAD 0 /* an always invalid thread id */
typedef struct threadinfo st *thread;
typedef struct threadinfo st {
    tid t tid; /* lightweight process id */
    unsigned long *stack; /* Base of allocated stack */
    size t stacksize; /* Size of allocated stack */

    rfile state; /* saved registers */
    unsigned int status; /* exited? exit status? */
    thread lib one; /* Two pointers reserved */
    thread lib two; /* for use by the library */
    thread sched one; /* Two more for */
    thread sched two; /* schedulers to use */
    thread exited; /* and one for lwp wait() */
} context;
typedef int (*lwpfun)(void *); /* type for lwp function */
/* Tuple that describes a scheduler */
typedef struct scheduler {
    void (*init)(void); /* initialize any structures */
    void (*shutdown)(void); /* tear down any structures */
    void (*admit)(thread new); /* add a thread to the pool */
    void (*remove)(thread victim); /* remove a thread from the pool */
    thread (*next)(void); /* select a thread to schedule */
    int (*qlen)(void); /* number of ready threads */
} *scheduler;
/* lwp functions */
extern tid t lwp create(lwpfun,void *);
extern void lwp exit(int status);
extern tid t lwp gettid(void);
extern void lwp yield(void);
extern void lwp start(void);
extern void lwp stop(void);
extern tid t lwp wait(int *);
extern void lwp set scheduler(scheduler fun);
extern scheduler lwp get scheduler(void);
extern thread tid2thread(tid t tid);
/* for lwp wait */
#define TERMOFFSET 8
#define MKTERMSTAT(a,b) ( (a)<<TERMOFFSET | ((b) & ((1<<TERMOFFSET)−1)) )
#define LWP TERM 1
#define LWP LIVE 0
#define LWPTERMINATED(s) ( (((s)>>TERMOFFSET)&LWP TERM) == LWP TERM )
#define LWPTERMSTAT(s) ( (s) & ((1<<TERMOFFSET)−1) )
/* prototypes for asm functions */
void swap rfiles(rfile *old, rfile *new);

#endif