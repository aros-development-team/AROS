#ifndef DOS_DOSEXTENS_H
#define DOS_DOSEXTENS_H

/*
    (C) 1995-97 AROS - The Amiga Replacement OS
    $Id$

    Desc: LibBase and some important structures
    Lang: english
*/
#ifndef AROS_CONFIG_H
#   include <aros/config.h>
#endif
#ifndef EXEC_TYPES_H
#   include <exec/types.h>
#endif
#ifndef EXEC_TASKS_H
#   include <exec/tasks.h>
#endif
#ifndef EXEC_PORTS_H
#   include <exec/ports.h>
#endif
#ifndef EXEC_LIBRARIES_H
#   include <exec/libraries.h>
#endif
#ifndef EXEC_SEMAPHORES
#   include <exec/semaphores.h>
#endif
#include <devices/timer.h>
#ifndef EXEC_INTERRUPTS_H
#   include <exec/interrupts.h>
#endif
#ifndef DOS_DOS_H
#   include <dos/dos.h>
#endif

/**********************************************************************
 ***************************** DosLibrary *****************************
 **********************************************************************/

/* This is how the base of dos.library looks like. */
struct DosLibrary
{
    /* A normal library-base as defined in <exec/libraries.h>. */
    struct Library dl_lib;

#if (AROS_FLAVOUR & AROS_FLAVOUR_BINCOMPAT)
    /* The following field is not used by AROS and is just there to
       guarantee binary compatibility. DO NOT USE THIS FIELD IN ANY WAY.
       Original names were: dl_Root, dl_GV, dl_A2, dl_A5 and dl_A6 */
    struct RootNode * NoAROS1;
    APTR              NoAROS2;
    LONG              NoAROS3[3];
#endif

    /* The following fields are PRIVATE! */
    struct ErrorString	 * dl_Errors;
    struct timerequest	 * dl_TimeReq;
    struct Library	 * dl_UtilityBase;
    struct Library	 * dl_IntuitionBase;

    /* These are AROS specific extensions. They are all PRIVATE! */
    struct SignalSemaphore dl_DosListLock;
    struct DosList	 * dl_DevInfo;
    struct ExecBase	 * dl_SysBase;
    BPTR		   dl_SegList;
    struct Device	 * dl_NulHandler;
    struct Unit 	 * dl_NulLock;

    struct SignalSemaphore dl_LDSigSem;
    struct Interrupt	   dl_LDHandler;
    APTR		   dl_LDOpenLibrary;
    APTR		   dl_LDOpenDevice;
    struct Process	 * dl_LDDemon;
    STRPTR		   dl_LDName;
    struct Process	 * dl_LDCaller;
    APTR		   dl_LDPtr;
    LONG		   dl_LDReturn;

    ULONG		   dl_ProcCnt;
    ULONG		   dl_Flags; /* see below */
};

/* The following structures in this sections are not used by AROS and may
   be removed in the future. So BEWARE, if you use them. */

struct RootNode
{
    BPTR             rn_TaskArray;
      /* (IPTR *) Pointer to an array containing pointers to CLI processes.
         The CLI process number is equal to the index of that array. The
         first field (index 0) contains the maximal number of CLI processes.
         See also rn_CliList. */
    BPTR             rn_ConsoleSegment;
      /* (void *) Pointer to the SegList for CLIs. */
    struct DateStamp rn_Time;
      /* The current time. */
    LONG             rn_RestartSeg;
      /* (APTR) The SegList of the process that handles validation of devices.
      */
    BPTR             rn_Info;
      /* (struct DosInfo *) see below for DosInfo */
    BPTR             rn_FileHandlerSegment;
    struct MinList   rn_CliList;
      /* List of all CLI processes (struct CliProcList - see below). See also
         rn_TaskArray. */
    struct MsgPort * rn_BootProc;
      /* Message port of boot filesystem. (PRIVATE) */
    BPTR             rn_ShellSegment;
      /* (void *) Pointer to the SegList for shells. */
    LONG             rn_Flags;
      /* Additional flags (see below). */
};

/* dl_Flags/rn_Flags */
#define RNB_WILDSTAR 24 /* Activate '*' as wildcard character. */
#define RNF_WILDSTAR (1L<<RNB_WILDSTAR)


/* This is a CLI node as pointed to by rn_CliList (see above). This structure
   is READ-ONLY. */
struct CliProcList
{
    struct MinNode    cpl_Node;
    LONG              cpl_First;
      /* The first CLI process number in this list. */
    struct MsgPort ** cpl_Array;
      /* This works like rn_TaskArray (see above), except that the index is
         equal to CLI process number + cpl_First. */
};

struct DosInfo
{
    BPTR di_McName;   /* PRIVATE */
    BPTR di_DevInfo;
    BPTR di_Devices;
    BPTR di_Handlers;
    APTR di_NetHand;

    /* The following semaphores are PRIVATE. */
    struct SignalSemaphore di_DevLock;
    struct SignalSemaphore di_EntryLock;
    struct SignalSemaphore di_DeleteLock;
};
#define di_ResList di_McName

/**********************************************************************
 ***************************** Processes ******************************
 **********************************************************************/

/* Standard process structure. Processes are just extended tasks. */
struct Process
{
    struct Task     pr_Task;
      /* Embedded task structure as defined in <exec/tasks.h>. */

    struct MsgPort  pr_MsgPort;
    WORD	    pr_Pad;     /* PRIVATE */
    BPTR	    pr_SegList;
      /* SegList array, used by this process. (void **) */
    LONG	    pr_StackSize;
      /* StackSize of the current process. */
    APTR	    pr_GlobVec;
    LONG	    pr_TaskNum;
      /* CLI process number. This may be 0, in which case the process is not
         connected to a CLI. */
    BPTR	    pr_StackBase;
      /* Pointer to upper end of stack. (void *) */
    LONG	    pr_Result2;
      /* Secondary return-value, as defined in <dos/dos.h>. As of now this
         field is declared PRIVATE. Use IoErr()/SetIoErr() to access it. */
    BPTR	    pr_CurrentDir;
      /* Lock of the current directory. As of now this is declared READ-ONLY.
         Use CurrentDir() to set it. (struct FileLock *) */
    BPTR	    pr_CIS;
      /* Standard input file. As of now this is declared WRITE-ONLY. Use
         Input() to query it. */
    BPTR	    pr_COS;
      /* Standard output file. As of now this is declared WRITE-ONLY. Use
         Output() to query it. */
    APTR	    pr_ConsoleTask;
      /* Task to handle the console associated with process. */
    APTR	    pr_FileSystemTask;
      /* The task that is responsible for handling the filesystem. */
    BPTR	    pr_CLI;
      /* CLI the process is connected to. (struct CommandLineInterface *) */
    APTR	    pr_ReturnAddr;
    APTR	    pr_PktWait;
      /* Function to be called, when process waits for a packet-message. */
    APTR	    pr_WindowPtr;
      /* Standard-Window of process. */
    BPTR	    pr_HomeDir;
      /* Lock to home-directory of process. (struct FileLock *) */
    LONG	    pr_Flags; /* see below */
    void	 (* pr_ExitCode)();
      /* Code that is called, when the process exits. pr_ExitData takes an
         argument to be passed to this code. */
    IPTR	    pr_ExitData;
    STRPTR	    pr_Arguments;
      /* Arguments passed to the process from caller. */
    struct MinList  pr_LocalVars;
      /* List of local environment variables. This list should be in
         alphabetical order. Multiple entries may have the same name, if they
         are of different types. See <dos/var.h> for more information. */
    ULONG	    pr_ShellPrivate;
    BPTR	    pr_CES;
      /* Standard error file. May be NULL, in which case pr_COS is to be used.
         Use this instead of Output() to report errors. */
};

/* pr_Flags (all PRIVATE) They mainly descibe what happens if the process
   exits, i.e. which resources the process should clean itself. The flags
   are self-explaining. */
#define PRB_FREESEGLIST 0
#define PRB_FREECURRDIR 1
#define PRB_FREECLI	2
#define PRB_CLOSEINPUT	3
#define PRB_CLOSEOUTPUT 4
#define PRB_FREEARGS	5
#define PRB_CLOSEERROR	6
#define PRF_FREESEGLIST (1L<<PRB_FREESEGLIST)
#define PRF_FREECURRDIR (1L<<PRB_FREECURRDIR)
#define PRF_FREECLI	(1L<<PRB_FREECLI)
#define PRF_CLOSEINPUT	(1L<<PRB_CLOSEINPUT)
#define PRF_CLOSEOUTPUT (1L<<PRB_CLOSEOUTPUT)
#define PRF_FREEARGS	(1L<<PRB_FREEARGS)
#define PRF_CLOSEERROR	(1L<<PRB_CLOSEERROR)

/* Structure used for CLIs and Shells. */
struct CommandLineInterface
{
    LONG cli_Result2;
      /* Secondary error code, set by last command. */
    BSTR cli_SetName;
      /* Name of the current directory. */
    BPTR cli_CommandDir;
      /* Lock of the first directory in path. (struct FileLock *) */
    LONG cli_ReturnCode;
      /* Error code, the last command returned. See <dos/dos.h> for
         definitions. */
    BSTR cli_CommandName;
      /* Name of the command that is currently executed. */
    LONG cli_FailLevel;
      /* Fail-Level as set by the command "FailAt". */
    BSTR cli_Prompt;
      /* Current prompt in the CLI window. */
    BPTR cli_StandardInput;
      /* Standard/Default input file. (struct FileLock *) */
    BPTR cli_CurrentInput;
      /* Current input file. (struct FileLock *) */
    BSTR cli_CommandFile;
      /* Name of the file that is currently executed. */
    LONG cli_Interactive;
      /* !=0 if the currently CLI is connected to a controlling terminal. */
    LONG cli_Background;
      /* !=0 if there is no controlling terminal. */
    BPTR cli_CurrentOutput;
      /* Current output file. (struct FileLock *) */
    LONG cli_DefaultStack;
      /* Dafault stack size as set by the command "Stack". */
    BPTR cli_StandardOutput;   
      /* Standard/Default output file. (struct FileLock *) */
    BPTR cli_Module;
      /* SegList of currently loaded command. */
};


/* Devices process structure as returned by GetDeviceProc(). */
struct DevProc
{
    struct MsgPort * dvp_Port;
    BPTR	     dvp_Lock;    /* struct FileLock * */
    ULONG	     dvp_Flags;   /* see below */
    struct DosList * dvp_DevNode; /* PRIVATE */
};

/* dvp_Flags */
#define DVPB_UNLOCK 0
#define DVPB_ASSIGN 1
#define DVPF_UNLOCK (1L<<DVPB_UNLOCK)
#define DVPF_ASSIGN (1L<<DVPB_ASSIGN)

/**********************************************************************
 ******************************* Files ********************************
 **********************************************************************/

/* Standard file-handle as returned by Open() (as BPTR). Generaly said, you
   should not use this structure in any way and only use library-calls to
   access files. Note that this structure is very different to the structure
   used in AmigaOS! Treat this structure as PRIVATE.
*/
struct FileHandle
{
#if (AROS_FLAVOUR & AROS_FLAVOUR_BINCOMPAT)
    /* The following three fields are not used by AROS and are just there for
       compatibility reasons. So keep your hands away. The original names
       were: fh_Link, fh_Port and fh_Type */
    struct Message * fh_NoAROS1;
    struct MsgPort * fh_NoAROS2[2];
#endif

    UBYTE * fh_Buf;
    UBYTE * fh_Pos;
    UBYTE * fh_End;

    /* The following four fields have different names and a different
       function than their AmigaOS equivalents. The original names were:
       fh_Funcs/fh_Func1, fh_Func2, fh_Func3, fh_Args/fh_Arg1 and fh_Arg2 */
    ULONG	    fh_Size;
    ULONG	    fh_Flags;   /* see below */
    struct Device * fh_Device;
    struct Unit   * fh_Unit;
#if (AROS_FLAVOUR & AROS_FLAVOUR_BINCOMPAT)
    LONG            fh_NoAROS3; /* not used in AROS */
#endif
};

/* fh_Flags. The flags are AROS specific and therefore PRIVATE.. */
#define FHF_WRITE (~0UL/2+1)
#define FHF_BUF   1


/* Structure of a lock, as returned by Lock() and similar functions. */
struct FileLock
{
    BPTR             fl_Link; /* (struct FileLock *) Pointer to next lock. */
    LONG             fl_Key;
    LONG             fl_Access;
    struct MsgPort * fl_Task;
    BPTR             fl_Volume; /* (struct DeviceList * - see below) */
};


/* Constants, defining of what kind a file is. These constants are used in
   many structures, including FileInfoBlock (<dos/dos.h>) and ExAllData
   (<dos/exall.h>). */
#define ST_PIPEFILE -5 /* File is a pipe */
#define ST_LINKFILE -4 /* Hard link to a file */
#define ST_FILE     -3 /* Plain file */
#define ST_ROOT      1 /* Root directory of filesystem */
#define ST_USERDIR   2 /* Normal directory */
#define ST_SOFTLINK  3 /* Soft link (may be a file or directory) */
#define ST_LINKDIR   4 /* Hard link to a directory */

/**********************************************************************
 ****************************** DosLists ******************************
 **********************************************************************/

/* This structure is returned by LockDosList() and similar calls. This
   structure is different to the AmigaOS one. But this structure is PRIVATE
   anyway. Use system-calls for dos list-handling. */
struct DosList
{
    struct DosList * dol_Next;
      /* PRIVATE pointer to next entry. In AmigaOS this used to be a BPTR. */
    LONG             dol_Type;
      /* Type of the current node (see below). */

#if (AROS_FLAVOUR & AROS_FLAVOUR_BINCOMPAT)
    /* The next two fields are not used by AROS. Their original name was:
       dol_Task and dol_Lock */
    APTR dol_NoAROS1;
    BPTR dol_NoAROS2;
#endif

    IPTR             dol_Union1[3];
    LONG             dol_Union2[3];

    BSTR dol_OldName;
      /* This field is called dol_Name in AmigaOS. It is now named dol_OldName
         to give you a hint that something has changed. Additionally to the
         old nasty BSTR there is now a new clean STRPTR for the same purpose.
         You may want to:
         1. Change your sources to reflect this change thus getting rid of
            all BCPL stuff or
         2. just define dol_OldName to dol_Name before including this file
            to stay downwards compatible. */

    /* The following fields are new to AROS. */
    STRPTR	    dol_DevName;
    struct Device * dol_Device;
    struct Unit   * dol_Unit;
};
/* #define dol_Name dol_OldName */

/* dol_Type/dl_Type/dvi_Type. Given to MakeDosEntry(). */
#define DLT_DEVICE     0 /* A real filesystem (or similar) */
#define DLT_DIRECTORY  1 /* Just a simple assign */
#define DLT_VOLUME     2 /* Volume node (for removable media) */
#define DLT_LATE       3 /* Late binding assign (not yet) */
#define DLT_NONBINDING 4 /* Nonbinding assign (not yet) */


/* The following structures are essentially the same as DosList above. The
   difference is that they support just one type of entry. You can use them
   instead of DosList, if you have a list, containing just one type of
   entries. For more information see above. */

/* Structure that describes a volume. */
struct DeviceList
{
    struct DeviceList * dl_Next;
    LONG                dl_Type; /* see above, always = DLT_VOLUME */

#if (AROS_FLAVOUR & AROS_FLAVOUR_BINCOMPAT)
    struct MsgPort * dl_NoAROS1;
    BPTR             dl_NoAROS2;
#endif

    struct DateStamp dl_VolumeDate;
      /* Embedded DateStamp structured as defined in <dos/dos.h>. At this
         date the volume was created. */
    BPTR             dl_LockList;
      /* (void *) List of all locks on the volume. */
    LONG             dl_DiskType;
      /* Type of the disk. (see <dos/dos.h> for definitions) */
    IPTR             dl_unused; /* PRIVATE */
    /* ATTENTION: This struture does currently work on 32bit computers only
       due to the fact that dl_unused does not compensate the missing pointers
       in this structure. In DevInfo we have three pointer and three longwords,
       while in this structure we have only two pointers and four longwords.
    */

    BSTR dl_OldName;

    STRPTR	    dl_DevName;
    struct Device * dl_Device;
    struct Unit   * dl_Unit;
};
/* #define dl_Name dl_OldName */


/* Structure that describes a device. This is essentially the same structure
   as DeviceNode, defined in <dos/filehandler.h>. */
struct DevInfo
{
    struct DevInfo * dvi_Next;
    LONG             dvi_Type; /* see above, always = DLT_DEVICE */

#if (AROS_FLAVOUR & AROS_FLAVOUR_BINCOMPAT)
    struct MsgPort * dvi_NoAROS1;
    BPTR             dvi_NoAROS2;
#endif

    BSTR dvi_Handler;    /* Device name for handler. */
    LONG dvi_NoAROS3[2]; /* PRIVATE */
    BPTR dvi_Startup;    /* (struct FileSysStartupMsg * - defined in
                            <dos/filehandler.h>) */
    BPTR dvi_NoAROS4[2]; /* PRIVATE */

    BSTR dvi_OldName;

    STRPTR	    dvi_DevName;
    struct Device * dvi_Device;
    struct Unit   * dvi_Unit;
};
/* #define dvi_Name dvi_OldName */


/* Dos list scanning and locking modes as used in LockDosList() */
/* Specify either LDF_READ, if you want a non-exclusive lock, or LDF_WRITE,
   if you want an exclusive lock (i.e. if you want to modify the list).
*/
#define LDB_READ    0 /* Non-exclusive/read lock */
#define LDB_WRITE   1 /* Exclusive/write lock */
/* Specify which list(s) to lock. */
#define LDB_DEVICES 2 /* Device list */
#define LDB_VOLUMES 3 /* Volume list */
#define LDB_ASSIGNS 4 /* Assign list */
#define LDB_ENTRY   5
#define LDB_DELETE  6

#define LDF_READ    (1L<<LDB_READ)
#define LDF_WRITE   (1L<<LDB_WRITE)
#define LDF_DEVICES (1L<<LDB_DEVICES)
#define LDF_VOLUMES (1L<<LDB_VOLUMES)
#define LDF_ASSIGNS (1L<<LDB_ASSIGNS)
#define LDF_ENTRY   (1L<<LDB_ENTRY)
#define LDF_DELETE  (1L<<LDB_DELETE)
#define LDF_ALL     (LDF_DEVICES | LDF_VOLUMES | LDF_ASSIGNS)


/* Used for assigns that point to multiple directories. */
struct AssignList
{
    struct AssignList * al_Next; /* Pointer to next assign node. */
    BPTR                al_Lock; /* (struct FileLock *) Lock of on of the
                                    directories. */
};

/**********************************************************************
 ********************** Low Level File Handling ***********************
 **********************************************************************/

struct DosPacket
{
   struct Message * dp_Link; /* Pointer to a standard exec message. */
   struct MsgPort * dp_Port; /* Reply-Port of that packet. */

   LONG dp_Type; /* see below */
   LONG dp_Res1; /* Normal return value. */
   LONG dp_Res2; /* Secondary return value (as returned by IoErr()). */

   /* The actual data. */
   LONG dp_Arg1;
   LONG dp_Arg2;
   LONG dp_Arg3;
   LONG dp_Arg4;
   LONG dp_Arg5;
   LONG dp_Arg6;
   LONG dp_Arg7;
};
#define dp_Action   dp_Type
#define dp_Status   dp_Res1
#define dp_Status2  dp_Res2
#define dp_BufAddr  dp_Arg1

/* dp_Type */
#define ACTION_NIL             0
#define ACTION_STARTUP         0
#define ACTION_SET_MAP         4
#define ACTION_DIE             5
#define ACTION_EVENT           6
#define ACTION_CURRENT_VOLUME  7
#define ACTION_LOCATE_OBJECT   8
#define ACTION_RENAME_DISK     9
#define ACTION_FREE_LOCK      15
#define ACTION_DELETE_OBJECT  16
#define ACTION_RENAME_OBJECT  17
#define ACTION_MORE_CACHE     18
#define ACTION_COPY_DIR       19
#define ACTION_WAIT_CHAR      20
#define ACTION_SET_PROTECT    21
#define ACTION_CREATE_DIR     22
#define ACTION_EXAMINE_OBJECT 23
#define ACTION_EXAMINE_NEXT   24
#define ACTION_DISK_INFO      25
#define ACTION_INFO           26
#define ACTION_FLUSH          27
#define ACTION_SET_COMMENT    28
#define ACTION_PARENT         29
#define ACTION_TIMER          30
#define ACTION_INHIBIT        31
#define ACTION_DISK_TYPE      32
#define ACTION_DISK_CHANGE    33
#define ACTION_SET_DATE       34
#define ACTION_SAME_LOCK      40

#define ACTION_WRITE 'W'
#define ACTION_READ  'R'

#define ACTION_SCREEN_MODE      994
#define ACTION_CHANGE_SIGNAL    995
#define ACTION_READ_RETURN     1001
#define ACTION_WRITE_RETURN    1002
#define ACTION_FINDUPDATE      1004
#define ACTION_FINDINPUT       1005
#define ACTION_FINDOUTPUT      1006
#define ACTION_END             1007
#define ACTION_SEEK            1008
#define ACTION_FORMAT          1020
#define ACTION_MAKE_LINK       1021
#define ACTION_SET_FILE_SIZE   1022
#define ACTION_WRITE_PROTECT   1023
#define ACTION_READ_LINK       1024
#define ACTION_FH_FROM_LOCK    1026
#define ACTION_IS_FILESYSTEM   1027
#define ACTION_CHANGE_MODE     1028
#define ACTION_COPY_DIR_FH     1030
#define ACTION_PARENT_FH       1031
#define ACTION_EXAMINE_ALL     1033
#define ACTION_EXAMINE_FH      1034
#define ACTION_EXAMINE_ALL_END 1035
#define ACTION_SET_OWNER       1036

#define ACTION_LOCK_RECORD   2008
#define ACTION_FREE_RECORD   2009

#define ACTION_ADD_NOTIFY    4097
#define ACTION_REMOVE_NOTIFY 4098

#define ACTION_SERIALIZE_DISK 4200

/* Special packet types for shell-startup. */
#define RUN_EXECUTE       -1
#define RUN_SYSTEM        -2
#define RUN_SYSTEM_ASYNCH -3

/* Structure for easy handling of DosPackets. DosPackets don´t have to be in
   this structure, but this struture may ease the use of it. */
struct StandardPacket
{
    struct Message   sp_Msg;
    struct DosPacket sp_Pkt;
};

#define CMD_SYSTEM	-1
#define CMD_INTERNAL	-2
#define CMD_DISABLED	-999

/**********************************************************************
 ****************************** Segments ******************************
 **********************************************************************/

/* Resident list structure as returned by AddSegment(). */
struct Segment
{
    BPTR  seg_Next;    /* Pointer to next segment. */
    LONG  seg_UC;
    BPTR  seg_Seg;
    UBYTE seg_Name[4]; /* The first characters of the name (BSTR). */
};

/**********************************************************************
 *************************** Error Handling ***************************
 **********************************************************************/

struct ErrorString
{
    LONG * estr_Nums;
    STRPTR estr_Strings;
};

/* Return values for ErrorReport(). */
#define REPORT_STREAM 0
#define REPORT_TASK   1
#define REPORT_LOCK   2
#define REPORT_VOLUME 3
#define REPORT_INSERT 4

#define ABORT_BUSY       288
#define ABORT_DISK_ERROR 296

#endif /* DOS_DOSEXTENS_H */
