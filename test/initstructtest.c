/*
    (C) 1995-96 AROS - The Amiga Research OS
    $Id$
    $Log$
    Revision 1.6  1998/10/20 16:46:38  hkiel
    Amiga Research OS

    Revision 1.5  1998/04/13 22:50:02  hkiel
    Include <proto/exec.h>

    Revision 1.4  1996/10/23 14:06:54  aros
    Missing include

    Revision 1.3  1996/10/19 17:07:31  aros
    Include <aros/machine.h> instead of machine.h

    Revision 1.2  1996/08/01 17:41:39  digulla
    Added standard header for all files

    Desc:
    Lang:
*/
#include <proto/exec.h>
#include <clib/exec_protos.h>
#include <aros/machine.h>
#include "initstruct.h"
#include <stdio.h>
#include <stddef.h>

struct demo
{
    LONG a[3];
    BYTE b[7];
    BYTE dummy1[2];
    LONG c[3];
    WORD d;
};

#define O(n) offsetof(struct demo,n)

struct init
{
    S_CPY   (1,3,L); /* 3 LONGs */
    S_REP   (2,7,B); /* 7 BYTEs */
    S_CPYO  (3,3,L); /* 3 LONGs */
    S_CPYO24(4,1,W); /* 1 WORD  */
    S_END   (end);
} inittable=
{
    { { I_CPY	(3,L),      { 1, 2, 3 } } },
    { { I_REP	(7,B),        4         } },
    { { I_CPYO	(3,L,O(c)), { 5, 6, 7 } } },
    { { I_CPYO24(1,W,O(d)), { 8 }       } },
	I_END	()
};

#undef O

int main(int argc,char *argv[])
{
    struct demo d;
    int i;

    InitStruct(&inittable,&d,sizeof(d));

    for(i=0;i<sizeof(d);i++)
      printf("%02x ",((UBYTE *)&d)[i]);

    return 0;
}

