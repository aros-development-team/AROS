#ifndef LIBRARIES_CONFIGREGS_H
#define LIBRARIES_CONFIGREGS_H

/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: AutoConfig(tm) hardware register definitions
    Lang: english
*/

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif

struct ExpansionRom
{
    UBYTE   er_Type;
    UBYTE   er_Product;
    UBYTE   er_Flags;
    UBYTE   er_Reserved03;
    UWORD   er_Manufacturer;
    ULONG   er_SerialNumber;
    UWORD   er_InitDiagVec;
    union {
	struct {
	    UBYTE   c;
	    UBYTE   d;
	    UBYTE   e;
	    UBYTE   f;
	} Reserved0;
	struct DiagArea *DiagArea;
    } er_;
};

/* This allows for simpler definition when assigning a DiagArea pointer. */
#define er_Reserved0c	er_.Reserved0.c
#define er_Reserved0d	er_.Reserved0.d
#define er_Reserved0e	er_.Reserved0.e
#define er_Reserved0f	er_.Reserved0.f
#define er_DiagArea	er_.DiagArea

struct ExpansionControl
{
    UBYTE   ec_Interrupt;
    UBYTE   ec_Z3_HighBase;
    UBYTE   ec_BaseAddress;
    UBYTE   ec_Shutup;
    UBYTE   ec_Reserved14;
    UBYTE   ec_Reserved15;
    UBYTE   ec_Reserved16;
    UBYTE   ec_Reserved17;
    UBYTE   ec_Reserved18;
    UBYTE   ec_Reserved19;
    UBYTE   ec_Reserved1a;
    UBYTE   ec_Reserved1b;
    UBYTE   ec_Reserved1c;
    UBYTE   ec_Reserved1d;
    UBYTE   ec_Reserved1e;
    UBYTE   ec_Reserved1f;
};

#define E_SLOTSIZE      0x10000
#define E_SLOTMASK      0xFFFF
#define E_SLOTSHIFT     16

#define E_EXPANSIONBASE     0x00e80000
#define E_EXPANSIONSIZE     0x00080000
#define E_EXPANSIONSLOTS    8

#define E_MEMORYBASE        0x00200000
#define E_MEMORYSIZE        0x00800000
#define E_MEMORYSLOTS       128

#define EZ3_EXPANSIONBASE   0xFF000000
#define EZ3_CONFIGAREA      0x40000000
#define EZ3_CONFIGAREAEND   0x7FFFFFFF
#define EZ3_SIZEGRANULARITY 0x00080000

/* er_Type */
#define ERT_TYPEMASK        0xc0
#define ERT_TYPEBIT         6
#define ERT_TYPESIZE        2
#define ERT_NEWBOARD        0xc0
#define ERT_ZORROII         ERT_NEWBOARD
#define ERT_ZORROIII        0x80

#define ERTB_MEMLIST        5
#define ERTF_MEMLIST        (1L<<5)
#define ERTB_DIAGVALID      4
#define ERTF_DIAGVALID      (1L<<4)
#define ERTB_CHAINEDCONFIG  3
#define ERTF_CHAINEDCONFIG  (1L<<3)

#define ERT_MEMMASK         0x7
#define ERT_MEMBIT          0
#define ERT_MEMSIZE         3

/* er_Flags */
#define ERFB_MEMSPACE       7
#define ERFF_MEMSPACE       (1L<<7)
#define ERFB_NOSHUTUP       6
#define ERFF_NOSHUTUP       (1L<<6)
#define ERFB_EXTENDED       5
#define ERFF_EXTENDED       (1L<<5)
#define ERFB_ZORRO_III      4
#define ERFF_ZORRO_III      (1L<<4)

#define ERT_Z3_SSMASK       0x0F
#define ERT_Z3_SSBIT        0
#define ERT_Z3_SSSIZE       4

/* ec_Interrupt register (unused) */
#define ECIB_INTENA         1
#define ECIF_INTENA         (1L<<1)
#define ECIB_RESET          3
#define ECIF_RESET          (1L<<3)
#define ECIB_INT2PEND       4
#define ECIF_INT2PEND       (1L<<4)
#define ECIB_INT6PEND       5
#define ECIF_INT6PEND       (1L<<5)
#define ECIB_INT7PEND       6
#define ECIF_INT7PEND       (1L<<6)
#define ECIB_INTERRUPTING   7
#define ECIF_INTERRUPTING   (1L<<7)

#define ERT_MEMNEEDED(t)    \
    (((t)&ERT_MEMMASK)? 0x10000 << (((t)&ERT_MEMMASK) -1) : 0x800000 )

#define ERT_SLOTSNEEDED(t)  \
    (((t)&ERT_MEMMASK)? 1 << (((t)&ERT_MEMMASK) -1) : 0x80 )


#define EC_MEMADDR(slot)    ((slot) << (E_SLOTSHIFT))
#define EROFFSET(er)        (int)&((struct ExpansionRom *)0)->er)
#define ECOFFSET(ec)        \
    ((sizeof(struct ExpansionRom)+((int)&((struct ExpansionControl *)0)->ec))

/* DiagArea */

struct DiagArea
{
    UBYTE   da_Config;
    UBYTE   da_Flags;
    UWORD   da_Size;
    UWORD   da_DiagPoint;
    UWORD   da_BootPoint;
    UWORD   da_Name;
    UWORD   da_Reserved01;
    UWORD   da_Reserved02;
};

#define DAC_BUSWIDTH    0xC0
#define DAC_NIBBLEWIDE  0x00
#define DAC_BYTEWIDE    0x40
#define DAC_WORDWIDE    0x80

#define DAC_BOOTTIME    0x30
#define DAC_NEVER       0x00
#define DAC_CONFIGTIME  0x10
#define DAC_BINDTIME    0x20

/*  da_DiagPoint function definition should be as follows

AROS_UFH5(ULONG, diagPoint,
    AROS_UFHA(APTR,                     boardBase, A0),
    AROS_UFHA(APTR,                     diagCopy,  A2),
    AROS_UFHA(struct ConfigDev *,       configDev, A3),
    AROS_UFHA(struct ExpansionBase *,   ExpansionBase, A5),
    AROS_UFHA(struct ExecBase *,        SysBase, A6)
);
{
    ...
}

As most of these will be in ROMs on the expansion board, this will
generally be coded in assembly language.

*/

#endif /* LIBRARIES_CONFIGREGS_H */
