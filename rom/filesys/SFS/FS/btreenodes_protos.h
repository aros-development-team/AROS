#ifndef _BTREENODES_PROTOS_H
#define _BTREENODES_PROTOS_H

#include <exec/types.h>
#include "blockstructure.h"
#include "btreenodes.h"
#include "cachebuffers.h"

LONG createbnode(BLCK rootblock, ULONG key, struct CacheBuffer **returned_cb, struct BNode **returned_bnode);
LONG findbnode(BLCK rootblock, ULONG key, struct CacheBuffer **returned_cb, struct BNode **returned_bnode);
LONG deletebnode(BLCK rootblock, ULONG key);
LONG nextbnode(BLCK rootblock, struct CacheBuffer **io_cb, struct BNode **io_bnode);
LONG previousbnode(BLCK rootblock, struct CacheBuffer **io_cb, struct BNode **io_bnode);
LONG lastbnode(BLCK rootblock, struct CacheBuffer **returned_cb, struct BNode **returned_bnode);

#endif // _BTREENODES_PROTOS_H
