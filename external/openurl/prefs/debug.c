/***************************************************************************

 openurl.library - universal URL display and browser launcher library
 Copyright (C) 1998-2005 by Troels Walsted Hansen, et al.
 Copyright (C) 2005-2009 by openurl.library Open Source Team

 This library is free software; it has been placed in the public domain
 and you can freely redistribute it and/or modify it. Please note, however,
 that some components may be under the LGPL or GPL license.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

 openurl.library project: http://sourceforge.net/projects/openurllib/

 $Id$

***************************************************************************/

#ifdef DEBUG

#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

#include <proto/utility.h>
#include <proto/dos.h>
#include <proto/exec.h>

#include "version.h"

#include "debug.h"
#include "macros.h"

// our static variables with default values
static int indent_level = 0;
static BOOL ansi_output = FALSE;
static ULONG debug_flags = DBF_ALWAYS | DBF_STARTUP; // default debug flags
static ULONG debug_classes = DBC_ERROR | DBC_DEBUG | DBC_WARNING | DBC_ASSERT | DBC_REPORT; // default debug classes

/****************************************************************************/

void SetupDebug(void)
{
  char var[256];

  kprintf("** openurl.prefs %s (%s) startup ****************************\n", LIB_REV_STRING, LIB_DATE);
  kprintf("Initializing runtime debugging:\n");

  if(GetVar("openurl.prefs.debug", var, sizeof(var), 0) > 0)
  {
    char *s = var;

    // static list of our debugging classes tokens.
    // in the yamdebug variable these classes always start with a @
    static struct { const char *token; unsigned long flag; } dbclasses[] =
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

    static struct { const char *token; unsigned long flag; } dbflags[] =
    {
      { "always",   DBF_ALWAYS  },
      { "startup",  DBF_STARTUP },
      { "all",      DBF_ALL     },
      { NULL,       0           }
    };

    // we parse the env variable token-wise
    while(*s)
    {
      ULONG i;
      char *e;

      if((e = strpbrk(s, " ,;")) == NULL)
        e = s+strlen(s);

      // check if the token is class definition or
      // just a flag definition
      if(s[0] == '@')
      {
        // skip the '@'
        s++;
        // check if this call is a negation or not
        if(s[0] == '!')
        {
          // skip the '!'
          s++;
          // search for the token and clear the flag
          for(i=0; dbclasses[i].token; i++)
          {
            if(strnicmp(s, dbclasses[i].token, strlen(dbclasses[i].token)) == 0)
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
            if(strnicmp(s, dbclasses[i].token, strlen(dbclasses[i].token)) == 0)
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
        if(s[0] == '!')
        {
          // skip the '!'
          s++;
          for(i=0; dbflags[i].token; i++)
          {
            if(strnicmp(s, dbflags[i].token, strlen(dbflags[i].token)) == 0)
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
          if(strnicmp(s, "ansi", 4) == 0)
          {
            kprintf("ansi output enabled\n");
            ansi_output = TRUE;
          }
          else
          {
            for(i=0; dbflags[i].token; i++)
            {
              if(strnicmp(s, dbflags[i].token, strlen(dbflags[i].token)) == 0)
              {
                kprintf("set '%s' debug flag\n", dbflags[i].token);
                SET_FLAG(debug_flags, dbflags[i].flag);
              }
            }
          }
        }
      }

      // set the next start to our last search
      if(*e)
        s = ++e;
      else
        break;
    }
  }

  kprintf("set debug classes/flags (env:openurl.prefs.debug): %08lx/%08lx\n", debug_classes, debug_flags);
  kprintf("** Normal processing follows ***************************************\n");
}

/****************************************************************************/

// define variables for using ANSI colors in our debugging scheme
#define ANSI_ESC_CLR        "\033[0m"
#define ANSI_ESC_BOLD       "\033[1m"
#define ANSI_ESC_UNDERLINE  "\033[4m"
#define ANSI_ESC_BLINK      "\033[5m"
#define ANSI_ESC_REVERSE    "\033[7m"
#define ANSI_ESC_INVISIBLE  "\033[8m"
#define ANSI_ESC_FG_BLACK   "\033[0;30m"
#define ANSI_ESC_FG_RED     "\033[0;31m"
#define ANSI_ESC_FG_GREEN   "\033[0;32m"
#define ANSI_ESC_FG_BROWN   "\033[0;33m"
#define ANSI_ESC_FG_BLUE    "\033[0;34m"
#define ANSI_ESC_FG_PURPLE  "\033[0;35m"
#define ANSI_ESC_FG_CYAN    "\033[0;36m"
#define ANSI_ESC_FG_LGRAY   "\033[0;37m"
#define ANSI_ESC_FG_DGRAY   "\033[1;30m"
#define ANSI_ESC_FG_LRED    "\033[1;31m"
#define ANSI_ESC_FG_LGREEN  "\033[1;32m"
#define ANSI_ESC_FG_YELLOW  "\033[1;33m"
#define ANSI_ESC_FG_LBLUE   "\033[1;34m"
#define ANSI_ESC_FG_LPURPLE "\033[1;35m"
#define ANSI_ESC_FG_LCYAN   "\033[1;36m"
#define ANSI_ESC_FG_WHITE   "\033[1;37m"
#define ANSI_ESC_BG         "\033[0;4"    // background esc-squ start with 4x
#define ANSI_ESC_BG_BLACK   "\033[0;40m"
#define ANSI_ESC_BG_RED     "\033[0;41m"
#define ANSI_ESC_BG_GREEN   "\033[0;42m"
#define ANSI_ESC_BG_BROWN   "\033[0;43m"
#define ANSI_ESC_BG_BLUE    "\033[0;44m"
#define ANSI_ESC_BG_PURPLE  "\033[0;45m"
#define ANSI_ESC_BG_CYAN    "\033[0;46m"
#define ANSI_ESC_BG_LGRAY   "\033[0;47m"

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
    const char *fmt;

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
    const char *fmt;

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
  if((isFlagSet(debug_classes, dclass) && isFlagSet(debug_flags, dflags)) ||
     (isFlagSet(dclass, DBC_ERROR) || isFlagSet(dclass, DBC_WARNING)))
  {
    va_list args;
    static char buf[1024];

    _INDENT();

    va_start(args, format);
    vsnprintf(buf, 1024, format, args);
    va_end(args);

    if(ansi_output)
    {
      const char *highlight = ANSI_ESC_FG_GREEN;

      switch(dclass)
      {
        case DBC_CTRACE:  highlight = ANSI_ESC_FG_BROWN; break;
        case DBC_REPORT:  highlight = ANSI_ESC_FG_GREEN; break;
        case DBC_ASSERT:  highlight = ANSI_ESC_FG_RED;   break;
        case DBC_TIMEVAL: highlight = ANSI_ESC_FG_GREEN; break;
        case DBC_DEBUG:   highlight = ANSI_ESC_FG_GREEN; break;
        case DBC_ERROR:   highlight = ANSI_ESC_FG_RED;   break;
        case DBC_WARNING: highlight = ANSI_ESC_FG_PURPLE;break;
      }

      kprintf("%s%s:%ld:%s%s\n", highlight, file, line, buf, ANSI_ESC_CLR);
    }
    else
      kprintf("%s:%ld:%s\n", file, line, buf);
  }
}

/****************************************************************************/

#endif
