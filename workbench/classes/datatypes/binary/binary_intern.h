/*
    (C) 1995-2000 AROS - The Amiga Research OS
    $Id$

    Desc: 
    Lang:
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
