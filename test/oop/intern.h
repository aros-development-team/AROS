#ifndef INTERN_H
#define INTERN_H
/*
   (C) 1997-98 AROS - The Amiga Replacement OS
   $Id$

   Desc: Demo of new OOP system
   Lang: english
*/

#include "types.h"

#define CalcHash(id, ht_size) (id & ht_size)

struct Bucket
{
    struct Bucket *Next;
    APTR   MethodFunc;
    ULONG  MethodID;
    Class *Class; /* This is to be able to skip class calls */
};

#endif /* INTERN_H */
