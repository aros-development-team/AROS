/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.
*/

#include <sys/wait.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#ifdef __linux__
#include <sys/prctl.h>
#include <signal.h>
#endif

#if defined(__APPLE__) && defined(__aarch64__)
/* Forward-declare pthread to avoid conflict with AROS pthread.h */
typedef struct _opaque_pthread_t *pthread_t_host;
typedef struct _opaque_pthread_attr_t *pthread_attr_t_host;
int pthread_create(pthread_t_host *, const pthread_attr_t_host *, void *(*)(void *), void *);
int pthread_detach(pthread_t_host);
#endif

/* These macros are defined in both UNIX and AROS headers. Get rid of warnings. */
#undef __pure
#undef __const
#undef __pure2
#undef __deprecated

#include <aros/kernel.h>
#include <runtime.h>

#include "kickstart.h"
#include "platform.h"
#include "hostinterface.h"

#if defined(__APPLE__) && defined(__aarch64__)

extern void *cocoa_display_init(int width, int height);
extern int   cocoa_display_get_pitch(void);
extern void  cocoa_runloop_step(void);
extern void  cocoa_set_hiface(void *hiface);

struct kick_args {
    kernel_entry_fun_t addr;
    struct TagItem *msg;
    int result;
};

static void *kernel_thread(void *arg)
{
    struct kick_args *ka = (struct kick_args *)arg;
    fprintf(stderr, "[Bootstrap] Kernel thread started, entering at %p...\n", ka->addr);
    Host_PreBoot();
    ka->result = ka->addr(ka->msg, AROS_BOOT_MAGIC);
    fprintf(stderr, "[Bootstrap] Kernel returned %d\n", ka->result);
    exit(ka->result);
    return NULL;
}

int kick(kernel_entry_fun_t addr, struct TagItem *msg)
{
    struct kick_args ka = { addr, msg, 0 };
    pthread_t_host tid;

    /* Init Cocoa display FIRST (must happen on main thread) */
    {
        extern void *HostIFace;
        struct HostInterface *hi = (struct HostInterface *)HostIFace;
        void *fb = cocoa_display_init(640, 480);
        hi->cocoa_fb_base = fb;
        hi->cocoa_fb_width = 640;
        hi->cocoa_fb_height = 480;
        hi->cocoa_fb_pitch = cocoa_display_get_pitch();
        cocoa_set_hiface(hi);
        fprintf(stderr, "[Bootstrap] Framebuffer at %p, %dx%d pitch=%d\n",
                fb, 640, 480, hi->cocoa_fb_pitch);
    }

    /* Start kernel on background thread */
    fprintf(stderr, "[Bootstrap] Starting kernel on background thread...\n");
    pthread_create(&tid, NULL, kernel_thread, &ka);
    pthread_detach(tid);

    /* Main thread: pump the Cocoa run loop forever */
    for (;;)
        cocoa_runloop_step();

    /* Not reached */
    return ka.result;
}

#else /* !darwin-aarch64 */

#define D(x)

/*
 * This is the UNIX-hosted kicker. Theory of operation:
 * We want to run the loaded code multiple times, every time as a new process (in order
 * to drop all open file descriptors etc).
 * We have already loaded our kickstart and allocated RAM. Now we use fork() to mark
 * the point where we are started.
 * AROS is executed inside child process. The parent just sits and waits for the return
 * code.
 * When AROS shuts down, it sets exit status in order to indicate a reason. There are
 * three status codes:
 * 1. Shutdown
 * 2. Cold reboot.
 * 3. Warm reboot.
 * We pick up this code and see what we need to do. Warm reboot means just creating a
 * new AROS process using the same kickstart and RAM image. When the RAM is made shared,
 * this will effectively keep KickTags etc.
 * Cold reboot is the same as before, re-running everything from scratch.
 * Shutdown is just plain exit.
 */
int kick(kernel_entry_fun_t addr, struct TagItem *msg)
{
    int i;
#ifdef __linux__
    pid_t bootstrap_pid = getpid();
#endif

    do
    {
        pid_t child = fork();

        switch (child)
        {
        case -1:
            DisplayError("Failed to run kickstart!");
            return -1;

        case 0:
#ifdef __linux__
            /* The AROS kernel runs in this child; if the bootstrap parent is
             * killed the child must not linger as an orphan.  Ask the kernel to
             * SIGKILL us when the parent dies, and bail out immediately if it
             * already died in the fork() window. */
            prctl(PR_SET_PDEATHSIG, SIGKILL);
            if (getppid() != bootstrap_pid)
                _exit(0);
#endif
            fprintf(stderr, "[Bootstrap] Entering kernel at %p...\n", addr);
            Host_PreBoot();
            i = addr(msg, AROS_BOOT_MAGIC);
            exit(i);
        }

        /* Wait until AROS process exits */
        waitpid(child, &i, 0);

        if (!WIFEXITED(i))
        {
            D(fprintf(stderr, "AROS process died with error\n"));
            return -1;
        }

        D(fprintf(stderr, "AROS exited with status 0x%08X\n", WEXITSTATUS(i)));

        /* ColdReboot() returns 0x8F */
    } while (WEXITSTATUS(i) == 0x8F);

    if (WEXITSTATUS(i) == 0x81)
    {
        /*
         * Perform cold boot if requested.
         * Before rebooting, we clean up. Otherwise execvp()'ed process will
         * inherit what we allocated here, then again... This will cause memory leak.
         */
        Host_FreeMem();
        Host_ColdBoot();
    }

    return WEXITSTATUS(i);
}

#endif
