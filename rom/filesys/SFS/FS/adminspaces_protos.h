#ifndef _ADMINSPACES_PROTOS_H
#define _ADMINSPACES_PROTOS_H

#include <exec/types.h>
#include "blockstructure.h"
#include "cachebuffers.h"

LONG allocadminspace(struct CacheBuffer **);
LONG freeadminspace(BLCK block);

#endif // _ADMINSPACES_PROTOS_H
