/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#define G(o) ((struct Gadget *)(o))

struct BinaryData
{
    APTR	Pool;
    ULONG	Flags;
    LONG	NumLines;
    WORD	OffsetSize;
    WORD	LineSize;
};
