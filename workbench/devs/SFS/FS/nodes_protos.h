#ifndef _NODES_PROTOS_H
#define _NODES_PROTOS_H

#include <exec/types.h>
#include "blockstructure.h"
#include "cachebuffers.h"
#include "nodes.h"

LONG createnode(BLCK noderoot, UWORD nodesize, struct CacheBuffer **returned_cb, struct fsNode **returned_node, NODE *returned_nodeno);
LONG findnode(BLCK nodeindex, UWORD nodesize, NODE nodeno, struct CacheBuffer **returned_cb, struct fsNode **returned_node);
LONG deletenode(BLCK noderoot, struct CacheBuffer *cb, struct fsNode *n, UWORD nodesize);

#endif // _NODES_PROTOS_H
