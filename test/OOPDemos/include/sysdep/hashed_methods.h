#ifndef HASHED_METHODS_H
#define HASHED_METHODS_H

/*
   Copyright © 1997-98, The AROS Development Team. All rights reserved.
   $Id$

   Desc: Demo of new OOP system
   Lang: english
*/

#include "types.h"

struct MethodBucket
{
    struct MethodBucket *Next;
    ULONG MethodID;
    IPTR (*MethodFunc)();
    Class *mClass;
    
};

struct InterfaceDescr
{
    IPTR (**MethodTable)();
    ULONG InterfaceID;
    ULONG NumMethods;
};

#endif /* HASHED_METHODS_H */
