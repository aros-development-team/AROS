/*
    Copyright © 1995-2002, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <proto/exec.h>
#include <clib/exec_protos.h>
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

