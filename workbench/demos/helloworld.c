/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.4  1997/01/27 00:22:39  ldp
    Include proto instead of clib

    Revision 1.3  1996/09/17 16:43:00  digulla
    Use general startup code

    Revision 1.2  1996/08/01 17:40:44  digulla
    Added standard header for all files

    Desc: most simple demo for AROS
    Lang: english
*/
#include <proto/dos.h>

int main (int argc, char ** argv)
{
    Write (Output (), "hello, world\n", 13);
    return 0;
}
