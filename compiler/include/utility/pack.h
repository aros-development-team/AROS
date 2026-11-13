#ifndef UTILITY_PACK_H
#define UTILITY_PACK_H

/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Used for (Un)PackTagItems()
    Lang: english
*/

#ifndef EXEC_TYPES_H
#   include <exec/types.h>
#endif
#ifndef UTILITY_TAGITEM_H
#   include <utility/tagitem.h>
#endif

#define PSTB_EXISTS      26
#define PSTF_EXISTS (1L<<26)
#define PSTB_PACK        29
#define PSTF_PACK   (1L<<29)
#define PSTB_UNPACK      30
#define PSTF_UNPACK (1L<<30)
#define PSTB_SIGNED      31
#define PSTF_SIGNED (1L<<31)

#define PKCTRL_UBYTE      0x00000000
#define PKCTRL_BYTE       0x80000000
#define PKCTRL_UWORD      0x08000000
#define PKCTRL_WORD       0x88000000
#define PKCTRL_ULONG      0x10000000
#define PKCTRL_LONG       0x90000000
#define PKCTRL_PACKUNPACK 0x00000000
#define PKCTRL_UNPACKONLY 0x20000000
#define PKCTRL_PACKONLY   0x40000000
#define PKCTRL_BIT        0x18000000
#define PKCTRL_FLIPBIT    0x98000000

/* Macros (don't use!) */

#if AROS_BIG_ENDIAN
 #define PK_WORDOFFSET(flag) ((flag) < 0x0100 ? 1 : 0)
 #define PK_LONGOFFSET(flag) ((flag) < 0x0100 ? 3 : (flag) < 0x010000 ? 2 : (flag) < 0x01000000 ? 1 : 0)
#else
 #define PK_WORDOFFSET(flag) ((flag) < 0x0100 ? 0 : 1)
 #define PK_LONGOFFSET(flag) ((flag) < 0x0100 ? 0 : (flag) < 0x010000 ? 1 : (flag) < 0x01000000 ? 2 : 3)
#endif

/* Full IPTR (or SIPTR) value. Makes difference on 64 bits. */
#if __WORDSIZE > 32
 #define PKCTRL_IPTR  (PKCTRL_ULONG | 0x00002000)
 #define PKCTRL_SIPTR (PKCTRL_LONG  | 0x00002000)
#else
 #define PKCTRL_IPTR  PKCTRL_ULONG
 #define PKCTRL_SIPTR PKCTRL_LONG
#endif

#define PK_CALCOFFSET(type,field) ((IPTR)(&((struct type *)0)->field))
#define PK_BITNUM1(flag) ((flag) == 0x01 ? 0 : (flag) == 0x02 ? 1 : (flag) == 0x04 ? 2 : (flag) == 0x08 ? 3 : (flag) == 0x10 ? 4 : (flag) == 0x20 ? 5 : (flag) == 0x40 ? 6 : 7)
#define PK_BITNUM2(flag) ((flag) < 0x0100 ? PK_BITNUM1(flag) : 8 + PK_BITNUM1((flag)>>8))
#define PK_BITNUM(flag) ((flag) < 0x010000 ? PK_BITNUM2(flag) : 16 + PK_BITNUM2((flag)>>16))

/* Macros to create pack tables */
#define PACK_STARTTABLE(tagbase) (tagbase)
#define PACK_NEWOFFSET(tagbase)  (-1L),(tagbase)
#define PACK_ENDTABLE            0
#define PACK_ENTRY(tagbase,tag,type,field,control) (control | ((tag - tagbase)<<16L) | PK_CALCOFFSET(type,field))
#define PACK_BYTEBIT(tagbase,tag,type,field,control,flags) (control | ((tag - tagbase)<<16L) | PK_CALCOFFSET(type,field) | (PK_BITNUM(flags) <<13L))
#define PACK_WORDBIT(tagbase,tag,type,field,control,flags) (control | ((tag - tagbase)<<16L) | (PK_CALCOFFSET(type,field) + PK_WORDOFFSET(flags)) | ((PK_BITNUM(flags) & 7)<<13L))
#define PACK_LONGBIT(tagbase,tag,type,field,control,flags) (control | ((tag - tagbase)<<16L) | (PK_CALCOFFSET(type,field) + PK_LONGOFFSET(flags)) | ((PK_BITNUM(flags) & 7)<<13L))

#endif /* UTILITY_PACK_H */
