#ifndef MISC_H
#define MISC_H

/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/types.h>

#include "volumes.h"

ULONG writeHeader(struct afsbase *, struct Volume *, struct BlockCache *);

#endif
