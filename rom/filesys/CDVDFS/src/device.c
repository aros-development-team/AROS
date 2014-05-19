/* device.c:
 *
 * Handler for ISO-9660 (+ Rock Ridge) + HFS CDROM filing system.
 * Based on DOSDEV V1.10 (2 Nov 87) by Matthew Dillon.
 *
 * ----------------------------------------------------------------------
 * This code is (C) Copyright 1993,1994 by Frank Munkert.
 *              (C) Copyright 2002-2013 The AROS Development Team
 * All rights reserved.
 * This software may be freely distributed and redistributed for
 * non-commercial purposes, provided this notice is included.
 * ----------------------------------------------------------------------
 * History:
 *
 * 08-Nov-13 neil      - Only delay 2 seconds upon exiting if debugging is
 *                       enabled.
 * 18-Dec-11 twilen    - Added media change interrupt support (SCANINTERVAL=-1).
 * 11-Aug-10 sonic     - Fixed for 64-bit compatibility
 * 04-Jun-10 neil      - No longer removes device node and seglist when
 *                       exiting. It isn't our job.
 *                     - Delay returning ACTION_DIE packet until handler is
 *                       mostly closed down.
 * 20-Mar-09 sonic     - Fixed ID_CDFS_DISK redefinition warning when building with new
 *                       MorphOS SDK
 * 18-Mar-09 sonic     - Numerous BSTR handling adjustments, now can be compiled as a
 *			 packet-type handler for AROS
 * 27-Aug-07 sonic     - Register_Volume_Node() now takes separate pointer to a volume name.
 *                     - Now reports correct DOS type for volumes ('CDFS').
 * 29-May-07 sonic     - Replies FileSysStartupMsg and loads character
 *                       set translation table before mounting volume
 *                     - Does not unmount volume any more in case of
 *                       disk read error
 * 15-May-07 sonic     - Fixed CDDA handling
 * 06-May-07 sonic     - Added support for protection bits and file comments
 *                     - Fixed memory trashing when file name length is greater than AmigaOS
 *                       can accept
 * 08-Apr-07 sonic     - removed redundant "TRACKDISK" option
 * 01-Apr-07 sonic     - seglist unloading is much more safe
 *		       - removed unneeded search in Remove_Volume_Node()
 * 31-Mar-07 sonic     - ACTION_SAME_LOCK now returns 100% correct values
 *		       - removed Make_FSSM()
 *		       - removed many #ifdef's
 *                     - fixed warnings
 * 09-Apr-06 sonic     - fixed binary-compatible BCPL file name string handling
 *                     - code style improved, removed many #ifdef's.
 *                     - now works if compiled for binary-compatible flavour
 * 07-Jul-02 sheutlin  various changes/fixes when porting to AROS
 *                     - all global variables are now in the structure global
 *                     - changed types in lowercase to uppercase (ulong, ...)
 *                     - replaced BTOC() by dos/dos.h/BADDR()
 *                     - replaced CTOB() by dos/dos.h/MKBADDR()
 *                     - fixed various warnings
 *                     - replaced (unsigned) long by (U)LONG
 *                     - replaced short by WORD
 *                     - removed GetHead(), NextNode() (was unused)
 *                     - changed Create_Volume_Node() to use
 *                       MakeDosEntry()/AddDosEntry()
 *                     - changed Remove_Volume_Node() to use
 *                       RemDosEntry()/FreeDosEntry() now
 *                     - removed dosalloc()/dosfree() and replaced these calls
 *                       by AllocVec()/FreeVec()
 *                     - put packet handling into own function to split
 *                       code into init-main-deinit
 *                     - replaced memcpy by CopyMem
 *                     - changed SAME_LOCK returning LOCK_SAME/LOCK_DIFFERENT
 *                     - replaced DOS_[TRUE|FALSE] by dos/dos.h/DOS[TRUE|FALSE]
 *                     - optimisations here and there
 *                     - removed static definitions
 * 03-Nov-94   fmu   Truncate file names to 30 characters.
 * 12-Oct-94   fmu   Return startup packet even if the scsi device cannot
 *                   be opened immediately.
 * 01-Sep-94   fmu   - Added ACTION_EXAMINE_FH.
 *		     - Added ACTION_PARENT_FH.
 *		     - Added ACTION_MAKE_LINK (-> error message).
 *		     - Automatically remove volume if all locks on the
 *		       the volume have been freed.
 * 17-May-94   fmu   New option MAYBELOWERCASE (=ML).
 * 20-Apr-94   fmu   Improved implementation of ACTION_INHIBIT.
 * 17-Apr-94   fmu   Fixed bug concerning TRACKDISK disk change recognition.
 * 12-Apr-94   fmu   Adapted ACTION_CURRENT_VOLUME to new filehandle
 *		     management.
 * 09-Apr-94   fmu   Volume management: locks and filehandles will not
 *                   be forgotten if a CDROM is removed from the drive.
 * 06-Feb-94   dmb   - Full support for ACTION_INHIBIT
 *                   - Change Check_Disk() for trackdisk support
 * 05-Jan-93   fmu   - Retry displaying CD-DA icon if WB is not open.
 *                   - id_UnitNumber of InfoData set to SCSI unit number.
 *                   - Added Make_FSSM().
 * 01-Jan-93   fmu   Support for symbolic links on RockRidge disks.
 * 11-Dec-93   fmu   - ACTION_FLUSH always returns DOSTRUE.
 *                   - ISO volume names are mapped to lowercase if
 *                     the option LOWERCASE has been selected.
 * 26-Nov-93   fmu   Some packets are now handled even if no disk
 *                   is inserted.
 * 21-Nov-93   fmu   - User programmable diskchange check interval.
 *                   - Better support for ACTION_INHIBIT.
 *                   - Handles filenames with ';'.
 * 15-Nov-93   fmu   Missing return value for 'handler' inserted.
 * 14-Nov-93   fmu   Added ACTION_USER packet for 'cdcontrol' program.
 * 15-Oct-93   fmu   Adapted to new VOLUME structure.
 * 10-Oct-93   fmu   - Creates volume node for 'no DOS' disks.
 *		     - Volume node now contains the correct volume
 *		       creation date.
 * 09-Oct-93   fmu   - New format for mountlist startup field.
 *		     - SAS/C support.
 *		     - Debug process assembly tag adapted to small
 *		       memory model.
 *		     - Get_Startup moved to file devsupp.c.
 * 03-Oct-93   fmu   - New buffering options 'S' and 'C'.
 *                   - Fixed bug in cdlock.
 *		     - Fixed bug in ACTION_CURRENT_VOLUME.
 * 27-Sep-93   fmu   Added ACTION_SAME_LOCK
 * 25-Sep-93   fmu   - Send 'disk inserted' / 'disk removed' event via
 *                     input.device if disk has been changed.
 *                   - Corrected bug in ACTION_DISK_INFO.
 * 24-Sep-93   fmu   - Added fast memory option 'F'.
 *                   - Added ACTION_IS_FILESYSTEM.
 *                   - Added 'write protected' error for write actions.
 *                   - Added ACTION_CURRENT_VOLUME.
 *                   - Unload handler code after ACTION_DIE.
 *                   - Immediately terminate program if called from CLI.
 *                   - Added library version number.
 *                   - Set volume label to "Unnamed" for disks without name.
 * 16-Sep-93   fmu   Added code to detect whether a lock stems from the
 *                   current volume or from another volume which has
 *                   been removed from the drive.
 */

/*
 *  Debugging routines are disabled by simply attempting to open the
 *  file "debugoff", turned on again with "debugon".  No prefix may be
 *  attached to these names (you must be CD'd to TEST:).
 *
 *  See Documentation for a detailed discussion.
 */

#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/utility.h>
#include <stdlib.h>
#include <string.h>

#include "debug.h"
#include "device.h"
#include "intui.h"
#include "devsupp.h"
#include "cdcontrol.h"
#include "params.h"
#include "rock.h"
#include "globals.h"
#include "volumes.h"
#include "charset.h"
#include "prefs.h"
#include "aros_stuff.h"

#ifndef ID_CDFS_DISK
#define ID_CDFS_DISK 0x43444653
#endif

LONG handler(struct ExecBase *);
static LOCK *cdlock(CDROM_OBJ *, int);
static void cdunlock (LOCK *);
static CDROM_OBJ *getlockfile(struct CDVDBase *global, IPTR);
static void returnpacket(struct CDVDBase *global, struct DosPacket *);
static int packetsqueued (struct CDVDBase *global);
static int Check_For_Volume_Name_Prefix (struct CDVDBase *global, char *);
static void Fill_FileInfoBlock (struct CDVDBase *global, FIB *, CDROM_INFO *, VOLUME *);
static void Mount (struct CDVDBase *global);
static void Unmount (struct CDVDBase *global);
static int Mount_Check (struct CDVDBase *global);
static void Check_Disk (struct CDVDBase *global);
static void Send_Timer_Request (struct CDVDBase *global);
static void Cleanup_Timer_Device (struct CDVDBase *global);
static int Open_Timer_Device (struct CDVDBase *global);
static void Send_Event (struct CDVDBase *global, int);
/* BPTR Make_FSSM (void); */
static void Remove_Volume_Node (struct CDVDBase *global, struct DeviceList *);
static LONG handlemessage(struct CDVDBase *global, ULONG);

#ifdef USE_FAST_BSTR
#define MAX_NAME_LEN    107
#define MAX_COMMENT_LEN 79
#define btos(bstr, buf) strcpy(buf, bstr)
#else
#define MAX_NAME_LEN 106
#define MAX_COMMENT_LEN 78

static void btos(BSTR, char *);
#endif

#ifdef __MORPHOS__
ULONG __abox__ = 1;
#endif

#ifndef __AROS__
char __version__[] = "\0$VER: CDVDFS 1.8 (8.11.2013)";

LONG SAVEDS Main(void)
{
    return handler(*(struct ExecBase **)4L);
}
#endif

static struct CDVDBase *AllocCDVDBase(struct ExecBase *SysBase)
{
    struct CDVDBase *cdvd = NULL;

    cdvd = AllocMem(sizeof(*cdvd), MEMF_CLEAR | MEMF_PUBLIC);
    if (!cdvd)
        return NULL;

    cdvd->g_iconname = "CD-DA";

    return cdvd;
}

static void FreeCDVDBase(struct CDVDBase *cdvd)
{
    FreeMem(cdvd, sizeof(*cdvd));
}

LONG handler(struct ExecBase *SysBase)
{
register PACKET *packet;
MSG     *msg;
ULONG signals;

    struct CDVDBase *global = AllocCDVDBase(SysBase);

    D(bug("[CDVDFS] In handler, cdvd=%p\n", global));
    global->playing = FALSE;

    /*
     *	Initialize all global variables. SysBase MUST be initialized before
     *  we can make Exec calls. The DOS library is opened for the debug
     *  process only.
     */
    global->DosProc = (PROC *) FindTask(NULL);
    DOSBase = (APTR)OpenLibrary("dos.library",37);
    UtilityBase = (APTR)OpenLibrary("utility.library",37);
    CodesetsBase = NULL;

    global->Dback = CreateMsgPort();

#if !defined(NDEBUG) || defined(DEBUG_SECTORS)
    D(bug("[CDVDFS] Initializing debugging code\n"));
    global->DBDisable = 0;				  /*  Init. globals	  */
    global->Dbport = NULL;

    /*
     *	Initialize debugging code
     */

    if (DOSBase && global->Dback)
	dbinit(global);
#endif
    global->DevList = NULL;
    global->PrefsProc = NULL;
    global->uniCodeset = NULL;

    WaitPort(&global->DosProc->pr_MsgPort);         /*  Get Startup Packet  */
    msg = GetMsg(&global->DosProc->pr_MsgPort);
    packet = (PACKET *)msg->mn_Node.ln_Name;

    /*
     *  Loading DosNode->dn_Task causes DOS *NOT* to startup a new
     *  instance of the device driver for every reference.  E.G. if
     *  you were writing a CON device you would want this field to
     *  be NULL.
     */

    global->DosNode = (DEVNODE *)BADDR(packet->dp_Arg3);

    D(bug("[CDVDFS] Fixing up DosNode %p\n", global->DosNode));
    /*
     *  Set dn_Task field which tells DOS not to startup a new
     *  process on every reference.
     */
    global->DosNode->dn_Task = &global->DosProc->pr_MsgPort;

    Init_Intui (global);

    if (UtilityBase && DOSBase && global->Dback &&
			Get_Startup (global, (struct FileSysStartupMsg *)(BADDR(packet->dp_Arg2))))
    {
	D(bug("[CDVDFS] On track, globals at %p\n", global));
	packet->dp_Res1 = DOSTRUE;
 	packet->dp_Res2 = 0;

	/*
	 *  Load dn_Startup field with a BPTR to a FileSysStartupMsg.
	 *  (According to the official documentation, this is not
	 *  required. Some debugging tools, however, depend on it.)
	 */
	    
/*	global->DosNode->dn_Startup = Make_FSSM (global); */

    } else {		                /*  couldn't open dos.library   */
	D(bug("[CDVDFS] Off track, globals at %p\n", global));
	D(Alert(0));
	packet->dp_Res1 = DOSFALSE;
	packet->dp_Res2 = 333; /* any error code */
	global->DosNode->dn_Task = BNULL;
	returnpacket(global, packet);
	Forbid ();
	if (DOSBase) {
	    BUG2(dbuninit(global);)
	    Close_Intui (global);
	    if (global->Dback)
		DeleteMsgPort(global->Dback);
	    if (UtilityBase)
		CloseLibrary((struct Library *)UtilityBase);
	    CloseLibrary ((struct Library *)DOSBase);
	}
	return 0;		        /*  exit process	        */
    }

    global->g_inhibited = 0;

    BUG2(dbprintf (global, "g_cd = %08lx\n", global->g_cd);)
    BUG(dbprintf(global, "%d std buffers, %d file buffers\n",
    		 global->g_std_buffers, global->g_file_buffers);)

    global->g_changeint_signumber = AllocSignal(-1);
    global->g_changeint_sigbit = 1L << global->g_changeint_signumber;
    /* Initialize timer: */
    global->g_timer_sigbit = 0;
    if (Open_Timer_Device (global)) {
      Send_Timer_Request (global);
      global->g_scan_time = global->g_scan_interval;
      global->g_time = 0;
    }

    global->g_vol_name = AllocVec(128, MEMF_PUBLIC | MEMF_CLEAR);
    global->g_dos_sigbit = 1L << global->DosProc->pr_MsgPort.mp_SigBit;
    returnpacket(global, packet);
    Prefs_Init(global);
    global->g_disk_inserted = 0;

    if (global->g_cd) {
      /* Mount volume (if any disk is inserted): */
      Mount_Check (global);
    }

    /*
     *	Here begins the endless loop, waiting for requests over our
     *	message port and executing them.  Since requests are sent over
     *	our message port, this precludes being able to call DOS functions
     *	ourselves (that is why the debugging routines are a separate process)
     */
	do
	{
		signals = Wait(global->g_dos_sigbit | global->g_timer_sigbit | global->g_app_sigbit | global->g_changeint_sigbit);
	} while	(handlemessage(global, signals));
    BUG(dbprintf(global, "Terminating the handler\n");)
    /* remove timer device and any pending timer requests: */
    if (global->g_timer_sigbit)
      Cleanup_Timer_Device (global);

    if (global->g_cd)
      Unmount (global);
    
    FreeVec(global->g_vol_name);

    if (global->g_cd)
      Cleanup_CDROM (global->g_cd);

    if (global->g_changeint_signumber != -1)
      FreeSignal(global->g_changeint_signumber);

    Close_Intui (global);

    /*
     * Delay returning a successful ACTION_DIE packet until now, to avoid
     * inconveniences such as having the trackdisk device disappear before
     * we close it (e.g. when using a USB drive).
     */
    returnpacket(global, global->g_death_packet);

    /*
     *	Remove processes, closedown, fall off the end of the world
     *	(which is how you kill yourself if a PROCESS.  A TASK would have
     *	had to RemTask(NULL) itself).
     */
    Prefs_Uninit(global);
    BUG2(Delay (50);)
    BUG2(dbuninit(global);)
    DeleteMsgPort(global->Dback);

    if (CodesetsBase)
	CloseLibrary(CodesetsBase);
    CloseLibrary((struct Library *)UtilityBase);
    CloseLibrary((struct Library *)DOSBase);

    FreeCDVDBase(global);
    return 0;
}

LONG handlemessage(struct CDVDBase *global, ULONG signals) {
register PACKET *packet;
MSG     *msg;
void    *tmp;
char    buf[256];
register WORD   error;
UBYTE   notdone = 1;
 
	if (signals & global->g_changeint_sigbit)
	{
		if (global->g_cd && !global->g_inhibited)
			Check_Disk (global);
	}
	if (signals & global->g_timer_sigbit)
	{
		GetMsg (global->g_timer_mp);
		global->g_time++;
		/* retry opening the SCSI device (every 2 seconds): */
		if (!global->g_cd && (global->g_time & 1))
		{
			BUG(dbprintf(global, "Device is closed, attempting to open\n");)
			Get_Startup(global, (struct FileSysStartupMsg *)-1);
		}
		if (global->g_cd)
		{
			/* icon retry (every second): */
			if (global->g_retry_show_cdda_icon)
			{
				BUG(dbprintf(global, "Showing CDDA icon\n");)
				Show_CDDA_Icon (global);
			}
			/* diskchange check: */
			if (global->g_scan_interval > 0)
			{
				if (global->g_scan_time == 1)
				{
					if (!global->g_inhibited)
						Check_Disk (global);
					global->g_scan_time = global->g_scan_interval;
				}
				else
					global->g_scan_time--;
			}
		}
		Send_Timer_Request (global);
	}
	if (signals & global->g_app_sigbit)
	{
	struct Message *msg;
		BUG(dbprintf(global, "CDDA icon double-clicked\n");)
		while ((msg = GetMsg (global->g_app_port)))
		{
			struct TagItem PlayTags[] = {
			    {SYS_Input , 0   },
			    {SYS_Output, 0   },
			    {SYS_Asynch, TRUE},
			    {TAG_END   , 0   }
			};

			ReplyMsg (msg);
			if (global->g_play_cdda_command[0]) {
				PlayTags[0].ti_Data = (IPTR)Open ("NIL:", MODE_OLDFILE);
				System(global->g_play_cdda_command, PlayTags);
			}
			else
			{
			int res;
				if (global->playing)
					res = Stop_Play_Audio (global->g_cd);
				else
					res = Start_Play_Audio (global->g_cd);
				if (!res)
					Display_Error ("Cannot perform play audio command!");
				else
					global->playing = !global->playing;
			}
		}
	}
	if (!(signals & global->g_dos_sigbit))
		return TRUE;
	while ((msg = GetMsg(&global->DosProc->pr_MsgPort)))
	{
		packet = (PACKET *)msg->mn_Node.ln_Name;
		packet->dp_Res1 = DOSTRUE;
		packet->dp_Res2 = 0;
		error = 0;
		tmp = NULL;
		BUG(dbprintf(global, 
			"[CDVDFS]\tPacket: %3ld %08lx %08lx %08lx %10s ",
			packet->dp_Type,
			packet->dp_Arg1,
			packet->dp_Arg2,
			packet->dp_Arg3,
			typetostr(packet->dp_Type)
		);)
		if (!global->g_cd)
		{
			switch (packet->dp_Type)
			{
			/* packets we will handle even if the SCSI device is not yet open: */
			case ACTION_DIE:
				break;
			/* packets we cannot handle because the SCSI device is not yet open: */
			default:
				packet->dp_Res1 = DOSFALSE;
				/*
					some return codes cause the WB to pop a requester; we don't
					want this, though. Therefore we use this "exotic" return code:
				*/ 
				packet->dp_Res2 = ERROR_INVALID_COMPONENT_NAME;
				BUG(dbprintf(global, "ERR=%ld\n", (LONG) packet->dp_Res2);)
				returnpacket(global, packet);
				return TRUE;
			}
		}
		else if (global->g_inhibited)
		{
			switch (packet->dp_Type)
			{
			/* packets we will handle even if the handler is inhibited: */
			case ACTION_DIE:
			case ACTION_INHIBIT:
			case ACTION_MORE_CACHE:
			case ACTION_DISK_INFO:
				break;
			/* packets we cannot handle because the handler is inhibited: */
			default:
				packet->dp_Res1 = DOSFALSE;
				packet->dp_Res2 = ERROR_NOT_A_DOS_DISK;
				BUG(dbprintf(global, "ERR=%ld\n", (LONG) packet->dp_Res2);)
				returnpacket(global, packet);
				return TRUE;
			}
		}
		else if (global->DevList == NULL)
		{
			switch (packet->dp_Type)
			{
			/* packets we will handle even if no disk is inserted: */
			case ACTION_DIE:
			case ACTION_USER:
			case ACTION_IS_FILESYSTEM:
			case ACTION_INHIBIT:
			case ACTION_MORE_CACHE:
			case ACTION_DISK_INFO:
			case ACTION_FLUSH:
			case ACTION_FREE_LOCK:
				break;
			/* packets we cannot handle because no disk is inserted: */
			default:
				packet->dp_Res1 = DOSFALSE;
				packet->dp_Res2 =
					(global->g_disk_inserted ?
						ERROR_NOT_A_DOS_DISK :
			   		ERROR_NO_DISK
					);
				BUG(dbprintf(global, "ERR=%ld\n", (LONG) packet->dp_Res2);)
				returnpacket(global, packet);
				return TRUE;
			}
		}

		switch(packet->dp_Type)
		{
		case ACTION_DIE:  /* attempt to die? */
			notdone = 0;   /* try to die */
			break;
		case ACTION_USER: /* Mode,Par1,Par2    Bool */
			error = Handle_Control_Packet
				(
					global,
					packet->dp_Arg1,
					packet->dp_Arg2,
					packet->dp_Arg3
				);
			break;
		case ACTION_FINDINPUT: /* FileHandle,Lock,Name   Bool */
		{
			if (!(error = Mount_Check (global)))
			{
				CDROM_OBJ *obj;
				CDROM_OBJ *parentdir = getlockfile(global, packet->dp_Arg2);
				int       offs;

				if (parentdir->volume != global->g_volume)
				{
					/* old lock from another disk: */
					error = ERROR_DEVICE_NOT_MOUNTED;
					goto openbreak;
				}
				btos((BSTR)packet->dp_Arg3,buf);
				BUG(dbprintf(global, "'%s' ", buf);)
				offs = Check_For_Volume_Name_Prefix (global, buf);
				if ((obj = Open_Object (parentdir, buf + offs)))
				{
					if (obj->symlink_f)
					{
						error = ERROR_IS_SOFT_LINK;
						goto openbreak;
					}
					if (obj->directory_f)
					{
						error = ERROR_OBJECT_WRONG_TYPE;
						goto openbreak;
					}
				}
				else
				{
					if (global->iso_errno == ISOERR_ILLEGAL_NAME)
					{
						error = ERROR_INVALID_COMPONENT_NAME;
						goto openbreak;
					}
					else if (global->iso_errno == ISOERR_NOT_FOUND)
						error = ERROR_OBJECT_NOT_FOUND;
					else if (global->iso_errno == ISOERR_NO_MEMORY)
					{
						error = ERROR_NO_FREE_STORE;
						goto openbreak;
					}
					else if (global->iso_errno == ISOERR_IS_SYMLINK)
					{
						error = ERROR_IS_SOFT_LINK;
						goto openbreak;
					}
					else
					{
						error = 333;
						goto openbreak;
					}
				}
				if (!error)
				{
					global->g_volume->file_handles++;
					((FH *)BADDR(packet->dp_Arg1))->fh_Arg1 = (IPTR)obj;
					Register_File_Handle (obj);
				}
			}
		}
openbreak:
		break;
		case ACTION_READ: /* FHArg1,CPTRBuffer,Length   ActLength */
		{
			CDROM_OBJ *obj = (CDROM_OBJ *) packet->dp_Arg1;
			char      *ptr = (char *) packet->dp_Arg2;
			LONG    length = packet->dp_Arg3;
			int     actual;

			if (obj->volume != global->g_volume)
			{
				/* old lock from another disk: */
				error = ERROR_DEVICE_NOT_MOUNTED;
				break;
			}
			actual = Read_From_File (obj, ptr, length);
			packet->dp_Res1 = actual;
		}
		break;
		case ACTION_END: /* FHArg1      Bool:TRUE */
		{
			CDROM_OBJ *obj = (CDROM_OBJ *) packet->dp_Arg1;

			if (obj->volume != global->g_volume)
			{
				/* old lock from another disk: */
				error = ERROR_DEVICE_NOT_MOUNTED;
				break;
			}
			Unregister_File_Handle (obj);
			Close_Object (obj);
			global->g_volume->file_handles--;
		}
		break;
		case ACTION_SEEK: /* FHArg1,Position,Mode     OldPosition */
		{
			CDROM_OBJ *obj = (CDROM_OBJ *) packet->dp_Arg1;
			LONG offset = packet->dp_Arg2;
			int mode = packet->dp_Arg3;

			if (obj->volume != global->g_volume)
			{
				/* old lock from another disk: */
				error = ERROR_DEVICE_NOT_MOUNTED;
				break;
			}
			packet->dp_Res1 = obj->pos;
			if (!Seek_Position (obj, offset, mode))
			{
				error = ERROR_SEEK_ERROR;
				packet->dp_Res1 = -1;
			}
		}
		break;
		case ACTION_EXAMINE_NEXT: /* Lock,Fib     Bool */
		{
			FIB       *fib = (FIB *)BADDR (packet->dp_Arg2);
			CDROM_OBJ *dir = getlockfile (global, packet->dp_Arg1);
			CDROM_INFO info;

			if (dir->volume != global->g_volume)
			{
				/* old lock from another disk: */
				error = ERROR_DEVICE_NOT_MOUNTED;
				break;
			}
			if (!dir->directory_f)
			{
				error = ERROR_OBJECT_WRONG_TYPE;
				break;
			}
			if (Examine_Next (dir, &info, (ULONG *) &fib->fib_DiskKey))
			{
				error = 0;
				Fill_FileInfoBlock (global, fib, &info, dir->volume);
			}
			else
			{
				error = ERROR_NO_MORE_ENTRIES;
			}
			break;
		}
		case ACTION_EXAMINE_FH:     /* FHArg1,Fib     Bool */
		case ACTION_EXAMINE_OBJECT: /* Lock,Fib       Bool */
		{
			FIB *fib = (FIB *)BADDR(packet->dp_Arg2);
			CDROM_INFO info;
			CDROM_OBJ *obj;

			if (packet->dp_Type == ACTION_EXAMINE_FH)
				obj = (CDROM_OBJ*) packet->dp_Arg1;
			else
				obj = getlockfile (global, packet->dp_Arg1);
			if (obj->volume != global->g_volume)
			{
				/* old lock from another disk: */
				error = ERROR_DEVICE_NOT_MOUNTED;
				break;
			}
			fib->fib_DiskKey = 0;
			error = 0;
			if (!CDROM_Info (obj, &info))
				error = -1;
			else
				Fill_FileInfoBlock (global, fib, &info, obj->volume);
		}
		break;
		case ACTION_INFO:      /* Lock, InfoData    Bool:TRUE */
			tmp = (void *)BADDR(packet->dp_Arg2);
			error = -1;
			/* fall through */
		case ACTION_DISK_INFO: /* InfoData          Bool:TRUE */
		{
			{
				register INFODATA *id;

				id = (INFODATA *)(error ? tmp : (void *)BADDR (packet->dp_Arg1));
				error = 0;
				memset (id, 0, sizeof(*id));
				id->id_UnitNumber = global->g_unit;
				if (global->g_inhibited) {
					id->id_DiskType = 0x42555359 /* "BUSY" */;
				} else {
				    int errcode = Mount_Check (global);
				    if (errcode == ERROR_NO_DISK) {
					id->id_DiskType = ID_NO_DISK_PRESENT;
				    } else if (!errcode) {
					id->id_DiskType = ID_DOS_DISK;
					id->id_NumBlocks= Volume_Size (global->g_volume);
					 id->id_BytesPerBlock = Block_Size (global->g_volume);
					id->id_VolumeNode = MKBADDR(global->DevList);
		                    } else {
				        /* For example empty CD-R */
				        id->id_DiskType = ID_UNREADABLE_DISK;
				        id->id_NumBlocks = 0;
				        id->id_BytesPerBlock = 2048;
				    }
				    id->id_DiskState = ID_WRITE_PROTECTED;
				}
				id->id_NumBlocksUsed = id->id_NumBlocks;
			}
		}
		break;
		case ACTION_IS_FILESYSTEM: /* -                       Bool */
			packet->dp_Res1 = DOSTRUE;
			break;
		case ACTION_PARENT_FH:     /* FHArg1                  ParentLock */
		case ACTION_PARENT:        /* Lock                    ParentLock */
		{
			if (!(error = Mount_Check (global)))
			{
				if (packet->dp_Arg1)
				{
					CDROM_OBJ *obj;
					CDROM_OBJ *parent;

					if (packet->dp_Type == ACTION_PARENT_FH)
						obj = (CDROM_OBJ*) packet->dp_Arg1;
					else
						obj = getlockfile (global, packet->dp_Arg1);
					if (obj->volume != global->g_volume)
					{
						/* old lock from another disk: */
						error = ERROR_DEVICE_NOT_MOUNTED;
						break;
					}
					if (Is_Top_Level_Object (obj))
					{
						packet->dp_Res1 = packet->dp_Res2 = 0;
					}
					else
					{
						parent = Find_Parent (obj);
						if (!parent)
						{
							if (global->iso_errno == ISOERR_NO_MEMORY)
								error = ERROR_NO_FREE_STORE;
							else
								error = ERROR_OBJECT_NOT_FOUND;
						}
						else
						{
							packet->dp_Res1 = (IPTR)MKBADDR(cdlock (parent, ACCESS_READ));
						}
					}
				}
				else
					error = ERROR_OBJECT_NOT_FOUND;
			}
		}
		break;
		case ACTION_LOCATE_OBJECT: /* Lock,Name,Mode   Lock */
		{
			if (!(error = Mount_Check (global)))
			{
				CDROM_OBJ *parentdir = getlockfile (global, packet->dp_Arg1);
				CDROM_OBJ *obj;
				int offs;

				if (parentdir->volume != global->g_volume)
				{
					/* old lock from another disk: */
					error = ERROR_DEVICE_NOT_MOUNTED;
					break;
				}
				btos ((BSTR)packet->dp_Arg2, buf);
#ifndef NDEBUG
				dbprintf (global, "'%s' %ld ", buf, packet->dp_Arg3);
				if (strcmp(buf,"debugoff") == 0)
					global->DBDisable = 1;
				if (strcmp(buf,"debugon") == 0)
					global->DBDisable = 0;
#endif
				offs = Check_For_Volume_Name_Prefix (global, buf);
				if (buf[offs]==0)
				{
					if (parentdir)
						obj = Clone_Object (parentdir);
					else
						obj = Open_Top_Level_Directory (global->g_volume);
				}
				else
					obj = Open_Object (parentdir, buf + offs);
				if (obj)
				{
						if (obj->symlink_f)
						error = ERROR_IS_SOFT_LINK;
					else
						packet->dp_Res1 = (IPTR)MKBADDR(cdlock (obj, packet->dp_Arg3));
				}
				else
				{
					if (global->iso_errno == ISOERR_SCSI_ERROR)
						error = ERROR_SEEK_ERROR;
					else if (global->iso_errno == ISOERR_ILLEGAL_NAME)
						error = ERROR_INVALID_COMPONENT_NAME;
					else if (global->iso_errno == ISOERR_NOT_FOUND)
						error = ERROR_OBJECT_NOT_FOUND;
					else if (global->iso_errno == ISOERR_NO_MEMORY)
						error = ERROR_NO_FREE_STORE;
					else if (global->iso_errno == ISOERR_IS_SYMLINK)
						error = ERROR_IS_SOFT_LINK;
					else
						error = 333;
				}
			}
		}
		break;
		case ACTION_COPY_DIR:   /*	 Lock,			    Lock       */
		{
			if (packet->dp_Arg1)
			{
				CDROM_OBJ *obj = getlockfile (global, packet->dp_Arg1);
				CDROM_OBJ *new;

				if (obj->volume != global->g_volume)
				{
					/* old lock from another disk: */
					error = ERROR_DEVICE_NOT_MOUNTED;
					break;
				}
				new = Clone_Object (obj);
				if (!new)
					error = ERROR_NO_FREE_STORE;
				else
					packet->dp_Res1 = (IPTR)MKBADDR(cdlock (new, ACCESS_READ));
			}
			else
				packet->dp_Res1 = 0;
		}
		break;
		case ACTION_FREE_LOCK:      /* Lock      Bool */
			if (packet->dp_Arg1);
				cdunlock((LOCK *)BADDR(packet->dp_Arg1));
			break;
		case ACTION_CURRENT_VOLUME: /* FHArg1    DevList */
		{
			CDROM_OBJ *obj = (CDROM_OBJ*) packet->dp_Arg1;
			if (obj)
				packet->dp_Res1 = (IPTR)MKBADDR(Find_Dev_List (obj));
			else
				packet->dp_Res1 = (IPTR)MKBADDR(global->DevList);
			break;
		}
		case ACTION_INHIBIT:        /* Bool       Bool */
			if (packet->dp_Arg1 != DOSFALSE)
			{
				/* true means forbid access */
				global->g_inhibited++;
				if (global->DevList)
					Unmount(global);
				Hide_CDDA_Icon (global);
				global->g_cd->t_changeint2 = -2;
			}
			else
			{
				/* false means access allowed */
				if (global->g_inhibited)
					global->g_inhibited--;
				if (global->g_inhibited == 0)
				{
					global->g_disk_inserted = FALSE;
					Check_Disk(global);
				}
			}
		break;
    /*
     *  FINDINPUT and FINDOUTPUT normally should return the
     *  'write protected' error. If the field 'Name', however,
     *  designates the root (e.g. CD0:), then the 'wrong type'
     *  error should be returned. Otherwise, AmigaDOS would do
     *  some funny things (such as saying 'Volume CD0: is write-
     *  protected') if you try to mount the handler with the
     *  field 'Mount' set to 1.
     */
		case ACTION_FINDOUTPUT: /* Handle  Lock  Name         Bool */
		case ACTION_FINDUPDATE: /* Handle  Lock  Name         Bool */
		{
			int pos;

			btos((BSTR)packet->dp_Arg3,buf);
			BUG(dbprintf(global, "'%s' ", buf);)
			if ((pos = Check_For_Volume_Name_Prefix (global, buf)) && buf[pos] == 0)
				error = ERROR_OBJECT_WRONG_TYPE;
			else
				error = ERROR_DISK_WRITE_PROTECTED;
			break;
		}
		case ACTION_SAME_LOCK:  /* Lock  Lock                 Bool */
		{
			CDROM_OBJ *obj1 = getlockfile(global, packet->dp_Arg1),
			          *obj2 = getlockfile(global, packet->dp_Arg2);
			if (Same_Objects (obj1, obj2))
				packet->dp_Res1 = DOSTRUE;
			else
				packet->dp_Res1 = DOSFALSE;
			break;
		}
		case ACTION_READ_LINK: /* Lock Name Buf Length         NumChar */
		{
			CDROM_OBJ *obj;
			CDROM_OBJ *parentdir = getlockfile (global, packet->dp_Arg1);
			char *outbuf = (char *) packet->dp_Arg3;
			t_ulong maxlength = packet->dp_Arg4;
			int offs;
			char buf[256];
			int res;

			if (parentdir->volume != global->g_volume)
			{
				/* old lock from another disk: */
				error = ERROR_DEVICE_NOT_MOUNTED;
				break;
			}
			BUG(dbprintf (global, "'%s' len=%lu ", packet->dp_Arg2, maxlength);)
			offs = Check_For_Volume_Name_Prefix (global, (char *) packet->dp_Arg2);
			obj = Open_Object (parentdir, (char *) packet->dp_Arg2 + offs);
			if (obj)
			{
				res = Get_Link_Name (obj, buf, sizeof (buf));
				if (
						res == 0 ||
							strlen (buf) +
							strlen (global->g_vol_name+1) + 1
						>=
							maxlength
					)
					strncpy (outbuf, "illegal_link", maxlength - 1);
				else
				{
					if (buf[0] == ':')
						strcpy (outbuf, global->g_vol_name+1);
					else
						outbuf[0] = 0;
					strcat (outbuf, buf);
				}
				outbuf[maxlength - 1] = 0;
				Close_Object (obj);
				packet->dp_Res1 = strlen (outbuf);
			}	
			else
			{
				if (global->iso_errno == ISOERR_ILLEGAL_NAME)
					error = ERROR_INVALID_COMPONENT_NAME;
				else if (global->iso_errno == ISOERR_NOT_FOUND)
					error = ERROR_OBJECT_NOT_FOUND;
				else if (global->iso_errno == ISOERR_NO_MEMORY)
					error = ERROR_NO_FREE_STORE;
				else if (global->iso_errno == ISOERR_IS_SYMLINK)
					error = ERROR_IS_SOFT_LINK;
				else
					error = 333;
			}
			break;
		}
		case ACTION_MAKE_LINK:
		case ACTION_RENAME_DISK:
		case ACTION_WRITE:
		case ACTION_SET_PROTECT:
		case ACTION_DELETE_OBJECT:
		case ACTION_RENAME_OBJECT:
		case ACTION_CREATE_DIR:
		case ACTION_SET_COMMENT:
		case ACTION_SET_DATE:
		case ACTION_SET_FILE_SIZE:
			error = ERROR_DISK_WRITE_PROTECTED;
			break;
		case ACTION_FLUSH: /* writeout bufs, disk motor off */
			break;
    /*
     *	A few other packet types which we do not support
     */
		case ACTION_MORE_CACHE: /*	 #BufsToAdd		    Bool       */
		case ACTION_WAIT_CHAR:  /*	 Timeout, ticks 	    Bool       */
		case ACTION_SCREEN_MODE:/*	 Bool(-1:RAW 0:CON)	    OldState   */
		default:
			error = ERROR_ACTION_NOT_KNOWN;
			break;
		}
		if (packet)
		{
			if (error)
			{
				BUG(dbprintf(global, "ERR=%ld\n", error);)
				packet->dp_Res1 = DOSFALSE;
				packet->dp_Res2 = error;
			}
			else
			{
				BUG(dbprintf(global, "RES=%06lx\n", packet->dp_Res1));
			}
			if (packet->dp_Type != ACTION_DIE)
				returnpacket(global, packet);
			else
				global->g_death_packet = packet;
		}
	}
	if (notdone)
		return notdone;
   BUG(dbprintf(global, "Can we remove ourselves? ");)
   BUG(Delay(100);)	    /*	I wanna even see the debug message! */
   Forbid();
	if (
			global->g_cd &&
			(
				packetsqueued(global) ||
				(
					global->g_volume &&
					(
						global->g_volume->locks || global->g_volume->file_handles
					)
				)
			)
		)
	{
		Permit();
		BUG(dbprintf(global, " ..  not yet!\n");)
		return TRUE;		/*  sorry... can't exit     */
	}
	else
	{
		BUG(dbprintf(global, " ..  yes!\n");)
	}
	return FALSE;
}

/*
 *  PACKET ROUTINES.	Dos Packets are in a rather strange format as you
 *  can see by this and how the PACKET structure is extracted in the
 *  GetMsg() of the main routine.
 */

static void returnpacket(struct CDVDBase *global, struct DosPacket *packet)
{
    register struct Message *mess;
    register struct MsgPort *replyport;

    replyport		     = packet->dp_Port;
    mess		     = packet->dp_Link;
    packet->dp_Port	     = &global->DosProc->pr_MsgPort;
    mess->mn_Node.ln_Name    = (char *)packet;
    mess->mn_Node.ln_Succ    = NULL;
    mess->mn_Node.ln_Pred    = NULL;
    PutMsg(replyport, mess);
}

/*
 *  Are there any packets queued to our device?
 */

int packetsqueued (struct CDVDBase *global)
{
    return ((void *)global->DosProc->pr_MsgPort.mp_MsgList.lh_Head !=
	    (void *)&global->DosProc->pr_MsgPort.mp_MsgList.lh_Tail);
}

#ifndef USE_FAST_BSTR
/*
 *  Convert a BSTR into a normal string.. copying the string into buf.
 *  I use normal strings for internal storage, and convert back and forth
 *  when required.
 */
static void btos(BSTR bstr, char *buf)
{
    UBYTE *src = BADDR(bstr);
    UBYTE len = *src++;

    CopyMem(src, buf, len);
    buf[len] = 0;
}
#endif

/*
 *  The lock function.	The file has already been checked to see if it
 *  is lockable given the mode.
 */

static LOCK *cdlock(CDROM_OBJ *cdfile, int mode)
{
  struct CDVDBase *global = cdfile->global;
  LOCK *lock = AllocVec(sizeof(LOCK), MEMF_PUBLIC | MEMF_CLEAR);

  cdfile->volume->locks++;
  lock->fl_Key = (LONG) Location (cdfile);
  lock->fl_Link = (BPTR) cdfile;
  lock->fl_Access = ACCESS_READ;
  lock->fl_Task = &global->DosProc->pr_MsgPort;
  lock->fl_Volume = (BPTR)MKBADDR(global->DevList);
  Register_Lock (lock);
  return(lock);
}

static void cdunlock (LOCK *lock)
{
  CDROM_OBJ *obj = (CDROM_OBJ *) lock->fl_Link;
  struct CDVDBase *global = obj->global;

  Unregister_Lock (lock);
  --obj->volume->locks;
  
  /* if all locks and filehandles have been removed, and if the volume
   * is not the current volume, then the volume node may be removed:
   */
  if (obj->volume != global->g_volume &&
      obj->volume->locks == 0 && obj->volume->file_handles == 0) {     
    VOLUME *vol = obj->volume;
    
    Forbid ();
      Remove_Volume_Node (global, vol->devlist);
    Permit ();
    Close_Object (obj);
    Close_Volume (vol);
    Send_Event (global, FALSE);
  } else
    Close_Object (obj);

  FreeVec(lock);				/* free lock	    */
}

/*
 *  GETLOCKFILE(bptrlock)
 *
 *  Return the CDROM_OBJ (file or directory) associated with the
 *  given lock, which is passed as a BPTR.
 *
 *  According to the DOS spec, the only way a NULL lock will ever be
 *  passed to you is if the DosNode->dn_Lock is NULL, but I'm not sure.
 *  In anycase, If a NULL lock is passed to me I simply assume it means
 *  the root directory of the CDROM.
 */

CDROM_OBJ *getlockfile (struct CDVDBase *global, IPTR lock)
{
  LOCK *rl = (LOCK *)BADDR (lock);

  if (rl)
    return (CDROM_OBJ *) rl->fl_Link;
  return global->g_top_level_obj;
}

/*
 * If p_pathname contains a ':' character, return the position of the first
 * character after ':'
 * Otherwise, return 0.
 */

int Check_For_Volume_Name_Prefix (struct CDVDBase *global, char *p_pathname)
{
  char *pos = strchr (p_pathname, ':');
  
  return pos ? (pos - p_pathname) + 1 : 0;
}

/*
 * Fills a FileInfoBlock with the information contained in the
 * directory record of a CD-ROM directory or file.
 */

static void Fill_FileInfoBlock (struct CDVDBase *global, FIB *p_fib, CDROM_INFO *p_info, VOLUME *p_volume)
{
	char *src = p_info->name;
	char *dest = p_fib->fib_FileName;
	int len = p_info->name_length;

	if (len > MAX_NAME_LEN)
		len = MAX_NAME_LEN;
	if (p_info->symlink_f)
		p_fib->fib_DirEntryType = ST_SOFTLINK;
	else
		p_fib->fib_DirEntryType = p_info->directory_f ? ST_USERDIR : ST_FILE;
  
	if (len == 1 && *src == ':')
	{
		/* root of file system: */
		p_fib->fib_DirEntryType = ST_ROOT;
		/* file name == volume name: */
		len = global->g_vol_name[0];
		*dest++ = len;
		strncpy(dest, global->g_vol_name+1, len);
	}
	else
	{
		/* copy file name: */
		if (!global->g_show_version_numbers)
		{
			WORD i;
			for (i=0; i<len; i++)
			{
				if (src[i] == ';') {
					len = i;
					break;
				}
			}
		}
		*dest++ = len;
		strncpy(dest, src, len);

		if ((global->g_map_to_lowercase && p_volume->protocol == PRO_ISO) ||
		    (global->g_maybe_map_to_lowercase && !p_volume->mixed_char_filenames))
		{
			/* convert ISO filename to lowercase: */
			int i;
			char *cp = dest;
			for (i=0; i<len; i++, cp++)
				*cp = ToLower (*cp);
		}
	}
	/* terminate string */
	dest[len] = 0;
	/* I don't know exactly why I have to set fib_EntryType, but other
	 * handlers (e.g. DiskHandler by J Toebes et.al.) also do this.
	 */
   
	p_fib->fib_EntryType = p_fib->fib_DirEntryType;
  
	p_fib->fib_Protection = p_info->protection;
	p_fib->fib_Size = p_info->file_length;
	p_fib->fib_NumBlocks = p_info->file_length >> 11;
	p_fib->fib_Date.ds_Days   = p_info->date / (24 * 60 * 60);
	p_fib->fib_Date.ds_Minute = (p_info->date % (24 * 60 * 60)) / 60;
	p_fib->fib_Date.ds_Tick   = (p_info->date % 60) * TICKS_PER_SECOND;

	dest = p_fib->fib_Comment;
	len = p_info->comment_length;
	if (len > MAX_COMMENT_LEN)
		len = MAX_COMMENT_LEN;
	*dest++ = len;
	strncpy(dest, p_info->comment, len);
	dest[len] = 0;
}

/*
 * Create Volume node and add to the device list. This will
 * cause the WORKBENCH to recognize us as a disk. If we don't
 * create a Volume node, Wb will not recognize us.
 */
 
static void Create_Volume_Node (struct CDVDBase *global, LONG p_disk_type, ULONG p_volume_date) {
	struct DeviceList *dl;

	BUG(dbprintf(global, "Inserted volume name: %s\n", global->g_vol_name + 1));
	dl = Find_Volume_Node (global, global->g_vol_name + 1);
	if (dl)
	{
		BUG(dbprintf(global, "[Reusing old volume node]");)
		Forbid ();
		global->DevList = dl;
		dl->dl_Task = &global->DosProc->pr_MsgPort;
		Permit ();
	}
	else
	{
		global->DevList = dl = (struct DeviceList *)MakeDosEntry(global->g_vol_name+1, DLT_VOLUME);
		dl->dl_Task = &global->DosProc->pr_MsgPort;
		dl->dl_DiskType = p_disk_type;
		dl->dl_VolumeDate.ds_Days = p_volume_date / (24 * 60 * 60);
		dl->dl_VolumeDate.ds_Minute = (p_volume_date % (24 * 60 * 60)) / 60;
		dl->dl_VolumeDate.ds_Tick = (p_volume_date % 60) * TICKS_PER_SECOND;
		BUG(dbprintf(global, "Name to add: <0x%08lX> %b\n", dl->dl_Name, dl->dl_Name));
		AddDosEntry((struct DosList *)dl);
		BUG(dbprintf(global, "Added name: <0x%08lX> %b\n", dl->dl_Name, dl->dl_Name));
		/* Under MorphOS AddDosEntry() can modify volume name in case if such a volume
		   is already present in the DosList. In this case it reallocates the name string,
		   appends "_%d" to it and modifies dl_Name. In this case we can lose our
		   volume node if a user removes a disc while some locks are pending and then
		   puts it back, because we wouldn't be able to find it by name in our list.
		   A typical case is using CDVDFS under MorphOS along with built-in MorphOS
		   handler, */
		Register_Volume_Node (global, dl, global->g_vol_name + 1);
	}
}

/*
 * Mount a volume.
 */

static void Mount (struct CDVDBase *global)
{
  char buf[33];

  if (Has_Audio_Tracks (global->g_cd)) {
    Show_CDDA_Icon (global);
    global->g_cd->t_changeint2 = global->g_cd->t_changeint;
  }

  global->g_volume = Open_Volume (global->g_cd, global->g_use_rock_ridge, global->g_use_joliet);
  if (!global->g_volume) {
    BUG(dbprintf (global, "!!! cannot open VOLUME (iso_errno=%d) !!!\n", global->iso_errno);)
    return;
  } else {
    global->g_disk_inserted = TRUE;
    global->g_cd->t_changeint2 = global->g_cd->t_changeint;
    global->g_top_level_obj = Open_Top_Level_Directory (global->g_volume);
    if (!global->g_top_level_obj) {
      BUG(dbprintf (global, "!!! cannot open top level directory !!!\n");)
      return;
    }
  }
  
  BUG(dbprintf (global, "***mounting*** ");)
  Volume_ID (global->g_volume, buf, sizeof (buf)-1);  
  global->g_vol_name[0] = strlen (buf);
  CopyMem(buf, global->g_vol_name+1, strlen (buf));

  if (!(global->g_vol_name[0]))
    CopyMem("\7Unnamed", global->g_vol_name, 8);

  /* AmigaDOS expects the BCPL string g_vol_name to be null-terminated: */
  global->g_vol_name[(int)(global->g_vol_name[0])+1] = 0;

  /* convert ISO volume name to lowercase: */
  if ((global->g_map_to_lowercase && global->g_volume->protocol == PRO_ISO) ||
      (global->g_maybe_map_to_lowercase && !global->g_volume->mixed_char_filenames)) {
    int i;
    for (i=0; i<global->g_vol_name[0]; i++)
      global->g_vol_name[i+1] = ToLower (global->g_vol_name[i+1]);
  }

  global->g_volume->locks = Reinstall_Locks (global);
  global->g_volume->file_handles = Reinstall_File_Handles (global);

  Create_Volume_Node (global, ID_CDFS_DISK, Volume_Creation_Date (global->g_volume));
  global->g_volume->devlist = global->DevList;
  global->g_cd->t_changeint2 = global->g_cd->t_changeint;
  Send_Event (global, TRUE);
}

/*  Remove a volume node from the DOS list.
 *  (Must be nested between Forbid() and Permit()!)
 *  Since DOS uses singly linked lists, we must (ugg) search it manually to find
 *  the link before our Volume entry.
 */

static void Remove_Volume_Node (struct CDVDBase *global, struct DeviceList* p_volume)
{
struct DosList *dl;

  Unregister_Volume_Node (global, p_volume);

  dl = LockDosList(LDF_WRITE | LDF_VOLUMES);
  if (dl)
  {
    RemDosEntry((struct DosList *)p_volume);
    FreeDosEntry((struct DosList *)p_volume);
    UnLockDosList(LDF_WRITE | LDF_VOLUMES);
  }
}

/*  Unmount a volume, and optionally unload the handler code.
 */

static void Unmount (struct CDVDBase *global)
{
  global->g_cd->t_changeint2 = global->g_cd->t_changeint;
  Hide_CDDA_Icon (global);

  if (global->DevList) {

    BUG(dbprintf(global, "***unmounting*** ");)
    
    Close_Object (global->g_top_level_obj);
    
    if (global->g_volume->locks == 0 && global->g_volume->file_handles == 0) {
      Remove_Volume_Node (global, global->DevList);
      Close_Volume (global->g_volume);
    } else {
      BUG(dbprintf(global, "[there are still %d locks on this volume]",
      		   global->g_volume->locks);)
      BUG(dbprintf(global, "[there are still %d file handles on this volume]",
      		   global->g_volume->file_handles);)
      global->DevList->dl_Task = NULL;
    }

    global->DevList = NULL;
  }

  Send_Event (global, FALSE);

  global->g_volume = 0;

  /* when the handler code exits the corresponding device
   * node (e.g. "CD0") will be modified. The handler code
   * will be unloaded and the task entry will be set to
   * zero, so the next device access will reload and
   * restart the handler code.
   */
}

/*
 * Mount_Check returns 0 if a valid disk is inserted in the drive.
 * Error code is returned if drive is empty or disk is unknown.
 * A check is only performed if previously the drive was empty.
 */

int Mount_Check (struct CDVDBase *global)
{
    D(bug("[CDVDFS]\tMount_Check\n"));
    if (!global->g_disk_inserted) {
	/*
	 * No disk was inserted up to now: we will check whether
	 * a disk has been inserted by sending the test unit ready
	 * command. We have to send the command twice because
	 * the first SCSI command after inserting a new disk is
	 * always rejected.
	 */
	if (0 == Test_Unit_Ready (global->g_cd)) 
	{
	    D(bug("[CDVDFS]\tDrive not ready, not mounting\n"));
	    return ERROR_NO_DISK;
	}

	D(bug("[CDVDFS]\tDrive ready, mounting disc\n"));
	global->g_disk_inserted = TRUE;
	Mount (global);
	if (!global->DevList)
	    return ERROR_NO_DISK;
    } else {
        D(bug("[CDVDFS]\tDisc already mounted.\n"));
    }
    if (global->g_volume == NULL || global->g_volume->handler == NULL)
        return ERROR_NOT_A_DOS_DISK;
    return 0;
}

/*
 *  Open timer device structures:
 */

int Open_Timer_Device (struct CDVDBase *global) {

	global->g_timer_mp = CreateMsgPort();
	if (global->g_timer_mp)
	{
		global->g_timer_io = (struct timerequest *)CreateIORequest
			(
				global->g_timer_mp,
				sizeof (struct timerequest)
			);
		if (global->g_timer_io)
		{
			if (
					OpenDevice
					(
						(UBYTE *)TIMERNAME,
						UNIT_VBLANK,
						(struct IORequest *)global->g_timer_io,
						0
					)==0
				)
			{
				global->g_timer_sigbit = 1L << global->g_timer_mp->mp_SigBit;
				return 1;
			}
			else
			{
				BUG(dbprintf (global, "cannot open timer device!\n");)
			}
			DeleteIORequest ((struct IORequest *) global->g_timer_io);
		}
		else
		{
			BUG(dbprintf (global, "cannot create timer i/o structure!\n");)
		}
		DeleteMsgPort (global->g_timer_mp);
	}
	else
	{
		BUG(dbprintf (global, "cannot create timer message port!\n");)
	}
	return 0;
}

/*
 *  Remove timer device structures:
 */

static void Cleanup_Timer_Device (struct CDVDBase *global)
{
  /* remove any pending requests: */
  if (!CheckIO ((struct IORequest *) global->g_timer_io))
    AbortIO ((struct IORequest *) global->g_timer_io);
  WaitIO ((struct IORequest *) global->g_timer_io);
  
  CloseDevice ((struct IORequest *) global->g_timer_io);
  DeleteIORequest ((struct IORequest *) global->g_timer_io);
  DeleteMsgPort (global->g_timer_mp);
}

/*
 *  Send timer request
 */

static void Send_Timer_Request (struct CDVDBase *global)
{
  global->g_timer_io->tr_node.io_Command = TR_ADDREQUEST;
  global->g_timer_io->tr_time.tv_secs = 1;
  global->g_timer_io->tr_time.tv_micro = 0;
  SendIO ((struct IORequest *) global->g_timer_io);
}

/*
 *  Check whether the disk has been removed or inserted.
 */

static void Check_Disk (struct CDVDBase *global) 
{
    int i;
    ULONG l1, l2;

    BUG(dbprintf (global, "[CDVDFS]\tChecking Disk... ");)
	i = Test_Unit_Ready (global->g_cd);

    l1 = global->g_cd->t_changeint;
    l2 = global->g_cd->t_changeint2;
    BUG(if (l1==l2 && i) dbprintf (global, "no disk change (T %ld)", l1);)
	if (l1!=l2 && i)
	{
	    global->g_disk_inserted = TRUE;
	    BUG(dbprintf (global, "disk has been inserted (T %ld)", l1);)
		if (global->DevList)
		    Unmount (global);
	    Delay (50);
	    Clear_Sector_Buffers (global->g_cd);
	    Mount (global);
	}
    BUG(if (l1==l2 && !i) dbprintf (global, "no disk in drive (T %ld)", l1);)
	if (l1!=l2 && !i)
	{
	    global->g_disk_inserted = FALSE;
	    BUG(dbprintf (global, "disk has been removed (T %ld)", l1);)
		global->playing = FALSE;
	    Unmount (global);
	    global->g_cd->t_changeint2 = global->g_cd->t_changeint;
	}
    BUG(dbprintf (global, "\n");)
}

/* The following lines will generate a `disk inserted/removed' event, in order
 * to get Workbench to rescan the DosList and update the list of
 * volume icons.
 */

static void Send_Event (struct CDVDBase *global, int p_inserted)
{
  struct IOStdReq *InputRequest;
  struct MsgPort *InputPort;
  struct InputEvent InputEvent;


  InputPort = (struct MsgPort *) CreateMsgPort ();
  if (InputPort)
  {
    InputRequest = (struct IOStdReq *)
	CreateIORequest (InputPort, sizeof (struct IOStdReq));
    if (InputRequest)
    {
      if (!OpenDevice ((UBYTE *) "input.device", 0,
      		       (struct IORequest *) InputRequest, 0))
      {
	memset (&InputEvent, 0, sizeof (struct InputEvent));

	InputEvent.ie_Class = p_inserted ? IECLASS_DISKINSERTED :
					   IECLASS_DISKREMOVED;

	InputRequest->io_Command = IND_WRITEEVENT;
	InputRequest->io_Data = &InputEvent;
	InputRequest->io_Length = sizeof (struct InputEvent);

	DoIO ((struct IORequest *) InputRequest);

	CloseDevice ((struct IORequest *) InputRequest);
      }
      DeleteIORequest ((struct IORequest *)InputRequest);
    }
    DeleteMsgPort (InputPort);
  }
}

