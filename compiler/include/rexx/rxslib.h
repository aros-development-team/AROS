#ifndef REXX_RXSLIB_H
#define REXX_RXSLIB_H

/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: ARexx data structures
    Lang: English
*/

#ifndef EXEC_EXECBASE_H
#include <exec/execbase.h>
#endif

#ifndef REXX_STORAGE_H
#include <rexx/storage.h>
#endif

#define RXSNAME  "rexxsyslib.library"
#define RXSDIR	 "REXX"
#define RXSTNAME "ARexx"

/* RxsLib is only here to provide backwards compatibility with Amiga
 * programs. This structure should be considered read-only as a whole.
 * Only use the functions of rexxsyslib.library or send the appropriate
 * command to the REXX port if you want to change something in
 * this structure.
 */
struct RxsLib
{
	struct Library     rl_Node;
	UBYTE              rl_Flags;
	UBYTE              rl_Shadow;
	struct ExecBase *  rl_SysBase;
	struct DOSBase *   rl_DOSBase;
	struct Library *   rl_Unused1; /* rl_IeeeCDBase */
	BPTR               rl_SegList;
	struct FileHandle *rl_Unused2; /* rl_NIL */
	LONG               rl_Unused3; /* rl_Chunk */
	LONG               rl_Unused4; /* rl_MaxNest */
	APTR               rl_Unused5; /* rl_NULL */
	APTR               rl_Unused6; /* rl_FALSE */
	APTR               rl_Unused7; /* rl_TRUE */
	APTR               rl_Unused8; /* rl_REXX */
	APTR               rl_Unused9; /* rl_COMMAND */
	APTR               rl_Unused10; /* rl_STDIN */
	APTR               rl_Unused11; /* rl_STDOUT */
	APTR               rl_Unused12; /* rl_STDERR */
	STRPTR             rl_Version;
	STRPTR             rl_Unused13; /* rl_TaskName */
	LONG               rl_Unused14; /* rl_TaskPri */
	LONG               rl_Unused15; /* rl_TaskSeg */
	LONG               rl_Unused16; /* rl_StackSize */
	STRPTR             rl_Unused17; /* rl_RexxDir */
	STRPTR             rl_Unused18; /* rl_CTABLE */
	STRPTR             rl_Notice; /* The copyright notice */
	struct MsgPort     rl_Unused19; /* rl_REXX public port */
	UWORD              rl_Unused20; /* rl_ReadLock */
	LONG               rl_Unused21; /* rl_TraceFH */
	struct List        rl_Unused22; /* rl_TaskList */
	WORD               rl_Unused23; /* rl_NumTask */
	struct List        rl_LibList; /* Library list header */
	WORD               rl_NumLib; /* Nodes count in library list */
	struct List        rl_ClipList; /* Clip list header */
	WORD               rl_NumClip; /* Nodes count in clip list */
	struct List        rl_Unused24; /* rl_MsgList */
	WORD               rl_Unused25; /* rl_NumMsg */
	struct List        rl_Unused26; /* rl_PgmList */
	WORD               rl_Unused27; /* rl_NumPgm */
	UWORD              rl_Unused28; /* rl_TraceCnt */
	WORD               rl_Unused29; /* rl_Avail */
};

/* These are not necessary for client program either I think
#define RLFB_TRACE  RTFB_TRACE
#define RLFB_HALT   RTFB_HALT
#define RLFB_SUSP   RTFB_SUSP
#define RLFB_STOP   6
#define RLFB_CLOSE  7

#define RLFMASK     ((1<<RLFB_TRACE) | (1<<RLFB_HALT) | (1<<RLFB_SUSP))

#define RXSCHUNK    1024
#define RXSNEST     32
#define RXSTPRI     0
#define RXSSTACK    4096
*/

/* I'm not sure about these ones but let's dissable them for now
#define CTB_SPACE   0
#define CTB_DIGIT   1
#define CTB_ALPHA   2
#define CTB_REXXSYM 3
#define CTB_REXXOPR 4
#define CTB_REXXSPC 5
#define CTB_UPPER   6
#define CTB_LOWER   7

#define CTF_SPACE   (1 << CTB_SPACE)
#define CTF_DIGIT   (1 << CTB_DIGIT)
#define CTF_ALPHA   (1 << CTB_ALPHA)
#define CTF_REXXSYM (1 << CTB_REXXSYM)
#define CTF_REXXOPR (1 << CTB_REXXOPR)
#define CTF_REXXSPC (1 << CTB_REXXSPC)
#define CTF_UPPER   (1 << CTB_UPPER)
#define CTF_LOWER   (1 << CTB_LOWER)
*/

#endif
