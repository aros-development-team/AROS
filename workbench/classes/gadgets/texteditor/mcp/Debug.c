/***************************************************************************

 TextEditor.mcc - Textediting MUI Custom Class
 Copyright (C) 1997-2000 Allan Odgaard
 Copyright (C) 2005 by TextEditor.mcc Open Source Team

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.

 TextEditor class Support Site:  http://www.sf.net/projects/texteditor-mcc

 $Id: Debug.c,v 1.3 2005/08/02 08:24:16 itix Exp $

***************************************************************************/

#ifdef DEBUG

#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/dos.h>

#include "SDI_compiler.h"
#include "Debug.h"
#include "rev.h"

// special flagging macros
#define isFlagSet(v,f)      (((v) & (f)) == (f))  // return TRUE if the flag is set
#define hasFlag(v,f)        (((v) & (f)) != 0)    // return TRUE if one of the flags in f is set in v
#define isFlagClear(v,f)    (((v) & (f)) == 0)    // return TRUE if flag f is not set in v
#define SET_FLAG(v,f)       ((v) |= (f))          // set the flag f in v
#define CLEAR_FLAG(v,f)     ((v) &= ~(f))         // clear the flag f in v
#define MASK_FLAG(v,f)      ((v) &= (f))          // mask the variable v with flag f bitwise

#if !defined(__MORPHOS__) && !defined(__AROS__)
extern void KPutFmt(const char *format, va_list arg);
#endif

// our static variables with default values
static int indent_level = 0;
static BOOL ansi_output = FALSE;
static ULONG debug_flags = DBF_ALWAYS | DBF_STARTUP; // default debug flags
static ULONG debug_classes = DBC_ERROR | DBC_DEBUG | DBC_WARNING | DBC_ASSERT | DBC_REPORT; // default debug classes

/****************************************************************************/

void SetupDebug(void)
{
  char var[256];

  kprintf("** TextEditor.mcc v" LIB_REV_STRING " startup ***********************\n");
  kprintf("Initializing runtime debugging:\n");

	if(GetVar("texteditor.mcp.debug", var, sizeof(var), 0) > 0)
	{
		char *tok;
    char *debug = var;

    // static list of our debugging classes tokens.
    // in the yamdebug variable these classes always start with a @
    static struct { char *token; unsigned long flag; } dbclasses[] =
		{
			{ "ctrace",  DBC_CTRACE   },
			{ "report",  DBC_REPORT   },
			{ "assert",  DBC_ASSERT   },
			{ "timeval", DBC_TIMEVAL  },
			{ "debug",   DBC_DEBUG    },
			{ "error",   DBC_ERROR    },
			{ "warning", DBC_WARNING  },
			{ "all",     DBC_ALL      },
			{ NULL,      0            }
		};

		static struct { char *token; unsigned long flag; } dbflags[] =
		{
			{ "always",   DBF_ALWAYS  },
			{ "startup",  DBF_STARTUP },
			{ "all",      DBF_ALL     },
			{ NULL,       0           }
		};

    // we parse the env variable token-wise
		while((tok = strtok(debug, ", ;")))
		{
			ULONG i;

      // check if the token is class definition or
      // just a flag definition
      if(tok[0] == '@')
      {
        // check if this call is a negation or not
        if(tok[1] == '!')
        {
          // search for the token and clear the flag
    			for(i=0; dbclasses[i].token; i++)
	    		{
		    		if(stricmp(tok+2, dbclasses[i].token) == 0)
            {
              kprintf("clear '%s' debug class flag.\n", dbclasses[i].token);
			      	CLEAR_FLAG(debug_classes, dbclasses[i].flag);
            }
			    }
        }
        else
        {
          // search for the token and set the flag
    			for(i=0; dbclasses[i].token; i++)
	    		{
		    		if(stricmp(tok+1, dbclasses[i].token) == 0)
            {
              kprintf("set '%s' debug class flag\n", dbclasses[i].token);
			      	SET_FLAG(debug_classes, dbclasses[i].flag);
            }
			    }
        }
			}
      else
      {
        // check if this call is a negation or not
        if(tok[1] == '!')
        {
    			for(i=0; dbflags[i].token; i++)
	    		{
		    		if(stricmp(tok+1, dbflags[i].token) == 0)
            {
              kprintf("clear '%s' debug flag\n", dbflags[i].token);
			      	CLEAR_FLAG(debug_flags, dbflags[i].flag);
            }
			    }
        }
        else
        {
          // check if the token was "ansi" and if so enable the ANSI color
          // output
          if(stricmp(tok, "ansi") == 0)
          {
            kprintf("ansi output enabled\n");
            ansi_output = TRUE;
          }
          else
          {
      			for(i=0; dbflags[i].token; i++)
	      		{
		      		if(stricmp(tok, dbflags[i].token) == 0)
              {
                kprintf("set '%s' debug flag\n", dbflags[i].token);
			        	SET_FLAG(debug_flags, dbflags[i].flag);
              }
	  		    }
          }
        }
      }

      debug = NULL;
		}
	}

	kprintf("set debug classes/flags (env:texteditor.mcp.debug): %08x/%08x\n", debug_classes, debug_flags);
  kprintf("** Normal processing follows ***************************************\n");
}

/****************************************************************************/

// define variables for using ANSI colors in our debugging scheme
#define ANSI_ESC_CLR				"\033[0m"
#define ANSI_ESC_BOLD				"\033[1m"
#define ANSI_ESC_UNDERLINE	"\033[4m"
#define ANSI_ESC_BLINK			"\033[5m"
#define ANSI_ESC_REVERSE		"\033[7m"
#define ANSI_ESC_INVISIBLE	"\033[8m"
#define ANSI_ESC_FG_BLACK		"\033[0;30m"
#define ANSI_ESC_FG_RED			"\033[0;31m"
#define ANSI_ESC_FG_GREEN		"\033[0;32m"
#define ANSI_ESC_FG_BROWN		"\033[0;33m"
#define ANSI_ESC_FG_BLUE		"\033[0;34m"
#define ANSI_ESC_FG_PURPLE	"\033[0;35m"
#define ANSI_ESC_FG_CYAN		"\033[0;36m"
#define ANSI_ESC_FG_LGRAY		"\033[0;37m"
#define ANSI_ESC_FG_DGRAY		"\033[1;30m"
#define ANSI_ESC_FG_LRED		"\033[1;31m"
#define ANSI_ESC_FG_LGREEN	"\033[1;32m"
#define ANSI_ESC_FG_YELLOW	"\033[1;33m"
#define ANSI_ESC_FG_LBLUE		"\033[1;34m"
#define ANSI_ESC_FG_LPURPLE	"\033[1;35m"
#define ANSI_ESC_FG_LCYAN		"\033[1;36m"
#define ANSI_ESC_FG_WHITE		"\033[1;37m"
#define ANSI_ESC_BG					"\033[0;4"		// background esc-squ start with 4x
#define ANSI_ESC_BG_BLACK		"\033[0;40m"
#define ANSI_ESC_BG_RED			"\033[0;41m"
#define ANSI_ESC_BG_GREEN		"\033[0;42m"
#define ANSI_ESC_BG_BROWN		"\033[0;43m"
#define ANSI_ESC_BG_BLUE		"\033[0;44m"
#define ANSI_ESC_BG_PURPLE	"\033[0;45m"
#define ANSI_ESC_BG_CYAN		"\033[0;46m"
#define ANSI_ESC_BG_LGRAY		"\033[0;47m"

/****************************************************************************/

INLINE void _INDENT(void)
{
  int i;
	for(i=0; i < indent_level; i++)
	  kprintf(" ");
}

/****************************************************************************/

void _ENTER(unsigned long dclass, const char *file, int line, const char *function)
{
	if(isFlagSet(debug_classes, dclass))
	{
		_INDENT();
		if(ansi_output)
      kprintf("%s%s:%ld:Entering %s%s\n", ANSI_ESC_FG_BROWN, file, line, function, ANSI_ESC_CLR);
    else
      kprintf("%s:%ld:Entering %s\n", file, line, function);
	}

	indent_level++;
}

void _LEAVE(unsigned long dclass, const char *file, int line, const char *function)
{
	indent_level--;

	if(isFlagSet(debug_classes, dclass))
	{
		_INDENT();
    if(ansi_output)
  		kprintf("%s%s:%ld:Leaving %s%s\n", ANSI_ESC_FG_BROWN, file, line, function, ANSI_ESC_CLR);
    else
  		kprintf("%s:%ld:Leaving %s\n", file, line, function);
	}
}

void _RETURN(unsigned long dclass, const char *file, int line, const char *function, unsigned long result)
{
	indent_level--;

	if(isFlagSet(debug_classes, dclass))
	{
		_INDENT();
    if(ansi_output)
  		kprintf("%s%s:%ld:Leaving %s (result 0x%08lx, %ld)%s\n", ANSI_ESC_FG_BROWN, file, line, function, result, result, ANSI_ESC_CLR);
    else
  		kprintf("%s:%ld:Leaving %s (result 0x%08lx, %ld)\n", file, line, function, result, result);
	}
}

/****************************************************************************/

void _SHOWVALUE(unsigned long dclass, unsigned long dflags, unsigned long value, int size, const char *name, const char *file, int line)
{
	if(isFlagSet(debug_classes, dclass) &&
     isFlagSet(debug_flags, dflags))
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

    if(ansi_output)
      kprintf(ANSI_ESC_FG_GREEN);

		kprintf(fmt, file, line, name, value, value);

		if(size == 1 && value < 256)
		{
			if(value < ' ' || (value >= 127 && value < 160))
				kprintf(", '\\x%02lx'", value);
			else
				kprintf(", '%lc'", value);
		}

    if(ansi_output)
      kprintf("%s\n", ANSI_ESC_CLR);
    else
  		kprintf("\n");
	}
}

/****************************************************************************/

void _SHOWPOINTER(unsigned long dclass, unsigned long dflags, const void *p, const char *name, const char *file, int line)
{
	if(isFlagSet(debug_classes, dclass) &&
     isFlagSet(debug_flags, dflags))
	{
		char *fmt;

		_INDENT();

		if(p != NULL)
			fmt = "%s:%ld:%s = 0x%08lx\n";
		else
			fmt = "%s:%ld:%s = NULL\n";

		if(ansi_output)
    {
      kprintf(ANSI_ESC_FG_GREEN);
      kprintf(fmt, file, line, name, p);
      kprintf(ANSI_ESC_CLR);
    }
    else
      kprintf(fmt, file, line, name, p);
	}
}

/****************************************************************************/

void _SHOWSTRING(unsigned long dclass, unsigned long dflags, const char *string, const char *name, const char *file, int line)
{
	if(isFlagSet(debug_classes, dclass) &&
     isFlagSet(debug_flags, dflags))
	{
		_INDENT();

    if(ansi_output)
  		kprintf("%s%s:%ld:%s = 0x%08lx \"%s\"%s\n", ANSI_ESC_FG_GREEN, file, line, name, string, string, ANSI_ESC_CLR);
    else
  		kprintf("%s:%ld:%s = 0x%08lx \"%s\"\n", file, line, name, string, string);
	}
}

/****************************************************************************/

void _SHOWMSG(unsigned long dclass, unsigned long dflags, const char *msg, const char *file, int line)
{
	if(isFlagSet(debug_classes, dclass) &&
     isFlagSet(debug_flags, dflags))
	{
		_INDENT();

    if(ansi_output)
  		kprintf("%s%s:%ld:%s%s\n", ANSI_ESC_FG_GREEN, file, line, msg, ANSI_ESC_CLR);
	  else
  		kprintf("%s:%ld:%s\n", file, line, msg);
  }
}

/****************************************************************************/

void _DPRINTF(unsigned long dclass, unsigned long dflags, const char *file, int line, const char *format, ...)
{
	if(isFlagSet(debug_classes, dclass) &&
     isFlagSet(debug_flags, dflags))
	{
		va_list args;

		_INDENT();

    va_start(args, format);

    if(ansi_output)
    {
  		char *highlight = ANSI_ESC_FG_GREEN;
	
    	switch(dclass)
  		{
        case DBC_CTRACE:  highlight = ANSI_ESC_FG_BROWN; break;
        case DBC_REPORT:  highlight = ANSI_ESC_FG_GREEN; break;
        case DBC_ASSERT:  highlight = ANSI_ESC_FG_RED;   break;
        case DBC_TIMEVAL: highlight = ANSI_ESC_FG_GREEN; break;
        case DBC_DEBUG:   highlight = ANSI_ESC_FG_GREEN; break;
        case DBC_ERROR:   highlight = ANSI_ESC_FG_RED;   break;
        case DBC_WARNING: highlight = ANSI_ESC_FG_YELLOW;break;
  		}

      kprintf("%s%s:%ld:", highlight, file, line);

	  	KPutFmt((char *)format, args);

  		kprintf("%s\n", ANSI_ESC_CLR);
  	}
    else
    {
      kprintf("%s:%ld:", file, line);

	  	KPutFmt((char *)format, args);

  		kprintf("\n");
	  }

    va_end(args);
	}
}

/****************************************************************************/

#endif
