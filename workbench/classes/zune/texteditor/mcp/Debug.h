/***************************************************************************

 TextEditor.mcc - Textediting MUI Custom Class
 Copyright (C) 1997-2000 Allan Odgaard
 Copyright (C) 2005-2014 TextEditor.mcc Open Source Team

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

#ifndef DEBUG_H
#define DEBUG_H

// first we make sure all previously defined symbols are undefined now so
// that no other debug system interferes with ours.
#undef ENTER
#undef LEAVE
#undef RETURN
#undef SHOWVALUE
#undef SHOWPOINTER
#undef SHOWSTRING
#undef SHOWMSG
#undef STARTCLOCK
#undef STOPCLOCK
#undef MEMTRACK
#undef UNMEMTRACK
#undef D
#undef E
#undef W
#undef ASSERT

#if defined(DEBUG)

#include <string.h>
#include <stdarg.h>
#include <stddef.h>
#include <assert.h>

// debug classes
#define DBC_CTRACE    (1<<0) // call tracing (ENTER/LEAVE etc.)
#define DBC_REPORT    (1<<1) // reports (SHOWVALUE/SHOWSTRING etc.)
#define DBC_ASSERT    (1<<2) // asserts (ASSERT)
#define DBC_TIMEVAL   (1<<3) // time evaluations (STARTCLOCK/STOPCLOCK)
#define DBC_DEBUG     (1<<4) // debugging output D()
#define DBC_ERROR     (1<<5) // error output     E()
#define DBC_WARNING   (1<<6) // warning output   W()
#define DBC_MTRACK    (1<<7) // memory tracking MEMTRACK/UNMEMTRACK()
#define DBC_ALL       0xffffffff

// debug flags
#define DBF_ALWAYS    (1<<0)
#define DBF_STARTUP   (1<<1)     // for startup/shutdown events
#define DBF_ALL       0xffffffff

void SetupDebug(void);
void CleanupDebug(void);
void DumpDbgMalloc(void);

void _ENTER(unsigned long dclass, const char *file, int line, const char *function);
void _LEAVE(unsigned long dclass, const char *file, int line, const char *function);
void _RETURN(unsigned long dclass, const char *file, int line, const char *function, unsigned long result);
void _SHOWVALUE(unsigned long dclass, unsigned long dflags, unsigned long value, int size, const char *name, const char *file, int line);
void _SHOWPOINTER(unsigned long dclass, unsigned long dflags, const void *p, const char *name, const char *file, int line);
void _SHOWSTRING(unsigned long dclass, unsigned long dflags, const char *string, const char *name, const char *file, int line);
void _SHOWMSG(unsigned long dclass, unsigned long dflags, const char *msg, const char *file, int line);
void _DPRINTF(unsigned long dclass, unsigned long dflags, const char *file, unsigned long line, const char *format, ...);
void _VDPRINTF(unsigned long dclass, unsigned long dflags, const char *file, unsigned long line, const char *format, va_list args);
void _MEMTRACK(const char *file, const int line, const char *func, void *ptr, size_t size);
void _UNMEMTRACK(const char *file, const int line, const char *func, const void *ptr);

// Core class information class messages
#define ENTER()               _ENTER(DBC_CTRACE, __FILE__, __LINE__, __FUNCTION__)
#define LEAVE()               _LEAVE(DBC_CTRACE, __FILE__, __LINE__, __FUNCTION__)
#define RETURN(r)             _RETURN(DBC_CTRACE, __FILE__, __LINE__, __FUNCTION__, (long)r)
#define SHOWVALUE(f, v)       _SHOWVALUE(DBC_REPORT, f, (long)v, sizeof(v), #v, __FILE__, __LINE__)
#define SHOWPOINTER(f, p)     _SHOWPOINTER(DBC_REPORT, f, p, #p, __FILE__, __LINE__)
#define SHOWSTRING(f, s)      _SHOWSTRING(DBC_REPORT, f, s, #s, __FILE__, __LINE__)
#define SHOWMSG(f, m)         _SHOWMSG(DBC_REPORT, f, m, __FILE__, __LINE__)
#define MEMTRACK(f, p, s)     _MEMTRACK(__FILE__, __LINE__, f, p, s)
#define UNMEMTRACK(f, p)      _UNMEMTRACK(__FILE__, __LINE__, f, p)
#define D(f, ...)             _DPRINTF(DBC_DEBUG, f, __FILE__, __LINE__, __VA_ARGS__)
#define E(f, ...)             _DPRINTF(DBC_ERROR, f, __FILE__, __LINE__, __VA_ARGS__)
#define W(f, ...)             _DPRINTF(DBC_WARNING, f, __FILE__, __LINE__, __VA_ARGS__)
#define ASSERT(expression)      \
  ((void)                       \
   ((expression) ? 0 :          \
    (                            \
     _DPRINTF(DBC_ASSERT,       \
              DBF_ALWAYS,       \
              __FILE__,         \
              __LINE__,         \
              "failed assertion '%s'", \
              #expression),     \
     assert(#expression),       \
     0                          \
    )                           \
   )                            \
  )

#if !defined(DEBUG_USE_MALLOC_REDEFINE) && !defined(__SASC) && !defined(__VBCC__)

// standard C-library memory functions
#define malloc(s)               ({void *P = malloc(s);     _MEMTRACK(__FILE__, __LINE__, "malloc", P, s); P;})
#define calloc(n, s)            ({void *P = calloc(n, s);  _MEMTRACK(__FILE__, __LINE__, "calloc", P, s); P;})
#define realloc(p, s)           ({void *P; _UNMEMTRACK(__FILE__, __LINE__, "malloc|calloc|strdup|memdup|asprintf", p); P = realloc(p, s); _MEMTRACK(__FILE__, __LINE__, "realloc", P, s); P;})
#define strdup(s)               ({char *P = strdup(s);     _MEMTRACK(__FILE__, __LINE__, "strdup", P, strlen(s)+1); P;})
#define memdup(p, s)            ({void *P = memdup(p, s);  _MEMTRACK(__FILE__, __LINE__, "memdup", P, s); P;})
#define asprintf(p, f, ...)     ({int P = asprintf(p, f, __VA_ARGS__); _MEMTRACK(__FILE__, __LINE__, "asprintf", *(p), 1); P;})
#define free(p)                 ({_UNMEMTRACK(__FILE__, __LINE__, "malloc|calloc|strdup|memdup|asprintf", p); free(p);})

// standard C-library IO functions
#define fopen(f, m)             ({FILE *P = fopen(f, m); _MEMTRACK(__FILE__, __LINE__, "fopen", P, 1); P;})
#define fclose(p)               ({int P; _UNMEMTRACK(__FILE__, __LINE__, "fopen", p); P = fclose(p); P;})

#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/graphics.h>

// memory tracking of internal AmigaOS functions
#if !defined(__AROS__)
#undef AllocPooled
#undef FreePooled
#undef AllocVecPooled
#undef FreeVecPooled
#undef AllocDosObject
#undef AllocDosObjectTags
#undef FreeDosObject
#undef AllocSysObject
#undef AllocSysObjectTags
#undef FreeSysObject
#undef ExamineObject
#undef ExamineObjectTags
#undef AllocBitMap
#undef FreeBitMap
#undef ObtainDirContext
#undef ObtainDirContextTags
#undef ReleaseDirContext
#undef AllocSignal
#undef FreeSignal
#undef StartNotify
#undef EndNotify
#endif

#if defined(__amigaos4__)

#define AllocPooled(p, s)             ({APTR P = IExec->AllocPooled(p, s); _MEMTRACK(__FILE__, __LINE__, "AllocPooled", P, s); P;})
#define FreePooled(p, m, s)           ({_UNMEMTRACK(__FILE__, __LINE__, "AllocPooled", m); IExec->FreePooled(p, m, s);})
#define AllocVecPooled(p, s)          ({APTR P = IExec->AllocVecPooled(p, s); _MEMTRACK(__FILE__, __LINE__, "AllocVecPooled", P, s); P;})
#define FreeVecPooled(p, m)           ({_UNMEMTRACK(__FILE__, __LINE__, "AllocVecPooled", m); IExec->FreeVecPooled(p, m);})
#define AllocDosObject(t, p)          ({APTR P = IDOS->AllocDosObject(t, p); _MEMTRACK(__FILE__, __LINE__, "AllocDosObject", P, t+1); P;})
#define AllocDosObjectTags(t, ...)    ({APTR P = IDOS->AllocDosObjectTags(t, __VA_ARGS__); _MEMTRACK(__FILE__, __LINE__, "AllocDosObjectTags", P, t+1); P;})
#define ExamineObject(t)              ({APTR P = IDOS->ExamineObject(t); _MEMTRACK(__FILE__, __LINE__, "ExamineObject", P, 1); P;})
#define ExamineObjectTags(t, ...)     ({APTR P = IDOS->ExamineObjectTags(t, __VA_ARGS__); _MEMTRACK(__FILE__, __LINE__, "ExamineObjectTags", P, 1); P;})
#define FreeDosObject(t, p)           ({_UNMEMTRACK(__FILE__, __LINE__, "AllocDosObject|AllocDosObjectTags|ExamineObject|ExamineObjectTags", p); IDOS->FreeDosObject(t, p);})
#define AllocSysObject(t, p)          ({APTR P = IExec->AllocSysObject(t, p); _MEMTRACK(__FILE__, __LINE__, "AllocSysObject", P, t+1); P;})
#define AllocSysObjectTags(t, ...)    ({APTR P = IExec->AllocSysObjectTags(t, __VA_ARGS__); _MEMTRACK(__FILE__, __LINE__, "AllocSysObjectTags", P, t+1); P;})
#define FreeSysObject(t, p)           ({_UNMEMTRACK(__FILE__, __LINE__, "AllocSysObject|AllocSysObjectTags", p); IExec->FreeSysObject(t, p);})
#define AllocBitMap(sx, sy, d, f, bm) ({APTR P = IGraphics->AllocBitMap(sx, sy, d, f, bm); _MEMTRACK(__FILE__, __LINE__, "AllocBitMap", P, sx); P;})
#define FreeBitMap(p)                 ({_UNMEMTRACK(__FILE__, __LINE__, "AllocBitMap", p); IGraphics->FreeBitMap(p);})
#define ObtainDirContext(t)           ({APTR P = IDOS->ObtainDirContext(t); _MEMTRACK(__FILE__, __LINE__, "ObtainDirContextTags", P, 1); P;})
#define ObtainDirContextTags(...)     ({APTR P = IDOS->ObtainDirContextTags(__VA_ARGS__); _MEMTRACK(__FILE__, __LINE__, "ObtainDirContextTags", P, 1); P;})
#define ReleaseDirContext(p)          ({_UNMEMTRACK(__FILE__, __LINE__, "ObtainDirContext|ObtainDirContextTags", p); IDOS->ReleaseDirContext(p);})
#define AllocSignal(s)                ({BYTE P = IExec->AllocSignal(s); _MEMTRACK(__FILE__, __LINE__, "AllocSignal", (APTR)(LONG)P, (size_t)s); P;})
#define FreeSignal(s)                 ({_UNMEMTRACK(__FILE__, __LINE__, "AllocSignal", (APTR)s); IExec->FreeSignal(s);})
#define StartNotify(p)                ({LONG P = IDOS->StartNotify(p); _MEMTRACK(__FILE__, __LINE__, "StartNotify", p, (size_t)p); P;})
#define EndNotify(p)                  ({_UNMEMTRACK(__FILE__, __LINE__, "StartNotify", p); IDOS->EndNotify(p);})

#elif defined(__MORPHOS__)

#define AllocPooled(__p0, __p1) ({ \
  APTR P = LP2(708, APTR , AllocPooled, \
    APTR , __p0, a0, \
    ULONG , __p1, d0, \
    , EXEC_BASE_NAME, 0, 0, 0, 0, 0, 0); \
   _MEMTRACK(__FILE__, __LINE__, "AllocPooled", P, __p1); \
   P; \
})

#define FreePooled(__p0, __p1, __p2) ({ \
   _UNMEMTRACK(__FILE__, __LINE__, "AllocPooled", __p1); \
   LP3NR(714, FreePooled, \
      APTR , __p0, a0, \
      APTR , __p1, a1, \
      ULONG , __p2, d0, \
      , EXEC_BASE_NAME, 0, 0, 0, 0, 0, 0); \
})

#define AllocVecPooled(__p0, __p1) ({ \
  APTR P = LP2(894, APTR , AllocVecPooled, \
    APTR , __p0, a0, \
    ULONG , __p1, d0, \
    , EXEC_BASE_NAME, 0, 0, 0, 0, 0, 0); \
   _MEMTRACK(__FILE__, __LINE__, "AllocVecPooled", P, __p1); \
   P; \
})

#define FreeVecPooled(__p0, __p1) ({ \
   _UNMEMTRACK(__FILE__, __LINE__, "AllocVecPooled", __p1); \
  LP2NR(900, FreeVecPooled, \
    APTR , __p0, a0, \
    APTR , __p1, a1, \
    , EXEC_BASE_NAME, 0, 0, 0, 0, 0, 0); \
})

#define AllocDosObject(__p0, __p1) ({ \
  APTR P = LP2(228, APTR , AllocDosObject, \
    ULONG , __p0, d1, \
    CONST struct TagItem *, __p1, d2, \
    , DOS_BASE_NAME, 0, 0, 0, 0, 0, 0); \
  _MEMTRACK(__FILE__, __LINE__, "AllocDosObject", P, __p0); \
  P; \
})

#define FreeDosObject(__p0, __p1) ({ \
   _UNMEMTRACK(__FILE__, __LINE__, "AllocDosObject|AllocDosObjectTags", __p1); \
  LP2NR(234, FreeDosObject, \
    ULONG , __p0, d1, \
    APTR , __p1, d2, \
    , DOS_BASE_NAME, 0, 0, 0, 0, 0, 0); \
})

#define AllocSysObject(t, p) ({APTR P = AllocSysObject(t, p); _MEMTRACK(__FILE__, __LINE__, "AllocSysObject", P, t+1); P;})

#define AllocSysObjectTags(t, ...) ({ \
   ULONG _tags[] = { __VA_ARGS__ }; \
   APTR P = AllocSysObject(t, (struct TagItem *)_tags); \
   _MEMTRACK(__FILE__, __LINE__, "AllocSysObjectTags", P, t+1); \
   P; \
})

#define FreeSysObject(t, p) ({_UNMEMTRACK(__FILE__, __LINE__, "AllocSysObject|AllocSysObjectTags", p); FreeSysObject(t, p);})

#define AllocBitMap(__p0, __p1, __p2, __p3, __p4) ({ \
  APTR P = LP5(918, struct BitMap *, AllocBitMap, \
    ULONG , __p0, d0, \
    ULONG , __p1, d1, \
    ULONG , __p2, d2, \
    ULONG , __p3, d3, \
    CONST struct BitMap *, __p4, a0, \
    , GRAPHICS_BASE_NAME, 0, 0, 0, 0, 0, 0); \
   _MEMTRACK(__FILE__, __LINE__, "AllocBitMap", P, __p0); \
   P; \
})

#define FreeBitMap(__p0) ({ \
   _UNMEMTRACK(__FILE__, __LINE__, "AllocBitMap", __p0); \
  LP1NR(924, FreeBitMap, \
    struct BitMap *, __p0, a0, \
    , GRAPHICS_BASE_NAME, 0, 0, 0, 0, 0, 0); \
})

#define ObtainDirContext(t) ({APTR P = ObtainDirContext(t); _MEMTRACK(__FILE__, __LINE__, "ObtainDirContextTags", P, 1); P;})

#define ObtainDirContextTags(...) ({ \
   ULONG _tags[] = { __VA_ARGS__ }; \
   APTR P = ObtainDirContext((struct TagItem *)_tags); \
   _MEMTRACK(__FILE__, __LINE__, "ObtainDirContextTags", P, 1); \
   P; \
})

#define ReleaseDirContext(p) ({_UNMEMTRACK(__FILE__, __LINE__, "ObtainDirContext|ObtainDirContextTags", p); ReleaseDirContext(p);})

#define AllocSignal(__p0) ({ \
   BYTE P = LP1(330, BYTE , AllocSignal, \
      LONG , __p0, d0, \
      , EXEC_BASE_NAME, 0, 0, 0, 0, 0, 0); \
   _MEMTRACK(__FILE__, __LINE__, "AllocSignal", (APTR)(LONG)P, (size_t)__p0); \
   P; \
})

#define FreeSignal(__p0) ({ \
   _UNMEMTRACK(__FILE__, __LINE__, "AllocSignal", (APTR)__p0); \
   LP1NR(336, FreeSignal, \
      LONG , __p0, d0, \
      , EXEC_BASE_NAME, 0, 0, 0, 0, 0, 0); \
})

#define StartNotify(__p0) ({ \
   BOOL P = LP1(888, BOOL , StartNotify, \
      struct NotifyRequest *, __p0, d1, \
      , DOS_BASE_NAME, 0, 0, 0, 0, 0, 0); \
   _MEMTRACK(__FILE__, __LINE__, "StartNotify", __p0, (size_t)__p0); \
   P; \
})

#define EndNotify(__p0) ({ \
   _UNMEMTRACK(__FILE__, __LINE__, "StartNotify", __p0); \
   LP1NR(894, EndNotify, \
      struct NotifyRequest *, __p0, d1, \
      , DOS_BASE_NAME, 0, 0, 0, 0, 0, 0); \
})

#elif !defined(__AROS__) // AmigaOS 3

#define AllocPooled(poolHeader, memSize) ({ \
  APTR _AllocPooled_poolHeader = (poolHeader); \
  ULONG _AllocPooled_memSize = (memSize); \
  APTR _AllocPooled__re = \
  ({ \
  register struct ExecBase * const __AllocPooled__bn __asm("a6") = (struct ExecBase *) (EXEC_BASE_NAME);\
  register APTR __AllocPooled__re __asm("d0"); \
  register APTR __AllocPooled_poolHeader __asm("a0") = (_AllocPooled_poolHeader); \
  register ULONG __AllocPooled_memSize __asm("d0") = (_AllocPooled_memSize); \
  __asm volatile ("jsr a6@(-708:W)" \
  : "=r"(__AllocPooled__re) \
  : "r"(__AllocPooled__bn), "r"(__AllocPooled_poolHeader), "r"(__AllocPooled_memSize)  \
  : "d1", "a0", "a1", "fp0", "fp1", "cc", "memory"); \
  __AllocPooled__re; \
  }); \
  _MEMTRACK(__FILE__, __LINE__, "AllocPooled", _AllocPooled__re, memSize); \
  _AllocPooled__re; \
})

#define FreePooled(poolHeader, memory, memSize) ({ \
  _UNMEMTRACK(__FILE__, __LINE__, "AllocPooled", memory); { \
  APTR _FreePooled_poolHeader = (poolHeader); \
  APTR _FreePooled_memory = (memory); \
  ULONG _FreePooled_memSize = (memSize); \
  { \
  register struct ExecBase * const __FreePooled__bn __asm("a6") = (struct ExecBase *) (EXEC_BASE_NAME);\
  register APTR __FreePooled_poolHeader __asm("a0") = (_FreePooled_poolHeader); \
  register APTR __FreePooled_memory __asm("a1") = (_FreePooled_memory); \
  register ULONG __FreePooled_memSize __asm("d0") = (_FreePooled_memSize); \
  __asm volatile ("jsr a6@(-714:W)" \
  : \
  : "r"(__FreePooled__bn), "r"(__FreePooled_poolHeader), "r"(__FreePooled_memory), "r"(__FreePooled_memSize) \
  : "d0", "d1", "a0", "a1", "fp0", "fp1", "cc", "memory"); \
  } \
}})

#define AllocVecPooled(p, s) ({APTR P = AllocVecPooled(p, s); _MEMTRACK(__FILE__, __LINE__, "AllocVecPooled", P, s); P;})
#define FreeVecPooled(p, m)  ({_UNMEMTRACK(__FILE__, __LINE__, "AllocVecPooled", m); FreeVecPooled(p, m);})

#define AllocDosObject(type, tags) ({ \
  ULONG _AllocDosObject_type = (type); \
  const struct TagItem * _AllocDosObject_tags = (tags); \
  APTR _AllocDosObject__re = \
  ({ \
  register struct DosLibrary * const __AllocDosObject__bn __asm("a6") = (struct DosLibrary *) (DOS_BASE_NAME);\
  register APTR __AllocDosObject__re __asm("d0"); \
  register ULONG __AllocDosObject_type __asm("d1") = (_AllocDosObject_type); \
  register const struct TagItem * __AllocDosObject_tags __asm("d2") = (_AllocDosObject_tags); \
  __asm volatile ("jsr a6@(-228:W)" \
  : "=r"(__AllocDosObject__re) \
  : "r"(__AllocDosObject__bn), "r"(__AllocDosObject_type), "r"(__AllocDosObject_tags)  \
  : "d1", "a0", "a1", "fp0", "fp1", "cc", "memory"); \
  __AllocDosObject__re; \
  }); \
  _MEMTRACK(__FILE__, __LINE__, "AllocDosObject", _AllocDosObject__re, type); \
  _AllocDosObject__re; \
})

#define FreeDosObject(type, ptr) ({ _UNMEMTRACK(__FILE__, __LINE__, "AllocDosObject|AllocDosObjectTags", ptr); { \
  ULONG _FreeDosObject_type = (type); \
  APTR _FreeDosObject_ptr = (ptr); \
  { \
  register struct DosLibrary * const __FreeDosObject__bn __asm("a6") = (struct DosLibrary *) (DOS_BASE_NAME);\
  register ULONG __FreeDosObject_type __asm("d1") = (_FreeDosObject_type); \
  register APTR __FreeDosObject_ptr __asm("d2") = (_FreeDosObject_ptr); \
  __asm volatile ("jsr a6@(-234:W)" \
  : \
  : "r"(__FreeDosObject__bn), "r"(__FreeDosObject_type), "r"(__FreeDosObject_ptr)  \
  : "d0", "d1", "a0", "a1", "fp0", "fp1", "cc", "memory"); \
  } \
}})

#define AllocSysObject(t, p) ({APTR P = AllocSysObject(t, p); _MEMTRACK(__FILE__, __LINE__, "AllocSysObject", P, t+1); P;})

#define AllocSysObjectTags(t, ...) ({APTR P = AllocSysObjectTags(t, __VA_ARGS__); _MEMTRACK(__FILE__, __LINE__, "AllocSysObjectTags", P, t+1); P;})

#define FreeSysObject(t, p) ({_UNMEMTRACK(__FILE__, __LINE__, "AllocSysObject|AllocSysObjectTags", p); FreeSysObject(t, p);})

#define AllocBitMap(sizex, sizey, depth, flags, friend_bitmap) ({ \
  ULONG _AllocBitMap_sizex = (sizex); \
  ULONG _AllocBitMap_sizey = (sizey); \
  ULONG _AllocBitMap_depth = (depth); \
  ULONG _AllocBitMap_flags = (flags); \
  CONST struct BitMap * _AllocBitMap_friend_bitmap = (friend_bitmap); \
  struct BitMap * _AllocBitMap__re = \
  ({ \
  register struct GfxBase * const __AllocBitMap__bn __asm("a6") = (struct GfxBase *) (GRAPHICS_BASE_NAME);\
  register struct BitMap * __AllocBitMap__re __asm("d0"); \
  register ULONG __AllocBitMap_sizex __asm("d0") = (_AllocBitMap_sizex); \
  register ULONG __AllocBitMap_sizey __asm("d1") = (_AllocBitMap_sizey); \
  register ULONG __AllocBitMap_depth __asm("d2") = (_AllocBitMap_depth); \
  register ULONG __AllocBitMap_flags __asm("d3") = (_AllocBitMap_flags); \
  register CONST struct BitMap * __AllocBitMap_friend_bitmap __asm("a0") = (_AllocBitMap_friend_bitmap); \
  __asm volatile ("jsr a6@(-918:W)" \
  : "=r"(__AllocBitMap__re) \
  : "r"(__AllocBitMap__bn), "r"(__AllocBitMap_sizex), "r"(__AllocBitMap_sizey), "r"(__AllocBitMap_depth), "r"(__AllocBitMap_flags), "r"(__AllocBitMap_friend_bitmap)  \
  : "d1", "a0", "a1", "fp0", "fp1", "cc", "memory"); \
  __AllocBitMap__re; \
  }); \
  _MEMTRACK(__FILE__, __LINE__, "AllocBitMap", _AllocBitMap__re, sizex); \
  _AllocBitMap__re; \
})

#define FreeBitMap(bm) ({ _UNMEMTRACK(__FILE__, __LINE__, "AllocBitMap", bm); { \
  struct BitMap * _FreeBitMap_bm = (bm); \
  { \
  register struct GfxBase * const __FreeBitMap__bn __asm("a6") = (struct GfxBase *) (GRAPHICS_BASE_NAME);\
  register struct BitMap * __FreeBitMap_bm __asm("a0") = (_FreeBitMap_bm); \
  __asm volatile ("jsr a6@(-924:W)" \
  : \
  : "r"(__FreeBitMap__bn), "r"(__FreeBitMap_bm)  \
  : "d0", "d1", "a0", "a1", "fp0", "fp1", "cc", "memory"); \
  } \
}})

#define ObtainDirContext(t)       ({APTR P = ObtainDirContext(t); _MEMTRACK(__FILE__, __LINE__, "ObtainDirContextTags", P, 1); P;})
#define ObtainDirContextTags(...) ({APTR P = ObtainDirContextTags(__VA_ARGS__); _MEMTRACK(__FILE__, __LINE__, "ObtainDirContextTags", P, 1); P;})
#define ReleaseDirContext(p)      ({_UNMEMTRACK(__FILE__, __LINE__, "ObtainDirContext|ObtainDirContextTags", p); ReleaseDirContext(p);})

#define AllocSignal(signalNum) ({ \
  LONG _AllocSignal_signalNum = (signalNum); \
  BYTE _AllocSignal__re = \
  ({ \
  register struct ExecBase * const __AllocSignal__bn __asm("a6") = (struct ExecBase *) (EXEC_BASE_NAME);\
  register BYTE __AllocSignal__re __asm("d0"); \
  register LONG __AllocSignal_signalNum __asm("d0") = (_AllocSignal_signalNum); \
  __asm volatile ("jsr a6@(-330:W)" \
  : "=r"(__AllocSignal__re) \
  : "r"(__AllocSignal__bn), "r"(__AllocSignal_signalNum)  \
  : "d1", "a0", "a1", "fp0", "fp1", "cc", "memory"); \
  __AllocSignal__re; \
  }); \
  _MEMTRACK(__FILE__, __LINE__, "AllocSignal", (APTR)(LONG)_AllocSignal__re, (size_t)signalNum); \
  _AllocSignal__re; \
})

#define FreeSignal(signalNum) ({ _UNMEMTRACK(__FILE__, __LINE__, "AllocSignal", (APTR)signalNum); { \
  LONG _FreeSignal_signalNum = (signalNum); \
  { \
  register struct ExecBase * const __FreeSignal__bn __asm("a6") = (struct ExecBase *) (EXEC_BASE_NAME);\
  register LONG __FreeSignal_signalNum __asm("d0") = (_FreeSignal_signalNum); \
  __asm volatile ("jsr a6@(-336:W)" \
  : \
  : "r"(__FreeSignal__bn), "r"(__FreeSignal_signalNum)  \
  : "d0", "d1", "a0", "a1", "fp0", "fp1", "cc", "memory"); \
  } \
}})

#define StartNotify(notify) ({ \
  struct NotifyRequest * _StartNotify_notify = (notify); \
  BOOL _StartNotify__re = \
  ({ \
  register struct DosLibrary * const __StartNotify__bn __asm("a6") = (struct DosLibrary *) (DOS_BASE_NAME);\
  register BOOL __StartNotify__re __asm("d0"); \
  register struct NotifyRequest * __StartNotify_notify __asm("d1") = (_StartNotify_notify); \
  __asm volatile ("jsr a6@(-888:W)" \
  : "=r"(__StartNotify__re) \
  : "r"(__StartNotify__bn), "r"(__StartNotify_notify)  \
  : "d1", "a0", "a1", "fp0", "fp1", "cc", "memory"); \
  __StartNotify__re; \
  }); \
  _MEMTRACK(__FILE__, __LINE__, "StartNotify", _StartNotify_notify, (size_t)_StartNotify_notify); \
  _StartNotify__re; \
})

#define EndNotify(notify) ({ _UNMEMTRACK(__FILE__, __LINE__, "StartNotify", notify); { \
  struct NotifyRequest * _EndNotify_notify = (notify); \
  { \
  register struct DosLibrary * const __EndNotify__bn __asm("a6") = (struct DosLibrary *) (DOS_BASE_NAME);\
  register struct NotifyRequest * __EndNotify_notify __asm("d1") = (_EndNotify_notify); \
  __asm volatile ("jsr a6@(-894:W)" \
  : \
  : "r"(__EndNotify__bn), "r"(__EndNotify_notify)  \
  : "d0", "d1", "a0", "a1", "fp0", "fp1", "cc", "memory"); \
  } \
}})

#endif // amigaos4

#endif // !DEBUG_USE_MALLOC_REDEFINE

#else // DEBUG

// to replace with NOPs is important here!
#define ENTER()              ((void)0)
#define LEAVE()              ((void)0)
#define RETURN(r)            ((void)0)
#define SHOWVALUE(f, v)      ((void)0)
#define SHOWPOINTER(f, p)    ((void)0)
#define SHOWSTRING(f, s)     ((void)0)
#define SHOWMSG(f, m)        ((void)0)
#define MEMTRACK(f, p, s)    ((void)0)
#define UNMEMTRACK(f, p)     ((void)0)
#define D(f, ...)            ((void)0)
#define E(f, ...)            ((void)0)
#define W(f, ...)            ((void)0)
#define ASSERT(expression)   ((void)0)

#define DumpDbgMalloc()      ((void)0)

#endif // DEBUG

#endif // DEBUG_H
