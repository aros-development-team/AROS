#ifndef DATATYPES_DTIO64_H
#define DATATYPES_DTIO64_H

/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Shared 64-bit file access helpers for the datatype classes.

    These helpers use dos64.library when it is available, so files and
    positions beyond the 32-bit limits are handled correctly on large
    filesystems, and transparently fall back to the plain 32-bit
    dos.library calls when it is not, so the classes keep working on
    systems without dos64.library.

    The macros expand at the call site and expect both DOSBase and
    DOS64Base to resolve there - either as the usual globals, or as
    class-specific defines (e.g. '#define DOS64Base (base->dos64_base)').
    When a class uses such defines, include this header AFTER the header
    providing them. DOS64Base may be NULL (dos64.library not available).

    Classes built as genmodule modules can instantiate the DOS64Base
    global and its open/close handling with DTIO_DOS64_SUPPORT() in one
    source file; classes with their own library base management should
    instead open/close "dos64.library" themselves, treating failure to
    open it as acceptable.
*/

#include <exec/types.h>
#include <dos/dos.h>
#include <dos/dos64.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/dos64.h>
#include <aros/symbolsets.h>

/* True when a (non-negative) 64-bit size is usable as an allocation
   size on this system */
#define DTIO_SIZE_OK(size) \
    ((QUAD)(size) >= 0 && (QUAD)(size) == (QUAD)(IPTR)(size))

/* Size of the file behind fh, -1 on error */
#define DTIO_GetFileSize(fh)                                            \
({                                                                      \
    QUAD __dtio_size = -1;                                              \
    if (DOS64Base != NULL)                                              \
        __dtio_size = GetFileSize64(fh);                                \
    else                                                                \
    {                                                                   \
        struct FileInfoBlock *__dtio_fib = AllocDosObject(DOS_FIB, NULL); \
        if (__dtio_fib != NULL)                                         \
        {                                                               \
            if (ExamineFH((fh), __dtio_fib))                            \
                __dtio_size = (QUAD)(ULONG)__dtio_fib->fib_Size;        \
            FreeDosObject(DOS_FIB, __dtio_fib);                         \
        }                                                               \
    }                                                                   \
    __dtio_size;                                                        \
})

/* Current position in fh, -1 on error */
#define DTIO_GetFilePosition(fh)                                        \
({                                                                      \
    QUAD __dtio_pos;                                                    \
    if (DOS64Base != NULL)                                              \
        __dtio_pos = GetFilePosition64(fh);                             \
    else                                                                \
        __dtio_pos = Seek((fh), 0, OFFSET_CURRENT);                     \
    __dtio_pos;                                                         \
})

/* 64-bit Seek(); previous position, or -1 on error. Positions the
   32-bit fallback cannot express fail with ERROR_OBJECT_TOO_LARGE */
#define DTIO_Seek64(fh, mode, pos)                                      \
({                                                                      \
    QUAD __dtio_res, __dtio_skpos = (pos);                              \
    if (DOS64Base != NULL)                                              \
        __dtio_res = Seek64((fh), (mode), __dtio_skpos);                \
    else if (__dtio_skpos != (QUAD)(LONG)__dtio_skpos)                  \
    {                                                                   \
        SetIoErr(ERROR_OBJECT_TOO_LARGE);                               \
        __dtio_res = -1;                                                \
    }                                                                   \
    else                                                                \
        __dtio_res = Seek((fh), (LONG)__dtio_skpos, (mode));            \
    __dtio_res;                                                         \
})

/* 64-bit Read(); bytes read, or -1 on error. Lengths the 32-bit
   fallback cannot express fail with ERROR_OBJECT_TOO_LARGE */
#define DTIO_Read64(fh, buf, len)                                       \
({                                                                      \
    QUAD __dtio_res, __dtio_rdlen = (len);                              \
    if (DOS64Base != NULL)                                              \
        __dtio_res = Read64((fh), (buf), __dtio_rdlen);                 \
    else if (__dtio_rdlen != (QUAD)(LONG)__dtio_rdlen)                  \
    {                                                                   \
        SetIoErr(ERROR_OBJECT_TOO_LARGE);                               \
        __dtio_res = -1;                                                \
    }                                                                   \
    else                                                                \
        __dtio_res = Read((fh), (buf), (LONG)__dtio_rdlen);             \
    __dtio_res;                                                         \
})

/*
 * Instantiate the optional DOS64Base global for a genmodule-built
 * class: opened at module init (may fail, leaving it NULL) and closed
 * at expunge. Use once, in a single source file of the module.
 */
#define DTIO_DOS64_SUPPORT()                                            \
    struct Library *DOS64Base;                                          \
    static int DTIO_DOS64Init(struct Library *DTIO_base)                \
    {                                                                   \
        DOS64Base = OpenLibrary("dos64.library", 50);                   \
        return TRUE; /* dos64.library is optional */                    \
    }                                                                   \
    static int DTIO_DOS64Expunge(struct Library *DTIO_base)             \
    {                                                                   \
        if (DOS64Base != NULL)                                          \
        {                                                               \
            CloseLibrary(DOS64Base);                                    \
            DOS64Base = NULL;                                           \
        }                                                               \
        return TRUE;                                                    \
    }                                                                   \
    ADD2INITLIB(DTIO_DOS64Init, 0)                                      \
    ADD2EXPUNGELIB(DTIO_DOS64Expunge, 0)

#endif /* DATATYPES_DTIO64_H */
