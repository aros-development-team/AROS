/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <stdio.h>
#include <dos/dos.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/alib.h>
#include <proto/aros.h>

int main (int argc, char ** argv)
{
    BPTR fh;
    UBYTE b;
    UWORD w;
    ULONG l;
    FLOAT f;
    DOUBLE d;
    STRPTR s = NULL;

    fh = Open ("test.bed", MODE_NEWFILE);

    if (!fh)
    {
	printf ("Couldn't open file (1)\n");
	return 10;
    }

    WriteByte (fh, 0x11);
    WriteWord (fh, 0x1122);
    WriteLong (fh, 0x11223344);
    WriteString (fh, "Hello world.");
    WriteFloat (fh, 1.5);
    WriteDouble (fh, 1.75);

    Close (fh);

    fh = Open ("test.bed", MODE_OLDFILE);

    if (!fh)
    {
	printf ("Couldn't open file (2)\n");
	return 10;
    }

    ReadByte (fh, &b);
    kprintf ("Byte = %02x\n", b);
    ReadWord (fh, &w);
    kprintf ("Word = %04x\n", w);
    ReadLong (fh, &l);
    kprintf ("Long = %08lx\n", l);
    ReadString (fh, &s);
    kprintf ("String = \"%s\"\n", s);
    ReadFloat (fh, &f);
    kprintf ("Float = %08lx\n", ((ULONG *)&f)[0]);
    ReadDouble (fh, &d);
    kprintf ("Double = %08lx%08lx\n", ((ULONG *)&d)[1], ((ULONG *)&d)[0]);

    FreeVec (s);

    Close (fh);

    return 0;
}
