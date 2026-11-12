/*
 * Copyright (C) 2026 The AROS Dev Team
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef KERN_NETDIAG_H
#define KERN_NETDIAG_H

/*
 * Counters that say where the stack's task is and what it has been doing,
 * kept so that a stack that stops serving its signals can be read from
 * outside without its help: a program reads them in place, with plain
 * memory reads, while the stack is stuck.
 *
 * Found two ways, both plain addresses:
 *  - FindKernelVar("_netdiag") (miami.library), with "_spl_semaphore",
 *    "_spl_level", "_syscall_semaphore" and "_netisr" (api/amiga_kernvars.c);
 *  - a public message port named NETDIAG_PORTNAME, which is the first
 *    member of the structure, found with FindPort() under Forbid(). It is
 *    there from the start of main() until the stack has freed everything,
 *    so a stack that stops while starting - before bsdsocket.library can be
 *    opened - still says how far it got (init_stage). It ignores messages
 *    (PA_IGNORE); nobody is meant to send it any.
 *
 * Every field is written with one plain store or increment, by whichever
 * task is in the stack at the time; nothing here takes a lock. A reader may
 * see a count that is one behind; it never needs more than that. Only the
 * stack writes; a reader checks nd_size and nd_version before it trusts the
 * layout. All counters are 32 bits and wrap.
 *
 * The reading of the fields when the stack has stopped is in
 * DEFECT3-ANALYSIS.md ("Instrumentation as built") of the modularisation
 * notes; in short with each field below.
 */

#include <exec/ports.h>
#include <exec/semaphores.h>

#define NETDIAG_PORTNAME    "AROSTCP.diag"
#define NETDIAG_VERSION     1

/* main loop: ml_where - the step the stack's task is in or last entered */
#define NDW_WAIT        0   /* in (or about to enter) the main Wait() */
#define NDW_SANA        1   /* sana_poll(): the interfaces and the input queues */
#define NDW_TIMER       2   /* timer_poll() */
#define NDW_CONTROL     3   /* control_poll(): reload, modules, shutdown requests */
#define NDW_STOP        4   /* handling a break or a shutdown request */
#define NDW_OUTSIDE     5   /* not in the loop: starting or ended */

/* main loop: ml_sig[] - how many times each signal was taken, by Wait() or SetSignal() */
#define NDS_TIMER       0
#define NDS_SANA        1
#define NDS_CONTROL     2
#define NDS_BREAK       3
#define NDS_MAX         4

/*
 * tm_last[]: which request each slot is. The periodic calls of the modules
 * (netperiodic_add()) follow, in the order of their slots.
 */
#define NDT_FAST        0   /* protocols' fast timeout, 50 ms */
#define NDT_SLOW        1   /* protocols' slow timeout, 500 ms */
#define NDT_IF          2   /* interfaces' timeout, 1 s */
#define NDT_DEFER       3   /* the name database's deferred work */
#define NDT_DEADLINE    4   /* the TCP deadline request */
#define NDT_PERIODIC    5   /* first periodic call of a module */
#define NETDIAG_TIMERS  13  /* NDT_PERIODIC + kern/amiga_time.c PERIODIC_MAX */

/* init_stage: the last step of the start that has completed */
#define NDI_MAIN        1   /* main() entered, this port up */
#define NDI_LOCKS       2   /* malloc, spl and sleep queues */
#define NDI_CONFIG      3   /* readconfig() */
#define NDI_LOG         4   /* log_init(): NETTRACE and its ARexx port "AROSTCP" */
#define NDI_MBUF        5   /* mbinit() */
#define NDI_TIMER       6   /* timer_init() */
#define NDI_API         7   /* api_init(): the libraries exist, still hidden */
#define NDI_NETCORE     8   /* netcore_init() */
#define NDI_SANA        9   /* sana_init() */
#define NDI_DOMAINS     10  /* domaininit(): the protocol modules started */
#define NDI_PFIL        11  /* pfil_init() */
#define NDI_LOOPBACK    12  /* loconfig() */
#define NDI_NETDB       13  /* init_netdb() */
#define NDI_MODULES     14  /* the modules MODULES= names */
#define NDI_CTRLPORT    15  /* the control port (made or not) */
#define NDI_SHOWN       16  /* api_show(): bsdsocket.library can be opened */
#define NDI_RC          17  /* rc_start() */
#define NDI_DONE        18  /* init_all() returned success */
#define NDI_RUNNING     19  /* the timers sent, the main loop runs */
#define NDI_STOPPING    20  /* deinit_all() entered */

#define NETDIAG_PANICLEN 80

struct netdiag {
    struct MsgPort  nd_port;        /* public, NETDIAG_PORTNAME; must stay first */
    UWORD           nd_size;        /* sizeof(struct netdiag) */
    UWORD           nd_version;     /* NETDIAG_VERSION */

    /* what a reader needs and cannot otherwise find without the library */
    struct Task    *nd_task;        /* the stack's task (AROSTCP_Task) */
    struct SignalSemaphore *nd_spl_semaphore;
    struct SignalSemaphore *nd_syscall_semaphore;
    int            *nd_spl_level;

    ULONG   init_stage;             /* NDI_xxx */

    /* the main loop (kern/amiga_main.c) */
    ULONG   ml_where;               /* NDW_xxx */
    ULONG   ml_iter;                /* returns from the main Wait() */
    ULONG   ml_wokeby;              /* the signals the last Wait() returned */
    ULONG   ml_sig[NDS_MAX];        /* each signal taken, by Wait() or SetSignal() */

    /* the timer requests (kern/amiga_time.c) */
    ULONG   tm_reqs;                /* requests in use: 5 and the modules' periodic ones */
    ULONG   tm_sent;                /* SendIO()s */
    ULONG   tm_replied;             /* replies taken off the port and handled */
    ULONG   tm_collected;           /* aborted and taken back with WaitIO() */
    ULONG   tm_empty;               /* timer_poll() passes that found no reply */
    ULONG   tm_last[NETDIAG_TIMERS];/* ml_iter at each request's last reply */

    /* the deadline request (kern/amiga_time.c) */
    ULONG   dl_want;                /* a deadline earlier than the request's asked for */
    ULONG   dl_signal;              /* ... by another task, which signalled the stack's */
    ULONG   dl_forward;             /* request aborted and sent again for the earlier one */
    ULONG   dl_back;                /* ... found already back: left to fire */
    ULONG   dl_fire;                /* the deadline request fired */

    /* the input queues (net/netisr.c) */
    ULONG   nq_sched;               /* schednetisr(): queued and the stack's task signalled */
    ULONG   nq_sched_quiet;         /* schednetisr_nosignal() */
    ULONG   nq_run;                 /* net_poll() passes that found work */

    /* the stack's task in tsleep() (kern/kern_synch.c): must never happen */
    ULONG   tsleep_stack;
    const char *tsleep_wmesg;       /* its reason, of the last one */

    /* panic() (kern/subr_prf.c) */
    ULONG   panics;
    char    panic_text[NETDIAG_PANICLEN];   /* the first one's message */
};

extern struct netdiag netdiag;

#endif /* KERN_NETDIAG_H */
