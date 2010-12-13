#ifndef _FS_H
#define _FS_H

#include <exec/types.h>
#include <libraries/iffparse.h>
#include <dos/dos.h>

#ifndef SFS_BE
#define SFS_BE 1
#endif

#define CHECKCODE
// #define CHECKCODE_SLOW
//#define CHECKCODE_BNODES
// #define CHECKCODE_TRANSACTIONS  // MagicValue, also uncomment stuff in redblacktree.h

// #define CHECKCHECKSUMSALWAYS

// #define DEBUGCODE
// #define DEBUGKPRINTF  /* If defined and DEBUGCODE is defined then uses kprintf as output */
// #define DEBUG115200

// #define WORDCOMPRESSION
#define LONGCOMPRESSION
// #define NOCOMPRESSION
// #define ALTERNATIVECOMPRESSION
// #define BLOCKCOMPRESSION

#define PROGRAMNAME     "Smart Filesystem"
#define PROGRAMNAMEVER  "SmartFilesystem"

#define MAX_NAME_LENGTH          (100)
#define MAX_TRANSACTIONPOOLSIZE  (32768)
#define MAX_RETRIES              (0)

#define ALWAYSFREE            (3)    // The number of blocks SFS always tries to keep free.

#define ROVING_SMALL_WRITE    (8192) // If a write is larger than this then no extra space will be
                                     // skipped by the roving pointer for possible future use by
                                     // the same file.

#define ROVING_RESERVED_SPACE (8192) // Space the roving ptr will skip extra for possible future
                                     // use by the same file.

#define INTERR_CHECKSUM_FAILURE       80
#define INTERR_BLOCK_WRONG_TYPE       81
#define INTERR_OWNBLOCK_WRONG         82
#define INTERR_BTREE                  83
#define INTERR_SAFE_DELETE            84
#define INTERR_NODES                  85
#define INTERR_EXTENT                 86
#define INTERR_DEFRAGMENTER           87

/* macros */

#ifndef __AROS__
    #define TOBADDR(x)      (((ULONG)(x)) >> 2)
#else
    #define TOBADDR(x)      (MKBADDR(x))
#endif

#define remove(n)    (n)->ln_Succ->ln_Pred=(n)->ln_Pred; (n)->ln_Pred->ln_Succ=(n)->ln_Succ
#define addtail(l,n) (n)->ln_Succ=(l)->lh_TailPred->ln_Succ; (l)->lh_TailPred->ln_Succ=(n); (n)->ln_Pred=(l)->lh_TailPred; (l)->lh_TailPred=(n)

#define removem(n)    (n)->mln_Succ->mln_Pred=(n)->mln_Pred; (n)->mln_Pred->mln_Succ=(n)->mln_Succ
#define addtailm(l,n) (n)->mln_Succ=(l)->mlh_TailPred->mln_Succ; (l)->mlh_TailPred->mln_Succ=(n); (n)->mln_Pred=(l)->mlh_TailPred; (l)->mlh_TailPred=(n)
#define addheadm(l,n) (n)->mln_Succ=(l)->mlh_Head; (n)->mln_Pred=(struct MinNode *)(l); (l)->mlh_Head->mln_Pred=(n); (l)->mlh_Head=(n);

#if SFS_BE
#define DOSTYPE_ID      AROS_MAKE_ID('S','F','S',0)
#else
#define DOSTYPE_ID      AROS_MAKE_ID('s','f','s',0)
#endif

/* HASHENTRY(x) is used to determine which hashchain to use for
   a specific hash value. */

#define HASHCHAIN(x) (UWORD)(x % (UWORD)((globals->bytes_block-sizeof(struct fsHashTable))>>2))


struct fsStatistics {
  ULONG cache_accesses;
  ULONG cache_operationdecode;
  ULONG cache_emptyoperationdecode;
  ULONG cache_misses;

  ULONG cachedio_hits;
  ULONG cachedio_misses;
};

#endif // _FS_H
