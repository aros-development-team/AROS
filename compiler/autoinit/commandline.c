/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$
*/
#include <dos/dos.h>
#include <exec/memory.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <aros/symbolsets.h>
#include <aros/autoinit.h>

#define DEBUG 0
#include <aros/debug.h>

int __nocommandline __attribute__((weak)) = 0;

extern void *WBenchMsg;
extern char *__argstr;
extern ULONG __argsize;

extern char **__argv;
extern int  __argc;

static char *__args;
static int  __argmax;

static void process_cmdline(int *argc, char *args, char *argv[]);

int __initcommandline(void)
{
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
	process_cmdline(&__argmax, __args, NULL);

	if (!(__argv = AllocMem (sizeof (char *) * (__argmax+1), MEMF_ANY | MEMF_CLEAR)) )
	    return 0;

	D(bug("arg(%d)=\"%s\", argmax=%d\n", __argsize, __args, __argmax));

	/* create argv */
	process_cmdline(&__argc, __args, __argv);
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

#if DEBUG /* Debug argument parsing */

    kprintf("argc = %d\n", __argc);
    {
	int t;
	for (t=0; t<__argc; t++)
	    kprintf("argv[%d] = \"%s\"\n", t, __argv[t]);
    }

#endif

    return 1;
}

static void process_cmdline(int *pargc, char *args, char *argv[])
{
    int argc, exit_loop, quote;
    char *ptr, prev;

    for (argc = 1, ptr = args; *ptr; )
    {
	if (*ptr == ' ' || *ptr == '\t' || *ptr == '\n')
	{
	    /* Skip whitespace */
	    ptr++;
	    while (*ptr && (*ptr == ' ' || *ptr == '\t' || *ptr == '\n'))
		ptr ++;
	}

	if (*ptr)
	{
	    if (*ptr == '"' || *ptr == '\'')
		quote = *ptr++;
	    else
		quote = -1;

	    if (argv)
		argv[argc++] = ptr;
	    else
		argc++;

	/*
	 * This code can now handle a command line like:
	 * kpsewhich --format="web2c files" mktex.opt
	 * fontinfo "Vera Sans.font" 40
	 */

	    prev = -1;
	    while (*ptr)
	    {
		exit_loop = 0;

		switch (*ptr)
		{
		case '"':
		case '\'':
		    if (prev != '\\')
		    {
			if (quote == *ptr)
			{
			    quote = -1;

			    if (argv)
			    {
				char *src = ptr + 1, *dst = ptr;
				while (*src)
				    *dst++ = *src++;
				*dst = '\0';
			    }

			    exit_loop = 1;
			}
			else
			    quote = *ptr++;

			prev = quote;
		    }
		    else
		    {
			prev = *ptr;
			if (argv)
			{
			    char *src = ptr, *dst = ptr - 1;
			    while (*src)
				*dst++ = *src++;
			    *dst = '\0';
			}
			++ptr;
		    }
		    break;

		case '\\':
		    if (prev == '\\')
		    {
			if (argv)
			{
			    char *src = ptr + 1, *dst = ptr;
			    while (*src)
				*dst++ = *src++;
			    *dst = '\0';
			}
			prev = -1;
		    }
		    else
			prev = '\\';
		    ++ptr;
		    break;

		case ' ':
		case '\t':
		case '\n':
		    if (quote == -1)
			exit_loop = 1;
		    else
			prev = *ptr++;
		    break;

		default:
		    prev = *ptr++;
		    break;
		}

		if (exit_loop)
		    break;
	    }

	    if (argv && *ptr)
		*ptr++ = 0;
	}
    }

    *pargc = argc;
}

void __exitcommandline(void)
{
    if (WBenchMsg != NULL)
        return;

    if (__argv)
	FreeMem(__argv, sizeof (char *) * (__argmax+1));

    if (__args)
	FreeMem(__args, __argsize+1);
}

ADD2INIT(__initcommandline, 0);
ADD2EXIT(__exitcommandline, 0);
