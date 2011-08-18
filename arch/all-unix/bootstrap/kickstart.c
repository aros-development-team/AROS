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
#include "platform.h"

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
int kick(int (*addr)(), struct TagItem *msg)
{
    int i;
    
    do
    {
    	pid_t child = fork();

    	switch (child)
    	{
    	case -1:
    	    DisplayError("Failed to run kickstart!");
    	    return -1;

    	case 0:
            fprintf(stderr, "[Bootstrap] entering kernel at %p...\n", addr);
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
