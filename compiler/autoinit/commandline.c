/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <dos/dos.h>
#include <exec/memory.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <aros/symbolsets.h>
#include <aros/autoinit.h>
#include <aros/debug.h>

int __nocommandline __attribute__((weak)) = 0;

extern void *WBenchMsg;
extern char *__argstr;
extern ULONG __argsize;

extern char **__argv;
extern int  __argc;

static char *__args;
static int  __argmax;

int __initcommandline(void)
{
    AROS_GET_SYSBASE_OK
    char *ptr    = NULL;

    if (WBenchMsg)
        return 1;

    if (__argsize)
    {
        /* Copy args into buffer */
    	if (!(__args = AllocMem(__argsize+1, MEMF_ANY)))
	    return 0;

    	ptr = __args;
    	while ((*ptr++ = *__argstr++)) {}

    	/* Find out how many arguments we have */
	for (__argmax=1,ptr=__args; *ptr; )
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
		__argmax ++;
		ptr ++;

		/* Skip until next " */
		while (*ptr && *ptr != '"')
		    ptr ++;

		if (*ptr)
		    ptr ++;
	    }
	    else if (*ptr)
	    {
		__argmax ++;

		while (*ptr && *ptr != ' ' && *ptr != '\t' && *ptr != '\n')
		    ptr ++;
	    }
	}

	if (!(__argv = AllocMem (sizeof (char *) * (__argmax+1), MEMF_ANY | MEMF_CLEAR)) )
	    return 0;

	/* create argv */
	for (__argc=1,ptr=__args; *ptr; )
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
		__argv[__argc++] = ptr;

		/* Skip until next " */
		while (*ptr && *ptr != '"')
		    ptr ++;

		/* Terminate argument */
		if (*ptr)
		    *ptr ++ = 0;
	    }
	    else if (*ptr)
	    {
		__argv[__argc++] = ptr;

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
	__argmax = 1;
	__argc = 1;
	if (!(__argv = AllocMem (sizeof (char *)*2, MEMF_CLEAR | MEMF_ANY)))
	    return 0;
    }

    /*
     * get program name
     */
     __argv[0] = FindTask(NULL)->tc_Node.ln_Name;

     if (!__argv[0])
         return 0;

#if 0 /* Debug argument parsing */

    kprintf("arg(%d)=\"%s\", argmax=%d, argc=%d\n", __argsize, __argstr, __argmax, __argc);
    {
	int t;
	for (t=0; t<__argc; t++)
	    kprintf("argv[%d] = \"%s\"\n", t, __argv[t]);
    }

#endif

    return 1;
}

void __exitcommandline(void)
{
    AROS_GET_SYSBASE_OK

    if (WBenchMsg != NULL)
        return;

    if (__argv)
	FreeMem(__argv, sizeof (char *) * (__argmax+1));

    if (__args)
	FreeMem(__args, __argsize+1);
}

ADD2INIT(__initcommandline, 0);
ADD2EXIT(__exitcommandline, 0);
