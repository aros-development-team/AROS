/* $Id$ */
/* $Log: dostohandlerinterface.c $
 * Revision 13.10  1999/05/14  11:31:34  Michiel
 * Long filename support implemented; bugfixes
 *
 * Revision 13.9  1999/02/22  16:22:35  Michiel
 * ACTION_SET_DELDIR added
 *
 * Revision 13.8  1998/05/31  16:27:42  Michiel
 * added ACTION_IS_PFS2
 *
 * Revision 13.7  1997/03/03  22:04:04  Michiel
 * Release 16.21
 *
 * Revision 13.6  1995/12/29  11:04:12  Michiel
 * ACTION_SET_ROLLOVER
 *
 * Revision 13.4  1995/08/21  04:20:43  Michiel
 * added some extrapackets
 *
 * Revision 13.3  1995/08/16  14:28:28  Michiel
 * fixed function table bug
 * added ACTION_KILL_EMPTY (after being forgotten earlier)
 * added ACTION_REMOVE_DIRENTRY
 *
 * Revision 13.2  1995/08/12  12:14:31  Michiel
 * fixed
 * */

#define __USE_SYSBASE

/* system includes */
#include <exec/memory.h>
#include <exec/types.h>
#include <exec/interrupts.h>
#include <dos/filehandler.h>
#include <dos/notify.h>
#if MULTIUSER
#include <proto/multiuser.h>
#endif
#include <string.h>
#include <math.h>
#include "debug.h"

/* own includes */
#include "versionhistory.doc"
#include "blocks.h"
#include "struct.h"
#include "directory_protos.h"
#include "disk_protos.h"
#include "lock_protos.h"
#include "volume_protos.h"
#include "format_protos.h"
#include "update_protos.h"
#include "lru_protos.h"
#include "ass_protos.h"
#include "anodes_protos.h"
#include "init_protos.h"

/* from boot.c */
void ReturnPacket(struct DosPacket *, struct MsgPort *, globaldata *g);

#ifdef DEBUG
extern BOOL debug;
static UBYTE debugbuf[120];
#define DebugOn debug++
#define DebugOff debug = 0
#define DebugMsg(msg) NormalErrorMsg(msg, NULL);
#define DebugMsgNum(msg, num) sprintf(debugbuf, "%s 0x%08lx.", msg, num); \
			if(debug) {NormalErrorMsg(debugbuf, NULL);debug=0;}
#define DebugMsgName(msg, name) sprintf(debugbuf, "%s >%s<.", msg, name); \
			if(debug) {NormalErrorMsg(debugbuf, NULL);debug=0;}
#else
#define DebugOn
#define DebugOff
#define DebugMsg(m)
#define DebugMsgNum(msg,num)
#define DebugMsgName(msg, name)
#endif


/**********************
 * Prototypes
 */

void NormalCommands(struct DosPacket *, globaldata *);
void InhibitedCommands(struct DosPacket *, globaldata *);
#if EXTRAPACKETS
void SleepCommands (struct DosPacket *action, globaldata *g);
#endif

/**********************
 * lower levels
 */

#define RES1(p) (p->dp_Res1)
#define RES2(p) (p->dp_Res2)
#define BARG1(p) BADDR(p->dp_Arg1)
#define BARG2(p) BADDR(p->dp_Arg2)
#define BARG3(p) BADDR(p->dp_Arg3)
#define BARG4(p) BADDR(p->dp_Arg4)

#include "dd_support.c"
#include "dd_funcs.c"


/**********************
 * structures and globals
 */

struct functable
{
	SIPTR (*function)(struct DosPacket *, globaldata *);
	ULONG timeout;
};

static const struct functable functiontable0[] =
{
	{NotKnown, 0},            /* ACTION_NIL 0 */
	{NotKnown, 0},
	{NotKnown, 0},            /* ACTION_GET_BLOCK 2 */
	{NotKnown, 0},
	{NotKnown, 0},            /* ACTION_SET_MAP 4 */
	{dd_Quit, 0},             /* ACTION_DIE 5 */
	{NotKnown, 0},                /* ACTION_EVENT 6 */
	{dd_CurrentVolume, 0},    /* ACTION_CURRENT_VOLUME 7 */
	{dd_Lock, 1},             /* ACTION_LOCATE_OBJECT 8 */
	{dd_Relabel, 1},          /* ACTION_RENAME_DISK 9 */
	{NotKnown, 0},
	{NotKnown, 0},
	{NotKnown, 0},
	{NotKnown, 0},
	{NotKnown, 0},
	{dd_Unlock, 0},           /* ACTION_FREE_LOCK 15 */
	{dd_DeleteObject, 1},     /* ACTION_DELETE_OBJECT 16 */   
	{dd_Rename, 1},           /* ACTION_RENAME_OBJECT 17 */
	{dd_AddBuffers, 0},       /* ACTION_MORE_CACHE 18 */
	{dd_DupLock, 1},          /* ACTION_COPY_DIR 19 */
	{NotKnown, 0},            /* ACTION_WAIT_CHAR 20 */
	{dd_SetProperty, 1},      /* ACTION_SET_PROTECT 21 */
	{dd_CreateDir, 1},        /* ACTION_CREATE_DIR 22 */
	{dd_Examine, 1},          /* ACTION_EXAMINE_OBJECT 23 */
	{dd_Examine, 1},          /* ACTION_EXAMINE_NEXT 24 */
	{dd_Info, 0},             /* ACTION_DISK_INFO 25 */
	{dd_Info, 0},             /* ACTION_INFO 26 */
	{dd_Flush, 0},            /* ACTION_FLUSH 27 */
	{dd_SetProperty, 1},      /* ACTION_SET_COMMENT 28 */
	{dd_Parent, 1},           /* ACTION_PARENT 29 */
	{NotKnown, 0},            /* ACTION_TIMER 30 */
	{dd_InhibitOn, 0},        /* ACTION_INHIBIT 31 */
	{NotKnown, 0},            /* ACTION_DISK_TYPE 32 */
	{NotKnown, 0},            /* ACTION_DISK_CHANGE 33 */
	{dd_SetProperty, 1},      /* ACTION_SET_DATE 34 */
	{NotKnown, 0},
	{NotKnown, 0},
	{NotKnown, 0},
	{NotKnown, 0},
	{NotKnown, 0},
	{dd_SameLock, 0}          /* ACTION_SAME_LOCK 40 */
};

static const struct functable functiontable1000[] =
{
	{NotKnown, 0},
	{NotKnown, 0},            /* ACTION_READ_RETURN 1001 */
	{NotKnown, 0},            /* ACTION_WRITE_RETURN 1002 */
	{NotKnown, 0},
	{dd_Open, 3},             /* ACTION_FINDUPDATE 1004 */
	{dd_Open, 3},             /* ACTION_FINDINPUT 1005 */
	{dd_Open, 3},             /* ACTION_FINDOUTPUT 1006 */
	{dd_Close, 0},            /* ACTION_END 1007 */
	{dd_SeekRead, 3},         /* ACTION_SEEK 1008 */
	{NotKnown, 0},
	{NotKnown, 0},
	{NotKnown, 0},
	{NotKnown, 0},
	{NotKnown, 0},
	{NotKnown, 0},
	{NotKnown, 0},
	{NotKnown, 0},
	{NotKnown, 0},
	{NotKnown, 0},
	{NotKnown, 0},
	{NotKnown, 0},            /* ACTION_FORMAT 1020 (inhibited only) */
	{dd_MakeLink, 1},         /* ACTION_MAKE_LINK 1021 */
	{dd_WriteSFS, 3},         /* ACTION_SET_FILE_SIZE 1022 */
	{dd_WriteProtect, 0},     /* ACTION_WRITE_PROTECT 1023 */
	{dd_ReadLink, 1},         /* ACTION_READ_LINK 1024 */
	{NotKnown, 0},
	{dd_OpenFromLock, 3},     /* ACTION_FH_FROM_LOCK 1026 */
	{dd_IsFileSystem, 0},     /* ACTION_IS_FILESYSTEM 1027 */
	{dd_ChangeMode, 1},       /* ACTION_CHANGE_MODE 1028 */
	{NotKnown, 0},
	{dd_DupLock, 1},          /* ACTION_COPY_DIR_FH 1030 */
	{dd_Parent, 1},           /* ACTION_PARENT_FH 1031 */
	{NotKnown, 0},
	{dd_ExamineAll, 1},       /* ACTION_EXAMINE_ALL 1033 */
	{dd_Examine, 1},          /* ACTION_EXAMINE_FH 1034 */
	{NotYetImplemented, 0},   /* ACTION_EXAMINE_ALL_END 1035 */
	{dd_SetProperty, 1},      /* ACTION_SET_OWNER 1036 */
};



/**********************
 * Normal, not-inhibited packet processor
 */

/* NormalCommands funtion: normal commandmode
 *
 * This function executes the operation specified by the passed packet. Can be
 * called from a testenviroment and from the Handler. The Handler has to take
 * care of receiving and answering the DOS packet. 
 *
 * This function does not contain initialisation. Its sole purpose is the
 * execution of commands on an already active filesystem.
 *
 * pre: filesystem ready; dos- and intuition open; unitdata ready
 * post: command executed.
 */
void NormalCommands(struct DosPacket *action, globaldata *g)
{
  ULONG packettype;

	/* clear error field */
	action->dp_Res2 = 0;

	packettype = action->dp_Type;
	if (packettype == ACTION_WRITE)
	{
		action->dp_Res1 = dd_WriteSFS (action, g);
		g->timeout |= 3;
	}
	else if (packettype == ACTION_READ)
	{
		action->dp_Res1 = dd_SeekRead (action, g);
		g->timeout |= 3;
	}
	else if (packettype <= ACTION_SAME_LOCK)
	{
		action->dp_Res1 = functiontable0[packettype].function(action, g);
		g->timeout |= functiontable0[packettype].timeout;
	}
	else if (packettype >= 1000 && packettype <= ACTION_SET_OWNER)
	{
		action->dp_Res1 = functiontable1000[packettype-1000].function(action, g);
		g->timeout |= functiontable1000[packettype-1000].timeout;
	}
	else switch (packettype)
	{
		case ACTION_LOCK_RECORD:
		case ACTION_FREE_RECORD:
			action->dp_Res1 = NotYetImplemented(action, g);
			break;

		case ACTION_ADD_NOTIFY:
			action->dp_Res1 = dd_AddNotify(action, g);
			break;

		case ACTION_REMOVE_NOTIFY:
			action->dp_Res1 = dd_RemoveNotify(action, g);
			break;

		/*
		 * Own packets
		 */
		case ACTION_KILL_EMPTY:
			action->dp_Res1 = dd_KillEmpty(action, g);
			break;

		case ACTION_REMOVE_DIRENTRY:
			action->dp_Res1 = dd_RemoveDirEntry(action, g);
			break;

		case ACTION_SLEEP:
			action->dp_Res1 = dd_Sleep(action, g);
			break;

		case ACTION_CREATE_ROLLOVER:
			action->dp_Res1 = dd_MakeRollover(action, g);
			g->timeout |= 1;
			break;

		case ACTION_SET_ROLLOVER:
			action->dp_Res1 = dd_SetRollover(action, g);
			g->timeout |= 1;
			break;

		case ACTION_IS_PFS2:
			action->dp_Res1 = dd_IsPFS2(action, g);
			break;

		case ACTION_ADD_IDLE_SIGNAL:
			action->dp_Res1 = dd_SignalIdle(action, g);
			g->timeout |= 1;
			break;

		case ACTION_SET_DELDIR:
			action->dp_Res1 = dd_SetDeldir(action, g);
			break;

		case ACTION_SET_FNSIZE:
			action->dp_Res1 = dd_SetFileSize(action, g);
			break;

#if EXTENDED_PACKETS_OS4
		case ACTION_CHANGE_FILE_POSITION64:
			dd_ChangeFilePosition64(action, g);
			break;
		case ACTION_GET_FILE_POSITION64:
			dd_GetFilePosition64(action, g);
			break;
		case ACTION_CHANGE_FILE_SIZE64:
			dd_ChangeFileSize64(action, g);
			break;
		case ACTION_GET_FILE_SIZE64:
			dd_GetFileSize64(action, g);
			break;
#endif

#if EXTENDED_PACKETS_MORPHOS
		/*
		 * Note: If we ever support file sizes between 2^31 to 2^32-2 then SEEK64 needs
		 * to be implemented. - Piru
		 */
		case ACTION_SEEK64:
		case ACTION_SET_FILE_SIZE64:
		case ACTION_LOCK_RECORD64:
		case ACTION_FREE_RECORD64:
		case ACTION_EXAMINE_OBJECT64:
		case ACTION_EXAMINE_NEXT64:
		case ACTION_EXAMINE_FH64:
			action->dp_Res1 = NotKnown(action, g);
			break;

#if defined(__MORPHOS__)
		case ACTION_NEW_READ_LINK:
			/*
			 * This really ought to be implemented at some point.
			 * It'd mostly require support for reading hardlink destination.
			 * - Piru
			 */
			action->dp_Res1 = NotKnown(action, g);
			break;

		case ACTION_QUERY_ATTR:
			action->dp_Res1 = dd_MorphOSQueryAttr(action, g);
			break;
#endif
#endif
		case ACTION_SERIALIZE_DISK: // Inhibited only
		default:
			action->dp_Res1 = NotKnown(action, g);
			break;
	}
}


/**********************
 * Inhibited packet processor
 */

void InhibitedCommands(struct DosPacket *action, globaldata *g)
{
	/* clear error field */
	action->dp_Res2 = 0;

	switch (action->dp_Type)
	{
		case ACTION_INHIBIT:
			action->dp_Res1 = dd_InhibitOff(action, g);
			break;

		case ACTION_FORMAT:         // Format(fs,vol,type) 2.0
			action->dp_Res1 = dd_Format(action, g);
			break;

		case ACTION_FREE_LOCK:      // UnLock()
			action->dp_Res1 = dd_Unlock(action, g);
			break;

		case ACTION_CURRENT_VOLUME: // <sendpkt only>
			action->dp_Res1 = 0;
			break;

		case ACTION_DISK_INFO:      // Info(..)
		case ACTION_INFO:
			action->dp_Res1 = dd_Info(action, g);
			break;

		case ACTION_IS_FILESYSTEM:  // IsFileSystem(devname)
			action->dp_Res1 = DOSTRUE;
			break;

		case ACTION_FLUSH:          // <sendpkt only>
			// UpdateDisk(g);
			action->dp_Res1 = DOSTRUE;
			break;

		case ACTION_DIE:            // <sendpkt only>
			g->dieing = TRUE;
			action->dp_Res1 = dd_Quit(action, g);
			break;

		case ACTION_SERIALIZE_DISK: // zie dosextens.h regel 220
			action->dp_Res1 = dd_SerializeDisk(action, g);
			break;

		case ACTION_IS_PFS2:
			action->dp_Res1 = dd_IsPFS2(action, g);
			break;

		case ACTION_SET_FNSIZE:
		case ACTION_FINDINPUT:      // Open(.., MODE_OLDFILE)
		case ACTION_FINDOUTPUT:     // Open(.., MODE_NEWFILE)
		case ACTION_FINDUPDATE:     // Open(.., MODE_READWRITE)
		case ACTION_EXAMINE_FH:     // ExamineFH(fh,fib)
		case ACTION_EXAMINE_OBJECT: // Examine(..)
		case ACTION_EXAMINE_NEXT:   // ExNext(..)
		case ACTION_CREATE_DIR:     // CreateDir(..)
		case ACTION_DELETE_OBJECT:  // DeleteFile(..)
		case ACTION_RENAME_OBJECT:  // Rename(..)
		case ACTION_WRITE:          // Write(..)
		case ACTION_LOCATE_OBJECT:  // Lock(..)
		case ACTION_COPY_DIR_FH:
		case ACTION_COPY_DIR:       // DupLock(..) COULD be implemented ?
		case ACTION_PARENT_FH:      // ParentOfFH(fh)
		case ACTION_PARENT:         // Parent(..)
		case ACTION_SET_PROTECT:    // SetProtection(..)
		case ACTION_SET_COMMENT:    // SetComment(..)       
		case ACTION_SET_DATE:       // SetFileDate(..)
		case ACTION_FH_FROM_LOCK:   // OpenFromLock(lock)
		case ACTION_CHANGE_MODE:    // ChangeMode(type,obj,mode)
		case ACTION_RENAME_DISK:    // Relabel(..)
		case ACTION_EXAMINE_ALL:    // ExAll(lock,buff,size,type,ctl)
		case ACTION_SET_FILE_SIZE:  // SetFileSize(file,off,mode)
		case ACTION_SAME_LOCK:      // SameLock(lock1, lock2)
		case ACTION_MAKE_LINK:      // MakeLink(name,targ,mode)
		case ACTION_READ_LINK:      // ReadLink(port,lck,nam,buf,len)
		case ACTION_ADD_NOTIFY:     // StartNotify(NotifyRequest)
		case ACTION_REMOVE_NOTIFY:  // EndNotify(NotifyRequest)
		case ACTION_MORE_CACHE:     // AddBuffers(..)
		case ACTION_WRITE_PROTECT:  // <sendpkt only>
		case ACTION_END:            // Close(..)
		case ACTION_CREATE_ROLLOVER:

#if EXTENDED_PACKETS_OS4
		case ACTION_CHANGE_FILE_POSITION64:
		case ACTION_GET_FILE_POSITION64:
		case ACTION_CHANGE_FILE_SIZE64:
		case ACTION_GET_FILE_SIZE64:
#endif

			action->dp_Res2 = ERROR_NOT_A_DOS_DISK;
			action->dp_Res1 = DOSFALSE;
			break;

		case ACTION_READ:           // Read(..)
		case ACTION_SEEK:           // Seek(..)
			action->dp_Res2 = ERROR_NOT_A_DOS_DISK;
			action->dp_Res1 = -1;
			break;

#if EXTENDED_PACKETS_MORPHOS
		case ACTION_SEEK64:
		case ACTION_SET_FILE_SIZE64:
		case ACTION_LOCK_RECORD64:
		case ACTION_FREE_RECORD64:
		case ACTION_EXAMINE_OBJECT64:
		case ACTION_EXAMINE_NEXT64:
		case ACTION_EXAMINE_FH64:
#if defined(__MORPHOS__)
		case ACTION_NEW_READ_LINK:
			action->dp_Res2 = ERROR_NOT_A_DOS_DISK;
			action->dp_Res1 = DOSFALSE;
			break;
		case ACTION_QUERY_ATTR:
			action->dp_Res1 = dd_MorphOSQueryAttr(action, g);
			break;
#endif
#endif

		default:
			NotKnown(action, g);
			break;
	}
}


#if EXTRAPACKETS

/**********************
 * MODE_SLEEP packet processor
 */

/*
 * Packethandler during MODE_SLEEP
 */
void SleepCommands(struct DosPacket *action, globaldata *g)
{
	/* clear error field */
	action->dp_Res2 = 0;

	switch (action->dp_Type)
	{
		case ACTION_FREE_LOCK:      // UnLock()
			action->dp_Res1 = dd_Unlock(action, g);
			break;

		case ACTION_CURRENT_VOLUME: // <sendpkt only>
			action->dp_Res1 = dd_CurrentVolume(action, g);
			break;

		case ACTION_DISK_INFO:      // Info(..)
		case ACTION_INFO:
			action->dp_Res1 = dd_Info(action, g);
			break;

		case ACTION_IS_FILESYSTEM:  // IsFileSystem(devname)
			action->dp_Res1 = DOSTRUE;
			break;

		case ACTION_FLUSH:          // <sendpkt only>
			action->dp_Res1 = DOSTRUE;
			break;

		/*
		 * special sleep packets
		 */
		case ACTION_SLEEP:
			action->dp_Res1 = dd_Sleep(action, g);
			break;

		case ACTION_UPDATE_ANODE:
			action->dp_Res1 = dd_UpdateAnode(action, g);
			break;

		/*
		 * otherwise wake up
		 */
		default:
			Alarm(g);
			NormalCommands(action, g);
	}
}

#endif
