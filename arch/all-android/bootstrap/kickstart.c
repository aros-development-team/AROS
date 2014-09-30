/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

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
#include "platform.h"

#define D(x) x

/*
 * This is Android-hosted kicker.
 * Android environment is the most alien one to AROS. It's proved to be impossible
 * to run AROS inside JNI because AROS task switching is no friend to threads.
 * However it's possible to run AROS as a separate process. This way it will be
 * completely detached and live on its own.
 *
 * The bootstrap itself still runs in JVM context, this makes it easier to
 * implement DisplayError().
 */

/* Interface with the display driver */
int DisplayPipe;
int InputPipe;

/* This is where we remember our data */
static int (*EntryPoint)() = NULL;
static struct TagItem *BootMsg = NULL;
static pid_t AROS_pid = -1;

/*
 * Our SIGCHLD handler.
 * It will notify Java side when AROS process exits. Additionally it will free allocated memory
 * before performing a cold reboot.
 */
static void childHandler(int sig)
{
    int i;

    wait(&i);

    if (WIFEXITED(i))
    {
    	int code = WEXITSTATUS(i);

    	D(kprintf("[Bootstrap] AROS process exited with code 0x%08X\n", code));

	/*
	 * If the requested action is not a warm reboot, free all the RAM.
	 * On cold reboot we will reload the kickstart from scratch.
	 */
	if (code != STATUS_WARM_REBOOT)
	    Host_FreeMem();

	/* Let the Java part to do the work */
	(*Java_Env)->CallVoidMethod(Java_Env, Java_Object, HandleExit_mid, WEXITSTATUS(i));
    }
    else
    {
    	DisplayError("AROS process terminated, status 0x%08X\n", i);
    }
}

/*
 * Just remember arguments and return with zero returncode.
 * This will signal to Java side that preparation (Load() method)
 * completed succesfully. Next Java side can execute Kick() method
 * in order to actually run AROS.
 * This separation helps to implement warm restart. Kick() can be executed
 * multiple times.
 */
int kick(int (*addr)(), struct TagItem *msg)
{
    EntryPoint = addr;
    BootMsg    = msg;

    return 0;
}

/* 
 * The actual kicker.
 * Similar to generic UNIX one, but uses SIGCHILD handler instead of waitpid()
 * in order to detect when AROS exits.
 * When this method succesfully exits, a display pipe server is run on Java side.
 */
int Java_org_aros_bootstrap_AROSBootstrap_Kick(JNIEnv* env, jobject this, jobject readfd, jobject writefd)
{
    struct sigaction sa;
    int displaypipe[2];
    int inputpipe[2];
    int i;
    jclass *class_fdesc = (*env)->GetObjectClass(env, readfd);
    /*
     * In Sun JVM this is 'fd' field, in Android it's 'descriptor'.
     * This is the only place which can be considered a hack. I hope Android guys
     * won't change this.
     */
    jfieldID field_fd = (*env)->GetFieldID(env, class_fdesc, "descriptor", "I");

    if (!field_fd)
    {
    	DisplayError("Failed to set up pipe descriptor objects");
    	return -1;
    }

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
    AROS_pid = fork();

    switch (AROS_pid)
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

        D(kprintf("[Bootstrap] entering kernel at %p...\n", EntryPoint));
        i = EntryPoint(BootMsg, AROS_BOOT_MAGIC);
        exit(i);
    }

    D(kprintf("[Bootstrap] AROS PID %d, bootstrap PID %d\n", AROS_pid, getpid()));

    /* Set up server side of pipes */
    (*env)->SetIntField(env, readfd, field_fd, displaypipe[0]);
    (*env)->SetIntField(env, writefd, field_fd, inputpipe[1]);
    close(displaypipe[1]);
    close(inputpipe[0]);

    /* Return to JVM with success indication */
    return 0;
}

/* A small method which allows display server to send signals to AROS process */
int Java_org_aros_bootstrap_AROSBootstrap_Kill(JNIEnv* env, jobject this, jint signal)
{
    int res = kill(AROS_pid, signal);

    D(kprintf("[Bootstrap] kill(%d, %d) returned %d, errno %d\n", AROS_pid, signal, res, errno));    
    return res;
}
