#include <sys/wait.h>

#include <errno.h>
#include <signal.h>
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

#include "android.h"
#include "kickstart.h"

#define D(x) x

static void childHandler(int sig)
{
    int i;

    wait(&i);

    if (WIFEXITED(i))
    {
    	D(kprintf("[Bootstrap] AROS process exited with code 0x%08X\n", WEXITSTATUS(i)));

	/* TODO: Process return code here */
    	exit(0);
    }
    else
    {
    	DisplayError("AROS process terminated, status 0x%08X\n", i);
    }
}

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
    struct sigaction sa;
    int displaypipe[2];
    int inputpipe[2];
    int i;
    pid_t child;

    sigemptyset(&sa.sa_mask);
    sa.sa_flags    = SA_RESTART;
    sa.sa_restorer = NULL;
    sa.sa_handler  = childHandler;

    sigaction(SIGCHLD, &sa, NULL);

    if (pipe(displaypipe))
    {
    	DisplayError("Failed to create display pipe: %s", strerror(errno));
    	return -1;
    }
    
    if (pipe(inputpipe))
    {
    	close(displaypipe[0]);
    	close(displaypipe[1]);

    	DisplayError("Failed to create input pipe: %s", strerror(errno));
    	return -1;
    }

    D(kprintf("[Bootstrap] Launching kickstart...\n"));
    child = fork();

    switch (child)
    {
    case -1:
    	close(displaypipe[0]);
    	close(displaypipe[1]);
    	close(inputpipe[0]);
    	close(inputpipe[1]);

    	DisplayError("Failed to run kickstart!");
    	return -1;

    case 0:
    	/* Set up client side of pipes */
	DisplayPipe = displaypipe[1];
	InputPipe   = inputpipe[0];
    	close(displaypipe[0]);
    	close(inputpipe[1]);

        D(kprintf("[Bootstrap] entering kernel at 0x%p...\n", addr));
        i = addr(msg, AROS_BOOT_MAGIC);
        exit(i);
    }

    D(kprintf("[Bootstrap] AROS PID is %d\n", child));

    /* Set up server side of pipes */
    DisplayPipe = displaypipe[0];
    InputPipe   = inputpipe[1];
    close(displaypipe[1]);
    close(inputpipe[0]);

    /* Return to JVM with success indication */
    return 0;
}
