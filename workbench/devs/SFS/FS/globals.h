#ifndef GLOBALS_H_
#define GLOBALS_H_

#include <exec/execbase.h>
#include "deviceio.h"
#include "bitmap.h"
#include "transactions.h"
#include "nodes.h"
#include "fs.h"
#include "asfsbase.h"
#include "aros_stuff.h"

struct Globals {
    struct ASFSBase *asfsbase;
    struct ASFSDeviceInfo *device;
    struct ExecBase *sysBase;
    struct DosLibrary *dosBase;
    struct IntuitionBase *intuitionBase;
    struct UtilityBase *utilityBase;
    struct Device *timerBase;

    struct DosPacket *packet;
    struct DeviceNode *devnode;
    struct DeviceList *volumenode;
    struct MsgPort *sdlhport;       // The SFS DosList handler port.
    
    struct DosEnvec *dosenvec;
    struct timerequest *inactivitytimer_ioreq;
    struct timerequest *activitytimer_ioreq;        /* How long activity can delay flushing data. */
    struct FileSysStartupMsg *startupmsg;
    struct MsgPort *msgporttimer;
    struct MsgPort *msgportflushtimer;
    struct MsgPort *msgportnotify;
    struct Process *mytask;

    ULONG diskstate;     /* ID_WRITE_PROTECTED, ID_VALIDATING, ID_VALIDATED */
    ULONG blockpercycle;
    ULONG numsofterrors;
    ULONG inhibitnestcounter;

    ULONG block_defragptr;
    ULONG *defragmentsteps;
    ULONG defragmentlongs;
    
    ULONG blocks_reserved_start;      /* number of blocks reserved at start (=reserved) */
    ULONG blocks_reserved_end;        /* number of blocks reserved at end (=prealloc) */
    
    ULONG blocks_bitmap;              /* number of BTMP blocks for this partition */
    ULONG blocks_inbitmap;            /* number of blocks a single bitmap block can contain info on */
    ULONG blocks_admin;               /* the size of all AdminSpaces */

    ULONG block_root;                 /* the block offset of the root block */
    ULONG block_bitmapbase;           /* the block offset of the first bitmap block */
    ULONG block_extentbnoderoot;      /* the block offset of the root of the extent bnode tree */
    ULONG block_adminspace;           /* the block offset of the first adminspacecontainer block */
    ULONG block_objectnoderoot;       /* the block offset of the root of the objectnode tree */
    
    ULONG block_rovingblockptr;       /* the roving block pointer! */

    ULONG node_containers;            /* number of containers per ExtentIndexContainer */
    
    ULONG mask_block32;               /* masks the least significant bits of a BLCKf pointer */
    ULONG mask_debug;
    
    ULONG disktype;                   /* the type of media inserted (same as id_DiskType in InfoData
                                         structure) */

    ULONG max_name_length;
    ULONG activity_timeout;
    ULONG inactivity_timeout;

    UWORD shifts_block32;             /* shift count needed to convert a blockoffset<->32byteoffset (only used by nodes.c!) */

    UBYTE is_casesensitive;     /* See ROOTBITS_CASESENSITIVE in blockstructure.h */
    UBYTE has_recycled;         /* See ROOTBITS_RECYCLED in blockstructure.h */

    struct ExtFileLock *locklist;   /* pointer to the first lock in the list, or zero if none */

    LONG totalbuffers;                /* Number of buffers currently in use by filesystem! */
    struct MinList globalhandles;
    struct fsNotifyRequest *notifyrequests;

    BYTE activitytimeractive;
    BYTE pendingchanges;  /* indicates that the commit timer is active and that there are pending changes. */
    BYTE timerreset;

    BOOL is_LittleEndian; /* Little endian filesystem? */

    struct Space spacelist[SPACELIST_MAX+1];
    UBYTE string[260];  /* For storing BCPL string (usually path) */
    UBYTE string2[260]; /* For storing BCPL string (usually comment) */
    UBYTE pathstring[520];    /* Used by fullpath to build a full path */

    struct fsStatistics statistics;
    
    #ifdef CHECKCODE_SLOW
    extern struct MinList cblrulist;
    #endif
    
    struct fsIORequest *iolist;
    struct IOStdReq *ioreq;           /* Used for Read/Write */
    struct IOStdReq *ioreq2;          /* Used for WriteProtection & DisksChange */
    struct IOStdReq *ioreqchangeint;  /* Used for DiskChange interrupt */
    struct MsgPort *msgport;
    BYTE deviceopened;
    BYTE newstyledevice;
    BYTE does64bit;
    BYTE scsidirect;

    LONG retries;

    struct fsIORequest fsioreq;

    ULONG sectors_total;              /* size of the partition in sectors */
    UWORD sectors_block;              /* number of sectors in a block */
    
    UWORD shifts_block;               /* shift count needed to convert a blockoffset<->byteoffset */
    
    UWORD cmdread;
    UWORD cmdwrite;
    
    ULONG sector_low;                 /* sector offset of our partition, needed for SCSI direct */
    ULONG sector_high;                /* last sector plus one of our partition */
    
    ULONG blocks_total;               /* size of the partition in blocks */
    ULONG blocks_maxtransfer; /* max. blocks which may be transfered to the device at once (limits io_Length) */
    
    ULONG byte_low;                   /* the byte offset of our partition on the disk */
    ULONG byte_lowh;                  /* high 32 bits */
    ULONG byte_high;                  /* the byte offset of the end of our partition (excluding) on the disk */
    ULONG byte_highh;                 /* high 32 bits */
    
    ULONG bytes_block;                /* size of a block in bytes */
    ULONG bytes_sector;               /* size of a sector in bytes */
    
    ULONG mask_block;
    ULONG mask_mask;       /* mask as specified by mountlist */
    
    ULONG bufmemtype;   /* default value */
    
    /* Internal globals */
    
    struct SCSICmd scsicmd;       /* for SCSIdirect */
    struct SCSI10Cmd scsi10cmd;

    struct Interrupt changeint;
    struct IntData
        {
            ULONG *diskchanged;
            struct Task *task;
            ULONG signal;
        } intdata;
    ULONG diskchanged;

    #define MINSAFETYBLOCKS (16)
    #define MINCACHESIZE    (MINSAFETYBLOCKS*2)
    #define HASHSHIFT       (7)
    #define HASHSIZE        (1<<HASHSHIFT)

    struct MinList cbhashlist[HASHSIZE];
    struct MinList cblrulist;

    void *transactionpool;
    ULONG transactionpoolsize;
    UBYTE *compressbuffer;
    LONG transactionnestcount;
    
    struct Operation *operationroot;
    struct Operation operationsentinel;

    #define IOC_HASHSHIFT (6)
    #define IOC_HASHSIZE (1<<IOC_HASHSHIFT)
    
    struct IOCache *ioc_hashtable[IOC_HASHSIZE];
    struct MinList *iocache_lruhead;
    struct IOCache *ioc_buffer;           /* Used for reading data when another iocache has dirty data in it. */
    ULONG iocache_mask;
    ULONG iocache_sizeinblocks;
    ULONG iocache_lines;
    WORD iocache_shift;
    BYTE iocache_copyback;
    // BYTE iocache_readonwrite=TRUE;       /* Determines whether a new line is read before writing to it. */
    BYTE iocache_readonwrite;       /* Determines whether a new line is read before writing to it. */

    struct EClockVal ecv;
    
    NODE templockedobjectnode;
    
    UBYTE internalrename;      /* If TRUE, then the file is being renamed to recycled... */
    
    #define FRAGMENTS (300)

    ULONG fragment[FRAGMENTS];
    //UBYTE fragmenttype[FRAGMENTS];

    ULONG bestkey;
    ULONG bestblocks;
    ULONG searchedblocks;
    
    #define OPTBUFSIZE (131072)

    ULONG defrag_maxfilestoscan;
    LONG debugreqs;
};

extern struct Globals *globals;
void initGlobals();

#ifdef SysBase
#undef SysBase
#endif

#ifdef DOSBase
#undef DOSBase
#endif

#ifdef IntuitionBase
#undef IntuitionBase
#endif

#ifdef UtilityBase
#undef UtilityBase
#endif

#ifdef TimerBase
#undef TimerBase
#endif

#define SysBase (globals->sysBase)
#define DOSBase (globals->dosBase)
#define IntuitionBase (globals->intuitionBase)
#define UtilityBase (globals->utilityBase)
#define TimerBase (globals->timerBase)

#endif /*GLOBALS_H_*/
