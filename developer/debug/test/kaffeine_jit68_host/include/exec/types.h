/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    The AROS exec types that kaffeine-jit68 and its interface header use,
    for the host build of the engine checks only. On AROS the real
    <exec/types.h> is used.
*/

#ifndef EXEC_TYPES_H
#define EXEC_TYPES_H

#include <stdint.h>

typedef uint8_t     UBYTE;
typedef int8_t      BYTE;
typedef uint16_t    UWORD;
typedef int16_t     WORD;
typedef uint32_t    ULONG;
typedef int32_t     LONG;
typedef uint64_t    UQUAD;
typedef int64_t     QUAD;
typedef void       *APTR;
typedef uintptr_t   IPTR;
typedef char       *STRPTR;
typedef const char *CONST_STRPTR;
typedef short       BOOL;

#ifndef TRUE
#define TRUE  1
#endif
#ifndef FALSE
#define FALSE 0
#endif

#endif /* EXEC_TYPES_H */
