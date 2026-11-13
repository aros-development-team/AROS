#ifndef INTUITION_INTUITIONBASE_H
#define INTUITION_INTUITIONBASE_H

/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Structure of intuition.library
    Lang: english
*/

#ifndef EXEC_INTERRUPTS_H
#   include <exec/interrupts.h>
#endif
#ifndef EXEC_LIBRARIES_H
#   include <exec/libraries.h>
#endif
#ifndef EXEC_TYPES_H
#   include <exec/types.h>
#endif
#ifndef INTUITION_INTUITION_H
#   include <intuition/intuition.h>
#endif

/* You have to call LockIBase() before reading this struct! */
struct IntuitionBase
{
    struct Library LibNode;

    struct View ViewLord;

    struct Window * ActiveWindow;
    struct Screen * ActiveScreen;
    struct Screen * FirstScreen;

    ULONG Flags;

    WORD  MouseY;
    WORD  MouseX;

    ULONG Seconds;
    ULONG Micros;
};

#define HIRESPICK  0x0000
#define LOWRESPICK 0x0001
#define DMODECOUNT 0x0002

#define HIRESGADGET  0
#define LOWRESGADGET 1
#define RESCOUNT     2

#define UPFRONTGADGET   0
#define DOWNBACKGADGET  1
#define SIZEGADGET      2
#define CLOSEGADGET     3
#define DRAGGADGET      4
#define SUPFRONTGADGET  5
#define SDOWNBACKGADGET 6
#define SDRAGGADGET     7
#define GADGETCOUNT     8

#define EVENTMAX 10

#endif /* INTUITION_INTUITIONBASE_H */
