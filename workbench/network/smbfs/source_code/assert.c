/*
 * $Id$
 *
 * :ts=8
 *
 * SMB file system wrapper for AmigaOS, using the AmiTCP V3 API
 *
 * Copyright (C) 2000-2009 by Olaf `Olsen' Barthel <obarthel -at- gmx -dot- net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/****************************************************************************/

#include <dos/dos.h>

#include <clib/exec_protos.h>
#include <clib/dos_protos.h>

#ifdef __SASC
#include <pragmas/exec_pragmas.h>
#include <pragmas/dos_pragmas.h>
#else
#include <inline/exec.h>
#include <inline/dos.h>
#endif /* __SASC */

#include <string.h>

#if !defined(__AROS__)
extern struct Library * AbsExecBase;

#define SysBase AbsExecBase
#endif

extern void kprintf(const char *,...);
extern void __stdargs kputc(char c);

/****************************************************************************/

#include <stdarg.h>

/****************************************************************************/

#define DEBUGLEVEL_OnlyAsserts	0
#define DEBUGLEVEL_Reports	1
#define DEBUGLEVEL_CallTracing	2

/****************************************************************************/

static int indent_level = 0;
int __debug_level = DEBUGLEVEL_CallTracing;

static char program_name[40];
static int program_name_len = 0;

/****************************************************************************/

void
_SETPROGRAMNAME(char *name)
{
	if(name != NULL && name[0] != '\0')
	{
		program_name_len = strlen(name);
		if(program_name_len >= sizeof(program_name))
			program_name_len = sizeof(program_name)-1;

		strncpy(program_name,name,program_name_len);
		program_name[program_name_len] = '\0';
	}
	else
	{
		program_name_len = 0;
	}
}

/****************************************************************************/

int
_SETDEBUGLEVEL(int level)
{
	int old_level = __debug_level;

	__debug_level = level;

	return(old_level);
}

/****************************************************************************/

int
_GETDEBUGLEVEL(void)
{
	return(__debug_level);
}

/****************************************************************************/

static int previous_debug_level = -1;

void
_PUSHDEBUGLEVEL(int level)
{
	previous_debug_level = _SETDEBUGLEVEL(level);
}

void
_POPDEBUGLEVEL(void)
{
	if(previous_debug_level != -1)
	{
		_SETDEBUGLEVEL(previous_debug_level);

		previous_debug_level = -1;
	}
}

/****************************************************************************/

void
_INDENT(void)
{
	if(program_name_len > 0)
		kprintf("(%s) ",program_name);

	if(__debug_level >= DEBUGLEVEL_CallTracing)
	{
		int i;

		for(i = 0 ; i < indent_level ; i++)
			kprintf("   ");
	}
}

/****************************************************************************/

void
_SHOWVALUE(
	unsigned long value,
	int size,
	const char *name,
	const char *file,
	int line)
{
	if(__debug_level >= DEBUGLEVEL_Reports)
	{
		char *fmt;

		switch(size)
		{
			case 1:

				fmt = "%s:%ld:%s = %ld, 0x%02lx";
				break;

			case 2:

				fmt = "%s:%ld:%s = %ld, 0x%04lx";
				break;

			default:

				fmt = "%s:%ld:%s = %ld, 0x%08lx";
				break;
		}

		_INDENT();

		kprintf(fmt,file,line,name,value,value);

		if(size == 1 && value < 256)
		{
			if(value < ' ' || (value >= 127 && value < 160))
				kprintf(", '\\x%02lx'",value);
			else
				kprintf(", '%lc'",value);
		}

		kprintf("\n");
	}
}

/****************************************************************************/

void
_SHOWPOINTER(
	void *pointer,
	const char *name,
	const char *file,
	int line)
{
	if(__debug_level >= DEBUGLEVEL_Reports)
	{
		char *fmt;

		_INDENT();

		if(pointer != NULL)
			fmt = "%s:%ld:%s = 0x%08lx\n";
		else
			fmt = "%s:%ld:%s = NULL\n";

		kprintf(fmt,file,line,name,pointer);
	}
}

/****************************************************************************/

void
_SHOWSTRING(
	const char *string,
	const char *name,
	const char *file,
	int line)
{
	if(__debug_level >= DEBUGLEVEL_Reports)
	{
		_INDENT();
		kprintf("%s:%ld:%s = 0x%08lx \"%s\"\n",file,line,name,string,string);
	}
}

/****************************************************************************/

void
_SHOWMSG(
	const char *string,
	const char *file,
	int line)
{
	if(__debug_level >= DEBUGLEVEL_Reports)
	{
		_INDENT();
		kprintf("%s:%ld:%s\n",file,line,string);
	}
}

/****************************************************************************/

void
_DPRINTF_HEADER(
	const char *file,
	int line)
{
	if(__debug_level >= DEBUGLEVEL_Reports)
	{
		_INDENT();
		kprintf("%s:%ld:",file,line);
	}
}

#ifdef __SASC

static void __asm
putch(register __d0 UBYTE c)
{
	if(c != '\0')
		kputc(c);
}

#else

static void
putch(UBYTE c __asm("d0"))
{
	if(c != '\0')
		kputc(c);
}

#endif /* __SASC */

void
_DPRINTF(const char *fmt,...)
{
	if(__debug_level >= DEBUGLEVEL_Reports)
	{
		va_list args;

		va_start(args,fmt);
		RawDoFmt((char *)fmt,args,(VOID (*)())putch,NULL);
		va_end(args);

		kprintf("\n");
	}
}

void
_DLOG(const char *fmt,...)
{
	if(__debug_level >= DEBUGLEVEL_Reports)
	{
		va_list args;

		va_start(args,fmt);
		RawDoFmt((char *)fmt,args,(VOID (*)())putch,NULL);
		va_end(args);
	}
}

/****************************************************************************/

void
_ENTER(
	const char *file,
	int line,
	const char *function)
{
	if(__debug_level >= DEBUGLEVEL_CallTracing)
	{
		_INDENT();
		kprintf("%s:%ld:Entering %s\n",file,line,function);
	}

	indent_level++;
}

void
_LEAVE(
	const char *file,
	int line,
	const char *function)
{
	indent_level--;

	if(__debug_level >= DEBUGLEVEL_CallTracing)
	{
		_INDENT();
		kprintf("%s:%ld: Leaving %s\n",file,line,function);
	}
}

void
_RETURN(
	const char *file,
	int line,
	const char *function,
	unsigned long result)
{
	indent_level--;

	if(__debug_level >= DEBUGLEVEL_CallTracing)
	{
		_INDENT();
		kprintf("%s:%ld: Leaving %s (result 0x%08lx, %ld)\n",file,line,function,result,result);
	}
}

/****************************************************************************/

void
_ASSERT(
	int x,
	const char *xs,
	const char *file,
	int line,
	const char *function)
{
	#ifdef CONFIRM
	{
		STATIC BOOL ScrollMode	= FALSE;
		STATIC BOOL BatchMode	= FALSE;

		if(BatchMode == FALSE)
		{
			if(x == 0)
			{
				kprintf("%s:%ld:Expression `%s' failed assertion in %s().\n",
				        file,
				        line,
				        xs,
				        function);

				if(ScrollMode == FALSE)
				{
					ULONG Signals;

					SetSignal(0,SIGBREAKF_CTRL_C | SIGBREAKF_CTRL_D | SIGBREAKF_CTRL_E);

					kprintf(" ^C to continue, ^D to enter scroll mode, ^E to enter batch mode\r");

					Signals = Wait(SIGBREAKF_CTRL_C | SIGBREAKF_CTRL_D | SIGBREAKF_CTRL_E);

					if(Signals & SIGBREAKF_CTRL_D)
					{
						ScrollMode = TRUE;

						kprintf("Ok, entering scroll mode\033[K\n");
					}
					else if (Signals & SIGBREAKF_CTRL_E)
					{
						BatchMode = TRUE;

						kprintf("Ok, entering batch mode\033[K\n");
					}
					else
					{
						/* Continue */

						kprintf("\033[K\r");
					}
				}
			}
		}
	}
	#else
	{
		if(x == 0)
		{
			_INDENT();
			kprintf("%s:%ld:Expression `%s' failed assertion in %s().\n",
			        file,
			        line,
			        xs,
			        function);
		}
	}
	#endif	/* CONFIRM */
}
