/*
    Copyright © 2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <dos.h>

#ifndef _COMPILER_H
#   include "compiler.h"
#endif

#define AROS_LONG2BE(x) (x)

char *StrDup(char *x);
int snprintf(char *buf, int size, const char *fmt, ...);
int strlcat(char *buf, char *src, int len);
