#ifndef _TRANSACTIONS_H
#define _TRANSACTIONS_H

#include <exec/types.h>
#include <libraries/iffparse.h>
#include "blockstructure.h"
#include "redblacktree.h"

/* Operation   : A single block modification
   Transaction : A series of block modifications which together result in a valid disk */

struct OperationInformation {
  UWORD length;     /* Length in bytes of the compressed data. */
  BLCK blckno;
  UBYTE bits;       /* See defines below. */
  UBYTE data[0];
};

/* Defines for bits: */

#define OI_EMPTY (1)   /* When set indicates that this block has no original. */
#define OI_DELETE (2)  /* When set indicates that this block does not need to be written
                          anymore (ie, when blocks become unused and marked free). */


struct Operation {
  struct Operation *left;
  struct Operation *right;
  struct Operation *parent;
  #ifdef CHECKCODE_TRANSACTIONS
  UWORD magicvalue;
  #endif
  NodeColor color;
  UBYTE new;

//  struct Operation *next;
//  struct Operation *previous;

  struct OperationInformation oi;
};


#define TRANSACTIONSTORAGE_ID   AROS_LONG2BE(MAKE_ID('T','R','S','T'))
#define TRANSACTIONFAILURE_ID   AROS_LONG2BE(MAKE_ID('T','R','F','A'))
#define TRANSACTIONOK_ID        AROS_LONG2BE(MAKE_ID('T','R','O','K'))

/* The blocks used for storing the Transaction buffer are linked in a
   singly linked list.  The data they hold is a direct copy of all
   Transaction data. */

struct fsTransactionStorage {
  struct fsBlockHeader bheader;

  BLCK be_next;

  UBYTE data[0];
};


struct fsTransactionFailure {
  struct fsBlockHeader bheader;

  BLCK be_firsttransaction;
};

#endif // _TRANSACTIONS_H
