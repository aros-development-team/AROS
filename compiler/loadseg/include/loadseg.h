/*
 * Copyright (C) 2011, The AROS Development Team.  All rights reserved.
 * Author: Jason S. McMullan <jason.mcmullan@gmail.com>
 *
 * Licensed under the AROS PUBLIC LICENSE (APL) Version 1.1
 */

#ifndef LOADSEG_LOADSEG_H
#define LOADSEG_LOADSEG_H

#include <exec/types.h>
#include <aros/asmcall.h>

BPTR LoadSegment(BPTR fh, BPTR table, SIPTR *funcarr, LONG *stack, SIPTR *error, struct Library *lib);
BOOL UnLoadSegment(BPTR seglist , VOID_FUNC freefunc, struct DosLibrary *DOSBase);


#endif /* LOADSEG_LOADSEG_H */
