/*
 * Copyright (C) 2013, The AROS Development Team
 * All right reserved.
 * Author: Jason S. McMullan <jason.mcmullan@gmail.com>
 *
 * Licensed under the AROS PUBLIC LICENSE (APL) Version 1.1
 */

#ifndef FREEANIM_H
#define FREEANIM_H

#include <exec/libraries.h>
#include <dos/bptr.h>

struct FreeAnimBase {
    struct Library  fa_Library;
    BPTR    fa_SegList;
};

#endif /* FREEANIM_H */
