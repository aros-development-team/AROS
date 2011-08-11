#include <sys/wait.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

/* These macros are defined in both UNIX and AROS headers. Get rid of warnings. */
#undef __pure
#undef __const
#undef __pure2
#undef __deprecated

#include <aros/kernel.h>
#include <runtime.h>

#include "kickstart.h"

#define D(x)

/*
 * This is Android-hosted kicker.
 * Android environment is the most alien one to AROS. It's proved to be impossible
 * to run AROS inside JNI because AROS task switching is no friend to threads.
 * However it's possible to run AROS as a separate process. This way it will be
 * completely detached and live on its own.
 * AROS display driver needs to communicate to Java user interface via pipes,
 * UNIX sockets, or some other similar mechanism. This is still a TODO.
 *
 * The bootstrap itself still runs in JVM context, this makes it easier to
 * implement DisplayError().
 */
int kick(int (*addr)(), struct TagItem *msg)
{
    int i;
    pid_t child;

    D(kprintf("[Bootstrap] Launching kickstart...\n"));
    child = fork();

    switch (child)
    {
    case -1:
    	DisplayError("Failed to run kickstart!");
    	return -1;

    case 0:
        D(kprintf("[Bootstrap] entering kernel at 0x%p...\n", addr));
        i = addr(msg, AROS_BOOT_MAGIC);
        exit(i);
    }

    /* Return to JVM with success indication */
    return 0;
}
