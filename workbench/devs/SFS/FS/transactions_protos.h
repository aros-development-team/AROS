#ifndef _TRANSACTIONS_PROTOS_H
#define _TRANSACTIONS_PROTOS_H

#include <exec/types.h>
#include "blockstructure.h"
#include "cachebuffers.h"

// LONG savetransaction(BLCK *firsttransactionblock);
// void combineoperations(void);
LONG addoperation(BLCK block,UBYTE *data,UWORD length,UBYTE bits);
LONG addfreeoperation(BLCK block);
struct Operation *getlatestoperation(BLCK block);

#ifdef NOCOMPRESSION
LONG addoperation2(struct CacheBuffer *cb_org, struct CacheBuffer *cb_new);
#endif

void removeoperation(struct Operation *o);
LONG applyoperation(BLCK blckno,struct CacheBuffer **returned_cb);
LONG checkfortransaction(void);
ULONG transactionspace(void);
WORD isthereanoperationfor(BLCK block);

void newtransaction(void);
void endtransaction(void);
void deletetransaction(void);
LONG flushtransaction(void);
LONG inittransactions(void);
void cleanuptransactions(void);
BOOL hastransaction(void);

void restorecachebuffer(struct CacheBuffer *cb);

#endif // _TRANSACTIONS_PROTOS_H
