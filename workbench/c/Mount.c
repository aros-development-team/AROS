/*
    (C) 1995-2008 The AROS Development Team
    (C) 2002-2005 Harry Sintonen
    (C) 2005-2007 Pavel Fedin
    $Id$

    Desc: Mount CLI command
    Lang: English
*/

/******************************************************************************

    NAME

        Mount

    FORMAT

        Mount <Device> <From>

    SYNOPSIS

        DEVICE/M, FROM/K

    LOCATION

        Sys:C

    FUNCTION

        Loads and mounts a device

    INPUTS

        DEVICE -- The device type to be mounted
        FROM   -- Search device in this mountlist

    RESULT

        Standard DOS error codes.

    NOTES

    EXAMPLE

        Mount DEVS:FAT0
        (Mounts a FAT device defined in the DEVS:FAT0 file)

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

******************************************************************************/

#include <exec/devices.h>
#include <exec/io.h>
#include <exec/memory.h>
#include <exec/semaphores.h>
#include <exec/types.h>
#include <dos/dosextens.h>
#include <dos/exall.h>
#include <dos/filehandler.h>
#include <dos/rdargs.h>
#include <libraries/configvars.h>
#include <libraries/expansion.h>
#include <workbench/workbench.h>
#include <workbench/startup.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/icon.h>
#include <proto/expansion.h>

#ifndef __SASC
#ifdef __AROS__
#include <clib/arossupport_protos.h>
#else
#include <clib/debug_protos.h>
#endif
#else
typedef unsigned long IPTR;
#endif

#include <resources/filesysres.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define	DEBUG_PATCHDOSNODE(x)
#define DEBUG_MOUNT(x)
#define	DEBUG_MAKEDOSNODE(x)
#define	DEBUG_CHECK(x)

#define	MOUNTLIST      "DEVS:MountList"
#define	DOSDRIVERS     "DEVS:DOSDrivers/"
#define	STORAGEDRIVERS "SYS:Storage/DOSDrivers/"
#define	PARAMSLENGTH   (sizeof(struct DosEnvec) + sizeof(IPTR)*4)

enum
{
	ERR_SPECIAL = 5000,
	ERR_DEVICENOTFOUND,
	ERR_INVALIDKEYWORD
};

const char *SearchTable[]=
{
	"",
	"DEVS:DOSDrivers/",
#ifdef __MORPHOS__
	"MOSSYS:DEVS/DOSDrivers/",
#endif
	"SYS:Storage/DOSDrivers/",
#ifdef __MORPHOS__
	"MOSSYS:Storage/DOSDrivers/",
#endif
	NULL
};

/*
 * startup,control need to be handled differently.
 */

enum
{
	ARG_HANDLER,
	ARG_EHANDLER,
	ARG_FILESYSTEM,
	ARG_DEVICE,
	ARG_UNIT,
	ARG_FLAGS,
	ARG_BLOCKSIZE,
	ARG_SURFACES,
	ARG_BLOCKSPERTRACK,
	ARG_SECTORSPERBLOCK,
	ARG_RESERVED,
	ARG_PREALLOC,
	ARG_INTERLEAVE,
	ARG_LOWCYL,
	ARG_HIGHCYL,
	ARG_BUFFERS,
	ARG_BUFMEMTYPE,
	ARG_MAXTRANSFER,
	ARG_MASK,
	ARG_BOOTPRI,
	ARG_DOSTYPE,
	ARG_BAUD,
	ARG_CONTROL,
	ARG_STACKSIZE,
	ARG_PRIORITY,
	ARG_GLOBVEC,
	ARG_STARTUP,
	ARG_ACTIVATE,
	ARG_FORCELOAD,
	NUM_ARGS
};

const UBYTE options[]=
	"HANDLER/K,"
	"EHANDLER/K,"
	"FILESYSTEM/K,"
	"DEVICE/K,"
	"UNIT/K,"
	"FLAGS/K,"
	"SECTORSIZE=BLOCKSIZE/K,"
	"SURFACES/K,"
	"SECTORSPERTRACK=BLOCKSPERTRACK/K,"
	"SECTORSPERBLOCK/K,"
	"RESERVED/K,"
	"PREALLOC/K,"
	"INTERLEAVE/K,"
	"LOWCYL/K,"
	"HIGHCYL/K,"
	"BUFFERS/K,"
	"BUFMEMTYPE/K,"
	"MAXTRANSFER/K,"
	"MASK/K,"
	"BOOTPRI/K,"
	"DOSTYPE/K,"
	"BAUD/K,"
	"CONTROL/K,"
	"STACKSIZE/K,"
	"PRIORITY/K,"
	"GLOBVEC/K,"
	"STARTUP/K,"
	"MOUNT=ACTIVATE/K,"
	"FORCELOAD/K";

#ifdef __MORPHOS__
#define PROGNAME "Mount unofficial"
#define __stackparm __attribute__((varargs68k))
typedef struct Library *UtilityBase_t;
#else
#define PROGNAME "Mount"
typedef struct UtilityBase *UtilityBase_t;
#endif

#ifdef __PPC__
#define ARGS(ap) ap->overflow_arg_area
#else
#define ARGS(ap) (IPTR *)ap
#endif

#ifdef __AROS__
#define _WBenchMsg WBenchMsg
#endif

#ifdef AROS_FAST_BPTR
#define BSTR_EXTRA 1
#define BSTR_OFFSET 0
#define bstrcpy(dest, src, len) strcpy(dest, src)
#else
#define BSTR_EXTRA 2
#define BSTR_OFFSET 1
#define bstrcpy(dest, src, len) \
	dest[0] = len; \
	strcpy(&dest[1], src);
#endif

static const int __nocommandline;
const TEXT version[] = "\0$VER: " PROGNAME " 50.14 (10.1.2008)";

ULONG CheckDevice(char *name);
void  InitParams(IPTR *params);
LONG  readfile(STRPTR name, STRPTR *mem, LONG *size);
ULONG readmountlist(IPTR *params, STRPTR name, char *mountlist);
ULONG readmountfile(IPTR *params, STRPTR name);
void preparefile(STRPTR buf, LONG size);
LONG parsemountfile(IPTR *params, STRPTR buf, LONG size);
LONG parsemountlist(IPTR *params, STRPTR name, STRPTR buf, LONG	size);
LONG mount(IPTR	*params, STRPTR	name);
void __stackparm ShowError(char *s, ...);
void ShowFault(LONG code, char *s, ...);

struct DosLibrary *DOSBase;
struct IntuitionBase *IntuitionBase;
UtilityBase_t UtilityBase;
struct Process *MyProcess;

ULONG StartupValue;
char *StartupString = NULL;
char *ControlString = NULL;
char *UnitString = NULL;
char *FlagsString = NULL;
ULONG StackSize;
ULONG Priority;
ULONG Activate;
int   GlobalVec;
ULONG ForceLoad;
char *HandlerString;
char *DeviceString;
BOOL  IsEHandler, IsFilesystem;
BOOL  IsCli;
BOOL flagargs[NUM_ARGS];
extern struct WBStartup *_WBenchMsg;

int main(void)
{
  STRPTR args[2];
  IPTR *params;
  LONG error = RETURN_FAIL;
  struct RDArgs	*rda;
  char dirname[512];

  if ((DOSBase = (struct DosLibrary *)OpenLibrary("dos.library",37))!=0)
  {
    if ((UtilityBase = (UtilityBase_t)OpenLibrary("utility.library",37)))
    {
	memset(&flagargs, 0, sizeof(flagargs));
	IsEHandler = TRUE;
	IsFilesystem = TRUE;
	if (!_WBenchMsg)
        {
          memset(args,0,sizeof(args));
          if ((rda = ReadArgs("DEVICE/M,FROM/K", (IPTR *)args, NULL)))
          {
            STRPTR	*MyDevPtr;
            int		len;

            error = 0;

            MyDevPtr	=(STRPTR *)args[0];
            if (MyDevPtr)
            {
              while (*MyDevPtr)
              {
                DEBUG_MOUNT(Printf("Mount: Current DevName <%s>\n",
                                   (ULONG)*MyDevPtr));

		if ((params = AllocVec(PARAMSLENGTH, MEMF_PUBLIC | MEMF_CLEAR)))
                {
		  StackSize	= 8192;
		  Priority	= 5;
		  GlobalVec	= -1;
		  HandlerString	= NULL;
		  DeviceString	= NULL;
		  StartupString	= NULL;

                  len = strlen(*MyDevPtr);
                  if ((*MyDevPtr)[len-1] == ':')
                  {
                    /* search for a devicename */
                    DEBUG_MOUNT(Printf("Mount: search for devname <%s>\n",
                                       (ULONG)*MyDevPtr));

		    strcpy(dirname, *MyDevPtr);
		    dirname[len-1] = '\0';

                    if ((error=CheckDevice(dirname))!=RETURN_OK)
                    {
                      DEBUG_MOUNT(Printf("Mount: is already mounted..stop\n"));
                    }
                    else
                    {
                      if (args[1])
                      {
			error=readmountlist(params, dirname, args[1]);
			DEBUG_MOUNT(Printf("Mount: readmountlist(%s) returned %ld\n", args[1], error));
                      }
                      else
                      {
                        char	**SearchPtr;
                        ULONG	slen;

                        DEBUG_MOUNT(Printf("Mount: search device definition <%s>\n",
                                           (ULONG)*MyDevPtr));
                        for (SearchPtr=(char**) SearchTable;
                             *SearchPtr;
                             SearchPtr++)
                        {
                          if(SetSignal(0L,SIGBREAKF_CTRL_C) & SIGBREAKF_CTRL_C)
                          {
                            error = RETURN_FAIL;
                            SetIoErr(ERROR_BREAK);
                            break;
                          }

                          slen = strlen(*SearchPtr);
			  strcpy(dirname, *SearchPtr);
			  dirname[slen]	= '\0';
                          strcat(dirname, *MyDevPtr);
			  dirname[slen+len-1] =	'\0';
			  DEBUG_MOUNT(Printf("Mount: try File <%s>\n", (ULONG)dirname));

			  error=readmountfile(params, dirname);
			  DEBUG_MOUNT(Printf("Mount: readmountfile returned %ld\n", error));
			  if (error != ERROR_OBJECT_NOT_FOUND)
			    break;
                        }
			if (error == ERROR_OBJECT_NOT_FOUND)
                        {
                          DEBUG_MOUNT(Printf("Mount: try from mountlist\n"));
			  dirname[0] = '\0';
                          strcat(dirname, *MyDevPtr);
			  dirname[len-1] = '\0';
			  error=readmountlist(params, dirname, MOUNTLIST);
			  DEBUG_MOUNT(Printf("Mount: readmountlist(default) returned %ld\n", error));
                        }
                      }
                    }
                  }
                  else
                  {
                    /* search for a filename */

                    LONG err;

                    UBYTE stack_ap[sizeof(struct AnchorPath) + 3];
                    struct AnchorPath	*MyAp = (struct AnchorPath *) (((ULONG) stack_ap + 3) & ~3);

                    DEBUG_MOUNT(Printf("Mount: search for mountfile <%s>\n",
                                       (ULONG)*MyDevPtr));

                    memset(MyAp,0,sizeof(struct AnchorPath));

                    dirname[0]	=	'\0';
                    for (err = MatchFirst(*MyDevPtr,MyAp);
                         err == 0;
                         err = MatchNext(MyAp))
                    {
                      if (MyAp->ap_Flags & APF_DirChanged)
                      {
                        DEBUG_MOUNT(Printf("Mount: Changed directories...\n"));
                      }

                      if (NameFromLock(MyAp->ap_Current->an_Lock,
                                       dirname,
                                       sizeof(dirname)) == FALSE)
                      {
			ShowFault(IoErr(), "Error on NameFromLock");
                        break;
                      }
                      if (AddPart(dirname,
                                  &(MyAp->ap_Info.fib_FileName[0]),
                                  sizeof(dirname)) == FALSE)
                      {
			ShowFault(IoErr(), "Error on AddPart");
                        break;
                      }
                      if (MyAp->ap_Info.fib_DirEntryType > 0)
                      {
                        if (MyAp->ap_Flags & APF_DIDDIR)
                        {
                          DEBUG_MOUNT(Printf("Mount: Ascending from directory %s\n",
                                        (ULONG)dirname));
                        }
                        else
                        {
                          DEBUG_MOUNT(Printf("Mount: The next dir is  ... %s\n", (ULONG)dirname));
                        }
                        /* clear the completed directory flag */
                        MyAp->ap_Flags     &=      ~APF_DIDDIR;

                      }
                      else
                      {
                        /* Here is code for handling each particular file */

                        DEBUG_MOUNT(Printf("Mount: try File <%s>\n",
                                           (ULONG)dirname));

                        memset(&flagargs, 0, sizeof(flagargs));
			IsEHandler = TRUE;
			IsFilesystem = TRUE;
			error=readmountfile(params, dirname);
			DEBUG_MOUNT(Printf("Mount: readmount file returned %ld\n", error));
                      }
                    }
                    /* This absolutely, positively must be called, all of the time. */
                    MatchEnd(MyAp);

                    if (err == ERROR_NO_MORE_ENTRIES)
                    {
                      SetIoErr(0);
                    }
                    else
                    {
                      /* if it was real error promote it - Piru */
                      error = err;
                    }
                  }
                  FreeVec(params);
                }
                else
                {
                  error = ERROR_NO_FREE_STORE;
                  break;
                }
                MyDevPtr++;
              }
            }
            FreeArgs(rda);
          } /* if (rda != NULL) */
          else
          {
            error = IoErr();
          }

          if (error && error != ERROR_NO_MORE_ENTRIES && error < ERR_SPECIAL)
          {
	    ShowFault(error, "ERROR");

            error = RETURN_FAIL;
          }
          else
          {
            error = error < ERR_SPECIAL ? RETURN_OK : RETURN_FAIL;
          }

        }
        else
        {
          /* wb startup */
	  if (_WBenchMsg->sm_NumArgs >= 2)
          {
            if ((params = AllocVec(PARAMSLENGTH,
                                   MEMF_PUBLIC | MEMF_CLEAR)))
            {
              int i;

	      for (i = 1; i < _WBenchMsg->sm_NumArgs; i++)
              {
                BPTR olddir;

                DEBUG_MOUNT(kprintf("Mount: try File <%s>\n",
				   (ULONG) _WBenchMsg->sm_ArgList[i].wa_Name));

		olddir = CurrentDir(_WBenchMsg->sm_ArgList[i].wa_Lock);

		error=readmountfile(params, _WBenchMsg->sm_ArgList[i].wa_Name);
		DEBUG_MOUNT(kprintf("Mount: readmountfile returned %ld\n", error));
                if (error && error != ERROR_NO_MORE_ENTRIES && error < ERR_SPECIAL)
	          ShowFault(error, "ERROR");

                (void) CurrentDir(olddir);
              }

              FreeVec(params);
            }
            else
            {
              error = ERROR_NO_FREE_STORE;
            }
          }
        }
        CloseLibrary((struct Library *)UtilityBase);
    }
    CloseLibrary((struct Library *)DOSBase);
  }

  return error;
}

/************************************************************************************************/
/************************************************************************************************/
ULONG CheckDevice(char *name)
{
struct DosList	*dl;
ULONG		Status;

  DEBUG_CHECK(Printf("CheckDevice: <%s>\n",
                     name));

  dl = LockDosList(LDF_DEVICES | LDF_VOLUMES | LDF_ASSIGNS | LDF_READ);
  if ((dl = FindDosEntry(dl,name,LDF_DEVICES | LDF_VOLUMES | LDF_ASSIGNS)))
  {
    Status	=	ERROR_OBJECT_EXISTS;
  }
  else
  {
    Status	=	0;
  }
  UnLockDosList(LDF_DEVICES | LDF_VOLUMES | LDF_ASSIGNS | LDF_READ);

  DEBUG_CHECK(Printf("CheckDevice: object %sexist\n", Status ? "does" : "doesn't "));

  return Status;
}

/************************************************************************************************/
/************************************************************************************************/

void InitParams(IPTR *params)
{
  struct DosEnvec *vec;

  memset(params,0, PARAMSLENGTH);

  vec = (struct DosEnvec *)&params[4];

  vec->de_TableSize	 = DE_BOOTBLOCKS;
  vec->de_SizeBlock	 = 512 >> 2;
  vec->de_Surfaces	 = 2;
  vec->de_SectorPerBlock = 1;
  vec->de_BlocksPerTrack = 11;
  vec->de_Reserved	 = 2;

/* memset above
  vec->de_SecOrg	 = 0;
  vec->de_BootBlocks	 = 0;
  vec->de_BootPri	 = 0;
  vec->de_PreAlloc	 = 0;
  vec->de_Interleave	 = 0;
  vec->de_LowCyl	 = 0;
*/

  vec->de_HighCyl	 = 79;
  vec->de_NumBuffers	 = 20; /* On AmigaOS 3.9 it's 5 */
  vec->de_BufMemType	 = 3;
  vec->de_Baud		 = 1200;
  vec->de_MaxTransfer	 = 0x7fffffff;
  vec->de_Mask		 = 0xfffffffe;
  vec->de_DosType	 = ID_DOS_DISK;

  StackSize		 = 8192;
  Priority		 = 5;
  GlobalVec		 = -1;
  HandlerString		 = NULL;
  DeviceString		 = NULL;
  StartupString		 = NULL;
}

void FreeStuff(void)
{
  if (UnitString)
  {
    FreeVec(UnitString);
    UnitString = NULL;
  }
  if (FlagsString)
  {
    FreeVec(FlagsString);
    FlagsString	= NULL;
  }
  if (ControlString)
  {
    FreeVec(ControlString);
    ControlString = NULL;
  }
  if (HandlerString)
  {
    FreeVec(HandlerString);
    HandlerString = NULL;
  }
  if (DeviceString)
  {
    FreeVec(DeviceString);
    DeviceString = NULL;
  }
  if (StartupString)
  {
    FreeVec(StartupString);
    StartupString = NULL;
  }
}

/************************************************************************************************/
/************************************************************************************************/

ULONG GetValue(char *buf, char **end)
{
        int base;
        char *c = buf;

	/* I decided to leave this routine in order to prevent reading numbers starting from '0'
           as octal. Probably this is not needed, or octal support would not do any harm, in this
           case this routine is not needed at all - Pavel Fedin */
        if ((c[0]=='-') || (c[1]=='+'))
		c++;
        if ((c[0] == '0') && (((c[1])=='x') || (c[1])=='X'))
        	base = 16;
        else
        	base = 10;
        return strtol(buf,end,base);
/* Left for reference - Pavel Fedin
ULONG	Value;
ULONG	Sign;
  Value = 0;
  Sign = 1;
  if (buf[0]=='-')
  {
    Sign = -1;
    buf++;
  }
  else
  if (buf[0]=='+')
  {
    buf++;
  }

  if ((buf[0] == '0') && (((buf[1])=='x') || (buf[1])=='X'))
  {
    int num;

    // base = hex
    buf+=2;
    while(*buf)
    {
      Value<<=4;
      num=*buf++;

      if((num >= 0x30) && (num <= 0x39))
      {
        num-=0x30;
        Value+=num;
      }
      else
      {
        num |= 0x20;

        if((num >= 0x61) && (num <= 0x66))
        {
          num-=(0x61-10);
          Value+=num;
        }
      }
    }
  }
  else
  {
    // base = dev
    Value=atoi(buf);
  }
  return Value * Sign;
*/
}

/************************************************************************************************/
/************************************************************************************************/

ULONG ReadMountArgs(IPTR *params, struct RDArgs	*rda)
{
	struct DosEnvec *vec;
	STRPTR args[NUM_ARGS];
	struct RDArgs *MyRDA;
	ULONG result = RETURN_OK;
	int i;
	char *s = NULL;

	DEBUG_MOUNT(Printf("ReadMountArgs:\n%s\n\n",(ULONG)&rda->RDA_Source.CS_Buffer[rda->RDA_Source.CS_CurChr]));

	memset(&args, 0, sizeof(args));

	if (!(MyRDA = ReadArgs((STRPTR)options, (IPTR *)args, rda)))
	{
		DEBUG_MOUNT(Printf("ReadMountArgs: ReadArgs failed\n"));
		DEBUG_MOUNT(PrintFault(IoErr(),"ReadMountArgs"));
		//return (ULONG) IoErr();
		return  ERR_INVALIDKEYWORD;
	}

	for (i = 0; i < NUM_ARGS; i++)
	{
		if (args[i] != NULL)
		{
			flagargs[i] = TRUE;
		}
	}

	if (args[ARG_HANDLER] != NULL)
	{
		s = (STRPTR)args[ARG_HANDLER];
		IsEHandler = FALSE;
		IsFilesystem = FALSE;
	}
	else if (args[ARG_EHANDLER] != NULL)
	{
		s = (STRPTR)args[ARG_EHANDLER];
		IsEHandler = TRUE;
		IsFilesystem = FALSE;
	}
	else if (args[ARG_FILESYSTEM] != NULL)
	{
		s = (STRPTR)args[ARG_FILESYSTEM];
		IsEHandler = TRUE;
		IsFilesystem = TRUE;
	} else
		s = NULL;
	if (s)
	{
		int len;
		DEBUG_MOUNT(Printf("ReadMountArgs: Handler <%s>\n",s));
		len = strlen(s);
		if (HandlerString)
		{
			FreeVec(HandlerString);
		}
		if ((HandlerString = AllocVec(len + BSTR_EXTRA, MEMF_PUBLIC|MEMF_CLEAR)))
		{
			bstrcpy(HandlerString, s, len);
		}
	}
	if (args[ARG_STACKSIZE] != NULL)
	{
		StackSize = GetValue(args[ARG_STACKSIZE], NULL);
	}

	if (args[ARG_PRIORITY] != NULL)
	{
		Priority = GetValue(args[ARG_PRIORITY], NULL);
	}

	if (args[ARG_GLOBVEC] != NULL)
	{
		GlobalVec = GetValue(args[ARG_GLOBVEC], NULL);
	}

	if (args[ARG_FORCELOAD] != NULL)
	{
		ForceLoad = GetValue((STRPTR)args[ARG_FORCELOAD], NULL);
	}

	if (args[ARG_ACTIVATE] != NULL)
	{
		Activate = GetValue(args[ARG_ACTIVATE], NULL);
	}

	if (args[ARG_DEVICE] != NULL)
	{
		int len;

		DEBUG_MOUNT(Printf("ReadMountArgs: Device <%s>\n",args[ARG_DEVICE]));

		len = strlen((STRPTR)args[ARG_DEVICE]);

		if (DeviceString)
		{
			FreeVec(DeviceString);
		}
		if ((DeviceString = AllocVec(len+1,MEMF_PUBLIC|MEMF_CLEAR)))
		{
			//Printf("copying...\n");

			strcpy(DeviceString, args[ARG_DEVICE]);
		}
	}

	if (args[ARG_UNIT] != NULL)
	{
		if (UnitString)
		{
			FreeVec(UnitString);
			UnitString = NULL;
		}
		params[2] = GetValue(args[ARG_UNIT], &s);
		if (*s)
		{
			int len;

			len = strlen(args[ARG_UNIT]);

			DEBUG_MOUNT(Printf("ReadMountArgs: len %ld\n",len));

			if ((UnitString = AllocVec(len + 1, MEMF_PUBLIC|MEMF_CLEAR)))
			{
				strcpy(UnitString, args[ARG_UNIT]);
				params[2] = (IPTR)UnitString;
				DEBUG_MOUNT(Printf("ReadMountArgs: Unit String <%s>\n", (STRPTR)params[2]));
			}
			else
			{
				result = ERROR_NO_FREE_STORE;
				goto error;
			}
		}
        	else
			DEBUG_MOUNT(Printf("ReadMountArgs: Unit Value %ld\n",params[2]));
	}
	if (args[ARG_FLAGS] != NULL)
	{
//			char *String;

		DEBUG_MOUNT(Printf("ReadMountArgs: Flags <%s>\n",args[ARG_FLAGS]));
		if (FlagsString)
		{
			FreeVec(FlagsString);
			FlagsString = NULL;
		}
/*
		String	=	args[ARG_FLAGS];

		if ((*String >= 0x30) && (*String <= 0x39))
		{
			params[3] = GetValue(String);
			DEBUG_MOUNT(Printf("ReadMountArgs: Flag Value %ld\n",params[3]));
		}
		else
*/
		params[3] = GetValue(args[ARG_FLAGS], &s);
		if (*s)
		{
			int len;

			len = strlen(args[ARG_FLAGS]);

			DEBUG_MOUNT(Printf("ReadMountArgs: len %ld\n",len));

			if ((FlagsString = AllocVec(len + 1, MEMF_PUBLIC|MEMF_CLEAR)))
			{
				strcpy(FlagsString, args[ARG_FLAGS]);
				params[3] = (IPTR) FlagsString;
				DEBUG_MOUNT(Printf("ReadMountArgs: Flags String <%s>\n",(STRPTR)params[3]));
			}
			else
			{
				result = ERROR_NO_FREE_STORE;
				goto error;
			}
		}
        	else
			DEBUG_MOUNT(Printf("ReadMountArgs: Flag Value %ld\n",params[3]));
	}

        vec = (struct DosEnvec *)&params[4];

	if (args[ARG_BLOCKSIZE] != NULL)
	{
		vec->de_SizeBlock = GetValue(args[ARG_BLOCKSIZE], NULL) >> 2;
	}
	if (args[ARG_SURFACES] != NULL)
	{
		vec->de_Surfaces = GetValue(args[ARG_SURFACES], NULL);
	}
	if (args[ARG_SECTORSPERBLOCK] != NULL)
	{
		vec->de_SectorPerBlock = GetValue(args[ARG_SECTORSPERBLOCK], NULL);
	}
	if (args[ARG_BLOCKSPERTRACK] != NULL)
	{
		vec->de_BlocksPerTrack = GetValue(args[ARG_BLOCKSPERTRACK], NULL);
	}
	if (args[ARG_RESERVED] != NULL)
	{
		vec->de_Reserved = GetValue(args[ARG_RESERVED], NULL);
	}
	if (args[ARG_PREALLOC] != NULL)
	{
		vec->de_PreAlloc = GetValue(args[ARG_PREALLOC], NULL);
	}
	if (args[ARG_INTERLEAVE] != NULL)
	{
		vec->de_Interleave = GetValue(args[ARG_INTERLEAVE], NULL);
	}
	if (args[ARG_LOWCYL] != NULL)
	{
		vec->de_LowCyl = GetValue(args[ARG_LOWCYL], NULL);
	}
	if (args[ARG_HIGHCYL] != NULL)
	{
		vec->de_HighCyl	= GetValue(args[ARG_HIGHCYL], NULL);
	}
	if (args[ARG_BUFFERS] != NULL)
	{
		vec->de_NumBuffers = GetValue(args[ARG_BUFFERS], NULL);
	}
	if (args[ARG_BUFMEMTYPE] != NULL)
	{
		vec->de_BufMemType = GetValue(args[ARG_BUFMEMTYPE], NULL);
	}
	if (args[ARG_BOOTPRI] != NULL)
	{
		vec->de_BootPri	= GetValue(args[ARG_BOOTPRI], NULL);
	}
	if (args[ARG_BAUD] != NULL)
	{
		vec->de_Baud = GetValue(args[ARG_BAUD], NULL);
	}
	if (args[ARG_MAXTRANSFER] != NULL)
	{
		vec->de_MaxTransfer = GetValue(args[ARG_MAXTRANSFER], NULL);
	}
	if (args[ARG_MASK] != NULL)
	{
		vec->de_Mask = GetValue(args[ARG_MASK], NULL);
	}

	if (args[ARG_DOSTYPE] != NULL)
	{
		vec->de_DosType	= (IPTR)GetValue(args[ARG_DOSTYPE], NULL);
	}

	if (args[ARG_CONTROL] != NULL)
	{
		int len;
		DEBUG_MOUNT(Printf("ReadMountArgs: Control <%s>\n",args[ARG_CONTROL]));
		if (ControlString)
		{
			FreeVec(ControlString);
			ControlString =	NULL;
		}
		len = strlen(args[ARG_CONTROL]);
		if (len < 0x100)
		{
			if ((ControlString=AllocVec(len + BSTR_EXTRA, MEMF_PUBLIC|MEMF_CLEAR)))
			{
				bstrcpy(ControlString, args[ARG_CONTROL], len);
				vec->de_Control	= (IPTR)MKBADDR((char*)ControlString);
			}
			else
			{
				ShowError("Unable to allocate Control string");
				result = ERROR_NO_FREE_STORE;
				goto error;
			}
		}
		else
		{
		        result = ERROR_LINE_TOO_LONG;
			SetIoErr(result);
//			ShowError("Control string too long");
			goto error;
		}
	}

	if (args[ARG_STARTUP] != NULL)
	{
//		      char *String;

		DEBUG_MOUNT(Printf("ReadMountArgs: Startup <%s>\n",args[ARG_STARTUP]));
		if (StartupString)
		{
			FreeVec(StartupString);
			StartupString = NULL;
		}
/*
		String = args[ARG_STARTUP];
		if ((*String >= 0x30) && (*String <= 0x39))
		{
			StartupValue = GetValue(String);
		}
		else
*/
		StartupValue = GetValue(args[ARG_STARTUP], &s);
		if (*s)
		{
			int len;

			len = strlen(args[ARG_STARTUP]);

			DEBUG_MOUNT(Printf("ReadMountArgs: len %ld\n",len));

			if ((StartupString = AllocVec(len + 1, MEMF_PUBLIC|MEMF_CLEAR)))
			{
				strcpy(StartupString,args[ARG_STARTUP]);
			}
			else
			{
				result = ERROR_NO_FREE_STORE;
				goto error;
			}
		}
	}

error:
	FreeArgs(MyRDA);

	return result;
}

/************************************************************************************************/
/************************************************************************************************/

ULONG	readmountlist(IPTR	*params,
                      STRPTR	name,
                      char	*mountlist)
{
STRPTR	MountListBuf;
LONG	MountListBufSize;
ULONG	error;

  DEBUG_MOUNT(Printf("ReadMountList: find <%s> in mountlist <%s>\n",
                     name,
                     mountlist));

  error = readfile(mountlist,
                   &MountListBuf,
                   &MountListBufSize);
  if (error==RETURN_OK)
  {
    preparefile(MountListBuf,
                MountListBufSize);


    InitParams(params);

    if ((error=parsemountlist(params,
                              name,
                              MountListBuf,
                              MountListBufSize))==RETURN_OK)
    {
      if ((error = mount(params,name))!=RETURN_OK)
      {
        DEBUG_MOUNT(Printf("ReadMountList: mount failed error %ld\n",
                           error));
      }
    }
    else
    {
      switch (error)
      {
        case ERR_DEVICENOTFOUND:
        case ERR_INVALIDKEYWORD:
	  ShowError("Device '%s:' not found in file '%s'", name, mountlist);
          break;
      }
    }

    FreeStuff();
    FreeVec(MountListBuf);
  }
  return error;
}

/************************************************************************************************/
/************************************************************************************************/

ULONG readmountfile(IPTR *params, STRPTR filename)
{
struct Library		*IconBase;
struct DiskObject	*diskobj;
char			**myargv;
STRPTR			MountListBuf;
LONG			MountListBufSize;
struct RDArgs		rda;
ULONG			error = RETURN_FAIL;
UBYTE			*nameptr;
int			toollen;
BOOL			mountinfo=FALSE;
char			name[256+1];

  DEBUG_MOUNT(Printf("ReadMountFile: <%s>\n", (ULONG)filename));

  {
    struct Process *me = (APTR) FindTask(NULL);
    APTR oldwinptr;
    BPTR lock;

    name[0] = '\0';

    oldwinptr = me->pr_WindowPtr;
    me->pr_WindowPtr = (APTR) -1;
    lock = Lock(filename, SHARED_LOCK);
    if (lock)
    {
      struct FileInfoBlock fib;
      if (Examine(lock, &fib))
      {
        nameptr = fib.fib_FileName;
        memmove(name, nameptr, strlen(nameptr) + 1);
      }
      UnLock(lock);
    }
    me->pr_WindowPtr = oldwinptr;

    if (name[0] == '\0')
    {
      nameptr		=	FilePart(filename);
      strcpy(name,
             nameptr);
    }
  }

  DEBUG_MOUNT(Printf("ReadMountFile: mount <%s>\n", (ULONG)name));

  if ((error=CheckDevice(name))!=RETURN_OK)
  {
    return error;
  }

  InitParams(params);

  DEBUG_MOUNT(Printf("ReadMountFile: readfile\n"));

  error = readfile(filename,
                   &MountListBuf,
                   &MountListBufSize);
  if (error==RETURN_OK)
  {
    DEBUG_MOUNT(Printf("ReadMountFile: preparsefile\n"));
    preparefile(MountListBuf, MountListBufSize);


    DEBUG_MOUNT(Printf("ReadMountFile: parsemountfile\n"));
    if ((error = parsemountfile(params, MountListBuf, MountListBufSize))!=RETURN_OK)
    {
      DEBUG_MOUNT(Printf("ReadMountFile: parsemountfile error %ld\n", error));
      ShowFault(IoErr(), "Mountfile '%s' is invalid", filename);
    }
    else
    {
      mountinfo	=	TRUE;
    }
    FreeVec(MountListBuf);
  }
  else
  {
    DEBUG_MOUNT(Printf("ReadMountFile: mountfile not found..search for <%s.info>\n",
                       filename));
  }

  if ((error==RETURN_OK) ||
      (error==ERROR_OBJECT_NOT_FOUND))
  {
    DEBUG_MOUNT(Printf("ReadMountFile: look for icon\n"));

    if ((IconBase =  OpenLibrary("icon.library", 37)))
    {
      if ((diskobj = GetDiskObject(filename)))
      {
        myargv	=(char**) diskobj->do_ToolTypes;
        if (myargv)
        {
          while (*myargv)
          {
            char	*ToolPtr;
            ToolPtr	=	*myargv;
            DEBUG_MOUNT(Printf("ReadMountFile: ToolType <%s>\n",
                               ToolPtr));
            if ((ToolPtr[0] != '(') && (ToolPtr[0] != '*') &&
                !((ToolPtr[0] == 'I') && (ToolPtr[1] == 'M') && (ToolPtr[3] == '=')))
            {
              char	*ToolString;
              toollen	=	strlen(ToolPtr);
              if ((ToolString = AllocVec(toollen + 2,MEMF_ANY)))
              {
                memcpy(ToolString,ToolPtr,toollen);
                ToolString[toollen]	=	'\n';
                ToolString[toollen+1]	=	'\0';
                memset(&rda,0,sizeof(struct RDArgs));
		rda.RDA_Source.CS_Buffer = ToolString;
		rda.RDA_Source.CS_Length = toollen+1;
		rda.RDA_Source.CS_CurChr = 0;
                rda.RDA_Flags = RDAF_NOPROMPT;
		if ((ReadMountArgs(params, &rda)==RETURN_OK))
                {
		  mountinfo = TRUE;
                }
                else
                {
                  DEBUG_MOUNT(Printf("ReadMountFile: ReadArgs failed error %ld\n",
                                     error));
                }
                FreeVec(ToolString);
              }
              else
              {
                error = ERROR_NO_FREE_STORE;
                break;
              }
            }
            else
            {
              DEBUG_MOUNT(Printf("ReadMountFile: skipped\n"));
            }
            myargv++;
          }
        }
        FreeDiskObject(diskobj);
      }
      else
      {
      }
      CloseLibrary(IconBase);
    }
  }

  if (mountinfo)
  {
    DEBUG_MOUNT(Printf("ReadMountFile: mount information exists\n"));

    if ((error = mount(params,name)) != RETURN_OK)
    {
      DEBUG_MOUNT(Printf("ReadMountFile: mount failed error %ld\n",
                         error));
    }
  }

  FreeStuff();

  return error;
}

/************************************************************************************************/
/************************************************************************************************/


LONG readfile(STRPTR name, STRPTR *mem, LONG *size)
{
    BPTR ml;
    ULONG rest,sub;
    STRPTR buf;
    struct Process *me = (struct Process *) FindTask(NULL);
    APTR oldwinptr;

    oldwinptr = me->pr_WindowPtr;
    me->pr_WindowPtr = (APTR) -1;
    ml = Open(name, MODE_OLDFILE);
    me->pr_WindowPtr = oldwinptr;

    DEBUG_MOUNT(Printf("ReadFile: <%s>\n", (LONG) name));

    if (ml)
    {
	if (Seek(ml, 0, OFFSET_END) != -1)
	{
	    *size = Seek(ml, 0, OFFSET_BEGINNING);

	    if (*size != -1)
	    {
		*mem = (STRPTR)AllocVec(*size+2, MEMF_ANY);

		if (*mem)
		{
		    rest = *size;
		    buf = *mem;

		    for (;;)
		    {
			if (!rest)
			{
			    Close(ml);

                            *buf++ = '\n';
			    *buf = '\0';

			    return 0;
			}

			sub = Read(ml, buf, rest);

			if (sub == -1)
			{
			    break;
			}

			rest -= sub;
			buf += sub;
		    }

		    FreeVec(*mem);
		}
		else
		{
		    SetIoErr(ERROR_NO_FREE_STORE);
		}
	    }
	}

	Close(ml);
    }

    DEBUG_MOUNT(Printf("ReadFile: error %ld\n", IoErr()));
    return IoErr();
}

/************************************************************************************************/
/************************************************************************************************/


void preparefile(STRPTR buf, LONG size)
{
    STRPTR end = buf + size;

    while (buf < end)
    {
	/* Convert comments to spaces */
	if (buf + 1 < end && *buf == '/' && buf[1] == '*')
	{
	    *buf++ = ' ';
	    *buf++ = ' ';

	    while (buf < end)
	    {
		if (*buf == '*')
		{
		    *buf++ = ' ';

		    if (buf >= end)
		    {
			break;
		    }

		    if (*buf == '/')
		    {
			*buf++ = ' ';
			break;
		    }
		}
		else
		{
		    *buf++=' ';
		}
	    }

	    continue;
	}

	/* Skip strings */
	if (*buf=='\"')
	{
	    /*
	     * skip first
	     */
	    buf++;
	    while (buf < end && *buf != '\"')
	    {
		buf++;
	    }
	    /*
	     * skip last "
	     */
	    buf++;
	    continue;
	}

	/* Convert '\n' and ';' to spaces */
	if (*buf == '\n' || *buf == ';')
	{
	    *buf++ = ' ';
	    continue;
	}

	/* Convert '#' to \n */
	if (*buf == '#')
	{
	    *buf++ = '\n';
	    continue;
	}

	/* Skip all other characters */
	buf++;
    }
}

/************************************************************************************************/
/************************************************************************************************/

struct FileSysEntry *GetFileSysEntry(ULONG DosType)
{

  struct FileSysResource *MyFileSysRes;
  struct FileSysEntry *MyFileSysEntry;
  struct FileSysEntry *CurrentFileSysEntry;

  MyFileSysEntry = NULL;
  MyFileSysRes = OpenResource(FSRNAME);
  if (MyFileSysRes)
  {
    Forbid();
    CurrentFileSysEntry	= (struct FileSysEntry*) MyFileSysRes->fsr_FileSysEntries.lh_Head;
    while (CurrentFileSysEntry->fse_Node.ln_Succ)
    {
      if (CurrentFileSysEntry->fse_DosType == DosType)
      {
        if (MyFileSysEntry)
        {
          if (CurrentFileSysEntry->fse_Version > MyFileSysEntry->fse_Version)
          {
            MyFileSysEntry	=	CurrentFileSysEntry;
          }
        }
        else
        {
          MyFileSysEntry	=	CurrentFileSysEntry;
        }
      }
      CurrentFileSysEntry	=(struct FileSysEntry*) CurrentFileSysEntry->fse_Node.ln_Succ;
    }
    Permit();
  }
  return MyFileSysEntry;
}

/************************************************************************************************/
/************************************************************************************************/


void	PatchDosNode(struct DeviceNode	*MyDeviceNode,
                     ULONG		DosType)
{
struct FileSysEntry	*MyFileSysEntry;
ULONG			MyPatchFlags;

  DEBUG_PATCHDOSNODE(Printf("MakeDosNode: DeviceNode 0x%lx\n",
                    (ULONG)MyDeviceNode));

  if ((MyFileSysEntry=GetFileSysEntry(DosType)))
  {
    MyPatchFlags		=	MyFileSysEntry->fse_PatchFlags;

    DEBUG_PATCHDOSNODE(Printf("PatchDosNode: FileSysEntry 0x%lx PatchFlags 0x%lx\n",
                      (ULONG)MyFileSysEntry,
                      MyPatchFlags));

    if (MyPatchFlags)
    {
      ULONG	*PatchDeviceNode;
      ULONG	*PatchDeviceNodeEnd;
      ULONG	*PatchFileSysEntry;

      PatchFileSysEntry		=(ULONG*) &MyFileSysEntry->fse_Type;
      PatchDeviceNode		=(ULONG*) &MyDeviceNode->dn_Type;
      PatchDeviceNodeEnd	=(ULONG*) ((ULONG) MyDeviceNode + sizeof(struct DeviceNode));

      while (MyPatchFlags)
      {
        if (MyPatchFlags & 1)
        {
          *PatchDeviceNode	=	*PatchFileSysEntry;
        }
        PatchDeviceNode++;
        PatchFileSysEntry++;

        if (PatchDeviceNode >= PatchDeviceNodeEnd)
        {
          /* Savety */
          break;
        }

        MyPatchFlags	>>=	1;
      }
    }
  }
  else
  {
    /* Mount with no BootNode */
    DEBUG_PATCHDOSNODE(Printf("PatchDosNode: Can't get FileSysEntry..no bootnode\n"));
  }
}


/************************************************************************************************/
/************************************************************************************************/



#define	DOSNAME_INDEX	 0
#define	EXECNAME_INDEX	 1
#define	UNIT_INDEX	 2
#define	FLAGS_INDEX	 3
#define	ENVIROMENT_INDEX 4

struct DeviceNode *MyMakeDosNode(char *DosName, IPTR *ParameterPkt, char *StartupName)
{
  int DosNameSize;
  int ExecNameSize;
  int MyEnvSize = 0;
  struct DeviceNode *MyDeviceNode = NULL;
  struct FileSysStartupMsg *MyFileSysStartupMsg = NULL;
  struct DosEnvec *MyDosEnvec = NULL;
  char *MyString = NULL;
  ULONG Status = FALSE;
  DEBUG_MAKEDOSNODE(Printf("MakeDosNode: Pkt 0x%lx\n",(IPTR)ParameterPkt));

  if (ParameterPkt)
  {
    DEBUG_MAKEDOSNODE(Printf("MakeDosNode: DosName <%s> DeviceName <%s> Unit ", DosName, ParameterPkt[EXECNAME_INDEX]));
    DEBUG_MAKEDOSNODE(if (UnitString)
			     Printf("<%s>",ParameterPkt[UNIT_INDEX]);
                      else
			     Printf("%ld",ParameterPkt[UNIT_INDEX]);)
    DEBUG_MAKEDOSNODE(Printf(" Flags 0x%lx DE_TABLESIZE 0x%lx\n", ParameterPkt[FLAGS_INDEX], ParameterPkt[ENVIROMENT_INDEX]));
  }
  else
  {
    DEBUG_MAKEDOSNODE(Printf("MakeDosNode: DosName <%s> Startup <%s>\n", (IPTR)DosName, (IPTR)StartupName));
  }

  DosNameSize =	strlen(DosName);

  if (ParameterPkt)
  {
    if (ParameterPkt[EXECNAME_INDEX])
    {
      ExecNameSize = strlen((UBYTE *)ParameterPkt[EXECNAME_INDEX]);
    }
    else
    {
      ExecNameSize = 0;
    }
    MyEnvSize =	(ParameterPkt[ENVIROMENT_INDEX] + 1) * sizeof(IPTR);
  }
  else
  {
    ExecNameSize = StartupName ? strlen(StartupName) : 0;
  }

  if ((MyDeviceNode = AllocVec(sizeof(struct DeviceNode), MEMF_PUBLIC | MEMF_CLEAR)))
  {
    DEBUG_MAKEDOSNODE(Printf("MakeDosNode: MyDeviceNode 0x%lx\n", (ULONG)MyDeviceNode));

    MyDeviceNode->dn_StackSize = 600;
    MyDeviceNode->dn_Priority  = 10;

    if ((MyString=AllocVec(((DosNameSize + BSTR_EXTRA + 4) & ~3) + ((ExecNameSize + BSTR_EXTRA + 4) & ~3), MEMF_PUBLIC | MEMF_CLEAR)))
    {
      bstrcpy(MyString, DosName, DosNameSize);

      MyDeviceNode->dn_Name = MKBADDR(MyString);

      if (ParameterPkt)
      {
	if ((MyFileSysStartupMsg = AllocVec(sizeof(struct FileSysStartupMsg), MEMF_PUBLIC | MEMF_CLEAR)))
        {
	  DEBUG_MAKEDOSNODE(Printf("MakeDosNode: MyFileSysStartupMsg 0x%lx\n", (IPTR)MyFileSysStartupMsg));

	  if ((MyDosEnvec = AllocVec(MyEnvSize, MEMF_PUBLIC | MEMF_CLEAR)))
          {
	    char *ExecNamePtr;

	    DEBUG_MAKEDOSNODE(Printf("MakeDosNode: MyDosEnvec 0x%lx\n", (IPTR)MyDosEnvec));
	    ExecNamePtr	= &MyString[(1 + DosNameSize + BSTR_EXTRA + 3) & ~3];

            /* .device name must absolutely **NOT** include the 0 in the
             * length!!
             *
             * the string *MUST* be 0 terminated, however!
             */
            if (ParameterPkt[EXECNAME_INDEX])
            {
	      bstrcpy(ExecNamePtr, (UBYTE *)ParameterPkt[EXECNAME_INDEX], ExecNameSize);
            }
	    else
	    {
	      ExecNamePtr[0] = 0;
#ifndef AROS_FAST_BPTR
	      ExecNamePtr[1] = 0;
#endif
	    }
	    MyFileSysStartupMsg->fssm_Device  =	MKBADDR(ExecNamePtr);
	    MyFileSysStartupMsg->fssm_Unit    =	ParameterPkt[UNIT_INDEX];
	    MyFileSysStartupMsg->fssm_Flags   =	ParameterPkt[FLAGS_INDEX];
	    MyFileSysStartupMsg->fssm_Environ =	MKBADDR(MyDosEnvec);
	    MyDeviceNode->dn_Startup	      =	MKBADDR(MyFileSysStartupMsg);

	    CopyMem(&ParameterPkt[ENVIROMENT_INDEX], MyDosEnvec, MyEnvSize);

            Status=TRUE;
            DEBUG_MAKEDOSNODE(Printf("MakeDosNode: done\n"));
          }
        }
      }
      else
      {
	if (StartupName && ExecNameSize)
        {
          char	*StartupNamePtr;

	  StartupNamePtr = &MyString[(1 + DosNameSize + BSTR_EXTRA + 3) & ~3];
	  bstrcpy(StartupNamePtr, StartupName, ExecNameSize);
	  MyDeviceNode->dn_Startup = MKBADDR(StartupNamePtr);
        }
        Status=TRUE;
      }
    }
  }
  if (Status)
  {
    DEBUG_MAKEDOSNODE(Printf("MakeDosNode: done\n"));
    return MyDeviceNode;
  }
  else
  {
    DEBUG_MAKEDOSNODE(Printf("MakeDosNode: failed\n"));
    FreeVec(MyFileSysStartupMsg);
    FreeVec(MyDeviceNode);
    FreeVec(MyString);
    return NULL;
  }
}



/************************************************************************************************/
/************************************************************************************************/


LONG parsemountfile(IPTR *params, STRPTR buf, LONG size)
{
STRPTR args[NUM_ARGS];
LONG   error;
struct RDArgs rda;

    DEBUG_MOUNT(Printf("ParseMountFile:\n"));

    memset(&args, 0, sizeof(args));
    memset(&rda,0,sizeof(struct RDArgs));

    rda.RDA_Source.CS_Buffer = buf;
    rda.RDA_Source.CS_Length = size;
    rda.RDA_Source.CS_CurChr = 0;
    rda.RDA_Flags = RDAF_NOPROMPT;

    DEBUG_MOUNT(Printf("ReadArgs..\n%s\n\n",(ULONG)rda.RDA_Source.CS_Buffer));

    if ((error=ReadMountArgs(params,
                            &rda))!=RETURN_OK)
    {
	DEBUG_MOUNT(Printf("Parse: ReadArgs failed\n"));
    }
    return error;
}

/************************************************************************************************/
/************************************************************************************************/


LONG	parsemountlist(IPTR		*params,
                       STRPTR		name,
                       STRPTR		buf,
                       LONG		size)
{
STRPTR args[NUM_ARGS];
UBYTE  buffer[1024];
LONG   error=RETURN_OK, res;
STRPTR end = buf + size;
STRPTR s2;
char *ptr;
struct RDArgs rda;

    DEBUG_MOUNT(Printf("ParseMountList: <%s>\n",(ULONG)name));

    memset(&args,0,sizeof(args));
    memset(&rda,0,sizeof(struct RDArgs));

    rda.RDA_Source.CS_Buffer = buf;
    rda.RDA_Source.CS_Length = end - buf;
    rda.RDA_Source.CS_CurChr = 0;
    rda.RDA_Flags = RDAF_NOPROMPT;

    while (rda.RDA_Source.CS_CurChr < rda.RDA_Source.CS_Length)
    {
	res = ReadItem(buffer, sizeof(buffer), &rda.RDA_Source);

	DEBUG_MOUNT(Printf("ParseMountList: buffer <%s>\n",(ULONG)buffer));
	DEBUG_MOUNT(Printf("ParseMountList: ReadItem res %ld\n",res));

	if (res == ITEM_ERROR)
	{
	    return IoErr();
	}

	if (res == ITEM_NOTHING &&
	    rda.RDA_Source.CS_CurChr == rda.RDA_Source.CS_Length)
	{
	    return 0;
	}

	if (res != ITEM_QUOTED && res != ITEM_UNQUOTED)
	{
	    return 1;
	}

	s2 = buffer;

	while (*s2)
	{
	    s2++;
	}

	if (s2 == buffer || s2[-1] != ':')
	{
	    DEBUG_MOUNT(Printf("ParseMountList: failure\n"));
	    return ERR_DEVICENOTFOUND;
	}

	*--s2 = 0;

	if (!Strnicmp(name, buffer, s2 - buffer) &&
	   (!name[s2 - buffer] || (name[s2 - buffer] == ':' || !name[s2 - buffer + 1])))
	{
	    DEBUG_MOUNT(Printf("ParseMountList: found\n"));

	    /* Copy the string so we get proper case - Piru */
	    memcpy(name, buffer, s2 - buffer);
	    name[s2 - buffer] = '\0';

	    ptr = name;
	    while (*ptr)
	    {
		if (*ptr++ == ':')
		{
			ptr[-1] = '\0';
			break;
		}
	    }

	    DEBUG_MOUNT(Printf("ReadArgs..\n%s\n\n",(ULONG)&rda.RDA_Source.CS_Buffer[rda.RDA_Source.CS_CurChr]));

	    if ((error=ReadMountArgs(params,
                                     &rda))!=RETURN_OK)
	    {
		DEBUG_MOUNT(Printf("ParseMountList: ReadArgs failed\n"));
		//return IoErr();
	    }

	    return error;
	}

	while (rda.RDA_Source.CS_CurChr < rda.RDA_Source.CS_Length)
	{
	    if (rda.RDA_Source.CS_Buffer[rda.RDA_Source.CS_CurChr++] == '\n')
	    {
		DEBUG_MOUNT(Printf("ParseMountList: reach the end of the block\n"));
		break;
	    }
	}
    }

    DEBUG_MOUNT(Printf("ParseMountList: mount found nothing\n"));
    return ERR_DEVICENOTFOUND;
}

/************************************************************************************************/
/************************************************************************************************/

LONG checkmount(IPTR *params)
{
struct DosEnvec *vec;

	vec = (struct DosEnvec *)&params[4];

	params[1] = (IPTR) DeviceString;

        if (IsFilesystem && (!flagargs[ARG_DEVICE]
            || !flagargs[ARG_SURFACES] || !flagargs[ARG_BLOCKSPERTRACK]
            || !flagargs[ARG_LOWCYL] || !flagargs[ARG_HIGHCYL]))
	{
		ShowError("Could not find some of the following keywords:\n"
		   "       Surfaces, BlocksPerTrack, LowCyl, HighCyl, Device");
		return ERR_INVALIDKEYWORD;
	}
	/* bootpri -129 shouldn't be started and not automatic mounted..whatever that means */
	if ((vec->de_BootPri < -129) || (vec->de_BootPri > 127))
	{
	    ShowError("BootPri %ld is not allowed. Legal range is -128..127",vec->de_BootPri);
	    return ERROR_BAD_NUMBER;
	}

	if (flagargs[ARG_GLOBVEC])
	{
		if ((GlobalVec != -1) && (GlobalVec != -2))
		{
		    ShowError("Globvec %ld is not supported. Only -1 and -2 are supported here", GlobalVec);
		    return ERROR_BAD_NUMBER;
		}
	}

	if (flagargs[ARG_STARTUP] && !StartupString)
	{
		if (StartupValue >= 0x100)
		{
			ShowError("Startup uses a too large numerical number %ld",StartupValue);
			return ERROR_BAD_NUMBER;
		}
	}

	return RETURN_OK;
}

/************************************************************************************************/
/************************************************************************************************/

LONG mount(IPTR	*params, STRPTR	name)
{
    struct DosEnvec *vec;
    LONG error = RETURN_OK;
    struct DeviceNode *dn;

    strupr(name);
    DEBUG_MOUNT(Printf("MountDev: <%s>\n",(ULONG)name));

    if ((error=checkmount(params))!=RETURN_OK)
    {
	DEBUG_MOUNT(Printf("MountDev: checkmount failed\n"));
	return error;
    }

    vec = (struct DosEnvec *)&params[4];

    DEBUG_MOUNT(Printf("MountDev: DosName         <%s>\n",(ULONG)name));
    DEBUG_MOUNT(Printf("MountDev: Filesystem      <%s>\n",(ULONG)HandlerString + BSTR_OFFSET));
    DEBUG_MOUNT(Printf("MountDev: Device          <%s>\n",(ULONG)DeviceString));
    DEBUG_MOUNT(Printf("MountDev: TableSize       %ld\n",vec->de_TableSize));
    DEBUG_MOUNT(Printf("MountDev: SizeBlock       %ld\n",vec->de_SizeBlock));
    DEBUG_MOUNT(Printf("MountDev: SecOrg          %ld\n",vec->de_SecOrg));
    DEBUG_MOUNT(Printf("MountDev: Surfaces        %ld\n",vec->de_Surfaces));
    DEBUG_MOUNT(Printf("MountDev: SectorsPerBlock %ld\n",vec->de_SectorPerBlock));
    DEBUG_MOUNT(Printf("MountDev: BlocksPerTrack  %ld\n",vec->de_BlocksPerTrack));
    DEBUG_MOUNT(Printf("MountDev: Reserved        %ld\n",vec->de_Reserved));
    DEBUG_MOUNT(Printf("MountDev: PreAlloc        %ld\n",vec->de_PreAlloc));
    DEBUG_MOUNT(Printf("MountDev: Interleave      %ld\n",vec->de_Interleave));
    DEBUG_MOUNT(Printf("MountDev: LowCyl          %ld\n",vec->de_LowCyl));
    DEBUG_MOUNT(Printf("MountDev: UpperCyl        %ld\n",vec->de_HighCyl));
    DEBUG_MOUNT(Printf("MountDev: NumBuffers      %ld\n",vec->de_NumBuffers));
    DEBUG_MOUNT(if (vec->de_TableSize >= DE_BUFMEMTYPE))
    DEBUG_MOUNT(Printf("MountDev: BufMemType      0x%lx\n",vec->de_BufMemType));
    DEBUG_MOUNT(Printf("MountDev: MaxTransfer     0x%lx\n",vec->de_MaxTransfer));
    DEBUG_MOUNT(if (vec->de_TableSize >= DE_MASK))
    DEBUG_MOUNT(Printf("MountDev: Mask            0x%lx\n",vec->de_Mask));
    DEBUG_MOUNT(if (vec->de_TableSize >= DE_BOOTPRI))
    DEBUG_MOUNT(Printf("MountDev: BootPri         %ld\n",vec->de_BootPri));
    DEBUG_MOUNT(if (vec->de_TableSize >= DE_DOSTYPE))
    DEBUG_MOUNT(Printf("MountDev: DosType         0x%lx\n",vec->de_DosType));
    DEBUG_MOUNT(if (vec->de_TableSize >= DE_BAUD))
    DEBUG_MOUNT(Printf("MountDev: Baud            %ld\n",vec->de_Baud));
    DEBUG_MOUNT(if (vec->de_TableSize >= DE_CONTROL))
    DEBUG_MOUNT(Printf("MountDev: Control         0x%lx\n",vec->de_Control));
    DEBUG_MOUNT(if (vec->de_TableSize >= DE_BOOTBLOCKS))
    DEBUG_MOUNT(Printf("MountDev: BootBlocks      %ld\n",vec->de_BootBlocks));

    if ((dn=MyMakeDosNode(name, IsEHandler ? params : NULL, StartupString)))
    {
        DEBUG_MOUNT(Printf("MountDev: DeviceNode 0x%lx\n",(ULONG)dn));

	dn->dn_StackSize = StackSize;
	dn->dn_Priority	 = Priority;
	dn->dn_GlobalVec = (BPTR)GlobalVec;

	if (!IsEHandler && !StartupString)
        {
	    dn->dn_Startup = (BPTR)StartupValue;
        }

	if (IsFilesystem && ((ForceLoad==0) || (HandlerString==NULL)))
        {
	    DEBUG_MOUNT(Printf("MountDev: patchdosnode\n"));
	    PatchDosNode(dn,vec->de_DosType);
        }

        if (ForceLoad || dn->dn_SegList==NULL)
        {
	    DEBUG_MOUNT(Printf("MountDev: Load Handler\n"));
	    dn->dn_Handler = MKBADDR(HandlerString);
        }
        else
        {
	    /*
	     * We don't need the HandlerString anymore...free it
	     */
	    if (HandlerString)
	    {
	        FreeVec(HandlerString);
		HandlerString =	NULL;
	    }
        }
	DEBUG_MOUNT(Printf("MountDev: Name      %b\n",dn->dn_Name));
	DEBUG_MOUNT(Printf("MountDev: Handler   0x%lx <%b>\n",dn->dn_Handler,dn->dn_Handler));
	DEBUG_MOUNT(Printf("MountDev: SegList   0x%lx\n",dn->dn_SegList));
        DEBUG_MOUNT(Printf("MountDev: StackSize %ld\n",dn->dn_StackSize));
	DEBUG_MOUNT(Printf("MountDev: Priority  %ld\n",dn->dn_Priority));
	if (!IsEHandler && StartupString)
	DEBUG_MOUNT(Printf("MountDev: Startup   <%b>\n", dn->dn_Startup));
	else
	DEBUG_MOUNT(Printf("MountDev: Startup   0x%lx\n",dn->dn_Startup));
        DEBUG_MOUNT(Printf("MountDev: GlobalVec %ld\n",dn->dn_GlobalVec));

        if (dn->dn_SegList || dn->dn_Handler)
        {
	    if (AddDosEntry((struct DosList *)dn))
	    {
		    DEBUG_MOUNT(Printf("MountDev: AddDosEntry worked\n"));
		    /*
		     * Don't free these anymore as they belong to the dosnode
		     */
		    HandlerString = NULL;
		    if (IsEHandler)
		    {
			UnitString = NULL;
			FlagsString = NULL;
			ControlString = NULL;
		    }
		    if (Activate)
		    {
			DEBUG_MOUNT(Printf("Activating\n"));
			DeviceProc(name);
		    }
		    error = 0;
	    }
	    else
	    {
		    DEBUG_MOUNT(Printf("MountDev: AddDosEntry failed\n"));
		    error = ERROR_INVALID_RESIDENT_LIBRARY;
		    if (HandlerString)
		    {
			    FreeVec(HandlerString);
		    }
	    }
        }
        else
        {
	        DEBUG_MOUNT(Printf("MountDev: no loadseg and no handler specified\n"));
	        error = ERROR_OBJECT_NOT_FOUND;
        }
    }
    else
    {
        error = ERROR_NO_FREE_STORE;
    }

    return error;
}

void ShowError(char *s, ...)
{
	va_list ap;

	va_start(ap, s);
	if (IsCli)
	{
		PutStr("ERROR: ");
		VPrintf(s, ARGS(ap));
		PutStr("\n");
	}
	else
	{
		struct EasyStruct es =
		{
			sizeof(struct EasyStruct),
			0,
			"Mount Failure",
			s,
			"OK"
		};

		IntuitionBase = (struct IntuitionBase *)OpenLibrary("intuition.library", 36);
		if (IntuitionBase)
		{
			EasyRequestArgs(NULL, &es, NULL, ARGS(ap));
			CloseLibrary((struct Library *)IntuitionBase);
		}
	}
	va_end(ap);
}

void ShowFault(LONG code, char *s, ...)
{
	char buf[256];
	va_list ap;
	int l;

	va_start(ap, s);
	l = vsnprintf(buf, sizeof(buf) - 2, s, ap);
	va_end(ap);
	strcpy(&buf[l], ": ");
	l += 2;
	Fault(code, NULL, &buf[l], sizeof(buf) - l);
	if (IsCli)
	{
		PutStr(buf);
		PutStr("\n");
	}
	else
	{
		struct EasyStruct es =
		{
			sizeof(struct EasyStruct),
			0,
			"Mount Failure",
			buf,
			"OK"
		};

		IntuitionBase = (struct IntuitionBase *)OpenLibrary("intuition.library", 36);
		if (IntuitionBase)
		{
			EasyRequestArgs(NULL, &es, NULL, NULL);
			CloseLibrary((struct Library *)IntuitionBase);
		}
	}
}

