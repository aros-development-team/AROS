/*
    Copyright (C) 1995-2000 AROS - The Amiga Research OS
    $Id$

    Desc: Common startup code
    Lang: english
*/
#include <aros/config.h>
#include <setjmp.h>
#include <dos/dos.h>
#include <exec/memory.h>
#include <exec/semaphores.h>
#include <workbench/startup.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <aros/asmcall.h>
#include <aros/debug.h>

#if (AROS_FLAVOUR & AROS_FLAVOUR_NATIVE)
asm("
	.text

	move.l	4.w,a6
	jra	_entry(pc)
");
#endif

/* Don't define symbols before the entry point. */
extern struct ExecBase * SysBase;
extern struct WBStartup *WBenchMsg;
extern int main (int argc, char ** argv);

extern struct SignalSemaphore __startup_memsem;
extern APTR __startup_mempool; /* malloc() and free() */
extern jmp_buf __startup_jmp_buf;
extern LONG __startup_error;

/*
    This won't work for normal AmigaOS because you can't expect SysBase to be
    in A6. The correct way is to use *(struct ExecBase **)4 and because gcc
    emits strings for a certain function _before_ the code the program will
    crash immediately because the first element in the code won't be valid
    assembler code.

    970314 ldp: It will now work because of the asm-stub above.

*/
#warning TODO: reset and initialize the FPU
#warning TODO: resident startup
AROS_UFH3(LONG, entry,
    AROS_UFHA(char *,argstr,A0),
    AROS_UFHA(ULONG,argsize,D0),
    AROS_UFHA(struct ExecBase *,sysbase,A6)
)
{
    char * args = NULL,
	** argv,
	 * ptr;
    int    argc,
	   argmax;
    LONG   namlen = 64;
    int    done = 0;
    struct Process *myproc;


    __startup_error = RETURN_FAIL;
    
    SysBase = sysbase;

    InitSemaphore(&__startup_memsem);

    if (!(DOSBase = (struct DosLibrary *)OpenLibrary(DOSNAME, 39)))
	return -1;

    myproc = (struct Process *)FindTask(NULL);

    /* Do we have a CLI structure? */
    if (myproc->pr_CLI)
    {
	/* Yes, started from the CLI */

	if (argsize)
	{
	    /* Copy args into buffer */
	    if (!(args = AllocMem(argsize+1, MEMF_ANY)))
	    {
		argv = NULL;
		goto error;
	    }

	    ptr = args;
	    while ((*ptr++ = *argstr++)) {}

	    /* Find out how many arguments we have */
	    for (argmax=1,ptr=args; *ptr; )
	    {
		if (*ptr == ' ' || *ptr == '\t' || *ptr == '\n')
		{
		    /* Skip whitespace */
		    while (*ptr && (*ptr == ' ' || *ptr == '\t' || *ptr == '\n'))
			ptr ++;
		}

		if (*ptr == '"')
		{
		    /* "..." argument ? */
		    argmax ++;
		    ptr ++;

		    /* Skip until next " */
		    while (*ptr && *ptr != '"')
			ptr ++;

		    if (*ptr)
			ptr ++;
		}
		else if (*ptr)
		{
		    argmax ++;

		    while (*ptr && *ptr != ' ' && *ptr != '\t' && *ptr != '\n')
			ptr ++;
		}
	    }

	    if (!(argv = AllocMem (sizeof (char *) * argmax, MEMF_CLEAR)) )
		goto error;

	    /* create argv */
	    for (argc=1,ptr=args; *ptr; )
	    {
		if (*ptr == ' ' || *ptr == '\t' || *ptr == '\n')
		{
		    /* Skip whitespace */
		    while (*ptr && (*ptr == ' ' || *ptr == '\t' || *ptr == '\n'))
			ptr ++;
		}

		if (*ptr == '"')
		{
		    /* "..." argument ? */
		    ptr ++;
		    argv[argc++] = ptr;

		    /* Skip until next " */
		    while (*ptr && *ptr != '"')
			ptr ++;

		    /* Terminate argument */
		    if (*ptr)
			*ptr ++ = 0;
		}
		else if (*ptr)
		{
		    argv[argc++] = ptr;

			while (*ptr && *ptr != ' ' && *ptr != '\t' && *ptr != '\n')
			    ptr ++;

		    /* Not at end of string ? Terminate arg */
		    if (*ptr)
			*ptr ++ = 0;
		}
	    }
	}
	else
	{
	    argmax = 1;
	    argc = 1;
	    if (!(argv = AllocMem (sizeof (char *), MEMF_ANY)))
		goto error;
	}

	/*
	 * get program name
	 */
	do {
	    if (!(argv[0] = AllocVec(namlen, MEMF_ANY)))
		goto error;
      
	    if (!(GetProgramName(argv[0], namlen)))
	    {
		if (IoErr() == ERROR_LINE_TOO_LONG)
		{
		    namlen *= 2;
		    FreeVec(argv[0]);
		}
		else
		    goto error;
	    }
	    else
		done = 1;
	} while (!done);

#if 0 /* Debug argument parsing */

	kprintf("arg(%d)=\"%s\", argmax=%d, argc=%d\n", argsize, argstr, argmax, argc);
	{
	    int t;
	    for (t=0; t<argc; t++)
		kprintf("argv[%d] = \"%s\"\n", t, argv[t]);
	}

#endif

    }
    else
    {
	/* Workbench startup. Get WBenchMsg and pass it to main() */

	WaitPort(&myproc->pr_MsgPort);
	WBenchMsg = (struct WBStartup *)GetMsg(&myproc->pr_MsgPort);
	argv = (char **)WBenchMsg;
	argsize = 0;
	argc = 0;
    }

    /* Invoke main() */
    if (!setjmp(__startup_jmp_buf))
	__startup_error = main (argc, argv);

error:
    if (argsize)
    {
	if (argv) {
	    if (argv[0])
		FreeVec(argv[0]);
	    FreeMem(argv, sizeof (char *) * argmax);
	}

	if (args)
	    FreeMem(args, argsize+1);
    }

    if (__startup_mempool)
	DeletePool(__startup_mempool);

    CloseLibrary((struct Library *)DOSBase);

    /* Reply startup message to Workbench.
     * We Forbid() to avoid being UnLoadSeg()ed before we're really finished.
     */
    if (WBenchMsg)
    {
	Forbid();
	ReplyMsg((struct Message *)WBenchMsg);
    }

    return __startup_error;
} /* entry */

struct ExecBase *SysBase;
struct DosLibrary *DOSBase;
struct WBStartup *WBenchMsg;

struct SignalSemaphore __startup_memsem;
APTR __startup_mempool = NULL;
jmp_buf __startup_jmp_buf;
LONG __startup_error;

/*	Stub function for GCC __main().

	The __main() function is originally used for C++ style constructors
	and destructors in C. This replacement does nothing and gets rid of
	linker-errors about references to __main().
*/

void __main(void)
{
/* Do nothing. */
}

