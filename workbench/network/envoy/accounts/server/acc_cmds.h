#ifndef ACC_CMDS_H
#define ACC_CMDS_H
#include "acc_store.h"
ULONG AccHandle(struct AccStore *s, UBYTE cmd, UBYTE *req, ULONG len, CONST_STRPTR rhost);
#endif
