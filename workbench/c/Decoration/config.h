/*
    Copyright  2011, The AROS Development Team.
    $Id$
*/

#ifndef CONFIG_H
#define CONFIG_H

#include <exec/types.h>

LONG GetInt(STRPTR v);
void GetIntegers(STRPTR v, LONG *v1, LONG *v2);
void GetTripleIntegers(STRPTR v, LONG *v1, LONG *v2, LONG *v3);
void GetColors(STRPTR v, LONG *v1, LONG *v2);
BOOL GetBool(STRPTR v, STRPTR id);

#endif
