/* $Id$ */
/* $Log: init.c $
 * Revision 12.9  1999/09/11  17:05:14  Michiel
 * bugfix version 18.4
 *
 * Revision 12.8  1999/05/14  11:31:34  Michiel
 * Long filename support implemented; bugfixes
 *
 * Revision 12.7  1999/02/22  16:25:30  Michiel
 * Changes for increasing deldir capacity
 *
 * Revision 12.6  1998/09/27  11:26:37  Michiel
 * Memory allocation mask check removed (issue 00126)
 * ErrorMsg param
 *
 * Revision 12.5  1998/09/03  07:12:14  Michiel
 * versie 17.4
 * bugfixes 118, 121, 123 and superindexblocks and td64 support
 *
 * Revision 12.4  1998/05/27  21:07:28  Mark
 * Added second DMAMask failover to include MEMF_LOCAL
 *
 * Revision 12.3  1997/03/03  22:04:04  Michiel
 * Release 16.21
 *
 * Revision 12.2  1996/03/07  10:05:24  Michiel
 * format fix (wt)
 *
 * Revision 12.1  1995/11/15  15:50:20  Michiel
 * Postponed operation handler 'DoPostponed' added
 *
 * Revision 11.17  1995/11/07  14:59:20  Michiel
 * initialization of new datacache (version 16.2)
 *
 * Revision 11.16  1995/10/04  14:03:53  Michiel
 * using new memorypool functions (from support.c 10.9)
 *
 * Revision 11.15  1995/09/01  11:19:39  Michiel
 * ErrorMsg adaption (see disk.c and volume.c)
 *
 * Revision 11.14  1995/08/21  04:21:36  Michiel
 * Initialise now creates the sleepport for MODE_SLEEP
 *
 * Revision 11.13  1995/07/17  12:22:34  Michiel
 * AFS User adaptions
 *
 * Revision 11.12  1995/07/11  17:29:31  Michiel
 * ErrorMsg () calls use messages.c variables now.
 *
 * Revision 11.11  1995/07/11  09:23:36  Michiel
 * DELDIR stuff
 *
 * Revision 11.10  1995/07/07  14:39:17  Michiel
 * AFSLITE stuff
 *
 * Revision 11.9  1995/06/16  10:01:02  Michiel
 * added pool for buffer memory
 *
 * Revision 11.8  1995/06/15  18:56:09  Michiel
 * pooled mem
 *
 * Revision 11.7  1995/05/20  12:12:12  Michiel
 * Updated messages to reflect Ami-FileLock
 * CUTDOWN version
 * protection update
 *
 * Revision 11.6  1995/03/30  18:57:49  Michiel
 * Initialization of notifyport added
 * Myproc has come back (for diskchangeint)
 *
 * Revision 11.5  1995/02/28  18:34:03  Michiel
 * MODE_HARDDISK, MDOE_SPLITTED_ANODES and MODE_DIR_EXTENSION added
 *
 * Revision 11.4  1995/02/15  16:43:39  Michiel
 * Release version
 * Using new headers (struct.h & blocks.h)
 *
 * Revision 11.3  1995/01/29  07:34:57  Michiel
 * Datacache now is BufMemType
 * Blockshift, directsize init added
 *
 * Revision 11.2  1995/01/15  05:25:29  Michiel
 * now always sets removable to true to
 * accommodate Syquest & Floptical
 *
 * Revision 11.1  1995/01/08  16:21:28  Michiel
 * Compiled (new MODE_BIG version)
 * added InitModules
 *
 * Revision 10.4  1994/11/15  17:52:30  Michiel
 * *** empty log message ***
 *
 * Revision 10.3  1994/10/29  08:54:34  Michiel
 * changed process references to msgport references
 *
 * Revision 10.2  1994/10/27  11:31:46  Michiel
 * *** empty log message ***
 *
 * Revision 10.1  1994/10/24  11:04:22  Michiel
 * first RCS revision
 * */

#define __USE_SYSBASE

#include <exec/types.h>
#include <exec/memory.h>
#include <exec/devices.h>
#include <exec/interrupts.h>
#include <exec/execbase.h>
#include <exec/resident.h>
#include <devices/timer.h>
#include <devices/keyboard.h>
#include <dos/filehandler.h>
#include <resources/filesysres.h>
#include <clib/alib_protos.h>

#include <string.h>
#include "debug.h"

/* own includes */
#include "blocks.h"
#include "struct.h"
#include "versionhistory.doc"
#include "init_protos.h"
#include "volume_protos.h"
#include "anodes_protos.h"
#include "directory_protos.h"
#include "lru_protos.h"
#include "allocation_protos.h"

/* external globals
 */
extern CONST UBYTE *version;
#if defined(__SASC)
extern void __asm ResetHandler(void);
#elif defined(__AROS__)
extern AROS_INTP(DiskChangeHandler);
extern AROS_INTP(ResetHandler);
#else
extern void ResetHandler(void);
#endif

/* prototypes
 */
static BOOL OpenDiskDevice(struct FileSysStartupMsg * , struct MsgPort ** , struct IOExtTD **, BOOL *, globaldata *);
static BOOL init_device_unit_sema(struct FileSysStartupMsg *, globaldata *);
static BOOL OpenTimerDevice(struct MsgPort ** , struct timerequest ** , ULONG, globaldata * );
static BOOL TestRemovability(globaldata *);
static void InstallDiskChangeHandler(globaldata *);
static void InstallResetHandler(struct globaldata *);
#if !defined(__MORPHOS__) || defined(DISK_BASED_FILESYSTEM)
/* MorphOS ROM build uses other means to add the filesystem.resource entry */
static BOOL AddToFSResource(ULONG, BPTR, struct globaldata *);
#endif
#if VERSION23
static void DoPostponed (struct volumedata *volume, globaldata *g);
#endif

/**********************************************************************/
/*                             INITIALIZE                             */
/*                             INITIALIZE                             */
/*                             INITIALIZE                             */
/**********************************************************************/

#ifdef DEBUGMODE
BOOL Initialize(DSTR mountname, struct FileSysStartupMsg *fssm,
	struct DeviceNode *devnode, globaldata *g)
{
	return(TRUE);
}
#else
BOOL Initialize(DSTR mountname, struct FileSysStartupMsg *fssm,
	struct DeviceNode *devnode, globaldata *g)
{
  LONG i;
  ULONG t;

	ENTER("Initialize");
	g->ErrorMsg = _NormalErrorMsg;

	/* mountname can be something like "pfs:hello". I only need 
	** the "pfs" part. <i> will be the mountnamelen.
	** sizeof(mountname) MUST be >0
	*/

	for (i=1; i<*mountname+1 && mountname[i]!=':'; i++);
	/* now mountname[i]==':' || i==*mountname+1 ==> i=sizeof(mnname) */
	g->mountname    = AllocMemR (i, 0, g);
	g->mountname[0] = i-1; 
	CopyMem(mountname+1, g->mountname+1, i-1);

	/* allocate messageport for notify */
	g->notifyport = CreateMsgPort();

	g->myproc   = (struct Process *)FindTask(NULL);
	g->msgport  = devnode->dn_Task;
	g->devnode  = devnode;
	g->startup  = fssm;
	g->dosenvec = (struct DosEnvec *)BADDR(fssm->fssm_Environ);
	g->currentvolume = NULL;

	/*
	 * Assign memory functions; pooled or not depending
	 * on exec version (V16: always used LibPool functions
	 */
	g->allocmemp = AllocPooledVec;
	g->freememp = FreePooledVec;
	g->allocbufmem = AllocPooledBuf;
	g->freebufmem = FreePooledBuf;

	if (!(g->mainPool = LibCreatePool (MEMF_CLEAR, 2*1024, 1*1024)))
	{
		ErrorMsg (AFS_ERROR_MEMORY_POOL, NULL, g);
		return FALSE;
	}

	g->bufferPool = LibCreatePool (g->dosenvec->de_BufMemType, 20*1024, 16*1024);
	if (g->bufferPool)
	{
#if 0
Removed because of problems with Phase 5 boards

		/* check if memory allocated is ok (First fail-over) */
		if (((ULONG)LibAllocPooled(g->bufferPool, 8)) & ~g->dosenvec->de_Mask)
		{
			g->dosenvec->de_BufMemType |= MEMF_24BITDMA;
			LibDeletePool (g->bufferPool);
			g->bufferPool = LibCreatePool (g->dosenvec->de_BufMemType, 20*1024, 16*1024);
		}
		/* check if memory allocated is ok (Second fail-over) */
		if (((ULONG)LibAllocPooled(g->bufferPool, 8)) & ~g->dosenvec->de_Mask)
		{
			g->dosenvec->de_BufMemType |= MEMF_LOCAL;
			LibDeletePool (g->bufferPool);
			g->bufferPool = LibCreatePool (g->dosenvec->de_BufMemType, 20*1024, 16*1024);
		}
#endif
	}

	if (!g->bufferPool)
	{
		ErrorMsg (AFS_ERROR_MEMORY_POOL, NULL, g);
		return FALSE;
	}

    g->blocksize = g->dosenvec->de_SizeBlock << 2;
	t = BLOCKSIZE;
	for (i=-1; t; i++)
		t >>= 1;
	g->blockshift = i;
	g->directsize = 16*1024>>i;

#if ACCESS_DETECT
	g->tdmode = ACCESS_UNDETECTED;
#else
#define DE(x) g->dosenvec->de_##x
	g->tdmode = ACCESS_STD;
	if ((DE(HighCyl)+1)*DE(BlocksPerTrack)*DE(Surfaces) >= (1UL << (32-BLOCKSHIFT))) {
#if TD64
		g->tdmode = ACCESS_TD64;
#elif NSD
		g->tdmode = ACCESS_NSD;
#elif SCSIDIRECT
		g->tdmode = ACCESS_DS;
#endif
	}
#undef DE
#endif

	if (!(g->geom = AllocMemP (sizeof(struct DriveGeometry), g)))
		return FALSE;

	NewList((struct List *)&g->volumes);    /* %9.1 */
	NewList((struct List *)&g->idlelist);

	if (!OpenDiskDevice(fssm, &g->port, &g->request, &g->trackdisk, g) )
		return FALSE;

	DB(Trace(4,"Init","result = %ld",(ULONG)g->trackdisk));

	if (!init_device_unit_sema(fssm, g))
		return FALSE;

	/* mode now always big */
	g->harddiskmode = TRUE;

	/* data cache */
	g->dc.size = DATACACHELEN;
	g->dc.mask = DATACACHEMASK;
	g->dc.roving = 0;
	g->dc.ref = AllocMemR (DATACACHELEN * sizeof(struct reftable), MEMF_CLEAR, g);
	g->dc.data = AllocMemR (DATACACHELEN * BLOCKSIZE, g->dosenvec->de_BufMemType, g);
	if (!g->dc.ref || !g->dc.data)
		return FALSE;

	/* check memory against mask */
	if (((IPTR)g->dc.data) & ~g->dosenvec->de_Mask)
		ErrorMsg (AFS_WARNING_MEMORY_MASK, NULL, g);

	if (!InitLRU(g))
		return FALSE;

	if (!OpenTimerDevice(&g->timeport, &g->trequest, UNIT_VBLANK, g) )
		return FALSE;

	g->removable = TestRemovability(g);
	if(g->removable)
		InstallDiskChangeHandler(g);

	InstallResetHandler(g);

#if !defined(__MORPHOS__) || defined(DISK_BASED_FILESYSTEM)
	AddToFSResource (g->dosenvec->de_DosType, ((struct DosList *)devnode)->dol_misc.dol_handler.dol_SegList, g);
#endif

#if EXTRAPACKETS
	g->sleepport = CreateMsgPort();
#endif

	DB(Trace(1,"Initialize","Removable = %ld", (ULONG)g->removable));
	EXIT("initialize");

	return TRUE;
}

/* added from HDVersion7.0B 11-4-94 */
static BOOL TestRemovability(globaldata *g)
{
#if 0

  struct DriveGeometry *geom = g->geom;
  struct IOExtTD *request = g->request;
  BOOL result;

	if(g->trackdisk)
	{
		request->iotd_Req.io_Data = g->geom;
		request->iotd_Req.io_Command = TD_GETGEOMETRY;
		request->iotd_Req.io_Length = sizeof(struct DriveGeometry);

		if(DoIO((struct IORequest *)request) == 0)
			result = geom->dg_Flags & DGF_REMOVABLE;
		else
			result = 1;     /* trackdisk assumed to be removable */
	}
	else
	{
	  struct DosEnvec *env = g->dosenvec;

		if( (env->de_Surfaces <= 2) && (env->de_LowCyl == 0) )
			result = 1;     /* to accomodate things like the diskspare.device */
		else
			result = 0;     /* disk is assumed to be NOT removable by default */
	}

	return(result);

#else
	return 1;           /* to accomodate SyQuest, Floptical etc */
#endif
}

/* added from HDVersion7.0B 11-4-94 */
static void InstallDiskChangeHandler(struct globaldata *g)
{
  struct IOExtTD *request;
  struct Interrupt *di;
  UBYTE *intname;
  static const UBYTE *intext = "_interrupt";

	intname = AllocMemR (g->mountname[0]+strlen(intext)+1, MEMF_PUBLIC, g);
	CopyMem(g->mountname+1,intname,g->mountname[0]);
	CopyMem((APTR)intext,intname+g->mountname[0],strlen(intext)+1);
	DB(Trace(1,"Installdiskchangehandler","intname: %s\n",intname));

	di = g->diskinterrupt = AllocMemR (sizeof(struct Interrupt), MEMF_PUBLIC|MEMF_CLEAR, g);
	di->is_Node.ln_Type = NT_INTERRUPT;
	di->is_Node.ln_Name = intname;
	di->is_Data = (APTR)g;
	di->is_Code = (VOID_FUNC)DiskChangeHandler;

	g->diskchangesigbit = AllocSignal(-1);
	g->diskchangesignal = 1 << g->diskchangesigbit;

	/* make special request structure for inthandler */
	request = g->handlrequest = (struct IOExtTD *)
					AllocMemR (sizeof(struct IOExtTD), MEMF_PUBLIC, g);
	CopyMem(g->request, request, sizeof(*request));

	request->iotd_Req.io_Length  = sizeof(struct Interrupt);
	request->iotd_Req.io_Data    = (APTR)di;
	request->iotd_Req.io_Command = TD_ADDCHANGEINT;
	SendIO((struct IORequest *)request);
}

void UninstallDiskChangeHandler(struct globaldata *g)
{
	struct IOExtTD *request = g->handlrequest;

    if (!g->diskinterrupt)
    	return;
	request->iotd_Req.io_Length  = sizeof(struct Interrupt);
	request->iotd_Req.io_Data    = (APTR)g->diskinterrupt;
	request->iotd_Req.io_Command = TD_REMCHANGEINT;
	DoIO ((struct IORequest *)request);
	FreeVec (request);
	FreeVec (g->diskinterrupt->is_Node.ln_Name);
	FreeVec (g->diskinterrupt);
	FreeSignal (g->diskchangesigbit);
}

static void InstallResetHandler(struct globaldata *g)
{
	static const char handlername[] = "_resethandler";
	struct Interrupt *ri;
	ri = AllocMemR( sizeof(*ri) + g->mountname[0] + sizeof(handlername), MEMF_PUBLIC, g);
	if (ri)
	{
		LONG sigbit = AllocSignal(-1);
		if (sigbit != -1)
		{
			ri->is_Node.ln_Type = NT_INTERRUPT;
			ri->is_Node.ln_Pri  = -32; /* Lowest possible priority, after everything else */
			ri->is_Node.ln_Name = (char *) (ri + 1);
			ri->is_Data = (APTR) g;
			ri->is_Code = (void (*)(void)) ResetHandler;
			CopyMem(g->mountname + 1, ri->is_Node.ln_Name, g->mountname[0]);
			CopyMem((APTR)handlername, ri->is_Node.ln_Name + g->mountname[0], sizeof(handlername));
#if defined(__MORPHOS__) && defined(SYSTEM_PRIVATE)
			AddResetHandler(ri);
			g->resethandlerinterrupt = ri;
			g->resethandlersigbit    = sigbit;
			g->resethandlersignal    = 1L << sigbit;
			ri = 0;
			sigbit = -1;
#else
			{
			struct MsgPort mp;
			struct IOStdReq *ioreq;
			mp.mp_Node.ln_Type = NT_MSGPORT;
			mp.mp_Flags = PA_SIGNAL;
			mp.mp_SigBit = sigbit;
			mp.mp_SigTask = FindTask(0);
			NewList(&mp.mp_MsgList);

			ioreq = CreateIORequest(&mp, sizeof(*ioreq));
			if (ioreq)
			{
				if (OpenDevice("keyboard.device", 0, (APTR) ioreq, 0) == 0)
				{
					ioreq->io_Command = KBD_ADDRESETHANDLER;
					ioreq->io_Data    = ri;
					ioreq->io_Length  = sizeof(struct Interrupt);
					if (DoIO((APTR) ioreq) == 0)
					{
						g->resethandlerioreq     = ioreq;
						g->resethandlerinterrupt = ri;
						g->resethandlersigbit    = sigbit;
						g->resethandlersignal    = 1L << sigbit;
						SetSignal(0, g->resethandlersignal);
						ioreq->io_Message.mn_ReplyPort = 0;
						ioreq = 0;
						ri = 0;
						sigbit = -1;
					}
				}
				DeleteIORequest(ioreq);
			}
			}
#endif
			FreeSignal(sigbit);
		}
		FreeVec(ri);
	}
}

void HandshakeResetHandler(struct globaldata *g)
{
	struct Interrupt *ri = g->resethandlerinterrupt;
	if (ri)
	{
#if defined(__MORPHOS__) && defined(SYSTEM_PRIVATE)
		FinishResetHandler(ri);
#else
		struct IOStdReq *ioreq = g->resethandlerioreq;
		struct MsgPort mp;

		mp.mp_Node.ln_Type = NT_MSGPORT;
		mp.mp_Flags = PA_SIGNAL;
		mp.mp_SigBit = g->resethandlersigbit;
		mp.mp_SigTask = FindTask(0);
		NewList(&mp.mp_MsgList);

		ioreq->io_Message.mn_ReplyPort = &mp;
		ioreq->io_Command = KBD_RESETHANDLERDONE;
		ioreq->io_Data    = ri;
		ioreq->io_Length  = sizeof(struct Interrupt);
		DoIO((APTR) ioreq);

		ioreq->io_Message.mn_ReplyPort = 0;
		SetSignal(0, g->resethandlersignal);
#endif
	}
}

void UninstallResetHandler(struct globaldata *g)
{
	struct Interrupt *ri = g->resethandlerinterrupt;
	g->resethandlerinterrupt = 0;
	if (ri)
	{
#if defined(__MORPHOS__) && defined(SYSTEM_PRIVATE)
		RemResetHandler(ri);
#else
		struct IOStdReq *ioreq = g->resethandlerioreq;
		struct MsgPort mp;
		mp.mp_Node.ln_Type = NT_MSGPORT;
		mp.mp_Flags = PA_SIGNAL;
		mp.mp_SigBit = g->resethandlersigbit;
		mp.mp_SigTask = FindTask(0);
		NewList(&mp.mp_MsgList);

		ioreq->io_Message.mn_ReplyPort = &mp;
		ioreq->io_Command = KBD_REMRESETHANDLER;
		ioreq->io_Data    = ri;
		ioreq->io_Length  = sizeof(struct Interrupt);
		DoIO((APTR) ioreq);
		CloseDevice((APTR) ioreq);
		DeleteIORequest((APTR) ioreq);
#endif
		FreeVec(ri);
		FreeSignal(g->resethandlersigbit);
	}
}

#endif

/* OpenDiskDevice
**
** Used by L2.InitUnit()
** 
** Opens the diskdevice specified by 'startup' and generates the corresponding
** messageport and requeststructure which are returned in 'port' and 'request'
**
** in startup
** out port, request, trackdisk
*/
static BOOL OpenDiskDevice(struct FileSysStartupMsg *startup, struct MsgPort **port,
		struct IOExtTD **request, BOOL *trackdisk, globaldata *g)
{
  UBYTE name[FNSIZE];

	*port = CreateMsgPort();
	if(*port)
	{
		*request = (struct IOExtTD *)CreateIORequest(*port, sizeof(struct IOExtTD));
		if(*request)
		{
			BCPLtoCString(name, (UBYTE *)BADDR(startup->fssm_Device));
			*trackdisk = (strcmp(name, "trackdisk.device") == 0) || (strcmp(name, "diskspare.device") == 0);

#if KS13WRAPPER_DEBUG
	DebugPutStr("Opening device..\n");
	DebugPutStr(name);
	DebugPutStr("\n");
#endif

			if(OpenDevice(name, startup->fssm_Unit, (struct IORequest *)*request,
				startup->fssm_Flags) == 0)
				return TRUE;
			DeleteIORequest(*request);
			*request = 0;
		}
		DeleteMsgPort(*port);
		*port = 0;
	}
	return FALSE;
}

static BOOL init_device_unit_sema(struct FileSysStartupMsg *startup, globaldata *g)
{
	struct {
		struct SignalSemaphore ss;
		char   name[1];
	} *sema;
	sema = AllocVec(sizeof(*sema) + 4 + ((UBYTE *)BADDR(startup->fssm_Device))[0] + 14, MEMF_PUBLIC);
	if (sema)
	{
		strcpy(sema->name, "PFS_");
		BCPLtoCString(&sema->name[4], (UBYTE *)BADDR(startup->fssm_Device));
		strcat(sema->name, "/");
		stcu_d(&sema->name[strlen(sema->name)], startup->fssm_Unit);

		Forbid();
		g->device_unit_lock_sema = FindSemaphore(sema->name);
		if (g->device_unit_lock_sema)
		{
			FreeVec(sema);
		}
		else
		{
			memset(&sema->ss, 0, sizeof(sema->ss));
			InitSemaphore(&sema->ss);
			sema->ss.ss_Link.ln_Name = sema->name;
			sema->ss.ss_Link.ln_Pri  = -128;
			sema->ss.ss_Link.ln_Type = NT_SIGNALSEM;
			if (g->g_SysBase->LibNode.lib_Version < 36) {
				Enqueue(&g->g_SysBase->SemaphoreList, &sema->ss);
			}  else {
				AddSemaphore(&sema->ss);
			}
			g->device_unit_lock_sema = &sema->ss;
		}
		Permit();
		g->device_unit_lock_count = 0;
		return TRUE;
	}
	return FALSE;
}

void lock_device_unit(struct globaldata *g)
{
	if (g->device_unit_lock_count++ == 0)
		ObtainSemaphore(g->device_unit_lock_sema);
}

void unlock_device_unit(struct globaldata *g)
{
	if (g->device_unit_lock_count)
	{
		if (--g->device_unit_lock_count == 0)
			ReleaseSemaphore(g->device_unit_lock_sema);
	}
}


static BOOL OpenTimerDevice(struct MsgPort **port, struct timerequest **request,
	ULONG unit, globaldata *g)
{
	*port = CreateMsgPort();
	if(*port)
	{
		*request = (struct timerequest *)
			CreateIORequest(*port, sizeof(struct timerequest));
		if(*request)
		{
			if(!OpenDevice(TIMERNAME, unit, (struct IORequest *)*request, 0))
				return(TRUE);
		}
	}
	return(FALSE);
}

#if !defined(__MORPHOS__) || defined(DISK_BASED_FILESYSTEM)
/* AddToFSResource
**
** function supplied by Nicola Salmoria
*/
static BOOL AddToFSResource(ULONG dostype, BPTR seglist, struct globaldata *g)
{
  struct FileSysResource *FileSysResBase;

	FileSysResBase = (struct FileSysResource *)OpenResource(FSRNAME);
	if (FileSysResBase)
	{
	  struct FileSysEntry *fse,*nfse;

		Forbid();

		fse = (struct FileSysEntry *)FileSysResBase->fsr_FileSysEntries.lh_Head;
		while ((nfse = (struct FileSysEntry *)fse->fse_Node.ln_Succ))
		{
			/* if filesystem already in resource, return */
			if (fse->fse_DosType == dostype)
			{
				DB(Trace(4,"ADDTORESOURCE","Already there\n"));    
				break;
			}

			fse = nfse;
		}

		if (!nfse && (fse = AllocMem(sizeof(struct FileSysEntry), MEMF_PUBLIC | MEMF_CLEAR)))
		{
			fse->fse_Node.ln_Name = (UBYTE *)&version[6];
			fse->fse_DosType = dostype;
			fse->fse_Version = ((LONG)VERNUM) << 16 | REVNUM;
			fse->fse_PatchFlags = 0x180;
			fse->fse_SegList = seglist;
			fse->fse_GlobalVec = (BPTR)(SIPTR)-1;

			AddHead(&FileSysResBase->fsr_FileSysEntries,&fse->fse_Node);
		}

		Permit();
	}

	return TRUE;
}
#endif


/* Reconfigure the filesystem from a rootblock
** GetDriveGeometry already called by GetCurrentRoot, which does
** g->firstblock and g->lastblock.
** rootblockextension must have been loaded
*/
void InitModules (struct volumedata *volume, BOOL formatting, globaldata *g)
{
  rootblock_t *rootblock = volume->rootblk;

	g->rootblock = rootblock;
	g->uip = 0;
	g->harddiskmode = rootblock->options & MODE_HARDDISK;
	g->anodesplitmode = rootblock->options & MODE_SPLITTED_ANODES;
	g->dirextension = rootblock->options & MODE_DIR_EXTENSION;
#if DELDIR
	g->deldirenabled = (rootblock->options & MODE_DELDIR) && 
		g->dirextension && (volume->rblkextension->blk.deldirsize > 0);
#endif
	g->supermode = rootblock->options & MODE_SUPERINDEX;
	g->fnsize = (volume->rblkextension) ? (volume->rblkextension->blk.fnsize) : 32;
	if (!g->fnsize) g->fnsize = 32;

	InitAnodes (volume, formatting, g);
	InitAllocation (volume, g);

#if VERSION23
	if (!formatting)
		DoPostponed (volume, g);
#endif
}


#if VERSION23
static void DoPostponed (struct volumedata *volume, globaldata *g)
{
  struct crootblockextension *rext;
  struct anodechain *achain;
  struct postponed_op *postponed;

	rext = volume->rblkextension;
	if (rext)
	{
		postponed = &rext->blk.tobedone;

		switch (postponed->operation_id)
		{
			case PP_FREEBLOCKS_FREE:

				/* we assume we have enough memory at startup.. */
				achain = GetAnodeChain (postponed->argument1, g);
				FreeBlocksAC (achain, postponed->argument2, freeanodes, g);
				break;

			case PP_FREEBLOCKS_KEEP:

				/* we assume we have enough memory at startup.. */
				achain = GetAnodeChain (postponed->argument1, g);
				alloc_data.clean_blocksfree -= postponed->argument3;
				alloc_data.alloc_available -= postponed->argument3;
				FreeBlocksAC (achain, postponed->argument2, keepanodes, g);
				break;

			case PP_FREEANODECHAIN:

				FreeAnodesInChain (postponed->argument1, g);
				break;
		}

		postponed->operation_id = 0;
		postponed->argument1 = 0;
		postponed->argument2 = 0;
		postponed->argument3 = 0;
	}
}
#endif

