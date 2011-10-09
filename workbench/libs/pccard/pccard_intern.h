/*
    Copyright © 2011, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef PCCARD_LIBRARY_H
#define PCCARD_LIBRARY_H

#include <exec/types.h>
#include <exec/libraries.h>
#include <libraries/pccard.h>

#ifndef UPINT
typedef ULONG UPINT;
typedef LONG PINT;
#endif

struct PCCardBase
{
   struct Library library;
};

#define LEWord(P) (*(P)|(*((P)+1)<<8))

#endif
