/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef ARP_H
#define ARP_H

#include <exec/types.h>
#include <exec/libraries.h>
#include <exec/lists.h>
#include <exec/semaphores.h>
#include <dos/dos.h>

struct ArpBase
{
  BPTR   seglist;
  UBYTE  Flags;
  UBYTE  pad;
  LONG   AB_reserved;
  APTR   EnvBase;

  struct Library * DosBase;
  struct Library * GfxBase;
  struct Library * IntuiBase;
  struct MinList ResLists;

  struct Library * ExecBase;
  struct Library * AslBase;
  struct Library * UtilityBase;
  char   ESCChar;
};

/*--------------- Following is here only for compatibility with MANX,
 *--------------- don't use in new code!
 */
struct EnvLib
{
  APTR  EnvSpace; /* access only when Forbidden! */
  ULONG EnvSize;  /* size of environment */
};

/*---------- Flags bit definitions
 *
 * NOTE:  You can check these, but better not change them!
 *	 It is currently undetermined how these will be set, but they should
 *	 only be set by the user, or by the startup code.  Possibly we could
 *	 check for an environment variable.  This is likely to be the best idea.
 *
 * NOTE FOR THOSE WRITING LIBRARY CODE:
 *	Routines which use these flags to decide which escape or wildcards to
 *	use default to the BCPL version if neither one is set.  For consistancy,
 *	please observe the same convention in your code.
 *
 *-------------------------------------------------------
 */

#define WILD_WORLD 0
#define WILD_BCPL  1

/*---------- Arp alert numbers, the cutesy BAD CODE one is gone.
 *---------- We will have to get these blessed by Amiga soon, or changed, or whatever.
 *
 *-------- Alert Object
 */

#define AO_ArpLib	0x00008036

/*-------- Specific Alerts you can get when opening ArpLib. -----------*
 *-------  ONLY ARPLIB can return these, you should construct your own, preferably
 *-------  NON dead end alerts, for example, if you can't open ARP:
 *-------  	MYALERT AG_OpenLib!AO_ArpLib
 *---------------------------------------------------------------------*/

#define AN_ArpLib	EQU		0x03600000
#define AN_ArpNoMem	EQU		0x03610000	/* Arplibrary out of memory.   */
#define AN_ArpTaskArray		0x83610001	/* No memory for task array    */
#define AN_ArpInputMem		0x03610002	/* No memory for input buffer. */
#define AN_ArpNoMakeEnv		0x83610003	/* No memory for EnvLib */

#define AN_ArpNoDOS		0x83630001	/* Can't open DOS library */
#define AN_ArpNoGfx		0x83630002	/* Can't open graphics    */
#define AN_ArpNoIntuit	0x83630003	/* Can't open intuition */

#define AN_ArpObsFunc		0x03600001	/* Call of obsolete function. */
#define AN_ArpScattered	0x83600002	/* Scatter loading not allowed for lib. */

/*  Return codes you can get from calling Assign: */
#define ASSIGN_OK	 	  0
#define ASSIGN_NODEV  1
#define ASSIGN_FATAL  2
#define ASSIGN_CANCEL 3	/* Can't cancel a VOLUME */

/*--------- Size of buffer you need for ReadLine */

#define MaxInputBuf	256

/*--------- Input structure for File Requester */
#define FRB_DoWildFunc	7
#define FRB_DoMsgFunc		6

#define FCHARS	32			/* Directory name sizes */
#define DSIZE		33

/* SET ALL OTHER FLAGS TO NULL -
 *	They will be used in later revs of Arp.library.
 *	If you set them non-null, they will do something besides default.
 *	You may not be set up for it.
 */

struct FR_struct
{
	APTR	FR_Hail;			/* Hailing text                       */
	APTR	FR_Ddef;			/* *Filename array (FCHARS+1)         */
	APTR	FR_Ddir;			/* *Directory array (DSIZE+1)         */
	APTR	FR_Wind;			/* Window requesting or NULL          */
	WORD	FR_Flags;		/* Control. See above. SET TO NULL.   */
	APTR	FR_WildFunc;		/* Func to call for wildcards       */
	APTR	FR_MsgFunc;		/* Func to call with IntuiMessages  */
};
/*---------- Structures and constants used by the wildcard routines */

#define P_TAG			0x85
#define P_TAGEND		0x86
#define ERROR_INTERNAL		999
//#define BUFSIZ			200
#define DOS_TRUE		0
#define DOS_FALSE		-1
#define MAXFNAME		200


/* This is the structure you pass to FindFirst/FindNext.
 * In it you can set AP_BREAKBITS to be the bits you want to match
 * for break checking while the directory is being searched.
 * Also you may allocate a buffer at AP_BUF, the size of which is
 * in AP_LENGTH, where the full path name of the current entry will
 * be built.  If AP_LENGTH is 0, the path name will not be built.
 */
struct AP
{
	APTR  AP_BASE;
	APTR  AP_LAST;
	LONG	AP_BREAKBITS;
	LONG	AP_FOUNDBREAK;
	APTR  AP_LENGTH;
	struct	FileInfoBlock AP_INFO;
};

struct AN
{
	APTR    AN_NEXT;
	APTR    AN_PRED;
	LONG	  AN_LOCK;
	APTR    AN_INFO;
	LONG    AN_STATUS; /* Text must be longword aligned for passing to dos */
	WORD    AN_TEXT;		/* ;Account for */
};


/* Structure used by AddDANode, AddDADevs, FreeDAList
 *	This structure is used to create lists of names,
 * which normally are devices, assigns, volumes, files, or directories
 */

struct DirectoryEntry
{
	LONG	DA_Next;
	BYTE	DA_Type;
	BYTE	DA_Flags;
};

#define DLB_DEVICES		0
#define DLB_DISKONLY	1		; If only DISK devices
#define DLB_VOLUMES		2
#define DLB_DIRS		3

#define DLX_FILE		0
#define DLX_DIR			8
#define DLX_DEVICE		16

#define DLX_VOLUME		24
#define DLX_UNMOUNTED		32

#define DLX_ASSIGN		40

/************************************************************************
 *
 * ResLists
 *	This list is used to track resources, normally within a Task.
 * Each ResList has a single-linked node linking together ResLists;  this
 * list is maintained in ArpBase now but ideally would use TC_UserData,
 * so the ResList for a task could be found more quickly.  However, there
 * must also be a link so ResLists can be maintained in ArpBase, for
 * Resource Tracking.  To facilitate this at a later data, we have
 * added the ReservedList field, which could be used if ArpBase gets
 * burned into ROM.
 *
 * NOTE: This is a DosAllocMem'd list, with length @-4
 */

struct ArpResList
{
	struct MinNode ARL_node;  /*  Links these together */
	LONG	TaskID;
	struct	MinList FirstItem;
	APTR	ARL_link;   /* For temp removal from task rlist */
};

/* Tracked Items
 * Each item in the list has a double-link node which is used to
 * attach to the FirstItem list header in a ResList.  All list items are
 * based on a simple memory allocation; the generic node is a DosAllocMem'd
 * memory block set to all NULL for the control fields.  This makes it very
 * easy to track the nodes themselves, in case there is a problem during
 * allocation.
 */
struct TrackedResource
{
	struct MinNode TR_Node;  /*	Double linked pointer         */
	BYTE	TR_Flags;	 /* See flag bits below           */
	BYTE	TR_Lock;	 /* Used by GetAccess/FreeAccess  */
	WORD	TR_ID;		 /* ID for this item class        */
	APTR	TR_Stuff;	 /* Whatever the dude wants to free. */
	LONG	TR_Extra;	 /* Unused now, even to memchunk size*/
};

/*	The function "GetTracker" will allocate a generic tracking node,
 * which is a minimum-sized memory block passed back to the user.
 * The pointer the user gets actually points at the TR_Stuff node,
 * rather than at the head of the list.  The user-accessible fields are
 * below:
 */
struct UserResource
{
	WORD	TRU_ID;
	APTR	TRU_Stuff;
	LONG	TRU_Extra;
};
/* See discussion of the TRU_Extra field below; it is always allocated
 * because the minimum memory grain size is 8 bytes, and this extra
 * field actually does not require any extra memory.
 */

/* Tracked Item Types
 *	The id types below show the types of resources which may
 * be tracked in a resource list.
 */
#define TRAK_AAMEM		0		/* Default generic (ArpAlloc) element */
#define TRAK_LOCK		1		/* File Lock */
#define TRAK_FILE		2		/* Opened File */
#define TRAK_WINDOW		3		/* Window (see discussion) */
#define TRAK_SCREEN		4		/* Screen */
#define TRAK_LIBRARY		5		/* Opened library */
#define TRAK_DAMEM		6		/* Pointer to DosAllocMem block */
#define TRAK_MEMLIST		7		/* Exec Memlist */
#define TRAK_SEGLIST		8		/* Program Segment List */
#define TRAK_RESLIST		9		/* ARP (nested) ResList */
#define TRAK_MEM		10		/* Memory ptr/length */
#define TRAK_GENERIC		11		/* Generic Element */
#define TRAK_DALIST		12		/* DAlist ( as used by file request ) */
#define TRAK_ANCHOR		13		/* Anchor chain */
#define TRACK_MAX		13		/* Anything else is tossed. */

/* For TRAK_GENERIC, you set up a task function to be called when
 * an item is freed.  This is very dangerous if not used properly.
 * PROGRAMMER BEWARE.  ( If you leave a relist untracked, too bad )
 */

#define TG_FuncAddr		TR_Extra
#define TG_VALUE		TR_Stuff

/* For TRAK_WINDOW, set the TW_OTHER field if this window shares a msgport
 * and the msgport should not be freed
 */
#define TW_WINDOW		TR_Extra

/* NOTE - THINGS IN THIS LIST MUST NOT DEPEND ON THE TASK STILL EXISTING
 * Thus, no DeletePort or RemTask.  This is to allow a Flush executable
 * to be created which will go throug all of the Tracked list, and for
 * tasks that exited improperly, free the resources.
 */
/* Special Considerations
 *	The TR_Lock and TR_Flags bytes have special meaning.  These
 * are internally managed fields, which should not be touched by the
 * application.
 *	The TR_Lock field is managed by GetAccess/FreeAccess.  If
 * this field is -1, the resource being tracked may be freed at will
 * by the
 */
#define TRB_UNLINK		7		/* Bit for freeing the node */
#define TRB_RELOC		6		/* This element may be relocated!!!
					; ( This bit is not used yet ) */
#define TRB_MOVED		5		/* Bit set if the item moved. */

#define TRV_UNLINK		1<<TRB_UNLINK
#define TRV_RELOC		1<<TRB_RELOC
/**************************************************************************/



#endif /* ARP */
