#ifndef _DOS_DOSEXTENS_H_
#define _DOS_DOSEXTENS_H_

#include <exec/tasks.h>
#include <exec/ports.h>
#include <exec/libraries.h>
#include <exec/semaphores.h>
#include <devices/timer.h>
#include <exec/interrupts.h>
#include <dos/dos.h>

struct DosLibrary
{
    /* No public fields in there */
    struct Library dl_lib;
    struct ErrorString *dl_Errors;
    struct timerequest *dl_TimeReq;
    struct Library *dl_UtilityBase;
    struct Library *dl_IntuitionBase;
    struct SignalSemaphore dl_DosListLock;
    struct DosList *dl_DevInfo;
    struct ExecBase *dl_SysBase;
    BPTR dl_SegList;
    struct Device *dl_NulHandler;
    struct Unit *dl_NulLock;

    struct SignalSemaphore dl_LDSigSem;
    struct Interrupt dl_LDHandler;
    APTR dl_LDOpenLibrary;
    APTR dl_LDOpenDevice;
    struct Process *dl_LDDemon;
    STRPTR dl_LDName;
    struct Process *dl_LDCaller;
    APTR dl_LDPtr;
    LONG dl_LDReturn;

    ULONG dl_ProcCnt;
    ULONG dl_Flags;
};

/* dl_Flags values */
#define RNF_WILDSTAR 0x1000000	/* Activate '*' as wildcard character */

struct CommandLineInterface
{
    LONG cli_Result2;
    BPTR cli_SetName;
    BPTR cli_CommandDir;
    LONG cli_ReturnCode;
    BPTR cli_CommandName;
    LONG cli_FailLevel;
    BPTR cli_Prompt;
    BPTR cli_StandardInput;
    BPTR cli_CurrentInput;
    BPTR cli_CommandFile;
    LONG cli_Interactive;
    LONG cli_Background;
    BPTR cli_CurrentOutput;
    LONG cli_DefaultStack;
    BPTR cli_StandardOutput;
    BPTR cli_Module;
};

struct Process
{
    struct Task pr_Task;
    struct MsgPort pr_MsgPort;
    WORD pr_Pad;
    BPTR pr_SegList;
    LONG pr_StackSize;
    APTR pr_GlobVec;
    LONG pr_TaskNum;
    BPTR pr_StackBase;
    LONG pr_Result2;
    BPTR pr_CurrentDir;
    BPTR pr_CIS;
    BPTR pr_COS;
    APTR pr_ConsoleTask;
    APTR pr_FileSystemTask;
    BPTR pr_CLI;
    APTR pr_ReturnAddr;
    APTR pr_PktWait;
    APTR pr_WindowPtr;
    BPTR pr_HomeDir;
    LONG pr_Flags;
    void (*pr_ExitCode)();
    LONG pr_ExitData;
    STRPTR pr_Arguments;
    struct MinList pr_LocalVars;
    ULONG pr_ShellPrivate;
    BPTR pr_CES;
};

/* pr_Flags (all private) */
#define PRB_FREESEGLIST 0
#define PRB_FREECURRDIR 1
#define PRB_FREECLI	2
#define PRB_CLOSEINPUT	3
#define PRB_CLOSEOUTPUT 4
#define PRB_FREEARGS	5
#define PRB_CLOSEERROR	6
#define PRF_FREESEGLIST 0x1
#define PRF_FREECURRDIR 0x2
#define PRF_FREECLI	0x4
#define PRF_CLOSEINPUT	0x8
#define PRF_CLOSEOUTPUT 0x10
#define PRF_FREEARGS	0x20
#define PRF_CLOSEERROR	0x40

/* Dos list scanning and locking modes */
#define LDB_READ	0
#define LDB_WRITE	1
#define LDB_DEVICES	2
#define LDB_VOLUMES	3
#define LDB_ASSIGNS	4
#define LDB_ENTRY	5
#define LDB_DELETE	6
#define LDF_READ	0x1
#define LDF_WRITE	0x2
#define LDF_DEVICES	0x4
#define LDF_VOLUMES	0x8
#define LDF_ASSIGNS	0x10
#define LDF_ENTRY	0x20
#define LDF_DELETE	0x40
#define LDF_ALL 	0x1c

struct FileHandle
{
    APTR fh_Dummy1;
    APTR fh_Dummy2;
    APTR fh_Dummy3;
    UBYTE *fh_Buf;
    UBYTE *fh_Pos;
    UBYTE *fh_End;
    ULONG fh_Size;
    ULONG fh_Flags;
    struct Device *fh_Device;
    struct Unit *fh_Unit;
    LONG fh_Dummy4;
};

/* Private fh_Flags values */
#define FHF_WRITE	(~0ul/2+1)
#define FHF_BUF 	1

struct DosList
{
    struct DosList *dol_Next;	/* Private pointer to next entry */
    LONG dol_Type;		/* Node types (see below) */
    APTR dol_Dummy1;
    LONG dol_Dummy2[7];

    /*
	This field once was named dol_Name. It is now named dol_OldName
	to give you a hint that something has changed. Additionally to the
	old nasty BSTR there is now a new clean STRPTR for the same purpose.
	You may want to:
	1. Change your sources to reflect this change thus getting rid of
	   all BCPL stuff or
	2. just define dol_OldName to dol_Name before including this file
	   to stay downwards compatible.
    */
    BPTR dol_OldName;		/* Old field */
    STRPTR dol_DevName; /* New field (in fact pointing to the same string) */
    struct Device *dol_Device;
    struct Unit *dol_Unit;
};

/* dol_Type type values. Given to MakeDosEntry(). */
#define DLT_DEVICE	0	/* A real filesystem (or similar) */
#define DLT_DIRECTORY	1	/* Just a simple assign */
#define DLT_VOLUME	2	/* Volume node (for removable media) */
#define DLT_LATE	3	/* Late binding assign (not yet) */
#define DLT_NONBINDING	4	/* Nonbinding assign (not yet) */

/* Directory entry types */
#define ST_ROOT 	1	/* Root directory of filesystem */
#define ST_USERDIR	2	/* Normal directory */
#define ST_SOFTLINK	3	/* Soft link */
#define ST_LINKDIR	4	/* Hard link to a directory */
#define ST_FILE 	-3	/* Normal file */
#define ST_LINKFILE	-4	/* Hard link to a file */
#define ST_PIPEFILE	-5	/* Special file */

struct DosPacket {
   struct Message *dp_Link;
   struct MsgPort *dp_Port;
   LONG dp_Type;
   LONG dp_Res1;
   LONG dp_Res2;
   LONG dp_Arg1;
   LONG dp_Arg2;
   LONG dp_Arg3;
   LONG dp_Arg4;
   LONG dp_Arg5;
   LONG dp_Arg6;
   LONG dp_Arg7;
}; /* DosPacket */

#define dp_Action  dp_Type
#define dp_Status  dp_Res1
#define dp_Status2 dp_Res2
#define dp_BufAddr dp_Arg1

/*
    Structure of the Dos resident list. Do NOT allocate it, use
    AddSegment(), and have a loot at the autodocs!
*/

struct Segment
{
    BPTR seg_Next;
    LONG seg_UC;
    BPTR seg_Seg;
    UBYTE seg_Name[4]; /* actually the first 4 chars of BSTR name */
};

#define CMD_SYSTEM	-1
#define CMD_INTERNAL	-2
#define CMD_DISABLED	-999


#endif
