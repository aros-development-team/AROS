#ifndef HASHED_IF_H
#define HASHED_IFS_H

/*
   Copyright © 1997-98, The AROS Development Team. All rights reserved.
   $Id$

   Desc: Demo of new OOP system
   Lang: english
*/

#include "types.h"

struct InterfaceBucket
{
    struct InterfaceBucket *Next;
    ULONG InterfaceID;
    struct IFMethod *MethodTable;
    ULONG NumMethods;
};

struct IFMethod
{
    IPTR (*MethodFunc)();
    Class *mClass;
};

struct InterfaceDescr
{
    IPTR (**MethodTable)();
    ULONG InterfaceID;
    ULONG NumMethods; /* Max method idx */
};

#endif /* HASHED_METHODS_H */
