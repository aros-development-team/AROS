#ifndef HASHED_STRINGS_H
#define HASHED_STRINGS_H

/*
   (C) 1997-98 AROS - The Amiga Research OS
   $Id$

   Desc: Demo of new OOP system
   Lang: english
*/

#include "types.h"

struct MethodBucket
{
    struct MethodBucket *Next;
    STRPTR MethodID;
    IPTR (*MethodFunc)();
    Class *mClass;
    
};

struct MethodDescr
{
    IPTR (*MethodFunc)();
    STRPTR MethodID;
};

struct InterfaceDescr
{
    struct MethodDescr *MethodTable;
    STRPTR InterfaceID;
    ULONG NumMethods;
};

#endif /* HASHED_STRINGS_H */
