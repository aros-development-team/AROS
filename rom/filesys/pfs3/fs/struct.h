/* $Id$
 * $Log: struct.h $
 * Revision 2.16  1999/05/14  11:31:34  Michiel
 * Long filename support implemented; bugfixes
 *
 * Revision 2.15  1999/03/09  10:41:34  Michiel
 * Deldir extension en bitwise reserved roving
 *
 * Revision 2.14  1998/10/02  07:22:45  Michiel
 * final release 4.2 version
 *
 * Revision 2.13  1998/09/27  11:26:37  Michiel
 * Removed ErrorMsg (now is a function)
 *
 * Revision 2.12  1998/09/03  07:12:14  Michiel
 * versie 17.4
 * bugfixes 118, 121, 123 and superindexblocks and td64 support
 *
 * Revision 2.11  1998/05/31  16:27:42  Michiel
 * added ACTION_IS_PFS2
 * moved freeblocktype from allocprotos
 *
 * Revision 2.10  1998/05/27  20:16:13  Michiel
 * AFS --> PFS2
 *
 * Revision 2.9  1998/05/22  20:48:29  Michiel
 * Idle handle, anode_data_s uitbreiding
 *
 * Revision 2.8  1995/12/29  11:01:05  Michiel
 * rolloverinfo structure and directscsi stuff added
 *
 * Revision 2.7  1995/11/15  15:54:48  Michiel
 * IsTail() bug fixed
 * volumedata->rblkextension added
 *
 * Revision 2.6  1995/11/07  17:28:46  Michiel
 * struct allocation_data: reservedtobefreed cache, rtbf_index, res_alert, tbf_resneed
 * macros (IsUpdateNeeded, ReservedAreaIsLocked and limits)
 *
 * Revision 2.5  1995/10/20  10:12:38  Michiel
 * Anode reserved area adaptions (16.3)
 * --> andata.reserved, 'RESERVEDANODES' added; AllocAnode macro removed
 *
 * Revision 2.4  1995/10/11  23:27:26  Michiel
 * new diskcache stuff (see disk.c r14)
 *
 * Revision 2.3  1995/10/05  09:18:18  Michiel
 * rovingbit added to alloc_data
 *
 * Revision 2.2  1995/10/03  12:06:33  Michiel
 * merge fix
 *
 * Revision 1.12  1995/09/01  11:27:01  Michiel
 * ErrorMsg adaption (see disk.c and volume.c)
 *
 * Revision 1.11  1995/08/21  04:21:10  Michiel
 * added some extra packets
 *
 * Revision 1.10  1995/07/28  08:26:17  Michiel
 * dieing field
 *
 * Revision 1.9  1995/07/21  07:00:57  Michiel
 * importing messages.h
 * DELDIR stuff
 *
 * Revision 1.8  1995/07/11  09:24:38  Michiel
 * DELDIR stuff
 *
 * Revision 1.7  1995/06/23  17:33:00  Michiel
 * added 'action' to globaldata
 *
 * Revision 1.6  1995/06/23  06:42:42  Michiel
 * Soft-Protection after ErrorMsg
 * Pooled allocation stuff
 * muFS stuff
 *
 * Revision 1.5  1995/06/04  06:12:33  Michiel
 * added demo stuff
 *
 * Revision 1.4  1995/03/30  10:49:35  Michiel
 * notifyobject, notifylist and notifyport added
 *
 * Revision 1.3  1995/03/24  16:34:27  Michiel
 * g->myproc is back (needed for changeint)
 * softlinkdir added
 *
 * Revision 1.2  1995/02/28  18:28:10  Michiel
 * changed // to C comment and added originalsize
 *
 * Revision 1.1  1995/02/15  16:46:16  Michiel
 * Initial revision
 *
 */

#ifndef _STRUCT_H
#define _STRUCT_H 1

#ifndef _STRING_H
#include <string.h>
#endif
#ifndef DOS_DOSEXTENS_H
#include <dos/dosextens.h>
#endif
#ifndef DEVICES_TRACKDISK_H
#include <devices/trackdisk.h>
#endif
#ifndef PROTO_EXEC_H
#include <proto/exec.h>
#endif
#ifndef PROTO_DOS_H
#include <proto/dos.h>
#endif
#ifndef _STDDEFH
#include <stddef.h>
#endif
#if MULTIUSER
#ifndef LIBRARIES_MULTIUSER_H
#include <libraries/multiuser.h>
#endif
#endif
#ifndef DEVICES_SCSIDISK_H
#include <devices/scsidisk.h>
#endif



/****************************************************************************/
/* Useful macros to handle various compiler dependecies                     */
/****************************************************************************/

#if defined(__GNUC__)
#define __READONLY__ __attribute__((section(".rodata")))
#if __GNUC__ > 2
#define __USED__ __attribute__((used))
#else
#define __USED__
#endif
#else
#define __READONLY__
#define __USED__
#endif

/****************************************************************************/
/* For SAS/C use amiga.lib assembly memory pool routines                    */
/****************************************************************************/
#ifdef __SASC
void * __asm AsmCreatePool(register __d0 ULONG,
                           register __d1 ULONG,
                           register __d2 ULONG,
                           register __a6 struct ExecBase *);
void __asm AsmDeletePool(register __a0 void *,
                         register __a6 struct ExecBase *);
void * __asm AsmAllocPooled(register __a0 void *,
                            register __d0 ULONG,
                            register __a6 struct ExecBase *);
void __asm AsmFreePooled(register __a0 void *,
                         register __a1 void *,
                         register __d0 ULONG,
                         register __a6 struct ExecBase *);
#define LibCreatePool(a,b,c) AsmCreatePool(a,b,c,SysBase)
#define LibDeletePool(p) AsmDeletePool(p,SysBase)
#define LibAllocPooled(p,s) AsmAllocPooled(p,s,SysBase)
#define LibFreePooled(p,m,s) AsmFreePooled(p,m,s,SysBase)
/* Workaround for old NDK */
#ifndef CONST
#define CONST const
#endif
#if INCLUDE_VERSION < 44
typedef CONST unsigned char *CONST_STRPTR;
#endif
#endif

/****************************************************************************/
/* MorphOS specific global headers                                          */
/****************************************************************************/

#ifdef __MORPHOS__
#include <clib/macros.h>
#define min(a,b)       MIN(a,b)
#define max(a,b)       MAX(a,b)
#undef NewList
#define NewList(l)     NEWLIST(l)
#undef Insert
#define Insert(l,n,ln) INSERT(l,n,ln)
#undef AddHead
#define AddHead(l,n)   ADDHEAD(l,n)
#undef AddTail
#define AddTail(l,n)   ADDTAIL(l,n)
#undef Remove
#define Remove(n)      REMOVE(n)
#undef RemHead
#define RemHead(l)     REMHEAD(l)
#undef RemTail
#define RemTail(l)     REMTAIL(l)
#define memcpy(d,s,n)  CopyMem(s,d,n)
#endif

/****************************************************************************/
/* AROS specific global headers                                          */
/****************************************************************************/

#ifdef __AROS__
#include <proto/alib.h>
#include <clib/macros.h>
#define min(a,b)       MIN(a,b)
#define max(a,b)       MAX(a,b)
#undef IsMinListEmpty
#define __saveds
#define COUNT UWORD
#define UCOUNT WORD
#endif

/****************************************************************************/
/* New actions (packets)                                                    */
/****************************************************************************/

#define ACTION_KILL_EMPTY 3000
#define ACTION_SLEEP 2200
#define ACTION_UPDATE_ANODE 2201
#define ACTION_PFS2_INFO 2202
#define ACTION_PFS2_CONFIG 2203
#define ACTION_REMOVE_DIRENTRY 2204
//#if ROLLOVER
#define ACTION_CREATE_ROLLOVER 2205
#define ACTION_SET_ROLLOVER 2206
#define ACTION_IS_PFS2 2211
#define ACTION_ADD_IDLE_SIGNAL 2220
#define ACTION_SET_DELDIR 2221
#define ACTION_SET_FNSIZE 2222
//#endif

/****************************************************************************/
/* muFS related defines                                                     */
/****************************************************************************/

#if MULTIUSER
#define MUFS(x) x
#else
#define MUFS(x)
#endif

/* flags that allow SetProtect, SetOwner, SetDate, SetComment etc */
#define muRel_PROPERTY_ACCESS (muRelF_ROOT_UID|muRelF_UID_MATCH|muRelF_NO_OWNER)

/****************************************************************************/
/* CACHE related defines                                                    */
/****************************************************************************/

/* Locking. Dirblocks used during a operation have to be locked
 * with LOCK() (happens in LoadDirBlock() and UpdateLE())
 * UNLOCKALL() unlocks all blocks..
 */
#define LOCK(blk) ((blk)->used = g->locknr)
#define UNLOCKALL() (g->locknr++)
#define ISLOCKED(blk) ((blk)->used == g->locknr)

/* Cache hashing table mask values for dir and anode */
#define HASHM_DIR 0x1f
#define HASHM_ANODE 0x7


/****************************************************************************/
/* general defines                                                          */
/****************************************************************************/

#define WITH(x) x;
typedef unsigned char *DSTR;      /* pascal string: length, than characters     */
typedef enum {false, true} bool;

/*****************************************************************************/
/* Rollover info structure                                                   */
/*****************************************************************************/

/* used by ACTION_SET_ROLLOVER */
#if defined(__GNUC__) || defined(__VBCC__)
/* Force SAS/C compatible alignment rules for this structure */
#pragma pack(2)
#endif
struct rolloverinfo
{
	BOOL set;           /* 0 -> read; 1 -> write */
	ULONG realsize;
	ULONG virtualsize;
	ULONG rollpointer;
};
#if defined(__GNUC__) || defined(__VBCC__)
#pragma pack()
#endif


/*****************************************************************************/
/* Allocation data                                                           */
/*****************************************************************************/

/*
 * The global allocation data
 *
 * the number of blocks really free can be found in rootblock->blocksfree
 * rootblock fields:
 *   ULONG blocksfree       // total blocks free
 *   ULONG alwaysfree       // blocks to be kept always free 
 *   ULONG rovingptr        // roving 'normal' alloc pointer 
 *   ULONG reserved_free    // number of free reserved blocks
 *
 * volumedata fields:
 *   ULONG numblocks        // total number of blocks
 *   struct MinList bmblks
 *   struct MinList bmindex
 *
 * andata field indexperblock is also used
 *
 * res_rovingptr: roving 'reserved' alloc pointer 
 *
 * clean_blocksfree: directly available blocks (inc alwaysfree!). Updated by UpdateFreeList.
 *                  increased only by Update(), UpdateFreeList() and FlushBlock()
 *                  decreased by AllocateBlocks(), AllocReserved()
 *
 * alloc_available: number of eventually available blocks (exc alwaysfree!).
 *                 increased by FreeBlocks(), FreeReservedBlock()
 *                 decreased by AllocateBlocks()
 *
 * rootblock->blocksfree: only updated by Update(). Real number of blocks free (inc alwaysfree).
 *
 * reserved bitmap: behind rootblock. [0] = #free.
 */

/* cache grootte */
#define RTBF_CACHE_SIZE 512
#define TBF_CACHE_SIZE 256

/* update thresholds */
#define RTBF_THRESHOLD 256
#define RTBF_CHECK_TH  128
#define RTBF_POSTPONED_TH 48
#define TBF_THRESHOLD 252
#define RESFREE_THRESHOLD 10

/* indices in tobefreed array */
#define TBF_BLOCKNR 0
#define TBF_SIZE 1

/* buffer for AllocReservedBlockSave */
#define RESERVED_BUFFER 10

/* check for reserved block allocation lock */
#define ReservedAreaIsLocked (alloc_data.res_alert)

/* checks if update is needed now */
#define IsUpdateNeeded(rtbf_threshold)                              \
	((alloc_data.rtbf_index > rtbf_threshold) ||                    \
	(g->rootblock->reserved_free < RESFREE_THRESHOLD + 5 + alloc_data.tbf_resneed))         \

/* keep or free anodes when freeing blocks */
enum freeblocktype {keepanodes, freeanodes};

struct allocation_data_s
{
	ULONG clean_blocksfree;             /* number of blocks directly allocatable            */
	ULONG alloc_available;              /* cleanblocksfree + blockstobefreed - alwaysfree   */
	ULONG longsperbmb;                  /* longwords per bitmapblock                        */
	ULONG no_bmb;                       /* number of bitmap blocks                          */
	ULONG bitmapstart;                  /* blocknr at which bitmap starts                   */
	ULONG tobefreed[TBF_CACHE_SIZE][2]; /* tobefreed array                                  */
	ULONG tobefreed_index;
	ULONG tbf_resneed;                  /* max reserved blks needed for tbf cache           */
	struct bitmapblock *res_bitmap;     /* reserved block bitmap pointer                    */
	ULONG res_roving;                   /* reserved roving pointer (0 at startup)           */
	UWORD rovingbit;                    /* bitnumber (within LW) of main roving pointer     */
	ULONG numreserved;                  /* total # reserved blocks (== lastreserved+1)      */
	ULONG *reservedtobefreed;           /* tbf cache for flush reserved blocks  */
	ULONG rtbf_size;                    /* size of the allocated cache */
	ULONG rtbf_index;                   /* current index in reserved tobefreed cache        */
	BOOL res_alert;                     /* TRUE if low on available reserved blocks         */
};

/*****************************************************************************/
/* anode data                                                                */
/*****************************************************************************/

/*
 * The global anode data
 *
 * Other used globaldata fields:
 *  (*)getanodeblock()
 *  (*)allocanode()
 */
struct anode_data_s
{
  UWORD curranseqnr;        /* current anode seqnr for anode allocation */
  UWORD indexperblock;      /* ALSO used by allocation (for bitmapindex blocks) */
  ULONG maxanodeseqnr;
  UWORD anodesperblock;     /* number of anodes that fit in one block */
  UWORD reserved;           /* offset of first reserved anode within an anodeblock */
  ULONG *anblkbitmap;       /* anodeblock full-flag bitmap */
  ULONG anblkbitmapsize;    /* size of anblkbitmap */
  ULONG maxanseqnr;         /* maximum anodeblock seqnr */
};


/*
 * anodecache structures
 */
struct anodechainnode
{
	struct anodechainnode *next;
	struct canode an;
};

struct anodechain
{
	struct anodechain *next;
	struct anodechain *prev;
	ULONG refcount;             /* will be discarded if refcount becomes 0 */
	struct anodechainnode head;
};

/* number of reserved anodes per anodeblock */
#define RESERVEDANODES 6

/*****************************************************************************/
/* LRU data                                                                  */
/*****************************************************************************/

/* the LRU global data */
struct lru_data_s
{
	struct MinList LRUqueue;
	struct MinList LRUpool;
	ULONG poolsize;
	struct lru_cachedblock *LRUarray;
};


/*****************************************************************************/
/* the diskcache                                                             */
/*****************************************************************************/

/* the cache is filled in a round robin manner, using 'roving' for
 * the roundrobin pointer. Cache checking is done in the same manner;
 * making the cache large will make it slow!
 */

struct diskcache
{
	struct reftable *ref;   /* reference table; one entry per slot */
	UBYTE *data;            /* the data (one slot per block) */
	UWORD size;             /* cache capacity in blocks (order of 2) */
	UWORD mask;             /* size expressed in a bitmask */
	UWORD roving;           /* round robin roving pointer */
};

struct reftable
{
	ULONG blocknr;          /* blocknr of cached block; 0 = empty slot */
	UBYTE dirty;            /* dirty flag (TRUE/FALSE) */
	UBYTE pad;
};

#define DATACACHELEN 32
#define DATACACHEMASK 0x1f

#define MarkDataDirty(i) (g->dc.ref[i].dirty = 1)

/*****************************************************************************/
/* globaldata structure                                                      */
/*****************************************************************************/

#define ACCESS_UNDETECTED 0
#define ACCESS_STD 1
#define ACCESS_DS 2
#define ACCESS_TD64 3
#define ACCESS_NSD 4

/* ALL globals are defined here */
struct globaldata
{
	struct Process *myproc;             /* our process (needed for diskchange interrupt) */
	struct Interrupt *diskinterrupt;    /* diskint & signal also used by interrupt. Don't change! */
	ULONG diskchangesignal;
	struct ExecBase *g_SysBase;
	struct IntuitionBase *g_IntuitionBase;
	struct Library *g_UtilityBase;
	struct DosLibrary *g_DOSBase;
	struct muBase *g_muBase;
	struct MsgPort  *msgport;           /* communication port to DOS (normally == g->devnode->dn_Task) */
	struct MsgPort  *port;              /* for communication with diskdevice    */
	struct IOExtTD  *request;           /* request structure for diskdevice     */
	struct IOExtTD  *handlrequest;      /* copy of request for diskchangehndl   */
	struct MsgPort  *timeport;
	struct timerequest *trequest;
	struct MsgPort  *notifyport;        /* the replies to notification msgs come here */
	struct MsgPort  *sleepport;         /* for update anode and unsleep packets */
	struct DosPacket *action;           /* the current handled dospacket        */

	struct DeviceNode   *devnode;       /* <4A> from startup msg                */
	struct FileSysStartupMsg *startup;  /* idem                                 */
	struct DosEnvec     *dosenvec;      /* the fssm devlist                     */
	struct DriveGeometry *geom;         /* adapted according to DosEnvec!! (GetGeometry()) */
	DSTR  mountname;                    /* <4A> DSTR mountname                  */

	/* partition info (volume dependent) %7 */
	ULONG firstblock;                   /* first and last block of partition    */
	ULONG lastblock;
	ULONG maxtransfer;
	struct diskcache dc;                /* cache to make '196 byte mode' faster */

	/* LRU stuff */
	BOOL uip;                           /* update in progress flag              */
	UWORD locknr;                       /* prevents blocks from being flushed   */

	/* The DOS packet interpreter */
	void (*DoCommand)(struct DosPacket *, struct globaldata *);

	/* Volume stuff */
	struct MinList volumes;             /* Volume list. Normally only one volume */
	struct volumedata *currentvolume;
	ULONG changecount;                  /* diskchange counter. Should be initialized by NewVolume */
	ULONG inhibitcount;

	/* disktype: ID_PFS_DISK/NO_DISK_PRESENT/UNREADABLE_DISK
	 * (only valid if currentvolume==NULL) 
	 */
	ULONG disktype;

	/* state of currentvolume (ID_WRITE_PROTECTED/VALIDATED/VALIDATING) */
	ULONG diskstate;

	BOOL dieing;                        /* TRUE if ACTION_DIE called            */
	BYTE softprotect;                   /* 1 if 'ACTION_WRITE_PROTECTED'     	*/
										/* -1 if protection failed				*/
	ULONG protectkey;                   /* key to unprotect                     */
										/* ~0 als protected wegens error		*/
	UWORD timeout;                      /* DosToHandlerInterface timeout value  */
	BOOL dirty;                         /* Global dirty flag                    */
	BOOL timeron;                       /* change is being timed                */
	BOOL postpone;                      /* repeat timer when finished           */
	BOOL removable;                     /* Is volume removable?                 */
	BOOL trackdisk;                     /* Is the device trackdisk?             */
	LONG (*ErrorMsg)(CONST_STRPTR, APTR, ULONG, struct globaldata *);    /* The error message routine        */

	struct rootblock *rootblock;        /* shortcut of currentvolume->rootblk   */
	UBYTE harddiskmode;                 /* flag: harddisk mode?                 */
	UBYTE anodesplitmode;               /* flag: anodesplit mode?               */
	UBYTE dirextension;                 /* flag: dirextension?                  */
	UBYTE deldirenabled;                /* flag: deldir enabled?                */
	UBYTE sleepmode;                    /* flag: sleepmode?                     */
	UBYTE supermode;					/* flag: supermode? (104 bmi blocks)	*/
	UBYTE tdmode;						/* ACCESS_x mode                        */
	UBYTE pad;
	ULONG blocksize;                    /* g->dosenvec->de_SizeBlock << 2       */
	UWORD blockshift;                   /* 2 log van block size                 */
	UWORD fnsize;						/* filename size (18+)					*/
	ULONG directsize;                   /* number of blocks after which direct  */
										/* access is preferred (config)         */

	char *unparsed;                     /* rest of path after a softlinkdir     */
	BOOL protchecked;                   /* checked protection?                  */

	struct muExtOwner *user;            /* task which called filesystem (muFS)  */
	BOOL muFS_ready;                    /* is the muFS server ready?            */

	void *mainPool;                     /* pool for pooled alloc (V39)          */
	void *bufferPool;                   /* pool for buffer alloc (V39)          */

	struct Task *sleeptask;             /* task to send alarm messages to       */
	ULONG alarmsignal;                  /* ... and the signal to use            */
	struct MinList idlelist;            /* list of tasks to notify when idle    */

	/* SCSIDIRECT stuff */
	UBYTE sense[20];                    /* area for scsi sense data             */
	struct SCSICmd scsicmd;             /* scsi command structure               */

//  int (*allocate)(ULONG, ULONG, struct globaldata *);
//  void (*free)(ULONG, ULONG, struct globaldata *);
//  ULONG (*allocreserved)(ULONG, ULONG, struct globaldata *);
//  void (*freereserved)(ULONG, struct globaldata *);
	struct canodeblock *(*getanodeblock)(UWORD, struct globaldata *);
//  ULONG (*allocanode)(APTR);
	void *(*allocmemp)(ULONG, struct globaldata *);
	void (*freememp)(void *, struct globaldata *);
	void *(*allocbufmem)(ULONG, struct globaldata *);
	void (*freebufmem)(void *, struct globaldata *);

	struct anode_data_s glob_anodedata;
	struct lru_data_s glob_lrudata;
	struct allocation_data_s glob_allocdata;

	BOOL updateok;

	struct SignalSemaphore *device_unit_lock_sema;
	LONG device_unit_lock_count;

#if !defined(__MORPHOS__) || !defined(SYSTEM_PRIVATE)
	struct IOStdReq *resethandlerioreq;
#endif
	LONG   diskchangesigbit;
	LONG   resethandlersigbit;
	ULONG  resethandlersignal;
	struct Interrupt *resethandlerinterrupt;
};

typedef struct globaldata globaldata;

/*****************************************************************************/
/* Library base macros                                                       */
/*****************************************************************************/
#define SysBase       g->g_SysBase
#define IntuitionBase g->g_IntuitionBase
#define UtilityBase   g->g_UtilityBase
#define DOSBase       g->g_DOSBase
#define muBase        g->g_muBase

/*****************************************************************************/
/* defined function macros                                                   */
/*****************************************************************************/

#define GetAnodeBlock(a, b) (g->getanodeblock)(a,b)
//#define AllocAnode(a) (g->allocanode)(a)
//#define AllocReservedBlock(a,b) (g->allocreserved)(a,b)
//#define FreeReservedBlock(a,b) (g->freereserved)(a,b)

//#define AllocateBlocks(a,b,c) (g->allocate)(a, b, c)
//#define FreeBlocks(a,b,c) (g->free)(a,b,c)

#define AllocMemP(size,g) ((g->allocmemp)(size,g))
#define FreeMemP(mem,g) ((g->freememp)(mem,g))
#define AllocBufmem(size,g) ((g->allocbufmem)(size,g))
#define FreeBufmem(mem,g) ((g->freebufmem)(mem,g))

/*****************************************************************************/
/* local globdata additions                                                  */
/*****************************************************************************/

#define alloc_data (g->glob_allocdata)
#define andata (g->glob_anodedata)
#define lru_data (g->glob_lrudata)


/*****************************************************************************/
/* volumedata                                                                */
/*****************************************************************************/

struct volumedata
{
	struct volumedata   *next;          /* volumechain                          */
	struct volumedata   *prev;      
	struct DeviceList   *devlist;       /* <4A> device dos list                 */
	struct rootblock    *rootblk;       /* the cached rootblock. Also in g.     */

#if VERSION23
	struct crootblockextension *rblkextension; /* extended rblk, NULL if disabled*/ 
#endif

	struct MinList fileentries;         /* all locks and open files             */
	struct MinList anblks[HASHM_ANODE+1];   /* anode block hash table           */
	struct MinList dirblks[HASHM_DIR+1];    /* dir block hash table             */
	struct MinList indexblks;               /* cached index blocks              */
	struct MinList bmblks;              /* cached bitmap blocks                 */
	struct MinList superblks;			/* cached super blocks					*/
	struct MinList deldirblks;			/* cached deldirblocks					*/
	struct MinList bmindexblks;         /* cached bitmap index blocks           */
	struct MinList anodechainlist;      /* list of cached anodechains           */
	struct MinList notifylist;          /* list of notifications                */

	BOOL    rootblockchangeflag;        /* indicates if rootblock dirty         */
	WORD    numsofterrors;              /* number of soft errors on this disk   */
	WORD    diskstate;                  /* normally ID_VALIDATED                */
	ULONG   numblocks;                  /* total number of blocks               */
	UWORD   bytesperblock;              /* blok size (datablocks)               */
	UWORD   rescluster;                 /* reserved blocks cluster              */
};

/*
 * Notify list. This is volume dependent, so part of volume 
 * structure
 */
struct notifyobject
{
	struct notifyobject *next;
	struct notifyobject *prev;
	struct NotifyRequest *req;
	ULONG parentanodenr;        /* anodenr of (parsed part of) notification object's path */
	ULONG anodenr;              /* anodenr of notification object itself (dirs only) */
	UBYTE *namemem;             /* memory used for 'unparsed' */
	UBYTE *unparsed;            /* (CSTR!) unparsed part of nr_FullName pathpart (intl uppercase) */
	UBYTE *objectname;          /* (DSTR!) name of object (without path intl uppercase) */
};


/*
 * Idlehandle.
 */
struct idlehandle
{
	struct idlehandle *next;
	struct idlehandle *previous;
	struct Task *task;
	UWORD cleansignal;      /* idle after read access */
	UWORD dirtysignal;      /* idle after write access */
};

/*****************************************************************************/
/* LRU macro functions                                                       */
/*****************************************************************************/

/* Cached blocks are in two lists. This gets the outer list (the lru chain) from
 * the inner list
 */
#define LRU_CHAIN(b) \
 ((struct lru_cachedblock *)(((UBYTE *)(b))-offsetof(struct lru_cachedblock, cblk)))
#define LRU_CANODEBLOCK(blk) ((struct lru_canodeblock *)((ULONG *)blk - 2))
#define LRU_CDIRBLOCK(blk) ((struct lru_cdirblock *)((ULONG *)blk - 2))
#define LRU_NODE(blk) ((struct MinNode *)((ULONG *)blk - 2))

/* Make a block the most recently used one. The block
 * should already be in the chain!
 * Argument blk = struct cachedblock *
 */ 
#define MakeLRU(blk)                                    \
{                                                       \
	MinRemove(LRU_CHAIN(blk));                          \
	MinAddHead(&g->glob_lrudata.LRUqueue, LRU_CHAIN(blk));           \
}

/* Free a block from the LRU chain and add it to
 * the pool
 * Argument blk = struct cachedblock *
 */
#define FreeLRU(blk)                                    \
{                                                       \
	MinRemove(LRU_CHAIN(blk));                          \
	memset(blk, 0, SIZEOF_CACHEDBLOCK);                 \
	MinAddHead(&g->glob_lrudata.LRUpool, LRU_CHAIN(blk));            \
}

/*
 * Hashing macros
 */
#define ReHash(blk, list, mask)                         \
{                                                       \
	MinRemove(blk);                                     \
	MinAddHead(&list[(blk->blocknr/2)&mask], blk);      \
}

#define Hash(blk, list, mask)                           \
	MinAddHead(&list[(blk->blocknr/2)&mask], blk)


/*****************************************************************************/
/* other macro definitions                                                   */
/*****************************************************************************/

#define IsSoftLink(oi) ((IPTR)(oi).file.direntry>2 && ((oi).file.direntry->type==ST_SOFTLINK))
#define IsRealDir(oi) ((IPTR)(oi).file.direntry>2 && ((oi).file.direntry->type==ST_USERDIR))
#define IsDir(oi) ((IPTR)(oi).file.direntry>2 && ((oi).file.direntry->type)>0)
#define IsFile(oi) ((IPTR)(oi).file.direntry>2 && ((oi).file.direntry->type)<=0)
#define IsVolume(oi) ((oi).volume.root==0)
#if DELDIR
#define IsDelDir(oi) ((oi).deldir.special==SPECIAL_DELDIR)
#define IsDelFile(oi) ((oi).deldir.special==SPECIAL_DELFILE)
#endif /* DELDIR */
#if ROLLOVER
#define IsRollover(oi) ((IPTR)(oi).file.direntry>2 && ((oi).file.direntry->type==ST_ROLLOVERFILE))
#endif /* ROLLOVER */
#define ISCURRENTVOLUME(v) (g->currentvolume && \
	dstricmp(g->currentvolume->rootblk->diskname, v) == 0)
#define IsSameOI(oi1, oi2) ((oi1.file.direntry == oi2.file.direntry) && \
	(oi1.file.dirblock == oi2.file.dirblock))

// CHK(x) voorkomt indirectie van null pointer
// IsRoot(fi) checked of *oi bij de rootdir hoort
// IsRootA(fi) checked of oi bij de rootdir hoort
#define N(x) ((x)?(&(x)):NULL)
#define IsRoot(oi) (((oi)==NULL) || ((oi)->volume.root == 0))
#define IsRootA(oi) ((oi).volume.root == 0)

// voor VolumeRequest:
#define VR_URGENT   0
#define VR_PLEASE   1

/**********************************************************************/
/*                        Lists                                       */
/**********************************************************************/
#define MinAddHead(list, node)  AddHead((struct List *)(list), (struct Node *)(node))
#define MinAddTail(list, node)  AddTail((struct List *)(list), (struct Node *)(node))
#define MinInsert(list, node, listnode) Insert((struct List *)list, (struct Node *)node, (struct Node *)listnode)
#define MinRemove(node) Remove((struct Node *)node)
#define HeadOf(list) ((void *)((list)->mlh_Head))
#define IsHead(node) (!((node)->prev->prev))
#define IsTail(node) (!((node)->next->next))
#define IsMinListEmpty(x) ( ((x)->mlh_TailPred) == (struct MinNode *)(x) )

/**********************************************************************/
/*                        File administration                         */
/**********************************************************************/

/* FileInfo
**
** Fileinfo wordt door FindFile opgeleverd. Bevat pointers die wijzen naar
** gecachede directoryblokken. Deze blokken mogen dus alleen uit de cache
** verwijderd worden als deze verwijzingen verwijderd zijn. Op 't ogenblik
** is het verwijderen van fileinfo's uit in gebruik zijnde fileentries
** niet toegestaan. Een fileinfo gevuld met {NULL, xxx} is een volumeinfo. Deze
** wordt in locks naar de rootdir gebruikt.
** Een *fileinfo van NULL verwijst naar de root van de current volume
*/
struct fileinfo
{
	struct direntry *direntry;      // pointer wijst naar direntry binnen gecached dirblock
	struct cdirblock *dirblock;     // pointer naar gecached dirblock
};

struct volumeinfo
{
	ULONG   root;                   // 0 =>it's a volumeinfo; <>0 => it's a fileinfo
	struct volumedata *volume;
};

#if DELDIR
struct deldirinfo
{
	ULONG special;                  // 0 => volumeinfo; 1 => deldirinfo; 2 => delfile; >2 => fileinfo
	struct volumedata *volume;
};

struct delfileinfo
{
	ULONG special;					// 2
	ULONG slotnr;					// het slotnr voor deze deldirentry
};

/* info id's: delfile, deldir and flushed reference */
#define SPECIAL_DELDIR 1
#define SPECIAL_DELFILE 2
#define SPECIAL_FLUSHED 3
#endif /* DELDIR */

union objectinfo
{
	struct fileinfo file;
	struct volumeinfo volume;
#if DELDIR
	struct deldirinfo deldir;
	struct delfileinfo delfile;
#endif
};

/**********************************************************************/
/*                    Fileentries/locks & volumes                     */
/**********************************************************************
**
** Drie structuren met zelfde basis, maar verschillende lengte.
** Allemaal gekoppeld via next; type geeft type entry aan.
** CurrentAnode en AnodeOffset zijn eigenlijk redundant, want afleidbaar van 'offset'.
** FileInfo 'info' moet ingevulde zijn; het betreffende directoryblock moet dus ge-
** cached zijn.
** Van 'lock' is fl_Key het directoryblocknr (redundant met info.dirblock->blocknr)
** fl_Task, fl_Volume en fl_Access dienen ingevuld te zijn.
*/

/* entrytype's
** NB: ETF_VOLUME en ETF_LOCK zijn ALLEBIJ LOCKS!! TEST ON BOTH
*/
#define ET_VOLUME           0x0004
#define ET_FILEENTRY        0x0008
#define ET_LOCK             0x000c
#define ETF_VOLUME          1
#define ETF_FILEENTRY       2
#define ETF_LOCK            3
#define ET_SHAREDREAD       0
#define ET_SHAREDWRITE      1
#define ET_EXCLREAD         2
#define ET_EXCLWRITE        3

#define IsVolumeEntry(e)    ((e)->type.flags.type == ETF_VOLUME)
#define IsFileEntry(e)      ((e)->type.flags.type == ETF_FILEENTRY)
#define IsLockEntry(e)      ((e)->type.flags.type == ETF_LOCK)

#define IsVolumeLock(le)    ((le)->type.flags.type == ETF_VOLUME)

#define SHAREDLOCK(t) ((t).flags.access <= 1)

// ANODENR voor fe's en le's; FIANODENR voor fileinfo's; NOT FOR VOLUMES!!
#define ANODENR(fe) ((fe)->anodenr)
#define FIANODENR(fi) ((fi)->direntry->anode)

union listtype
{
	struct
	{
		unsigned pad:11;
		unsigned dir:1;     // 0 = file; 1 = dir or volume
		unsigned type:2;    // 0 = unknown; 3 = lock; 1 = volume; 2 = fileentry
		unsigned access:2;  // 0 = read shared; 2 = read excl; 1,3 = write shared, excl
	} flags;

	UWORD value;
};

typedef union listtype listtype;

/* Listentry
**
**- Alle locks op een disk zijn geketend vanuit volume->firstfe via 'next'. Het einde
**  van de keten is 0. Uitbreiden van de lijst gaat dmv ADDHEAD en ADDTAIL
**- [volume] verwijst terug naar de volume
**- [info] is {NULL, don't care} als root.  
**  FileInfo van file/dir moet ingevulde zijn; het betreffende directoryblock moet dus ge-
**  cached zijn.
**- [self] verwijst naar begin structuur
**- [lock] bevat verwijzing naar DLT_VOLUME DosList entry. MOET 4-aligned zijn !!
**- [currentanode] en [anodeoffset] zijn eigenlijk redundant, want afleidbaar van [offset].
**- Van [lock] is fl_Key het directoryblocknr (redundant met info.dirblock->blocknr)
**  fl_Task, fl_Volume en fl_Access dienen ingevuld te zijn.
**
**  Possible locks: root of volume; readlock; writelock; dir; file; readfe; writefe
*/

/* de algemene structure */
typedef struct listentry
{
	struct listentry    *next;          /* for linkage                                      */
	struct listentry    *prev;
	struct FileLock     lock;           /* <4A> contains accesstype, dirblocknr (redundant) */
	listtype            type;
	ULONG               anodenr;        /* object anodenr. Always valid. Used by ACTION_SLEEP */
	ULONG               diranodenr;     /* anodenr of parent. Only valid during SLEEP_MODE. */
	union objectinfo    info;           /* refers to dir                                    */
	ULONG               dirblocknr;     /* set when block flushed and info is set to NULL   */
	ULONG               dirblockoffset;
	struct volumedata   *volume;        /* pointer to volume                                */
} listentry_t;

/* de specifieke structuren */
typedef struct
{
	listentry_t le;
	
	struct anodechain *anodechain;      // the cached anodechain of this file
	struct anodechainnode *currnode;    // anode behorende bij offset in file
	ULONG   anodeoffset;        // blocknr binnen currentanode
	ULONG   blockoffset;        // byteoffset binnen huidig block
	ULONG   offset;             // offset tov start of file
	ULONG   originalsize;       // size of file at time of opening
	BOOL    checknotify;        // set if a notify is necessary at ACTION_END time > ALSO touch flag <
} fileentry_t;

typedef struct lockentry
{
	listentry_t le;

	ULONG               nextanode;          // anodenr of next entry (dir/vollock only)
	struct fileinfo     nextentry;          // for examine
	ULONG               nextdirblocknr;     // for flushed block only.. (dir/vollock only)
	ULONG               nextdirblockoffset;
} lockentry_t;

// *lock -> *fileentry
#define LOCKTOFILEENTRY(l) ((fileentry_t *)(((UBYTE*)l)-offsetof(fileentry_t, le.lock)))

// Maakt geen lock naar 'root of currentdir' aan!!
#define LockEntryFromLock(x) ((x) ? \
 (lockentry_t *)((UBYTE*)BADDR(x)-offsetof(listentry_t, lock)) : 0)

#define ListEntryFromLock(x) ((x) ? \
 (listentry_t *)((UBYTE*)BADDR(x)-offsetof(listentry_t, lock)) : 0)

#define MAXEXACTFIT 10

/**********************************************************************/
/*                        Disk administration                         */
/**********************************************************************/

#define InReservedArea(blocknr) \
	(((blocknr) >= g->currentvolume->rootblk->firstreserved) && \
	 ((blocknr) <= g->currentvolume->rootblk->lastreserved))

#define LastReserved    (g->currentvolume->rootblk->lastreserved)
#define FirstReserved   (g->currentvolume->rootblk->firstreserved)
#define InPartition(blk)  ((blk)>=g->firstblock && (blk)<=g->lastblock)
#define BLOCKSIZE (g->blocksize)
#define BLOCKSHIFT (g->blockshift)
#define DIRECTSIZE (g->directsize)

/*
 * TD64 support
 */
#ifndef TD_READ64
#define TD_READ64	24
#define TD_WRITE64	25
#define TD_SEEK64	26
#define TD_FORMAT64	27
#endif


/* NSD support */
#ifndef NSCMD_DEVICEQUERY
#define NSCMD_DEVICEQUERY 0x4000
#define NSCMD_TD_READ64 0xc000
#define NSCMD_TD_WRITE64 0xc001
#define NSDEVTYPE_TRACKDISK 5
struct NSDeviceQueryResult
{
    ULONG   DevQueryFormat;
    ULONG   SizeAvailable;
    UWORD   DeviceType;
    UWORD   DeviceSubType;
    UWORD   *SupportedCommands;
};
#endif

#define ACCESS_DETECT (TD64 + NSD + SCSIDIRECT > 1)

/*
 * Includes messages
 */
#include "messages.h"
#endif
