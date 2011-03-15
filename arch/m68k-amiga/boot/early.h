/*
 * Copyright (C) 2011, The AROS Development Team.  All rights reserved.
 * Author: Jason S. McMullan <jason.mcmullan@gmail.com>
 *
 * Licensed under the AROS PUBLIC LICENSE (APL) Version 1.1
 */

#ifndef EARLY_H
#define EARLY_H

#include <exec/memory.h>

APTR Early_AllocAbs(struct MemHeader *mh, APTR location, IPTR byteSize);

#endif /* EARLY_H */
