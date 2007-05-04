#ifndef INTERN_H
#define INTERN_H
/*
   Copyright © 1997-98, The AROS Development Team. All rights reserved.
   $Id$

   Desc: Demo of new OOP system
   Lang: english
*/

#include "types.h"

#define CalcHash(id, ht_size) (id & ht_size)

struct Bucket
{
    struct Bucket *Next;
    ULONG  MethodID;
    APTR   MethodFunc;
    Class *mClass; /* This is to be able to skip class calls */
};

#endif /* INTERN_H */
