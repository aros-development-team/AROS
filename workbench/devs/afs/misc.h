#ifndef MISC_H
#define MISC_H

#include <exec/types.h>

#include "volumes.h"

ULONG writeHeader(struct Volume *, struct BlockCache *);

#endif
