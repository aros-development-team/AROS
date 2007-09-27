#ifndef AROS_STRUCTDESC_H
#define AROS_STRUCTDESC_H

/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Read and write big endian structures from and to a file
    Lang: english
*/

#ifndef EXEC_TYPES_H
#   include <exec/types.h>
#endif
#ifndef DOS_DOS_H
#   include <dos/dos.h>
#endif
#ifndef UTILITY_HOOKS_H
#   include <utility/hooks.h>
#endif
#include <stddef.h>

/* Big endian streamhook mathods */
struct BEIOM_Read
{
    STACKULONG MethodID; /* BEIO_READ */
};

struct BEIOM_Write
{
    STACKULONG MethodID; /* BEIO_WRITE */
    STACKULONG Data;     /* One byte to emit (0..255) */
};

struct BEIOM_Ignore
{
    STACKULONG MethodID; /* BEIO_IGNORE */
    STACKULONG Count;    /* How many bytes */
};

/* Big endian streamhook access modes */
#define BEIO_READ	0   /* Read a byte */
#define BEIO_WRITE	1   /* Write a byte */
#define BEIO_IGNORE	2   /* Skip some bytes (read only) */

#define SDT_END 	0 /* Read one  8bit byte */
#define SDT_UBYTE	1 /* Read one  8bit byte */
#define SDT_UWORD	2 /* Read one 16bit word */
#define SDT_ULONG	3 /* Read one 32bit long */
#define SDT_FLOAT	4 /* Read one 32bit IEEE */
#define SDT_DOUBLE	5 /* Read one 64bit IEEE */
#define SDT_STRING	6 /* Read a string */
#define SDT_STRUCT	7 /* Read a structure */
#define SDT_PTR 	8 /* Follow a pointer */
#define SDT_IGNORE	9 /* Ignore x bytes */
#define SDT_FILL_BYTE  10 /* Fill x bytes */
#define SDT_FILL_LONG  11 /* Fill x longs */
#define SDT_IFILL_BYTE 12 /* Ignore and fill x bytes */
#define SDT_IFILL_LONG 13 /* Ignore and fill x longs */
#define SDT_SPECIAL    14 /* Call user routine */

struct SDData
{
    APTR   sdd_Dest;
    WORD   sdd_Mode;
    void * sdd_Stream;
};

#define SDV_SPECIALMODE_READ	0  /* Function was called to read from file */
#define SDV_SPECIALMODE_WRITE	1  /* Function was called to write to file */
#define SDV_SPECIALMODE_FREE	2  /* Function was called to free memory */

#define SDM_END 			SDT_END
#define SDM_UBYTE(offset)               SDT_UBYTE, offset
#define SDM_UWORD(offset)               SDT_UWORD, offset
#define SDM_ULONG(offset)               SDT_ULONG, offset
#define SDM_FLOAT(offset)               SDT_FLOAT, offset
#define SDM_DOUBLE(offset)              SDT_DOUBLE, offset
#define SDM_STRING(offset)              SDT_STRING, offset
#define SDM_STRUCT(offset,sd)           SDT_STRUCT, offset, ((IPTR)sd)
#define SDM_PTR(offset,sd)              SDT_PTR, offset, ((IPTR)sd)
#define SDM_IGNORE(n)                   SDT_IGNORE, n
#define SDM_FILL_BYTE(offset,v,n)       SDT_FILL_BYTE, offset, v, n
#define SDM_FILL_LONG(offset,v,n)       SDT_FILL_LONG, offset, v, n
#define SDM_IFILL_BYTE(offset,v,n)      SDT_FILL_BYTE, offset, v, n
#define SDM_IFILL_LONG(offset,v,n)      SDT_FILL_LONG, offset, v, n
#define SDM_SPECIAL(offset,sdhook)      SDT_SPECIAL, offset, ((IPTR)sdhook)

#define SDM_BYTE    SDM_UBYTE
#define SDM_WORD    SDM_UWORD
#define SDM_LONG    SDM_ULONG

#endif /* AROS_STRUCTDESC_H */

