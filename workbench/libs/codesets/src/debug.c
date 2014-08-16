/***************************************************************************

 codesets.library - Amiga shared library for handling different codesets
 Copyright (C) 2001-2005 by Alfonso [alfie] Ranieri <alforan@tin.it>.
 Copyright (C) 2005-2014 codesets.library Open Source Team

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.

 codesets.library project: http://sourceforge.net/projects/codesetslib/

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
#include <exec/semaphores.h>

#include "version.h"

#include "debug.h"
#include "macros.h"

#if defined(__MORPHOS__) || defined(__AROS__)
#include <exec/rawfmt.h>
#else
#include <clib/debug_protos.h>
#endif

// our static variables with default values
static int indent_level = 0;
static BOOL ansi_output = FALSE;
static ULONG debug_flags = DBF_ALWAYS | DBF_STARTUP; // default debug flags
static ULONG debug_classes = DBC_ERROR | DBC_DEBUG | DBC_WARNING | DBC_ASSERT | DBC_REPORT; // default debug classes
static struct SignalSemaphore debug_sema;

/****************************************************************************/

void _DBPRINTF(const char *format, ...)
{
  va_list args;
  #if defined(__amigaos4__)
  static char buf[1024];
  #endif

  va_start(args, format);

  ObtainSemaphore(&debug_sema);

  #if defined(__MORPHOS__) || defined(__AROS__)
  VNewRawDoFmt(format, (APTR)RAWFMTFUNC_SERIAL, NULL, args);
  #elif defined(__amigaos4__)
  vsnprintf(buf, sizeof(buf), format, args);
  DebugPrintF("%s", buf);
  #else
  KPutFmt(format, args);
  #endif

  ReleaseSemaphore(&debug_sema);

  va_end(args);
}

/****************************************************************************/

void InitDebug(void)
{
  memset(&debug_sema, 0, sizeof(debug_sema));
  InitSemaphore(&debug_sema);
}

/****************************************************************************/

void SetupDebug(void)
{
  char var[256];

  _DBPRINTF("** codesets.library %s (%s) startup ****************************\n", LIB_REV_STRING, LIB_DATE);
  _DBPRINTF("Initializing runtime debugging:\n");

  if(GetVar("codesets.library.debug", var, sizeof(var), 0) > 0)
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
      { "utf",      DBF_UTF     },
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
              _DBPRINTF("clear '%s' debug class flag.\n", dbclasses[i].token);
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
              _DBPRINTF("set '%s' debug class flag\n", dbclasses[i].token);
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
              _DBPRINTF("clear '%s' debug flag\n", dbflags[i].token);
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
            _DBPRINTF("ansi output enabled\n");
            ansi_output = TRUE;
          }
          else
          {
            for(i=0; dbflags[i].token; i++)
            {
              if(strnicmp(s, dbflags[i].token, strlen(dbflags[i].token)) == 0)
              {
                _DBPRINTF("set '%s' debug flag\n", dbflags[i].token);
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

  _DBPRINTF("set debug classes/flags (env:codesets.library.debug): %08lx/%08lx\n", debug_classes, debug_flags);
  _DBPRINTF("** Normal processing follows ***************************************\n");
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
    _DBPRINTF(" ");
}

/****************************************************************************/

void _ENTER(unsigned long dclass, const char *file, int line, const char *function)
{
  if(isFlagSet(debug_classes, dclass))
  {
    _INDENT();
    if(ansi_output)
      _DBPRINTF("%s%s:%ld:Entering %s%s\n", ANSI_ESC_FG_BROWN, file, line, function, ANSI_ESC_CLR);
    else
      _DBPRINTF("%s:%ld:Entering %s\n", file, line, function);
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
      _DBPRINTF("%s%s:%ld:Leaving %s%s\n", ANSI_ESC_FG_BROWN, file, line, function, ANSI_ESC_CLR);
    else
      _DBPRINTF("%s:%ld:Leaving %s\n", file, line, function);
  }
}

void _RETURN(unsigned long dclass, const char *file, int line, const char *function, unsigned long result)
{
  indent_level--;

  if(isFlagSet(debug_classes, dclass))
  {
    _INDENT();
    if(ansi_output)
      _DBPRINTF("%s%s:%ld:Leaving %s (result 0x%08lx, %ld)%s\n", ANSI_ESC_FG_BROWN, file, line, function, result, result, ANSI_ESC_CLR);
    else
      _DBPRINTF("%s:%ld:Leaving %s (result 0x%08lx, %ld)\n", file, line, function, result, result);
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
      _DBPRINTF(ANSI_ESC_FG_GREEN);

    _DBPRINTF(fmt, file, line, name, value, value);

    if(size == 1 && value < 256)
    {
      if(value < ' ' || (value >= 127 && value < 160))
        _DBPRINTF(", '\\x%02lx'", value);
      else
        _DBPRINTF(", '%lc'", value);
    }

    if(ansi_output)
      _DBPRINTF("%s\n", ANSI_ESC_CLR);
    else
      _DBPRINTF("\n");
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
      _DBPRINTF(ANSI_ESC_FG_GREEN);
      _DBPRINTF(fmt, file, line, name, p);
      _DBPRINTF(ANSI_ESC_CLR);
    }
    else
      _DBPRINTF(fmt, file, line, name, p);
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
      _DBPRINTF("%s%s:%ld:%s = 0x%08lx \"%s\"%s\n", ANSI_ESC_FG_GREEN, file, line, name, string, string, ANSI_ESC_CLR);
    else
      _DBPRINTF("%s:%ld:%s = 0x%08lx \"%s\"\n", file, line, name, string, string);
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
      _DBPRINTF("%s%s:%ld:%s%s\n", ANSI_ESC_FG_GREEN, file, line, msg, ANSI_ESC_CLR);
    else
      _DBPRINTF("%s:%ld:%s\n", file, line, msg);
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

      _DBPRINTF("%s%s:%ld:%s%s\n", highlight, file, line, buf, ANSI_ESC_CLR);
    }
    else
      _DBPRINTF("%s:%ld:%s\n", file, line, buf);
  }
}

/****************************************************************************/

#endif
