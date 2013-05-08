/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$
*/
#include <dos/dos.h>
#include <exec/memory.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <aros/symbolsets.h>
#include <aros/autoinit.h>
#include <aros/startup.h>

#define DEBUG 0
#include <aros/debug.h>

#include "autoinit_intern.h"

int __nocommandline __attribute__((weak)) = 0;

extern char *__argstr;
extern ULONG __argsize;

extern char **__argv;
extern int  __argc;

static char *__args;
static int  __argmax;

static void process_cmdline(int *argc, char *args, char *argv[]);

static void __initcommandline(struct ExecBase *SysBase)
{
    char *ptr    = NULL;

    if (WBenchMsg || __nocommandline) {
        __startup_entries_next();
        return;
    }

    if (__argsize)
    {
        ULONG size;

        /* Copy args into buffer */
    	if (!(__args = AllocMem(__argsize+1, MEMF_ANY)))
	    return;

    	ptr = __args;
    	size= __argsize;
    	while (size--)
    	    *ptr++ = *__argstr++;
    	*ptr = 0;

    	/* Find out how many arguments we have */
	process_cmdline(&__argmax, __args, NULL);

	if (!(__argv = AllocMem (sizeof (char *) * (__argmax+1), MEMF_ANY | MEMF_CLEAR)) )
	    return;

	D(bug("arg(%d)=\"%s\", argmax=%d\n", __argsize, __args, __argmax));

	/* create argv */
	process_cmdline(&__argc, __args, __argv);
    }
    else
    {
	__argmax = 1;
	__argc = 1;
	if (!(__argv = AllocMem (sizeof (char *)*2, MEMF_CLEAR | MEMF_ANY)))
	    return;
    }

    /*
     * get program name
     */
     __argv[0] = __get_command_name();

#if DEBUG /* Debug argument parsing */

    kprintf("argc = %d\n", __argc);
    {
	int t;
	for (t=0; t<__argc; t++)
	    kprintf("argv[%d] = \"%s\"\n", t, __argv[t]);
    }

#endif

    __startup_entries_next();

    if (WBenchMsg != NULL)
        return;

    if (__argv)
	FreeMem(__argv, sizeof (char *) * (__argmax+1));

    if (__args)
	FreeMem(__args, __argsize+1);
}

static BOOL is_space(char c)
{
	switch(c)
	{
	case ' ':
	case '\n':
	case '\t':
	case '\v':
	case '\f':
	case '\r':
		return TRUE;
	default:
		return FALSE;
	}
}

static BOOL is_escape(char c)
{
	if(c == '*')
		return TRUE;
	else
		return FALSE;
}

static BOOL is_final_quote(char *ptr)
{
	if(*ptr == '\"' && (ptr[1] == '\0' || is_space(ptr[1])))
		return TRUE;
	else
		return FALSE;
}

static void process_cmdline(int *pargc, char *args, char *argv[])
{
	char *ptr = args;
	char *arg;
	int argc = 1;
	
	while(TRUE)
	{
		/* skip leading white spaces */
		while(is_space(*ptr)) 
			ptr++;
	
		if(*ptr == '\0')
			break;
		
		argc++;
		if(*ptr == '\"')
		{
			/* quoted parameter starts here */
			ptr++;
			
			/* store pointer to the parameter */
			if(argv)
				argv[argc-1] = ptr;
			
			/* unescape quoted parameter */
			arg = ptr;
			while(!(*ptr == '\0' || is_final_quote(ptr)))
			{
				if(argv)
				{
					/* unescaping */
					if(is_escape(*ptr))
					{
						ptr++;
						switch(*ptr)
						{
						case 'e':
						case 'E':
							*arg++ = '\033';
							break;
						case 'N':
						case 'n':
							*arg++ = '\n';
							break;
						case '\0':
							break;
						default:
							*arg++ = *ptr;
						}
						ptr++;
					}
					else
						*arg++ = *ptr++;
				}
				else
				{
					/* don't touch anything, just skip escapes */
					if(is_escape(*ptr))
					{
						ptr++;
						if(*ptr != '\0')
							ptr++;
					}
					else
						ptr++;
				}
			}
			/* skip final quote */
			if(*ptr != '\0')
				ptr++;
			/* quoted parameter ends here */
			if(argv)
				*arg = '\0';
		}
		else
		{
			/* unquoted parameter starts here */

			/* store pointer to the parameter */
			if(argv)
				argv[argc-1] = ptr;
			
			/* no escaping, just find the end */
			while(!(*ptr == '\0' || is_space(*ptr)))
				ptr++;
			
			/* stop processing if we reached the end of argument string */
			if(*ptr == '\0') 
				break;
			
			/* unquoted parameter ends here */
			if(argv)
				*ptr++ = '\0';
		}
	}
	/* store the number of arguments */
	*pargc = argc;
}

ADD2SET(__initcommandline, PROGRAM_ENTRIES, -10);
