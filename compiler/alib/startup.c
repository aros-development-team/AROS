/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$

    Desc: Common startup code
    Lang: english
*/
#include <aros/config.h>
#include <setjmp.h>
#include <dos/dos.h>
#include <exec/memory.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <aros/asmcall.h>
#if 1
#   include <aros/debug.h>
#endif

#if AROS_FLAVOUR == AROS_FLAVOUR_NATIVE
asm("
	.text

	move.l	4.w,a6
	jra	_entry(pc)
");
#endif

/* Don't define symbols before the entry point. */
extern struct ExecBase * SysBase;
extern int main (int argc, char ** argv);
extern APTR __startup_mempool; /* malloc() and free() */
extern jmp_buf __startup_jmp_buf;
extern LONG __startup_error;

/*
    This won't work for normal AmigaOS because you can't expect SysBase to
    be in A6. The correct way is to use *(struct ExecBase **)4 and because
    gcc emits strings for a certain function _before_ the code the program
    will crash immediately because the first element in the code won't be
    valid assembler code.

    970314 ldp: It will now work because of the asm-stub above.

    TODO: reset and initialise the FPU.
          handle WBStartup message
          resident startup
*/
AROS_UFH3(LONG, entry,
    AROS_UFHA(char *,argstr,A0),
    AROS_UFHA(ULONG,argsize,D0),
    AROS_UFHA(struct ExecBase *,sysbase,A6)
)
{
    char * args,
	** argv,
	 * ptr;
    int    argc,
	   argmax;
    __startup_error = RETURN_FAIL;

    SysBase=sysbase;

    if (argsize)
    {
	/* Copy args into buffer */
	if (!(args = AllocMem (argsize+1, MEMF_ANY)) )
	{
	    argv = NULL;
	    goto error;
	}

	ptr = args;

	while ((*ptr++ = *argstr++));

	/* Find out how many arguments we have */
	for (argmax=1,ptr=args; *ptr; )
	{
	    if (*ptr == ' ' || *ptr == '\t')
	    {
		/* Skip whitespace */
		while (*ptr && (*ptr == ' ' || *ptr == '\t'))
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

		while (*ptr && *ptr != ' ' && *ptr != '\t')
		    ptr ++;
	    }
	}

	if (!(argv = AllocMem (sizeof (char *) * argmax, MEMF_ANY)) )
	{
	    goto error;
	}

	/* Find out how many arguments we have */
	for (argc=1,ptr=args; *ptr; )
	{
	    if (*ptr == ' ' || *ptr == '\t')
	    {
		/* Skip whitespace */
		while (*ptr && (*ptr == ' ' || *ptr == '\t'))
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

		while (*ptr && *ptr != ' ' && *ptr != '\t')
		    ptr ++;

		/* Not at end of string ? Terminate arg */
		if (*ptr)
		    *ptr ++ = 0;
	    }
	}
    }
    else
    {
	argv = &args;
	argc = 0;
    }

    DOSBase = (struct DosLibrary *)OpenLibrary (DOSNAME, 39);

    /* Get name of program * /
    argv[0] = GetProgrammName (); */
    argv[0] = "dummy";

#if 0
kprintf ("arg(%d)=\"%s\", argmax=%d, argc=%d\n", argsize, argstr, argmax, argc);
{
int t;
for (t=0; t<argc; t++)
    kprintf ("argv[%d] = \"%s\"\n", t, argv[t]);
}
#endif

    if (DOSBase != NULL)
    {
	if (!setjmp (__startup_jmp_buf))
	    __startup_error = main (argc, argv);

	CloseLibrary((struct Library *)DOSBase);
    }

error:
    if (argsize)
    {
	if (argv)
	    FreeMem (argv, sizeof (char *) * argmax);

	if (args)
	    FreeMem (args, argsize+1);
    }

    if (__startup_mempool)
	DeletePool (__startup_mempool);

    return __startup_error;
} /* entry */

struct ExecBase *SysBase;
struct DosLibrary *DOSBase;

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

