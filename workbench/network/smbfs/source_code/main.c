/*
 * $Id: main.c,v 1.9 2009/04/14 12:16:11 obarthel Exp $
 *
 * :ts=4
 *
 * SMB file system wrapper for AmigaOS, using the AmiTCP V3 API
 *
 * Copyright (C) 2000-2009 by Olaf `Olsen' Barthel <obarthel -at- gmx -dot- net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include "smbfs.h"

/****************************************************************************/

#include "smb_abstraction.h"

/****************************************************************************/

#include <smb/smb.h>

/****************************************************************************/

#if defined(__amigaos4__) && !defined(Flush)
#define Flush(fh) FFlush(fh)
#endif /* __amigaos4__ && !Flush */

/****************************************************************************/

/* A quick workaround for the timeval/timerequest->TimeVal/TimeRequest
   change in the recent OS4 header files. */
#if defined(__NEW_TIMEVAL_DEFINITION_USED__)

#define timeval		TimeVal
#define tv_secs		Seconds
#define tv_micro	Microseconds

#define timerequest	TimeRequest
#define tr_node		Request
#define tr_time		Time

#endif /* __NEW_TIMEVAL_DEFINITION_USED__ */

/****************************************************************************/

/* This is for backwards compatibility only. */
#if defined(__amigaos4__)
#define fib_EntryType fib_Obsolete
#endif /* __amigaos4__ */

/****************************************************************************/

#include "smbfs_rev.h"
STRPTR Version = VERSTAG;

/****************************************************************************/

#define UNIX_TIME_OFFSET 252460800
#define MAX_FILENAME_LEN 256

/****************************************************************************/

#define SMB_ROOT_DIR_NAME	"\\"
#define SMB_PATH_SEPARATOR	'\\'

/****************************************************************************/

typedef STRPTR	KEY;
typedef LONG *	NUMBER;
typedef LONG	SWITCH;

/****************************************************************************/

struct FileNode
{
	struct MinNode		fn_MinNode;
	struct FileHandle *	fn_Handle;
	LONG				fn_Offset;
	LONG				fn_Mode;
	smba_file_t *		fn_File;
	STRPTR				fn_FullName;
};

struct LockNode
{
	struct MinNode	ln_MinNode;
	struct FileLock	ln_FileLock;
	smba_file_t *	ln_File;
	BOOL			ln_RestartExamine;
	UWORD			ln_Pad;
	STRPTR			ln_FullName;
};

/****************************************************************************/

/* The minimum operating system version we require to work. */
#define MINIMUM_OS_VERSION 37		/* Kickstart 2.04 or better */
/*#define MINIMUM_OS_VERSION 39*/	/* Kickstart 3.0 or better */

/****************************************************************************/

/* Careful: the memory pool routines in amiga.lib are available only to
 *          SAS/C and similar compilers (not necessarily to GCC).
 */
#if defined(__GNUC__) && (MINIMUM_OS_VERSION < 39)

#undef MINIMUM_OS_VERSION
#define MINIMUM_OS_VERSION 39

#endif /* __GNUC__ */

/****************************************************************************/

#if (MINIMUM_OS_VERSION < 39)

/* These are in amiga.lib */
APTR ASM AsmCreatePool(REG(d0,ULONG memFlags),REG(d1,ULONG puddleSize),REG(d2,ULONG threshSize),REG(a6,struct Library * SysBase));
VOID ASM AsmDeletePool(REG(a0,APTR poolHeader),REG(a6,struct Library * SysBase));
APTR ASM AsmAllocPooled(REG(a0,APTR poolHeader),REG(d0,ULONG memSize),REG(a6,struct Library * SysBase));
VOID ASM AsmFreePooled(REG(a0,APTR poolHeader),REG(a1,APTR memory),REG(d0,ULONG memSize),REG(a6,struct Library * SysBase));

#define CreatePool(memFlags,puddleSize,threshSize) AsmCreatePool((memFlags),(puddleSize),(threshSize),SysBase)
#define DeletePool(poolHeader) AsmDeletePool((poolHeader),SysBase)
#define AllocPooled(poolHeader,memSize) AsmAllocPooled((poolHeader),(memSize),SysBase)
#define FreePooled(poolHeader,memory,memSize) AsmFreePooled((poolHeader),(memory),(memSize),SysBase)

#endif /* MINIMUM_OS_VERSION */

/****************************************************************************/

/* Forward declarations for local routines. */
#if !defined(__AROS__)
LONG _start(VOID);
#endif
LONG VARARGS68K LocalPrintf(STRPTR format, ...);
STRPTR amitcp_strerror(int error);
STRPTR host_strerror(int error);
LONG CompareNames(STRPTR a, STRPTR b);
VOID StringToUpper(STRPTR s);
#ifndef __AROS__
VOID VARARGS68K ReportError(STRPTR fmt, ...);
#endif
VOID FreeMemory(APTR address);
APTR AllocateMemory(ULONG size);
LONG GetTimeZoneDelta(VOID);
ULONG GetCurrentTime(VOID);
VOID GMTime(time_t seconds, struct tm *tm);
time_t MakeTime(const struct tm *const tm);
#ifndef __AROS__
VOID VARARGS68K SPrintf(STRPTR buffer, STRPTR formatString, ...);
#endif
int BroadcastNameQuery(char *name, char *scope, UBYTE *address);

/****************************************************************************/

INLINE STATIC BOOL ReallyRemoveDosEntry(struct DosList *entry);
INLINE STATIC LONG BuildFullName(STRPTR parent_name, STRPTR name, STRPTR *result_ptr, LONG *result_size_ptr);
INLINE STATIC VOID TranslateCName(UBYTE *name, UBYTE *map);
INLINE STATIC VOID ConvertCString(LONG max_len, APTR bstring, STRPTR cstring);
STATIC VOID DisplayErrorList(VOID);
STATIC VOID AddError(STRPTR fmt, APTR args);
STATIC LONG CVSPrintf(STRPTR format_string, APTR args);
STATIC VOID VSPrintf(STRPTR buffer, STRPTR formatString, APTR args);
STATIC VOID SendDiskChange(ULONG class);
STATIC struct FileNode *FindFileNode(STRPTR name, struct FileNode *skip);
STATIC struct LockNode *FindLockNode(STRPTR name, struct LockNode *skip);
STATIC LONG CheckAccessModeCollision(STRPTR name, LONG mode);
STATIC LONG NameAlreadyInUse(STRPTR name);
STATIC BOOL IsReservedName(STRPTR name);
STATIC LONG MapErrnoToIoErr(int error);
STATIC VOID TranslateBName(UBYTE *name, UBYTE *map);
STATIC VOID Cleanup(VOID);
STATIC BOOL Setup(STRPTR program_name, STRPTR service, STRPTR workgroup, STRPTR username, STRPTR opt_password, BOOL opt_changecase, STRPTR opt_clientname, STRPTR opt_servername, int opt_cachesize, LONG *opt_time_zone_offset, LONG *opt_dst_offset, STRPTR device_name, STRPTR volume_name, STRPTR translation_file);
STATIC VOID ConvertBString(LONG max_len, STRPTR cstring, APTR bstring);
STATIC BPTR Action_Parent(struct FileLock *parent, SIPTR *error_ptr);
STATIC LONG Action_DeleteObject(struct FileLock *parent, APTR bcpl_name, SIPTR *error_ptr);
STATIC BPTR Action_CreateDir(struct FileLock *parent, APTR bcpl_name, SIPTR *error_ptr);
STATIC BPTR Action_LocateObject(struct FileLock *parent, APTR bcpl_name, LONG mode, SIPTR *error_ptr);
STATIC BPTR Action_CopyDir(struct FileLock *lock, SIPTR *error_ptr);
STATIC LONG Action_FreeLock(struct FileLock *lock, SIPTR *error_ptr);
STATIC LONG Action_SameLock(struct FileLock *lock1, struct FileLock *lock2, SIPTR *error_ptr);
STATIC LONG Action_SetProtect(struct FileLock *parent, APTR bcpl_name, LONG mask, SIPTR *error_ptr);
STATIC LONG Action_RenameObject(struct FileLock *source_lock, APTR source_bcpl_name, struct FileLock *destination_lock, APTR destination_bcpl_name, SIPTR *error_ptr);
STATIC LONG Action_DiskInfo(struct InfoData *id, SIPTR *error_ptr);
STATIC LONG Action_Info(struct FileLock *lock, struct InfoData *id, SIPTR *error_ptr);
STATIC LONG Action_ExamineObject(struct FileLock *lock, struct FileInfoBlock *fib, SIPTR *error_ptr);
STATIC BOOL NameIsAcceptable(STRPTR name, LONG max_len);
STATIC LONG Action_ExamineNext(struct FileLock *lock, struct FileInfoBlock *fib, SIPTR *error_ptr);
STATIC LONG Action_ExamineAll(struct FileLock *lock, struct ExAllData *ed, ULONG size, ULONG type, struct ExAllControl *eac, SIPTR *error_ptr);
STATIC LONG Action_Find(LONG action, struct FileHandle *fh, struct FileLock *parent, APTR bcpl_name, SIPTR *error_ptr);
STATIC LONG Action_Read(struct FileNode *fn, APTR mem, LONG length, SIPTR *error_ptr);
STATIC LONG Action_Write(struct FileNode *fn, APTR mem, LONG length, SIPTR *error_ptr);
STATIC LONG Action_End(struct FileNode *fn, SIPTR *error_ptr);
STATIC LONG Action_Seek(struct FileNode *fn, LONG position, LONG mode, SIPTR *error_ptr);
STATIC LONG Action_SetFileSize(struct FileNode *fn, LONG position, LONG mode, SIPTR *error_ptr);
STATIC LONG Action_SetDate(struct FileLock *parent, APTR bcpl_name, struct DateStamp *ds, SIPTR *error_ptr);
STATIC LONG Action_ExamineFH(struct FileNode *fn, struct FileInfoBlock *fib, SIPTR *error_ptr);
STATIC BPTR Action_ParentFH(struct FileNode *fn, SIPTR *error_ptr);
STATIC BPTR Action_CopyDirFH(struct FileNode *fn, SIPTR *error_ptr);
STATIC LONG Action_FHFromLock(struct FileHandle *fh, struct FileLock *fl, SIPTR *error_ptr);
STATIC LONG Action_RenameDisk(APTR bcpl_name, SIPTR *error_ptr);
STATIC LONG Action_ChangeMode(LONG type, APTR object, LONG new_mode, SIPTR *error_ptr);
STATIC LONG Action_WriteProtect(LONG flag, ULONG key, SIPTR *error_ptr);
STATIC LONG Action_MoreCache(LONG buffer_delta, SIPTR *error_ptr);
STATIC LONG Action_SetComment(struct FileLock *parent, APTR bcpl_name, APTR bcpl_comment, SIPTR *error_ptr);
STATIC LONG Action_LockRecord(struct FileNode *fn, LONG offset, LONG length, LONG mode, ULONG timeout, SIPTR *error_ptr);
STATIC LONG Action_FreeRecord(struct FileNode *fn, LONG offset, LONG length, SIPTR *error_ptr);
STATIC VOID HandleFileSystem(STRPTR device_name, STRPTR volume_name, STRPTR service_name);

/****************************************************************************/

#if !defined(__AROS__)
struct Library *			SysBase;
#endif
struct Library *			DOSBase;
struct Library *			UtilityBase;
struct Library *			IntuitionBase;
struct Library *			SocketBase;
struct Library *			LocaleBase;
struct Library *			TimerBase;
struct Library *			IconBase;

/****************************************************************************/

#if defined(__amigaos4__)

/****************************************************************************/

struct ExecIFace *			IExec;
struct DOSIFace *			IDOS;
struct UtilityIFace *		IUtility;
struct IntuitionIFace *		IIntuition;
struct SocketIFace *		ISocket;
struct LocaleIFace *		ILocale;
struct TimerIFace *			ITimer;
struct IconIFace *			IIcon;

/****************************************************************************/

#endif /* __amigaos4__ */

/****************************************************************************/

struct timerequest			TimerRequest;

/****************************************************************************/

struct Locale *				Locale;

/****************************************************************************/

#ifndef h_errno
int							h_errno;
#endif

/****************************************************************************/

STATIC struct DosList *		DeviceNode;
STATIC BOOL					DeviceNodeAdded;
STATIC struct DosList *		VolumeNode;
STATIC BOOL					VolumeNodeAdded;
STATIC struct MsgPort *		FileSystemPort;

STATIC smba_server_t *		ServerData;

STATIC BOOL					Quit;
STATIC BOOL					Quiet;
STATIC BOOL					CaseSensitive;
STATIC BOOL					OmitHidden;

STATIC LONG					DSTOffset;
STATIC LONG					TimeZoneOffset;
STATIC BOOL					OverrideLocaleTimeZone;

STATIC BOOL					WriteProtected;
STATIC ULONG				WriteProtectKey;

STATIC struct MinList		FileList;
STATIC struct MinList		LockList;

STATIC APTR					MemoryPool;

STATIC struct RDArgs *		Parameters;
STATIC struct DiskObject *	Icon;

STATIC struct WBStartup * 	WBStartup;

STATIC struct MinList		ErrorList;

STATIC STRPTR				NewProgramName;

STATIC BOOL					TranslateNames;
STATIC UBYTE				A2M[256];
STATIC UBYTE				M2A[256];

/****************************************************************************/

#if !defined(__AROS__)
LONG _start(VOID)
#else
int main(void)
#endif
{
	struct
	{
		KEY		Workgroup;
		KEY		UserName;
		KEY		Password;
		SWITCH	ChangeCase;
		SWITCH	CaseSensitive;
		SWITCH	OmitHidden;
		SWITCH	Quiet;
		KEY		ClientName;
		KEY		ServerName;
		KEY		DeviceName;
		KEY		VolumeName;
		NUMBER	CacheSize;
		NUMBER	DebugLevel;
		NUMBER	TimeZoneOffset;
		NUMBER	DSTOffset;
		KEY		TranslationFile;
		KEY		Service;
	} args;

	STRPTR cmd_template =
		"DOMAIN=WORKGROUP/K,"
		"USER=USERNAME/K,"
		"PASSWORD/K,"
		"CHANGECASE/S,"
		"CASE=CASESENSITIVE/S,"
		"OMITHIDDEN/S,"
		"QUIET/S,"
		"CLIENT=CLIENTNAME/K,"
		"SERVER=SERVERNAME/K,"
		"DEVICE=DEVICENAME/K,"
		"VOLUME=VOLUMENAME/K,"
		"CACHE=CACHESIZE/N/K,"
		"DEBUGLEVEL=DEBUG/N/K,"
		"TZ=TIMEZONEOFFSET/N/K,"
		"DST=DSTOFFSET/N/K,"
		"TRANSLATE=TRANSLATIONFILE/K,"
		"SERVICE/A";

	struct Process * this_process;
	UBYTE program_name[MAX_FILENAME_LEN];
	LONG result = RETURN_FAIL;
	LONG number;
	LONG other_number;
	LONG cache_size = 0;
	char env_workgroup_name[17];
	char env_user_name[64];
	char env_password[64];

#if !defined(__AROS__)
	SysBase = (struct Library *)AbsExecBase;

	#if defined(__amigaos4__)
	{
		IExec = (struct ExecIFace *)((struct ExecBase *)SysBase)->MainInterface;
	}
	#endif /* __amigaos4__ */
#endif
	/* Pick up the Workbench startup message, if
	 * there is one.
	 */
	this_process = (struct Process *)FindTask(NULL);
	if(this_process->pr_CLI == ZERO)
	{
		WaitPort(&this_process->pr_MsgPort);
		WBStartup = (struct WBStartup *)GetMsg(&this_process->pr_MsgPort);
	}
	else
	{
		WBStartup = NULL;
	}

	/* Don't emit any debugging output before we are ready. */
	SETDEBUGLEVEL(0);

	/* Open the libraries we need and check
	 * whether we could get them.
	 */
	DOSBase = OpenLibrary("dos.library",0);

	#if defined(__amigaos4__)
	{
		if(DOSBase != NULL)
		{
			IDOS = (struct DOSIFace *)GetInterface(DOSBase, "main", 1, 0);
			if(IDOS == NULL)
			{
				CloseLibrary(DOSBase);
				DOSBase = NULL;
			}
		}
	}
	#endif /* __amigaos4__ */

	UtilityBase = OpenLibrary("utility.library",37);

	#if defined(__amigaos4__)
	{
		if(UtilityBase != NULL)
		{
			IUtility = (struct UtilityIFace *)GetInterface(UtilityBase, "main", 1, 0);
			if(IUtility == NULL)
			{
				CloseLibrary(UtilityBase);
				UtilityBase = NULL;
			}
		}
	}
	#endif /* __amigaos4__ */

	if(UtilityBase == NULL || DOSBase == NULL || DOSBase->lib_Version < MINIMUM_OS_VERSION)
	{
		/* Complain loudly if this is not the operating
		 * system version we expected.
		 */
		if(DOSBase != NULL && this_process->pr_CLI != ZERO)
		{
			STRPTR msg;

			#if (MINIMUM_OS_VERSION < 39)
			{
				msg = "AmigaOS 2.04 or higher required.\n";
			}
			#else
			{
				msg = "AmigaOS 3.0 or higher required.\n";
			}
			#endif /* MINIMUM_OS_VERSION */

			Write(Output(),msg,strlen(msg));
		}

		goto out;
	}

	/* This needs to be set up properly for ReportError()
	 * to work.
	 */
	NewList((struct List *)&ErrorList);

	memset(&args,0,sizeof(args));

	/* If this program was launched from Workbench,
	 * parameter passing will have to be handled
	 * differently.
	 */
	if(WBStartup != NULL)
	{
		STRPTR str;
		BPTR old_dir;
		LONG n;

		if(WBStartup->sm_NumArgs > 1)
			n = 1;
		else
			n = 0;

		/* Get the name of the program, as it was launched
		 * from Workbench. We actually prefer the name of
		 * the first project file, if there is one.
		 */
		strlcpy(program_name,WBStartup->sm_ArgList[n].wa_Name,sizeof(program_name));

		/* Now open icon.library and read that icon. */
		IconBase = OpenLibrary("icon.library",0);

		#if defined(__amigaos4__)
		{
			if(IconBase != NULL)
			{
				IIcon = (struct IconIFace *)GetInterface(IconBase, "main", 1, 0);
				if(IIcon == NULL)
				{
					CloseLibrary(IconBase);
					IconBase = NULL;
				}
			}
		}
		#endif /* __amigaos4__ */

		if(IconBase == NULL)
		{
			ReportError("Could not open 'icon.library'.");
			goto out;
		}

		old_dir = CurrentDir(WBStartup->sm_ArgList[n].wa_Lock);
		Icon = GetDiskObject(WBStartup->sm_ArgList[n].wa_Name);
		CurrentDir(old_dir);

		if(Icon == NULL)
		{
			ReportError("Icon not found.");
			goto out;
		}

		/* Only input validation errors are reported below. */
		result = RETURN_ERROR;

		/* Examine the icon's tool types and use the
		 * information to fill the startup parameter
		 * data structure.
		 */
		str = FindToolType(Icon->do_ToolTypes,"DOMAIN");
		if(str == NULL)
			str = FindToolType(Icon->do_ToolTypes,"WORKGROUP");

		if(str == NULL)
		{
			if(GetVar("smbfs_domain",env_workgroup_name,sizeof(env_workgroup_name),0) > 0 ||
			   GetVar("smbfs_workgroup",env_workgroup_name,sizeof(env_workgroup_name),0) > 0)
			{
				str = env_workgroup_name;
			}
			else
			{
				ReportError("Required 'WORKGROUP' parameter was not provided.");
				goto out;
			}
		}

		args.Workgroup = str;

		str = FindToolType(Icon->do_ToolTypes,"USER");
		if(str == NULL)
			str = FindToolType(Icon->do_ToolTypes,"USERNAME");

		if(str == NULL)
		{
			if(GetVar("smbfs_user",env_user_name,sizeof(env_user_name),0) > 0 ||
			   GetVar("smbfs_username",env_user_name,sizeof(env_user_name),0) > 0)
			{
				str = env_user_name;
			}
		}

		args.UserName = str;

		str = FindToolType(Icon->do_ToolTypes,"PASSWORD");
		if(str == NULL)
		{
			if(GetVar("smbfs_password",env_password,sizeof(env_password),0) > 0)
				str = env_password;
		}

		args.Password = str;

		if(FindToolType(Icon->do_ToolTypes,"CHANGECASE") != NULL)
			args.ChangeCase = TRUE;

		if(FindToolType(Icon->do_ToolTypes,"OMITHIDDEN") != NULL)
			args.OmitHidden = TRUE;

		if(FindToolType(Icon->do_ToolTypes,"QUIET") != NULL)
			args.Quiet = TRUE;

		if(FindToolType(Icon->do_ToolTypes,"CASE") != NULL ||
		   FindToolType(Icon->do_ToolTypes,"CASESENSITIVE") != NULL)
		{
			args.CaseSensitive = TRUE;
		}

		str = FindToolType(Icon->do_ToolTypes,"CLIENT");
		if(str == NULL)
			str = FindToolType(Icon->do_ToolTypes,"CLIENTNAME");

		args.ClientName = str;

		str = FindToolType(Icon->do_ToolTypes,"SERVER");
		if(str == NULL)
			str = FindToolType(Icon->do_ToolTypes,"SERVERNAME");

		args.ServerName = str;

		str = FindToolType(Icon->do_ToolTypes,"DEVICE");
		if(str == NULL)
			str = FindToolType(Icon->do_ToolTypes,"DEVICENAME");

		args.DeviceName = str;

		str = FindToolType(Icon->do_ToolTypes,"VOLUME");
		if(str == NULL)
			str = FindToolType(Icon->do_ToolTypes,"VOLUMENAME");

		args.VolumeName = str;

		str = FindToolType(Icon->do_ToolTypes,"TRANSLATE");
		if(str == NULL)
			str = FindToolType(Icon->do_ToolTypes,"TRANSLATIONFILE");

		args.TranslationFile = str;

		str = FindToolType(Icon->do_ToolTypes,"SERVICE");
		args.Service = str;

		if(str != NULL)
		{
			/* Set up the name of the program, as it will be
			 * displayed in error requesters.
			 */
			NewProgramName = AllocVec(strlen(WBStartup->sm_ArgList[0].wa_Name) + strlen(" ''") + strlen(str)+1,MEMF_ANY|MEMF_PUBLIC);
			if(NewProgramName != NULL)
				SPrintf(NewProgramName,"%s '%s'",WBStartup->sm_ArgList[0].wa_Name,str);
		}

		str = FindToolType(Icon->do_ToolTypes,"DEBUG");
		if(str == NULL)
			str = FindToolType(Icon->do_ToolTypes,"DEBUGLEVEL");

		if(str != NULL)
		{
			if(StrToLong(str,&number) == -1)
			{
				ReportError("Invalid number '%s' for 'DEBUG' parameter.",str);
				goto out;
			}

			args.DebugLevel = &number;
		}

		str = FindToolType(Icon->do_ToolTypes,"TZ");
		if(str == NULL)
			str = FindToolType(Icon->do_ToolTypes,"TIMEZONEOFFSET");

		if(str != NULL)
		{
			if(StrToLong(str,&other_number) == -1)
			{
				ReportError("Invalid number '%s' for 'TIMEZONEOFFSET' parameter.",str);
				goto out;
			}

			args.TimeZoneOffset = &other_number;
		}

		str = FindToolType(Icon->do_ToolTypes,"DST");
		if(str == NULL)
			str = FindToolType(Icon->do_ToolTypes,"DSTOFFSET");

		if(str != NULL)
		{
			if(StrToLong(str,&other_number) == -1)
			{
				ReportError("Invalid number '%s' for 'DSTOFFSET' parameter.",str);
				goto out;
			}

			args.DSTOffset = &other_number;
		}

		str = FindToolType(Icon->do_ToolTypes,"CACHE");
		if(str == NULL)
			str = FindToolType(Icon->do_ToolTypes,"CACHESIZE");

		if(str != NULL)
		{
			if(StrToLong(str,&number) == -1)
			{
				ReportError("Invalid number '%s' for 'CACHE' parameter.",str);
				goto out;
			}

			cache_size = number;
		}

		if(args.Workgroup == NULL)
		{
			ReportError("Required 'WORKGROUP' parameter was not provided.");
			goto out;
		}

		if(args.Service == NULL)
		{
			ReportError("'SERVICE' parameter needs an argument.");
			goto out;
		}
	}
	else
	{
		/* Only input validation errors are reported below. */
		result = RETURN_ERROR;

		GetProgramName(program_name,sizeof(program_name));

		Parameters = ReadArgs(cmd_template,(IPTR *)&args,NULL);
		if(Parameters == NULL)
		{
			PrintFault(IoErr(),FilePart(program_name));
			goto out;
		}

		if(args.Workgroup == NULL)
		{
			if(GetVar("smbfs_domain",env_workgroup_name,sizeof(env_workgroup_name),0) > 0 ||
			   GetVar("smbfs_workgroup",env_workgroup_name,sizeof(env_workgroup_name),0) > 0)
			{
				args.Workgroup = env_workgroup_name;
			}
			else
			{
				ReportError("Required 'WORKGROUP' parameter was not provided.");
				goto out;
			}
		}

		if(args.UserName == NULL)
		{
			if(GetVar("smbfs_user",env_user_name,sizeof(env_user_name),0) > 0 ||
			   GetVar("smbfs_username",env_user_name,sizeof(env_user_name),0) > 0)
			{
				args.UserName = env_user_name;
			}
		}

		if(args.Password == NULL)
		{
			if(GetVar("smbfs_password",env_password,sizeof(env_password),0) > 0)
				args.Password = env_password;
		}

		if(args.Service != NULL)
		{
			STRPTR name = FilePart(program_name);

			/* Set up the name of the program, as it will be
			 * displayed in the proces status list.
			 */
			NewProgramName = AllocVec(strlen(name) + strlen(" ''") + strlen(args.Service)+1,MEMF_ANY|MEMF_PUBLIC);
			if(NewProgramName != NULL)
				SPrintf(NewProgramName,"%s '%s'",name,args.Service);
		}

		if(args.CacheSize != NULL)
			cache_size = (*args.CacheSize);
	}

	/* Use the default if no user name is given. */
	if(args.UserName == NULL)
		args.UserName = "GUEST";

	/* Use the default if no device or volume name is given. */
	if(args.DeviceName == NULL && args.VolumeName == NULL)
		args.DeviceName = "SMBFS";

	CaseSensitive = (BOOL)args.CaseSensitive;
	OmitHidden = (BOOL)args.OmitHidden;

	/* Configure the debugging options. */
	SETPROGRAMNAME(FilePart(program_name));

	if(args.DebugLevel != NULL)
		SETDEBUGLEVEL(*args.DebugLevel);
	else
		SETDEBUGLEVEL(0);

	D(("%s (%s)",VERS,DATE));

	if(Setup(
		FilePart(program_name),
		args.Service,
		args.Workgroup,
		args.UserName,
		args.Password,
		args.ChangeCase,
		args.ClientName,
		args.ServerName,
		cache_size,
		args.TimeZoneOffset,
		args.DSTOffset,
		args.DeviceName,
		args.VolumeName,
		args.TranslationFile))
	{
		Quiet = args.Quiet;

		if(Locale != NULL)
			SHOWVALUE(Locale->loc_GMTOffset);

		HandleFileSystem(args.DeviceName,args.VolumeName,args.Service);

		result = RETURN_WARN;
	}
	else
	{
		result = RETURN_ERROR;
	}

 out:

	Cleanup();

	return(result);
}

/****************************************************************************/

#ifdef __AROS__
#define LocalPrintf(format,args...) Printf(format ,##args )
#else
LONG VARARGS68K
LocalPrintf(STRPTR format, ...)
{
	va_list args;
	LONG result;

	#if defined(__amigaos4__)
	{
		va_startlinear(args,format);
		result = VPrintf(format,va_getlinearva(args,APTR));
		va_end(args);
	}
	#else
	{
		va_start(args,format);
		result = VPrintf(format,args);
		va_end(args);
	}
	#endif /* __amigaos4__ */

	return(result);
}
#endif /* __AROS__ */

/****************************************************************************/

/* Obtain the descriptive text corresponding to an error number
 * that may have been generated by the TCP/IP stack.
 */
STRPTR
amitcp_strerror(int error)
{
	struct TagItem tags[2];
	STRPTR result;

	ENTER();

	tags[0].ti_Tag	= SBTM_GETVAL(SBTC_ERRNOSTRPTR);
	tags[0].ti_Data	= error;
	tags[1].ti_Tag	= TAG_END;

	SocketBaseTagList(tags);

	result = (STRPTR)tags[0].ti_Data;

	RETURN(result);
	return(result);
}

/****************************************************************************/

/* Return the descriptive text associated with a host lookup failure code. */
STRPTR
host_strerror(int error)
{
	struct TagItem tags[2];
	STRPTR result;

	ENTER();

	tags[0].ti_Tag	= SBTM_GETVAL(SBTC_HERRNOSTRPTR);
	tags[0].ti_Data	= error;
	tags[1].ti_Tag	= TAG_END;

	SocketBaseTagList(tags);

	result = (STRPTR)tags[0].ti_Data;

	RETURN(result);
	return(result);
}

/****************************************************************************/

/* Compare two strings, either case sensitive or not
 * sensitive to the case of the letters. How this is
 * to be done is controlled by a global option. This
 * routine is called whenever two SMB file names are
 * to be compared.
 */
LONG
CompareNames(STRPTR a,STRPTR b)
{
	LONG result;

	if(CaseSensitive)
		result = strcmp(a,b);
	else
		result = Stricmp(a,b);

	return(result);
}

/****************************************************************************/

/* Translate a string into all upper case characters. */
VOID
StringToUpper(STRPTR s)
{
	UBYTE c;

	while((c = (*s)) != '\0')
		(*s++) = ToUpper(c);
}

/****************************************************************************/

/* Prepare the accumulated list of error messages for display
 * and purge the contents of that list.
 */
STATIC VOID
DisplayErrorList(VOID)
{
	struct MinNode * last = NULL;
	struct MinNode * mn;
	STRPTR str = NULL;
	STRPTR msg;
	LONG len;

	/* Determine how much memory will have to be
	 * allocated to hold all the accumulated
	 * error messages.
	 */
	len = 0;

	for(mn = ErrorList.mlh_Head ;
	    mn->mln_Succ != NULL ;
	    mn = mn->mln_Succ)
	{
		last = mn;

		msg = (STRPTR)(mn + 1);

		len += strlen(msg)+1;
	}

	/* Allocate the memory for the messages, then
	 * copy them there.
	 */
	if(len > 0)
	{
		str = AllocVec(len,MEMF_ANY);
		if(str != NULL)
		{
			str[0] = '\0';

			for(mn = ErrorList.mlh_Head ;
			    mn->mln_Succ != NULL ;
			    mn = mn->mln_Succ)
			{
				msg = (STRPTR)(mn + 1);

				strcat(str,msg);
				if(mn != last)
					strcat(str,"\n");
			}
		}
	}

	/* Purge the list. */
	while((mn = (struct MinNode *)RemHead((struct List *)&ErrorList)) != NULL)
		FreeVec(mn);

	/* Display the error messages. */
	if(str != NULL)
	{
		IntuitionBase = OpenLibrary("intuition.library",37);

		#if defined(__amigaos4__)
		{
			if(IntuitionBase != NULL)
			{
				IIntuition = (struct IntuitionIFace *)GetInterface(IntuitionBase, "main", 1, 0);
				if(IIntuition == NULL)
				{
					CloseLibrary(IntuitionBase);
					IntuitionBase = NULL;
				}
			}
		}
		#endif /* __amigaos4__ */

		if(IntuitionBase != NULL)
		{
			struct EasyStruct es;
			STRPTR title;

			memset(&es,0,sizeof(es));

			if(NewProgramName == NULL)
				title = WBStartup->sm_ArgList[0].wa_Name;
			else
				title = NewProgramName;

			es.es_StructSize	= sizeof(es);
			es.es_Title			= title;
			es.es_TextFormat	= str;
			es.es_GadgetFormat	= "Ok";

			EasyRequestArgs(NULL,&es,NULL,NULL);
		}

		FreeVec(str);
	}

	#if defined(__amigaos4__)
	{
		if(IIntuition != NULL)
		{
			DropInterface((struct Interface *)IIntuition);
			IIntuition = NULL;
		}
	}
	#endif /* __amigaos4__ */

	CloseLibrary(IntuitionBase);
	IntuitionBase = NULL;
}

/* Add another error message to the list; the messages are
 * collected so that they may be displayed together when
 * necessary.
 */
STATIC VOID
AddError(STRPTR fmt,APTR args)
{
	LONG len;

	len = CVSPrintf(fmt,args);
	if(len > 0)
	{
		struct MinNode * mn;

		mn = AllocVec(sizeof(*mn) + len,MEMF_ANY|MEMF_PUBLIC);
		if(mn != NULL)
		{
			STRPTR msg = (STRPTR)(mn + 1);

			VSPrintf(msg,fmt,args);

			AddTail((struct List *)&ErrorList,(struct Node *)mn);
		}
	}
}

/****************************************************************************/

/* Report an error that has occured; if the program was not launched
 * from Shell, error messages will be accumulated for later display.
 */
#ifdef __AROS__
VOID VReportError(STRPTR fmt, IPTR *args)
{
	if(NOT Quiet)
	{
		if(WBStartup != NULL)
		{
			AddError(fmt,args);
		}
		else
		{
			UBYTE program_name[MAX_FILENAME_LEN];

			GetProgramName(program_name,sizeof(program_name));

			LocalPrintf("%s: ",FilePart(program_name));

			VPrintf(fmt,args);

			LocalPrintf("\n");
		}
	}
}
#else
VOID VARARGS68K
ReportError(STRPTR fmt,...)
{
	if(NOT Quiet)
	{
		va_list args;

		if(WBStartup != NULL)
		{
			#if defined(__amigaos4__)
			{
				va_startlinear(args,fmt);
				AddError(fmt,va_getlinearva(args,APTR));
				va_end(args);
			}
			#else
			{
				va_start(args,fmt);
				AddError(fmt,args);
				va_end(args);
			}
			#endif /* __amigaos4__ */
		}
		else
		{
			UBYTE program_name[MAX_FILENAME_LEN];

			GetProgramName(program_name,sizeof(program_name));

			LocalPrintf("%s: ",FilePart(program_name));

			#if defined(__amigaos4__)
			{
				va_startlinear(args,fmt);
				VPrintf(fmt,va_getlinearva(args,APTR));
				va_end(args);
			}
			#else
			{
				va_start(args,fmt);
				VPrintf(fmt,args);
				va_end(args);
			}
			#endif /* __amigaos4__ */

			LocalPrintf("\n");
		}
	}
}
#endif

/****************************************************************************/

/* Release memory allocated from the global pool. */
VOID
FreeMemory(APTR address)
{
	if(address != NULL)
	{
		ULONG * mem = address;

		#if DEBUG
		{
			if(GETDEBUGLEVEL() > 0)
				memset(address,0xA3,mem[-1] - sizeof(*mem));
		}
		#endif /* DEBUG */

		FreePooled(MemoryPool,&mem[-1],mem[-1]);
	}
}

/* Allocate memory from the global pool. */
APTR
AllocateMemory(ULONG size)
{
	APTR result = NULL;

	if(size > 0)
	{
		ULONG * mem;

		size = (sizeof(*mem) + size + 7) & ~7UL;

		mem = AllocPooled(MemoryPool,size);
		if(mem != NULL)
		{
			(*mem++) = size;

			#if DEBUG
			{
				if(GETDEBUGLEVEL() > 0)
					memset(mem,0xA5,mem[-1] - sizeof(*mem));
			}
			#endif /* DEBUG */

			result = mem;
		}
	}

	return(result);
}

/****************************************************************************/

/* Obtain the number of seconds to add to the current time
 * to translate local time into UTC.
 */
LONG
GetTimeZoneDelta(VOID)
{
	LONG seconds;

	if(OverrideLocaleTimeZone)
	{
		seconds = 60 * TimeZoneOffset;
	}
	else if (Locale != NULL)
	{
		/* The GMT offset actually is the number of minutes to add to
		 * the local time to yield Greenwich Mean Time. It is negative
		 * for all time zones east of the Greenwich meridian and
		 * positive for all time zones west of it.
		 */
		seconds = 60 * Locale->loc_GMTOffset;
	}
	else
	{
		seconds = 0;
	}

	return(seconds + DSTOffset);
}

/****************************************************************************/

/* Obtain the current time, in standard Unix format, adjusted for the
 * local time zone.
 */
ULONG
GetCurrentTime(VOID)
{
	struct timeval tv;
	ULONG result;

	GetSysTime((APTR)&tv);

	result = UNIX_TIME_OFFSET + GetTimeZoneDelta() + tv.tv_secs;

	return(result);
}

/****************************************************************************/

/* Fill in a 'tm' type time specification with time information
 * corresponding to the number of seconds provided. Input is
 * in Unix format.
 */
VOID
GMTime(time_t seconds,struct tm * tm)
{
	struct ClockData clock_data;

	if(seconds < UNIX_TIME_OFFSET)
		seconds = 0;
	else
		seconds -= UNIX_TIME_OFFSET;

	Amiga2Date(seconds,&clock_data);

	memset(tm,0,sizeof(*tm));

	tm->tm_sec	= clock_data.sec;
	tm->tm_min	= clock_data.min;
	tm->tm_hour	= clock_data.hour;
	tm->tm_mday	= clock_data.mday;
	tm->tm_mon	= clock_data.month - 1;
	tm->tm_year	= clock_data.year - 1900;
}

/* Calculate the number of seconds that have passed since January 1st 1970
 * based upon the time specification provided. Output is in Unix format.
 */
time_t
MakeTime(const struct tm * const tm)
{
	struct ClockData clock_data;
	time_t seconds;

	clock_data.sec		= tm->tm_sec;
	clock_data.min		= tm->tm_min;
	clock_data.hour		= tm->tm_hour;
	clock_data.mday		= tm->tm_mday;
	clock_data.month	= tm->tm_mon + 1;
	clock_data.year		= tm->tm_year + 1900;

	seconds = Date2Amiga(&clock_data) + UNIX_TIME_OFFSET;

	return(seconds);
}

/****************************************************************************/

struct FormatContext
{
	UBYTE *	fc_Buffer;
	LONG	fc_Size;
};

/****************************************************************************/

#if !defined(__AROS__)
STATIC VOID ASM
CountChar(REG(a3,struct FormatContext * fc))
#else
STATIC VOID CountChar(struct FormatContext * fc)
#endif
{
	fc->fc_Size++;
}

/* Count the number of characters SPrintf() would put into a string. */
STATIC LONG
CVSPrintf(STRPTR format_string,APTR args)
{
	struct FormatContext fc;

	fc.fc_Size = 0;

	RawDoFmt((STRPTR)format_string,args,(VOID (*)())CountChar,&fc);

	return(fc.fc_Size);
}

/****************************************************************************/

#if !defined(__AROS__)
STATIC VOID ASM
StuffChar(REG(d0,UBYTE c),REG(a3,struct FormatContext * fc))
#else
STATIC VOID StuffChar(UBYTE c, struct FormatContext * fc)
#endif
{
	(*fc->fc_Buffer++) = c;
}

STATIC VOID
VSPrintf(STRPTR buffer, STRPTR formatString, APTR args)
{
	struct FormatContext fc;

	fc.fc_Buffer = buffer;

	RawDoFmt(formatString,args,(VOID (*)())StuffChar,&fc);
}

/****************************************************************************/

#if !defined(__AROS__)
/* Format a string for output. */
VOID VARARGS68K
SPrintf(STRPTR buffer, STRPTR formatString,...)
{
	va_list varArgs;

	#if defined(__amigaos4__)
	{
		va_startlinear(varArgs,formatString);
		VSPrintf(buffer,formatString,va_getlinearva(varArgs,APTR));
		va_end(varArgs);
	}
	#else
	{
		va_start(varArgs,formatString);
		VSPrintf(buffer,formatString,varArgs);
		va_end(varArgs);
	}
	#endif /* __amigaos4__ */
}
#endif

/****************************************************************************/

/* NetBIOS broadcast name query code courtesy of Christopher R. Hertel.
 * Thanks much, Chris!
 */
struct addr_entry
{
	unsigned short flags;
	unsigned char address[4];
};

struct nmb_header
{
	unsigned short name_trn_id;
	unsigned short flags;
	unsigned short qdcount;
	unsigned short ancount;
	unsigned short nscount;
	unsigned short arcount;
};

static UBYTE *
L1_Encode(UBYTE * dst, const UBYTE * name, const UBYTE pad, const UBYTE sfx)
{
	int i = 0;
	int j = 0;
	int k;

	while(('\0' != name[i]) && (i < 15))
	{
		k = ToUpper(name[i]);
		i++;
		dst[j++] = 'A' + ((k & 0xF0) >> 4);
		dst[j++] = 'A' + (k & 0x0F);
	}

	i = 'A' + ((pad & 0xF0) >> 4);
	k = 'A' + (pad & 0x0F);

	while(j < 30)
	{
		dst[j++] = i;
		dst[j++] = k;
	}

	dst[30] = 'A' + ((sfx & 0xF0) >> 4);
	dst[31] = 'A' + (sfx & 0x0F);
	dst[32] = '\0';

	return(dst);
}

static int
L2_Encode(UBYTE * dst, const UBYTE * name, const UBYTE pad, const UBYTE sfx, const UBYTE * scope)
{
	int lenpos;
	int i;
	int j;

	if(NULL == L1_Encode(&dst[1], name, pad, sfx))
		return(-1);

	dst[0] = 0x20;
	lenpos = 33;

	if('\0' != (*scope))
	{
		do
		{
			for(i = 0, j = (lenpos + 1);
			    ('.' != scope[i]) && ('\0' != scope[i]);
			    i++, j++)
			{
				dst[j] = ToUpper(scope[i]);
			}

			dst[lenpos] = (UBYTE)i;
			lenpos += i + 1;
			scope += i;
		}
		while('.' == (*scope++));

		dst[lenpos] = '\0';
	}

	return(lenpos + 1);
}

int
BroadcastNameQuery(char *name, char *scope, UBYTE *address)
{
	static const UBYTE header[12] =
	{
		0x07, 0xB0,	/* 1964 == 0x07B0. */
		0x01, 0x10,	/* Binary 0 0000 0010001 0000 */
		0x00, 0x01,	/* One name query. */
		0x00, 0x00,	/* Zero answers. */
		0x00, 0x00,	/* Zero authorities. */
		0x00, 0x00	/* Zero additional. */
	};

	static const UBYTE query_tail[4] =
	{
		0x00, 0x20,
		0x00, 0x01
	};

	struct timeval tv;
	fd_set read_fds;
	int sock_fd;
	int option_true = 1;
	struct sockaddr_in sox;
	struct nmb_header nmb_header;
	UBYTE buffer[512];
	int total_len;
	int i,n;
	int result;
	struct servent * s;

	ENTER();

	sock_fd = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if(sock_fd < 0)
	{
		SHOWMSG("couldn't get the socket");
		result = (-errno);
		goto out;
	}

	if(setsockopt(sock_fd, SOL_SOCKET, SO_BROADCAST, &option_true, sizeof(option_true)) < 0)
	{
		SHOWMSG("couldn't enable the broadcast option");
		result = (-errno);
		goto out;
	}

	sox.sin_family = AF_INET;
	sox.sin_addr.s_addr = htonl(0xFFFFFFFF);

	s = getservbyname("netbios-ns","udp");
	if(s != NULL)
		sox.sin_port = htons(s->s_port);
	else
		sox.sin_port = htons(137);

	memcpy(buffer, header, (total_len = sizeof(header)));

	n = L2_Encode(&buffer[total_len], name, ' ', '\0', scope);
	if(n < 0)
	{
		SHOWMSG("name encoding failed");
		result = (-EINVAL);
		goto out;
	}

	total_len += n;
	memcpy(&buffer[total_len], query_tail, sizeof(query_tail));
	total_len += sizeof(query_tail);

	result = (-ENOENT);
	n = 0;

	/* Send the query packet; retry five times with a one second
	 * delay in between.
	 */
	for(i = 0 ; i < 5 ; i++)
	{
		if(sendto(sock_fd, (void *) buffer, total_len, 0, (struct sockaddr *)&sox, sizeof(struct sockaddr_in)) < 0)
		{
			SHOWMSG("could not send the packet");
			result = (-errno);
			goto out;
		}

		/* Wait for a response to arrive. */
		tv.tv_secs = 1;
		tv.tv_micro = 0;

		FD_ZERO(&read_fds);
		FD_SET(sock_fd,&read_fds);

		if(WaitSelect(sock_fd+1, &read_fds, NULL, NULL, &tv, NULL) > 0)
		{
			n = recvfrom(sock_fd, buffer, sizeof(buffer), 0, NULL, NULL);
			if(n < 0)
			{
				SHOWMSG("could not pick up the response packet");
				result = (-errno);
				goto out;
			}
			else if (n > 0)
			{
				break;
			}
		}
	}

	/* Did we get anything at all? */
	if(n > (int)sizeof(nmb_header))
	{
		/* Check whether the query was successful. */
		memcpy(&nmb_header, buffer, sizeof(nmb_header));
		if((nmb_header.flags & 0xF) == OK)
		{
			/* Find the NB/IP fields which directly follow
			 * the name.
			 */
			for(i = sizeof(header) + strlen(&buffer[sizeof(header)])+1 ; i < n - (int)sizeof(query_tail) ; i++)
			{
				if(memcmp(&buffer[i], query_tail, sizeof(query_tail)) == SAME)
				{
					int start;

					/* This should be the start of the interesting bits;
					 * we skip the NB/IP fields and the TTL field.
					 */
					start = i + sizeof(query_tail) + sizeof(long);
					if(start < n)
					{
						unsigned short read_len;
						struct addr_entry addr_entry;

						/* This should be the read length. */
						memcpy(&read_len, &buffer[start], 2);

						/* Is there any useful and readable data attached? */
						if(read_len >= sizeof(addr_entry) &&
						   start + (int)sizeof(read_len) + (int)sizeof(addr_entry) <= n)
						{
							/* Copy a single address entry; this should be
							 * just the one we need.
							 */
							memcpy(&addr_entry, &buffer[start + sizeof(read_len)], sizeof(addr_entry));

							/* Copy the address field (IPv4 only). */
							memcpy(address, addr_entry.address, 4);

							result = 0;
						}
					}

					break;
				}
			}
		}
	}

out:

	if(sock_fd >= 0)
		CloseSocket(sock_fd);

	RETURN(result);
	return(result);
}

/****************************************************************************/

/* Send a disk change notification message which will be picked up
 * by all applications that listen for this kind of event, e.g.
 * Workbench.
 */
STATIC VOID
SendDiskChange(ULONG class)
{
	struct IOStdReq * input_request = NULL;
	struct MsgPort * input_port;
	struct InputEvent ie;

	ENTER();

	input_port = CreateMsgPort();
	if(input_port == NULL)
		goto out;

	input_request = (struct IOStdReq *)CreateIORequest(input_port,sizeof(*input_request));
	if(input_request == NULL)
		goto out;

	if(OpenDevice("input.device",0,(struct IORequest *)input_request,0) != OK)
		goto out;

	memset(&ie,0,sizeof(ie));

	ie.ie_Class		= class;
	ie.ie_Qualifier	= IEQUALIFIER_MULTIBROADCAST;

	GetSysTime(&ie.ie_TimeStamp);

	input_request->io_Command	= IND_WRITEEVENT;
	input_request->io_Data		= &ie;
	input_request->io_Length	= sizeof(ie);

	DoIO((struct IORequest *)input_request);

 out:

	if(input_request != NULL)
	{
		if(input_request->io_Device != NULL)
			CloseDevice((struct IORequest *)input_request);

		DeleteIORequest((struct IORequest *)input_request);
	}

	DeleteMsgPort(input_port);

	LEAVE();
}

/****************************************************************************/

/* Find the file node corresponding to a given name,
 * skipping a particular entry if necessary.
 */
STATIC struct FileNode *
FindFileNode(STRPTR name,struct FileNode * skip)
{
	struct FileNode * result = NULL;
	struct FileNode * fn;

	for(fn = (struct FileNode *)FileList.mlh_Head ;
	    fn->fn_MinNode.mln_Succ != NULL ;
	    fn = (struct FileNode *)fn->fn_MinNode.mln_Succ)
	{
		if(fn != skip && CompareNames(name,fn->fn_FullName) == SAME)
		{
			result = fn;
			break;
		}
	}

	return(result);
}

/* Find the lock node corresponding to a given name,
 * skipping a particular entry if necessary.
 */
STATIC struct LockNode *
FindLockNode(STRPTR name,struct LockNode * skip)
{
	struct LockNode * result = NULL;
	struct LockNode * ln;

	for(ln = (struct LockNode *)LockList.mlh_Head ;
	    ln->ln_MinNode.mln_Succ != NULL ;
	    ln = (struct LockNode *)ln->ln_MinNode.mln_Succ)
	{
		if(ln != skip && CompareNames(name,ln->ln_FullName) == SAME)
		{
			result = ln;
			break;
		}
	}

	return(result);
}

/* Check whether a new reference to be made to a named
 * file will cause a conflict of access modes. No two
 * files and locks may refer to the same object if
 * either of these references is made in exclusive
 * mode. This is the case which this function is
 * trying to avoid.
 */
STATIC LONG
CheckAccessModeCollision(STRPTR name,LONG mode)
{
	struct LockNode * ln;
	struct FileNode * fn;
	LONG error = OK;

	ENTER();
	SHOWSTRING(name);

	fn = FindFileNode(name,NULL);
	if(fn != NULL)
	{
		if(mode != SHARED_LOCK || fn->fn_Mode != SHARED_LOCK)
		{
			D(("collides with '%s'",fn->fn_FullName));
			error = ERROR_OBJECT_IN_USE;
			goto out;
		}
	}

	ln = FindLockNode(name,NULL);
	if(ln != NULL)
	{
		if(mode != SHARED_LOCK || ln->ln_FileLock.fl_Access != SHARED_LOCK)
		{
			D(("collides with '%s'",ln->ln_FullName));
			error = ERROR_OBJECT_IN_USE;
			goto out;
		}
	}

 out:

	RETURN(error);
	return(error);
}

/* Find out whether there already exists a reference to a
 * certain file or directory.
 */
STATIC LONG
NameAlreadyInUse(STRPTR name)
{
	LONG error = OK;

	ENTER();

	if(FindFileNode(name,NULL) != NULL)
	{
		error = ERROR_OBJECT_IN_USE;
		goto out;
	}

	if(FindLockNode(name,NULL) != NULL)
	{
		error = ERROR_OBJECT_IN_USE;
		goto out;
	}

 out:

	RETURN(error);
	return(error);
}

/* Check whether an Amiga file name uses special characters which
 * should be avoided when used with the SMB file sharing protocol.
 */
STATIC BOOL
IsReservedName(STRPTR name)
{
	BOOL result = FALSE;

	/* Disallow "." and "..". */
	if(name[0] == '.' && (name[1] == '\0' || (name[1] == '.' && name[2] == '\0')))
	{
		result = TRUE;
	}
	else
	{
		UBYTE c;

		/* Disallow the use of the backslash in file names. */
		while((c = (*name++)) != '\0')
		{
			if(c == SMB_PATH_SEPARATOR)
			{
				result = TRUE;
				break;
			}
		}
	}

	return(result);
}

/****************************************************************************/

/* Convert a POSIX error code into an AmigaDOS error code. */
STATIC LONG
MapErrnoToIoErr(int error)
{
	/* Not all of these mappings make good sense; bear in mind that
	 * POSIX covers more than a hundred different error codes
	 * whereas with AmigaDOS we're stranded with a measly 48...
	 */
	STATIC const LONG Map[][2] =
	{
		{ EPERM,			ERROR_OBJECT_NOT_FOUND },		/* Operation not permitted */
		{ ENOENT,			ERROR_OBJECT_NOT_FOUND },		/* No such file or directory */
		{ ESRCH,			ERROR_OBJECT_NOT_FOUND },		/* No such process */
		{ EINTR,			ERROR_BREAK },					/* Interrupted system call */
		{ EIO,				ERROR_OBJECT_IN_USE },			/* Input/output error */
		{ E2BIG,			ERROR_TOO_MANY_ARGS },			/* Argument list too long */
		{ EBADF,			ERROR_INVALID_LOCK },			/* Bad file descriptor */
		{ ENOMEM,			ERROR_NO_FREE_STORE },			/* Cannot allocate memory */
		{ EACCES,			ERROR_OBJECT_IN_USE },			/* Permission denied */
#ifdef ENOTBLK
		{ ENOTBLK,			ERROR_OBJECT_WRONG_TYPE },		/* Block device required */
#endif
		{ EBUSY,			ERROR_OBJECT_IN_USE },			/* Device busy */
		{ EEXIST,			ERROR_OBJECT_EXISTS },			/* File exists */
		{ EXDEV,			ERROR_NOT_IMPLEMENTED },		/* Cross-device link */
		{ ENOTDIR,			ERROR_OBJECT_WRONG_TYPE },		/* Not a directory */
		{ EISDIR,			ERROR_OBJECT_WRONG_TYPE },		/* Is a directory */
		{ EINVAL,			ERROR_BAD_NUMBER },				/* Invalid argument */
		{ EFBIG,			ERROR_DISK_FULL },				/* File too large */
		{ ENOSPC,			ERROR_DISK_FULL },				/* No space left on device */
		{ ESPIPE,			ERROR_SEEK_ERROR },				/* Illegal seek */
		{ EROFS,			ERROR_WRITE_PROTECTED },		/* Read-only file system */
		{ EMLINK,			ERROR_TOO_MANY_LEVELS },		/* Too many links */
		{ ENOTSOCK,			ERROR_OBJECT_WRONG_TYPE },		/* Socket operation on non-socket */
		{ EDESTADDRREQ,		ERROR_REQUIRED_ARG_MISSING },	/* Destination address required */
		{ EMSGSIZE,			ERROR_LINE_TOO_LONG },			/* Message too long */
		{ EPROTOTYPE,		ERROR_BAD_TEMPLATE },			/* Protocol wrong type for socket */
		{ ENOPROTOOPT,		ERROR_NOT_IMPLEMENTED },		/* Protocol not available */
		{ EPROTONOSUPPORT,	ERROR_NOT_IMPLEMENTED },		/* Protocol not supported */
		{ ESOCKTNOSUPPORT,	ERROR_NOT_IMPLEMENTED },		/* Socket type not supported */
		{ EOPNOTSUPP,		ERROR_NOT_IMPLEMENTED },		/* Operation not supported */
		{ EPFNOSUPPORT,		ERROR_NOT_IMPLEMENTED },		/* Protocol family not supported */
		{ EAFNOSUPPORT,		ERROR_NOT_IMPLEMENTED },		/* Address family not supported by protocol family */
		{ EADDRINUSE,		ERROR_OBJECT_IN_USE },			/* Address already in use */
		{ EADDRNOTAVAIL,	ERROR_OBJECT_NOT_FOUND },		/* Can't assign requested address */
		{ ENETDOWN,			ERROR_OBJECT_NOT_FOUND },		/* Network is down */
		{ ENETUNREACH,		ERROR_OBJECT_NOT_FOUND },		/* Network is unreachable */
		{ ENETRESET,		ERROR_OBJECT_NOT_FOUND },		/* Network dropped connection on reset */
		{ ECONNABORTED,		ERROR_OBJECT_NOT_FOUND },		/* Software caused connection abort */
		{ ECONNRESET,		ERROR_OBJECT_NOT_FOUND },		/* Connection reset by peer */
		{ ENOBUFS,			ERROR_DISK_FULL },				/* No buffer space available */
		{ EISCONN,			ERROR_OBJECT_IN_USE },			/* Socket is already connected */
		{ ENOTCONN,			ERROR_OBJECT_WRONG_TYPE },		/* Socket is not connected */
		{ ESHUTDOWN,		ERROR_INVALID_LOCK },			/* Can't send after socket shutdown */
		{ ECONNREFUSED,		ERROR_OBJECT_IN_USE },			/* Connection refused */
		{ ELOOP,			ERROR_TOO_MANY_LEVELS },		/* Too many levels of symbolic links */
		{ ENAMETOOLONG,		ERROR_LINE_TOO_LONG },			/* File name too long */
		{ EHOSTDOWN,		ERROR_OBJECT_NOT_FOUND },		/* Host is down */
		{ EHOSTUNREACH,		ERROR_OBJECT_NOT_FOUND },		/* No route to host */
		{ ENOTEMPTY,		ERROR_DIRECTORY_NOT_EMPTY },	/* Directory not empty */
		{ EPROCLIM,			ERROR_TASK_TABLE_FULL },		/* Too many processes */
#ifdef EUSERS
		{ EUSERS,			ERROR_TASK_TABLE_FULL },		/* Too many users */
#endif
		{ EDQUOT,			ERROR_DISK_FULL },				/* Disc quota exceeded */
		{ ENOLCK,			ERROR_NOT_IMPLEMENTED },		/* no locks available */
		{ -1,				-1 }
	};

	LONG result = ERROR_ACTION_NOT_KNOWN;
	LONG i;

	ENTER();

	if(error < 0)
		error = (-error);

	for(i = 0 ; Map[i][0] != -1 ; i++)
	{
		if(Map[i][0] == error)
		{
			result = Map[i][1];
			break;
		}
	}

	RETURN(result);
	return(result);
}

/****************************************************************************/

/* Translate a BCPL style file name (i.e. length is in the first byte)
 * via a mapping table.
 */
INLINE STATIC VOID
TranslateBName(UBYTE * name,UBYTE * map)
{
	if(TranslateNames)
	{
		LONG len;
		UBYTE c;

#if !defined(__AROS__)
		len = (*name++);
#else
		len = AROS_BSTR_strlen(name);
		name = AROS_BSTR_ADDR(name);
#endif
		while(len-- > 0)
		{
			c = (*name);

			(*name++) = map[c];
		}
	}
}

/* Translate a NUL terminated file name via a mapping table. */
INLINE STATIC VOID
TranslateCName(UBYTE * name,UBYTE * map)
{
	if(TranslateNames)
	{
		UBYTE c;

		while((c = (*name)) != '\0')
			(*name++) = map[c];
	}
}

/****************************************************************************/

/* Remove a DosList entry using the proper protocols. Note that
 * this function can fail!
 */
STATIC BOOL
ReallyRemoveDosEntry(struct DosList * entry)
{
	struct Message * mn;
	struct MsgPort * port;
	struct DosList * dl;
	BOOL result = FALSE;
	LONG kind,i;

	if(entry->dol_Type == DLT_DEVICE)
		kind = LDF_DEVICES;
	else
		kind = LDF_VOLUMES;

	port = entry->dol_Task;

	for(i = 0 ; i < 100 ; i++)
	{
		dl = AttemptLockDosList(LDF_WRITE|kind);
		if(((IPTR)dl) <= 1)
			dl = NULL;

		if(dl != NULL)
		{
			RemDosEntry(entry);

			UnLockDosList(LDF_WRITE|kind);

			result = TRUE;

			break;
		}

		while((mn = GetMsg(port)) != NULL)
			ReplyPkt((struct DosPacket *)mn->mn_Node.ln_Name,DOSFALSE,ERROR_ACTION_NOT_KNOWN);

		Delay(TICKS_PER_SECOND / 10);
	}

	return(result);
}

/****************************************************************************/

/* Release all resources allocated by the Setup() routine. */
STATIC VOID
Cleanup(VOID)
{
	BOOL send_disk_change = FALSE;

	ENTER();

	/* If any errors have cropped up, display them now before
	 * call it quits.
	 */
	DisplayErrorList();

	if(NewProgramName != NULL)
	{
		FreeVec(NewProgramName);
		NewProgramName = NULL;
	}

	if(Parameters != NULL)
	{
		FreeArgs(Parameters);
		Parameters = NULL;
	}

	if(Icon != NULL)
	{
		FreeDiskObject(Icon);
		Icon = NULL;
	}

	if(ServerData != NULL)
	{
		smba_disconnect(ServerData);
		ServerData = NULL;
	}

	if(DeviceNode != NULL)
	{
		if(DeviceNodeAdded)
		{
			if(ReallyRemoveDosEntry(DeviceNode))
				FreeDosEntry(DeviceNode);
		}
		else
		{
			FreeDosEntry(DeviceNode);
		}

		DeviceNode = NULL;
	}

	if(VolumeNode != NULL)
	{
		if(VolumeNodeAdded)
		{
			if(ReallyRemoveDosEntry(VolumeNode))
				FreeDosEntry(VolumeNode);

			send_disk_change = TRUE;
		}
		else
		{
			FreeDosEntry(VolumeNode);
		}

		VolumeNode = NULL;
	}

	if(FileSystemPort != NULL)
	{
		struct Message * mn;

		/* Return all queued packets; there should be none, though. */
		while((mn = GetMsg(FileSystemPort)) != NULL)
			ReplyPkt((struct DosPacket *)mn->mn_Node.ln_Name,DOSFALSE,ERROR_ACTION_NOT_KNOWN);

		DeleteMsgPort(FileSystemPort);
		FileSystemPort = NULL;
	}

	if(WBStartup == NULL && send_disk_change)
		SendDiskChange(IECLASS_DISKREMOVED);

	#if defined(__amigaos4__)
	{
		if(ITimer != NULL)
		{
			DropInterface((struct Interface *)ITimer);
			ITimer = NULL;
		}
	}
	#endif /* __amigaos4__ */

	if(TimerBase != NULL)
	{
		CloseDevice((struct IORequest *)&TimerRequest);
		TimerBase = NULL;
	}

	#if defined(__amigaos4__)
	{
		if(ISocket != NULL)
		{
			DropInterface((struct Interface *)ISocket);
			ISocket = NULL;
		}
	}
	#endif /* __amigaos4__ */

	if(SocketBase != NULL)
	{
		CloseLibrary(SocketBase);
		SocketBase = NULL;
	}

	#if defined(__amigaos4__)
	{
		if(IUtility != NULL)
		{
			DropInterface((struct Interface *)IUtility);
			IUtility = NULL;
		}
	}
	#endif /* __amigaos4__ */

	if(UtilityBase != NULL)
	{
		CloseLibrary(UtilityBase);
		UtilityBase = NULL;
	}

	#if defined(__amigaos4__)
	{
		if(IIcon != NULL)
		{
			DropInterface((struct Interface *)IIcon);
			IIcon = NULL;
		}
	}
	#endif /* __amigaos4__ */

	if(IconBase != NULL)
	{
		CloseLibrary(IconBase);
		IconBase = NULL;
	}

	if(Locale != NULL)
	{
		CloseLocale(Locale);
		Locale = NULL;
	}

	#if defined(__amigaos4__)
	{
		if(ILocale != NULL)
		{
			DropInterface((struct Interface *)ILocale);
			ILocale = NULL;
		}
	}
	#endif /* __amigaos4__ */

	if(LocaleBase != NULL)
	{
		CloseLibrary(LocaleBase);
		LocaleBase = NULL;
	}

	if(MemoryPool != NULL)
	{
		DeletePool(MemoryPool);
		MemoryPool = NULL;
	}

	#if defined(__amigaos4__)
	{
		if(IDOS != NULL)
		{
			DropInterface((struct Interface *)IDOS);
			IDOS = NULL;
		}
	}
	#endif /* __amigaos4__ */

	if(DOSBase != NULL)
	{
		CloseLibrary(DOSBase);
		DOSBase = NULL;
	}

	if(WBStartup != NULL)
	{
		Forbid();
		ReplyMsg((struct Message *)WBStartup);
	}

	LEAVE();
}

/* Allocate all the necessary resources to get going. */
STATIC BOOL
Setup(
	STRPTR	program_name,
	STRPTR	service,
	STRPTR	workgroup,
	STRPTR 	username,
	STRPTR	opt_password,
	BOOL	opt_changecase,
	STRPTR	opt_clientname,
	STRPTR	opt_servername,
	int		opt_cachesize,
	LONG *	opt_time_zone_offset,
	LONG *	opt_dst_offset,
	STRPTR	device_name,
	STRPTR	volume_name,
	STRPTR	translation_file)
{
	BOOL result = FALSE;
	struct DosList * dl;
	int error;
	STRPTR actual_volume_name;
	LONG actual_volume_name_len;
	UBYTE name[MAX_FILENAME_LEN];
	BOOL device_exists = FALSE;
	LONG len,i;

	ENTER();

	NewList((struct List *)&FileList);
	NewList((struct List *)&LockList);

	MemoryPool = CreatePool(MEMF_ANY|MEMF_PUBLIC,4096,4096);
	if(MemoryPool == NULL)
	{
		ReportError("Could not create memory pool.");
		goto out;
	}

	LocaleBase = OpenLibrary("locale.library",38);

	#if defined(__amigaos4__)
	{
		if(LocaleBase != NULL)
		{
			ILocale = (struct LocaleIFace *)GetInterface(LocaleBase, "main", 1, 0);
			if(ILocale == NULL)
			{
				CloseLibrary(LocaleBase);
				LocaleBase = NULL;
			}
		}
	}
	#endif /* __amigaos4__ */

	if(LocaleBase != NULL)
		Locale = OpenLocale(NULL);

	if(opt_time_zone_offset != NULL)
	{
		TimeZoneOffset			= (*opt_time_zone_offset);
		OverrideLocaleTimeZone	= TRUE;
	}

	if(opt_dst_offset != NULL)
		DSTOffset = -60 * (*opt_dst_offset);

	memset(&TimerRequest,0,sizeof(TimerRequest));

	if(OpenDevice(TIMERNAME,UNIT_VBLANK,(struct IORequest *)&TimerRequest,0) != OK)
	{
		ReportError("Could not open 'timer.device'.");
		goto out;
	}

	TimerBase = (struct Library *)TimerRequest.tr_node.io_Device;

	#if defined(__amigaos4__)
	{
		if(TimerBase != NULL)
		{
			ITimer = (struct TimerIFace *)GetInterface(TimerBase, "main", 1, 0);
			if(ITimer == NULL)
			{
				ReportError("Could not open 'timer.device'.");
				goto out;
			}
		}
	}
	#endif /* __amigaos4__ */

	SocketBase = OpenLibrary("bsdsocket.library",3);

	#if defined(__amigaos4__)
	{
		if(SocketBase != NULL)
		{
			ISocket = (struct SocketIFace *)GetInterface(SocketBase, "main", 1, 0);
			if(ISocket == NULL)
			{
				CloseLibrary(SocketBase);
				SocketBase = NULL;
			}
		}
	}
	#endif /* __amigaos4__ */

	if(SocketBase == NULL)
	{
		ReportError("Could not open 'bsdsocket.library' V3; TCP/IP stack not running?");
		goto out;
	}

	error = SocketBaseTags(
		SBTM_SETVAL(SBTC_ERRNOPTR(sizeof(errno))),	&errno,
		SBTM_SETVAL(SBTC_HERRNOLONGPTR),			&h_errno,
		SBTM_SETVAL(SBTC_LOGTAGPTR),				program_name,
		SBTM_SETVAL(SBTC_BREAKMASK),				SIGBREAKF_CTRL_C,
	TAG_END);
	if(error != OK)
	{
		ReportError("Could not initialize 'bsdsocket.library' (%ld, %s).",error,amitcp_strerror(error));
		goto out;
	}

	if(opt_changecase)
	{
		for(i = 0 ; i < (LONG)strlen(opt_password) ; i++)
			opt_password[i] = ToUpper(opt_password[i]);
	}

	TranslateNames = FALSE;

	/* Read the translation file, if possible. */
	if(translation_file != NULL)
	{
		STRPTR msg = NULL;
		BPTR file;

		error = OK;

		file = Open(translation_file,MODE_OLDFILE);
		if(file != ZERO)
		{
			if(Read(file,A2M,256) != 256 ||
			   Read(file,M2A,256) != 256)
			{
				msg = "Could not read translation file";
				error = IoErr();
			}

			Close(file);
		}
		else
		{
			msg = "Could not open translation file";
			error = IoErr();
		}

		if(msg == NULL)
		{
			TranslateNames = TRUE;
		}
		else
		{
			UBYTE description[100];

			Fault(error,NULL,description,sizeof(description));
			for(i = ((int)strlen(description)) - 1 ; i >= 0 ; i--)
			{
				if(description[i] == '\n')
					description[i] = '\0';
			}

			ReportError("%s '%s' (%ld, %s).",msg,translation_file,error,description);
			goto out;
		}
	}

	error = smba_start(service,workgroup,username,opt_password,opt_clientname,opt_servername,opt_cachesize,&ServerData);
	if(error < 0)
		goto out;

	FileSystemPort = CreateMsgPort();
	if(FileSystemPort == NULL)
	{
		ReportError("Could not create filesystem port.");
		goto out;
	}

	/* If a device name was provided, check whether it is
	 * well-formed.
	 */
	if(device_name != NULL)
	{
		len = strlen(device_name);
		if(len > 255)
			len = 255;

		for(i = 0 ; i < len ; i++)
		{
			if(device_name[i] == '/')
			{
				ReportError("Device name '%s' cannot be used with AmigaDOS.",device_name);
				goto out;
			}
		}

		/* Lose any trailing colon characters. */
		for(i = len-1 ; i >= 0 ; i--)
		{
			if(device_name[i] == ':')
				len = i;
		}

		if(len == 0)
		{
			ReportError("Device name '%s' cannot be used with AmigaDOS.",device_name);
			goto out;
		}

		memcpy(name,device_name,len);
		name[len] = '\0';

		dl = LockDosList(LDF_WRITE|LDF_VOLUMES|LDF_DEVICES);

		if(FindDosEntry(dl,name,LDF_DEVICES) != NULL)
			device_exists = TRUE;
	}
	else
	{
		dl = LockDosList(LDF_WRITE|LDF_VOLUMES|LDF_DEVICES);

		/* Find a unique device name. */
		for(i = 0 ; i < 100 ; i++)
		{
			SPrintf(name,"SMBFS%ld",i);

			device_exists = (BOOL)(FindDosEntry(dl,name,LDF_DEVICES) != NULL);
			if(NOT device_exists)
			{
				device_name = name;
				break;
			}
		}
	}

	if(device_exists)
	{
		UnLockDosList(LDF_WRITE|LDF_VOLUMES|LDF_DEVICES);

		ReportError("Device name '%s:' is already taken.",device_name);
		goto out;
	}

	/* Finally, create the device node. */
	DeviceNode = MakeDosEntry(name,DLT_DEVICE);
	if(DeviceNode == NULL)
	{
		UnLockDosList(LDF_WRITE|LDF_VOLUMES|LDF_DEVICES);

		ReportError("Could not create device node.");
		goto out;
	}

	DeviceNode->dol_Task = FileSystemPort;

	/* Examine the volume name; make sure that it is
	 * well-formed.
	 */
	if(volume_name == NULL)
		actual_volume_name = device_name;
	else
		actual_volume_name = volume_name;

	actual_volume_name_len = strlen(actual_volume_name);
	if(actual_volume_name_len > 255)
		actual_volume_name_len = 255;

	for(i = 0 ; i < actual_volume_name_len ; i++)
	{
		if(actual_volume_name[i] == '/')
		{
			UnLockDosList(LDF_WRITE|LDF_VOLUMES|LDF_DEVICES);

			ReportError("Volume name '%s' cannot be used with AmigaDOS.",actual_volume_name);
			goto out;
		}
	}

	/* Lose any trailing colon characters. */
	for(i = actual_volume_name_len-1 ; i >= 0 ; i--)
	{
		if(actual_volume_name[i] == ':')
			actual_volume_name_len = i;
	}

	if(actual_volume_name_len == 0)
	{
		UnLockDosList(LDF_WRITE|LDF_VOLUMES|LDF_DEVICES);

		ReportError("Volume name '%s' cannot be used with AmigaDOS.",actual_volume_name);
		goto out;
	}

	/* Now, finally, take care of the volume name. */
	memcpy(name,actual_volume_name,actual_volume_name_len);
	name[actual_volume_name_len] = '\0';

	VolumeNode = MakeDosEntry(name,DLT_VOLUME);
	if(VolumeNode == NULL)
	{
		UnLockDosList(LDF_WRITE|LDF_VOLUMES|LDF_DEVICES);

		ReportError("Could not create volume node.");
		goto out;
	}

	VolumeNode->dol_Task = FileSystemPort;
	DateStamp(&VolumeNode->dol_misc.dol_volume.dol_VolumeDate);
	VolumeNode->dol_misc.dol_volume.dol_DiskType = ID_DOS_DISK;

	if(DeviceNode != NULL)
	{
		AddDosEntry(DeviceNode);

		DeviceNodeAdded = TRUE;
	}

	/* Note: we always need the volume node to make some file
	 *       system operations safe (e.g. Lock()), but we may
	 *       not always need to make it visible.
	 */
	if(volume_name != NULL && VolumeNode != NULL)
	{
 		AddDosEntry(VolumeNode);

		VolumeNodeAdded = TRUE;
	}

	/* And that concludes the mounting operation. */
	UnLockDosList(LDF_WRITE|LDF_VOLUMES|LDF_DEVICES);

	/* Tell Workbench and friends to update their volume lists. */
	if(VolumeNodeAdded)
		SendDiskChange(IECLASS_DISKINSERTED);

	SetProgramName(NewProgramName);

	result = TRUE;

 out:

	RETURN(result);
	return(result);
}

/****************************************************************************/


/* Convert a BCPL string into a standard NUL terminated 'C' string. */
#if !defined(__AROS__)
INLINE STATIC VOID
ConvertBString(LONG max_len,STRPTR cstring,APTR bstring)
{
	STRPTR from = bstring;
	LONG len = from[0];

	if(len > max_len-1)
		len = max_len-1;

	if(len > 0)
		memcpy(cstring,from+1,len);

	cstring[len] = '\0';
}
#else
INLINE STATIC VOID
ConvertBString(LONG max_len,STRPTR cstring,APTR bstring)
{
        UWORD _i = 0;
	UWORD _len = AROS_BSTR_strlen(bstring);

        while((_i < _len) && (_i < max_len))
        {
            cstring[_i] = AROS_BSTR_getchar(bstring, _i);
            _i++;
        }

	cstring[_i] = '\0';
}
#endif

/* Convert a NUL terminated 'C' string into a BCPL string. */
#if !defined(__AROS__)
INLINE STATIC VOID
ConvertCString(LONG max_len, APTR bstring, STRPTR cstring)
{
	LONG len = strlen(cstring);
	STRPTR to = bstring;

	if(len > max_len-1)
		len = max_len-1;

	(*to++) = len;
	memcpy(to,cstring,len);
}
#else
INLINE STATIC VOID
ConvertCString(LONG max_len, APTR bstring, STRPTR cstring)
{
        UWORD _i = 0;
        while((cstring[_i] != '\0') && (_i < max_len))
        {
            AROS_BSTR_putchar(bstring, _i, cstring[_i]);
            _i++;
        }
        AROS_BSTR_setstrlen(bstring, _i);
}
#endif

/****************************************************************************/

/* Build the fully qualified name of a file or directory in reference
 * to the name of the parent directory. This takes care of all the
 * special cases, such as the root directory. The result will be converted
 * to be in a form suitable for use with the SMB file sharing service.
 */
STATIC LONG
BuildFullName(
	STRPTR		parent_name,
	STRPTR		name,
	STRPTR *	result_ptr,
	LONG *		result_size_ptr)
{
	LONG error = OK;
	STRPTR buffer;
	LONG len,size;
	LONG i;

	ENTER();

	SHOWSTRING(parent_name);
	SHOWSTRING(name);

	(*result_ptr) = NULL;

	/* Throw everything left of the colon away. */
	if(name != NULL)
	{
		for(i = 0 ; i < (LONG)strlen(name) ; i++)
		{
			if(name[i] == ':')
			{
				name = &name[i+1];
				break;
			}
		}
	}

	/* Now, how much room is needed for the complete
	 * path to fit into a buffer?
	 */
	len = 2;

	if(parent_name != NULL)
		len += strlen(parent_name) + 1;

	if(name != NULL)
		len += strlen(name) + 1;

	if(len < SMB_MAXNAMELEN)
		len = SMB_MAXNAMELEN;

	size = len + 3;

	buffer = AllocateMemory(size);
	if(buffer == NULL)
	{
		error = ERROR_NO_FREE_STORE;
		goto out;
	}

	/* Start by filling in the path name. */
	if(parent_name != NULL)
	{
		/* Skip any excess separators. */
		while((*parent_name) == SMB_PATH_SEPARATOR)
			parent_name++;

		buffer[0] = SMB_PATH_SEPARATOR;
		strcpy(&buffer[1],parent_name);
	}
	else
	{
		strcpy(buffer,SMB_ROOT_DIR_NAME);
	}

	/* If there's a name to add, do just that. */
	if(name != NULL)
	{
		LONG segment_start;
		LONG segment_len;
		LONG buffer_len;
		LONG name_len;

		buffer_len = strlen(buffer);
		name_len = strlen(name);

		segment_start = 0;

		while(TRUE)
		{
			segment_len = 0;

			/* Extract the next path name segment. */
			for(i = segment_start ; i <= name_len ; i++)
			{
				if(i == name_len)
				{
					segment_len = i - segment_start;
					break;
				}
				else if (name[i] == '/')
				{
					segment_len = i - segment_start + 1;
					break;
				}
			}

			/* We're finished if there are no further
			 * path name segments to take care of.
			 */
			if(segment_len == 0)
			{
				buffer[buffer_len] = '\0';
				break;
			}

			/* A single slash indicates that we need to move up
			 * to the parent directory, if any.
			 */
			if(segment_len == 1 && name[segment_start] == '/')
			{
				/* Is this already the root directory name? */
				if(buffer_len <= 1)
				{
					FreeMemory(buffer);
					buffer = NULL;

					goto out;
				}
				else
				{
					/* Skip the last path component. */
					for(i = 1 ; i <= buffer_len ; i++)
					{
						if(i == buffer_len)
						{
							/* We just skipped the first path
							 * component following the root
							 * directory name. We preserve
							 * the first character since it
							 * refers to the root directory.
							 */
							buffer_len = 1;
							break;
						}
						else if (buffer[buffer_len-i] == SMB_PATH_SEPARATOR)
						{
							/* This removes both the path separator and
							 * the name following it.
							 */
							buffer_len -= i;
							break;
						}
					}
				}
			}
			else
			{
				/* Add a proper separator character if
				 * necessary.
				 */
				if(buffer_len > 0 && buffer[buffer_len-1] != SMB_PATH_SEPARATOR)
					buffer[buffer_len++] = SMB_PATH_SEPARATOR;

				/* Find out how many characters are in that name; this
				 * excludes the terminating slash.
				 */
				if(name[segment_start + segment_len - 1] == '/')
					len = segment_len - 1;
				else
					len = segment_len;

				memcpy(&buffer[buffer_len],&name[segment_start],len);
				buffer_len += len;
			}

			segment_start += segment_len;
		}
	}

	(*result_ptr) = buffer;
	(*result_size_ptr) = size;

	SHOWSTRING(buffer);

 out:

	if(error != OK)
		FreeMemory(buffer);

	RETURN(error);
	return(error);
}

/****************************************************************************/

STATIC BPTR
Action_Parent(
	struct FileLock *	parent,
	SIPTR *				error_ptr)
{
	BPTR result = ZERO;
	STRPTR full_name = NULL;
	LONG full_name_size;
	STRPTR parent_name;
	BOOL cleanup = TRUE;
	struct LockNode * ln = NULL;
	LONG error;

	ENTER();

	SHOWVALUE(parent);

	if(parent != NULL)
	{
		struct LockNode * parent_ln = (struct LockNode *)parent->fl_Key;

		parent_name = parent_ln->ln_FullName;
	}
	else
	{
		parent_name = NULL;
	}

	error = BuildFullName(parent_name,"/",&full_name,&full_name_size);
	if(error != OK)
		goto out;

	/* Check if we ended up having to return the parent of
	 * the root directory. This is indicated by a NULL
	 * name pointer and a zero error code.
	 */
	if(full_name == NULL)
		goto out;

	ln = AllocateMemory(sizeof(*ln));
	if(ln == NULL)
	{
		error = ERROR_NO_FREE_STORE;
		goto out;
	}

	memset(ln,0,sizeof(*ln));

	ln->ln_FileLock.fl_Key		= (IPTR)ln;
	ln->ln_FileLock.fl_Access	= SHARED_LOCK;
	ln->ln_FileLock.fl_Task		= FileSystemPort;
	ln->ln_FileLock.fl_Volume	= MKBADDR(VolumeNode);
	ln->ln_FullName				= full_name;

	SHOWSTRING(full_name);

	error = smba_open(ServerData,full_name,full_name_size,&ln->ln_File);
	if(error < 0)
	{
		error = MapErrnoToIoErr(error);
		goto out;
	}

	AddTail((struct List *)&LockList,(struct Node *)ln);
	result = MKBADDR(&ln->ln_FileLock);
	cleanup = FALSE;
	SHOWVALUE(&ln->ln_FileLock);

 out:

	if(cleanup)
	{
		FreeMemory(full_name);
		FreeMemory(ln);
	}

	(*error_ptr) = error;

	RETURN(result);
	return(result);
}

/****************************************************************************/

/* Find the lock node corresponding to a given name,
 * starting from node start. (if node, this one is skipped)
 */
STATIC struct LockNode *
FindNextLockNode(STRPTR name,struct LockNode * last_ln)
{
	struct LockNode * result = NULL;
	struct LockNode * ln;
	struct LockNode * start;

	if(last_ln != NULL)
		start = (struct LockNode *)last_ln->ln_MinNode.mln_Succ;
	else
		start = (struct LockNode *)LockList.mlh_Head;

	for(ln = start ;
	    ln->ln_MinNode.mln_Succ != NULL ;
	    ln = (struct LockNode *)ln->ln_MinNode.mln_Succ)
	{
		if(CompareNames(name,ln->ln_FullName) == SAME)
		{
			result = ln;
			break;
		}
	}

	return(result);
}

/****************************************************************************/

STATIC LONG
Action_DeleteObject(
	struct FileLock *	parent,
	APTR				bcpl_name,
	SIPTR *				error_ptr)
{
	LONG result = DOSFALSE;
	STRPTR full_name = NULL;
	LONG full_name_size;
	smba_file_t * file = NULL;
	STRPTR parent_name;
	STRPTR full_parent_name = NULL;
	UBYTE name[MAX_FILENAME_LEN];
	struct LockNode * ln;
	smba_stat_t st;
	LONG error;

	ENTER();

	if(WriteProtected)
	{
		error = ERROR_DISK_WRITE_PROTECTED;
		goto out;
	}

	SHOWVALUE(parent);

	if(parent != NULL)
	{
		ln = (struct LockNode *)parent->fl_Key;

		parent_name = ln->ln_FullName;
	}
	else
	{
		parent_name = NULL;
	}

	ConvertBString(sizeof(name),name,bcpl_name);
	TranslateCName(name,A2M);

	error = BuildFullName(parent_name,name,&full_name,&full_name_size);
	if(error != OK)
		goto out;

	/* Trying to delete the root directory, are you kidding? */
	if(full_name == NULL)
	{
		error = ERROR_OBJECT_WRONG_TYPE;
		goto out;
	}

	/* We need to find this file's parent directory, so that
	 * in case the directory contents are currently being
	 * examined, that process is restarted.
	 */
	full_parent_name = AllocateMemory(strlen(full_name)+3);
	if(full_parent_name == NULL)
	{
		error = ERROR_NO_FREE_STORE;
		goto out;
	}

	strcpy(full_parent_name,full_name);

	/* Build the parent object name - Piru */
	if (full_parent_name[0] != '\0')
	{
		int i;

		i = strlen(full_parent_name) - 1;
		if (full_parent_name[i] == SMB_PATH_SEPARATOR)
			i--;

		for ( ; i >= 0 ; i--)
		{
			if (full_parent_name[i] == SMB_PATH_SEPARATOR)
			{
				full_parent_name[i] = '\0';
				break;
			}
		}
	}

	/* NOTE: Mark all locks to this object as restart, not just first
	   one - Piru */
	ln = NULL;
	while ((ln = FindNextLockNode(full_parent_name, ln)) != NULL)
		ln->ln_RestartExamine = TRUE;

	ln = FindLockNode(full_parent_name,NULL);
	if(ln != NULL)
		ln->ln_RestartExamine = TRUE;

	FreeMemory(full_parent_name);
	full_parent_name = NULL;

	SHOWSTRING(full_name);

	error = smba_open(ServerData,full_name,full_name_size,&file);
	if(error < 0)
	{
		error = MapErrnoToIoErr(error);
		goto out;
	}

	error = smba_getattr(file,&st);
	if(error < 0)
	{
		error = MapErrnoToIoErr(error);
		goto out;
	}

	smba_close(file);
	file = NULL;

	if(st.is_dir)
	{
		SHOWMSG("removing a directory");

		error = smba_rmdir(ServerData,full_name);
		if(error < 0)
		{
			SHOWVALUE(error);

			/* This is a little bit difficult to justify since
			 * the error code may indicate a different cause,
			 * but in practice 'EACCES' seems to be returned
			 * if the directory to remove is not empty.
			 */
			if(error == (-EACCES))
				error = ERROR_DIRECTORY_NOT_EMPTY;
			else
				error = MapErrnoToIoErr(error);

			goto out;
		}
	}
	else
	{
		SHOWMSG("removing a file");

		error = smba_remove(ServerData,full_name);
		if(error < 0)
		{
			SHOWVALUE(error);

			error = MapErrnoToIoErr(error);
			goto out;
		}
	}

	SHOWMSG("done.");

	result = DOSTRUE;

 out:

	FreeMemory(full_name);
	FreeMemory(full_parent_name);
	if(file != NULL)
		smba_close(file);

	(*error_ptr) = error;

	RETURN(result);
	return(result);
}

/****************************************************************************/

STATIC BPTR
Action_CreateDir(
	struct FileLock *	parent,
	APTR				bcpl_name,
	SIPTR *				error_ptr)
{
	BPTR result = ZERO;
	STRPTR full_name = NULL;
	LONG full_name_size;
	struct LockNode * ln = NULL;
	STRPTR parent_name;
	STRPTR dir_name = NULL;
	smba_file_t * dir = NULL;
	STRPTR base_name;
	UBYTE name[MAX_FILENAME_LEN];
	LONG error;
	LONG i;

	ENTER();

	if(WriteProtected)
	{
		error = ERROR_DISK_WRITE_PROTECTED;
		goto out;
	}

	SHOWVALUE(parent);

	if(parent != NULL)
	{
		struct LockNode * parent_ln = (struct LockNode *)parent->fl_Key;

		parent_name = parent_ln->ln_FullName;
	}
	else
	{
		parent_name = NULL;
	}

	ConvertBString(sizeof(name),name,bcpl_name);
	TranslateCName(name,A2M);

	error = BuildFullName(parent_name,name,&full_name,&full_name_size);
	if(error != OK)
		goto out;

	/* Trying to overwrite the root directory, are you kidding? */
	if(full_name == NULL)
	{
		error = ERROR_OBJECT_IN_USE;
		goto out;
	}

	dir_name = AllocateMemory(strlen(full_name)+3);
	if(dir_name == NULL)
	{
		error = ERROR_NO_FREE_STORE;
		goto out;
	}

	strcpy(dir_name,full_name);
	base_name = NULL;
	for(i = strlen(dir_name)-1 ; i >= 0 ; i--)
	{
		if(dir_name[i] == SMB_PATH_SEPARATOR)
		{
			if(i == 0)
			{
				memmove(&dir_name[1],&dir_name[0],strlen(dir_name)+1);
				i++;
			}

			dir_name[i] = '\0';

			base_name = &dir_name[i+1];
			break;
		}
	}

	ln = AllocateMemory(sizeof(*ln));
	if(ln == NULL)
	{
		error = ERROR_NO_FREE_STORE;
		goto out;
	}

	memset(ln,0,sizeof(*ln));

	ln->ln_FileLock.fl_Key		= (IPTR)ln;
	ln->ln_FileLock.fl_Access	= EXCLUSIVE_LOCK;
	ln->ln_FileLock.fl_Task		= FileSystemPort;
	ln->ln_FileLock.fl_Volume	= MKBADDR(VolumeNode);
	ln->ln_FullName				= full_name;

	error = smba_open(ServerData,dir_name,strlen(full_name)+3,&dir);
	if(error < 0)
	{
		error = MapErrnoToIoErr(error);
		goto out;
	}

	error = smba_mkdir(dir,base_name);
	if(error < 0)
	{
		error = MapErrnoToIoErr(error);
		goto out;
	}

	smba_close(dir);
	dir = NULL;

	SHOWSTRING(full_name);

	error = smba_open(ServerData,full_name,full_name_size,&ln->ln_File);
	if(error < 0)
	{
		error = MapErrnoToIoErr(error);
		goto out;
	}

	AddTail((struct List *)&LockList,(struct Node *)ln);
	result = MKBADDR(&ln->ln_FileLock);
	SHOWVALUE(&ln->ln_FileLock);

 out:

	if(dir != NULL)
		smba_close(dir);

	FreeMemory(dir_name);

	if(result == ZERO)
	{
		FreeMemory(full_name);
		FreeMemory(ln);
	}

	(*error_ptr) = error;

	RETURN(result);
	return(result);
}

/****************************************************************************/

STATIC BPTR
Action_LocateObject(
	struct FileLock *	parent,
	APTR				bcpl_name,
	LONG				mode,
	SIPTR *				error_ptr)
{
	BPTR result = ZERO;
	STRPTR full_name = NULL;
	LONG full_name_size;
	struct LockNode * ln = NULL;
	STRPTR parent_name;
	UBYTE name[MAX_FILENAME_LEN];
	LONG error;

	ENTER();

	SHOWVALUE(parent);

	if(parent != NULL)
	{
		struct LockNode * parent_ln = (struct LockNode *)parent->fl_Key;

		parent_name = parent_ln->ln_FullName;
	}
	else
	{
		parent_name = NULL;
	}

	ConvertBString(sizeof(name),name,bcpl_name);
	TranslateCName(name,A2M);

	if(IsReservedName(FilePart(name)))
	{
		error = ERROR_OBJECT_NOT_FOUND;
		goto out;
	}

	error = BuildFullName(parent_name,name,&full_name,&full_name_size);
	if(error != OK)
		goto out;

	/* Trying to get a lock on the root directory's parent?
	 * My pleasure.
	 */
	if(full_name == NULL)
		goto out;

	ln = AllocateMemory(sizeof(*ln));
	if(ln == NULL)
	{
		error = ERROR_NO_FREE_STORE;
		goto out;
	}

	memset(ln,0,sizeof(*ln));

	ln->ln_FileLock.fl_Key		= (IPTR)ln;
	ln->ln_FileLock.fl_Access	= (mode != EXCLUSIVE_LOCK) ? SHARED_LOCK : EXCLUSIVE_LOCK;
	ln->ln_FileLock.fl_Task		= FileSystemPort;
	ln->ln_FileLock.fl_Volume	= MKBADDR(VolumeNode);
	ln->ln_FullName				= full_name;

	error = CheckAccessModeCollision(full_name,ln->ln_FileLock.fl_Access);
	if(error != OK)
		goto out;

	SHOWSTRING(full_name);

	error = smba_open(ServerData,full_name,full_name_size,&ln->ln_File);
	if(error < 0)
	{
		error = MapErrnoToIoErr(error);
		goto out;
	}

	AddTail((struct List *)&LockList,(struct Node *)ln);
	result = MKBADDR(&ln->ln_FileLock);
	SHOWVALUE(&ln->ln_FileLock);

 out:

	if(result == ZERO)
	{
		FreeMemory(full_name);
		FreeMemory(ln);
	}

	(*error_ptr) = error;

	RETURN(result);
	return(result);
}

/****************************************************************************/

STATIC BPTR
Action_CopyDir(
	struct FileLock *	lock,
	SIPTR *				error_ptr)
{
	BPTR result = ZERO;
	STRPTR full_name = NULL;
	LONG full_name_size;
	struct LockNode * ln = NULL;
	STRPTR source_name;
	LONG source_mode;
	LONG error;

	ENTER();

	SHOWVALUE(lock);

	if(lock != NULL && lock->fl_Access != SHARED_LOCK)
	{
		SHOWMSG("cannot duplicate exclusive lock");
		error = ERROR_OBJECT_IN_USE;
		goto out;
	}

	ln = AllocateMemory(sizeof(*ln));
	if(ln == NULL)
	{
		error = ERROR_NO_FREE_STORE;
		goto out;
	}

	memset(ln,0,sizeof(*ln));

	if(lock != NULL)
	{
		struct LockNode * source = (struct LockNode *)lock->fl_Key;

		source_name = source->ln_FullName;
		source_mode = source->ln_FileLock.fl_Access;
	}
	else
	{
		source_name = SMB_ROOT_DIR_NAME;
		source_mode = SHARED_LOCK;
	}

	full_name_size = strlen(source_name)+3;
	if(full_name_size < SMB_MAXNAMELEN+1)
		full_name_size = SMB_MAXNAMELEN+1;

	full_name = AllocateMemory(full_name_size);
	if(full_name == NULL)
	{
		error = ERROR_NO_FREE_STORE;
		goto out;
	}

	strcpy(full_name,source_name);

	ln->ln_FileLock.fl_Key		= (IPTR)ln;
	ln->ln_FileLock.fl_Access	= source_mode;
	ln->ln_FileLock.fl_Task		= FileSystemPort;
	ln->ln_FileLock.fl_Volume	= MKBADDR(VolumeNode);
	ln->ln_FullName				= full_name;

	SHOWSTRING(full_name);

	error = smba_open(ServerData,full_name,full_name_size,&ln->ln_File);
	if(error < 0)
	{
		error = MapErrnoToIoErr(error);
		goto out;
	}

	AddTail((struct List *)&LockList,(struct Node *)ln);
	result = MKBADDR(&ln->ln_FileLock);
	SHOWVALUE(&ln->ln_FileLock);

 out:

	if(result == ZERO)
	{
		FreeMemory(full_name);
		FreeMemory(ln);
	}

	(*error_ptr) = error;

	RETURN(result);
	return(result);
}

/****************************************************************************/

STATIC LONG
Action_FreeLock(
	struct FileLock *	lock,
	SIPTR *				error_ptr)
{
	LONG result = DOSTRUE;
	struct LockNode * ln;
	LONG error = OK;

	ENTER();

	SHOWVALUE(lock);

	if(lock == NULL)
		goto out;

	ln = (struct LockNode *)lock->fl_Key;

	Remove((struct Node *)ln);
	smba_close(ln->ln_File);
	FreeMemory(ln->ln_FullName);
	FreeMemory(ln);

 out:

	(*error_ptr) = error;

	RETURN(result);
	return(result);
}

/****************************************************************************/

STATIC LONG
Action_SameLock(
	struct FileLock *	lock1,
	struct FileLock *	lock2,
	SIPTR *				error_ptr)
{
	LONG result = DOSFALSE;
	STRPTR name1;
	STRPTR name2;
	LONG error = OK;

	ENTER();

	SHOWVALUE(lock1);
	SHOWVALUE(lock2);

	if(lock1 != NULL)
	{
		struct LockNode * ln = (struct LockNode *)lock1->fl_Key;

		name1 = ln->ln_FullName;
	}
	else
	{
		name1 = SMB_ROOT_DIR_NAME;
	}

	if(lock2 != NULL)
	{
		struct LockNode * ln = (struct LockNode *)lock2->fl_Key;

		name2 = ln->ln_FullName;
	}
	else
	{
		name2 = SMB_ROOT_DIR_NAME;
	}

	SHOWSTRING(name1);
	SHOWSTRING(name2);

	if(Stricmp(name1,name2) == SAME)
		result = DOSTRUE;

	(*error_ptr) = error;

	RETURN(result);
	return(result);
}

/****************************************************************************/

STATIC LONG
Action_SetProtect(
	struct FileLock *	parent,
	APTR				bcpl_name,
	LONG				mask,
	SIPTR *				error_ptr)
{
	LONG result = DOSFALSE;
	STRPTR full_name = NULL;
	LONG full_name_size;
	smba_file_t * file = NULL;
	STRPTR parent_name;
	UBYTE name[MAX_FILENAME_LEN];
	smba_stat_t st;
	LONG error;

	ENTER();

	if(WriteProtected)
	{
		error = ERROR_DISK_WRITE_PROTECTED;
		goto out;
	}

	SHOWVALUE(parent);

	if(parent != NULL)
	{
		struct LockNode * ln = (struct LockNode *)parent->fl_Key;

		parent_name = ln->ln_FullName;
	}
	else
	{
		parent_name = NULL;
	}

	ConvertBString(sizeof(name),name,bcpl_name);
	TranslateCName(name,A2M);

	error = BuildFullName(parent_name,name,&full_name,&full_name_size);
	if(error != OK)
		goto out;

	/* Trying to change the protection bits of the root
	 * directory, are you kidding?
	 */
	if(full_name == NULL)
	{
		error = ERROR_OBJECT_WRONG_TYPE;
		goto out;
	}

	SHOWSTRING(full_name);

	error = smba_open(ServerData,full_name,full_name_size,&file);
	if(error < 0)
	{
		error = MapErrnoToIoErr(error);
		goto out;
	}

	memset(&st,0,sizeof(st));

	mask ^= FIBF_READ | FIBF_WRITE | FIBF_EXECUTE | FIBF_DELETE;

	st.atime = -1;
	st.ctime = -1;
	st.mtime = -1;
	st.size = -1;

	if((mask & (FIBF_WRITE|FIBF_DELETE)) != (FIBF_WRITE|FIBF_DELETE))
	{
		SHOWMSG("write protection enabled");
		st.is_wp = TRUE;
	}
	else
	{
		SHOWMSG("write protection disabled");
	}

	/* Careful: the 'archive' attribute has exactly the opposite
	 *          meaning in the Amiga and the SMB worlds.
	 */
	st.is_archive = ((mask & FIBF_ARCHIVE) == 0);

	/* The 'system' attribute is associated with the 'pure' bit for now. */
	st.is_system = ((mask & FIBF_PURE) != 0);

	error = smba_setattr(file,&st);
	if(error < 0)
	{
		error = MapErrnoToIoErr(error);
		goto out;
	}

	result = DOSTRUE;

 out:

	FreeMemory(full_name);
	if(file != NULL)
		smba_close(file);

	(*error_ptr) = error;

	RETURN(result);
	return(result);
}

/****************************************************************************/

STATIC LONG
Action_RenameObject(
	struct FileLock *	source_lock,
	APTR				source_bcpl_name,
	struct FileLock *	destination_lock,
	APTR				destination_bcpl_name,
	SIPTR *				error_ptr)
{
	struct LockNode * ln;
	LONG result = DOSFALSE;
	STRPTR full_source_name = NULL;
	LONG full_source_name_size;
	STRPTR full_destination_name = NULL;
	LONG full_destination_name_size;
	UBYTE name[MAX_FILENAME_LEN];
	STRPTR parent_name;
	LONG error;

	ENTER();

	if(WriteProtected)
	{
		error = ERROR_DISK_WRITE_PROTECTED;
		goto out;
	}

	SHOWVALUE(source_lock);
	SHOWVALUE(destination_lock);

	if(source_lock != NULL)
	{
		ln = (struct LockNode *)source_lock->fl_Key;

		parent_name = ln->ln_FullName;
	}
	else
	{
		parent_name = NULL;
	}

	ConvertBString(sizeof(name),name,source_bcpl_name);
	TranslateCName(name,A2M);

	error = BuildFullName(parent_name,name,&full_source_name,&full_source_name_size);
	if(error != OK)
		goto out;

	/* Trying to rename the root directory, are you kidding? */
	if(full_source_name == NULL)
	{
		error = ERROR_OBJECT_IN_USE;
		goto out;
	}

	if(destination_lock != NULL)
	{
		ln = (struct LockNode *)destination_lock->fl_Key;

		parent_name = ln->ln_FullName;
	}
	else
	{
		parent_name = NULL;
	}

	ConvertBString(sizeof(name),name,destination_bcpl_name);
	TranslateCName(name,A2M);

	error = BuildFullName(parent_name,name,&full_destination_name,&full_destination_name_size);
	if(error != OK)
		goto out;

	/* Trying to rename the root directory, are you kidding? */
	if(full_destination_name == NULL)
	{
		error = ERROR_OBJECT_IN_USE;
		goto out;
	}

	error = NameAlreadyInUse(full_source_name);
	if(error != OK)
		goto out;

	error = NameAlreadyInUse(full_destination_name);
	if(error != OK)
		goto out;

	SHOWSTRING(full_source_name);
	SHOWSTRING(full_destination_name);

	error = smba_rename(ServerData,full_source_name,full_destination_name);
	if(error < 0)
	{
		error = MapErrnoToIoErr(error);
		goto out;
	}

	result = DOSTRUE;

 out:

	FreeMemory(full_source_name);
	FreeMemory(full_destination_name);

	(*error_ptr) = error;

	RETURN(result);
	return(result);
}

/****************************************************************************/

STATIC LONG
Action_DiskInfo(
	struct InfoData *	id,
	SIPTR *				error_ptr)
{
	LONG result = DOSTRUE;
	long block_size;
	long num_blocks;
	long num_blocks_free;
	LONG error;

	ENTER();

	memset(id,0,sizeof(*id));

	if(WriteProtected)
		id->id_DiskState = ID_WRITE_PROTECTED;
	else
		id->id_DiskState = ID_VALIDATED;

	error = smba_statfs(ServerData,&block_size,&num_blocks,&num_blocks_free);
	if(error >= 0)
	{
		SHOWMSG("got the disk data");
		SHOWVALUE(block_size);
		SHOWVALUE(num_blocks);
		SHOWVALUE(num_blocks_free);

		if(block_size <= 0)
			block_size = 512;

		if(block_size < 512)
		{
			num_blocks		/= (512 / block_size);
			num_blocks_free	/= (512 / block_size);
		}
		else if (block_size > 512)
		{
			num_blocks		*= (block_size / 512);
			num_blocks_free	*= (block_size / 512);
		}

		id->id_NumBlocks		= num_blocks;
		id->id_NumBlocksUsed	= num_blocks - num_blocks_free;
		id->id_BytesPerBlock	= 512;
		id->id_DiskType			= ID_DOS_DISK;
		id->id_VolumeNode		= MKBADDR(VolumeNode);
		id->id_InUse			= NOT (IsListEmpty((struct List *)&FileList) && IsListEmpty((struct List *)&LockList));

		if(id->id_NumBlocks == 0)
			id->id_NumBlocks = 1;

		if(id->id_NumBlocksUsed == 0)
			id->id_NumBlocksUsed = 1;
	}
	else
	{
		SHOWMSG("could not get any disk data");

		id->id_NumBlocks		= 1;
		id->id_NumBlocksUsed	= 1;
		id->id_BytesPerBlock	= 512;
		id->id_DiskType			= ID_NO_DISK_PRESENT;

		error = MapErrnoToIoErr(error);
		result = DOSFALSE;
	}

	SHOWVALUE(id->id_NumBlocks);
	SHOWVALUE(id->id_NumBlocksUsed);
	SHOWVALUE(id->id_BytesPerBlock);
	SHOWVALUE(id->id_DiskType);
	SHOWVALUE(id->id_VolumeNode);
	SHOWVALUE(id->id_InUse);

	(*error_ptr) = error;

	RETURN(result);
	return(result);
}

STATIC LONG
Action_Info(
	struct FileLock *	lock,
	struct InfoData *	id,
	SIPTR *				error_ptr)
{
	LONG result;

	ENTER();

	SHOWVALUE(lock);

	if(lock == NULL || lock->fl_Volume != MKBADDR(VolumeNode))
	{
		SHOWMSG("volume node does not match");

		result = DOSFALSE;

		(*error_ptr) = ERROR_NO_DISK;
	}
	else
	{
		result = Action_DiskInfo(id,error_ptr);
	}

	RETURN(result);
	return(result);
}

/****************************************************************************/

STATIC LONG
Action_ExamineObject(
	struct FileLock *		lock,
	struct FileInfoBlock *	fib,
	SIPTR *					error_ptr)
{
	LONG result = DOSFALSE;
	LONG error = OK;

	ENTER();

	SHOWVALUE(lock);

	memset(fib,0,sizeof(*fib));

	if(lock == NULL)
	{
#if !defined(__AROS__)
		STRPTR volume_name = BADDR(VolumeNode->dol_Name);
		LONG len = volume_name[0];
	    
		memcpy(fib->fib_FileName+1, volume_name + 1, len);
		fib->fib_FileName[0] = len;
#else
		STRPTR volume_name = AROS_BSTR_ADDR(VolumeNode->dol_Name);
		LONG len = AROS_BSTR_strlen(VolumeNode->dol_Name);

		memcpy(fib->fib_FileName, volume_name, len);
#endif
		SHOWMSG("ZERO root lock");

		fib->fib_DirEntryType	= ST_ROOT;
		fib->fib_EntryType		= ST_ROOT;
		fib->fib_NumBlocks		= 1;
		fib->fib_Date			= VolumeNode->dol_misc.dol_volume.dol_VolumeDate;
		fib->fib_DiskKey		= -1;
		fib->fib_Protection		= FIBF_OTR_READ|FIBF_OTR_EXECUTE|FIBF_OTR_WRITE|FIBF_OTR_DELETE|
								  FIBF_GRP_READ|FIBF_GRP_EXECUTE|FIBF_GRP_WRITE|FIBF_GRP_DELETE;
	}
	else
	{
		struct LockNode * ln = (struct LockNode *)lock->fl_Key;
		LONG seconds;
		smba_stat_t st;

		error = smba_getattr(ln->ln_File,&st);
		if(error < 0)
		{
			SHOWMSG("information not available");

			error = MapErrnoToIoErr(error);
			goto out;
		}

		seconds = st.mtime - UNIX_TIME_OFFSET - GetTimeZoneDelta();
		if(seconds < 0)
			seconds = 0;

		fib->fib_Date.ds_Days	= (seconds / (24 * 60 * 60));
		fib->fib_Date.ds_Minute	= (seconds % (24 * 60 * 60)) / 60;
		fib->fib_Date.ds_Tick	= (seconds % 60) * TICKS_PER_SECOND;

		SHOWSTRING(ln->ln_FullName);

		if(strcmp(ln->ln_FullName,SMB_ROOT_DIR_NAME) == SAME)
		{
#if !defined(__AROS__)
			STRPTR volume_name = BADDR(VolumeNode->dol_Name);
			LONG len = volume_name[0];

			memcpy(fib->fib_FileName+1, volume_name + 1, len);
			fib->fib_FileName[0] = len;
#else
			STRPTR volume_name = AROS_BSTR_ADDR(VolumeNode->dol_Name);
			LONG len = AROS_BSTR_strlen(VolumeNode->dol_Name);

			memcpy(fib->fib_FileName, volume_name, len);
#endif
			SHOWMSG("root lock");

			fib->fib_DirEntryType	= ST_ROOT;
			fib->fib_EntryType		= ST_ROOT;
			fib->fib_NumBlocks		= 1;
			fib->fib_Protection		= FIBF_OTR_READ|FIBF_OTR_EXECUTE|FIBF_OTR_WRITE|FIBF_OTR_DELETE|
									  FIBF_GRP_READ|FIBF_GRP_EXECUTE|FIBF_GRP_WRITE|FIBF_GRP_DELETE;
		}
		else
		{
			STRPTR name;
			LONG i;

			name = ln->ln_FullName;
			for(i = strlen(name)-1 ; i >= 0 ; i--)
			{
				if(name[i] == SMB_PATH_SEPARATOR)
				{
					name = &name[i+1];
					break;
				}
			}

			/* Just checking: will the name fit? */
			if(strlen(name) >= sizeof(fib->fib_FileName))
			{
				error = ERROR_INVALID_COMPONENT_NAME;
				goto out;
			}

			ConvertCString(sizeof(fib->fib_FileName),fib->fib_FileName,name);
			TranslateBName(fib->fib_FileName,M2A);

			fib->fib_DirEntryType	= st.is_dir ? ST_USERDIR : ST_FILE;
			fib->fib_EntryType		= fib->fib_DirEntryType;
			fib->fib_NumBlocks		= (st.size + 511) / 512;
			fib->fib_Size			= st.size;
			fib->fib_Protection		= FIBF_OTR_READ|FIBF_OTR_EXECUTE|FIBF_OTR_WRITE|FIBF_OTR_DELETE|
									  FIBF_GRP_READ|FIBF_GRP_EXECUTE|FIBF_GRP_WRITE|FIBF_GRP_DELETE;

			if(st.is_wp)
				fib->fib_Protection ^= (FIBF_OTR_WRITE|FIBF_OTR_DELETE|FIBF_GRP_WRITE|FIBF_GRP_DELETE|FIBF_WRITE|FIBF_DELETE);

			/* Careful: the 'archive' attribute has exactly the opposite
			 *          meaning in the Amiga and the SMB worlds.
			 */
			if(NOT st.is_archive)
				fib->fib_Protection |= FIBF_ARCHIVE;

			if(st.is_system)
				fib->fib_Protection |= FIBF_PURE;

			if(NOT st.is_dir)
				fib->fib_DiskKey = -1;
		}
	}

	result = DOSTRUE;

	D(("fib->fib_FileName = \"%b\"",MKBADDR(fib->fib_FileName)));
	SHOWVALUE(fib->fib_DirEntryType);
	SHOWVALUE(fib->fib_NumBlocks);
	SHOWVALUE(fib->fib_Size);
	SHOWVALUE(fib->fib_Date.ds_Days);
	SHOWVALUE(fib->fib_Date.ds_Minute);
	SHOWVALUE(fib->fib_Date.ds_Tick);
	SHOWVALUE(fib->fib_DiskKey);

 out:

	(*error_ptr) = error;

	RETURN(result);
	return(result);
}

/****************************************************************************/

STATIC BOOL
NameIsAcceptable(STRPTR name,LONG max_len)
{
	BOOL result = FALSE;
	UBYTE c;

	/* This takes care of "." and "..". */
	if(name[0] == '.' && (name[1] == '\0' || (name[1] == '.' && name[2] == '\0')))
		goto out;

	/* Now for embedded '/', ':' and '\' characters and
	 * names that just don't want to fit.
	 */
	while((c = (*name++)) != '\0')
	{
		max_len--;
		if(max_len == 0 || c == '/' || c == ':' || c == SMB_PATH_SEPARATOR)
			goto out;
	}

	result = TRUE;

 out:

	return(result);
}

/****************************************************************************/

static int
dir_scan_callback_func_exnext(
	struct FileInfoBlock *	fib,
	int						unused_fpos,
	int						nextpos,
	char *					name,
	int						eof,
	smba_stat_t *			st)
{
	int result;

	ENTER();

	D((" '%s'",name));
	D(("   is_dir=%ld is_wp=%ld is_hidden=%ld size=%ld",
		st->is_dir,st->is_wp,st->is_hidden,st->size));
	D(("   nextpos=%ld eof=%ld",nextpos,eof));

	/* Skip file and drawer names that we wouldn't be
	 * able to handle in the first place.
	 */
	if(NameIsAcceptable((STRPTR)name,sizeof(fib->fib_FileName)) && NOT (st->is_hidden && OmitHidden))
	{
		LONG seconds;

		ConvertCString(sizeof(fib->fib_FileName),fib->fib_FileName,name);
		TranslateBName(fib->fib_FileName,M2A);

		fib->fib_DirEntryType	= st->is_dir ? ST_USERDIR : ST_FILE;
		fib->fib_EntryType		= fib->fib_DirEntryType;
		fib->fib_NumBlocks		= (st->size + 511) / 512;
		fib->fib_Size			= st->size;
		fib->fib_Protection		= FIBF_OTR_READ|FIBF_OTR_EXECUTE|FIBF_OTR_WRITE|FIBF_OTR_DELETE|
								  FIBF_GRP_READ|FIBF_GRP_EXECUTE|FIBF_GRP_WRITE|FIBF_GRP_DELETE;

		if(st->is_wp)
			fib->fib_Protection ^= (FIBF_OTR_WRITE|FIBF_OTR_DELETE|FIBF_GRP_WRITE|FIBF_GRP_DELETE|FIBF_WRITE|FIBF_DELETE);

		/* Careful: the 'archive' attribute has exactly the opposite
		 *          meaning in the Amiga and the SMB worlds.
		 */
		if(NOT st->is_archive)
			fib->fib_Protection |= FIBF_ARCHIVE;

		if(st->is_system)
			fib->fib_Protection |= FIBF_PURE;

		seconds = st->mtime - UNIX_TIME_OFFSET - GetTimeZoneDelta();
		if(seconds < 0)
			seconds = 0;

		fib->fib_Date.ds_Days	= (seconds / (24 * 60 * 60));
		fib->fib_Date.ds_Minute	= (seconds % (24 * 60 * 60)) / 60;
		fib->fib_Date.ds_Tick	= (seconds % 60) * TICKS_PER_SECOND;

		result = 1;
	}
	else
	{
		result = 0;
	}

	fib->fib_DiskKey = eof ? -1 : nextpos;

	RETURN(result);
	return(result);
}

STATIC LONG
Action_ExamineNext(
	struct FileLock *		lock,
	struct FileInfoBlock *	fib,
	SIPTR *					error_ptr)
{
	struct LockNode * ln;
	LONG result = DOSFALSE;
	LONG error = OK;
	long offset;
	int count;

	ENTER();

	SHOWVALUE(lock);

	if(fib->fib_DiskKey == -1)
	{
		SHOWMSG("scanning finished.");
		error = ERROR_NO_MORE_ENTRIES;
		goto out;
	}

	if(lock == NULL)
	{
		SHOWMSG("invalid lock");
		error = ERROR_INVALID_LOCK;
		goto out;
	}

	offset = fib->fib_DiskKey;

	ln = (struct LockNode *)lock->fl_Key;

	/* Check if we should restart scanning the directory
	 * contents. This is tricky at best and may produce
	 * irritating results :(
	 */
	if(ln->ln_RestartExamine)
	{
		offset = 0;

		ln->ln_RestartExamine = FALSE;
	}

	memset(fib,0,sizeof(*fib));

	SHOWMSG("calling 'smba_readdir'");
	SHOWVALUE(offset);

	count = smba_readdir(ln->ln_File,offset,fib,(smba_callback_t)dir_scan_callback_func_exnext);

	SHOWVALUE(count);

	if(count == 0 || fib->fib_FileName[0] == '\0')
	{
		SHOWMSG("nothing to be read");
		fib->fib_DiskKey = -1;

		error = ERROR_NO_MORE_ENTRIES;
		goto out;
	}
	else if (count == (-EIO))
	{
		SHOWMSG("ouch! directory read error");
		fib->fib_DiskKey = -1;

		error = ERROR_NO_DEFAULT_DIR;
		goto out;
	}
	else if (count < 0)
	{
		SHOWMSG("error whilst scanning");
		SHOWVALUE(count);
		fib->fib_DiskKey = -1;

		error = MapErrnoToIoErr(count);
		goto out;
	}

	result = DOSTRUE;

 out:

	(*error_ptr) = error;

	RETURN(result);
	return(result);
}

/****************************************************************************/

struct ExAllContext
{
	struct ExAllData *		ec_Last;
	struct ExAllData *		ec_Next;
	ULONG					ec_BytesLeft;
	ULONG					ec_MinSize;
	struct ExAllControl * 	ec_Control;
	ULONG					ec_Type;
	LONG					ec_Error;
	BOOL					ec_FirstAttempt;
};

static int
dir_scan_callback_func_exall(
	struct ExAllContext *	ec,
	int						unused_fpos,
	int						nextpos,
	char *					name,
	int						eof,
	smba_stat_t *			st)
{
	int result = 0;

	ENTER();

	D((" '%s'",name));
	D(("   is_dir=%ld is_wp=%ld is_hidden=%ld size=%ld",
		st->is_dir,st->is_wp,st->is_hidden,st->size));
	D(("   nextpos=%ld eof=%ld",nextpos,eof));

	/* Skip file and drawer names that we wouldn't be
	 * able to handle in the first place.
	 */
	if(NameIsAcceptable((STRPTR)name,MAX_FILENAME_LEN) && NOT (st->is_hidden && OmitHidden))
	{
		struct ExAllData * ed;
		ULONG size;
		ULONG type = ec->ec_Type;
		BOOL take_it;

		size = (ec->ec_MinSize + strlen(name)+1 + 3) & ~3UL;
		SHOWVALUE(size);
		if(size > ec->ec_BytesLeft)
		{
			D(("size %ld > ec->ec_BytesLeft %ld",size,ec->ec_BytesLeft));

			/* If this is the first directory entry,
			 * stop the entire process before it has
			 * really begun.
			 */
			if(ec->ec_FirstAttempt)
			{
				SHOWMSG("this was the first read attempt.");
				ec->ec_Control->eac_Entries = 0;
				ec->ec_Error = ERROR_NO_FREE_STORE;
			}
			else
			{
				SHOWMSG("try again");
				ec->ec_Error = 0;
			}

			result = 1;
			goto out;
		}

		ed = ec->ec_Next;

		ed->ed_Next = NULL;
		ed->ed_Name = (STRPTR)(((IPTR)ed) + ec->ec_MinSize);
		strcpy(ed->ed_Name,name);

		TranslateCName(ed->ed_Name,M2A);

		if(type >= ED_TYPE)
			ed->ed_Type = st->is_dir ? ST_USERDIR : ST_FILE;

		if(type >= ED_SIZE)
			ed->ed_Size = st->size;

		if(type >= ED_PROTECTION)
		{
			ed->ed_Prot = FIBF_OTR_READ|FIBF_OTR_EXECUTE|FIBF_OTR_WRITE|FIBF_OTR_DELETE|
			              FIBF_GRP_READ|FIBF_GRP_EXECUTE|FIBF_GRP_WRITE|FIBF_GRP_DELETE;

			if(st->is_wp)
				ed->ed_Prot ^= (FIBF_OTR_WRITE|FIBF_OTR_DELETE|FIBF_GRP_WRITE|FIBF_GRP_DELETE|FIBF_WRITE|FIBF_DELETE);

			/* Careful: the 'archive' attribute has exactly the opposite
			 *          meaning in the Amiga and the SMB worlds.
			 */
			if(NOT st->is_archive)
				ed->ed_Prot |= FIBF_ARCHIVE;

			if(st->is_system)
				ed->ed_Prot |= FIBF_PURE;
		}

		if(type >= ED_DATE)
		{
			LONG seconds;

			seconds = st->mtime - UNIX_TIME_OFFSET - GetTimeZoneDelta();
			if(seconds < 0)
				seconds = 0;

			ed->ed_Days		= (seconds / (24 * 60 * 60));
			ed->ed_Mins 	= (seconds % (24 * 60 * 60)) / 60;
			ed->ed_Ticks	= (seconds % 60) * TICKS_PER_SECOND;
		}

		if(type >= ED_COMMENT)
			ed->ed_Comment = "";

		if(type >= ED_OWNER)
			ed->ed_OwnerUID = ed->ed_OwnerGID = 0;

		take_it = TRUE;

		if(ec->ec_Control->eac_MatchString != NULL)
		{
			SHOWMSG("checking against match string");
			if(NOT MatchPatternNoCase(ec->ec_Control->eac_MatchString,ed->ed_Name))
			{
				SHOWMSG("does not match");
				take_it = FALSE;
			}
		}

		if(take_it && ec->ec_Control->eac_MatchFunc != NULL)
		{
			SHOWMSG("calling match func");

			/* NOTE: the order of the parameters passed to the match hook
			 *       function can be somewhat confusing. For standard
			 *       hook functions, the order of the parameters and the
			 *       registers they go into is hook=A0, object=A2,
			 *       message=A1. However, the documentation for the 'ExAll()'
			 *       function always lists them in ascending order, that is
			 *       hook=A0, message=A1, object=A2, which can lead to
			 *       quite some confusion and strange errors.
			 */
			if(NOT CallHookPkt(ec->ec_Control->eac_MatchFunc,&type,ed))
			{
				SHOWMSG("does not match");
				take_it = FALSE;
			}
		}

		if(take_it)
		{
			SHOWMSG("registering new entry");

			if(ec->ec_Last != NULL)
				ec->ec_Last->ed_Next = ed;

			ec->ec_Last = ed;
			ec->ec_Next = (struct ExAllData *)(((IPTR)ed) + size);
			ec->ec_BytesLeft -= size;
			ec->ec_Control->eac_Entries++;

			SHOWVALUE(ec->ec_Last->ed_Next);
			SHOWVALUE(ed->ed_Name);
			SHOWVALUE(ed->ed_Comment);
		}
	}

	ec->ec_Control->eac_LastKey = (ULONG)(eof ? -1 : nextpos);

 out:

	ec->ec_FirstAttempt = FALSE;

	RETURN(result);
	return(result);
}

STATIC LONG
Action_ExamineAll(
	struct FileLock *		lock,
	struct ExAllData *		ed,
	ULONG					size,
	ULONG					type,
	struct ExAllControl *	eac,
	SIPTR *					error_ptr)
{
	struct ExAllContext ec;
	struct LockNode * ln;
	LONG result = DOSFALSE;
	LONG error = OK;
	LONG offset;
	int count;

	ENTER();

	SHOWVALUE(lock);

	SHOWVALUE(eac->eac_LastKey);

	eac->eac_Entries = 0;

	if(size < sizeof(ed->ed_Next))
	{
		SHOWMSG("buffer is far too short.");
		error = ERROR_NO_FREE_STORE;
		goto out;
	}

	ed->ed_Next = NULL;

	if(eac->eac_LastKey == (ULONG)-1)
	{
		SHOWMSG("scanning finished.");
		error = ERROR_NO_MORE_ENTRIES;
		goto out;
	}

	if(lock == NULL)
	{
		SHOWMSG("invalid lock");
		error = ERROR_INVALID_LOCK;
		goto out;
	}

	if(type < ED_NAME || type > ED_OWNER)
	{
		D(("type %ld not supported",type));
		error = ERROR_BAD_NUMBER;
		goto out;
	}

	SHOWVALUE(type);

	memset(&ec,0,sizeof(ec));

	ec.ec_Next			= ed;
	ec.ec_BytesLeft		= size;
	ec.ec_Control		= eac;
	ec.ec_Type			= type;
	ec.ec_Error			= ERROR_NO_MORE_ENTRIES;
	ec.ec_FirstAttempt	= TRUE;

	switch(type)
	{
		case ED_NAME:

			ec.ec_MinSize = offsetof(struct ExAllData,ed_Type);
			break;

		case ED_TYPE:

			ec.ec_MinSize = offsetof(struct ExAllData,ed_Size);
			break;

		case ED_SIZE:

			ec.ec_MinSize = offsetof(struct ExAllData,ed_Prot);
			break;

		case ED_PROTECTION:

			ec.ec_MinSize = offsetof(struct ExAllData,ed_Days);
			break;

		case ED_DATE:

			ec.ec_MinSize = offsetof(struct ExAllData,ed_Comment);
			break;

		case ED_COMMENT:

			ec.ec_MinSize = offsetof(struct ExAllData,ed_OwnerUID);
			break;

		case ED_OWNER:

			ec.ec_MinSize = sizeof(struct ExAllData);
			break;
	}

	SHOWVALUE(ec.ec_MinSize);

	offset = eac->eac_LastKey;

	ln = (struct LockNode *)lock->fl_Key;

	/* Check if we should restart scanning the directory
	 * contents. This is tricky at best and may produce
	 * irritating results :(
	 */
	if(ln->ln_RestartExamine)
	{
		offset = 0;

		ln->ln_RestartExamine = FALSE;
	}

	if(offset == 0)
	{
		smba_stat_t st;

		SHOWMSG("first invocation");

		SHOWMSG("getting file attributes");
		error = smba_getattr(ln->ln_File,&st);
		if(error < 0)
		{
			SHOWMSG("didn't work");
			error = MapErrnoToIoErr(error);
			eac->eac_LastKey = (ULONG)-1;
			goto out;
		}

		if(NOT st.is_dir)
		{
			SHOWMSG("lock does not refer to a directory");
			error = ERROR_OBJECT_WRONG_TYPE;
			eac->eac_LastKey = (ULONG)-1;
			goto out;
		}
	}

	SHOWMSG("calling 'smba_readdir'");
	SHOWVALUE(offset);

	count = smba_readdir(ln->ln_File,offset,&ec,(smba_callback_t)dir_scan_callback_func_exall);

	SHOWVALUE(count);

	if(count == 0 || eac->eac_Entries == 0)
	{
		SHOWMSG("nothing to be read");
		if(ec.ec_Error != OK)
		{
			SHOWMSG("flagging an error");
			SHOWVALUE(ec.ec_Error);
			eac->eac_LastKey = (ULONG)-1;
			error = ec.ec_Error;
		}

		goto out;
	}
	else if (count == (-EIO))
	{
		SHOWMSG("ouch! directory read error");
		eac->eac_LastKey = (ULONG)-1;

		error = ERROR_NO_DEFAULT_DIR;
		goto out;
	}
	else if (count < 0)
	{
		SHOWMSG("error whilst scanning");
		eac->eac_LastKey = (ULONG)-1;

		error = MapErrnoToIoErr(count);
		goto out;
	}

	SHOWMSG("ok");
	result = DOSTRUE;

 out:

	#if DEBUG
	{
		SHOWVALUE(eac->eac_Entries);

		while(ed != NULL)
		{
			SHOWSTRING(ed->ed_Name);

			ed = ed->ed_Next;
		}
	}
	#endif /* DEBUG */

	(*error_ptr) = error;

	RETURN(result);
	return(result);
}

/****************************************************************************/

STATIC LONG
Action_Find(
	LONG					action,
	struct FileHandle *		fh,
	struct FileLock *		parent,
	APTR					bcpl_name,
	SIPTR *					error_ptr)
{
	LONG result = DOSFALSE;
	STRPTR parent_path = NULL;
	STRPTR full_name = NULL;
	LONG full_name_size;
	struct FileNode * fn = NULL;
	STRPTR parent_name;
	UBYTE name[MAX_FILENAME_LEN];
	BOOL create_new_file;
	LONG error;

	ENTER();

	switch(action)
	{
		case ACTION_FINDINPUT:
			D(("ACTION_FINDINPUT [Open(\"%b\",MODE_OLDFILE)]",MKBADDR(bcpl_name)));
			break;

		case ACTION_FINDOUTPUT:
			D(("ACTION_FINDOUTPUT [Open(\"%b\",MODE_NEWFILE)]",MKBADDR(bcpl_name)));
			break;

		case ACTION_FINDUPDATE:
			D(("ACTION_FINDUPDATE [Open(\"%b\",MODE_READWRITE)]",MKBADDR(bcpl_name)));
			break;
	}

	SHOWVALUE(parent);

	if(parent != NULL)
	{
		struct LockNode * ln = (struct LockNode *)parent->fl_Key;

		parent_name = ln->ln_FullName;
	}
	else
	{
		parent_name = NULL;
	}

	ConvertBString(sizeof(name),name,bcpl_name);
	TranslateCName(name,A2M);

	if(IsReservedName(FilePart(name)))
	{
		error = ERROR_OBJECT_NOT_FOUND;
		goto out;
	}

	error = BuildFullName(parent_name,name,&full_name,&full_name_size);
	if(error != OK)
		goto out;

	/* Trying to open the root directory? */
	if(full_name == NULL)
	{
		error = ERROR_OBJECT_WRONG_TYPE;
		goto out;
	}

	fn = AllocateMemory(sizeof(*fn));
	if(fn == NULL)
	{
		error = ERROR_NO_FREE_STORE;
		goto out;
	}

	memset(fn,0,sizeof(*fn));

	fn->fn_Handle	= fh;
	fn->fn_FullName	= full_name;
	fn->fn_Mode		= (action == ACTION_FINDOUTPUT) ? EXCLUSIVE_LOCK : SHARED_LOCK;

	error = CheckAccessModeCollision(full_name,fn->fn_Mode);
	if(error != OK)
		goto out;

	SHOWSTRING(full_name);

	if(action == ACTION_FINDOUTPUT)
	{
		/* Definitely create a new file. */
		create_new_file = TRUE;
	}
	else if (action == ACTION_FINDINPUT)
	{
		/* Open an existing file for reading. */
		create_new_file = FALSE;
	}
	else if (action == ACTION_FINDUPDATE)
	{
		smba_file_t * file = NULL;
		smba_stat_t st;

		if(smba_open(ServerData,full_name,full_name_size,&file) == OK &&
		   smba_getattr(file,&st) == OK)
		{
			/* File apparently opens Ok and information on it
			 * is available, don't try to replace it.
			 */
			create_new_file = FALSE;
		}
		else
		{
			/* We try to ignore the error here and assume
			 * that the remainder of the file opening
			 * procedure will produce a useful error
			 * report. In the mean time, assume that the
			 * file needs to be created.
			 */
			create_new_file = TRUE;
		}

		if(file != NULL)
			smba_close(file);
	}
	else
	{
		/* What's that? */
		error = ERROR_ACTION_NOT_KNOWN;
		goto out;
	}

	/* Create a new file? */
	if(create_new_file)
	{
		smba_stat_t st;
		smba_file_t * dir;
		STRPTR base_name;
		LONG i;

		if(WriteProtected)
		{
			error = ERROR_DISK_WRITE_PROTECTED;
			goto out;
		}

		parent_path = AllocateMemory(strlen(full_name)+3);
		if(parent_path == NULL)
		{
			error = ERROR_NO_FREE_STORE;
			goto out;
		}

		strcpy(parent_path,full_name);
		base_name = NULL;
		for(i = strlen(parent_path)-1 ; i >= 0 ; i--)
		{
			if(parent_path[i] == SMB_PATH_SEPARATOR)
			{
				if(i == 0)
				{
					memmove(&parent_path[1],&parent_path[0],strlen(parent_path)+1);
					i++;
				}

				parent_path[i] = '\0';

				base_name = &parent_path[i+1];
				break;
			}
		}

		SHOWMSG("creating a file; finding parent path first");
		SHOWSTRING(parent_path);

		error = smba_open(ServerData,parent_path,strlen(full_name)+3,&dir);
		if(error < 0)
		{
			error = MapErrnoToIoErr(error);
			goto out;
		}

		/* Only one attribute counts: the file should not be write protected. */
		memset(&st,0,sizeof(st));

		SHOWMSG("now trying to create the file");
		SHOWSTRING(base_name);

		error = smba_create(dir,base_name,&st);
		if(error < 0)
		{
			SHOWMSG("didn't work.");
			SHOWVALUE(error);

			smba_close(dir);
			error = MapErrnoToIoErr(error);

			SHOWVALUE(error);

			goto out;
		}

		SHOWMSG("good.");

		smba_close(dir);
	}

	/* Now for the remainder... */
	error = smba_open(ServerData,full_name,full_name_size,&fn->fn_File);
	if(error < 0)
	{
		error = MapErrnoToIoErr(error);
		goto out;
	}

	fh->fh_Arg1 = (IPTR)fn;

	AddTail((struct List *)&FileList,(struct Node *)fn);
	result = DOSTRUE;

 out:

	if(result == DOSFALSE)
	{
		FreeMemory(full_name);
		FreeMemory(fn);
	}

	FreeMemory(parent_path);

	(*error_ptr) = error;

	RETURN(result);
	return(result);
}

/****************************************************************************/

STATIC LONG
Action_Read(
	struct FileNode *	fn,
	APTR				mem,
	LONG				length,
	SIPTR *				error_ptr)
{
	LONG result = 0;
	LONG error = OK;

	ENTER();

	if(length > 0)
	{
		result = smba_read(fn->fn_File,mem,length,fn->fn_Offset);
		if(result < 0)
		{
			error = MapErrnoToIoErr(result);
			result = -1;
			goto out;
		}

		fn->fn_Offset += result;
	}

 out:

	(*error_ptr) = error;

	RETURN(result);
	return(result);
}

/****************************************************************************/

STATIC LONG
Action_Write(
	struct FileNode *	fn,
	APTR				mem,
	LONG				length,
	SIPTR *				error_ptr)
{
	LONG result = DOSFALSE;
	LONG error = OK;

	ENTER();

	if(WriteProtected)
	{
		error = ERROR_DISK_WRITE_PROTECTED;
		goto out;
	}

	if(length > 0)
	{
		result = smba_write(fn->fn_File,mem,length,fn->fn_Offset);
		if(result < 0)
		{
			error = MapErrnoToIoErr(result);
			result = -1;
			goto out;
		}

		fn->fn_Offset += result;
	}

 out:

	(*error_ptr) = error;

	RETURN(result);
	return(result);
}

/****************************************************************************/

STATIC LONG
Action_End(
	struct FileNode *	fn,
	SIPTR *				error_ptr)
{
	Remove((struct Node *)fn);

	smba_close(fn->fn_File);
	FreeMemory(fn->fn_FullName);
	FreeMemory(fn);

	(*error_ptr) = OK;
	return(DOSTRUE);
}

/****************************************************************************/

STATIC LONG
Action_Seek(
	struct FileNode *	fn,
	LONG				position,
	LONG				mode,
	SIPTR *				error_ptr)
{
	LONG previous_position = fn->fn_Offset;
	LONG result = -1;
	LONG offset;
	LONG error;

	ENTER();

	/* olsen: This doesn't really work with Microsoft SMB servers, but it works with Samba. */
	#if 0
	{
		switch(mode)
		{
			case OFFSET_BEGINNING:
	
				mode = 0;
				break;
	
			case OFFSET_CURRENT:
	
				mode = 1;
				break;
	
			case OFFSET_END:
	
				mode = 2;
				break;
	
			default:
	
				error = ERROR_ACTION_NOT_KNOWN;
				goto out;
		}

		error = smba_seek (fn->fn_File, position, mode, (off_t *) &offset);
		if(error < 0)
		{
			error = MapErrnoToIoErr(error);
			goto out;
		}
	}
	#endif

	/* olsen: This is the original implementation. */
	#if 0
	{
		smba_stat_t st;

		error = smba_getattr(fn->fn_File,&st);
		if(error < 0)
		{
			error = MapErrnoToIoErr(error);
			goto out;
		}

		offset = fn->fn_Offset;

		switch(mode)
		{
			case OFFSET_BEGINNING:

				offset = position;
				break;

			case OFFSET_CURRENT:

				offset += position;
				break;

			case OFFSET_END:

				offset = st.size + position;
				break;

			default:

				error = ERROR_ACTION_NOT_KNOWN;
				goto out;
		}

		if(offset < 0 || offset > st.size)
		{
			error = ERROR_SEEK_ERROR;
			goto out;
		}
	}
	#endif

	/* olsen: This is a mix of the two above. First we calculate the absolute
	 *        position, then seek to that position. The SMB server is supposed
	 *        to do its housekeeping before the position is changed. I wish this
	 *        worked differently, but it seems we've got the best of both worlds
	 *        here...
	 */
	#if 1
	{
		smba_stat_t st;

		switch(mode)
		{
			case OFFSET_BEGINNING:

				offset = position;
				break;

			case OFFSET_CURRENT:

				offset = fn->fn_Offset + position;
				break;

			case OFFSET_END:

				error = smba_getattr(fn->fn_File,&st);
				if(error < 0)
				{
					error = MapErrnoToIoErr(error);
					goto out;
				}

				offset = st.size + position;
				break;

			default:

				error = ERROR_ACTION_NOT_KNOWN;
				goto out;
		}

		if(offset < 0)
		{
			error = ERROR_SEEK_ERROR;
			goto out;
		}

		error = smba_seek (fn->fn_File, offset, 0, (off_t *) &offset);
		if(error < 0)
		{
			error = MapErrnoToIoErr(error);
			goto out;
		}
	}
	#endif

	error = OK;

	fn->fn_Offset = offset;

	result = previous_position;

 out:

	(*error_ptr) = error;

	RETURN(result);
	return(result);
}

/****************************************************************************/

STATIC LONG
Action_SetFileSize(
	struct FileNode *	fn,
	LONG				position,
	LONG				mode,
	SIPTR *				error_ptr)
{
	smba_stat_t st;
	LONG result = -1;
	LONG error;
	long offset;

	ENTER();

	if(WriteProtected)
	{
		error = ERROR_DISK_WRITE_PROTECTED;
		goto out;
	}

	error = smba_getattr(fn->fn_File,&st);
	if(error < 0)
	{
		error = MapErrnoToIoErr(error);
		goto out;
	}

	offset = fn->fn_Offset;

	switch(mode)
	{
		case OFFSET_BEGINNING:

			offset = position;
			break;

		case OFFSET_CURRENT:

			offset += position;
			break;

		case OFFSET_END:

			offset = st.size + position;
			break;

		default:

			error = ERROR_ACTION_NOT_KNOWN;
			goto out;
	}

	if(offset < 0)
	{
		error = ERROR_SEEK_ERROR;
		goto out;
	}

	st.atime	= -1;
	st.ctime	= -1;
	st.mtime	= -1;
	st.size		= offset;

	error = smba_setattr(fn->fn_File,&st);
	if(error < 0)
	{
		error = MapErrnoToIoErr(error);
		goto out;
	}

	if(fn->fn_Offset > offset)
		fn->fn_Offset = offset;

	result = offset;

 out:

	(*error_ptr) = error;

	RETURN(result);
	return(result);
}

/****************************************************************************/

STATIC LONG
Action_SetDate(
	struct FileLock *	parent,
	APTR				bcpl_name,
	struct DateStamp *	ds,
	SIPTR *				error_ptr)
{
	LONG result = DOSFALSE;
	STRPTR full_name = NULL;
	LONG full_name_size;
	smba_file_t * file = NULL;
	STRPTR parent_name;
	UBYTE name[MAX_FILENAME_LEN];
	smba_stat_t st;
	LONG seconds;
	LONG error;

	ENTER();

	if(WriteProtected)
	{
		error = ERROR_DISK_WRITE_PROTECTED;
		goto out;
	}

	SHOWVALUE(parent);

	if(parent != NULL)
	{
		struct LockNode * ln = (struct LockNode *)parent->fl_Key;

		parent_name = ln->ln_FullName;
	}
	else
	{
		parent_name = NULL;
	}

	ConvertBString(sizeof(name),name,bcpl_name);
	TranslateCName(name,A2M);

	error = BuildFullName(parent_name,name,&full_name,&full_name_size);
	if(error != OK)
		goto out;

	/* Trying to change the date of the root directory? */
	if(full_name == NULL)
	{
		error = ERROR_OBJECT_IN_USE;
		goto out;
	}

	SHOWSTRING(full_name);

	error = smba_open(ServerData,full_name,full_name_size,&file);
	if(error < 0)
	{
		error = MapErrnoToIoErr(error);
		goto out;
	}

	error = smba_getattr(file,&st);
	if(error < 0)
	{
		error = MapErrnoToIoErr(error);
		goto out;
	}

	seconds = (ds->ds_Days * 24 * 60 + ds->ds_Minute) * 60 + (ds->ds_Tick / TICKS_PER_SECOND);

	st.atime = -1;
	st.ctime = -1;
	st.mtime = seconds + UNIX_TIME_OFFSET + GetTimeZoneDelta();
	st.size = -1;

	error = smba_setattr(file,&st);
	if(error < 0)
	{
		error = MapErrnoToIoErr(error);
		goto out;
	}

	result = DOSTRUE;

 out:

	FreeMemory(full_name);
	if(file != NULL)
		smba_close(file);

	(*error_ptr) = error;

	RETURN(result);
	return(result);
}

/****************************************************************************/

STATIC LONG
Action_ExamineFH(
	struct FileNode *		fn,
	struct FileInfoBlock *	fib,
	SIPTR *					error_ptr)
{
	LONG result = DOSFALSE;
	smba_stat_t st;
	LONG error;
	LONG seconds;
	STRPTR name;
	LONG i;

	ENTER();

	error = smba_getattr(fn->fn_File,&st);
	if(error < 0)
	{
		error = MapErrnoToIoErr(error);
		goto out;
	}

	name = fn->fn_FullName;
	for(i = strlen(name)-1 ; i >= 0 ; i--)
	{
		if(name[i] == SMB_PATH_SEPARATOR)
		{
			name = &name[i+1];
			break;
		}
	}

	/* Just checking: will the name fit? */
	if(strlen(name) >= sizeof(fib->fib_FileName))
	{
		error = ERROR_INVALID_COMPONENT_NAME;
		goto out;
	}

	memset(fib,0,sizeof(*fib));

	ConvertCString(sizeof(fib->fib_FileName),fib->fib_FileName,name);
	TranslateBName(fib->fib_FileName,M2A);

	fib->fib_DirEntryType	= ST_FILE;
	fib->fib_EntryType		= ST_FILE;
	fib->fib_NumBlocks		= (st.size + 511) / 512;
	fib->fib_Size			= st.size;
	fib->fib_DiskKey		= -1;

	fib->fib_Protection		= FIBF_OTR_READ|FIBF_OTR_EXECUTE|FIBF_OTR_WRITE|FIBF_OTR_DELETE|
							  FIBF_GRP_READ|FIBF_GRP_EXECUTE|FIBF_GRP_WRITE|FIBF_GRP_DELETE;

	if(st.is_wp)
		fib->fib_Protection ^= (FIBF_OTR_WRITE|FIBF_OTR_DELETE|FIBF_GRP_WRITE|FIBF_GRP_DELETE|FIBF_WRITE|FIBF_DELETE);

	/* Careful: the 'archive' attribute has exactly the opposite
	 *          meaning in the Amiga and the SMB worlds.
	 */
	if(NOT st.is_archive)
		fib->fib_Protection |= FIBF_ARCHIVE;

	if(st.is_system)
		fib->fib_Protection |= FIBF_PURE;

	seconds = st.mtime - UNIX_TIME_OFFSET - GetTimeZoneDelta();
	if(seconds < 0)
		seconds = 0;

	fib->fib_Date.ds_Days	= (seconds / (24 * 60 * 60));
	fib->fib_Date.ds_Minute	= (seconds % (24 * 60 * 60)) / 60;
	fib->fib_Date.ds_Tick	= (seconds % 60) * TICKS_PER_SECOND;

	result = DOSTRUE;

 out:

	(*error_ptr) = error;

	RETURN(result);
	return(result);
}

/****************************************************************************/

STATIC BPTR
Action_ParentFH(
	struct FileNode *	fn,
	SIPTR *				error_ptr)
{
	BPTR result = ZERO;
	struct LockNode * ln = NULL;
	LONG error;
	STRPTR full_name;
	LONG full_name_size;
	LONG i;

	ENTER();

	full_name_size = strlen(fn->fn_FullName)+3;
	if(full_name_size < SMB_MAXNAMELEN+1)
		full_name_size = SMB_MAXNAMELEN+1;

	full_name = AllocateMemory(full_name_size);
	if(full_name == NULL)
	{
		error = ERROR_NO_FREE_STORE;
		goto out;
	}

	strcpy(full_name,fn->fn_FullName);

	for(i = strlen(full_name)-1 ; i >= 0 ; i--)
	{
		if(i == 0)
		{
			strcpy(full_name,SMB_ROOT_DIR_NAME);
			break;
		}
		else if (full_name[i] == SMB_PATH_SEPARATOR)
		{
			full_name[i] = '\0';
			break;
		}
	}

	ln = AllocateMemory(sizeof(*ln));
	if(ln == NULL)
	{
		error = ERROR_NO_FREE_STORE;
		goto out;
	}

	memset(ln,0,sizeof(*ln));

	ln->ln_FileLock.fl_Key		= (IPTR)ln;
	ln->ln_FileLock.fl_Access	= SHARED_LOCK;
	ln->ln_FileLock.fl_Task		= FileSystemPort;
	ln->ln_FileLock.fl_Volume	= MKBADDR(VolumeNode);
	ln->ln_FullName				= full_name;

	SHOWSTRING(full_name);

	error = smba_open(ServerData,full_name,full_name_size,&ln->ln_File);
	if(error < 0)
	{
		error = MapErrnoToIoErr(error);
		goto out;
	}

	AddTail((struct List *)&LockList,(struct Node *)ln);
	result = MKBADDR(&ln->ln_FileLock);
	SHOWVALUE(&ln->ln_FileLock);

 out:

	if(result == ZERO)
	{
		FreeMemory(ln);
		FreeMemory(full_name);
	}

	(*error_ptr) = error;

	RETURN(result);
	return(result);
}

/****************************************************************************/

STATIC BPTR
Action_CopyDirFH(
	struct FileNode *	fn,
	SIPTR *				error_ptr)
{
	BPTR result = ZERO;
	struct LockNode * ln = NULL;
	STRPTR full_name = NULL;
	LONG full_name_size;
	LONG error;

	ENTER();

	if(fn->fn_Mode != SHARED_LOCK)
	{
		error = ERROR_OBJECT_IN_USE;
		goto out;
	}

	full_name_size = strlen(fn->fn_FullName)+3;
	if(full_name_size < SMB_MAXNAMELEN+1)
		full_name_size = SMB_MAXNAMELEN+1;

	full_name = AllocateMemory(full_name_size);
	if(full_name == NULL)
	{
		error = ERROR_NO_FREE_STORE;
		goto out;
	}

	strcpy(full_name,fn->fn_FullName);

	ln = AllocateMemory(sizeof(*ln));
	if(ln == NULL)
	{
		error = ERROR_NO_FREE_STORE;
		goto out;
	}

	memset(ln,0,sizeof(*ln));

	ln->ln_FileLock.fl_Key		= (IPTR)ln;
	ln->ln_FileLock.fl_Access	= SHARED_LOCK;
	ln->ln_FileLock.fl_Task		= FileSystemPort;
	ln->ln_FileLock.fl_Volume	= MKBADDR(VolumeNode);
	ln->ln_FullName				= full_name;

	SHOWSTRING(full_name);

	error = smba_open(ServerData,full_name,full_name_size,&ln->ln_File);
	if(error < 0)
	{
		error = MapErrnoToIoErr(error);
		goto out;
	}

	AddTail((struct List *)&LockList,(struct Node *)ln);
	result = MKBADDR(&ln->ln_FileLock);
	SHOWVALUE(&ln->ln_FileLock);

 out:

	if(result == ZERO)
	{
		FreeMemory(ln);
		FreeMemory(full_name);
	}

	(*error_ptr) = error;

	RETURN(result);
	return(result);
}

/****************************************************************************/

STATIC LONG
Action_FHFromLock(
	struct FileHandle *	fh,
	struct FileLock *	fl,
	SIPTR *				error_ptr)
{
	LONG result = DOSFALSE;
	struct FileNode * fn;
	struct LockNode * ln;
	LONG error = OK;

	ENTER();

	SHOWVALUE(fl);

	fn = AllocateMemory(sizeof(*fn));
	if(fn == NULL)
	{
		error = ERROR_NO_FREE_STORE;
		goto out;
	}

	memset(fn,0,sizeof(*fn));

	ln = (struct LockNode *)fl->fl_Key;

	fn->fn_Handle	= fh;
	fn->fn_FullName	= ln->ln_FullName;
	fn->fn_File		= ln->ln_File;
	fn->fn_Mode		= fl->fl_Access;

	Remove((struct Node *)ln);
	FreeMemory(ln);

	fh->fh_Arg1 = (IPTR)fn;

	AddTail((struct List *)&FileList,(struct Node *)fn);
	result = DOSTRUE;

 out:

	(*error_ptr) = error;

	RETURN(result);
	return(result);
}

/****************************************************************************/

STATIC LONG
Action_RenameDisk(
	APTR	bcpl_name,
	SIPTR *	error_ptr)
{
	LONG result = DOSFALSE;
	LONG error = OK;
	STRPTR old_name;
	STRPTR new_name;
	UBYTE * name;
	LONG len;

	ENTER();

	if(NOT VolumeNodeAdded)
	{
		error = ERROR_OBJECT_IN_USE;
		goto out;
	}

	if(WriteProtected)
	{
		error = ERROR_DISK_WRITE_PROTECTED;
		goto out;
	}

	/* Now for the really interesting part; the new name
	 * is to be a NUL-terminated BCPL string, and as such
	 * must be allocated via AllocVec().
	 */

	name = bcpl_name;

	len = name[0];

	new_name = AllocVec(1 + len + 1,MEMF_ANY|MEMF_PUBLIC);
	if(new_name == NULL)
	{
		error = ERROR_NO_FREE_STORE;
		goto out;
	}

	new_name[0] = len;
	memcpy(&new_name[1],&name[1],len);
	new_name[len+1] = '\0';

	Forbid();

	old_name = BADDR(VolumeNode->dol_Name);
	VolumeNode->dol_Name = MKBADDR(new_name);

	Permit();

	FreeVec(old_name);

	SendDiskChange(IECLASS_DISKINSERTED);

	result = DOSTRUE;

 out:

	(*error_ptr) = error;

	RETURN(result);
	return(result);
}

/****************************************************************************/

STATIC LONG
Action_ChangeMode(
	LONG				type,
	APTR				object,
	LONG				new_mode,
	SIPTR *				error_ptr)
{
	LONG result = DOSFALSE;
	struct FileLock * fl = NULL;
	struct FileNode * fn = NULL;
	struct LockNode * ln = NULL;
	STRPTR name;
	LONG old_mode;
	LONG error = OK;

	ENTER();

	/* Sanity check; verify parameters */
	if((type != CHANGE_LOCK && type != CHANGE_FH) ||
	   (new_mode != EXCLUSIVE_LOCK && new_mode != SHARED_LOCK))
	{
		error = ERROR_ACTION_NOT_KNOWN;
		goto out;
	}

	/* Now obtain the data structures, name and mode
	 * associated with the object in question.
	 */
	if(type == CHANGE_LOCK)
	{
		fl = object;
		ln = (struct LockNode *)fl->fl_Key;
		name = ln->ln_FullName;
		old_mode = fl->fl_Access;
	}
	else
	{
		struct FileHandle * fh = object;

		fn = (struct FileNode *)fh->fh_Arg1;
		name = fn->fn_FullName;
		old_mode = fn->fn_Mode;
	}

	/* Do we need to change anything at all? */
	if(new_mode == old_mode)
	{
		result = DOSTRUE;
		goto out;
	}

	/* This is the easiest case; change an
	 * exclusive access mode to a shared
	 * access mode. Since the original mode
	 * can be used by one object only,
	 * we get away by updating the mode
	 * value.
	 */
	if(new_mode == SHARED_LOCK)
	{
		if(type == CHANGE_LOCK)
			fl->fl_Access = new_mode;
		else
			fn->fn_Mode = new_mode;

		result = DOSTRUE;
		goto out;
	}

	/* Is there another shared access lock
	 * which refers to the same object?
	 */
	if(FindLockNode(name,ln) != NULL)
	{
		error = ERROR_OBJECT_IN_USE;
		goto out;
	}

	/* Is there another shared access file
	 * which refers to the same object?
	 */
	if(FindFileNode(name,fn) != NULL)
	{
		error = ERROR_OBJECT_IN_USE;
		goto out;
	}

	/* There is just one single reference
	 * to this object; change the mode
	 * and quit.
	 */
	if(type == CHANGE_LOCK)
		fl->fl_Access = new_mode;
	else
		fn->fn_Mode = new_mode;

	result = DOSTRUE;

 out:

	(*error_ptr) = error;

	RETURN(result);
	return(result);
}

/****************************************************************************/

STATIC LONG
Action_WriteProtect(
	LONG	flag,
	ULONG	key,
	SIPTR *	error_ptr)
{
	LONG result = DOSFALSE;
	LONG error = OK;

	ENTER();

	if(flag == DOSFALSE)
	{
		if(WriteProtected)
		{
			if(key != WriteProtectKey)
			{
				error = ERROR_INVALID_LOCK;
				goto out;
			}

			WriteProtected = FALSE;

			if(VolumeNodeAdded)
			{
				SendDiskChange(IECLASS_DISKREMOVED);
				SendDiskChange(IECLASS_DISKINSERTED);
			}
		}
	}
	else
	{
		if(NOT WriteProtected)
		{
			WriteProtected = TRUE;
			WriteProtectKey = key;

			if(VolumeNodeAdded)
			{
				SendDiskChange(IECLASS_DISKREMOVED);
				SendDiskChange(IECLASS_DISKINSERTED);
			}
		}
		else
		{
			error = ERROR_INVALID_LOCK;
			goto out;
		}
	}

	result = DOSTRUE;

 out:

	(*error_ptr) = error;

	RETURN(result);
	return(result);
}

/****************************************************************************/

STATIC LONG
Action_MoreCache(
	LONG	buffer_delta,
	SIPTR *	error_ptr)
{
	LONG result;
	int old_size;

	ENTER();

	old_size = smba_get_dircache_size(ServerData);

	result = smba_change_dircache_size(ServerData,old_size + buffer_delta);

	if(result == old_size && buffer_delta != 0)
	{
		result = DOSFALSE;
		(*error_ptr) = ERROR_NO_FREE_STORE;
	}

	RETURN(result);
	return(result);
}

/****************************************************************************/

STATIC LONG
Action_SetComment(
	struct FileLock *	parent,
	APTR				bcpl_name,
	APTR				bcpl_comment,
	SIPTR *				error_ptr)
{
	LONG result = DOSFALSE;
	STRPTR full_name = NULL;
	LONG full_name_size;
	smba_file_t * file = NULL;
	STRPTR parent_name;
	UBYTE name[MAX_FILENAME_LEN];
	UBYTE comment[80];
	LONG error;

	ENTER();

	if(WriteProtected)
	{
		error = ERROR_DISK_WRITE_PROTECTED;
		goto out;
	}

	SHOWVALUE(parent);

	if(parent != NULL)
	{
		struct LockNode * ln = (struct LockNode *)parent->fl_Key;

		parent_name = ln->ln_FullName;
	}
	else
	{
		parent_name = NULL;
	}

	ConvertBString(sizeof(name),name,bcpl_name);
	TranslateCName(name,A2M);

	error = BuildFullName(parent_name,name,&full_name,&full_name_size);
	if(error != OK)
		goto out;

	/* Trying to change the comment of the root directory? */
	if(full_name == NULL)
	{
		error = ERROR_OBJECT_IN_USE;
		goto out;
	}

	SHOWSTRING(full_name);

	error = smba_open(ServerData,full_name,full_name_size,&file);
	if(error < 0)
	{
		error = MapErrnoToIoErr(error);
		goto out;
	}

	ConvertBString(sizeof(comment),comment,bcpl_comment);

	SHOWSTRING(comment);

	/* All this work and we're only doing something very silly... */
	if(strlen(comment) > 0)
	{
		error = ERROR_COMMENT_TOO_BIG;
		goto out;
	}

	result = DOSTRUE;

 out:

	FreeMemory(full_name);
	if(file != NULL)
		smba_close(file);

	(*error_ptr) = error;

	RETURN(result);
	return(result);
}

/****************************************************************************/

STATIC LONG
Action_LockRecord (
	struct FileNode *	fn,
	LONG				offset,
	LONG				length,
	LONG				mode,
	ULONG				timeout,
	SIPTR *				error_ptr)
{
	LONG result = DOSFALSE;
	LONG error;
	LONG umode;

	/* Sanity checks... */
	if (mode < REC_EXCLUSIVE || mode > REC_SHARED_IMMED)
	{
		error = ERROR_ACTION_NOT_KNOWN;
		goto out;
	}

	/* Invalid offset, size or integer overflow? */
	if (offset < 0 || length <= 0 || offset + length < offset)
	{
		error = ERROR_LOCK_COLLISION;
		goto out;
	}

	if ((mode == REC_SHARED) || (mode == REC_SHARED_IMMED))
		umode = 1;
	else
		umode = 0;

	if ((mode == REC_SHARED_IMMED) || (mode == REC_EXCLUSIVE_IMMED))
		timeout = 0;

	if (timeout > 0)
	{
		if (timeout > 214748364)
			timeout = ~0;	/* wait forever */
		else
			timeout *= 20;	/* milliseconds instead of Ticks */
	}

	error = smba_lockrec (fn->fn_File, offset, length, umode, 0, (long)timeout);
	if(error < 0)
	{
		error = MapErrnoToIoErr(error);
		goto out;
	}

	result = DOSTRUE;

 out:

	(*error_ptr) = error;

	RETURN(result);
	return(result);
}

/****************************************************************************/

STATIC LONG
Action_FreeRecord (
	struct FileNode *	fn,
	LONG				offset,
	LONG				length,
	SIPTR *				error_ptr)
{
	LONG result = DOSFALSE;
	LONG error;

	/* Sanity checks... */
	if(offset < 0 || length <= 0 || offset + length < offset)
	{
		error = ERROR_RECORD_NOT_LOCKED;
		goto out;
	}

	error = smba_lockrec (fn->fn_File, offset, length, 2, -1, 0);
	if (error < 0)
	{
		error = MapErrnoToIoErr(error);
		goto out;
	}

	result = DOSTRUE;

 out:

	(*error_ptr) = error;

	RETURN(result);
	return(result);
}

/****************************************************************************/

STATIC VOID
HandleFileSystem(STRPTR device_name,STRPTR volume_name,STRPTR service_name)
{
	BOOL sign_off = FALSE;
	ULONG signals;
	BOOL done;

	ENTER();

	DisplayErrorList();

	if(NOT Quiet && WBStartup == NULL)
	{
		struct CommandLineInterface * cli;

		cli = Cli();
		if(NOT cli->cli_Background)
		{
			struct Process * this_process;
			UBYTE name[MAX_FILENAME_LEN];
			LONG max_cli;
			LONG which;
			LONG i;

			this_process = (struct Process *)FindTask(NULL);

			Forbid();

			which = max_cli = MaxCli();

			for(i = 1 ; i <= max_cli ; i++)
			{
				if(FindCliProc(i) == this_process)
				{
					which = i;
					break;
				}
			}

			Permit();

			if(volume_name == NULL)
				strlcpy(name,device_name,sizeof(name));
			else
				strlcpy(name,volume_name,sizeof(name));

			for(i = strlen(name)-1 ; i >= 0 ; i--)
			{
				if(name[i] == ':')
					name[i] = '\0';
				else
					break;
			}

			LocalPrintf("Connected '%s' to '%s:'; \"Break %ld\" or [Ctrl-C] to stop... ",
			service_name,name,which);

			Flush(Output());

			sign_off = TRUE;
		}
	}

	Quiet = TRUE;

	done = FALSE;

	do
	{
		signals = Wait(SIGBREAKF_CTRL_C | SIGBREAKF_CTRL_F | (1UL << FileSystemPort->mp_SigBit));

		if(signals & (1UL << FileSystemPort->mp_SigBit))
		{
			struct DosPacket * dp;
			struct Message * mn;
			IPTR res1,res2;

			while((mn = GetMsg(FileSystemPort)) != NULL)
			{
				dp = (struct DosPacket *)mn->mn_Node.ln_Name;

				D(("got packet; sender '%s'",((struct Node *)dp->dp_Port->mp_SigTask)->ln_Name));

				res2 = 0;

				switch(dp->dp_Action)
				{
					case ACTION_DIE:

						SHOWMSG("ACTION_DIE");
						if(IsListEmpty((struct List *)&FileList) && IsListEmpty((struct List *)&LockList))
						{
							SHOWMSG("no locks or files pending; quitting");

							res1 = DOSTRUE;
						}
						else
						{
							SHOWMSG("locks or files still pending; cannot quit yet");

							res1 = DOSFALSE;
							res2 = ERROR_OBJECT_IN_USE;
						}

						Quit = TRUE;
						break;

					case ACTION_CURRENT_VOLUME:
						/* (Ignore) -> VolumeNode */

						res1 = (IPTR)MKBADDR(VolumeNode);
						break;

					case ACTION_LOCATE_OBJECT:
						/* Lock,Name,Mode -> Lock */

						res1 = (IPTR)Action_LocateObject((struct FileLock *)BADDR(dp->dp_Arg1),(APTR)BADDR(dp->dp_Arg2),dp->dp_Arg3,&res2);
						break;

					case ACTION_RENAME_DISK:
						/* Name -> Bool */

						res1 = Action_RenameDisk((UBYTE *)BADDR(dp->dp_Arg1),&res2);
						break;

					case ACTION_FREE_LOCK:
						/* Lock -> Bool */

						res1 = Action_FreeLock((struct FileLock *)BADDR(dp->dp_Arg1),&res2);
						break;

					case ACTION_DELETE_OBJECT:
						/* Lock,Name -> Bool */

						res1 = Action_DeleteObject((struct FileLock *)BADDR(dp->dp_Arg1),(APTR)BADDR(dp->dp_Arg2),&res2);
						break;

					case ACTION_RENAME_OBJECT:
						/* Source lock,source name,destination lock,destination name -> Bool */

						res1 = Action_RenameObject((struct FileLock *)BADDR(dp->dp_Arg1),BADDR(dp->dp_Arg2),
							(struct FileLock *)BADDR(dp->dp_Arg3),BADDR(dp->dp_Arg4),&res2);

						break;

					case ACTION_MORE_CACHE:
						/* Buffer delta -> Total number of buffers */

						/* NOTE: documentation for this packet type is inconsistent;
						 *       in the 'good old' 1.x days 'res1' was documented as
						 *       the total number of buffers to be returned. In the
						 *       2.x documentation it is said that 'res1' should
						 *       return the success code, with 'res2' to hold the
						 *       total number of buffers. However, the 'AddBuffers'
						 *       shell command doesn't work that way, and the
						 *       dos.library implementation of 'AddBuffers()' doesn't
						 *       work that way either. The 1.3 'AddBuffers' command
						 *       appears to treat a zero result as failure and a
						 *       non-zero result as success, which suggests that this
						 *       is how the packet is supposed to work, contrary to
						 *       what the official documentation says.
						 */
						res1 = Action_MoreCache(dp->dp_Arg1,&res2);
						break;

					case ACTION_COPY_DIR:
						/* Lock -> Lock */

						res1 = (IPTR)Action_CopyDir((struct FileLock *)BADDR(dp->dp_Arg1),&res2);
						break;

					case ACTION_SET_PROTECT:
						/* (Ignore),Lock,Name,Mask -> Bool */

						res1 = Action_SetProtect((struct FileLock *)BADDR(dp->dp_Arg2),BADDR(dp->dp_Arg3),dp->dp_Arg4,&res2);
						break;

					case ACTION_CREATE_DIR:
						/* Lock,Name -> Lock */

						res1 = (IPTR)Action_CreateDir((struct FileLock *)BADDR(dp->dp_Arg1),(APTR)BADDR(dp->dp_Arg2),&res2);
						break;

					case ACTION_EXAMINE_OBJECT:
						/* FileLock,FileInfoBlock -> Bool */

						res1 = Action_ExamineObject((struct FileLock *)BADDR(dp->dp_Arg1),(struct FileInfoBlock *)BADDR(dp->dp_Arg2),&res2);
						break;

					case ACTION_EXAMINE_NEXT:
						/* FileLock,FileInfoBlock -> Bool */

						res1 = Action_ExamineNext((struct FileLock *)BADDR(dp->dp_Arg1),(struct FileInfoBlock *)BADDR(dp->dp_Arg2),&res2);
						break;

					case ACTION_DISK_INFO:
						/* InfoData -> Bool */

						Action_DiskInfo((struct InfoData *)BADDR(dp->dp_Arg1),&res2);
						res1 = DOSTRUE;
						res2 = 0;
						break;

					case ACTION_INFO:
						/* FileLock,InfoData -> Bool */

						res1 = Action_Info((struct FileLock *)BADDR(dp->dp_Arg1),(struct InfoData *)BADDR(dp->dp_Arg2),&res2);
						break;

					case ACTION_SET_COMMENT:
						/* (Ignore),FileLock,Name,Comment -> Bool */

						res1 = Action_SetComment((struct FileLock *)BADDR(dp->dp_Arg2),BADDR(dp->dp_Arg3),BADDR(dp->dp_Arg4),&res2);
						break;

					case ACTION_PARENT:
						/* Lock -> Lock */

						res1 = (IPTR)Action_Parent((struct FileLock *)BADDR(dp->dp_Arg1),&res2);
						break;

					case ACTION_INHIBIT:

						SHOWMSG("ACTION_INHIBIT");
						res1 = DOSTRUE;
						break;

					case ACTION_SET_DATE:
						/* (Ignore),FileLock,Name,DateStamp(APTR) -> Bool */

						res1 = Action_SetDate((struct FileLock *)BADDR(dp->dp_Arg2),(APTR)BADDR(dp->dp_Arg3),(struct DateStamp *)dp->dp_Arg4,&res2);
						break;

					case ACTION_SAME_LOCK:
						/* Lock,Lock -> Bool */

						res1 = Action_SameLock((struct FileLock *)BADDR(dp->dp_Arg1),(struct FileLock *)BADDR(dp->dp_Arg2),&res2);
						break;

					case ACTION_READ:
						/* FileHandle->fh_Arg1,Buffer(APTR),Length -> Length */

						res1 = Action_Read((struct FileNode *)dp->dp_Arg1,(APTR)dp->dp_Arg2,dp->dp_Arg3,&res2);
						break;

					case ACTION_WRITE:
						/* FileHandle->fh_Arg1,Buffer(APTR),Length -> Length */

						res1 = Action_Write((struct FileNode *)dp->dp_Arg1,(APTR)dp->dp_Arg2,dp->dp_Arg3,&res2);
						break;

					case ACTION_FINDUPDATE:
					case ACTION_FINDINPUT:
					case ACTION_FINDOUTPUT:
						/* FileHandle,FileLock,Name -> Bool */

						res1 = Action_Find(dp->dp_Action,(struct FileHandle *)BADDR(dp->dp_Arg1),(struct FileLock *)BADDR(dp->dp_Arg2),(APTR)BADDR(dp->dp_Arg3),&res2);
						break;

					case ACTION_END:
						/* FileHandle->fh_Arg1 -> Bool */

						res1 = Action_End((struct FileNode *)dp->dp_Arg1,&res2);
						break;

					case ACTION_SEEK:
						/* FileHandle->fh_Arg1,Position,Mode -> Position */

						res1 = Action_Seek((struct FileNode *)dp->dp_Arg1,dp->dp_Arg2,dp->dp_Arg3,&res2);
						break;

					case ACTION_SET_FILE_SIZE:
						/* FileHandle->fh_Arg1,Offset,Mode -> New file size */

						res1 = Action_SetFileSize((struct FileNode *)dp->dp_Arg1,dp->dp_Arg2,dp->dp_Arg3,&res2);
						break;

					case ACTION_WRITE_PROTECT:
						/* Flag,Key -> Bool */

						res1 = Action_WriteProtect(dp->dp_Arg1,dp->dp_Arg2,&res2);
						break;

					case ACTION_FH_FROM_LOCK:
						/* FileHandle(BPTR),FileLock -> Bool */

						res1 = Action_FHFromLock((struct FileHandle *)BADDR(dp->dp_Arg1),(struct FileLock *)BADDR(dp->dp_Arg2),&res2);
						break;

					case ACTION_IS_FILESYSTEM:

						SHOWMSG("ACTION_IS_FILESYSTEM");
						res1 = DOSTRUE;
						break;

					case ACTION_CHANGE_MODE:
						/* Type,Object,Mode -> Bool */

						res1 = Action_ChangeMode(dp->dp_Arg1,(APTR)BADDR(dp->dp_Arg2),dp->dp_Arg3,&res2);
						break;

					case ACTION_COPY_DIR_FH:
						/* FileHandle->fh_Arg1 -> Bool */

						res1 = (IPTR)Action_CopyDirFH((struct FileNode *)dp->dp_Arg1,&res2);
						break;

					case ACTION_PARENT_FH:
						/* FileHandle->fh_Arg1 -> Bool */

						res1 = (IPTR)Action_ParentFH((struct FileNode *)dp->dp_Arg1,&res2);
						break;

					case ACTION_EXAMINE_ALL:
						/* FileLock,ExAllData(APTR),Size,Type,ExAllControl(APTR) -> Bool */

						res1 = Action_ExamineAll((struct FileLock *)BADDR(dp->dp_Arg1),(struct ExAllData *)dp->dp_Arg2,
							dp->dp_Arg3,dp->dp_Arg4,(struct ExAllControl *)dp->dp_Arg5,&res2);

						break;

					case ACTION_EXAMINE_FH:
						/* FileHandle->fh_Arg1,FileInfoBlock -> Bool */

						res1 = Action_ExamineFH((struct FileNode *)dp->dp_Arg1,(struct FileInfoBlock *)BADDR(dp->dp_Arg2),&res2);
						break;

					case ACTION_EXAMINE_ALL_END:
						/* FileLock,ExAllData(APTR),Size,Type,ExAllControl(APTR) -> Bool */

						res1 = DOSTRUE;
						break;

					case ACTION_LOCK_RECORD:
						/* FileHandle->fh_Arg1,position,length,mode,time-out -> Bool */
						res1 =  Action_LockRecord((struct FileNode *)dp->dp_Arg1,dp->dp_Arg2,dp->dp_Arg3,dp->dp_Arg4,(ULONG)dp->dp_Arg5,&res2);
						break;

					case ACTION_FREE_RECORD:
						/* FileHandle->fh_Arg1,position,length -> Bool */
						res1 =  Action_FreeRecord((struct FileNode *)dp->dp_Arg1,dp->dp_Arg2,dp->dp_Arg3,&res2);
						break;

					default:

						D(("Anything goes: dp->dp_Action=%ld (0x%lx)",dp->dp_Action,dp->dp_Action));

						res1 = DOSFALSE;
						res2 = ERROR_ACTION_NOT_KNOWN;

						break;
				}

				SHOWVALUE(res1);
				SHOWVALUE(res2);

				ReplyPkt(dp,res1,res2);

				D(("\n"));
			}
		}

		#if DEBUG
		{
			if(signals & SIGBREAKF_CTRL_F)
			{
				struct FileNode * fn;
				struct LockNode * ln;

				D(("list of open files:"));

				for(fn = (struct FileNode *)FileList.mlh_Head ;
				    fn->fn_MinNode.mln_Succ != NULL ;
				    fn = (struct FileNode *)fn->fn_MinNode.mln_Succ)
				{
					D(("  name='%s'",fn->fn_FullName));
					D(("  mode=%ld, offset=%ld",fn->fn_Mode,fn->fn_Offset));
					D((""));
				}

				D(("list of allocated locks:"));

				for(ln = (struct LockNode *)LockList.mlh_Head ;
				    ln->ln_MinNode.mln_Succ != NULL ;
				    ln = (struct LockNode *)ln->ln_MinNode.mln_Succ)
				{
					D(("  name='%s'",ln->ln_FullName));
					D(("  mode=%ld",ln->ln_FileLock.fl_Access));
					D((""));
				}
			}
		}
		#endif /* DEBUG */

		if(signals & SIGBREAKF_CTRL_C)
			Quit = TRUE;

		if(Quit)
		{
			if(IsListEmpty((struct List *)&FileList) && IsListEmpty((struct List *)&LockList))
			{
				SHOWMSG("no locks or files pending; quitting");
				done = TRUE;
			}
			else
			{
				SHOWMSG("locks or files still pending; cannot quit yet");
			}
		}
	}
	while(NOT done);

	if(sign_off)
		LocalPrintf("stopped.\n");

	LEAVE();
}

/****************************************************************************/

/*
 * Copy src to string dst of size siz.	At most siz-1 characters
 * will be copied.  Always NUL terminates (unless siz == 0).
 * Returns strlen(src); if retval >= siz, truncation occurred.
 */
size_t
strlcpy(char *dst, const char *src, size_t siz)
{
	char *d = dst;
	const char *s = src;
	size_t n = siz;
	size_t result;

	/* Copy as many bytes as will fit */
	if(n != 0 && --n != 0)
	{
		do
		{
			if(((*d++) = (*s++)) == '\0')
				break;
		}
		while(--n != 0);
	}

	/* Not enough room in dst, add NUL and traverse rest of src */
	if(n == 0)
	{
		if(siz != 0)
			(*d) = '\0';	/* NUL-terminate dst */

		while((*s++) != '\0')
			;
	}

	result = s - src - 1;	/* count does not include NUL */

	return(result);
}

/*
 * Appends src to string dst of size siz (unlike strncat, siz is the
 * full size of dst, not space left).  At most siz-1 characters
 * will be copied.  Always NUL terminates (unless siz <= strlen(dst)).
 * Returns strlen(src) + MIN(siz, strlen(initial dst)).
 * If retval >= siz, truncation occurred.
 */
size_t
strlcat(char *dst, const char *src, size_t siz)
{
	char *d = dst;
	const char *s = src;
	size_t n = siz;
	size_t dlen;
	size_t result;

	/* Find the end of dst and adjust bytes left but don't go past end */
	while(n-- != 0 && (*d) != '\0')
		d++;

	dlen = d - dst;
	n = siz - dlen;

	if(n == 0)
	{
		result = dlen + strlen(s);
	}
	else
	{
		while((*s) != '\0')
		{
			if(n != 1)
			{
				(*d++) = (*s);
				n--;
			}

			s++;
		}

		(*d) = '\0';

		result = dlen + (s - src);	 /* count does not include NUL */
	}

	return(result);
}
