#ifndef _AMILIB_H
#define _AMILIB_H

/* Copyright (c) 2012 The AROS Development Team. All rights reserved.
   See the file COPYING for copying permission.
*/

#include <exec/libraries.h>

struct ExpatBase
{
   struct Library _lib;
   struct Library *_aroscbase;
};

#endif /* _AMILIB_H */
