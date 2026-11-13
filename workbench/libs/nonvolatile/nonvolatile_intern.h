/*
 * Copyright (C) 2013, The AROS Development Team
 * All right reserved.
 * Author: Jason S. McMullan <jason.mcmullan@gmail.com>
 *
 * Licensed under the AROS PUBLIC LICENSE (APL) Version 1.1
 */

#ifndef NONVOLATILE_INTERN_H
#define NONVOLATILE_INTERN_H

#include <exec/libraries.h>
#include <dos/bptr.h>

struct NonvolatileBase {
    struct Library  nv_Library;
    BPTR            nv_SegList;
    struct Library *nv_nvdBase;
};

#define nvdBase (((struct NonvolatileBase *)nvBase)->nv_nvdBase)

#endif /* NONVOLATILE_INTERN_H */
