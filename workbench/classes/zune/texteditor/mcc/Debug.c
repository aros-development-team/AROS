/***************************************************************************

 TextEditor.mcc - Textediting MUI Custom Class
 Copyright (C) 1997-2000 Allan Odgaard
 Copyright (C) 2005-2013 by TextEditor.mcc Open Source Team

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.

 TextEditor class Support Site:  http://www.sf.net/projects/texteditor-mcc

 $Id$

***************************************************************************/

#ifdef DEBUG

#include <stdio.h> // vsnprintf
#include <string.h>
#include <stdarg.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <clib/alib_protos.h>

#include "SDI_compiler.h"

#include "private.h"

#define DEBUG_USE_MALLOC_REDEFINE 1
#include "Debug.h"
#include "version.h"

#if defined(__MORPHOS__)
#include <exec/rawfmt.h>
#elif defined(__AROS__)
#include <proto/arossupport.h>
#else
#include <clib/debug_protos.h>
#endif

// our static variables with default values
static int indent_level = 0;
static BOOL ansi_output = FALSE;
static ULONG debug_flags = DBF_ALWAYS | DBF_STARTUP; // default debug flags
static ULONG debug_classes = DBC_ERROR | DBC_DEBUG | DBC_WARNING | DBC_ASSERT | DBC_REPORT | DBC_MTRACK; // default debug classes

static void SetupDbgMalloc(void);
static void CleanupDbgMalloc(void);

/****************************************************************************/

void _DBPRINTF(const char *format, ...)
{
  va_list args;

  va_start(args, format);

  {
  #if defined(__MORPHOS__)
  VNewRawDoFmt(format, (APTR)RAWFMTFUNC_SERIAL, NULL, args);
  #elif defined(__amigaos4__)
  static char buf[1024];
  vsnprintf(buf, 1024, format, args);
  DebugPrintF("%s", buf);
  #elif defined(__AROS__)
  vkprintf(format, args);
  #else
  KPutFmt(format, args);
  #endif
  }

  va_end(args);
}

/****************************************************************************/

void SetupDebug(void)
{
  char var[256];

  _DBPRINTF("** TextEditor.mcc v" LIB_REV_STRING " startup **********************\n");
  _DBPRINTF("Exec version: v%ld.%ld\n", ((struct Library *)SysBase)->lib_Version, ((struct Library *)SysBase)->lib_Revision);
  _DBPRINTF("Initializing runtime debugging:\n");

  if(GetVar("texteditor.mcc.debug", var, sizeof(var), 0) > 0)
  {
    char *s = var;

    // static list of our debugging classes tokens.
    // in the texteditor.mcc.debug variable these classes always start with a @
    static const struct { const char *token; unsigned long flag; } dbclasses[] =
    {
      { "ctrace",  DBC_CTRACE   },
      { "report",  DBC_REPORT   },
      { "assert",  DBC_ASSERT   },
      { "timeval", DBC_TIMEVAL  },
      { "debug",   DBC_DEBUG    },
      { "error",   DBC_ERROR    },
      { "warning", DBC_WARNING  },
      { "mtrack",  DBC_MTRACK   },
      { "all",     DBC_ALL      },
      { NULL,      0            }
    };

    static const struct { const char *token; unsigned long flag; } dbflags[] =
    {
      { "always",    DBF_ALWAYS    },
      { "startup",   DBF_STARTUP   },
      { "input",     DBF_INPUT     },
      { "rexx",      DBF_REXX      },
      { "clipboard", DBF_CLIPBOARD },
      { "undo",      DBF_UNDO      },
      { "dump",      DBF_DUMP      },
      { "style",     DBF_STYLE     },
      { "spell",     DBF_SPELL     },
      { "block",     DBF_BLOCK     },
      { "import",    DBF_IMPORT    },
      { "export",    DBF_IMPORT    },
      { "all",       DBF_ALL       },
      { NULL,        0             }
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
        // check if this call is a negation or not
        if(s[1] == '!')
        {
          // search for the token and clear the flag
          for(i=0; dbclasses[i].token; i++)
          {
            if(strnicmp(&s[2], dbclasses[i].token, strlen(dbclasses[i].token)) == 0)
            {
              _DBPRINTF("clear '%s' debug class flag.\n", dbclasses[i].token);
              clearFlag(debug_classes, dbclasses[i].flag);
            }
          }
        }
        else
        {
          // search for the token and set the flag
          for(i=0; dbclasses[i].token; i++)
          {
            if(strnicmp(&s[1], dbclasses[i].token, strlen(dbclasses[i].token)) == 0)
            {
              _DBPRINTF("set '%s' debug class flag\n", dbclasses[i].token);
              setFlag(debug_classes, dbclasses[i].flag);
            }
          }
        }
      }
      else
      {
        // check if this call is a negation or not
        if(s[0] == '!')
        {
          for(i=0; dbflags[i].token; i++)
          {
            if(strnicmp(&s[1], dbflags[i].token, strlen(dbflags[i].token)) == 0)
            {
              _DBPRINTF("clear '%s' debug flag\n", dbflags[i].token);
              clearFlag(debug_flags, dbflags[i].flag);
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
                setFlag(debug_flags, dbflags[i].flag);
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

  _DBPRINTF("set debug classes/flags (env:texteditor.mcc.debug): %08lx/%08lx\n", debug_classes, debug_flags);
  _DBPRINTF("** Normal processing follows ***************************************\n");

  SetupDbgMalloc();
}

/****************************************************************************/

void CleanupDebug(void)
{
  CleanupDbgMalloc();

  _DBPRINTF("** Cleaned up debugging ********************************************\n");
}

/****************************************************************************/

// define variables for using ANSI colors in our debugging scheme
#define ANSI_ESC_CLR        "\033[0m"
#define ANSI_ESC_BOLD        "\033[1m"
#define ANSI_ESC_UNDERLINE  "\033[4m"
#define ANSI_ESC_BLINK      "\033[5m"
#define ANSI_ESC_REVERSE    "\033[7m"
#define ANSI_ESC_INVISIBLE  "\033[8m"
#define ANSI_ESC_FG_BLACK    "\033[0;30m"
#define ANSI_ESC_FG_RED      "\033[0;31m"
#define ANSI_ESC_FG_GREEN    "\033[0;32m"
#define ANSI_ESC_FG_BROWN    "\033[0;33m"
#define ANSI_ESC_FG_BLUE    "\033[0;34m"
#define ANSI_ESC_FG_PURPLE  "\033[0;35m"
#define ANSI_ESC_FG_CYAN    "\033[0;36m"
#define ANSI_ESC_FG_LGRAY    "\033[0;37m"
#define ANSI_ESC_FG_DGRAY    "\033[1;30m"
#define ANSI_ESC_FG_LRED    "\033[1;31m"
#define ANSI_ESC_FG_LGREEN  "\033[1;32m"
#define ANSI_ESC_FG_YELLOW  "\033[1;33m"
#define ANSI_ESC_FG_LBLUE    "\033[1;34m"
#define ANSI_ESC_FG_LPURPLE  "\033[1;35m"
#define ANSI_ESC_FG_LCYAN    "\033[1;36m"
#define ANSI_ESC_FG_WHITE    "\033[1;37m"
#define ANSI_ESC_BG          "\033[0;4"    // background esc-squ start with 4x
#define ANSI_ESC_BG_BLACK    "\033[0;40m"
#define ANSI_ESC_BG_RED      "\033[0;41m"
#define ANSI_ESC_BG_GREEN    "\033[0;42m"
#define ANSI_ESC_BG_BROWN    "\033[0;43m"
#define ANSI_ESC_BG_BLUE    "\033[0;44m"
#define ANSI_ESC_BG_PURPLE  "\033[0;45m"
#define ANSI_ESC_BG_CYAN    "\033[0;46m"
#define ANSI_ESC_BG_LGRAY    "\033[0;47m"

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

void _DPRINTF(unsigned long dclass, unsigned long dflags, const char *file, unsigned long line, const char *format, ...)
{
  if((isFlagSet(debug_classes, dclass) && isFlagSet(debug_flags, dflags)) ||
     (isFlagSet(dclass, DBC_ERROR) || isFlagSet(dclass, DBC_WARNING)))
  {
    va_list args;

    va_start(args, format);
    _VDPRINTF(dclass, dflags, file, line, format, args);
    va_end(args);
  }
}

/****************************************************************************/

void _VDPRINTF(unsigned long dclass, unsigned long dflags, const char *file, unsigned long line, const char *format, va_list args)
{
  if((isFlagSet(debug_classes, dclass) && isFlagSet(debug_flags, dflags)) ||
     (isFlagSet(dclass, DBC_ERROR) || isFlagSet(dclass, DBC_WARNING)))
  {
    static char buf[1024];

    _INDENT();

    vsnprintf(buf, 1024, format, args);

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

struct DbgMallocNode
{
  struct MinNode node;
  void *memory;
  size_t size;
  const char *file;
  const char *func;
  int line;
};

static struct MinList DbgMallocList[256];
static struct SignalSemaphore DbgMallocListSema;
static ULONG DbgMallocCount;
static ULONG DbgUnsuitableFreeCount;

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(a)         (sizeof(a) / sizeof(a[0]))
#endif

#ifndef NewList
#endif

// a very simple hashing function to spread the allocations across the lists
// Since AmigaOS memory allocation has a granularity of at least 8 bytes we simple ignore the
// lower 4 bits (=16 Bytes) and take the next 8 bits as hash value. Not very sophisticated, but
// it does the job quite good.
#define ptr2hash(p)           ((((ULONG)(p)) >> 4) & 0xff)

/// findDbgMallocNode
// find a given pointer in the tracking lists
static struct DbgMallocNode *findDbgMallocNode(const void *ptr)
{
  struct DbgMallocNode *result = NULL;
  struct Node *curNode;

  for(curNode = GetHead((struct List *)&DbgMallocList[ptr2hash(ptr)]); curNode != NULL; curNode = GetSucc(curNode))
  {
    struct DbgMallocNode *dmn = (struct DbgMallocNode *)curNode;

    if(dmn->memory == ptr)
    {
      result = dmn;
      break;
    }
  }

  return result;
}

///
/// matchAllocFunc
// check wether the used allocation function matches a set of given function names
// separated by '|', i.e. "malloc|calloc|strdup"
BOOL matchAllocFunc(const char *allocFunc, const char *freeFunc)
{
  BOOL match = FALSE;
  const char *p = freeFunc;
  const char *q;

  while((q = strchr(p, '|')) != NULL)
  {
    char tmp[16];

    // we have to handle more than one possible function name
    strlcpy(tmp, p, ((size_t)(q-p)+1 < sizeof(tmp)) ? (size_t)(q-p)+1 : sizeof(tmp));
    if(strcmp(allocFunc, tmp) == 0)
    {
      match = TRUE;
      break;
    }
    p = q+1;
  }

  if(match == FALSE)
  {
    // compare the last or only function name
    match = (strcmp(allocFunc, p) == 0);
  }

  return match;
}

///
/// _MEMTRACK
// add a new node to the memory tracking lists
void _MEMTRACK(const char *file, const int line, const char *func, void *ptr, size_t size)
{
  if(isFlagSet(debug_classes, DBC_MTRACK))
  {
    if(ptr != NULL && size != 0)
    {
      struct DbgMallocNode *dmn;

      if((dmn = AllocVec(sizeof(*dmn), MEMF_ANY)) != NULL)
      {
        dmn->memory = ptr;
        dmn->size = size;
        dmn->file = file;
        dmn->line = line;
        dmn->func = func;

        ObtainSemaphore(&DbgMallocListSema);

        AddTail((struct List *)&DbgMallocList[ptr2hash(ptr)], (struct Node *)&dmn->node);
        DbgMallocCount++;

        ReleaseSemaphore(&DbgMallocListSema);
      }
    }
    else
      _DPRINTF(DBC_WARNING, DBF_ALWAYS, file, line, "potential invalid %s call with return (0x%08lx, 0x%08lx)", func, ptr, size);
  }
}

///
/// _UNMEMTRACK
// remove a node from the memory tracking lists
void _UNMEMTRACK(const char *file, const int line, const char *func, const void *ptr)
{
  if(isFlagSet(debug_classes, DBC_MTRACK) && ptr != NULL)
  {
    BOOL success = FALSE;
    struct DbgMallocNode *dmn;

    ObtainSemaphore(&DbgMallocListSema);

    if((dmn = findDbgMallocNode(ptr)) != NULL)
    {
      Remove((struct Node *)dmn);

      if(matchAllocFunc(dmn->func, func) == FALSE)
      {
        _DPRINTF(DBC_WARNING, DBF_ALWAYS, file, line, "free of tracked memory area 0x%08lx with unsuitable function (allocated with %s, freed with %s counterpart)", ptr, dmn->func, func);
        DbgUnsuitableFreeCount++;
      }

      FreeVec(dmn);

      DbgMallocCount--;

      success = TRUE;
    }

    if(success == FALSE)
      _DPRINTF(DBC_WARNING, DBF_ALWAYS, file, line, "free of untracked memory area 0x%08lx attempted", ptr);

    ReleaseSemaphore(&DbgMallocListSema);
  }
}

///
/// SetupDbgMalloc
// initialize the memory tracking framework
static void SetupDbgMalloc(void)
{
  ENTER();

  if(isFlagSet(debug_classes, DBC_MTRACK))
  {
    ULONG i;

    for(i = 0; i < ARRAY_SIZE(DbgMallocList); i++)
      NewList((struct List *)&DbgMallocList[i]);

    DbgMallocCount = 0;
    DbgUnsuitableFreeCount = 0;

    memset(&DbgMallocListSema, 0, sizeof(DbgMallocListSema));
    InitSemaphore(&DbgMallocListSema);
  }

  LEAVE();
}

///
/// CleanupDbgMalloc
// cleanup the memory tracking framework and output possibly pending allocations
static void CleanupDbgMalloc(void)
{
  ENTER();

  if(isFlagSet(debug_classes, DBC_MTRACK))
  {
    _DBPRINTF("** Cleaning up memory tracking *************************************\n");

    ObtainSemaphore(&DbgMallocListSema);

    if(DbgMallocCount != 0 || DbgUnsuitableFreeCount != 0)
    {
      if(DbgMallocCount != 0)
      {
        ULONG i;

        E(DBF_ALWAYS, "there are still %ld unfreed memory trackings", DbgMallocCount);
        for(i = 0; i < ARRAY_SIZE(DbgMallocList); i++)
        {
          struct DbgMallocNode *dmn;

          while((dmn = (struct DbgMallocNode *)RemHead((struct List *)&DbgMallocList[i])) != NULL)
          {
            _DPRINTF(DBC_ERROR, DBF_ALWAYS, dmn->file, dmn->line, "unfreed memory tracking: 0x%08lx, size/type %ld, func (%s)", dmn->memory, dmn->size, dmn->func);

            // We only free the node structure here but not dmn->memory itself.
            // First of all, this is because the allocation could have been done
            // by other functions than malloc() and calling free() for these will
            // cause havoc. And second the c-library's startup code will/should
            // free all further pending allocations upon program termination.
            FreeVec(dmn);
          }
        }
      }
      if(DbgUnsuitableFreeCount != 0)
      {
        E(DBF_ALWAYS, "there were %ld unsuitable freeing calls", DbgUnsuitableFreeCount);
      }
    }
    else
      D(DBF_ALWAYS, "all memory trackings have been free()'d correctly");

    ReleaseSemaphore(&DbgMallocListSema);
  }

  LEAVE();
}

///
/// DumpDbgMalloc
// output all current allocations
void DumpDbgMalloc(void)
{
  ENTER();

  if(isFlagSet(debug_classes, DBC_MTRACK))
  {
    ULONG i;

    ObtainSemaphore(&DbgMallocListSema);

    D(DBF_ALWAYS, "%ld memory areas tracked", DbgMallocCount);
    for(i = 0; i < ARRAY_SIZE(DbgMallocList); i++)
    {
      struct Node *curNode;

      for(curNode = GetHead((struct List *)&DbgMallocList[i]); curNode != NULL; curNode = GetSucc(curNode))
      {
        struct DbgMallocNode *dmn = (struct DbgMallocNode *)curNode;

        _DPRINTF(DBC_MTRACK, DBF_ALWAYS, dmn->file, dmn->line, "memarea 0x%08lx, size/type %ld, func (%s)", dmn->memory, dmn->size, dmn->func);
      }
    }

    ReleaseSemaphore(&DbgMallocListSema);
  }

  LEAVE();
}

///

#endif /* DEBUG */
