#include "asmsupport.h"

#include <exec/types.h>
#include <clib/macros.h>          // MAX, MIN & ABS :-)
#include <devices/input.h>
#include <devices/inputevent.h>
#include <devices/timer.h>
#include <devices/trackdisk.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <dos/dostags.h>
#include <dos/exall.h>
#include <dos/filehandler.h>
#include <dos/notify.h>
#include <exec/errors.h>
#include <exec/interrupts.h>
#include <exec/io.h>
#include <exec/lists.h>
#include <exec/memory.h>
#include <exec/nodes.h>
#include <exec/resident.h>
#include <libraries/iffparse.h>
#include <resources/filesysres.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/timer.h>
#include <proto/utility.h>
/*
#define DEBUG 1
#define DEBUGCODE
*/
#include <math.h>

#include "sysdep.h"
#include "fs.h"
#include "adminspaces.h"
#include "bitmap.h"
#include "btreenodes.h"
#include "locks.h"
#include "nodes.h"
#include "objects.h"
#include "transactions.h"

#include "adminspaces_protos.h"
#include "bitmap_protos.h"
#include "btreenodes_protos.h"
#include "cachebuffers_protos.h"
#include "debug.h"
#include "locks_protos.h"
#include "nodes_protos.h"
#include "objects_protos.h"
#include "packets.h"
#include "query.h"
#include "support_protos.h"
#include "transactions_protos.h"
#include "req_protos.h"

#include <string.h>

#include "cachedio_protos.h"
#include "deviceio_protos.h"

static LONG fillgap(BLCK key);
LONG step(void);

#define BNODE

#define BITMAPFILL 0xFFFFFFFF    /* should be 0xFFFFFFFF !  Careful.. admin containers are 32 blocks! */

/* defines: */

#define BLCKFACCURACY   (5)                          /* 2^5 = 32 */

#define TIMEOUT         (1)   /* Timeout in seconds */
#define FLUSHTIMEOUT    (20)  /* Flush timeout in seconds */

#define ID_BUSY         AROS_LONG2BE(MAKE_ID('B','U','S','Y'))

/* Our own usage of NotifyRequest private data */
#define nr_Next nr_Reserved[2]
#define nr_Prev nr_Reserved[3]

/* structs */
#define SFSM_ADD_VOLUMENODE    (1)
#define SFSM_REMOVE_VOLUMENODE (2)

struct SFSMessage {
  struct Message msg;
  ULONG command;
  IPTR  data;
  LONG errorcode;
};

struct DefragmentStep {
  ULONG id;       // id of the step ("MOVE", "DONE" or 0)
  ULONG length;   // length in longwords (can be 0)
  ULONG data[0];  // size of this array is determined by length.
};


/* global variables */

#include "globals.h"

#ifndef __AROS__
struct SFSBase *globals=NULL;
#endif

void initGlobals()
{
    globals->is_LittleEndian = FALSE;
    globals->inhibitnestcounter = 0;
    globals->block_defragptr = 2;
    globals->is_casesensitive = FALSE;
    globals->has_recycled = TRUE;
    globals->locklist = NULL;
    globals->notifyrequests = NULL;
    globals->activitytimeractive = FALSE;
    globals->pendingchanges = FALSE;
    globals->timerreset = FALSE;
    globals->max_name_length = MAX_NAME_LENGTH;
    globals->activity_timeout = FLUSHTIMEOUT;
    globals->inactivity_timeout = TIMEOUT;
    globals->retries = MAX_RETRIES;
    globals->scsidirect = FALSE;
    globals->does64bit = FALSE;
    globals->newstyledevice = FALSE;
    globals->deviceopened = FALSE;
    globals->msgport = NULL;
    globals->ioreq = NULL;
    globals->ioreq2 = NULL;
    globals->ioreqchangeint = NULL;
    globals->cmdread = CMD_READ;
    globals->cmdwrite = CMD_WRITE;
    globals->blocks_maxtransfer = 1048576;
    globals->mask_mask = -1;
    globals->bufmemtype = MEMF_PUBLIC;
    globals->transactionpool = 0;
    globals->compressbuffer = 0;
    globals->transactionnestcount = 0;
    globals->iocache_lruhead = NULL;
    globals->iocache_lines = 8;
    globals->iocache_copyback = TRUE;
    globals->iocache_readonwrite = FALSE;
    globals->templockedobjectnode = 0;
    globals->internalrename = FALSE;
    globals->defrag_maxfilestoscan = 512;
    globals->debugreqs=TRUE;

    globals->mask_debug = 0xffff;
}

/* Prototypes */

static struct DosPacket *getpacket(struct Process *);
static struct DosPacket *waitpacket(struct Process *);
static void returnpacket(SIPTR,LONG);
static void sdlhtask(void);

/* Prototypes of cachebuffer related functions */

LONG readcachebuffercheck(struct CacheBuffer **,ULONG,ULONG);
void outputcachebuffer(struct CacheBuffer *cb);

/* Prototypes of node related functions */

LONG deleteextents(ULONG key);
static LONG findextentbnode(ULONG key,struct CacheBuffer **returned_cb,struct fsExtentBNode **returned_bnode);
static LONG createextentbnode(ULONG key,struct CacheBuffer **returned_cb,struct fsExtentBNode **returned_bnode);

/* Prototypes of debug functions */

ULONG calcchecksum(void);
void checksum(void);

/* Misc prototypes */

void starttimeout(void);
LONG flushcaches(void);
void invalidatecaches(void);

BOOL freeupspace(void);

BOOL checkchecksum(struct CacheBuffer *);
void setchecksum(struct CacheBuffer *);

void checknotifyforobject(struct CacheBuffer *cb,struct fsObject *o,UBYTE notifyparent);
void checknotifyforpath(UBYTE *path,UBYTE notifyparent);
void notify(struct NotifyRequest *nr);
UBYTE *fullpath(struct CacheBuffer *cbstart,struct fsObject *o);

LONG initdisk(void);
static void deinitdisk(void);

LONG handlesimplepackets(struct DosPacket *packet);
static LONG dumppackets(struct DosPacket *packet,LONG);
#ifdef DEBUGCODE
static void dumppacket(void);
#endif
static void actioncurrentvolume(struct DosPacket *);
static void actionsamelock(struct DosPacket *);
static void actiondiskinfo(struct DosPacket *);
static void fillinfodata(struct InfoData *);
static void fillfib(struct FileInfoBlock *,struct fsObject *);
static void diskchangenotify(ULONG class);

/* Prototypes of high-level filesystem functions */

LONG setfilesize(struct ExtFileLock *lock,ULONG bytes);
static LONG seek(struct ExtFileLock *lock,ULONG offset);
LONG seektocurrent(struct ExtFileLock *lock);
LONG seekextent(struct ExtFileLock *lock,ULONG offset,struct CacheBuffer **returned_cb,struct fsExtentBNode **returned_ebn,ULONG *returned_extentoffset);
void seekforward(struct ExtFileLock *lock, UWORD ebn_blocks, BLCK ebn_next, ULONG bytestoseek);
LONG writetofile(struct ExtFileLock *lock, UBYTE *buffer, ULONG bytestowrite);

static LONG extendblocksinfile(struct ExtFileLock *lock,ULONG blocks);
static LONG addblocks(UWORD blocks, BLCK newspace, NODE objectnode, BLCK *io_lastextentbnode);
LONG deletefileslowly(struct CacheBuffer *cbobject, struct fsObject *o);
                                                   
void mainloop(void);

/* ASM prototypes */

#define MAJOR_VERSION (1)
#define MINOR_VERSION (84)

#ifdef __GNUC__
const char ver_version[]="\0$VER: " PROGRAMNAMEVER " 1.84 (" ADATE ")\r\n";
#else
static const char ver_version[]={"\0$VER: " PROGRAMNAMEVER " 1.84 " __AMIGADATE__ "\r\n"};
#endif

#ifdef __AROS__
/* AROS builds in a 'struct Resident' automatically
 */
#else
/* ROMTag is useful for C:Version. */
#define res_Init NULL

const struct Resident resident =
{
    RTC_MATCHWORD,
    &resident,
    (APTR)&resident + sizeof(struct Resident),
    RTF_COLDSTART,
    MAJOR_VERSION,
    0,
    -81,
    PROGRAMNAME,
    &ver_version[7],
    res_Init
};
#endif

/* Main */

#ifndef __AROS__
extern const RESBASE;
extern const RESLEN;
extern const _LinkerDB;
extern const NEWDATAL;
#endif

LONG mainprogram(struct ExecBase *);

#ifndef __AROS__
#undef SysBase
#endif

#ifdef __AROS__
AROS_ENTRY(__startup ULONG, Start,
	   AROS_UFHA(char *, argstr, A0),
	   AROS_UFHA(ULONG, argsize, D0),
	   struct ExecBase *, sBase)
{
    AROS_USERFUNC_INIT

    return mainprogram(sBase);

    AROS_USERFUNC_EXIT
}
#else
LONG __saveds trampoline(void)
{
    struct ExecBase *sBase = (*((struct ExecBase **)4));
    
    return mainprogram(sBase);
}

LONG start(void)
{
  return(STACKSWAP(4096, trampoline));
/*  if(STACKSWAP()==0) {
    return(ERROR_NO_FREE_STORE);
  }

  return(mainprogram()); */
}
#endif

void request2(UBYTE *text);

// #define STARTDEBUG

LONG mainprogram(struct ExecBase *SysBase)
{
#ifndef __AROS__
  ULONG reslen;
  APTR old_a4;
  APTR newdata;
#endif

  D(bug("[SFS] Filesystem main\n"));

  globals = AllocMem(sizeof(struct SFSBase), MEMF_PUBLIC | MEMF_CLEAR);
#ifndef __AROS__
  globals->sysBase = SysBase;
#endif
  initGlobals();

#ifndef __AROS__
#undef SysBase
#define SysBase (globals->sysBase)

  old_a4=(APTR)getreg(REG_A4);
  reslen=((ULONG)&RESLEN-(ULONG)old_a4)+64;

  newdata=AllocMem(reslen,MEMF_CLEAR|MEMF_PUBLIC);

  CopyMem(old_a4,newdata,*(((ULONG *)old_a4)-2));

  putreg(REG_A4,(LONG)newdata);
#endif
  
  if((DOSBase=(struct DosLibrary *)OpenLibrary("dos.library",37))!=0) {
    D(bug("[SFS] DOSBase = %p\n", DOSBase));

    globals->mytask=(struct Process *)FindTask(0);
    D(bug("[SFS] mytask = %p\n", globals->mytask));

    globals->packet=waitpacket(globals->mytask);
    D(bug("[SFS] packet = %p\n", globals->packet));

    globals->devnode=(struct DeviceNode *)BADDR(globals->packet->dp_Arg3);
    globals->devnode->dn_Task=&globals->mytask->pr_MsgPort;
    globals->startupmsg=BADDR(globals->devnode->dn_Startup);
    D(bug("[SFS] devnode = %p\n", globals->devnode));
    D(bug("[SFS] startupmsg = %p\n", globals->startupmsg));

    if(initcachebuffers()==0) {

      if((IntuitionBase=(APTR)OpenLibrary("intuition.library",37))!=0) {

#ifdef STARTDEBUG
          dreq("(1) Filesystem initializing...");
#endif

        if((UtilityBase=(APTR)OpenLibrary("utility.library",37))!=0) {

          /* Create a msgport and iorequest for opening timer.device */

          if((globals->msgportnotify=CreateMsgPort())!=0) {
            if((globals->msgporttimer=CreateMsgPort())!=0) {
              if((globals->msgportflushtimer=CreateMsgPort())!=0) {
                if((globals->inactivitytimer_ioreq=(struct timerequest *)CreateIORequest(globals->msgporttimer, sizeof(struct timerequest)))!=0) {
                  if((globals->activitytimer_ioreq=(struct timerequest *)CreateIORequest(globals->msgportflushtimer, sizeof(struct timerequest)))!=0) {

#ifdef STARTDEBUG
                      dreq("(2) Message ports and iorequests created");
#endif

                    if(OpenDevice("timer.device",UNIT_VBLANK,&globals->inactivitytimer_ioreq->tr_node,0)==0) {
                      if(OpenDevice("timer.device",UNIT_VBLANK,&globals->activitytimer_ioreq->tr_node,0)==0) {

                        globals->dosenvec=(struct DosEnvec *)BADDR(globals->startupmsg->fssm_Environ);

                        globals->timerBase=(struct Device *)globals->inactivitytimer_ioreq->tr_node.io_Device;

                        /* Create a msgport and iorequest for opening the filesystem device */

                        initlist((struct List *)&globals->globalhandles);

#ifdef STARTDEBUG
                          dreq("(3) Timer.device opened");
#endif

                        if(initcachedio(AROS_BSTR_ADDR(globals->startupmsg->fssm_Device), globals->startupmsg->fssm_Unit, globals->startupmsg->fssm_Flags, globals->dosenvec)==0) {
#ifdef STARTDEBUG
                            dreq("(4) Cached IO layer started");
#endif

                          globals->shifts_block32=globals->shifts_block-BLCKFACCURACY;

                          globals->mask_block32=(1<<globals->shifts_block32)-1;
                          globals->blocks_inbitmap=(globals->bytes_block-sizeof(struct fsBitmap))<<3;  /* must be a multiple of 32 !! */
                          globals->blocks_bitmap=(globals->blocks_total+globals->blocks_inbitmap-1)/globals->blocks_inbitmap;
                          globals->blocks_admin=32;

                          globals->blocks_reserved_start=MAX(globals->dosenvec->de_Reserved,1);
                          globals->blocks_reserved_end=MAX(globals->dosenvec->de_PreAlloc,1);

                          {
                            ULONG blocks512, reserve;

                            blocks512=globals->blocks_total<<(globals->shifts_block-9);
                            reserve=SQRT(blocks512);
                            reserve=(reserve<<2) + reserve;

                            if(reserve > blocks512/100) {      // Do not use more than 1% of the disk.
                              reserve = blocks512/100;
                            }

                            if(reserve < globals->blocks_admin) {       // Use atleast 32 blocks, even if it is more than 1%.
                              reserve = globals->blocks_admin;
                              if(reserve > globals->blocks_total>>1) {
                                reserve = 0;
                              }
                            }

                            globals->block_rovingblockptr=globals->blocks_reserved_start + globals->blocks_admin + globals->blocks_bitmap + reserve;

                            _DEBUG(("RovingBlockPtr = %ld, reserve = %ld\n", globals->block_rovingblockptr, reserve));

                            // block_rovingblockptr=0;
                          }

                          globals->mask_debug=0x00000000;

                          globals->node_containers=(globals->bytes_block-sizeof(struct fsNodeContainer))/sizeof(BLCKn);

                          addchangeint((struct Task *)globals->mytask, 1<<globals->mytask->pr_MsgPort.mp_SigBit);

                          _DEBUG(("Initializing transactions\n"));

                          if(inittransactions()==0) {

                            #ifdef STARTDEBUG
                              dreq("(5) Transaction layer started");
                            #endif

                            if(addcachebuffers(globals->dosenvec->de_NumBuffers)==0) {

                              #ifdef STARTDEBUG
                                dreq("(6) Filesystem started succesfully!");
                              #endif

                              /* return startup-packet, the handler runs now */

                              _DEBUG(("Filesystem started!  Volumenode = %ld\n",globals->volumenode));
                              _DEBUG(("Mountlist entry says: Allocate %ld buffers of memtype 0x%08lx\n",globals->dosenvec->de_NumBuffers,globals->dosenvec->de_BufMemType));

                              //  returnpacket(DOSTRUE,0);   // Sep 19 1999: Moved down again.

                              _DEBUG(("CreateNewProc..."));

                                const struct TagItem 	     tags[]=
                                {
                                    {NP_Entry      , (IPTR)sdlhtask    	    	},
                                    {NP_Name       , (IPTR)"SFS DosList handler"},
                                    {NP_Priority   , 19                         },
                                    {TAG_DONE      , 0     	    	    	    }
                                };

                              if(CreateNewProc(tags)!=0) {

                              _DEBUG(("ok\n"));

                                while((globals->sdlhport=FindPort("SFS DosList handler"))==0) {
                                  Delay(2);
                                }

                                if(isdiskpresent()!=FALSE) {
                                  #ifdef STARTDEBUG
                                    dreq("There is a disk present.");
                                  #endif

                                  initdisk();
                                }
                                else {
                                  #ifdef STARTDEBUG
                                    dreq("No disk inserted.");
                                  #endif

                                  globals->disktype=ID_NO_DISK_PRESENT;
                                }

                                returnpacket(DOSTRUE,0);    // Jul  4 1999: Moved up...

                                #ifdef STARTDEBUG
                                  dreq("(7) Informed DOS about the new partition!");
                                #endif

                                mainloop();
                              }
                            }

                            cleanuptransactions();
                          }

                          removechangeint();
                          cleanupcachedio();
                        }
                        CloseDevice(&globals->activitytimer_ioreq->tr_node);
                      }
                      CloseDevice(&globals->inactivitytimer_ioreq->tr_node);
                    }
                    DeleteIORequest((struct IORequest *)globals->activitytimer_ioreq);
                  }
                  DeleteIORequest((struct IORequest *)globals->inactivitytimer_ioreq);
                }
                DeleteMsgPort(globals->msgportflushtimer);
              }
              DeleteMsgPort(globals->msgporttimer);
            }
            DeleteMsgPort(globals->msgportnotify);
          }
          CloseLibrary((struct Library *)UtilityBase);
        }

        #ifdef STARTDEBUG
          dreq("Filesystem failed.. exiting.");
        #endif

        CloseLibrary((struct Library *)IntuitionBase);
      }
    }

    _DEBUG(("Returning startup packet with DOSFALSE\n"));

    returnpacket(DOSFALSE,ERROR_NO_FREE_STORE);

    CloseLibrary((struct Library *)DOSBase);
  }

  FreeMem(globals, sizeof(struct SFSBase));
  _DEBUG(("Exiting filesystem\n"));

  return(ERROR_NO_FREE_STORE);
}


void mainloop(void) {
  ULONG signalbits;
  struct MsgPort *msgportpackets;
  struct Message *msg;

  msgportpackets=&globals->mytask->pr_MsgPort;    /* get port of our process */
  signalbits=1<<msgportpackets->mp_SigBit;
  signalbits|=1<<globals->msgporttimer->mp_SigBit;
  signalbits|=1<<globals->msgportnotify->mp_SigBit;
  signalbits|=1<<globals->msgportflushtimer->mp_SigBit;

  #ifdef STARTDEBUG
    dreq("Entering packet loop.");
  #endif

  for(;;) {

    Wait(signalbits);

    do {
      while((msg=GetMsg(globals->msgportflushtimer))!=0) {
        _TDEBUG(("mainloop: activity timeout -> flushed transaction\n"));
        flushtransaction();
        globals->activitytimeractive=FALSE;
      }

      while((msg=GetMsg(globals->msgporttimer))!=0) {
        if(globals->timerreset==TRUE) {
          /* There was another request during the timeout/2 period, so we extend the timeout a bit longer. */
          globals->pendingchanges=FALSE;
          starttimeout();
        }
        else {
          _TDEBUG(("mainloop: inactivity timeout -> flushed transaction\n"));
          flushcaches();
          globals->pendingchanges=FALSE;
        }
      }

      while((msg=GetMsg(globals->msgportnotify))!=0) {
        FreeMem(msg,sizeof(struct NotifyMessage));
      }

      if(getchange()!=0) {

        /* The disk was inserted or removed! */

        _DEBUG(("mainloop: disk inserted or removed\n"));

        if(isdiskpresent()==FALSE) {
          /* Disk was removed */

          globals->disktype=ID_NO_DISK_PRESENT;  /* Must be put before deinitdisk() */
          deinitdisk();
        }
        else {
          /* Disk was inserted */

          initdisk();
        }
      }

      if((msg=GetMsg(msgportpackets))!=0) {


        // diskstate=writeprotection();       /* Don't do this too often!!  It takes LOADS of time for scsi.device. */


#ifdef CHECKCODE_SLOW
  {
    struct CacheBuffer *cb;

    cb=(struct CacheBuffer *)cblrulist.mlh_Head;

    while(cb->node.mln_Succ!=0) {
      if(cb->locked!=0) {
        request(PROGRAMNAME " request","%s\n"\
                                       "mainloop: There was a locked CacheBuffer (lockcount = %ld, block = %ld, type = 0x%08lx)!\n"\
                                       "Nothing bad will happen, but let the author know.\n",
                                       "Ok",AROS_BSTR_ADDR(devnode->dn_Name), cb->locked, cb->blckno, *((ULONG *)cb->data));


        cb->locked=0; /* Nothing remains locked */
      }

      cb=(struct CacheBuffer *)(cb->node.mln_Succ);
    }
  }
#endif


        globals->packet=(struct DosPacket *)msg->mn_Node.ln_Name;

        #ifdef STARTDEBUG
          dreq("Received packet 0x%08lx, %ld.",globals->packet,globals->packet->dp_Type);
        #endif

        switch(globals->packet->dp_Type) {
        case ACTION_SFS_SET:
          {
            struct TagItem *taglist=(struct TagItem *)globals->packet->dp_Arg1;
            struct TagItem *tag;

            while((tag=NextTagItem(&taglist))!=NULL) {
              LONG data=tag->ti_Data;

              switch(tag->ti_Tag) {
              case ASS_MAX_NAME_LENGTH:
                if(data >= 30 && data <= 100) {
                  globals->max_name_length=data;
                }
                break;
              case ASS_ACTIVITY_FLUSH_TIMEOUT:
                if(data >= 5 && data <= 120) {
                  globals->activity_timeout=data;
                }
                break;
              case ASS_INACTIVITY_FLUSH_TIMEOUT:
                if(data >= 1 && data <= 5) {
                  globals->inactivity_timeout=data;
                }
                break;
              }
            }

            returnpacket(DOSTRUE,0);
          }
          break;
        case ACTION_SFS_QUERY:
          {
            struct TagItem *taglist=(struct TagItem *)globals->packet->dp_Arg1;
            struct TagItem *tag;

            while((tag=NextTagItem(&taglist)))
            {
              switch(tag->ti_Tag)
              {
              case ASQ_START_BYTEH:
                tag->ti_Data = globals->byte_low >> 32;
                break;

              case ASQ_START_BYTEL:
              	/*
              	 * Explicitly cast to ULONG here because on 64 bits
              	 * ti_Data is 64-bit wide, and this can confuse programs.
              	 */
                tag->ti_Data = (ULONG)globals->byte_low;
                break;

              case ASQ_END_BYTEH:
                tag->ti_Data = globals->byte_high >> 32;
                break;

              case ASQ_END_BYTEL:
                tag->ti_Data = (ULONG)globals->byte_high;
                break;

              case ASQ_DEVICE_API:
                tag->ti_Data=deviceapiused();
                break;
              case ASQ_BLOCK_SIZE:
                tag->ti_Data=globals->bytes_block;
                break;
              case ASQ_TOTAL_BLOCKS:
                tag->ti_Data=globals->blocks_total;
                break;
              case ASQ_ROOTBLOCK:
                tag->ti_Data=globals->block_root;
                break;
              case ASQ_ROOTBLOCK_OBJECTNODES:
                tag->ti_Data=globals->block_objectnoderoot;
                break;
              case ASQ_ROOTBLOCK_EXTENTS:
                tag->ti_Data=globals->block_extentbnoderoot;
                break;
              case ASQ_FIRST_BITMAP_BLOCK:
                tag->ti_Data=globals->block_bitmapbase;
                break;
              case ASQ_FIRST_ADMINSPACE:
                tag->ti_Data=globals->block_adminspace;
                break;
              case ASQ_CACHE_LINES:
                tag->ti_Data=queryiocache_lines();
                break;
              case ASQ_CACHE_READAHEADSIZE:
                tag->ti_Data=queryiocache_readaheadsize();
                break;
              case ASQ_CACHE_MODE:
                tag->ti_Data=queryiocache_copyback();
                break;
              case ASQ_CACHE_BUFFERS:
                tag->ti_Data=globals->totalbuffers;
                break;
              case ASQ_CACHE_ACCESSES:
                tag->ti_Data=globals->statistics.cache_accesses;
                break;
              case ASQ_CACHE_MISSES:
                tag->ti_Data=globals->statistics.cache_misses;
                break;
              case ASQ_OPERATIONS_DECODED:
                tag->ti_Data=globals->statistics.cache_operationdecode;
                break;
              case ASQ_EMPTY_OPERATIONS_DECODED:
                tag->ti_Data=globals->statistics.cache_emptyoperationdecode;
                break;
              case ASQ_IS_CASESENSITIVE:
                tag->ti_Data=globals->is_casesensitive;
                break;
              case ASQ_HAS_RECYCLED:
                tag->ti_Data=globals->has_recycled;
                break;
              case ASQ_VERSION:
                tag->ti_Data=MAJOR_VERSION * 65536 + MINOR_VERSION;
                break;
              case ASQ_MAX_NAME_LENGTH:
                tag->ti_Data=globals->max_name_length;
                break;
              case ASQ_ACTIVITY_FLUSH_TIMEOUT:
                tag->ti_Data=globals->activity_timeout;
                break;
              case ASQ_INACTIVITY_FLUSH_TIMEOUT:
                tag->ti_Data=globals->inactivity_timeout;
                break;
              }
            }

            returnpacket(DOSTRUE,0);
          }
          break;
        case ACTION_SET_DEBUG:
          _DEBUG(("New debug level set to 0x%08lx!\n",globals->packet->dp_Arg1));
          {
            globals->mask_debug=globals->packet->dp_Arg1;

            returnpacket(DOSTRUE,0);
          }
          break;
        case ACTION_SET_CACHE:
          _DEBUG(("ACTION_SET_CACHE\n"));
          {
            LONG errorcode;

            if((errorcode=setiocache(globals->packet->dp_Arg1, globals->packet->dp_Arg2, globals->packet->dp_Arg3 & 1))!=0) {
              returnpacket(DOSFALSE, errorcode);
            }
            else {
              returnpacket(DOSTRUE, 0);
            }
          }

          break;
        case ACTION_SFS_FORMAT:
        case ACTION_FORMAT:
          _DEBUG(("ACTION_FORMAT\n"));

          {
            struct CacheBuffer *cb;
            ULONG currentdate;
            BLCK block_recycled;
            LONG errorcode=0;

            UBYTE *name=0;
            UBYTE *recycledname=".recycled";
            BYTE casesensitive=FALSE;
            BYTE norecycled=FALSE;
            BYTE showrecycled=FALSE;
            currentdate=getdate();

            if(globals->packet->dp_Type==ACTION_SFS_FORMAT) {
              struct TagItem *taglist=(struct TagItem *)globals->packet->dp_Arg1;
              struct TagItem *tag;

              while((tag=NextTagItem(&taglist))) {
                switch(tag->ti_Tag) {
                case ASF_NAME:
                  name=(UBYTE *)tag->ti_Data;
                  break;
                case ASF_RECYCLEDNAME:
                  recycledname=(UBYTE *)tag->ti_Data;
                  break;
                case ASF_CASESENSITIVE:
                  casesensitive=tag->ti_Data;
                  break;
                case ASF_NORECYCLED:
                  norecycled=tag->ti_Data;
                  break;
                case ASF_SHOWRECYCLED:
                  showrecycled=tag->ti_Data;
                  break;
                }
              }
            }
            else {
              copybstrasstr((BSTR)globals->packet->dp_Arg1, globals->string, 30);
              name=globals->string;
            }

            /* Global block numbers */

            globals->block_adminspace=globals->blocks_reserved_start;
            globals->block_root=globals->blocks_reserved_start+1;
            globals->block_extentbnoderoot=globals->block_root+3;
            globals->block_bitmapbase=globals->block_adminspace+globals->blocks_admin;
            globals->block_objectnoderoot=globals->block_root+4;

            /* Temporary block numbers */

            block_recycled=globals->block_root+5;

            cb=getcachebuffer();

            if(isvalidcomponentname(name)==FALSE || isvalidcomponentname(recycledname)==FALSE) {
              errorcode=ERROR_INVALID_COMPONENT_NAME;
            }

            if(errorcode==0) {
              struct fsAdminSpaceContainer *ac=cb->data;

              _DEBUG(("ACTION_FORMAT: Creating AdminSpace container block\n"));

              /* Create AdminSpaceContainer block */

              cb->blckno=globals->block_adminspace;
              clearcachebuffer(cb);

              ac->bheader.id=ADMINSPACECONTAINER_ID;
              ac->bheader.be_ownblock=L2BE(globals->block_adminspace);
              ac->bits=globals->blocks_admin;
              ac->adminspace[0].be_space=L2BE(globals->block_adminspace);

              if(norecycled==FALSE) {
                /* NOT HASHED RECYCLED
                ac->adminspace[0].be_bits=L2BE(0xFF000000);       // admin + root + hashtable + restore + 2 * nodecontainers + recycled + hash 
                */
                ac->adminspace[0].be_bits=L2BE(0xFE000000);       /* admin + root + hashtable + restore + 2 * nodecontainers + recycled */
              }
              else {
                ac->adminspace[0].be_bits=L2BE(0xFC000000);       /* admin + root + hashtable + restore + 2 * nodecontainers */
              }

              setchecksum(cb);
              errorcode=writecachebuffer(cb);
            }

            if(errorcode==0) {
              struct fsObjectContainer *oc=cb->data;
              struct fsRootInfo *ri=(struct fsRootInfo *)((UBYTE *)cb->data+globals->bytes_block-sizeof(struct fsRootInfo));

              _DEBUG(("ACTION_FORMAT: Creating Root block\n"));

              /* Create Root block */

              cb->blckno=globals->block_root;
              clearcachebuffer(cb);

              oc->bheader.id=OBJECTCONTAINER_ID;
              oc->bheader.be_ownblock=L2BE(globals->block_root);
              oc->object[0].be_protection=L2BE(FIBF_READ|FIBF_WRITE|FIBF_EXECUTE|FIBF_DELETE);
              oc->object[0].be_datemodified=L2BE(currentdate);
              oc->object[0].bits=OTYPE_DIR;
              oc->object[0].be_objectnode=L2BE(ROOTNODE);

              oc->object[0].object.dir.be_hashtable=L2BE(globals->block_root+1);

              if(norecycled==FALSE) {
                oc->object[0].object.dir.be_firstdirblock=L2BE(block_recycled);
              }

              copystr(name, oc->object[0].name, 30);

              ri->be_freeblocks=L2BE(globals->blocks_total-globals->blocks_admin-globals->blocks_reserved_start-globals->blocks_reserved_end-globals->blocks_bitmap);
              ri->be_datecreated=oc->object[0].be_datemodified;   // BE-BE copy

              setchecksum(cb);
              errorcode=writecachebuffer(cb);
            }

            if(errorcode==0) {
              struct fsHashTable *ht=cb->data;

              _DEBUG(("ACTION_FORMAT: Creating Root's HashTable block\n"));

              /* Create Root's HashTable block */

              cb->blckno=globals->block_root+1;
              clearcachebuffer(cb);

              ht->bheader.id=HASHTABLE_ID;
              ht->bheader.be_ownblock=L2BE(globals->block_root+1);
              ht->be_parent=L2BE(ROOTNODE);

              if(norecycled==FALSE) {
                ht->be_hashentry[HASHCHAIN(hash(".recycled", casesensitive))]=L2BE(RECYCLEDNODE);
              }

              setchecksum(cb);
              errorcode=writecachebuffer(cb);
            }

            if(errorcode==0) {
              struct fsBlockHeader *bh=cb->data;

              _DEBUG(("ACTION_FORMAT: Creating Transaction block\n"));

              /* Create empty block as a placeholder for the TransactionFailure block. */

              cb->blckno=globals->block_root+2;
              clearcachebuffer(cb);

              bh->id=TRANSACTIONOK_ID;
              bh->be_ownblock=L2BE(globals->block_root+2);

              setchecksum(cb);
              errorcode=writecachebuffer(cb);
            }

            if(errorcode==0) {
              struct fsBNodeContainer *bnc=cb->data;
              struct BTreeContainer *btc=&bnc->btc;

              _DEBUG(("ACTION_FORMAT: Creating ExtentNode root block\n"));

              /* Create NodeContainer block for ExtentNodes */

              cb->blckno=globals->block_extentbnoderoot;
              clearcachebuffer(cb);

              bnc->bheader.id=BNODECONTAINER_ID;
              bnc->bheader.be_ownblock=L2BE(globals->block_extentbnoderoot);

              btc->isleaf=TRUE;
              btc->be_nodecount=0;
              btc->nodesize=sizeof(struct fsExtentBNode);

              setchecksum(cb);
              errorcode=writecachebuffer(cb);
            }

            if(errorcode==0) {
              struct fsNodeContainer *nc=cb->data;
              struct fsObjectNode *on;

              _DEBUG(("ACTION_FORMAT: Creating ObjectNode root block\n"));

              /* Create NodeContainer block for ObjectNodes */

              cb->blckno=globals->block_objectnoderoot;
              clearcachebuffer(cb);

              nc->bheader.id=NODECONTAINER_ID;
              nc->bheader.be_ownblock=L2BE(globals->block_objectnoderoot);

              nc->be_nodenumber=L2BE(1);  /* objectnode 0 is reserved :-) */
              nc->be_nodes=L2BE(1);

              on=(struct fsObjectNode *)nc->be_node;
              on->node.be_data=L2BE(globals->block_root);

              on++;
              if(norecycled==FALSE) {
                on->node.be_data=L2BE(block_recycled);
                on->be_hash16=W2BE(hash(".recycled", casesensitive));
              }
              else {
                on->node.be_data=-1;         // reserved 2
              }

              on++;
              on->node.be_data=-1;         // reserved 3

              on++;
              on->node.be_data=-1;         // reserved 4

              on++;
              on->node.be_data=-1;         // reserved 5

              on++;
              on->node.be_data=-1;         // reserved 6

              setchecksum(cb);
              errorcode=writecachebuffer(cb);
            }

            if(errorcode==0 && norecycled==FALSE) {
              struct fsObjectContainer *oc=cb->data;

              _DEBUG(("ACTION_FORMAT: Creating Root ObjectContainer block\n"));

              cb->blckno=block_recycled;
              clearcachebuffer(cb);

              oc->bheader.id=OBJECTCONTAINER_ID;
              oc->bheader.be_ownblock=L2BE(block_recycled);
              oc->be_parent=L2BE(ROOTNODE);
              oc->object[0].be_protection=L2BE(FIBF_READ|FIBF_WRITE);
              oc->object[0].be_datemodified=L2BE(currentdate);
              oc->object[0].bits=OTYPE_DIR|OTYPE_UNDELETABLE|OTYPE_QUICKDIR;
              if(showrecycled==FALSE) {
                oc->object[0].bits|=OTYPE_HIDDEN;
              }

              oc->object[0].be_objectnode=L2BE(RECYCLEDNODE);

              /* NOT HASHED RECYCLED
              oc->object[0].object.dir.hashtable=block_recycled+1;
              */
              oc->object[0].object.dir.be_hashtable=0;

              copystr(".recycled",oc->object[0].name,30);

              setchecksum(cb);
              errorcode=writecachebuffer(cb);
            }

            /* NOT HASHED RECYCLED
            if(errorcode==0 && norecycled==FALSE) {
              struct fsHashTable *ht=cb->data;

              _DEBUG(("ACTION_FORMAT: Creating Recycled's HashTable block\n"));

              cb->blckno=block_recycled+1;
              clearcachebuffer(cb);

              ht->bheader.id=HASHTABLE_ID;
              ht->bheader.ownblock=block_recycled+1;
              ht->parent=RECYCLEDNODE;

              setchecksum(cb);
              errorcode=writecachebuffer(cb);
            }
            */

            if(errorcode==0) {
              struct fsBitmap *bm;
              UWORD cnt,cnt2;
              ULONG block=globals->block_bitmapbase;
              LONG startfree=globals->blocks_admin+globals->blocks_bitmap+globals->blocks_reserved_start;
              LONG sizefree;

              _DEBUG(("ACTION_FORMAT: Creating the Bitmap blocks\n"));

              /* Create Bitmap blocks */

              sizefree=globals->blocks_total-startfree-globals->blocks_reserved_end;

              cnt=globals->blocks_bitmap;
              while(cnt-->0 && errorcode==0) {
                clearcachebuffer(cb);

                bm=cb->data;
                bm->bheader.id=BITMAP_ID;
                bm->bheader.be_ownblock=L2BE(block);

                for(cnt2=0; cnt2<(globals->blocks_inbitmap>>5); cnt2++) {
                  if(startfree>0) {
                    startfree-=32;
                    if(startfree<0) {
                      bm->bitmap[cnt2]=AROS_LONG2BE((1<<(-startfree))-1);
                      sizefree+=startfree;
                    }
                  }
                  else if(sizefree>0) {
                    sizefree-=32;
                    if(sizefree<0) {
                      bm->bitmap[cnt2]=AROS_LONG2BE(~((1<<(-sizefree))-1));
                    }
                    else {
                      bm->bitmap[cnt2]=BITMAPFILL;
                    }
                  }
                  else {
                    break;
                  }
                }

                cb->blckno=block++;

                setchecksum(cb);
                errorcode=writecachebuffer(cb);
              }
            }

            if(errorcode==0) {
              struct fsRootBlock *rb;

              _DEBUG(("ACTION_FORMAT: Creating the Root blocks\n"));

              /* Create Root blocks */

              cb->blckno=0;
              clearcachebuffer(cb);

              rb=cb->data;
              rb->bheader.id=L2BE(DOSTYPE_ID);
              rb->bheader.be_ownblock=0;

              rb->be_version=W2BE(STRUCTURE_VERSION);
              rb->be_sequencenumber=0;

              rb->be_datecreated=L2BE(currentdate);

              rb->be_firstbyteh = L2BE(globals->byte_low >> 32);
              rb->be_firstbyte  = L2BE(globals->byte_low);
              rb->be_lastbyteh  = L2BE(globals->byte_high >> 32);
              rb->be_lastbyte   = L2BE(globals->byte_high);

              rb->be_totalblocks=L2BE(globals->blocks_total);
              rb->be_blocksize=L2BE(globals->bytes_block);

              rb->be_bitmapbase=L2BE(globals->block_bitmapbase);
              rb->be_adminspacecontainer=L2BE(globals->block_adminspace);
              rb->be_rootobjectcontainer=L2BE(globals->block_root);
              rb->be_extentbnoderoot=L2BE(globals->block_extentbnoderoot);
              rb->be_objectnoderoot=L2BE(globals->block_objectnoderoot);

              if(casesensitive!=FALSE) {
                rb->bits|=ROOTBITS_CASESENSITIVE;
              }
              if(norecycled==FALSE) {
                rb->bits|=ROOTBITS_RECYCLED;
              }

              setchecksum(cb);
              if((errorcode=writecachebuffer(cb))==0) {
                cb->blckno=globals->blocks_total-1;

                _DEBUG(("ACTION_FORMAT: Creating the 2nd Root block\n"));

                rb->bheader.be_ownblock=L2BE(globals->blocks_total-1);

                setchecksum(cb);
                errorcode=writecachebuffer(cb);
              }
            }

            flushiocache();
            update();
            motoroff();

            if(errorcode!=0) {
              _DEBUG(("ACTION_FORMAT: Exiting with errorcode %ld\n",errorcode));

              returnpacket(DOSFALSE, errorcode);
            }
            else {
              returnpacket(DOSTRUE, 0);
            }
          }

          break;
        case ACTION_INHIBIT:
          _DEBUG(("ACTION_INHIBIT(%ld)\n",globals->packet->dp_Arg1));

          /* This function nests.  Each call to inhibit the disk should be matched
             with one to uninhibit the disk. */

          if(globals->packet->dp_Arg1!=DOSFALSE) {
            #ifdef STARTDEBUG
              dreq("Disk inhibited (nesting = %ld).", globals->inhibitnestcounter);
            #endif

            if(globals->inhibitnestcounter++==0) {   // Inhibited for the first time?
              globals->disktype=ID_BUSY;  /* Must be put before deinitdisk() Feb 27 1999: Maybe not needed anymore */
              deinitdisk();
            }

            returnpacket(DOSTRUE,0);
          }
          else if(globals->inhibitnestcounter>0 && --globals->inhibitnestcounter==0) {

            returnpacket(DOSTRUE, 0);      /* Workbench keeps doslist locked, and doesn't send any packets
                                              during that time to this handler.  As initdisk() needs to lock
                                              the doslist we MUST return the packet before calling initdisk() */
            initdisk();
          }
          else {
            /* Workbench revokes ACTION_INHIBIT without ever actually having
               inhibited the volume.  We'll just return the packet and ignore
               such requests. */

            returnpacket(DOSTRUE,0);
          }
          break;
        case ACTION_SERIALIZE_DISK:
          _DEBUG(("ACTION_SERIALIZE_DISK\n"));

          {
            struct CacheBuffer *cb;
            LONG errorcode;

            if((errorcode=readcachebuffercheck(&cb,globals->block_root,OBJECTCONTAINER_ID))==0) {
              struct fsObjectContainer *oc=cb->data;
              struct fsRootInfo *ri=(struct fsRootInfo *)((UBYTE *)cb->data+globals->bytes_block-sizeof(struct fsRootInfo));

              oc->object[0].be_datemodified=L2BE(getdate());
              ri->be_datecreated=oc->object[0].be_datemodified;   // BE-BE copy

              setchecksum(cb);
              errorcode=writecachebuffer(cb);
            }

            if(errorcode!=0) {
              returnpacket(DOSFALSE,errorcode);
            }
            else {
              returnpacket(DOSTRUE,0);
            }
          }

          break;
        default:
          if(handlesimplepackets(globals->packet)==0) {

            if(globals->disktype == DOSTYPE_ID) {
              switch(globals->packet->dp_Type) {
              case ACTION_MAKE_LINK:
                _DEBUG(("ACTION_MAKE_LINK\n"));

                {
                  if(globals->packet->dp_Arg4==LINK_HARD) {
                    returnpacket(DOSFALSE,ERROR_ACTION_NOT_KNOWN);
                  }
              /*  else if(packet->dp_Arg4!=LINK_SOFT) {
                    returnpacket(DOSFALSE,ERROR_BAD_NUMBER);
                  } */  /* Check removed because DOS apparantely defines non-zero as being a Soft link! */
                  else {
                    struct ExtFileLock *lock;
                    LONG errorcode;
  
                    lock=(struct ExtFileLock *)BADDR(globals->packet->dp_Arg1);
                    copybstrasstr((BSTR)globals->packet->dp_Arg2,globals->string,258);
  
                    _DEBUG(("ACTION_MAKE_LINK: Name = '%s', LinkPath = '%s'\n",globals->string,(UBYTE *)globals->packet->dp_Arg3));

                    if((errorcode=findcreate(&lock,globals->string,globals->packet->dp_Type,(UBYTE *)globals->packet->dp_Arg3))!=0) {
                      returnpacket(DOSFALSE,errorcode);
                    }
                    else {
//                      freelock(lock);
                      returnpacket(DOSTRUE,0);
                    }
                  }
                }

                break;
              case ACTION_READ_LINK:
                _DEBUG(("ACTION_READ_LINK\n"));

                {
                  struct CacheBuffer *cb;
                  struct fsObject *o;
                  struct ExtFileLock *lock;
                  UBYTE *dest=(UBYTE *)globals->packet->dp_Arg3;
                  LONG errorcode;
                  NODE objectnode;

                  lock=(struct ExtFileLock *)BADDR(globals->packet->dp_Arg1);

                  if(lock==0) {
                    objectnode=ROOTNODE;
                  }
                  else {
                    objectnode=lock->objectnode;
                  }

                  if((errorcode=readobject(objectnode, &cb, &o))==0) {
                    UBYTE *path=(UBYTE *)globals->packet->dp_Arg2, *prefix=path;

                    _DEBUG(("ACTION_READ_LINK: path = '%s', errorcode = %ld\n",path,errorcode));

                    errorcode=locateobject2(&path, &cb, &o);

                    if(errorcode!=ERROR_IS_SOFT_LINK) {
                      errorcode=ERROR_OBJECT_NOT_FOUND;
                    }
                    else {
                      struct CacheBuffer *cb2;
                      UBYTE *p=path;

                      /* Move on to remainder of path after the link */

                      while(*path!=0) {
                        if(*path=='/') {
                          break;
                        }
                        path++;
                      }

                      _DEBUG(("ACTION_READ_LINK: path = '%s'\n",path));

                      if((errorcode=readcachebuffercheck(&cb2, BE2L(o->object.file.be_data), SOFTLINK_ID))==0) {
                        struct fsSoftLink *sl=cb2->data;
                        LONG length=globals->packet->dp_Arg4;
                        UBYTE *src=sl->string, ch;
                        UBYTE *s=globals->string;

                        while((*s++=*path++)!=0) {        // Work-around for bug in ixemul, which sometimes provides the same pointer for dp_Arg2 (path) and dp_Arg3 (soft-link buffer)
                        }

                        s=globals->string;

                        _DEBUG(("ACTION_READ_LINK: length = %ld, sl->string = '%s', path = '%s'\n", length, sl->string, s));

                        /* cb is no longer valid at this point. */

                        /* Check if link target is an absolute path */

                        while((ch=*src++)!='\0') {
                          if(ch==':')
                            break;
                        }
                        src=sl->string;

                        /* If target is a relative path, put path preceding
                           link into buffer so that result is relative to
                           lock passed in */

                        if(ch!=':') {
                          while(prefix!=p && length-->0) {
                            *dest++=*prefix++;
                          }
                        }

                        /* Copy link target to buffer */

                        while(length-->0 && (*dest++=*src++)!=0) {
                        }
                        dest--;
                        length++;

                        /* Ensure we don't insert an extraneous slash */

                        if(length!=globals->packet->dp_Arg4 && *(dest-1)=='/') {
                          *--dest='\0';
                          length++;
                        }
                        if(*(dest-1)==':' && *s=='/') {
                          s++;
                        }

                        /* Append remainder of original path */

                        while(length-->0 && (*dest++=*s++)!=0) {
                        }

                        if(length<0) {
                          errorcode=ERROR_LINE_TOO_LONG;
                        }
                      }
                    }
                  }

                  if(errorcode!=0) {
                    if(errorcode==ERROR_LINE_TOO_LONG) {
                      returnpacket(-2, errorcode);
                    }
                    else {
                      returnpacket(-1, errorcode);
                    }
                  }
                  else {
                    returnpacket((LONG)((SIPTR)dest - globals->packet->dp_Arg3 - 1),
                      0);
                  }
                }
                break;
              case ACTION_CREATE_DIR:
                _DEBUG(("ACTION_CREATE_DIR\n"));

                {
                  struct ExtFileLock *lock;
                  LONG errorcode;

                  lock=(struct ExtFileLock *)BADDR(globals->packet->dp_Arg1);
                  copybstrasstr((BSTR)globals->packet->dp_Arg2,globals->string,258);

                  if((errorcode=findcreate(&lock,globals->string,globals->packet->dp_Type,0))!=0) {
                    returnpacket(0,errorcode);
                  }
                  else {
                    returnpacket((SIPTR)TOBADDR(lock),0);
                  }
                }

                break;
              case ACTION_FINDUPDATE:
              case ACTION_FINDINPUT:
              case ACTION_FINDOUTPUT:
              case ACTION_FH_FROM_LOCK:
                _XDEBUG((DEBUG_OBJECTS, "ACTION_FIND#? or ACTION_FH_FROM_LOCK\n"));

                {
                  struct ExtFileLock *lock;
                  struct FileHandle *fh;
                  LONG errorcode=0;

                  fh=(struct FileHandle *)BADDR(globals->packet->dp_Arg1);
                  lock=(struct ExtFileLock *)BADDR(globals->packet->dp_Arg2);

                  if(globals->packet->dp_Type!=ACTION_FH_FROM_LOCK) {
                    copybstrasstr((BSTR)globals->packet->dp_Arg3,globals->string,258);
                    _DEBUG(("OPEN FILE: %s (mode = %ld)\n", globals->string, globals->packet->dp_Type));
                    errorcode=findcreate(&lock,globals->string,globals->packet->dp_Type,0);
                  }

                  if(errorcode==0) {
                    if((errorcode=createglobalhandle(lock))==0) {
                      fh->fh_Arg1=(IPTR)lock;
                    }
                    else if(globals->packet->dp_Type!=ACTION_FH_FROM_LOCK) {
                      freelock(lock);
                    }
                  }

                  if(errorcode!=0) {
                    returnpacket(DOSFALSE,errorcode);
                  }
                  else {
                    if(globals->packet->dp_Type==ACTION_FINDOUTPUT && globals->has_recycled!=FALSE) {
                      ULONG files,blocks;

                      if((errorcode=getrecycledinfo(&files, &blocks))==0) {
                        if(files>35) {
                          cleanupdeletedfiles();
                        }
                      }
                    }

                    returnpacket(DOSTRUE,0);
                  }
                }

                break;
              case ACTION_END:
                _XDEBUG((DEBUG_OBJECTS, "ACTION_END\n"));

                /* ACTION_FREE_LOCK's code is similair */

                {
                  struct ExtFileLock *lock=(struct ExtFileLock *)globals->packet->dp_Arg1;
                  LONG errorcode;

                  /* If a file has been modified by the use of ACTION_SET_FILE_SIZE or
                     ACTION_WRITE, or ACTION_FINDOUTPUT or ACTION_FINDUPDATE (only when
                     creating a new file) were used to create the file then we must
                     also update the datestamp of the file.  We also must send out a
                     notification.  Finally we also need to clear the A bit.

                     For this purpose a special flag in the lock tells us
                     whether or not any of the above actions has occured. */

                  if((lock->bits & EFL_MODIFIED) != 0) {
                    struct CacheBuffer *cb;
                    struct fsObject *o;

                    /* Aha! */

                    if(lock->lastextendedblock!=0) {
                      globals->block_rovingblockptr=lock->lastextendedblock;
                      if(globals->block_rovingblockptr>=globals->blocks_total) {
                        globals->block_rovingblockptr=0;
                      }
                    }

                    if((errorcode=readobject(lock->objectnode, &cb, &o))==0) {
                      newtransaction();

                      errorcode=bumpobject(cb, o);
                      checknotifyforobject(cb, o, TRUE);

                      if(errorcode==0) {
                        endtransaction();
                      }
                      else {
                        deletetransaction();
                      }
                    }

                    /* Ignore any errorcodes -- not really interesting when closing a file... */
                  }

                  if((errorcode=freelock(lock))!=0) {
                    returnpacket(DOSFALSE,errorcode);
                  }
                  else {
                    returnpacket(DOSTRUE,0);
                  }
                }
                break;
              case ACTION_DELETE_OBJECT:
                {
                  LONG errorcode;

                  copybstrasstr((BSTR)globals->packet->dp_Arg2,globals->string,258);

                  _DEBUG(("ACTION_DELETE_OBJECT(0x%08lx,'%s')\n",BADDR(globals->packet->dp_Arg1),globals->string));

                  do {
                    newtransaction();

                    if((errorcode=deleteobject(BADDR(globals->packet->dp_Arg1), validatepath(globals->string), TRUE))==0) {
                      endtransaction();
                    }
                    else {
                      deletetransaction();
                    }
                  } while(errorcode==ERROR_DISK_FULL && freeupspace()!=FALSE);

                  if(errorcode!=0) {
                    returnpacket(DOSFALSE,errorcode);
                  }
                  else {
                    ULONG files,blocks;

                    if((errorcode=getrecycledinfo(&files, &blocks))==0) {
                      if(files>35) {
                        cleanupdeletedfiles();
                      }
                    }

                    returnpacket(DOSTRUE,0);
                  }
                }
                break;
              case ACTION_RENAME_DISK:
                _DEBUG(("ACTION_RENAME_DISK\n"));

                {
                  struct CacheBuffer *cb;
                  LONG errorcode;

                  if((errorcode=readcachebuffercheck(&cb,globals->block_root,OBJECTCONTAINER_ID))==0) {
                    if(AttemptLockDosList(LDF_WRITE|LDF_VOLUMES)!=0) {
                      struct fsObjectContainer *oc=cb->data;
                      UBYTE *s;
                      UBYTE *d;
#ifndef USE_FAST_BSTR
                      UBYTE len;
#endif

                      /* Succesfully locked the doslist */

                      newtransaction();

                      preparecachebuffer(cb);
                      

                      s=BADDR(globals->packet->dp_Arg1);
                      d=oc->object[0].name;
                      d[copybstrasstr((BSTR)globals->packet->dp_Arg1, d, 30)+1] = 0;
#if 0
                      len=*s++;

                      if(len>30) {
                        len=30;
                      }

                      while(len-->0) {
                        *d++=*s++;
                      }
                      *d++=0;
                      *d=0;    /* Zero for comment */
#endif
                      if((errorcode=storecachebuffer(cb))==0 && globals->volumenode!=0) {
                        s=BADDR(globals->packet->dp_Arg1);
                        d=BADDR(globals->volumenode->dl_Name);
#ifdef USE_FAST_BSTR
                        copystr(s, d, 30);
#else
                        len=*s++;

                        if(len>30) {
                          len=30;
                        }

                        *d++=len;
                        while(len-->0) {
                          *d++=*s++;
                        }
                        *d=0;
#endif
                      }

                      UnLockDosList(LDF_WRITE|LDF_VOLUMES);

                      if(errorcode==0) {
                        endtransaction();
                      }
                      else {
                        deletetransaction();
                      }
                    }
                    else {
                      /* Doslist locking attempt was unsuccesful.  Send this message back to our
                         port to try it again later. */
  
                      PutMsg(msgportpackets,msg);
                      break;
                    }
                  }
  
                  if(errorcode==0) {
                    returnpacket(DOSTRUE,0);
                  }
                  else {
                    returnpacket(DOSFALSE,errorcode);
                  }
                }
                break;
              case ACTION_RENAME_OBJECT:
                _DEBUG(("ACTION_RENAME_OBJECT\n"));

                {
                  LONG errorcode;

                  do {
                    struct CacheBuffer *cb;
                    struct fsObject *o;
                    struct ExtFileLock *lock;
                    UBYTE *newname;
                    UBYTE *s;

                    lock=BADDR(globals->packet->dp_Arg1);
                    copybstrasstr((BSTR)globals->packet->dp_Arg2,globals->string,258);

                    if((errorcode=locateobjectfromlock(lock,validatepath(globals->string),&cb,&o))==0) {
                      copybstrasstr((BSTR)globals->packet->dp_Arg4,globals->string,258);

                      settemporarylock(BE2L(o->be_objectnode));

                      newname=validatepath(globals->string);
                      s=FilePart(newname);

                      if(*s!=0) {
                        newtransaction();
                        if((errorcode=renameobject(cb,o,BADDR(globals->packet->dp_Arg3),newname))==0) {
                          endtransaction();
                        }
                        else {
                          deletetransaction();
                        }
                      }
                      else {
                        errorcode=ERROR_INVALID_COMPONENT_NAME;
                      }
                    }
                  } while(errorcode==ERROR_DISK_FULL && freeupspace()!=FALSE);

                  cleartemporarylock();

                  if(errorcode!=0) {
                    returnpacket(DOSFALSE,errorcode);
                  }
                  else {
                    returnpacket(DOSTRUE,0);
                  }
                }
                break;
              case ACTION_SET_COMMENT:
                _DEBUG(("ACTION_SET_COMMENT\n"));
                {
                  LONG errorcode;

                  do {
                    copybstrasstr((BSTR)globals->packet->dp_Arg3,globals->string,258);
                    copybstrasstr((BSTR)globals->packet->dp_Arg4,globals->string2,258);

                    newtransaction();
                    if((errorcode=setcomment(BADDR(globals->packet->dp_Arg2),validatepath(globals->string),globals->string2))!=0) {
                      deletetransaction();
                    }
                    else {
                      endtransaction();
                    }
                  } while(errorcode==ERROR_DISK_FULL && freeupspace()!=FALSE);

                  if(errorcode!=0) {
                    returnpacket(DOSFALSE,errorcode);
                  }
                  else {
                    returnpacket(DOSTRUE,0);
                  }
                }
                break;
              case ACTION_SET_DATE:
              case ACTION_SET_PROTECT:
              case ACTION_SET_OWNER:
              case ACTION_SFS_SET_OBJECTBITS:
                _XDEBUG((DEBUG_OBJECTS, "ACTION_SET_DATE or ACTION_SET_PROTECT or ACTION_SET_OWNER\n"));

                {
                  LONG errorcode;

                  do {
                    struct CacheBuffer *cb;
                    struct fsObject *o;
                    struct ExtFileLock *lock;

                    lock=BADDR(globals->packet->dp_Arg2);
                    copybstrasstr((BSTR)globals->packet->dp_Arg3,globals->string,258);

                    if((errorcode=locatelockableobject(lock,validatepath(globals->string),&cb,&o))==0) {
                      NODE objectnode=BE2L(o->be_objectnode);

                      if(objectnode!=ROOTNODE) {

                        settemporarylock(objectnode);

                        newtransaction();
                        preparecachebuffer(cb);

                        if(globals->packet->dp_Type==ACTION_SET_DATE) {
                          checksum_writelong_be(cb->data, &o->be_datemodified, datestamptodate((struct DateStamp *)globals->packet->dp_Arg4));

                          // o->datemodified=datestamptodate((struct DateStamp *)packet->dp_Arg4);
                        }
                        else if(globals->packet->dp_Type==ACTION_SET_PROTECT) {
                          checksum_writelong_be(cb->data, &o->be_protection, globals->packet->dp_Arg4^(FIBF_READ|FIBF_WRITE|FIBF_EXECUTE|FIBF_DELETE));

                          // o->protection=packet->dp_Arg4^(FIBF_READ|FIBF_WRITE|FIBF_EXECUTE|FIBF_DELETE);
                        }
                        else if(globals->packet->dp_Type==ACTION_SET_OWNER) {
                          // ULONG *owner=(ULONG *)&o->owneruid;

                          checksum_writelong_be(cb->data, (ULONG *)&o->be_owneruid, globals->packet->dp_Arg4);

                          // *owner=packet->dp_Arg4;
                        }
                        else {
                          o->bits=(globals->packet->dp_Arg4 & (OTYPE_HIDDEN|OTYPE_UNDELETABLE)) | (o->bits & ~(OTYPE_HIDDEN|OTYPE_UNDELETABLE));
                          setchecksum(cb);      // new
                        }

                        if((errorcode=storecachebuffer_nochecksum(cb))==0) {
                          struct GlobalHandle *gh;

                          checknotifyforobject(cb,o,TRUE);
                          endtransaction();

                          if(globals->packet->dp_Type==ACTION_SET_PROTECT && (gh=findglobalhandle(objectnode))!=0) {
                            gh->protection=globals->packet->dp_Arg4^(FIBF_READ|FIBF_WRITE|FIBF_EXECUTE|FIBF_DELETE);
                          }
                        }
                        else {
                          deletetransaction();
                        }
                      }
                      else {
                        errorcode=ERROR_OBJECT_WRONG_TYPE;
                      }
                    }

                    /* this 'loop' will only be left if the disk wasn't full, or
                       cleanupdeletedfiles() couldn't free up any more space. */

                  } while(errorcode==ERROR_DISK_FULL && freeupspace()!=FALSE);

                  cleartemporarylock();

                  if(errorcode!=0) {
                    returnpacket(DOSFALSE,errorcode);
                  }
                  else {
                    returnpacket(DOSTRUE,0);
                  }
                }
                break;
              case ACTION_SET_FILE_SIZE:
                _DEBUG(("ACTION_SET_FILE_SIZE(0x%08lx,0x%08lx,0x%08lx)\n",globals->packet->dp_Arg1,globals->packet->dp_Arg2,globals->packet->dp_Arg3));

                {
                  struct ExtFileLock *lock=(struct ExtFileLock *)globals->packet->dp_Arg1;
                  LONG newfilesize;
                  LONG errorcode;

                  if(globals->packet->dp_Arg3==OFFSET_BEGINNING) {
                    newfilesize=globals->packet->dp_Arg2;
                  }
                  else if(globals->packet->dp_Arg3==OFFSET_END) {
                    newfilesize=lock->gh->size+globals->packet->dp_Arg2;
                  }
                  else if(globals->packet->dp_Arg3==OFFSET_CURRENT) {
                    newfilesize=lock->offset+globals->packet->dp_Arg2;
                  }
                  else {
                    returnpacket(-1,ERROR_BAD_NUMBER);
                    break;
                  }

                  if(newfilesize>=0) {
                    ULONG data=lock->gh->data;

                    do {
                      errorcode=setfilesize(lock,newfilesize);
                    } while(errorcode==ERROR_DISK_FULL && freeupspace()!=FALSE);

                    if(errorcode!=0) {
                      lock->gh->data=data;
                    }
                  }
                  else {
                    errorcode=ERROR_SEEK_ERROR;
                  }

                  if(errorcode==0) {
                    returnpacket(lock->gh->size,0);
                  }
                  else {
                    returnpacket(-1,errorcode);
                  }
                }
                break;
              case ACTION_SEEK:
                _XDEBUG((DEBUG_SEEK,"ACTION_SEEK(0x%08lx,0x%08lx,0x%08lx)\n",globals->packet->dp_Arg1,globals->packet->dp_Arg2,globals->packet->dp_Arg3));
  
                {
                  struct ExtFileLock *lock=(struct ExtFileLock *)globals->packet->dp_Arg1;
                  struct GlobalHandle *gh=lock->gh;
                  ULONG oldpos=lock->offset;
                  LONG newpos;
                  LONG errorcode;
  
                  if(globals->packet->dp_Arg3==OFFSET_BEGINNING) {
                    newpos=globals->packet->dp_Arg2;
                  }
                  else if(globals->packet->dp_Arg3==OFFSET_END) {
                    newpos=gh->size+globals->packet->dp_Arg2;
                  }
                  else if(globals->packet->dp_Arg3==OFFSET_CURRENT) {
                    newpos=lock->offset+globals->packet->dp_Arg2;
                  }
                  else {
                    returnpacket(-1,ERROR_BAD_NUMBER);
                    break;
                  }
  
                  if(newpos>=0 && newpos<=gh->size) {
                    if(newpos!=oldpos) {
                      errorcode=seek(lock,newpos);
                    }
                    else {
                      errorcode=0;
                    }
                  }
                  else {
                    errorcode=ERROR_SEEK_ERROR;
                  }

                  if(errorcode==0) {
                    returnpacket(oldpos,0);
                  }
                  else {
                    returnpacket(-1,errorcode);
                  }
                }
                break;
              case ACTION_READ:
                _XDEBUG((DEBUG_IO,"ACTION_READ(0x%08lx,0x%08lx,%ld)\n",globals->packet->dp_Arg1,globals->packet->dp_Arg2,globals->packet->dp_Arg3));

                {
                  struct ExtFileLock *lock=(struct ExtFileLock *)globals->packet->dp_Arg1;
                  struct GlobalHandle *gh=lock->gh;
                  UBYTE *buffer=(UBYTE *)globals->packet->dp_Arg2;
                  ULONG bytesleft;
                  UBYTE *startofbuf=buffer;
                  LONG errorcode;

                  bytesleft=globals->packet->dp_Arg3;
                  if(lock->offset+bytesleft > gh->size) {
                    bytesleft=gh->size-lock->offset;
                  }

                  if((errorcode=seektocurrent(lock))==0) {
                    if((gh->protection & FIBF_READ)!=0) {
                      struct CacheBuffer *extent_cb;
                      struct fsExtentBNode *ebn;

                      while(bytesleft>0 && (errorcode=findextentbnode(lock->curextent, &extent_cb, &ebn))==0) {
                        ULONG bytestoread;
                        ULONG offsetinblock=lock->extentoffset & globals->mask_block;
                        BLCK ebn_next=BE2L(ebn->be_next);
                        UWORD ebn_blocks=BE2W(ebn->be_blocks);

                        _XDEBUG((DEBUG_IO,"ACTION_READ: bytesleft = %ld, offsetinblock = %ld, ExtentBNode = %ld, ebn->data = %ld, ebn->blocks = %ld\n",bytesleft,offsetinblock,lock->curextent,BE2L(ebn->be_key),BE2W(ebn->be_blocks)));

                        if(offsetinblock!=0 || bytesleft<globals->bytes_block) {

                          // _XDEBUG((DEBUG_IO,"ACTION_READ: nextextentbnode = %ld, extentnodesize = %ld, en->blocks = %ld\n",nextextentbnode,extentnodesize,(ULONG)ebn->blocks));

                          bytestoread=globals->bytes_block-offsetinblock;

                          /* Check if there are more bytes left in the block then we want to read */
                          if(bytestoread > bytesleft) {
                            bytestoread=bytesleft;
                          }

                          if((errorcode=readbytes(BE2L(ebn->be_key)+(lock->extentoffset>>globals->shifts_block), buffer, offsetinblock, bytestoread))!=0) {
                            break;
                          }
                        }
                        else {

                          bytestoread=(((ULONG)ebn_blocks)<<globals->shifts_block) - lock->extentoffset;

                          /* Check if there are more bytes left in the Extent then we want to read */
                          if(bytestoread > bytesleft) {
                            bytestoread=bytesleft & ~globals->mask_block;
                          }

                          if((errorcode=read(BE2L(ebn->be_key)+(lock->extentoffset>>globals->shifts_block), buffer, bytestoread>>globals->shifts_block))!=0) {
                            break;
                          }
                        }

                        seekforward(lock, ebn_blocks, ebn_next, bytestoread);

                        bytesleft-=bytestoread;
                        buffer+=bytestoread;

                        _XDEBUG((DEBUG_IO,"ACTION_READ: bytesleft = %ld, errorcode = %ld\n",bytesleft,errorcode));
                      }
                    }
                    else {
                      errorcode=ERROR_READ_PROTECTED;
                    }
                  }

                  _XDEBUG((DEBUG_IO,"ACTION_READ: errorcode = %ld, buffer = %ld, startofbuf = %ld\n",errorcode,buffer,startofbuf));

                  if(errorcode!=0) {
                    returnpacket(-1,errorcode);
                  }
                  else {
                    returnpacket(buffer-startofbuf,0);
                  }
                }
                break;
              case ACTION_WRITE:
                _XDEBUG((DEBUG_IO,"ACTION_WRITE(0x%08lx,0x%08lx,%ld)\n",globals->packet->dp_Arg1,globals->packet->dp_Arg2,globals->packet->dp_Arg3));

                {
                  struct ExtFileLock *lock=(struct ExtFileLock *)globals->packet->dp_Arg1;
                  ULONG bytestowrite=globals->packet->dp_Arg3;
                  LONG errorcode=0;

                  do {
                    struct GlobalHandle *gh=lock->gh;

                    /* Save some values in case of an error: */

                    BLCK curextent=lock->curextent;
                    ULONG extentoffset=lock->extentoffset;
                    ULONG offset=lock->offset;
                    ULONG size=gh->size;
                    ULONG data=gh->data;

                    if(bytestowrite!=0) {
                      newtransaction();

                      if((errorcode=writetofile(lock, (UBYTE *)globals->packet->dp_Arg2, bytestowrite))==0) {
                        endtransaction();
                      }
                      else {
                        lock->curextent=curextent;
                        lock->extentoffset=extentoffset;
                        lock->offset=offset;
                        gh->size=size;
                        gh->data=data;

                        deletetransaction();
                      }
                    }
                  } while(errorcode==ERROR_DISK_FULL && freeupspace()!=FALSE);

                  if(errorcode!=0) {
                    _XDEBUG((DEBUG_IO,"ACTION_WRITE returns (-1, %ld)\n",errorcode));

                    returnpacket(-1,errorcode);
                  }
                  else {
                    _XDEBUG((DEBUG_IO,"ACTION_WRITE returns (%ld, 0)\n",bytestowrite));

                    lock->bits|=EFL_MODIFIED;
                    returnpacket(bytestowrite,0);
                  }
                }
                break;
              case ACTION_FREE_LOCK:
                _XDEBUG((DEBUG_LOCK,"ACTION_FREE_LOCK\n"));

                {
                  LONG errorcode;

                  if((errorcode=freelock((struct ExtFileLock *)BADDR(globals->packet->dp_Arg1)))!=0) {
                    returnpacket(DOSFALSE,errorcode);
                    break;
                  }
                  returnpacket(DOSTRUE,0);
                }
                break;
              case ACTION_EXAMINE_ALL:
                _DEBUG(("ACTION_EXAMINE_ALL\n"));

                {
                  struct ExtFileLock *lock;
                  struct ExAllData *ead;
                  struct ExAllData *prevead=0;
                  struct ExAllControl *eac;
                  struct CacheBuffer *cb;
                  struct fsObject *o;
                  ULONG eadsize;
                  ULONG stringsize;
                  LONG spaceleft;
                  LONG errorcode;

                  lock=(struct ExtFileLock *)BADDR(globals->packet->dp_Arg1);
                  ead=(struct ExAllData *)globals->packet->dp_Arg2;
                  eac=(struct ExAllControl *)globals->packet->dp_Arg5;
                  spaceleft=globals->packet->dp_Arg3;

                  eac->eac_Entries=0;

                  if(lock==0) {
                    _DEBUG(("ACTION_EXAMINE_ALL: Zero lock was passed in...\n"));
  
                    returnpacket(DOSFALSE,ERROR_OBJECT_WRONG_TYPE);
                    break;
                  }
  
                  if(globals->packet->dp_Arg4>ED_OWNER) {
                    returnpacket(DOSFALSE,ERROR_BAD_NUMBER);
                    break;
                  }
  
                  if(eac->eac_LastKey==0) {
                    if((errorcode=readobject(lock->objectnode,&cb,&o))==0) {
                      if((o->bits & OTYPE_DIR)!=0) {
                        if(o->object.dir.be_firstdirblock!=0) {
                          if((errorcode=readcachebuffercheck(&cb,BE2L(o->object.dir.be_firstdirblock),OBJECTCONTAINER_ID))==0) {
                            struct fsObjectContainer *oc=cb->data;
  
                            o=oc->object;
                            eac->eac_LastKey=BE2L(o->be_objectnode);
                          }
                        }
                        else {
                          errorcode=ERROR_NO_MORE_ENTRIES;
                        }
                      }
                      else {
                        errorcode=ERROR_OBJECT_WRONG_TYPE;
                      }
                    }
                  }
                  else {
                    if((errorcode=readobject(eac->eac_LastKey,&cb,&o))==ERROR_IS_SOFT_LINK) {
                      errorcode=0;
                    }
                  }
  
                  while(errorcode==0) {
                    WORD namelength=strlen(o->name);
                    WORD keepentry;
  
                    stringsize=0;
                    eadsize=0;
  
                    switch(globals->packet->dp_Arg4) {
                    default:
                    case ED_OWNER:
                      eadsize += 4;			/* ed_OwnedGID, ed_OwnedUID */
                    case ED_COMMENT:
                      stringsize+=strlen(o->name+namelength+1)+1;
                      eadsize += sizeof(UBYTE *);	/* ed_Comment */
                    case ED_DATE:
                      eadsize += 12;			/* ed_Ticks, ed_Mins, ed_Days */
                    case ED_PROTECTION:
                      eadsize += 4;			/* ed_Prot */
                    case ED_SIZE:
                      eadsize += 4;			/* ed_Size */
                    case ED_TYPE:
                      eadsize += 4;
                    case ED_NAME:
                      stringsize += namelength+1;
                      eadsize += sizeof(APTR) * 2;	/* ed_Name, ed_Next */
                      break;
                    }

//                    _DEBUG(("ACTION_EXAMINE_ALL: eadsize = %ld, stringsize = %ld, spaceleft = %ld, packet->dp_Arg4 = %ld\n",eadsize,stringsize,spaceleft,packet->dp_Arg4));

                    if(spaceleft<eadsize+stringsize) {
                      break;
                    }

                    switch(globals->packet->dp_Arg4) {
                    default:
                    case ED_OWNER:
                      ead->ed_OwnerUID=BE2W(o->be_owneruid);
                      ead->ed_OwnerGID=BE2W(o->be_ownergid);
                    case ED_COMMENT:
                      {
                        UBYTE *src=o->name+namelength+1;
                        UBYTE *dest=(UBYTE *)ead+eadsize;

                        ead->ed_Comment=dest;

                        while(*src!=0) {
                          *dest++=*src++;
                          eadsize++;
                        }

                        *dest=0;
                        eadsize++;
                      }
                    case ED_DATE:
                      datetodatestamp(BE2L(o->be_datemodified),(struct DateStamp *)&ead->ed_Days);
                    case ED_PROTECTION:
                      ead->ed_Prot=BE2L(o->be_protection)^(FIBF_READ|FIBF_WRITE|FIBF_EXECUTE|FIBF_DELETE);
                    case ED_SIZE:
                      if((o->bits & OTYPE_DIR)==0) {
                        ead->ed_Size=BE2L(o->object.file.be_size);
                      }
                      else {
                        ead->ed_Size=0;
                      }
                    case ED_TYPE:
_DEBUG(("examine ED_TYPE, o->bits=%x, o->objectnode=%d\n", o->bits, BE2L(o->be_objectnode)));
                      if((o->bits & OTYPE_LINK)!=0) {
                        ead->ed_Type=ST_SOFTLINK;
                      }
                      if((o->bits & OTYPE_DIR)==0) {
                        ead->ed_Type=ST_FILE;
                      }
                      else if (o->be_objectnode == L2BE(ROOTNODE)) {
                        ead->ed_Type = ST_ROOT;
                        }
                      else {
                        ead->ed_Type=ST_USERDIR;
                      }
                    case ED_NAME:
                      {
                        UBYTE *src=o->name;
                        UBYTE *dest=(UBYTE *)ead+eadsize;

                        ead->ed_Name=dest;

                        while(*src!=0) {
                          *dest++=*src++;
                          eadsize++;
                        }

                        *dest=0;
                        eadsize++;

  //                    _DEBUG(("Stored entry %s\n",ead->ed_Name));
                      }
                    }

                    if(eac->eac_MatchString!=0) {
                      keepentry=MatchPatternNoCase(eac->eac_MatchString,ead->ed_Name);
                    }
                    else {
                      keepentry=DOSTRUE;
                    }

                    if(keepentry!=DOSFALSE && eac->eac_MatchFunc!=0) {
                      
#ifdef __AROS__
                      keepentry=CALLHOOKPKT(eac->eac_MatchFunc, ead, (APTR)globals->packet->dp_Arg4);
#else
                      LONG __asm(*hookfunc)(register __a0 struct Hook *,register __a1 struct ExAllData *,register __a2 ULONG)=(LONG __asm(*)(register __a0 struct Hook *,register __a1 struct ExAllData *,register __a2 ULONG))eac->eac_MatchFunc->h_Entry;
                      keepentry=hookfunc(eac->eac_MatchFunc,ead,packet->dp_Arg4);
#endif
                    }

                    if(keepentry!=DOSFALSE && (o->bits & OTYPE_HIDDEN)==0) {
                      ead->ed_Next=0;
                      eadsize = (eadsize + sizeof(APTR) - 1) & ~(sizeof(APTR) - 1);
                      if(prevead!=0) {
                        prevead->ed_Next=ead;
                      }
                      prevead=ead;
                      ead=(struct ExAllData *)((UBYTE *)ead+eadsize);
                      spaceleft-=eadsize;
                      eac->eac_Entries++;
                    }
  
                    {
                      struct fsObjectContainer *oc=cb->data;
                      UBYTE *endadr;
  
                      o=nextobject(o);
  
                      endadr=(UBYTE *)oc+globals->bytes_block-sizeof(struct fsObject)-2;
  
                      if((UBYTE *)o>=endadr || o->name[0]==0) {
                        if(oc->be_next!=0) {
                          if((errorcode=readcachebuffercheck(&cb,BE2L(oc->be_next),OBJECTCONTAINER_ID))==0) {
                            struct fsObjectContainer *oc=cb->data;
  
                            o=oc->object;
                            eac->eac_LastKey=BE2L(o->be_objectnode);
                          }
                        }
                        else {
                          errorcode=ERROR_NO_MORE_ENTRIES;
                        }
                      }
                      else {
                        eac->eac_LastKey=BE2L(o->be_objectnode);
                      }
                    }
                  }
  
                  if(errorcode!=0) {
                    returnpacket(DOSFALSE,errorcode);
                  }
                  else {
                    returnpacket(DOSTRUE,0);
                  }
                }
                break;
              case ACTION_EXAMINE_NEXT:
                // _DEBUG(("ACTION_EXAMINE_NEXT(0x%08lx,0x%08lx)\n",BADDR(packet->dp_Arg1),BADDR(packet->dp_Arg2)));

                /* An entry is added to a directory in the first dir block with
                   enough space to hold the entry.  If there is no space, then
                   a new block is added at the START of the directory, and the
                   new entry is added there.

                   In the directory block with enough space, the entry is added
                   at the end of the block.  The order of directory entries there
                   fore is like this (square parenthesis indicate blocks):

                   [789] [456] [123]

                   This means we should scan the directory in the exact opposite
                   order to avoid a trap when an entry is overwritten during
                   directory scanning (MODE_NEW_FILE removes old entry, and adds
                   a new one).  See below:

                   1. [123]; scan is at 1 (next = 2); 1 is overwritten.

                   2. [231]; scan is at 2 (next = 3); 2 is overwritten.

                   3. [312]; scan is at 3 (next = 1); 3 is overwritten.

                   4. [123]; same as 1 -> loop!

                   By scanning the internals of a dir block backwards the problem
                   is solved:

                   1. [123]; scan is at 3 (next = 2); 3 is overwritten.

                   2. [123]; scan is at 2 (next = 1); 2 is overwritten.

                   3. [132]; scan is at 1 (next = next block); 1 is overwritten.

                   4. new block is loaded -> no loop. */

                {
                  struct ExtFileLock *lock;
                  struct CacheBuffer *cb;
                  struct FileInfoBlock *fib=BADDR(globals->packet->dp_Arg2);
                  LONG errorcode=0;

                  lock=(struct ExtFileLock *)BADDR(globals->packet->dp_Arg1);

                  if(lock==0) {
                    _DEBUG(("ACTION_EXAMINE_NEXT: Zero lock was passed in...\n"));

                    returnpacket(DOSFALSE,ERROR_OBJECT_WRONG_TYPE);
                    break;
                  }

                  /*
                  if(lock->ocblck==0 && lock->ocnode==0) {
                    _DEBUG(("ACTION_EXAMINE_NEXT: Lock was never passed to EXAMINE_OBJECT or has been re-allocated or the object Examine()d was a file\n"));

                    returnpacket(DOSFALSE,ERROR_OBJECT_WRONG_TYPE);
                    break;
                  }
                  */

                  if(lock->currentnode!=0xFFFFFFFF && fib->fib_DiskKey!=lock->currentnode) {
                    struct fsObject *o;

                    /* It looks like the lock was reallocated!  In this case we must rebuild
                       the state information in the lock.  lock->currentnode, lock->nextnode
                       and lock->nextnodeblock. */

                    if((errorcode=readobject(fib->fib_DiskKey, &cb, &o))==0) {
                      struct fsObjectContainer *oc=cb->data;

                      lock->currentnode=fib->fib_DiskKey;

                      if((o=prevobject(o, oc))!=0) {
                        lock->nextnodeblock=cb->blckno;
                        lock->nextnode=BE2L(o->be_objectnode);
                      }
                      else {
                        lock->nextnodeblock=BE2L(oc->be_next);
                        lock->nextnode=0xFFFFFFFF;
                      }
                    }
                  }

                  if(lock->nextnodeblock==0) {
                    errorcode=ERROR_NO_MORE_ENTRIES;
                  }

                  /* The passed in lock describes a directory.  EXAMINE_NEXT should return
                     this directory's entries one at the time.  The lock has state information
                     which helps determine at which entry we currently are. */

                  while(errorcode==0 && (errorcode=readcachebuffercheck(&cb, lock->nextnodeblock, OBJECTCONTAINER_ID))==0) {
                    struct fsObjectContainer *oc=cb->data;
                    struct fsObject *o=0;
                    UBYTE bits;

                    if(lock->nextnode!=0xFFFFFFFF) {
                      /*** It is possible findobject returns 0... */
                      o=findobject(oc, lock->nextnode);
                    }

                    if(o==0) {
                      o=lastobject(oc);
                    }

                    bits=o->bits;
                    fillfib(fib, o);
                    lock->currentnode=BE2L(o->be_objectnode);

                    /* prepare for another EXAMINE_NEXT */

                    /* We need to check if there is another object in this ObjectContainer
                       following the one we just returned.  If there is then return its
                       node.  If there isn't then return the next ObjectContainer ptr and
                       set ocnode to zero. */

                    if((o=prevobject(o, oc))!=0) {
                      /* There IS another object */
                      lock->nextnode=BE2L(o->be_objectnode);
                    }
                    else {
                      lock->nextnodeblock=BE2L(oc->be_next);
                      lock->nextnode=0xFFFFFFFF;
                    }

                    if((bits & OTYPE_HIDDEN)==0) {
                      break;
                    }
                    else if(lock->nextnodeblock==0) {
                      errorcode=ERROR_NO_MORE_ENTRIES;
                      break;
                    }
                  }

                  if(errorcode==0) {
                    returnpacket(DOSTRUE,0);
                  }
                  else {
                    returnpacket(DOSFALSE,errorcode);
                  }
                }
                break;
#if 0
/******** OLD EXAMINE_NEXT CODE! */
              case ACTION_EXAMINE_NEXT:
                // _DEBUG(("ACTION_EXAMINE_NEXT(0x%08lx,0x%08lx)\n",BADDR(packet->dp_Arg1),BADDR(packet->dp_Arg2)));

                /* An entry is added to a directory in the first dir block with
                   enough space to hold the entry.  If there is no space, then
                   a new block is added at the START of the directory, and the
                   new entry is added there.

                   In the directory block with enough space, the entry is added
                   at the end of the block.  The order of directory entries there
                   fore is like this (square parenthesis indicate blocks):

                   [789] [456] [123]

                   This means we should scan the directory in the exact opposite
                   order to avoid a trap when an entry is overwritten during
                   directory scanning (MODE_NEW_FILE removes old entry, and adds
                   a new one).  See below:

                   1. [123]; scan is at 1 (next = 2); 1 is overwritten.

                   2. [231]; scan is at 2 (next = 3); 2 is overwritten.

                   3. [312]; scan is at 3 (next = 1); 3 is overwritten.

                   4. [123]; same as 1 -> loop!

                   By scanning the internals of a dir block backwards the problem
                   is solved:

                   1. [123]; scan is at 3 (next = 2); 3 is overwritten.

                   2. [123]; scan is at 2 (next = 1); 2 is overwritten.

                   3. [132]; scan is at 1 (next = next block); 1 is overwritten.

                   4. new block is loaded -> no loop. */

                {
                  struct ExtFileLock *lock;
                  struct CacheBuffer *cb;
                  struct FileInfoBlock *fib=BADDR(packet->dp_Arg2);
                  LONG errorcode=0;

                  lock=(struct ExtFileLock *)BADDR(packet->dp_Arg1);

                  if(lock==0) {
                    _DEBUG(("ACTION_EXAMINE_NEXT: Zero lock was passed in...\n"));

                    returnpacket(DOSFALSE,ERROR_OBJECT_WRONG_TYPE);
                    break;
                  }

                  /*
                  if(lock->ocblck==0 && lock->ocnode==0) {
                    _DEBUG(("ACTION_EXAMINE_NEXT: Lock was never passed to EXAMINE_OBJECT or has been re-allocated or the object Examine()d was a file\n"));

                    returnpacket(DOSFALSE,ERROR_OBJECT_WRONG_TYPE);
                    break;
                  }
                  */

                  if(lock->currentnode!=0xFFFFFFFF && fib->fib_DiskKey!=lock->currentnode) {
                    struct fsObject *o;

                    /* It looks like the lock was reallocated!  In this case we must rebuild
                       the state information in the lock.  lock->currentnode, lock->nextnode
                       and lock->nextnodeblock. */

                    if((errorcode=readobject(fib->fib_DiskKey, &cb, &o))==0) {
                      struct fsObjectContainer *oc=cb->data;

                      lock->currentnode=fib->fib_DiskKey;

                      o=nextobject(o);

                      if(isobject(o,oc)!=FALSE) {
                        lock->nextnodeblock=cb->blckno;
                        lock->nextnode=o->objectnode;
                      }
                      else {
                        lock->nextnodeblock=oc->next;
                        lock->nextnode=0xFFFFFFFF;
                      }
                    }
                  }

                  if(lock->nextnodeblock==0) {
                    errorcode=ERROR_NO_MORE_ENTRIES;
                  }

                  /* The passed in lock describes a directory.  EXAMINE_NEXT should return
                     this directory's entries one at the time.  The lock has state information
                     which helps determine at which entry we currently are. */

                  while(errorcode==0 && (errorcode=readcachebuffercheck(&cb,lock->nextnodeblock,OBJECTCONTAINER_ID))==0) {
                    struct fsObjectContainer *oc=cb->data;
                    struct fsObject *o=0;
                    UBYTE bits;

                    if(lock->nextnode!=0xFFFFFFFF) {
                      /*** It is possible findobject returns 0... */
                      o=findobject(oc,lock->nextnode);
                    }
                    if(o==0) {
                      o=oc->object;
                    }

                    bits=o->bits;
                    fillfib(fib,o);
                    lock->currentnode=o->objectnode;

                    /* prepare for another EXAMINE_NEXT */

                    /* We need to check if there is another object in this ObjectContainer
                       following the one we just returned.  If there is then return its
                       node.  If there isn't then return the next ObjectContainer ptr and
                       set ocnode to zero. */

                    o=nextobject(o);

                    if(isobject(o,oc)!=FALSE) {
                      /* There IS another object */
                      lock->nextnode=o->objectnode;
                    }
                    else {
                      lock->nextnodeblock=oc->next;
                      lock->nextnode=0xFFFFFFFF;
                    }

                    if((bits & OTYPE_HIDDEN)==0) {
                      break;
                    }
                    else if(lock->nextnodeblock==0) {
                      errorcode=ERROR_NO_MORE_ENTRIES;
                      break;
                    }
                  }

                  if(errorcode==0) {
                    returnpacket(DOSTRUE,0);
                  }
                  else {
                    returnpacket(DOSFALSE,errorcode);
                  }
                }
                break;
#endif

              case ACTION_EXAMINE_OBJECT:
              case ACTION_EXAMINE_FH:
                _DEBUG(("ACTION_EXAMINE_OBJECT\n"));

                {
                  struct ExtFileLock *lock;
                  struct CacheBuffer *cb;
                  struct fsObject *o;
                  NODE objectnode;
                  LONG errorcode;

                  if(globals->packet->dp_Type==ACTION_EXAMINE_OBJECT) {
                    lock=(struct ExtFileLock *)BADDR(globals->packet->dp_Arg1);
                  }
                  else {
                    lock=(struct ExtFileLock *)globals->packet->dp_Arg1;
                  }

                  if(lock==0) {
                    objectnode=ROOTNODE;
                  }
                  else {
                    objectnode=lock->objectnode;
                  }

                  errorcode=readobject(objectnode,&cb,&o);
                  if(errorcode==0 || errorcode==ERROR_IS_SOFT_LINK) {
                    fillfib((struct FileInfoBlock *)BADDR(globals->packet->dp_Arg2),o);

                    /* prepare for EXAMINE_NEXT */
                    if(lock!=0 && (o->bits & OTYPE_DIR)!=0) {
                      lock->currentnode=0xFFFFFFFF;
                      lock->nextnode=0xFFFFFFFF;
                      lock->nextnodeblock=BE2L(o->object.dir.be_firstdirblock);
                    }

                    returnpacket(DOSTRUE,0);
                  }
                  else {
                    returnpacket(DOSFALSE,errorcode);
                  }
                }

                break;
              case ACTION_INFO:
                _DEBUG(("ACTION_INFO\n"));

                {
                  struct ExtFileLock *lock=BADDR(globals->packet->dp_Arg1);
                  struct InfoData *id=BADDR(globals->packet->dp_Arg2);

                  if(lock!=0 && globals->volumenode!=(struct DeviceList *)BADDR(lock->volume)) {
                    _DEBUG(("ACTION_INFO: returning error\n"));

                    returnpacket(DOSFALSE,ERROR_DEVICE_NOT_MOUNTED);
                    break;
                  }
  
                  fillinfodata(id);
  
                  returnpacket(DOSTRUE,0);
                }
                break;
              case ACTION_MORE_CACHE:
                _DEBUG(("ACTION_MORE_CACHE\n"));

                {
                  LONG errorcode;
  
                  errorcode=addcachebuffers(globals->packet->dp_Arg1);
                  _DEBUG(("ACTION_MORE_CACHE: dp_Arg1 = %ld, totalbuffers = %ld, errorcode = %ld\n",globals->packet->dp_Arg1,globals->totalbuffers,errorcode));
  
                  if(errorcode==0) {
                    // returnpacket(DOSTRUE,totalbuffers);
  
                    returnpacket(globals->totalbuffers,0);
                  }
                  else {
                    returnpacket(DOSFALSE,errorcode);
                  }
                }
                break;
              case ACTION_CHANGE_MODE:
                {
                  struct ExtFileLock *lock=0;
                  LONG errorcode=0;
  
                  if(globals->packet->dp_Arg1==CHANGE_FH) {
                    lock=(struct ExtFileLock *)((struct FileHandle *)(BADDR(globals->packet->dp_Arg2)))->fh_Arg1;
                  }
                  else if(globals->packet->dp_Arg1==CHANGE_LOCK) {
                    lock=BADDR(globals->packet->dp_Arg2);
                  }
  
                  if(lock!=0) {
                    if(lock->access!=globals->packet->dp_Arg3) {
                      if(lock->access!=EXCLUSIVE_LOCK) {
                        /* Convert shared lock into an exclusive lock.  We need to check
                           if there are no locks besides this one, and that we aren't
                           trying to get an exclusive lock on the root (which is never
                           allowed). */

                        if(lock->objectnode!=ROOTNODE) {
                          NODE objectnode=lock->objectnode;
  
                          lock->objectnode=0;  /* this makes sure that our lock is not taken into account by lockable() */
  
                          if(lockable(objectnode,EXCLUSIVE_LOCK)!=DOSFALSE) {
                            /* Lockable says it is possible to lock it exclusively! */

                            lock->access=EXCLUSIVE_LOCK;
                          }
                          else {
                            errorcode=ERROR_OBJECT_IN_USE;
                          }
  
                          lock->objectnode=objectnode;
                        }
                        else {
                          errorcode=ERROR_OBJECT_IN_USE;
                        }
                      }
                      else {
                        /* Convert exclusive lock into a shared lock.  Should always be
                           possible. */
  
                        lock->access=SHARED_LOCK;
                      }
                    }
                  }
                  else {
                    errorcode=ERROR_OBJECT_WRONG_TYPE;
                  }
  
                  if(errorcode==0) {
                    returnpacket(DOSTRUE,0);
                  }
                  else {
                    returnpacket(DOSFALSE,errorcode);
                  }
                }
                break;
              case ACTION_PARENT:
              case ACTION_PARENT_FH:
                _DEBUG(("ACTION_PARENT\n"));

                {
                  LONG errorcode;
                  struct ExtFileLock *lock;
  
                  if(globals->packet->dp_Type==ACTION_PARENT) {
                    lock=(struct ExtFileLock *)BADDR(globals->packet->dp_Arg1);
                  }
                  else {
                    lock=(struct ExtFileLock *)globals->packet->dp_Arg1;
                  }
  
                  if(lock==0 || lock->objectnode==ROOTNODE) {
                    returnpacket(0,0);
                    break;
                  }
  
                  globals->string[0]='/';
                  globals->string[1]=0;
                  if((errorcode=lockobject(lock,globals->string,SHARED_LOCK,&lock))!=0) {
                    returnpacket(0,errorcode);
                  }
                  else {
                    returnpacket((SIPTR)TOBADDR(lock),0);
                  }
                }
                break;
              case ACTION_COPY_DIR:
              case ACTION_COPY_DIR_FH:
                _DEBUG(("ACTION_COPY_DIR\n"));
  
                {
                  LONG errorcode;
                  struct ExtFileLock *lock;
  
                  if(globals->packet->dp_Type==ACTION_COPY_DIR) {
                    lock=(struct ExtFileLock *)BADDR(globals->packet->dp_Arg1);
                  }
                  else {
                    lock=(struct ExtFileLock *)globals->packet->dp_Arg1;
                  }

                _DEBUG(("ACTION_COPY_DIR: lock=%p\n", lock));

                  if((errorcode=lockobject(lock,"",SHARED_LOCK,&lock))!=0) {
                    _DEBUG(("ACTION_COPY_DIR: Failed to obtain lock!\n"));
                    returnpacket(0,errorcode);
                  }
                  else {
                    returnpacket((SIPTR)TOBADDR(lock),0);
                  }
                }
                break;
              case ACTION_LOCATE_OBJECT:
                {
                  struct ExtFileLock *lock;
                  LONG errorcode;

                  copybstrasstr((BSTR)globals->packet->dp_Arg2,globals->string,258);

                  _XDEBUG((DEBUG_LOCK,"ACTION_LOCATE_OBJECT(0x%08lx,'%s',0x%08lx)\n",BADDR(globals->packet->dp_Arg1),globals->string,globals->packet->dp_Arg3));

                  if((errorcode=lockobject((struct ExtFileLock *)BADDR(globals->packet->dp_Arg1),validatepath(globals->string),globals->packet->dp_Arg3,&lock))!=0) {
                    returnpacket(0,errorcode);
                  }
                  else {
                    returnpacket((SIPTR)TOBADDR(lock),0);
                  }
                }
                break;
              case ACTION_SFS_LOCATE_OBJECT:
                {
                  struct CacheBuffer *cb;
                  struct fsObject *o;
                  struct ExtFileLock *lock=0;
                  LONG errorcode;

                  if((errorcode=readobject(globals->packet->dp_Arg1, &cb, &o))==0) {
                    errorcode=lockobject2(o, globals->packet->dp_Arg2, &lock);
                  }

                  if(errorcode!=0) {
                    returnpacket(0, errorcode);
                  }
                  else {
                    returnpacket((IPTR)lock, 0);
                  }
                }
                break;
              case ACTION_ADD_NOTIFY:
                {
                  struct NotifyRequest *nr;

                  nr=(struct NotifyRequest *)globals->packet->dp_Arg1;

                  nr->nr_Next = (IPTR)globals->notifyrequests;
                  nr->nr_Prev = 0;
                  if (globals->notifyrequests)
                    globals->notifyrequests->nr_Prev = (IPTR)nr;
                  globals->notifyrequests = nr;

                  _DEBUG(("ACTION_ADD_NOTIFY: Starting notification on %s (flags 0x%08lx)\n",nr->nr_FullName,nr->nr_Flags));

                  if((nr->nr_Flags & NRF_NOTIFY_INITIAL)!=0) {
                    notify(nr);
                  }

                  returnpacket(DOSTRUE,0);
                }
                break;
              case ACTION_REMOVE_NOTIFY:
                {
                  struct NotifyRequest *nr;

                  nr=(struct NotifyRequest *)globals->packet->dp_Arg1;

                  _DEBUG(("ACTION_REMOVE_NOTIFY: Removing notification of %s\n",nr->nr_FullName));

                  if((nr->nr_Flags & NRF_SEND_MESSAGE) != 0) {
                    /* Removing all outstanding messages form msgport */
                    while(GetMsg(nr->nr_stuff.nr_Msg.nr_Port)!=0) {
                    }
                    nr->nr_MsgCount=0;
                  }

                  if(nr->nr_Prev)
                    ((struct NotifyRequest *)nr->nr_Prev)->nr_Next = nr->nr_Next;
                  else
                    globals->notifyrequests = (struct NotifyRequest *)nr->nr_Next;

                  if (nr->nr_Next)
                    ((struct NotifyRequest *)nr->nr_Next)->nr_Prev = nr->nr_Prev;

                  nr->nr_Next = 0;
                  nr->nr_Prev = 0;

                  returnpacket(DOSTRUE,0);
                }
                break;
              case ACTION_FLUSH:
                {
                  LONG errorcode;

                  if((errorcode=flushcaches())==0) {
                    returnpacket(DOSTRUE, 0);
                  }
                  else {
                    returnpacket(DOSFALSE, errorcode);
                  }
                }
                break;
              case ACTION_SFS_READ_BITMAP:
                {
                  LONG errorcode;

                  Forbid();
                  Permit();

                  errorcode=extractspace((UBYTE *)globals->packet->dp_Arg1, globals->packet->dp_Arg2, globals->packet->dp_Arg3);

                  returnpacket(errorcode==0 ? DOSTRUE : DOSFALSE, errorcode);
                }
                break;
              case ACTION_SFS_DEFRAGMENT_INIT:
                {
                  globals->block_defragptr=2;

                  returnpacket(DOSTRUE, 0);
                }
                break;
              case ACTION_SFS_DEFRAGMENT_STEP:
                {
                  LONG errorcode;

                  globals->defragmentsteps=(ULONG *)globals->packet->dp_Arg1;
                  globals->defragmentlongs=globals->packet->dp_Arg2 - 2;

                  if(globals->defragmentsteps!=0) {
                    *globals->defragmentsteps=0;
                  }

                  while((errorcode=flushtransaction())!=0 && req("Pending buffers couldn't be flushed\nto the disk before defragmentation\nbecause of error %ld.", "Retry|Cancel", errorcode)==1) {
                  }

                  if(errorcode==0) {

                    newtransaction();

                    if((errorcode=step())!=0) {
                      deletetransaction();
                    }
                    else {
                      endtransaction();

                      while((errorcode=flushtransaction())!=0 && req("Pending buffers couldn't be flushed\nto the disk during defragmentation\nbecause of error %ld.", "Retry|Cancel", errorcode)==1) {
                      }
                    }
                  }

                  if(errorcode==0) {
                    struct DefragmentStep *ds=(struct DefragmentStep *)globals->packet->dp_Arg1;

                    while(ds->id!=0) {
                      if(ds->id==AROS_LONG2BE(MAKE_ID('M','O','V','E')) && ds->length==3) {
                        updatelocksaftermove(ds->data[1], ds->data[2], ds->data[0]);
                      }
                      ds=(struct DefragmentStep *)((ULONG *)ds + 2 + ds->length);
                    }

                    returnpacket(DOSTRUE, 0);
                  }
                  else {
                    returnpacket(DOSFALSE, errorcode);
                  }
                }
                break;
              default:
                _DEBUG(("ERROR_ACTION_NOT_KNOWN (packettype = %ld)\n",globals->packet->dp_Type));
                returnpacket(DOSFALSE,ERROR_ACTION_NOT_KNOWN);
                break;
              }
            }
            else if(globals->disktype==ID_NO_DISK_PRESENT) {
              dumppackets(globals->packet,ERROR_NO_DISK);
            }
            else {
              dumppackets(globals->packet,ERROR_NOT_A_DOS_DISK);
            }
          }
          break;
        }
      }
    } while(msg!=0);
  }
}



static void fillfib(struct FileInfoBlock *fib,struct fsObject *o)
{
  UBYTE *src;
  UBYTE *dest;
  UBYTE length;

  if (o->be_objectnode==L2BE(ROOTNODE)) {
    fib->fib_DirEntryType=ST_ROOT;
    fib->fib_Size=BE2L(o->object.file.be_size);
    fib->fib_NumBlocks=(BE2L(o->object.file.be_size)+globals->bytes_block-1) >> globals->shifts_block;
  } else if((o->bits & OTYPE_LINK)!=0) {
    fib->fib_DirEntryType=ST_SOFTLINK;
//  fib->fib_DirEntryType=ST_USERDIR;   // For compatibility with Diavolo 3.4 -> screw it, DOpus fails...
    fib->fib_Size=BE2L(o->object.file.be_size);
    fib->fib_NumBlocks=0;
  }
  else if((o->bits & OTYPE_DIR)==0) {
    fib->fib_DirEntryType=ST_FILE;
    fib->fib_Size=BE2L(o->object.file.be_size);
    fib->fib_NumBlocks=(BE2L(o->object.file.be_size)+globals->bytes_block-1) >> globals->shifts_block;
  }
  else {
    fib->fib_DirEntryType=ST_USERDIR;
    fib->fib_Size=0;
    fib->fib_NumBlocks=1;
  }
  fib->fib_Protection=BE2L(o->be_protection)^(FIBF_READ|FIBF_WRITE|FIBF_EXECUTE|FIBF_DELETE);
  fib->fib_EntryType=fib->fib_DirEntryType;
  fib->fib_DiskKey=BE2L(o->be_objectnode);
  fib->fib_OwnerUID=BE2W(o->be_owneruid);
  fib->fib_OwnerGID=BE2W(o->be_ownergid);
  datetodatestamp(BE2L(o->be_datemodified),&fib->fib_Date);

  src=o->name;
  dest = fib->fib_FileName;
  dest++;
  length=0;

  while(*src!=0) {
    *dest++=*src++;
    length++;
  }
  fib->fib_FileName[0]=length;

  src++;  /* comment follows name, so just skip the null-byte seperating them */

  dest = fib->fib_Comment;
  dest++;
  length=0;

  while(*src!=0) {
    *dest++=*src++;
    length++;
  }
  fib->fib_Comment[0]=length;
}



static struct DosPacket *getpacket(struct Process *p) {
  struct MsgPort *port=&p->pr_MsgPort;   /* get port of our process */
  struct Message *msg;

  if((msg=GetMsg(port))!=0) {
    return((struct DosPacket *)msg->mn_Node.ln_Name);
  }
  else {
    return(0);
  }
}



static struct DosPacket *waitpacket(struct Process *p) {
  struct MsgPort *port=&p->pr_MsgPort;   /* get port of our process */
  struct Message *msg;

  WaitPort(port);

  msg=GetMsg(port);

  return((struct DosPacket *)msg->mn_Node.ln_Name);
}



static void returnpacket(SIPTR res1,LONG res2) {
  struct Message *msg;
  struct MsgPort *replyport;

  globals->packet->dp_Res1=res1;  /* set return codes */
  globals->packet->dp_Res2=res2;

  replyport=globals->packet->dp_Port;  /* Get ReplyPort */

  msg=globals->packet->dp_Link;  /* Pointer to the Exec-Message of the packet */

  globals->packet->dp_Port=&globals->mytask->pr_MsgPort;  /* Setting Packet-Port back */

  msg->mn_Node.ln_Name=(char *)globals->packet;  /* Connect Message and Packet */
  msg->mn_Node.ln_Succ=NULL;
  msg->mn_Node.ln_Pred=NULL;

  PutMsg(replyport,msg);  /* Send the Message */
}



static void returnpacket2(struct DosPacket *packet, SIPTR res1, LONG res2)
{
  struct Message *msg;
  struct MsgPort *replyport;

  D(bug("[SFS] Replying, results are %ld/%ld\n", res1, res2));

  packet->dp_Res1=res1;  /* set return codes */
  packet->dp_Res2=res2;

  replyport=packet->dp_Port;  /* Get ReplyPort */

  msg=packet->dp_Link;  /* Pointer to the Exec-Message of the packet */

  packet->dp_Port=&globals->mytask->pr_MsgPort;  /* Setting Packet-Port back */

  msg->mn_Node.ln_Name=(char *)packet;  /* Connect Message and Packet */
  msg->mn_Node.ln_Succ=NULL;
  msg->mn_Node.ln_Pred=NULL;

  PutMsg(replyport,msg);  /* Send the Message */
}



void starttimeout() {
  /* From the AbortIO AutoDocs:
     iORequest - pointer to an I/O request block (must have been used
                 at least once.  May be active or finished).  */

  if(globals->pendingchanges==FALSE) {
    globals->inactivitytimer_ioreq->tr_time.tv_secs=globals->inactivity_timeout/2;
    globals->inactivitytimer_ioreq->tr_time.tv_micro=(globals->inactivity_timeout*500000)%1000000;
    globals->inactivitytimer_ioreq->tr_node.io_Command=TR_ADDREQUEST;

    SendIO(&globals->inactivitytimer_ioreq->tr_node);

    globals->pendingchanges=TRUE;
    globals->timerreset=FALSE;
  }
  else {
    globals->timerreset=TRUE;  /* Indicates that during the timeout there was another request. */
  }

  if(globals->activitytimeractive==FALSE) {
    globals->activitytimer_ioreq->tr_time.tv_secs=globals->activity_timeout;
    globals->activitytimer_ioreq->tr_time.tv_micro=0;
    globals->activitytimer_ioreq->tr_node.io_Command=TR_ADDREQUEST;

    SendIO(&globals->activitytimer_ioreq->tr_node);

    globals->activitytimeractive=TRUE;
  }
}



void stoptimeout(void) {

  if(globals->pendingchanges!=FALSE) {
    AbortIO(&globals->inactivitytimer_ioreq->tr_node);
    WaitIO(&globals->inactivitytimer_ioreq->tr_node);

    globals->pendingchanges=FALSE;
    globals->timerreset=FALSE;
  }

//  if(activitytimeractive!=FALSE) {
//    AbortIO(&activitytimer_ioreq->tr_node);
//    WaitIO(&activitytimer_ioreq->tr_node);
//
//    activitytimeractive=FALSE;
//  }
}



LONG flushcaches() {
  LONG errorcode;

  /* Flushes any pending changes to disk and ask the user what to do when
     an error occurs. */

  while((errorcode=flushtransaction())!=0 && req("Pending buffers couldn't be flushed\nto the disk because of error %ld.", "Retry|Cancel", errorcode)==1) {
  }

  motoroff();

  return(errorcode);
}



void invalidatecaches() {

  /* Invalidates all caches without flushing.  Call flushcaches()
     first. */

  invalidatecachebuffers();
  invalidateiocaches();
}


void dreqArgs(UBYTE *fmt, APTR params)
{
  APTR args[4];
  UBYTE *fmt2;

  if(globals->debugreqs!=FALSE) {
    args[0]=AROS_BSTR_ADDR(globals->devnode->dn_Name);
    args[1]=AROS_BSTR_ADDR(globals->startupmsg->fssm_Device);
    args[2]=(APTR)globals->startupmsg->fssm_Unit;
    args[3]=fmt;

    if((fmt2=AllocVec(strlen(fmt)+100,0))!=0) {
      _DEBUG(("\nREQUESTER\n\n"));
      RawDoFmt("SmartFilesystem %s: (%s, unit %ld)\n\n%s",args,putChProc,fmt2);

      if (requestArgs(PROGRAMNAME, fmt2, "Continue|No more requesters", params) == 0)
        globals->debugreqs=FALSE;

      FreeVec(fmt2);
    }
  }
}

LONG reqArgs(UBYTE *fmt, UBYTE *gads, APTR params)
{
  APTR args[5];
  APTR *arg=args;
  UBYTE *fmt2;
  LONG gadget=0;

  /* Simple requester function.  It will put up a requester
     in the form of:

     "Volume 'BOOT' (DH0: scsi.device, unit 0)"

     or:

     "Device DH0: (scsi.device, unit 0)"

     This depends on whether or not there is a valid
     VolumeNode. */

  if(globals->volumenode!=0) {
    *arg++=AROS_BSTR_ADDR(globals->volumenode->dl_Name);
  }

  *arg++=AROS_BSTR_ADDR(globals->devnode->dn_Name);
  *arg++=AROS_BSTR_ADDR(globals->startupmsg->fssm_Device);
  *arg++=(APTR)globals->startupmsg->fssm_Unit;
  *arg=fmt;

  if((fmt2=AllocVec(strlen(fmt)+100,0))!=0) {

    if(globals->volumenode!=0) {
      RawDoFmt("Volume '%s' (%s: %s, unit %ld)\n\n%s",args,putChProc,fmt2);
    }
    else {
      RawDoFmt("Device %s: (%s, unit %ld)\n\n%s",args,putChProc,fmt2);
    }

    gadget = requestArgs(PROGRAMNAME " request", fmt2, gads, params);
    FreeVec(fmt2);
  }

  return(gadget);
}

LONG req_unusualArgs(UBYTE *fmt, APTR params)
{
  APTR args[5];
  UBYTE *fmt2;
  LONG gadget=0;

  /* Simple requester function. */
  args[0]=AROS_BSTR_ADDR(globals->devnode->dn_Name);
  args[1]=AROS_BSTR_ADDR(globals->startupmsg->fssm_Device);
  args[2]=(APTR)globals->startupmsg->fssm_Unit;
  args[3]=fmt;
  args[4]="This is a safety check requester, which should\n"\
                 "never appear under normal conditions.  Please\n"\
                 "notify the author about the error above and if\n"\
                 "possible under what circumstances it appeared.\n\n"\
                 "BEWARE: SFS might crash if you click Continue.\n"\
                 "        Please save your work first!";

  if((fmt2=AllocVec(strlen(fmt)+400,0))!=0) {

    RawDoFmt("SmartFilesystem %s: (%s, unit %ld)\n\n%s\n\n%s",args,putChProc,fmt2);
    gadget = requestArgs(PROGRAMNAME " request", fmt2, "Continue", params);
    FreeVec(fmt2);
  }

  return(gadget);
}

void request2(UBYTE *text) {
  request(PROGRAMNAME, text, "Ok", 0);
}

LONG requestArgs(UBYTE *title, UBYTE *fmt, UBYTE *gads, APTR params)
{
  struct EasyStruct es;

  es.es_StructSize=sizeof(struct EasyStruct);
  es.es_Flags=0;
  es.es_Title=title;
  es.es_TextFormat=fmt;
  es.es_GadgetFormat=gads;

  return EasyRequestArgs(0, &es, 0, params);
}

void outputcachebuffer(struct CacheBuffer *cb) {
  ULONG *a;
  UWORD n;

  _DEBUG(("CacheBuffer at address 0x%08lx of block %ld (Locked = %ld, Bits = 0x%02lx)\n",cb,cb->blckno,(LONG)cb->locked,(LONG)cb->bits));

  a=cb->data;

  for(n=0; n<(globals->bytes_block>>5); n++) {
    _DEBUG(("%08lx %08lx %08lx %08lx %08lx %08lx %08lx %08lx\n",a[0],a[1],a[2],a[3],a[4],a[5],a[6],a[7]));
    a+=8;
  }
}




LONG readroots(void)
{
  struct CacheBuffer *cb1;
  struct CacheBuffer *cb2;
  struct fsRootBlock *rb1;
  struct fsRootBlock *rb2;
  WORD rb1okay=TRUE;
  WORD rb2okay=TRUE;
  UQUAD first, last;
  LONG errorcode;

  if((errorcode=readcachebuffer(&cb1,0))!=0) {
    return(errorcode);
  }

  lockcachebuffer(cb1);

  if((errorcode=readcachebuffer(&cb2,globals->blocks_total-1))!=0) {
    unlockcachebuffer(cb1);
    return(errorcode);
  }

  unlockcachebuffer(cb1);

  rb1=cb1->data;
  rb2=cb2->data;

  if(checkchecksum(cb1)==DOSFALSE || rb1->bheader.id!=L2BE(DOSTYPE_ID) || rb1->bheader.be_ownblock!=0) {
  _DEBUG(("cb1/rb1 not ok!\n"));
    rb1okay=FALSE;
  }
  
  _DEBUG(("checkchecksum(cb1)=%d, rb1->bheader.id=%08x (wanted %08x), rb1->bheader.ownblock=%d\n",
    checkchecksum(cb1),BE2L(rb1->bheader.id), DOSTYPE_ID, BE2L(rb1->bheader.be_ownblock)
    ));

  if(checkchecksum(cb2)==DOSFALSE || rb2->bheader.id!=L2BE(DOSTYPE_ID) || BE2L(rb2->bheader.be_ownblock)!=BE2L(rb2->be_totalblocks)-1) {
  _DEBUG(("cb2/rb2 not ok!\n"));
    rb2okay=FALSE;
  }

  _DEBUG(("checkchecksum(cb2)=%d, rb2->bheader.id=%08x, rb2->bheader.ownblock=%d, rb2->be_totalblocks =%d\n",
    checkchecksum(cb2),BE2L(rb2->bheader.id), BE2L(rb2->bheader.be_ownblock), BE2L(rb2->be_totalblocks)
    ));

  if(rb1okay!=FALSE && rb2okay!=FALSE) {
    /* Both root blocks look okay. */

    /*
    if(rb1->sequencenumber!=rb2->sequencenumber) {
      // Sequence numbers differ!
    }
    */

    /* Check sizes stored in rootblock */
    if ((rb1->be_blocksize != L2BE(globals->bytes_block)) || (rb1->be_totalblocks!=L2BE(globals->blocks_total)))
    {
      _DEBUG(("bad size in rb1!\n"));
      return(ERROR_NOT_A_DOS_DISK);
    }

    /*
     * Historically SFS rootblock holds absolute start and end positions on the disk in bytes.
     * They are used for validation and nothing else.
     * However, a situation is possible when for example someone takes an image of SFS partition
     * and then tries to mount it.
     * In order to make it working we compare lengths, not positions. If length is okay, the rootblock
     * is assumed to be okay.
     */
    first  = ((UQUAD)BE2L(rb1->be_firstbyteh) << 32) | BE2L(rb1->be_firstbyte);
    last   = ((UQUAD)BE2L(rb1->be_lastbyteh)  << 32) | BE2L(rb1->be_lastbyte);

    if (last - first != globals->byte_high - globals->byte_low)
    {
  	_DEBUG(("bad value in rb1!\n"));
      	return ERROR_NOT_A_DOS_DISK;
    }

    if(rb1->be_version!=BE2W(STRUCTURE_VERSION)) {
      /* Different version! */

      request(PROGRAMNAME " request","%s\n"\
                                     "is in a format unsupported by this version\n"\
                                     "of the filesystem.  Please reformat the disk or\n"\
                                     "install the correct version of this filesystem.",
                                     "Ok",AROS_BSTR_ADDR(globals->devnode->dn_Name));

      return(ERROR_NOT_A_DOS_DISK);
    }

    globals->block_bitmapbase=BE2L(rb1->be_bitmapbase);
    globals->block_adminspace=BE2L(rb1->be_adminspacecontainer);
    globals->block_root=BE2L(rb1->be_rootobjectcontainer);
    globals->block_extentbnoderoot=BE2L(rb1->be_extentbnoderoot);
    globals->block_objectnoderoot=BE2L(rb1->be_objectnoderoot);

    if((rb1->bits & ROOTBITS_CASESENSITIVE)!=0) {
      globals->is_casesensitive=TRUE;
    }
    else {
      globals->is_casesensitive=FALSE;
    }

    if((rb1->bits & ROOTBITS_RECYCLED)!=0) {
      globals->has_recycled=TRUE;
    }
    else {
      globals->has_recycled=FALSE;
    }
  }
  else {
    errorcode=ERROR_NOT_A_DOS_DISK;
  }

  return(errorcode);
}



struct DeviceList *usevolumenode(UBYTE *name, ULONG creationdate) {
  struct DosList *dol;
  struct DeviceList *vn=0;

  /* This function locates the specified volumenode, and if found
     uses it for the current volume inserted.  If the node is not
     found this function returns 0.  If the specified volumenode
     is found, but is found to be in use, then this function
     returns -1.  Otherwise the found volumenode is returned. */

  dol=LockDosList(LDF_READ|LDF_VOLUMES);

  while((dol=FindDosEntry(dol, name, LDF_VOLUMES))!=0) {
    if(datestamptodate(&dol->dol_misc.dol_volume.dol_VolumeDate)==creationdate) {                  // Do volumes have same creation date?
      Forbid();
      if(dol->dol_misc.dol_volume.dol_LockList!=0 || ((struct DeviceList *)dol)->dl_unused!=0) {   // Is volume not in use?
        struct NotifyRequest *nr;
        struct ExtFileLock *lock;

        /* Volume is not currently in use, so grab locklist & notifyrequests and patch fl_Task fields */

        _DEBUG(("usevolumenode: Found DosEntry with same date, and locklist!=0\n"));

        lock=(struct ExtFileLock *)BADDR(dol->dol_misc.dol_volume.dol_LockList);
        nr=(struct NotifyRequest *)BADDR((((struct DeviceList *)dol)->dl_unused));
        dol->dol_misc.dol_volume.dol_LockList=0;
        ((struct DeviceList *)dol)->dl_unused=0;

        Permit();

        globals->locklist=lock;
        globals->notifyrequests=nr;

        while(lock!=0) {
          lock->task=&globals->mytask->pr_MsgPort;
          lock=lock->next;
        }

        while(nr!=0) {
          nr->nr_Handler=&globals->mytask->pr_MsgPort;
          nr = (struct NotifyRequest *)nr->nr_Next;
        }

        vn=(struct DeviceList *)dol;
        break;
      }
      else {
        Permit();

        _DEBUG(("usevolumenode: Found DosEntry with same date, but it is in use!\n"));

        vn=(struct DeviceList *)-1;
      }
    }

    /* Volume nodes are of different date, continue search */
    dol=NextDosEntry(dol, LDF_VOLUMES);
  }

  UnLockDosList(LDF_READ|LDF_VOLUMES);

  return(vn);
}



LONG initdisk() {

  /* This routine is called whenever a disk has changed (via ACTION_INHIBIT or
     on startup).  The routine scans the disk and initializes the necessary
     variables.  Steps:

     - Check if the disk is a valid DOS disk
         If so, get name and datestamp of the disk
             If valid, look for volume node of the same name and datestamp
                If so, retrieve locklist from dol_LockList (fix fl_Task fields!)
                and use existing volume node.
                If not, a new volume node is created.
                If the disk carries the same attributes as another ACTIVE volume
                the handler ignores the newly inserted volume.
             If not valid, R/W error requester -> "NDOS"
         If not valid type is "NDOS"

  */

  changegeometry(globals->dosenvec);

  if(globals->volumenode==0) {
    struct CacheBuffer *cb;
    struct fsObjectContainer *oc=0;
    ULONG newdisktype;
    LONG errorcode;

    globals->diskstate=writeprotection();

    if((errorcode=readroots())==0) {

      /* Root blocks are valid for this filesystem */

      _DEBUG(("Initdisk: Root blocks read\n"));

      #ifdef STARTDEBUG
        dreq("Root blocks are okay!");
      #endif

      if((errorcode=checkfortransaction())==0) {

        _DEBUG(("Initdisk: Checked for an old Transaction and applied it if it was present.\n"));

        if((errorcode=readcachebuffercheck(&cb,globals->block_root,OBJECTCONTAINER_ID))==0) {
          struct fsRootInfo *ri=(struct fsRootInfo *)((UBYTE *)cb->data+globals->bytes_block-sizeof(struct fsRootInfo));
          ULONG blocksfree=0;

          lockcachebuffer(cb);
          oc=cb->data;

          _DEBUG(("Initdisk: '%s' was inserted\n",oc->object[0].name));

          /* ROOT block is valid for this filesystem */

          /* We should count the number of set bits in the bitmap now */

          {
            struct CacheBuffer *cb;
            struct fsBitmap *b;
            BLCK bitmapblock=globals->block_bitmapbase;
            UWORD cnt=globals->blocks_bitmap;
            WORD n;

            while(cnt-->0 && (errorcode=readcachebuffercheck(&cb,bitmapblock,BITMAP_ID))==0) {
              b=cb->data;

              for(n=0; n<((globals->bytes_block-sizeof(struct fsBitmap))>>2); n++) {
                if(b->bitmap[n]!=0) {
                  if(b->bitmap[n]==0xFFFFFFFF) {
                    blocksfree+=32;
                  }
                  else {
                    blocksfree+=bfcnto(b->bitmap[n]);
                  }
                }
              }
              bitmapblock++;
            }
          }

          unlockcachebuffer(cb);

          _DEBUG(("Initdisk: Traversed bitmap, found %ld free blocks\n",blocksfree));

          if(errorcode==0 && BE2L(ri->be_freeblocks)!=blocksfree) {
            if(ri->be_freeblocks!=0) {
              dreq("The number of free blocks (%ld) is incorrect.\n"\
                   "According to the bitmap it should be %ld.\n"\
                   "The number of free blocks will now be updated.", BE2L(ri->be_freeblocks), blocksfree);
            }

            newtransaction();

            preparecachebuffer(cb);

            ri->be_freeblocks=L2BE(blocksfree);

            if((errorcode=storecachebuffer(cb))==0) {
              endtransaction();
            }
            else {
              deletetransaction();
            }
          }

          if(errorcode==0) {
            _DEBUG(("Initdisk: A valid DOS disk\n"));

            newdisktype = DOSTYPE_ID;

            #ifdef STARTDEBUG
              dreq("There is a valid SFS disk present!");
            #endif
          }
          else {

            #ifdef STARTDEBUG
              dreq("SFS disk is invalid; bitmap error.");
            #endif

            newdisktype=ID_NOT_REALLY_DOS;
            errorcode=ERROR_NOT_A_DOS_DISK;
          }
        }
        else {

          #ifdef STARTDEBUG
            dreq("SFS disk is invalid; root objectcontainer error.");
          #endif

          newdisktype=ID_NOT_REALLY_DOS;
          errorcode=ERROR_NOT_A_DOS_DISK;
        }
      }
      else {

        #ifdef STARTDEBUG
          dreq("SFS disk is invalid; transaction error.");
        #endif

        newdisktype=ID_NOT_REALLY_DOS;
        errorcode=ERROR_NOT_A_DOS_DISK;
      }
    }
    else if(errorcode==INTERR_CHECKSUM_FAILURE || errorcode==ERROR_NOT_A_DOS_DISK) {
      newdisktype=ID_NOT_REALLY_DOS;
      errorcode=ERROR_NOT_A_DOS_DISK;

      #ifdef STARTDEBUG
        dreq("SFS disk is invalid; checksum failure.");
      #endif
    }
    else if(errorcode==INTERR_BLOCK_WRONG_TYPE) {
      #ifdef STARTDEBUG
        dreq("SFS disk is invalid; wrong block type.");
      #endif

      newdisktype=ID_NOT_REALLY_DOS;
      errorcode=ERROR_NOT_A_DOS_DISK;
    }
    else if(errorcode==TDERR_DiskChanged) {
      #ifdef STARTDEBUG
        dreq("SFS disk is invalid; disk was changed.");
      #endif

      newdisktype=ID_NO_DISK_PRESENT;
      errorcode=ERROR_NO_DISK;
    }
    else {
      #ifdef STARTDEBUG
        dreq("SFS disk is invalid; unreadable disk.");
      #endif

      newdisktype=ID_UNREADABLE_DISK;
      errorcode=ERROR_NOT_A_DOS_DISK;
    }

    if(errorcode==0) {
      struct DeviceList *vn;
      struct fsRootInfo *ri=(struct fsRootInfo *)((UBYTE *)oc+globals->bytes_block-sizeof(struct fsRootInfo));

      _DEBUG(("initdisk: Checking for an existing volume-node\n"));

      if((vn=usevolumenode(oc->object[0].name, BE2L(ri->be_datecreated)))!=(struct DeviceList *)-1) {
        if(vn==0) {
          /* VolumeNode was not found, so we need to create a new one. */

          _DEBUG(("initdisk: No volume-node found, creating new one instead.\n"));

          if((vn=(struct DeviceList *)MakeDosEntry("                              ",DLT_VOLUME))!=0) {
            struct SFSMessage *sfsm;
            UBYTE *d2=(UBYTE *)BADDR(vn->dl_Name);
#ifdef AROS_FAST_BSTR
            copystr(oc->object[0].name, d2, 30);
#else
            UBYTE *d=d2+1;
            UBYTE *s=oc->object[0].name;
            UBYTE len=0;

            while(*s!=0 && len<30) {
              *d++=*s++;
              len++;
            }

            *d=0;
            *d2=len;
#endif

            datetodatestamp(BE2L(ri->be_datecreated), &vn->dl_VolumeDate);

            _DEBUG(("initdisk: Sending msg.\n"));

            if((sfsm=AllocVec(sizeof(struct SFSMessage), MEMF_CLEAR))!=0) {
              sfsm->command=SFSM_ADD_VOLUMENODE;
              sfsm->data=(IPTR)vn;
              sfsm->msg.mn_Length=sizeof(struct SFSMessage);

              PutMsg(globals->sdlhport, (struct Message *)sfsm);
            }
          }
          else {
            errorcode=ERROR_NO_FREE_STORE;
          }
        }

        _DEBUG(("initdisk: Using new or old volumenode.\n"));

        if(errorcode==0) {    /* Reusing the found VolumeNode or using the new VolumeNode */
          vn->dl_Task=globals->devnode->dn_Task;
          vn->dl_DiskType=globals->dosenvec->de_DosType;
          globals->volumenode=vn;
        }
      }
      else { /* Volume is in use by another handler -- stay off */
        _DEBUG(("Initdisk: Found DosEntry with same date, and locklist==0\n"));
        newdisktype=ID_NO_DISK_PRESENT;             /* Hmmm... EXTREMELY unlikely, but may explain the strange bug Laire had. */
        errorcode=ERROR_NO_DISK;
     //   vn=0;
        globals->volumenode=0;
      }
    }

    if(errorcode==0) {
      diskchangenotify(IECLASS_DISKINSERTED);
    }

    globals->disktype=newdisktype;

    return(errorcode);
  }
  else {
    return(0);
  }
}



static struct DosList *attemptlockdoslist(LONG tries, LONG delay) {
  struct DosList *dol;
  struct DosPacket *dp;

  for(;;) {
    dol=AttemptLockDosList(LDF_WRITE|LDF_VOLUMES);

    if(((IPTR)dol & ~1)!=0) {
      return(dol);
    }

    if(--tries<=0) {
      return(0);
    }

    if((dp=getpacket(globals->mytask))!=0) {
      if(handlesimplepackets(dp)==0) {
        dumppackets(dp, ERROR_NOT_A_DOS_DISK);
      }
    }
    else {
      Delay(delay);
    }
  }
}


void removevolumenode(struct DosList *dol, struct DosList *vn) {
  while((dol=NextDosEntry(dol, LDF_VOLUMES))!=0) {
    if(dol==vn) {
      RemDosEntry(dol);
      break;
    }
  }
}


static void deinitdisk() {

  /* This function flushes all caches, and then invalidates them.
     If successful, this function then proceeds to either remove
     the volumenode, or transfer any outstanding locks/notifies to
     it.  Finally it notifies the system of the disk removal. */

  _DEBUG(("deinitdisk: entry\n"));

  flushcaches();
  invalidatecaches();

  if(globals->volumenode!=0) {

    /* We first check if the VolumeNode needs to be removed; if not
       then we do not lock the DosList and modify some of its fields.
       If it must be removed, we first attempt to lock the DosList
       synchronously; whether this fails or not, we always send a
       removal message to the DosList subtask. */

    if(globals->locklist!=0 || globals->notifyrequests!=0) {

      /* There are locks or notifyrequests, so we cannot kill the volumenode. */

      /* We haven't got the DosList locked here, but I think it is fairly
         safe to modify these fields directly. */

      Forbid();

      globals->volumenode->dl_Task=0;
      globals->volumenode->dl_LockList=MKBADDR(globals->locklist);
      globals->volumenode->dl_unused=MKBADDR(globals->notifyrequests);

      Permit();

      globals->locklist=0;
      globals->notifyrequests=0;
    }
    else {
      struct SFSMessage *sfsm;
      struct DosList *dol=attemptlockdoslist(5, 1);

      _DEBUG(("deinitdisk: dol = %ld\n", dol));

      if(dol!=0) {   /* Is DosList locked? */
        removevolumenode(dol, (struct DosList *)globals->volumenode);
        UnLockDosList(LDF_WRITE|LDF_VOLUMES);
      }

      _DEBUG(("deinitdisk: sending msg\n"));

      /* Even if we succesfully locked the DosList, we still should notify
         the DosList task to remove the node as well (and to free it), just
         in case a VolumeNode Add was still pending. */

      if((sfsm=AllocVec(sizeof(struct SFSMessage), MEMF_CLEAR))!=0) {
        sfsm->command=SFSM_REMOVE_VOLUMENODE;
        sfsm->data=(IPTR)globals->volumenode;
        sfsm->msg.mn_Length=sizeof(struct SFSMessage);

        PutMsg(globals->sdlhport, (struct Message *)sfsm);
      }
    }

    _DEBUG(("deinitdisk: done\n"));

    globals->volumenode=0;

    diskchangenotify(IECLASS_DISKREMOVED);
  }
}



LONG handlesimplepackets(struct DosPacket *packet) {
  LONG type=packet->dp_Type;  /* After returnpacket, packet->dp_Type is invalid! */

  switch(packet->dp_Type) {
  case ACTION_IS_FILESYSTEM:
    returnpacket2(packet, DOSTRUE,0);
    break;
#ifndef __AROS__
  case ACTION_GET_DISK_FSSM:
    returnpacket2(packet, (LONG)startupmsg,0);
    break;
  case ACTION_FREE_DISK_FSSM:
    returnpacket2(packet, DOSTRUE,0);
    break;
#endif
  case ACTION_DISK_INFO:
    actiondiskinfo(packet);
    break;
  case ACTION_SAME_LOCK:
    actionsamelock(packet);
    break;
  case ACTION_CURRENT_VOLUME:
    actioncurrentvolume(packet);
    break;
  default:
    return(0);
  }

  return(type);
}



static LONG dumppackets(struct DosPacket *packet,LONG returncode) {
  LONG type=packet->dp_Type;  /* After returnpacket, packet->dp_Type is invalid! */

  /* Routine which returns ERROR_NOT_A_DOS_DISK for all known
     packets which cannot be handled under such a situation */

  switch(type) {
  case ACTION_READ:
  case ACTION_WRITE:
  case ACTION_SEEK:
  case ACTION_SET_FILE_SIZE:
    returnpacket2(packet, -1,returncode);
    break;
  case ACTION_CREATE_DIR:
  case ACTION_FINDUPDATE:
  case ACTION_FINDINPUT:
  case ACTION_FINDOUTPUT:
  case ACTION_END:
  case ACTION_EXAMINE_OBJECT:
  case ACTION_EXAMINE_NEXT:
  case ACTION_COPY_DIR:
  case ACTION_LOCATE_OBJECT:
  case ACTION_PARENT:          /* till here verified to return 0 on error! */
  case ACTION_LOCK_RECORD:
  case ACTION_FREE_RECORD:
  case ACTION_FREE_LOCK:
  case ACTION_CHANGE_MODE:
  case ACTION_FH_FROM_LOCK:
  case ACTION_COPY_DIR_FH:
  case ACTION_PARENT_FH:
  case ACTION_EXAMINE_FH:
  case ACTION_EXAMINE_ALL:
  case ACTION_DELETE_OBJECT:
  case ACTION_RENAME_OBJECT:
  case ACTION_MAKE_LINK:
  case ACTION_READ_LINK:
  case ACTION_SET_COMMENT:
  case ACTION_SET_DATE:
  case ACTION_SET_PROTECT:
  case ACTION_INFO:
  case ACTION_RENAME_DISK:
  case ACTION_SERIALIZE_DISK:
#ifndef __AROS__
  case ACTION_GET_DISK_FSSM:
#endif
  case ACTION_MORE_CACHE:
  case ACTION_WRITE_PROTECT:
  case ACTION_ADD_NOTIFY:
  case ACTION_REMOVE_NOTIFY:
  case ACTION_INHIBIT:        /* These packets are only dumped when doslist is locked */
  case ACTION_FORMAT:
    returnpacket2(packet, DOSFALSE,returncode);
    break;
  default:
    returnpacket2(packet, DOSFALSE,ERROR_ACTION_NOT_KNOWN);
    break;
  }

  return(type);
}


#ifdef DEBUGCODE
static void dumppacket() {
  struct DosPacket *packet;

  /* routine which gets a packet and returns ERROR_NOT_A_DOS_DISK for all
     known packets which cannot be executed while we are attempting to access
     the doslist.  Some packets which don't require disk access will be
     handled as normal. */

  packet=waitpacket(globals->mytask);

  if(handlesimplepackets(packet)==0) {
    dumppackets(packet,ERROR_NOT_A_DOS_DISK);
  }
}
#endif


static void actioncurrentvolume(struct DosPacket *packet) {
  struct ExtFileLock *lock=(struct ExtFileLock *)packet->dp_Arg1;

  _DEBUG(("ACTION_CURRENT_VOLUME(%ld)\n",lock));

  if(lock==0) {
    _DEBUG(("ACTION_CURRENT_VOLUME: volumenode = %ld\n",globals->volumenode));
    returnpacket2(packet, (SIPTR)TOBADDR(globals->volumenode), 0);
  }
  else {
    returnpacket2(packet, (SIPTR)lock->volume,0);
  }
}



static void actionsamelock(struct DosPacket *packet) {
  struct ExtFileLock *lock;
  struct ExtFileLock *lock2;

  lock=(struct ExtFileLock *)BADDR(packet->dp_Arg1);
  lock2=(struct ExtFileLock *)BADDR(packet->dp_Arg2);

  if(lock->objectnode==lock2->objectnode && lock->task==lock2->task) {
    returnpacket2(packet, DOSTRUE,0);
    return;
  }
  returnpacket2(packet, DOSFALSE,0);
}



static void actiondiskinfo(struct DosPacket *packet) {
  struct InfoData *id=BADDR(packet->dp_Arg1);

  _DEBUG(("ACTION_DISK_INFO\n"));

  fillinfodata(id);

  returnpacket2(packet, DOSTRUE,0);
}



static void fillinfodata(struct InfoData *id) {
  ULONG usedblocks;

  id->id_NumSoftErrors=globals->numsofterrors;
  id->id_UnitNumber=globals->startupmsg->fssm_Unit;
  id->id_DiskState=globals->diskstate;
  id->id_NumBlocks=globals->blocks_total-globals->blocks_reserved_start-globals->blocks_reserved_end;

  usedblocks=id->id_NumBlocks;

  if(globals->disktype == DOSTYPE_ID) {
    ULONG deletedfiles, deletedblocks;

    getusedblocks(&usedblocks);
    if(getrecycledinfo(&deletedfiles, &deletedblocks)==0) {
      usedblocks-=deletedblocks;
    }
  }

  id->id_NumBlocksUsed=usedblocks;
  id->id_BytesPerBlock=globals->bytes_block;
  id->id_DiskType=globals->disktype;

  D(kprintf("Filling InfoData structure with a volumenode ptr to address %ld.  Disktype = 0x%08lx\n", globals->volumenode, globals->disktype));

  id->id_VolumeNode=TOBADDR(globals->volumenode);

  _DEBUG(("fillinfodata: volumenode = %ld, disktype = 0x%08lx\n", globals->volumenode, globals->disktype));

  if(globals->locklist!=0) {
    id->id_InUse=DOSTRUE;
  }
  else {
    id->id_InUse=DOSFALSE;
  }
}



BOOL checkchecksum(struct CacheBuffer *cb) {
#ifdef CHECKCHECKSUMSALWAYS
  if(CALCCHECKSUM(globals->bytes_block,cb->data)==0) {
#else
  if((cb->bits & CB_CHECKSUM)!=0 || CALCCHECKSUM(globals->bytes_block,cb->data)==0) {   //    -> copycachebuffer screws this up!
#endif
    cb->bits|=CB_CHECKSUM;
    return(DOSTRUE);
  }
  return(DOSFALSE);
}



void setchecksum(struct CacheBuffer *cb) {
  struct fsBlockHeader *bh=cb->data;

  bh->be_checksum=0;    /* Important! */
  bh->be_checksum=L2BE(-CALCCHECKSUM(globals->bytes_block,cb->data));
}



LONG readcachebuffercheck(struct CacheBuffer **returnedcb,ULONG blckno,ULONG type) {
  LONG errorcode;
  struct fsBlockHeader *bh;

  while((errorcode=readcachebuffer(returnedcb,blckno))==0) {
    bh=(*returnedcb)->data;
    if(type!=0 && bh->id!=type) {
      dumpcachebuffers();
      outputcachebuffer(*returnedcb);
      emptycachebuffer(*returnedcb);

      if(blckno!=0) {
        if(request(PROGRAMNAME " request","%s\n"\
                                          "has a blockid error in block %ld.\n"\
                                          "Expected was blockid 0x%08lx,\n"\
                                          "but the block says it is blockid 0x%08lx.",
                                          "Reread|Cancel",AROS_BSTR_ADDR(globals->devnode->dn_Name),blckno,type,bh->id)<=0) {
          return(INTERR_BLOCK_WRONG_TYPE);
        }
      }
      else {
/*        if(request(PROGRAMNAME " request","%s\n"\
                                          "is not a DOS disk.",
                                          "Retry|Cancel",AROS_BSTR_ADDR(devnode->dn_Name),blckno,type,bh->id)<=0) {
          return(INTERR_BLOCK_WRONG_TYPE);
        } */

        return(INTERR_BLOCK_WRONG_TYPE);
      }
      continue;
    }
    if(checkchecksum(*returnedcb)==DOSFALSE) {
      dumpcachebuffers();
      outputcachebuffer(*returnedcb);
      emptycachebuffer(*returnedcb);

      if(request(PROGRAMNAME " request","%s\n"\
                                        "has a checksum error in block %ld.",
                                        "Reread|Cancel",AROS_BSTR_ADDR(globals->devnode->dn_Name),blckno)<=0) {
        return(INTERR_CHECKSUM_FAILURE);
      }
      continue;
    }
    if(BE2L(bh->be_ownblock)!=blckno) {
      dumpcachebuffers();
      outputcachebuffer(*returnedcb);
      emptycachebuffer(*returnedcb);

      if(request(PROGRAMNAME " request","%s\n"\
                                        "has a block error in block %ld.\n"\
                                        "Expected was block %ld,\n"\
                                        "but the block says it is block %ld.",
                                        "Reread|Cancel",AROS_BSTR_ADDR(globals->devnode->dn_Name),blckno,blckno,BE2L(bh->be_ownblock))<=0) {
        return(INTERR_OWNBLOCK_WRONG);
      }
      continue;
    }
    break;
  }
  return(errorcode);
}



/* How the CacheBuffers work:
   ==========================

When the filesystem starts, the number of buffers indicated
by the Mountlist are allocated.  If the number of buffers
is below the minimum then the filesystem will increase the
number of buffers to the minimum.  AddBuffers can be used
later to change the number of CacheBuffers.

Each CacheBuffer is linked into two chains.  An LRU chain,
to quickly determine the least recently used CacheBuffer,
and a Hash chain.  The Hash chain is used to quickly locate
a CacheBuffer which contains a specific block.  Empty
cachebuffers are not linked in the Hash chain and have a
blckno of zero.

After having read a few blocks there comes a time when you
want to modify them.  The CacheBuffer systems helps to make
it simple to do safe updates to blocks without ever running
the risk of leaving the disk in a state where links or
pointers are not yet valid.  If you want to modify a block
or series of blocks you need to tell this to the caching
system by calling newtransaction().  An operation is a series
of modifications which, when written to disk completely,
would result in a valid disk.  By calling newtransaction() you
signal that you are about to start a series of modifications
which together will again result in a valid disk.

If however at any point during an operation somekind of
error occurs which prevents you from finishing the operation
you should call deletetransaction().  This will remove all
changes you made from the point when you called
newtransaction() from the cache.  These blocks will now never
get written to disk, so there is no chance that the disk
becomes invalid by partial modifications.

This sums up how the caching mechanism works.  The internal
workings of routines like newtransaction() and
deletetransaction() will be explained later on.  One very
important routine hasn't been mentioned here:
flushtransaction().  This is the routine which actually makes
modifications to the disk.  It does this by flushing one
operation at a time to disk (in old to new order of course)
in such a way that even if the machine crashed in the middle
of the flush that the disk would remain valid.  After
flushing all operations the cache is cleaned up.

*/




/* Creating and moving objects

   1. Changing the comment on an object
   2. Changing the name of an object
   3. Creating a new object
  (4. Moving an object)

Each of these three operations involves one or more of the following:

   1,2,3,4 - Locating enough new room for the object
   1,2,3,4 - Making sure the object-node points to the new block the
             object is located in.
   2 & 3   - Hashing the object
   3       - Allocating an ObjectNode.

Order:

'Allocating an ObjectNode' always precedes 'Hashing the Object' and 'Fixing the objectnode pointer'.
'FindObjectSpace' always precedes 'Fixing the objectnode pointer'.

 1. FindObjectSpace OR Allocating an ObjectNode
 2. Fixing the objectnode pointer OR Hashing the Object */



void fixlocks(struct GlobalHandle *gh,ULONG lastextentkey,ULONG lastextentoffset,ULONG filelength) {
  struct ExtFileLock *lock=globals->locklist;

  while(lock!=0) {
    if(lock->gh==gh) {
      if(filelength==0) {
        lock->offset=0;
        lock->extentoffset=0;
        lock->curextent=0;
      }
      else if(lock->offset>filelength) {
        /* Hmmm, this lock is positioned beyond the EOF... */

        if(lock->offset-lock->extentoffset >= filelength) {
          lock->extentoffset=filelength-(lock->offset-lock->extentoffset);
        }
        else {
          lock->extentoffset=filelength-lastextentoffset;
          lock->curextent=lastextentkey;
        }
        lock->offset=filelength;
      }
    }
    lock=lock->next;
  }
}



static LONG extendblocksinfile(struct ExtFileLock *lock, ULONG blocks) {
  BLCK lastextentbnode;
  LONG errorcode;

  /* This function extends the file identified by the lock with a number
     of blocks.  Only new Extents will be created -- the size of the file
     will not be altered, and changing it is left up to the caller.  If
     the file did not have any blocks yet, then lock->curextent will be
     set to the first (new) ExtentBNode.  It's left up up to the caller
     to reflect this change in object.file.data.

     This function must be called from within a transaction. */

  _XDEBUG((DEBUG_IO,"extendblocksinfile: Increasing number of blocks by %ld.  lock->curextent = %ld\n",blocks,lock->curextent));

  if((errorcode=seektocurrent(lock))==0) {

    if((lastextentbnode=lock->curextent)!=0) {
      struct CacheBuffer *cb;
      struct fsExtentBNode *ebn;

      while((errorcode=findextentbnode(lastextentbnode,&cb,&ebn))==0 && BE2L(ebn->be_next)!=0) {
        lastextentbnode=BE2L(ebn->be_next);
      }
    }

    if(errorcode==0) {
      struct GlobalHandle *gh=lock->gh;

      /* We found the last Extent.  Now we should see if we can extend it
         or if we need to add a new one and attach it to this one. */

      while(blocks!=0) {
        struct Space *sl=globals->spacelist;
        BLCK searchstart;

        if(lastextentbnode!=0) {
          struct CacheBuffer *cb;
          struct fsExtentBNode *ebn;

          if((errorcode=findextentbnode(lastextentbnode,&cb,&ebn))!=0) {
            break;
          }
          searchstart=BE2L(ebn->be_key)+BE2W(ebn->be_blocks);
        }
        else {
          searchstart=globals->block_rovingblockptr;
        }

        if((errorcode=smartfindandmarkspace(searchstart,blocks))!=0) {   /* only within transaction! */
          _DEBUG(("extendblocksinfile: sfams returned errorcode %ld\n", errorcode));
          break;
        }

        _XDEBUG((DEBUG_IO,"extendblocksinfile: Found some space!  blocks = %ld\n",blocks));

        while(blocks!=0 && sl->block!=0) {
          BLCK newspace=sl->block;
          ULONG newspaceblocks;

          newspaceblocks=sl->blocks > blocks ? blocks : sl->blocks;

          while(newspaceblocks!=0) {
            ULONG extentblocks;

            extentblocks=newspaceblocks > 8192 ? 8192 : newspaceblocks;

            newspaceblocks-=extentblocks;

            _XDEBUG((DEBUG_IO,"extendblocksinfile: sl->block = %ld, lastextentbnode = %ld, extentblocks = %ld\n",sl->block,lastextentbnode,extentblocks));

            if((errorcode=addblocks(extentblocks, newspace, gh->objectnode, &lastextentbnode))!=0) {
              _DEBUG(("extendblocksinfile: addblocks returned errorcode %ld\n", errorcode));
              return(errorcode);
            }

            if(lock->curextent==0) {
              lock->curextent=lastextentbnode;
            }

            lock->lastextendedblock=newspace + extentblocks;

            _XDEBUG((DEBUG_IO,"extendblocksinfile: Done adding or extending an extent -- blocks = %ld, extentblocks = %ld\n",blocks,extentblocks));

            blocks-=extentblocks;
            newspace+=extentblocks;
          }
          sl++;
        }
      }
    }
  }

  return(errorcode);
}



LONG truncateblocksinfile(struct ExtFileLock *lock,ULONG blocks,ULONG *lastextentkey,ULONG *lastextentoffset) {
  struct CacheBuffer *cb;
  struct fsExtentBNode *ebn;
  ULONG offset;
  ULONG extentoffset;
  LONG errorcode;

  /* Truncates the specified file to /blocks/ blocks.  This function does not
     take care of setting the filesize.  It also doesn't check for any locks
     which have a fileptr which is beyond the end of the file after calling
     this function.  Its sole purpose is to reduce the number of blocks in
     the Extent list. */

  offset=blocks<<globals->shifts_block;

  _DEBUG(("truncateblocksinfile: truncating file by %ld blocks\n",blocks));

  if((errorcode=seekextent(lock,offset,&cb,&ebn,&extentoffset))==0) {

    _DEBUG(("truncateblocksinfile: offset = %ld, extentoffset = %ld\n",offset,extentoffset));

    if(offset-extentoffset==0) {
      ULONG prevkey=BE2L(ebn->be_prev);

      _DEBUG(("truncateblocksinfile: prevkey = %ld\n",prevkey));

      /* Darn.  This Extent needs to be killed completely, meaning that we
         have to set the Next pointer of the previous Extent to zero. */

      deleteextents(BE2L(ebn->be_key));

      if((prevkey & 0x80000000)==0) {
        if((errorcode=findextentbnode(prevkey,&cb,&ebn))==0) {
          preparecachebuffer(cb);

          ebn->be_next=0;

          errorcode=storecachebuffer(cb);

          *lastextentkey=prevkey;
          *lastextentoffset=extentoffset-(BE2W(ebn->be_blocks)<<globals->shifts_block);
        }
      }
      else {
        struct fsObject *o;

        if((errorcode=readobject((prevkey & 0x7fffffff), &cb, &o))==0) {
          preparecachebuffer(cb);

          _DEBUG(("truncateblocksinfile: cb->blckno = %ld\n",cb->blckno));

          o->object.file.be_data=0;
          lock->gh->data=0;

          errorcode=storecachebuffer(cb);

          *lastextentkey=0;
          *lastextentoffset=0;
        }
      }
    }
    else {
      ULONG newblocks=blocks-(extentoffset>>globals->shifts_block);

      *lastextentkey=BE2L(ebn->be_key);
      *lastextentoffset=extentoffset;

      _DEBUG(("truncateblocksinfile: newblocks = %ld, ebn->blocks = %ld\n",newblocks,BE2W(ebn->be_blocks)));

      if(newblocks!=BE2W(ebn->be_blocks)) {
        /* There is only one case where newblocks could equal en->blocks here,
           and that is if we tried to truncate the file to the same number of
           blocks the file already had! */

        lockcachebuffer(cb);

        if((errorcode=freespace(BE2L(ebn->be_key)+newblocks,BE2W(ebn->be_blocks)-newblocks))==0) {
          BLCK next=BE2L(ebn->be_next);

          preparecachebuffer(cb);

          ebn->be_blocks=W2BE(newblocks);
          ebn->be_next=0;

          if((errorcode=storecachebuffer(cb))==0) {
            errorcode=deleteextents(next);   // Careful, deleting BNode's may change the location of other BNode's as well!
          }
        }

        unlockcachebuffer(cb);
      }
    }
  }

  return(errorcode);
}



LONG setfilesize(struct ExtFileLock *lock, ULONG bytes) {
  struct GlobalHandle *gh=lock->gh;
  LONG errorcode=0;

  /* This function sets the filesize of a file to /bytes/ bytes.  If the
     file is not yet long enough, additional blocks are allocated and
     added to the file.  If the file is too long the file will be shortened
     and the lost blocks are freed.

     Any filehandles using this file are set to the EOF if their current
     position would be beyond the new EOF when truncating the file.

     In any other case the position of the file ptr is not altered. */

  _DEBUG(("setfilesize: gh = 0x%08lx.  Setting to %ld bytes. Current size is %ld bytes.\n",gh,bytes,gh->size));

  if(bytes!=gh->size) {
    LONG blocksdiff;
    LONG curblocks;

    curblocks=(gh->size+globals->bytes_block-1)>>globals->shifts_block;
    blocksdiff=((bytes+globals->bytes_block-1)>>globals->shifts_block) - curblocks;

    _DEBUG(("setfilesize: blocksdiff = %ld\n",blocksdiff));

    if(blocksdiff>0) {

      newtransaction();

      if((errorcode=extendblocksinfile(lock, blocksdiff))==0) {
        struct CacheBuffer *cb;
        struct fsObject *o;

        /* File (now) has right amount of blocks, only exact size in bytes may differ */

        if((errorcode=readobject(gh->objectnode, &cb, &o))==0) {
          preparecachebuffer(cb);

          o->object.file.be_size=L2BE(bytes);
          if(o->object.file.be_data==0) {
            o->object.file.be_data=L2BE(lock->curextent);
          }

          errorcode=storecachebuffer(cb);
        }
      }

      if(errorcode==0) {
        endtransaction();
        gh->size=bytes;
        if(gh->data==0) {
          gh->data=lock->curextent;
        }
      }
      else {
        deletetransaction();
      }
    }
    else {
      ULONG lastextentkey=0;
      ULONG lastextentoffset=0;

      newtransaction();

      if(blocksdiff<0) {
        /* File needs to be shortened by -/blocksdiff/ blocks. */

        _DEBUG(("setfilesize: Decreasing number of blocks\n"));

        errorcode=truncateblocksinfile(lock,curblocks+blocksdiff,&lastextentkey,&lastextentoffset);
      }

      _DEBUG(("setfilesize: lastextentkey = %ld, lastextentoffset = %ld\n",lastextentkey,lastextentoffset));

      if(errorcode==0) {
        struct CacheBuffer *cb;
        struct fsObject *o;

        /* File (now) has right amount of blocks, only exact size in bytes may differ */

        if((errorcode=readobject(gh->objectnode,&cb,&o))==0) {
          preparecachebuffer(cb);

          _DEBUG(("setfilesize: gh->objectnode = %ld, cb->blcno = %ld\n",gh->objectnode,cb->blckno));

          o->object.file.be_size=L2BE(bytes);

          errorcode=storecachebuffer(cb);
        }
      }

      if(errorcode==0) {
        endtransaction();
        gh->size=bytes;

        fixlocks(gh,lastextentkey,lastextentoffset,bytes);
      }
      else {
        deletetransaction();
      }
    }

    if(errorcode==0) {
      lock->bits|=EFL_MODIFIED;
    }
  }

  return(errorcode);
}



void seekforward(struct ExtFileLock *lock, UWORD ebn_blocks, BLCK ebn_next, ULONG bytestoseek) {

  /* This function does a simple forward seek.  It assumes /bytestoseek/ is
     only large enough to skip within the current extent or to the very
     beginning of the next extent. */

  lock->offset+=bytestoseek;
  lock->extentoffset+=bytestoseek;
  if(lock->extentoffset >= ebn_blocks<<globals->shifts_block && ebn_next!=0) {
    lock->extentoffset=0;
    lock->curextent=ebn_next;
  }
}



LONG seektocurrent(struct ExtFileLock *lock) {
  LONG errorcode=0;

  /* This function checks if the currentextent is still valid.  If not,
     it will attempt to locate the currentextent. */

  if(lock->curextent==0) {
  
    /* lock->curextent==0 can indicate 2 things:

       - A previously empty file was extended by another handle; in this
         case the file-ptr (lock->offset) of this handle is still zero.
  
       - The extent has been moved by the defragmenter, and will have to
         be re-located.  In this case the file-ptr doesn't have to be
         zero. */
  
    if(lock->offset==0) {
      lock->curextent=lock->gh->data;
    }
    else {
      struct CacheBuffer *cb;
      struct fsExtentBNode *ebn;
      ULONG extentoffset;

      if((errorcode=seekextent(lock, lock->offset, &cb, &ebn, &extentoffset))==0) {
        lock->curextent=BE2L(ebn->be_key);
        lock->extentoffset=lock->offset - extentoffset;
      }
    }
  }
  
  return(errorcode);
}
  
  
static LONG seek(struct ExtFileLock *lock,ULONG offset) {
  struct CacheBuffer *cb;
  struct fsExtentBNode *ebn;
  ULONG extentoffset;
  LONG errorcode;
  
  /* This function moves the file-ptr to the specified absolute file
     position.  It will return an error if you try to seek beyond the
     end of the file. */
  
  _XDEBUG((DEBUG_SEEK,"seek: Attempting to seek to %ld\n",offset));
  
  if((errorcode=seekextent(lock,offset,&cb,&ebn,&extentoffset))==0) {
    lock->curextent=BE2L(ebn->be_key);
    lock->extentoffset=offset-extentoffset;
    lock->offset=offset;
  
    _XDEBUG((DEBUG_SEEK,"seek: lock->curextent = %ld, lock->extentoffset = %ld, lock->offset = %ld\n",lock->curextent,lock->extentoffset,lock->offset));
  }

  _XDEBUG((DEBUG_SEEK,"seek: Exiting with errorcode %ld\n",errorcode));

  return(errorcode);
}
  
  
  
LONG deleteextents(ULONG key) {
  struct CacheBuffer *cb;
  struct fsExtentBNode *ebn;
  LONG errorcode=0;
  
  /* Deletes an fsExtentBNode structure by key and any fsExtentBNodes linked to it.
     This function DOES NOT fix the next pointer in a possible fsExtentBNode which
     might have been pointing to the first BNode we are deleting.  Make sure you check
     this yourself, if needed.

     If key is zero, than this function does nothing.
  
     newtransaction() should have been called prior to calling this function. */
  
  _XDEBUG((DEBUG_NODES,"deleteextents: Entry -- deleting extents from key %ld\n",key));
  
  
  while(key!=0 && (errorcode=findbnode(globals->block_extentbnoderoot,key,&cb,(struct BNode **)&ebn))==0) {
    /* node to be deleted located. */
  
    // _XDEBUG((DDEBUG_NODES,"deleteextents: Now deleting key %ld.  Next key is %ld\n",key,ebn->next));
  
    key=BE2L(ebn->be_next);
  
    lockcachebuffer(cb);     /* Makes sure freespace() doesn't reuse this cachebuffer */

    if((errorcode=freespace(BE2L(ebn->be_key), BE2W(ebn->be_blocks)))==0) {
      unlockcachebuffer(cb);

      // _XDEBUG((DDEBUG_NODES,"deleteextents: deletebnode from root %ld, with key %ld\n",block_extentbnoderoot,ebn->key));
  
      if((errorcode=deletebnode(globals->block_extentbnoderoot,BE2L(ebn->be_key)))!=0) {   /*** Maybe use deleteinternalnode here??? */
        break;
      }
    }
    else {
      unlockcachebuffer(cb);
      break;
    }
  }
  
  // _XDEBUG((DEBUG_NODES,"deleteextents: Exiting with errorcode %ld\n",errorcode));
  
  return(errorcode);
}
  
  
  
static LONG findextentbnode(ULONG key,struct CacheBuffer **returned_cb,struct fsExtentBNode **returned_bnode) {
  LONG errorcode;
  
  errorcode=findbnode(globals->block_extentbnoderoot,key,returned_cb,(struct BNode **)returned_bnode);
  
  #ifdef CHECKCODE_BNODES
    if(*returned_bnode==0 || BE2L((*returned_bnode)->be_key)!=key) {
      dreq("findextentbnode: findbnode() can't find key %ld!",key);
      outputcachebuffer(*returned_cb);
      return(INTERR_BTREE);
    }
  #endif
  
  return(errorcode);
}
  

/*
LONG findobjectnode(NODE nodeno,struct CacheBuffer **returned_cb,struct fsObjectNode **returned_node) {
  return(findnode(block_objectnoderoot, sizeof(struct fsObjectNode), nodeno, returned_cb, (struct fsNode **)returned_node));
}
*/

  
static inline LONG createextentbnode(ULONG key,struct CacheBuffer **returned_cb,struct fsExtentBNode **returned_bnode) {
  return(createbnode(globals->block_extentbnoderoot,key,returned_cb,(struct BNode **)returned_bnode));
}

  
  
LONG seekextent(struct ExtFileLock *lock, ULONG offset, struct CacheBuffer **returned_cb, struct fsExtentBNode **returned_ebn, ULONG *returned_extentoffset) {
  BLCK extentbnode;
  LONG errorcode=0;

  /* When seeking there are 2 options; we start from the current extent,
     or we start from the first extent.  Below we determine the best
     startpoint: */
  
  if(offset>lock->gh->size) {
    _XDEBUG((DEBUG_SEEK,"seekextent: Attempting to seek beyond file\n"));
  
    return(ERROR_SEEK_ERROR);
  }

//  _DEBUG(("seekextent: offset = %ld, lock->offset = %ld, lock->extentoffset = %ld, lock->curextent = %ld\n",offset, lock->offset, lock->extentoffset, lock->curextent));

  if(lock->curextent!=0 && offset >= lock->offset - lock->extentoffset) {
    extentbnode=lock->curextent;
    *returned_extentoffset=lock->offset - lock->extentoffset;
  }
  else {
    extentbnode=lock->gh->data;
    *returned_extentoffset=0;
  }

  /* Starting point has been determined.  Let the seeking begin!
     We keep getting the next extent, until we find the extent which
     contains the required offset. */

  if(extentbnode!=0) {
    while((errorcode=findextentbnode(extentbnode,returned_cb,returned_ebn))==0) {
      ULONG endbyte=*returned_extentoffset+(BE2W((*returned_ebn)->be_blocks)<<globals->shifts_block);

      if(offset>=*returned_extentoffset && offset<endbyte) {
        /* Hooray!  We found the correct extent. */
        break;
      }

      if(BE2L((*returned_ebn)->be_next)==0) {
        /* This break is here in case we run into the end of the file.
           This prevents *returned_extentoffset and extentbnode
           from being destroyed in the lines below, since it is still
           valid to seek to the EOF */

        if(offset>endbyte) {
          /* There where no more blocks, but there should have been... */
          errorcode=ERROR_SEEK_ERROR;
        }
        break;
      }

      *returned_extentoffset+=BE2W((*returned_ebn)->be_blocks)<<globals->shifts_block;
      extentbnode=BE2L((*returned_ebn)->be_next);
    }
  }

  return(errorcode);
}



/* Notify support functions. */

UBYTE *fullpath(struct CacheBuffer *cbstart,struct fsObject *o) {
  struct fsObjectContainer *oc=cbstart->data;
  struct CacheBuffer *cb=cbstart;
  UBYTE *path=&globals->pathstring[519];
  UBYTE *name;

  /* Returns the full path of an object, or 0 if it fails. */

  lockcachebuffer(cbstart);

  *path=0;

  while(oc->be_parent!=0) {     /* Checking parent here means name in ROOT will be ignored. */
    name=o->name;
    while(*++name!=0) {
    }

    while(name!=o->name) {
      *--path=upperchar(*--name);
    }

    if(readobject(BE2L(oc->be_parent),&cb,&o)!=0) {
      path=0;
      break;
    }

    oc=cb->data;

    if(oc->be_parent!=0) {
      *--path='/';
    }
  }

  unlockcachebuffer(cbstart);

  return(path);
}



void checknotifyforpath(UBYTE *path,UBYTE notifyparent) {
  struct NotifyRequest *nr;
  UBYTE *s1,*s2;
  UBYTE *lastslash;

  /* /path/ doesn't have a trailing slash and start with a root directory (no colon) */

  s1=path;
  lastslash=path-1;
  while(*s1!=0) {
    if(*s1=='/') {
      lastslash=s1;
    }
    s1++;
  }

  nr=globals->notifyrequests;

  // _DEBUG(("checknotify: path = '%s'\n", path));

  while(nr!=0) {

    s1=path;
    s2=stripcolon(nr->nr_FullName);

    while(*s1!=0 && *s2!=0 && *s1==upperchar(*s2)) {
      s1++;  // If last character doesn't match, this increment won't take place.
      s2++;
    }

    /* "" == "hallo"  -> no match
       "" == "/"      -> match
       "" == ""       -> match
       "/shit" == ""  -> match (parent dir)
       "shit" == ""   -> match (parent dir) */

    // _DEBUG(("checknotify: fullpath = '%s', nr->FullName = '%s'\n",s1,s2));

    if( (s1[0]==0 && (s2[0]==0 || (s2[0]=='/' && s2[1]==0))) || ((s2[0]==0 && (s1==lastslash || s1==lastslash+1)) && notifyparent==TRUE) ) {
      /* Wow, the string in the NotifyRequest matches!  We need to notify someone! */

      _DEBUG(("checknotify: Notificating!! nr->FullName = %s, UserData = 0x%08lx\n",nr->nr_FullName, nr->nr_UserData));

      notify(nr);

      /* No else, if neither flag is set then do nothing. */
    }

    nr = (struct NotifyRequest *)nr->nr_Next;
  }
}



void checknotifyforobject(struct CacheBuffer *cb,struct fsObject *o,UBYTE notifyparent) {
  checknotifyforpath(fullpath(cb,o),notifyparent);
}



void notify(struct NotifyRequest *nr) {

  /* This function sends a Notify to the client indicated by the passed
     in notifyrequest structure. */

  if((nr->nr_Flags & NRF_SEND_SIGNAL)!=0) {
    /* Sending them a signal. */

    _DEBUG(("notify: Sending signal\n"));

    Signal(nr->nr_stuff.nr_Signal.nr_Task,1<<nr->nr_stuff.nr_Signal.nr_SignalNum);
  }
  else if((nr->nr_Flags & NRF_SEND_MESSAGE)!=0) {
    struct NotifyMessage *nm;

    /* Sending them a message. */

    _DEBUG(("notify: Sending message\n"));

    if((nm=AllocMem(sizeof(struct NotifyMessage),MEMF_CLEAR))!=0) {
      nm->nm_ExecMessage.mn_ReplyPort=globals->msgportnotify;
      nm->nm_ExecMessage.mn_Length=sizeof(struct NotifyMessage);

      nm->nm_Class=NOTIFY_CLASS;
      nm->nm_Code=NOTIFY_CODE;
      nm->nm_NReq=(struct NotifyRequest *)nr;

      _DEBUG(("notify: PutMsg() - UserData = 0x%08lx\n", nr->nr_UserData));

      PutMsg(nr->nr_stuff.nr_Msg.nr_Port,(struct Message *)nm);
    }
  }
}






#if 0
LONG writedata(ULONG newbytes, ULONG extentblocks, BLCK newspace, UBYTE *data) {
  ULONG blocks=newbytes>>shifts_block;
  LONG errorcode=0;

  if(blocks>extentblocks) {
    blocks=extentblocks;
  }

  if(blocks!=0 && (errorcode=write(newspace,data,blocks))!=0) {
    return(errorcode);
  }

  if(blocks<extentblocks) {
    struct CacheBuffer *cb;

    _XDEBUG((DEBUG_IO,"  writedata: blocks = %ld, newbytes = %ld\n",blocks,newbytes));

    newbytes-=blocks<<shifts_block;
    data+=blocks<<shifts_block;

    if((cb=newcachebuffer(newspace+blocks))!=0) {
      unlockcachebuffer(cb);

      CopyMem(data,cb->data,newbytes);

      errorcode=writecachebuffer(cb);
      /* At this point we can do 2 things with this cachebuffer.  We can leave it
         hashed so if the user requests part of this block again it can quickly be grabbed
         from this CacheBuffer.  The other option is the call emptycachebuffer()
         and let this buffer be reused ASAP. */

      emptycachebuffer(cb);
    }
    else {
      errorcode=ERROR_NO_FREE_STORE;
    }
  }

  return(errorcode);
}
#endif








static LONG addblocks(UWORD blocks, BLCK newspace, NODE objectnode, BLCK *io_lastextentbnode) {
  struct CacheBuffer *cb;
  struct fsExtentBNode *ebn;
  LONG errorcode;

  /* This function adds /blocks/ blocks starting at block /newspace/ to a file
     identified by /objectnode/ and /lastextentbnode/.  /io_lastextentbnode/ can
     be zero if there is no ExtentBNode chain attached to this file yet.

     This function must be called from within a transaction.

     /blocks/ ranges from 1 to 8192.  To be able to extend Extents which are
     almost full, it is wise to make this value no higher than 8192 blocks.

     /io_lastextentbnode/ will contain the new lastextentbnode value when this
     function completes.

     This function makes no attempt to update any locks associated with this file,
     nor does it update the filesize information in a possible globalhandle.

     If there was no chain yet, then this function will create a new one.  However
     it will NOT update object.file.data -- this is left up to the caller. */

  if(*io_lastextentbnode!=0) {
    /* There was already a ExtentBNode chain for this file.  Extending it. */

    _XDEBUG((DEBUG_IO,"  addblocks: Extending existing ExtentBNode chain.\n"));

    if((errorcode=findextentbnode(*io_lastextentbnode,&cb,&ebn))==0) {

      preparecachebuffer(cb);

      if(BE2L(ebn->be_key)+BE2W(ebn->be_blocks)==newspace && BE2W(ebn->be_blocks)+blocks<65536) {
        /* It is possible to extent the last ExtentBNode! */

        _XDEBUG((DEBUG_IO,"  addblocks: Extending last ExtentBNode.\n"));

        ebn->be_blocks=W2BE(BE2W(ebn->be_blocks)+blocks);

        errorcode=storecachebuffer(cb);
      }
      else {
        /* It isn't possible to extent the last ExtentBNode so we create
           a new one and link it to the last ExtentBNode. */

        ebn->be_next=L2BE(newspace);

        if((errorcode=storecachebuffer(cb))==0 && (errorcode=createextentbnode(newspace,&cb,&ebn))==0) {

          _XDEBUG((DEBUG_IO,"  addblocks: Created new ExtentBNode.\n"));

          ebn->be_key=L2BE(newspace);
          ebn->be_prev=L2BE(*io_lastextentbnode);
          ebn->be_next=0;
          ebn->be_blocks=W2BE(blocks);

          *io_lastextentbnode=newspace;

          if((errorcode=storecachebuffer(cb))==0) {
            globals->block_rovingblockptr=newspace+blocks;
            if((blocks<<globals->shifts_block) <= ROVING_SMALL_WRITE) {
              globals->block_rovingblockptr+=ROVING_RESERVED_SPACE>>globals->shifts_block;
            }

            if(globals->block_rovingblockptr>=globals->blocks_total) {
              globals->block_rovingblockptr=0;
            }
          }
        }
      }
    }
  }
  else {
    /* There is no ExtentBNode chain yet for this file.  Attaching one! */

    if((errorcode=createextentbnode(newspace,&cb,&ebn))==0) {

      _XDEBUG((DEBUG_IO,"  addblocks: Created new ExtentBNode chain.\n"));

      ebn->be_key=L2BE(newspace);
      ebn->be_prev=L2BE(objectnode+0x80000000);
      ebn->be_next=0;
      ebn->be_blocks=W2BE(blocks);

      *io_lastextentbnode=newspace;

      if((errorcode=storecachebuffer(cb))==0) {
        globals->block_rovingblockptr=newspace+blocks;
        if((blocks<<globals->shifts_block) <= ROVING_SMALL_WRITE) {
          globals->block_rovingblockptr+=ROVING_RESERVED_SPACE>>globals->shifts_block;
        }

        if(globals->block_rovingblockptr>=globals->blocks_total) {
          globals->block_rovingblockptr=0;
        }
      }
    }
  }

  return(errorcode);
}



static void diskchangenotify(ULONG class) {
  struct IOStdReq *inputreq;
  struct MsgPort *inputport;
  struct InputEvent ie;
  struct timeval tv;

  if((inputport=CreateMsgPort())!=0) {
    if((inputreq=(struct IOStdReq *)CreateIORequest(inputport,sizeof(struct IOStdReq)))!=0) {
      if(OpenDevice("input.device",0,(struct IORequest *)inputreq,0)==0) {

        GetSysTime(&tv);

        ie.ie_NextEvent=0;
        ie.ie_Class=class;
        ie.ie_SubClass=0;
        ie.ie_Code=0;
        ie.ie_Qualifier=IEQUALIFIER_MULTIBROADCAST;
        ie.ie_EventAddress=0;
        ie.ie_TimeStamp=tv;

        inputreq->io_Command = IND_WRITEEVENT;
        inputreq->io_Length  = sizeof(struct InputEvent);
        inputreq->io_Data    = &ie;

        DoIO((struct IORequest *)inputreq);

        CloseDevice((struct IORequest *)inputreq);
      }
      DeleteIORequest(inputreq);
    }
    DeleteMsgPort(inputport);
  }
}



#if 0

 () {
  struct ExtFileLock *lock;

  lock=locklist;

  while(lock!=0) {
    if((lock->bits & EFL_MODIFIED)!=0) {
      /* File belonging to this lock was modified. */

      if(lock->gh!=0 && lock->curextent!=0) {
        BLCK lastblock=lock->curextent;

        lastblock+=lock->extentoffset>>shifts_block;

        /* lastblock is now possibly the last block which is used by the file.  This
           should always be true for newly created files which are being extended. */


      }
    }

    lock=lock->next;
  }
}

#endif


BOOL freeupspace(void) {
  BOOL spacefreed=FALSE;

  /* This function tries to free up space.  It does this by
     permanently deleting deleted files (if case of a recycled)
     and by flushing the current transaction if there is one.

     If no space could be freed, then this function returns
     FALSE.  This is the go-ahead sign to report disk full
     to the user. */

  if(hastransaction()) {
    flushtransaction();
    spacefreed=TRUE;
  }

  if(cleanupdeletedfiles()!=FALSE) {
    spacefreed=TRUE;
  }

  return(spacefreed);
}



LONG writetofile(struct ExtFileLock *lock, UBYTE *buffer, ULONG bytestowrite) {
  struct GlobalHandle *gh=lock->gh;
  LONG errorcode;

  /* This function must be called from within a transaction! */

  if((gh->protection & FIBF_WRITE)!=0) {
    ULONG maxbytes;
    LONG newbytes;

    /* First thing we need to do is extend the file (if needed) to
       accomodate for all the data we are about to write. */

    maxbytes=(gh->size + globals->bytes_block-1) & ~globals->mask_block;  // Maximum number of bytes file can hold with the current amount of blocks.
    newbytes=bytestowrite-(maxbytes-lock->offset);      // Number of new bytes which would end up in newly allocated blocks.

    if((errorcode=seektocurrent(lock))==0 && (newbytes<=0 || (errorcode=extendblocksinfile(lock, (newbytes+globals->bytes_block-1)>>globals->shifts_block))==0)) {
      struct CacheBuffer *extent_cb;
      struct fsExtentBNode *ebn=0;
      ULONG newfilesize;

      /* At this point, the file either didn't need extending, or was succesfully extended. */

      newfilesize=lock->offset+bytestowrite;

      if(newfilesize<gh->size) {
        newfilesize=gh->size;
      }

      /* If the filesize will change, then we set it below */

      if(newfilesize!=gh->size) {
        struct CacheBuffer *cb;
        struct fsObject *o;

        if((errorcode=readobject(lock->objectnode,&cb,&o))==0) {
          preparecachebuffer(cb);

          checksum_writelong_be(cb->data, &o->object.file.be_size, newfilesize);
          gh->size=newfilesize;

          if(o->object.file.be_data==0) {
            checksum_writelong_be(cb->data, &o->object.file.be_data, lock->curextent);
            gh->data=lock->curextent;
          }

          errorcode=storecachebuffer_nochecksum(cb);
        }
      }

      if(errorcode==0) {
        while(bytestowrite!=0 && (errorcode=findextentbnode(lock->curextent, &extent_cb, &ebn))==0) {
          ULONG bytes;
          ULONG offsetinblock=lock->extentoffset & globals->mask_block;
          BLCK ebn_next=BE2L(ebn->be_next);
          UWORD ebn_blocks=BE2W(ebn->be_blocks);

          if(BE2W(ebn->be_blocks)==lock->extentoffset>>globals->shifts_block) {
            /* We are at the end +1 of this extent.  Skip to next one. */

            lock->curextent=BE2L(ebn->be_next);
            lock->extentoffset=0;
            continue;
          }

          if(offsetinblock!=0 || bytestowrite<globals->bytes_block) {

            /** Partial writes to the last block of the file will cause a
                read of that block, even if this block didn't yet contain
                any valid data, because the file was just extended.  This
                is unneeded... */

            /* File-ptr is located somewhere in the middle of a block, or at the
               start of the block but not at the end of the file.  To add data
               to it we'll first need to read this block. */

            _XDEBUG((DEBUG_IO,"writetofile: Partially overwriting a single block of a file.  ebn->key = %ld, lock->extentoffset = %ld\n",BE2L(ebn->be_key),lock->extentoffset));

            bytes=globals->bytes_block-offsetinblock;

            if(bytes>bytestowrite) {
              bytes=bytestowrite;
            }

//            if(newbytes>0 && offsetinblock==0) {      /** offsetinblock check is NOT redundant. */
//              struct CacheBuffer *cb=getcachebuffer();
//
//              CopyMem(buffer, cb->data, bytes);
//              errorcode=writethrough(lock->curextent + (lock->extentoffset>>shifts_block), cb->data, 1);
//              emptycachebuffer(cb);
//
//              if(errorcode!=0) {
//                break;
//              }
//            }
//            else {
            if((errorcode=writebytes(BE2L(ebn->be_key)+(lock->extentoffset>>globals->shifts_block), buffer, offsetinblock, bytes))!=0) {
              break;
            }
//            }
          }
          else {
            bytes=(((ULONG)ebn_blocks)<<globals->shifts_block) - lock->extentoffset;

            if(bytes > bytestowrite) {
              bytes=bytestowrite & ~globals->mask_block;

              /** This is a hack to speed up writes.

                  What it does is write more bytes than there are in the buffer
                  available (runaway writing), to avoid a seperate partial block
                  write.  It makes sure the extra data written doesn't extend
                  into a new 4K page (assuming MMU is using 4K pages). */

//              #define MMU_PAGESIZE (524288)
//
//              if(((ULONG)(buffer+bytes) & ~(MMU_PAGESIZE-1))==((ULONG)(buffer+bytes+bytes_block-1) & ~(MMU_PAGESIZE-1))) {
//                bytes=bytestowrite;
//              }
            }

            // _XDEBUG((DEBUG_IO,"writetofile: Writing multiple blocks: blockstowrite = %ld, ebn->key = %ld, lock->extentoffset = %ld, lock->offset = %ld\n",blockstowrite, ebn->key, lock->extentoffset, lock->offset));

            if((errorcode=write(BE2L(ebn->be_key)+(lock->extentoffset>>globals->shifts_block), buffer, (bytes+globals->bytes_block-1)>>globals->shifts_block))!=0) {
              break;
            }
          }

          seekforward(lock, ebn_blocks, ebn_next, bytes);

          bytestowrite-=bytes;
          buffer+=bytes;
        }
      }
    }
  }
  else {
    errorcode=ERROR_WRITE_PROTECTED;
  }

  return(errorcode);
}



LONG deletefileslowly(struct CacheBuffer *cbobject, struct fsObject *o) {
  ULONG size=BE2L(o->object.file.be_size);
  ULONG key=BE2L(o->object.file.be_data);
  LONG errorcode=0;

  /* cbobject & o refer to the file to be deleted (don't use this for objects
     other than files!).  The file is deleted a piece at the time.  This is
     because then even in low space situations files can be deleted.

     Note: This function deletes an object without first checking if
           this is allowed.  Use deleteobject() instead. */

  _DEBUG(("deletefileslowly: Entry\n"));

  /* First we search for the last ExtentBNode */

  if(key!=0) {
    struct CacheBuffer *cb;
    struct fsExtentBNode *ebn;
    ULONG currentkey=0, prevkey=0;
    ULONG blocks=0;

    lockcachebuffer(cbobject);

    while((errorcode=findbnode(globals->block_extentbnoderoot,key,&cb,(struct BNode **)&ebn))==0) {
      if(BE2L(ebn->be_next)==0) {
        currentkey=BE2L(ebn->be_key);
        prevkey=BE2L(ebn->be_prev);
        blocks=BE2W(ebn->be_blocks);
        break;
      }
      key=BE2L(ebn->be_next);
    }

    /* Key could be zero (in theory) or contains the last ExtentBNode for this file. */

    if(errorcode==0) {
      while((key & 0x80000000)==0) {
        key=prevkey;

        newtransaction();

        lockcachebuffer(cb);   /* Makes sure freespace() doesn't reuse this cachebuffer */

        if((errorcode=freespace(currentkey, blocks))==0) {
          unlockcachebuffer(cb);

          if((errorcode=deletebnode(globals->block_extentbnoderoot, currentkey))==0) {
            if((key & 0x80000000)==0) {
              if((errorcode=findbnode(globals->block_extentbnoderoot, key, &cb, (struct BNode **)&ebn))==0) {
                preparecachebuffer(cb);

                ebn->be_next=0;
                currentkey=BE2L(ebn->be_key);
                prevkey=BE2L(ebn->be_prev);
                blocks=BE2W(ebn->be_blocks);

                errorcode=storecachebuffer(cb);
              }
            }
            else {
              preparecachebuffer(cbobject);

              checksum_writelong_be(cbobject->data, &o->object.file.be_size, 0);
              checksum_writelong_be(cbobject->data, &o->object.file.be_data, 0);

              // o->object.file.data=0;
              // o->object.file.size=0;

              if((errorcode=storecachebuffer_nochecksum(cbobject))==0) {
                errorcode=setrecycledinfodiff(0, -((size+globals->bytes_block-1)>>globals->shifts_block));
              }
            }
          }
        }
        else {
          unlockcachebuffer(cb);
        }

        if(errorcode==0) {
          /* Unlocked CacheBuffers could be killed after an endtransaction()! */
          endtransaction();
        }
        else {
          deletetransaction();
          break;
        }
      }
    }

    unlockcachebuffer(cbobject);
  }

  _DEBUG(("deletefileslowly: Alternative way errorcode = %ld\n",errorcode));

  if(errorcode==0) {

    /* File-data was succesfully freed.  Now remove the empty object. */

    newtransaction();

    if((errorcode=removeobject(cbobject, o))==0) {
      endtransaction();
      return(0);
    }

    deletetransaction();
  }

  _DEBUG(("deletefileslowly: Exiting with errorcode = %ld\n",errorcode));

  return(errorcode);
}


/*

Transactions
------------

ReadCacheBuffer
ReadOriginalCacheBuffer
LockCacheBuffer
UnLockCacheBuffer
NewTransaction
DeleteTransaction
EndTransaction
StoreCacheBuffer
PrepareCacheBuffer

The transaction system allows the filesystem to keep track
of changes made to blocks in the cache.  Every change is
stored in a transaction buffer.  The transaction buffer and
the original version of a block can be used to restore the
latest version of a block at any time.

At any time you can use readcachebuffer() to get the latest
version of a block.  This could be a partially modified
version if you're in the middle of a transaction, the latest
version stored in the transaction buffer or the original
version from disk.

When you want to make changes to a block you call
preparecachebuffer().  This checks if the block you're
preparing to change is the original version, and if so makes
a backup copy.  This copy is not strictly needed, but it
speeds up the filesystem to keep the original version around
in case we need it later.  The preparecachebuffer() function
also calls lockcachebuffer() so the block you're changing
won't be reused in subsequent cache operations.

When you're satisfied with the changes you've made to a
specific block you should call storecachebuffer().  This
stores the changes into the transaction buffer.  From that
point on readcachebuffer() will be able to recreate the
block with your new modifications.

Before calling preparecachebuffer() for the first time, you
need to call newtransaction().  This is to let the
transaction system know you're about to start a new series
of modifications.

When you're done making all the changes to multiple blocks
you need to call endtransaction().  This makes all the
changes permanent.  If there was an error along the way you
can call deletetransaction() and all the changes you stored
with storecachebuffer() since you're last call to
newtransaction() will automatically be discarded.

Any cachebuffers you had locked at the time which had
changes in them will be restored to their original state (in
reality these buffers will simply be reread using
readcachebuffer()).

Examples:

x=readcachebuffer(0);    // x.data = 1

newtransaction();

preparecachebuffer(x);

x.data=2;

storecachebuffer(cb);

deletetransaction();     // x.data = 1;

read(x) -> "AA";  newtr(); prep(x); write("CC") -> "CC"; store(x); endtr() -> "CC";

read(x) -> "AA";  newtr(); prep(x); write("CC") -> "CC"; store(x); deltr() -> "AA";

read(x) -> "AA";  newtr(); prep(x); write("CC") -> "CC"; store(x); newtr(); prep(x); write("EE") -> "EE"; store(x); deltr() -> "CC"; deltr() -> "AA";

*/





LONG setnextextent(BLCK next, BLCK key) {
  struct CacheBuffer *cb;
  struct fsExtentBNode *ebn;
  LONG errorcode=0;

  /* This function set the previous value of the
     passed Extent.  If the passed Extent is zero
     then this function does nothing. */

  if(next!=0 && (errorcode=findextentbnode(next, &cb, &ebn))==0) {
    preparecachebuffer(cb);

    ebn->be_prev=L2BE(key);

    errorcode=storecachebuffer(cb);
  }

  return(errorcode);
}



LONG setprevextent(BLCK prev, BLCK key) {
  struct CacheBuffer *cb;
  struct fsExtentBNode *ebn;
  LONG errorcode;

  /* This function sets the next value of the
     passed Extent/Object to key. */

  if((prev & 0x80000000)==0 && (errorcode=findextentbnode(prev, &cb, &ebn))==0) {
    preparecachebuffer(cb);

    ebn->be_next=L2BE(key);

    errorcode=storecachebuffer(cb);
  }
  else {
    struct fsObject *o;

    if((errorcode=readobject((prev & 0x7fffffff), &cb, &o))==0) {
      preparecachebuffer(cb);

      o->object.file.be_data=L2BE(key);

      errorcode=storecachebuffer(cb);
    }
  }

  return(errorcode);
}



LONG mergeextent(BLCK key) {
  struct CacheBuffer *cb;
  struct fsExtentBNode *ebn;
  LONG errorcode;

  /* This function tries to merge the current extent with the
     next extent.  If the extent can't be merged, or was
     succesfully merged then this function returns 0.  Any
     error which occured during merging will be returned. */

  if((errorcode=findextentbnode(key, &cb, &ebn))==0) {

    /* We check if we found the right key, if there is a next Extent and
       if the two Extents touch each other: */

    if(BE2L(ebn->be_key)==key && BE2L(ebn->be_next)!=0 && BE2L(ebn->be_key)+BE2W(ebn->be_blocks) == BE2L(ebn->be_next)) {
      struct CacheBuffer *cb2;
      struct fsExtentBNode *ebn2;

      if((errorcode=findextentbnode(BE2L(ebn->be_next), &cb2, &ebn2))==0 && BE2W(ebn2->be_blocks)+BE2W(ebn->be_blocks) < 65536) {
        BLCK next=BE2L(ebn2->be_next);

        /* Merge next extent with our extent */

        preparecachebuffer(cb);

        ebn->be_blocks=W2BE(BE2W(ebn->be_blocks)+BE2W(ebn2->be_blocks));
        ebn->be_next=L2BE(next);

        if((errorcode=storecachebuffer(cb))==0) {   // call storecachebuffer() here, because deletebnode() may move our BNode.

          if((errorcode=deletebnode(globals->block_extentbnoderoot, BE2L(ebn2->be_key)))==0) {    /*** Maybe use deleteinternalnode here??? */
            errorcode=setnextextent(next, key);
          }
        }
      }
    }
  }

  return(errorcode);
}



LONG insertextent(BLCK key, BLCK next, BLCK prev, ULONG blocks) {
  struct CacheBuffer *cb=0;
  struct fsExtentBNode *ebn=0;
  LONG errorcode;

  /* This function creates a new extent, but won't create one if it can
     achieve the same effect by extending the next or previous extent.
     In the unlikely case that the new extent is located EXACTLY between
     its predecessor and successor then an attempt will be made to merge
     these 3 extents into 1. */

  /*
  prev available & mergeable        -> merge.
  prev available, but not mergeable -> create new -> update next -> update previous.
  prev not available                -> create new -> update next -> update object
  */

  if((prev & 0x80000000)!=0 || (errorcode=findextentbnode(prev, &cb, &ebn))==0) {

    if((prev & 0x80000000)==0 && prev+BE2W(ebn->be_blocks) == key && BE2W(ebn->be_blocks)+blocks < 65536) {

      /* Extent we are inserting is mergeable with its previous extent. */

      preparecachebuffer(cb);

      ebn->be_blocks=W2BE(BE2W(ebn->be_blocks)+blocks);   /* This merges the previous and the new extent */

      if((errorcode=storecachebuffer(cb))==0) {
        errorcode=mergeextent(BE2L(ebn->be_key));
      }
    }
    else {

      /* Extent we are inserting couldn't be merged with its previous extent. */

      if((errorcode=setprevextent(prev, key))==0 && (errorcode=createextentbnode(key, &cb, &ebn))==0) {

        /* Succesfully updated previous extent, or the object.  Also created new BNode */

        ebn->be_key=L2BE(key);
        ebn->be_prev=L2BE(prev);
        ebn->be_next=L2BE(next);
        ebn->be_blocks=BE2W(blocks);

        if((errorcode=storecachebuffer(cb))==0) {
          if((errorcode=setnextextent(next, key))==0) {
            mergeextent(key);
          }
        }
      }
    }
  }

  return(errorcode);
}



LONG truncateextent(BLCK key, LONG blocks) {
  struct CacheBuffer *cb;
  struct fsExtentBNode *ebn;
  LONG errorcode=0;

  /* Truncates the extended by the given amount of blocks.
     If the amount is negative then the truncation occurs
     at the start, otherwise at the end.

     This function returns INTERR_EXTENT if you tried to
     truncate to zero blocks (or beyond).  Any errorcode
     is returned.  If blocks is zero, then this function
     does nothing.

     Mar 20 1999: The truncation at the start could cause
                  BNode's to get lost (because they are
                  indexed by their start block).  Fixed. */

  if(blocks!=0) {
    if((errorcode=findextentbnode(key, &cb, &ebn))==0) {
      ULONG b;

      b=blocks<0 ? -blocks : blocks;

      if(b<BE2W(ebn->be_blocks) && BE2L(ebn->be_key)==key) {
        if(blocks<0) {
          ULONG next=BE2L(ebn->be_next);
          ULONG prev=BE2L(ebn->be_prev);
          UWORD blocks=BE2W(ebn->be_blocks)-b;

          /* Truncating at the start. */

          if((errorcode=deletebnode(globals->block_extentbnoderoot, key))==0) {

            key+=b;

            if((errorcode=createbnode(globals->block_extentbnoderoot, key, &cb, (struct BNode **)&ebn))==0) {
              ebn->be_key=L2BE(key);
              ebn->be_next=L2BE(next);
              ebn->be_prev=L2BE(prev);
              ebn->be_blocks=W2BE(blocks);

              if((errorcode=storecachebuffer(cb))==0) {
                /* Truncating at start means changing the key value.  This
                   means that the next and previous BNode's must also be
                   adjusted. */

                if((errorcode=setnextextent(next, key))==0) {
                  errorcode=setprevextent(prev, key);
                }
              }
            }
          }
        }
        else {

          /* Truncating at the end. */

          preparecachebuffer(cb);

          ebn->be_blocks=W2BE(BE2W(ebn->be_blocks)-b);

          errorcode=storecachebuffer(cb);
        }
      }
      else {
        errorcode=INTERR_EXTENT;
      }
    }
  }

  return(errorcode);
}



static LONG copy(BLCK source, BLCK dest, ULONG totblocks, UBYTE *optimizebuffer) {
  ULONG blocks;
  LONG errorcode=0;

  /* Low-level function to copy blocks from one place to another. */

  while(totblocks!=0) {
    blocks=totblocks;
    if(blocks > (OPTBUFSIZE>>globals->shifts_block)) {
      blocks=OPTBUFSIZE>>globals->shifts_block;
    }

    if((errorcode=read(source, optimizebuffer, blocks))!=0) {
      break;
    }

    if((errorcode=write(dest, optimizebuffer, blocks))!=0) {
      break;
    }

    totblocks-=blocks;
    source+=blocks;
    dest+=blocks;
  }

  return(errorcode);
}



LONG deleteextent(struct CacheBuffer *cb, struct fsExtentBNode *ebn) {
  BLCK next,prev;
  LONG errorcode;

  /* Deletes an fsExtentBNode structure and properly relinks the rest of the chain.
     No space will be given free.

     newtransaction() should have been called prior to calling this function. */

  next=BE2L(ebn->be_next);
  prev=BE2L(ebn->be_prev);

  if((errorcode=deletebnode(globals->block_extentbnoderoot,BE2L(ebn->be_key)))==0) {        /*** Maybe use deleteinternalnode here??? */
    if((errorcode=setnextextent(next, prev))==0) {
      errorcode=setprevextent(prev, next);
    }
  }

  return(errorcode);
}



static WORD enough_for_add_moved(void) {
  if(globals->defragmentlongs>5) {
    return TRUE;
  }
  return FALSE;
}

static inline void add_moved(ULONG blocks, ULONG from, ULONG to) {
  if(globals->defragmentsteps!=0) {
    *globals->defragmentsteps++=AROS_LONG2BE(MAKE_ID('M','O','V','E'));
    *globals->defragmentsteps++=3;
    *globals->defragmentsteps++=blocks;
    *globals->defragmentsteps++=from;
    *globals->defragmentsteps++=to;
    *globals->defragmentsteps=0;

    globals->defragmentlongs-=5;
  }
}

static inline void add_done(void) {
  if(globals->defragmentsteps!=0) {
    *globals->defragmentsteps++=AROS_LONG2BE(MAKE_ID('D','O','N','E'));
    *globals->defragmentsteps++=0;
    *globals->defragmentsteps=0;

    globals->defragmentlongs-=2;
  }
}



LONG moveextent(struct fsExtentBNode *ebn, BLCK dest, UWORD blocks) {
  UBYTE *buf;
  LONG errorcode;

  /* This function (partially) moves the Extent to /dest/.
     /blocks/ is the number of blocks moved; if blocks is
     equal to the size of the Extent, then the Extent will
     be deleted.  Else the start of the Extent will be
     truncated. */

  if((buf=AllocVec(OPTBUFSIZE, globals->bufmemtype))!=0) {
    BLCK key=BE2L(ebn->be_key);
    BLCK next=BE2L(ebn->be_next);
    BLCK prev=BE2L(ebn->be_prev);
    UWORD blocksinextent=BE2W(ebn->be_blocks);

    if((errorcode=copy(key, dest, blocks, buf))==0) {       // This functions knows that OPTBUFSIZE is the size of the buffer!

      /* Data has been physically moved -- nothing has been
         permanently altered yet. */

      if((errorcode=markspace(dest, blocks))==0) {

        if((errorcode=freespace(key, blocks))==0) {

          /* Bitmap has been altered to reflect the new location of the
             moved data.  Now we either need to truncate the Extent (if
             it was partially moved) or remove it. */

          if(blocksinextent==blocks) {
            struct CacheBuffer *cb;

            if((errorcode=findextentbnode(key, &cb, &ebn))==0) {
              errorcode=deleteextent(cb, ebn);
            }
          }
          else {
            next=key+blocks;
            errorcode=truncateextent(key, -blocks);
          }

          if(errorcode==0) {
            if((errorcode=insertextent(dest, next, prev, blocks))==0) {
              add_moved(blocks, key, dest);
            }
          }
        }
      }
    }

    FreeVec(buf);
  }
  else {
    errorcode=ERROR_NO_FREE_STORE;
  }

  return(errorcode);
}



static LONG fillgap(BLCK key) {
  struct CacheBuffer *cb;
  struct fsExtentBNode *ebn;
  LONG errorcode;

  /* This function will attempt to fill the gap behind an extent with
     data from the next extent.  This in effect merges the extent and
     the next extent (partially). */

  if((errorcode=findextentbnode(key, &cb, &ebn))==0) {
    ULONG next=BE2L(ebn->be_next);

    key+=BE2W(ebn->be_blocks);

    while(next!=0 && (errorcode=findextentbnode(next, &cb, &ebn))==0) {          //      !! failed !!
      UWORD blocks=BE2W(ebn->be_blocks);
      LONG free;

      lockcachebuffer(cb);

      if((free=availablespace(key, 1024))>0) {

        unlockcachebuffer(cb);

        _DEBUG(("fillgap: availablespace() returned %ld\n",free));

        /* The gap consists of /free/ blocks. */

        if(free > blocks && enough_for_add_moved()!=FALSE) {
          next=BE2L(ebn->be_next);
        }
        else {
          next=0;
        }

        if((errorcode=moveextent(ebn, key, MIN(free, blocks)))!=0) {
          return(errorcode);
        }

        key+=blocks;
      }
      else {
        unlockcachebuffer(cb);
      }
    }
  }

  _DEBUG(("fillgap: exiting with errorcode %ld\n",errorcode));

  return(errorcode);
}



LONG getbnode(BLCK block, struct CacheBuffer **returned_cb, struct fsExtentBNode **returned_ebn) {
  LONG errorcode;

  /* This function gets the ExtentBNode which starts at the given
     block or the first one after the given block.  Zero *ebn indicates
     there were no ExtentBNode's at or after the given block. */

  if((errorcode=findbnode(globals->block_extentbnoderoot, block, returned_cb, (struct BNode **)returned_ebn))==0) {

    _DEBUG(("getbnode: ebn->key = %ld, ebn->prev = %ld, ebn->blocks = %ld\n",BE2L((*returned_ebn)->be_key), BE2L((*returned_ebn)->be_prev), BE2W((*returned_ebn)->be_blocks)));

    if(*returned_ebn!=0 && BE2L((*returned_ebn)->be_key)<block) {
      errorcode=nextbnode(globals->block_extentbnoderoot, returned_cb, (struct BNode **)returned_ebn);

      _DEBUG(("getbnode: 2: ebn->key = %ld, ebn->prev = %ld, ebn->blocks = %ld\n",BE2L((*returned_ebn)->be_key), BE2L((*returned_ebn)->be_prev), BE2W((*returned_ebn)->be_blocks)));
    }
  }

  return(errorcode);
}



#if 0
LONG makefreespace(BLCK block) {
  struct CacheBuffer *cb;
  struct fsExtentBNode *ebn;
  LONG errorcode;

  /* This function tries to move the data located at /block/ to
     another area of the disk. */

  if((errorcode=findextentbnode(block, &cb, &ebn))==0) {
    ULONG blocks;
    ULONG newblocks;
    BLCK startblock;

    lockcachebuffer(cb);

    blocks=MIN(OPTBUFSIZE>>shifts_block, ebn->blocks);

    if((errorcode=findspace2_backwards(blocks, block, blocks_total, &startblock, &newblocks))==0) {
      unlockcachebuffer(cb);

      if(newblocks!=0) {

        _DEBUG(("makefreespace: Looking for %ld blocks from block %ld, and found %ld blocks at block %ld.\n", blocks, block, newblocks, startblock));

        errorcode=moveextent(ebn, startblock, newblocks);
      }
      else {
        errorcode=ERROR_DISK_FULL;
      }
    }
    else {
      unlockcachebuffer(cb);
    }
  }

  return(errorcode);
}
#endif


LONG makefreespace(BLCK block) {
  struct CacheBuffer *cb;
  struct fsExtentBNode *ebn;
  LONG errorcode;
  WORD count=0;

  /* This function tries to move the data located at /block/ to
     another area of the disk. */

  if((errorcode=findextentbnode(block, &cb, &ebn))==0) {
    do {
      ULONG blocks;
      ULONG newblocks;
      BLCK startblock;

      lockcachebuffer(cb);

      blocks=MIN(OPTBUFSIZE>>globals->shifts_block, BE2W(ebn->be_blocks));

      if((errorcode=findspace2_backwards(blocks, BE2L(ebn->be_key), globals->blocks_total, &startblock, &newblocks))==0) {      // ebn->key should not be changed to block.
        unlockcachebuffer(cb);

        if(newblocks!=0) {

          _DEBUG(("makefreespace: Looking for %ld blocks from block %ld, and found %ld blocks at block %ld.\n", blocks, block, newblocks, startblock));

          if((errorcode=moveextent(ebn, startblock, newblocks))!=0) {
            break;
          }
        }
        else {
          if(count==0) {
            errorcode=ERROR_DISK_FULL;
          }
          break;
        }
      }
      else {
        unlockcachebuffer(cb);
        break;
      }

      if(count++>=1) {
        break;
      }

      if((errorcode=getbnode(block, &cb, &ebn))!=0 || ebn==0) {
        break;
      }

    } while((BE2L(ebn->be_prev) & 0x80000000)==0);
  }

  return(errorcode);
}



LONG skipunmoveable(BLCK block) {
  struct CacheBuffer *cb;
  struct fsExtentBNode *ebn;

  /* This function looks for moveable data or free space starting
     from the given block.  It returns -1 in case of failure, or
     the first moveable block it finds.  If there ain't no more
     moveable blocks, then blocks_total is returned. */

  if((findbnode(globals->block_extentbnoderoot, block, &cb, (struct BNode **)&ebn))==0) {
    if(ebn!=0 && ebn->be_key==L2BE(block)) {
      return((LONG)block);
    }
    else if(ebn==0 || BE2L(ebn->be_key)>=block || nextbnode(globals->block_extentbnoderoot, &cb, (struct BNode **)&ebn)==0) {
      if(ebn!=0) {
        BLCK key=BE2L(ebn->be_key);
        LONG used;

        /* Found something moveable, but maybe there was some free space before the
           moveable Extent. */

        if((used=allocatedspace(block, key-block))!=-1) {
          if(block+used<key) {
            return((LONG)block+used);
          }
          else {
            return((LONG)key);
          }
        }
      }
      else {
        LONG errorcode;

        if((errorcode=findspace(1, block, globals->blocks_total, &block))==0) {
          return((LONG)block);
        }
        else if(errorcode==ERROR_DISK_FULL) {
          return((LONG)globals->blocks_total);
        }
      }
    }
  }

  return(-1);
}


struct fsExtentBNode *startofextentbnodechain(struct fsExtentBNode *ebn) {
  struct CacheBuffer *cb;
  LONG errorcode=0;

  while((BE2L(ebn->be_prev) & 0x80000000)==0 && (errorcode=findextentbnode(BE2L(ebn->be_prev), &cb, &ebn))==0) {
  }

  if(errorcode==0) {
    return(ebn);
  }

  return(0);
}



#if 0
LONG findmatch(BLCK startblock, ULONG blocks, ULONG *bestkey) {
  struct CacheBuffer *cb;
  struct fsExtentBNode *ebn;
//  struct fsExtentBNode *ebn_start;
  ULONG bestblocks=0;
  LONG errorcode;

  /* This function looks for the start of a ExtentBNode chain
     which matches the given size.  If none is found, then the
     next smaller chain is returned.  If none is found, then a
     larger chain is returned.  If there are no chains at all
     0 is returned.

     The first ExtentBNode examined is determined by the start
     block number which is passed. */

  _DEBUG(("findmatch: Looking for a chain of %ld blocks starting from block %ld\n", blocks, startblock));

  *bestkey=0;

  if((errorcode=getbnode(startblock, &cb, &ebn))==0 && ebn!=0) {

    _DEBUG(("findmatch: 1, ebn->key = %ld, ebn->blocks = %ld\n", ebn->key, ebn->blocks));

    do {
      struct CacheBuffer *cb2;
      struct fsExtentBNode *ebn_start=ebn;
      struct fsObject *o;
      ULONG total=ebn->blocks;      /* If larger than bestblocks, then we can stop looking for the start of the ExtentBNode chain early. */
      ULONG key, newblocks;

      lockcachebuffer(cb);
//      ebn_start=startofextentbnodechain(ebn);
      {
        struct CacheBuffer *cb;

        while((total<bestblocks || bestblocks==0) && (ebn_start->prev & 0x80000000)==0 && ebn_start->prev >= ebn->key && (errorcode=findextentbnode(ebn_start->prev, &cb, &ebn_start))==0) {
          total+=ebn_start->blocks;
        }
      }
      unlockcachebuffer(cb);

      if(errorcode!=0) {
        break;
      }

      if((total<bestblocks || bestblocks==0) && (ebn_start->prev & 0x80000000)!=0) {

        _DEBUG(("findmatch: 2, ebn->key = %ld\n", ebn->key));

        key=ebn_start->key;

        lockcachebuffer(cb);
        errorcode=readobject(ebn_start->prev & 0x7FFFFFFF, &cb2, &o);
        unlockcachebuffer(cb);

        if(errorcode!=0) {
          break;
        }

        _DEBUG(("findmatch: 3, filesize = %ld\n",o->object.file.size));

        newblocks=(o->object.file.size+bytes_block-1)>>shifts_block;

        if(newblocks==blocks) {          /* Found ideal match */
          *bestkey=key;
          break;
        }
        else if(newblocks<blocks) {
          if(newblocks>bestblocks || bestblocks>blocks) {
            *bestkey=key;
            bestblocks=newblocks;
          }
        }
        else if(bestblocks==0 || newblocks<bestblocks) {
          *bestkey=key;
          bestblocks=newblocks;
        }
      }

    } while((errorcode=nextbnode(block_extentbnoderoot, &cb, (struct BNode **)&ebn))==0 && ebn!=0);
  }

  return(errorcode);
}
#endif




void newfragmentinit_large(ULONG blocks) {
  globals->bestkey=0;
  globals->bestblocks=0;
  globals->searchedblocks=blocks;
}



ULONG newfragment_large(ULONG block, ULONG blocks) {
  if(blocks==globals->searchedblocks) {          /* Found ideal match */
    return(block);
  }
  else if(blocks<globals->searchedblocks) {
    if(blocks>globals->bestblocks || globals->bestblocks>globals->searchedblocks) {
      globals->bestkey=block;
      globals->bestblocks=blocks;
    }
  }
  else if(globals->bestblocks==0 || blocks<globals->bestblocks) {
    globals->bestkey=block;
    globals->bestblocks=blocks;
  }

  return(0);
}



#if 0
ULONG newfragment_large(ULONG block, ULONG blocks, UBYTE type) {
  if(type==0) {
    if(blocks==searchedblocks) {          /* Found ideal match */
      return(block);
    }
    else if(blocks<searchedblocks) {
      if(blocks>bestblocks || bestblocks>searchedblocks) {
        bestkey=block;
        bestblocks=blocks;
      }
    }
    else if(bestblocks==0 || blocks<bestblocks) {
      bestkey=block;
      bestblocks=blocks;
    }
  }

  return(0);
}
#endif


ULONG newfragmentend_large(void) {
  return(globals->bestkey);
}




void newfragmentinit_small(ULONG blocks) {
  ULONG *f=globals->fragment;
  WORD n=FRAGMENTS;

  newfragmentinit_large(blocks);

  while(--n>=0) {
    *f++=0;
  }
}





/*
void newfragmentinit_small(ULONG blocks) {
  ULONG *f=fragment;
  UBYTE *fb=fragmenttype;
  WORD n=FRAGMENTS;

  newfragmentinit_large(blocks);

  while(--n>=0) {
    *f++=0;
    *fb++=0;
  }
}



ULONG newfragment_small(ULONG block, ULONG blocks, UBYTE type) {
  if(blocks<searchedblocks) {
    if(fragment[blocks]==0 || (fragmenttype[blocks]!=0 && type==0)) {
      ULONG blocks2=searchedblocks-blocks;

      fragment[blocks]=block;
      fragmenttype[blocks]=type;

      if(blocks2==blocks) {
        blocks2=0;
      }

      if(fragment[blocks2]!=0) {
        if(fragmenttype[blocks]==0 && fragmenttype[blocks2]==0) {
          return(fragment[blocks]);
        }
      }
    }
  }

  return(newfragment_large(block, blocks, type));
}
*/



ULONG newfragment_small(ULONG block, ULONG blocks) {
  if(blocks<globals->searchedblocks && globals->fragment[blocks]==0) {
    ULONG blocks2=globals->searchedblocks-blocks;

    globals->fragment[blocks]=block;

    if(blocks2==blocks) {
      blocks2=0;
    }

    if(globals->fragment[blocks2]!=0) {
      return(globals->fragment[blocks]);
    }
  }

  return(newfragment_large(block, blocks));
}



ULONG newfragmentend_small(void) {
  return(newfragmentend_large());
}




#if 0
LONG findmatch(BLCK startblock, ULONG blocks, ULONG *bestkey) {
  struct CacheBuffer *cb;
  struct fsExtentBNode *ebn;
  ULONG lastextentend=0;
  LONG maxscan=defrag_maxfilestoscan;
  LONG errorcode;

  /* This function looks for the start of a ExtentBNode chain
     which matches the given size.  If none is found, then the
     next smaller chain is returned.  If none is found, then a
     larger chain is returned.  If there are no chains at all
     0 is returned.

     The first ExtentBNode examined is determined by the start
     block number which is passed. */

  _DEBUG(("findmatch: Looking for a chain of %ld blocks starting from block %ld\n", blocks, startblock));

  *bestkey=0;

  if(blocks>FRAGMENTS-1) {
    newfragmentinit_large(blocks);
  }
  else {
    newfragmentinit_small(blocks);
  }

  if((errorcode=getbnode(startblock, &cb, &ebn))==0 && ebn!=0) {

    _DEBUG(("findmatch: ebn->key = %ld, ebn->blocks = %ld\n", ebn->key, ebn->blocks));

    do {
      if((ebn->prev & 0x80000000)!=0) {
        struct CacheBuffer *cb2;
        struct fsObject *o;
        ULONG newblocks;
        UBYTE fragmenttype;

        /* Found the start of a candidate chain. */

        lockcachebuffer(cb);
        errorcode=readobject(ebn->prev & 0x7FFFFFFF, &cb2, &o);
        unlockcachebuffer(cb);

        if(errorcode!=0) {
          break;
        }

        newblocks=(o->object.file.size+bytes_block-1)>>shifts_block;

        fragmenttype=lastextentend==ebn->key ? 1 : 0;

        if(blocks>FRAGMENTS-1) {
          *bestkey=newfragment_large(ebn->key, newblocks, fragmenttype);
        }
        else {
          *bestkey=newfragment_small(ebn->key, newblocks, fragmenttype);
        }

        if(*bestkey!=0) {
          return(0);
        }

        if(--maxscan<0) {
          break;
        }

        lastextentend=ebn->next==0 ? ebn->key + ebn->blocks : 0;
      }
    } while((errorcode=nextbnode(block_extentbnoderoot, &cb, (struct BNode **)&ebn))==0 && ebn!=0);

    if(errorcode==0) {
      if(blocks>FRAGMENTS-1) {
        *bestkey=newfragmentend_large();
      }
      else {
        *bestkey=newfragmentend_small();
      }
    }
  }

  return(errorcode);
}
#endif



LONG findmatch_fromend(BLCK startblock, ULONG blocks, ULONG *bestkey) {
  struct CacheBuffer *cb;
  struct fsExtentBNode *ebn;
  LONG maxscan=globals->defrag_maxfilestoscan;
  LONG errorcode;

  /* This function looks for the start of a ExtentBNode chain
     which matches the given size.  If none is found, then the
     next smaller chain is returned.  If none is found, then a
     larger chain is returned.  If there are no chains at all
     0 is returned.

     The first ExtentBNode examined is determined by the start
     block number which is passed. */

  _DEBUG(("findmatch_fromend: Looking for a chain of %ld blocks starting from block %ld\n", blocks, startblock));

  *bestkey=0;

  if(blocks>FRAGMENTS-1) {
    newfragmentinit_large(blocks);
  }
  else {
    newfragmentinit_small(blocks);
  }

  if((errorcode=lastbnode(globals->block_extentbnoderoot, &cb, (struct BNode **)&ebn))==0 && ebn!=0 && BE2L(ebn->be_key)>=startblock) {

    _DEBUG(("findmatch_fromend: ebn->key = %ld, ebn->blocks = %ld\n", BE2L(ebn->be_key), BE2W(ebn->be_blocks)));

    do {
      if((BE2L(ebn->be_prev) & 0x80000000)!=0) {   // Is this a 'first fragment' of something?
        struct CacheBuffer *cb2;
        struct fsObject *o;
        ULONG newblocks;

        _DEBUG(("findmatch_fromend!: ebn->key = %ld, ebn->blocks = %ld\n", BE2L(ebn->be_key), BE2W(ebn->be_blocks)));

        /* Found the start of a candidate chain. */

        lockcachebuffer(cb);
        errorcode=readobject(BE2L(ebn->be_prev) & 0x7FFFFFFF, &cb2, &o);
        unlockcachebuffer(cb);

        if(errorcode!=0) {
          break;
        }

        newblocks=(BE2L(o->object.file.be_size)+globals->bytes_block-1)>>globals->shifts_block;

        if(blocks>FRAGMENTS-1) {
          *bestkey=newfragment_large(BE2L(ebn->be_key), newblocks);
        }
        else {
          *bestkey=newfragment_small(BE2L(ebn->be_key), newblocks);
        }

        if(*bestkey!=0) {
          return(0);
        }

        if(--maxscan<0) {
          break;
        }
      }
    } while((errorcode=previousbnode(globals->block_extentbnoderoot, &cb, (struct BNode **)&ebn))==0 && ebn!=0 && BE2L(ebn->be_key)>=startblock);

    if(errorcode==0) {
      if(blocks>FRAGMENTS-1) {
        *bestkey=newfragmentend_large();
      }
      else {
        *bestkey=newfragmentend_small();
      }
    }
    else {
      _DEBUG(("findmatch_fromend: errorcode=%ld\n", errorcode));
    }
  }

  return(errorcode);
}



LONG step(void) {
  LONG errorcode;
  LONG free;

  if((free=availablespace(globals->block_defragptr, 256))!=-1) {

    _DEBUG(("Defragmenter: Found %ld blocks of free space at block %ld.\n", free, globals->block_defragptr));

    if(free==0) {
      struct CacheBuffer *cb;
      struct fsExtentBNode *ebn;

      /* Determine in which extent block_defragptr is located. */

      if((errorcode=findbnode(globals->block_extentbnoderoot, globals->block_defragptr, &cb, (struct BNode **)&ebn))==0) {
        if(ebn==0 || BE2L(ebn->be_key)!=globals->block_defragptr) {
          LONG block;

          _DEBUG(("Defragmenter: Found unmoveable data at block %ld.\n", globals->block_defragptr));

          /* Skip unmoveable data */

          if((block=skipunmoveable(globals->block_defragptr))!=-1) {
            globals->block_defragptr=block;
          }
          else {
            errorcode=INTERR_DEFRAGMENTER;
          }
        }
        else if((BE2L(ebn->be_prev) & 0x80000000)!=0 || BE2L(ebn->be_prev)<globals->block_defragptr) {

          _DEBUG(("Defragmenter: Found a (partially) defragmented extent at block %ld.\n", globals->block_defragptr));

          if(BE2L(ebn->be_next)==0 || BE2L(ebn->be_next) == BE2L(ebn->be_key)+BE2W(ebn->be_blocks)) {
            /* If there is no next Extent, or if the next Extent is touching
               this one, then skip the current one. */

            _DEBUG(("Defragmenter: Extent has no next or next is touching this one.\n"));

            globals->block_defragptr+=BE2W(ebn->be_blocks);
          }
          else {
            LONG freeafter;
            BLCK key=BE2L(ebn->be_key);
            BLCK next=BE2L(ebn->be_next);
            UWORD blocks=BE2W(ebn->be_blocks);

            _DEBUG(("Defragmenter: Extent has a next extent.\n"));

            if((freeafter=availablespace(key+blocks, 256))!=-1) {

              _DEBUG(("Defragmenter: There are %ld blocks of free space after the extent at block %ld.\n", freeafter, key));

              if(freeafter>0) {
                /* Move (part of) data located in next extent to this free space. */

                _DEBUG(("Defragmenter: Filling the gap.\n"));

                /* The function below can be called multiple times in a row.  When there is a large
                   gap in which multiple extents of the file will fit, then these can all be transfered
                   into the gap at once. */

                errorcode=fillgap(key);
              }
              else {
                LONG block;

                /* Determine which extent it is which is located directly after the current extent. */

                if((block=skipunmoveable(key+blocks))!=-1) {
                  if(block==key+blocks) {

                    _DEBUG(("Defragmenter: There was a moveable extent after the extent at block %ld.\n", key));

                    /* There was no unmoveable data, so let's move it. */

                    errorcode=makefreespace(block);
                  }
                  else {
                    LONG freeafter;

                    _DEBUG(("Defragmenter: Skipped %ld blocks of unmoveable data.\n", block-(key+blocks)));

                    /* Unmoveable data was skipped. */

                    if(block!=next) {  // Mar 20 1999: Check to see if the extent after the unmoveable data is not already the correct one.

                      if((freeafter=availablespace(block, 256))!=-1) {

                        _DEBUG(("Defragmenter: There are %ld blocks of free space after the unmoveable data.\n", freeafter));

                        if(freeafter==0) {

                          _DEBUG(("Defragmenter: Clearing some space at block %ld.\n", block));

                          if((errorcode=makefreespace(block))==0) {
                            if((freeafter=availablespace(block, 256))==-1) {
                              errorcode=INTERR_DEFRAGMENTER;
                            }

                            _DEBUG(("Defragmenter: There are now %ld blocks of cleared space after the unmoveable data.\n", freeafter));
                          }
                        }

                        if(errorcode==0) {
                          struct CacheBuffer *cb;
                          struct fsExtentBNode *ebn;

                          if((errorcode=findextentbnode(next, &cb, &ebn))==0) {

                            _DEBUG(("Defragmenter: Moved next extent of our extent directly after the unmoveable space (block = %ld).\n", block));

                            if((errorcode=moveextent(ebn, block, MIN(freeafter, BE2W(ebn->be_blocks))))==0) {
                              globals->block_defragptr=block;
                            }
                          }
                        }
                      }
                      else {
                        errorcode=INTERR_DEFRAGMENTER;
                      }
                    }
                    else {

                      /* Extent after unmoveable data is already correct, so no need to move it. */

                      globals->block_defragptr=block;
                    }
                  }
                }
                else {
                  errorcode=INTERR_DEFRAGMENTER;
                }
              }
            }
            else {
              errorcode=INTERR_DEFRAGMENTER;
            }
          }
        }
        else {

          _DEBUG(("Defragmenter: Found an extent at block %ld which must be moved away.\n", globals->block_defragptr));

          errorcode=makefreespace(globals->block_defragptr);
        }
      }
    }
    else {
      ULONG bestkey;

      if((errorcode=findmatch_fromend(globals->block_defragptr, free, &bestkey))==0) {
        if(bestkey!=0) {
          struct CacheBuffer *cb;
          struct fsExtentBNode *ebn;

          if((errorcode=findextentbnode(bestkey, &cb, &ebn))==0) {

            _DEBUG(("Defragmenter: Moving a new first Extent to %ld\n", globals->block_defragptr));

            errorcode=moveextent(ebn, globals->block_defragptr, MIN(free, BE2W(ebn->be_blocks)));
          }
        }
        else {
          _DEBUG(("Defragmenter: Nothing more to optimize.\n"));

          add_done();
        }
      }
    }
  }
  else {
    errorcode=INTERR_DEFRAGMENTER;
  }

  _DEBUG(("Defragmenter: Exiting with errorcode %ld\n\n",errorcode));

  return(errorcode);
}



/*

  if (block at block_optimizeptr is full) {

    determine in which extent block_optimizeptr is located.

    if (not located in extent) {
      skip unmoveable data: add 1 to block_optimizeptr.
    }
    else if (extent is first extent OR previous extent is located before block_optimizeptr) {
      if (extent is last extent (as well)) {
        file was defragmented: set block_optimizeptr to point just after this extent.
      }
      else {
        determine if there is free space located after this extent.

        if (there is free space) {
          move (part of) data located in next extent to this free space.
        }
        else {

          determine which extent it is which is located directly after the current extent.

          if (no such extent exists) {
            skip unmoveable data

            if (no free space after unmoveable data) {
              make some free space by moving data.
            }
            move (part of) data located in next extent to the (newly made) free space: set block_optimizeptr in this part.
          }
          else {
            make some free space first by moving data.
          }
        }
      }
    }
    else {
      make some free space by moving data at block_optimizeptr.
    }
  }
  else {

    determine amount of free space.

    look for an extent-chain equal to the amount of free space beyond this location.

    if (found suitable extent-chain) {
      move data to block_optimizeptr; set block_optimizeptr to point just after this data.
    }
    else {
      scan for ANY 'first' extent beyond this location.

      if (none found) {
        optimization is done.
      }
      else {
        move found 'first' extent to block_optimizeptr.
      }
    }
  }


*/


/* The simple purpose of the SFS DosList handler task is to
   provide the following non-blocking functions:

   - Adding a VolumeNode to the DosList
     data = VolumeNode

   - Removing a VolumeNode from the DosList (synchronously)
     data = VolumeNode

   All messages send to the DosList handler are freed by
   the DosList handler itself.  This is to avoid having to
   wait for the reply and then free the message yourself.
*/

#ifdef __AROS__
#undef globals
#else
#undef SysBase
#endif
#undef DOSBase

static void sdlhtask(void)
{
#ifndef __AROS__
  struct ExecBase *SysBase=globals->sysBase;
#endif
  struct DosLibrary *DOSBase;

  if((DOSBase=(struct DosLibrary *)OpenLibrary("dos.library",37))!=0) {
    Forbid();
    if(FindPort("SFS DosList handler")==0) {
      struct MsgPort *port;

      if((port=CreateMsgPort())!=0) {
        struct SFSMessage *sfsm;

        port->mp_Node.ln_Name="SFS DosList handler";
        port->mp_Node.ln_Pri=1;
        AddPort(port);
        Permit();

        for(;;) {
          struct DosList *dol;

          WaitPort(port);

          dol=LockDosList(LDF_WRITE|LDF_VOLUMES);

          while((sfsm=(struct SFSMessage *)GetMsg(port))!=0) {
            if(sfsm->command==SFSM_ADD_VOLUMENODE) {
              /* AddDosEntry rejects volumes based on their name and date. */

              if(AddDosEntry((struct DosList *)sfsm->data)==DOSFALSE) {
                sfsm->errorcode=IoErr();
              }

//            if(AddDosEntry((struct DosList *)sfsm->data)==DOSFALSE) {
//              errorcode=IoErr();
//              FreeDosEntry((struct DosList *)vn);
//              vn=0;
//            }
            }
            else if(sfsm->command==SFSM_REMOVE_VOLUMENODE) {
              struct DosList *vn=(struct DosList *)sfsm->data;

              while((dol=NextDosEntry(dol, LDF_VOLUMES))!=0) {
                if(dol==vn) {
                  RemDosEntry(dol);
                  break;
                }
              }
              //  removevolumenode(dol, (struct DosList *)sfsm->data);      /* Dangerous because of DOSBase?? */
              FreeDosEntry(vn);
            }

            FreeVec(sfsm);
          }

          UnLockDosList(LDF_WRITE|LDF_VOLUMES);
        }
      }
      else {
        Permit();
      }
    }
    else {
      Permit();
    }

    CloseLibrary((struct Library *)DOSBase);
  }
}
