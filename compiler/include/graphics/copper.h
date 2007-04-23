#ifndef GRAPHICS_COPPER_H
#define GRAPHICS_COPPER_H

/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Copper definitions and structures.
    Lang: english
*/

#ifndef EXEC_TYPES_H
#   include <exec/types.h>
#endif

struct CopIns
{
    WORD OpCode;

    union
    {
        struct CopList * nxtlist;
        struct
        {
            union
            {
                WORD VWaitPos;
                WORD DestAddr;
            } u1;
            union
            {
                WORD HWaitPos;
                WORD DestData;
            } u2;
        } u4;
    } u3;
};
#define NXTLIST  u3.nxtlist
#define VWAITPOS u3.u4.u1.VWaitPos
#define DESTADDR u3.u4.u1.DestAddr
#define HWAITPOS u3.u4.u2.HWAitPos
#define DESTDATA u3.u4.u2.DestData

struct CopList
{
    struct CopList  * Next;
    struct CopList  * _CopList;
    struct ViewPort * _ViewPort;
    struct CopIns   * CopIns;
    struct CopIns   * CopPtr;

    UWORD * CopLStart;
    UWORD * CopSStart;
    WORD    Count;
    WORD    MaxCount;
    WORD    DyOffset;
#ifdef V1_3
    UWORD * Cop2Start;
    UWORD * Cop3Start;
    UWORD * Cop4Start;
    UWORD * Cop5Start;
#endif
    UWORD   SLRepeat;
    UWORD   Flags;    /* see below */
};

/* Flags (PRIVATE) */
#define EXACT_LINE 1
#define HALF_LINE  2

struct UCopList
{
    struct UCopList * Next;
    struct CopList  * FirstCopList;
    struct CopList  * CopList;
};

struct cprlist
{
    struct cprlist * Next;
    UWORD          * start;
    WORD             MaxCount;
};

#define COPPER_MOVE 0
#define COPPER_WAIT 1
#define CPRNXTBUF   2
#define CPR_NT_SYS  0x2000
#define CPR_NT_SHT  0x4000
#define CPR_NT_LOF  0x8000

struct copinit
{
    UWORD vsync_hblank[2];
    UWORD diagstrt[12];
    UWORD fm0[2];
    UWORD diwstart[10];
    UWORD bplcon2[2];
    UWORD sprfix[16];
    UWORD sprstrtup[32];
    UWORD wait14[2];
    UWORD norm_hblank[2];
    UWORD jump[2];
    UWORD wait_forever[6];
    UWORD sprstop[8];
};

#endif /* GRAPHICS_COPPER_H */
