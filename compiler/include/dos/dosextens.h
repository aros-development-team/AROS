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
};

struct CommandLineInterface
{
    LONG cli_Result2;
    BSTR cli_SetName;
    BPTR cli_CommandDir;
    LONG cli_ReturnCode;
    BSTR cli_CommandName;
    LONG cli_FailLevel;
    BSTR cli_Prompt;
    BPTR cli_StandardInput;
    BPTR cli_CurrentInput;
    BSTR cli_CommandFile;
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
#define	PRB_FREESEGLIST	0
#define	PRF_FREESEGLIST	1
#define	PRB_FREECURRDIR	1
#define	PRF_FREECURRDIR	2
#define	PRB_FREECLI	2
#define	PRF_FREECLI	4
#define	PRB_CLOSEINPUT	3
#define	PRF_CLOSEINPUT	8
#define	PRB_CLOSEOUTPUT	4
#define	PRF_CLOSEOUTPUT	16
#define	PRB_FREEARGS	5
#define	PRF_FREEARGS	32
#define PRB_CLOSEERROR	6
#define PRF_CLOSEERROR	64

#define LDB_READ	0
#define LDF_READ	1
#define LDB_WRITE	1
#define LDF_WRITE	2
#define LDB_DEVICES	2
#define LDF_DEVICES	4
#define LDB_VOLUMES	3
#define LDF_VOLUMES	8
#define LDB_ASSIGNS	4
#define LDF_ASSIGNS	16
#define LDB_ENTRY	5
#define LDF_ENTRY	32
#define LDB_DELETE	6
#define LDF_DELETE	64
#define LDF_ALL		(LDF_DEVICES|LDF_VOLUMES|LDF_ASSIGNS)

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

#define FHF_WRITE	(~0ul/2+1)
#define FHF_BUF		1

struct DosList
{
    struct DosList *dol_Next;
    LONG dol_Type;
    APTR dol_Dummy1;
    LONG dol_Dummy2;
    LONG dol_Dummy3[6];
    BSTR dol_OldName;		/* Compatibility */
    STRPTR dol_Name;
    struct Device *dol_Device;
    struct Unit *dol_Unit;
};

/* dl_Type type values */
#define DLT_DEVICE	0
#define DLT_DIRECTORY	1
#define DLT_VOLUME	2
#define DLT_LATE	3 /* Not yet */
#define DLT_NONBINDING	4 /* Not yet */

#define ST_ROOT		1
#define ST_USERDIR	2
#define ST_SOFTLINK	3
#define ST_LINKDIR	4
#define ST_FILE		-3
#define ST_LINKFILE	-4
#define ST_PIPEFILE	-5

#endif
