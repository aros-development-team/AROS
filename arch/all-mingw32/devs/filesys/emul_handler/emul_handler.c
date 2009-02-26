/*
 Copyright  1995-2009, The AROS Development Team. All rights reserved.
 $Id$
 
 Desc: Filesystem that accesses an underlying Windows filesystem.
 Lang: english

 Please always update the version-string below, if you modify the code!
 */

/*********************************************************************************************/

#define DEBUG 0
#define DERROR(x)
#define DASYNC(x)

#define __DOS_NOLIBBASE__
#include <aros/debug.h>
#include <aros/kernel.h>
#include <aros/system.h>
#include <aros/symbolsets.h>
#include <exec/resident.h>
#include <exec/memory.h>
#include <exec/alerts.h>
#include <devices/input.h>
#include <devices/inputevent.h>
#include <proto/exec.h>
#include <utility/tagitem.h>
#include <utility/hooks.h>
#include <dos/filesystem.h>
#include <dos/exall.h>
#include <dos/dosasl.h>
#include <dos/bptr.h>
#include <proto/dos.h>
#include <proto/arossupport.h>
#include <proto/expansion.h>
#include <proto/hostlib.h>
#include <proto/kernel.h>
#include <libraries/expansion.h>
#include <libraries/configvars.h>
#include <libraries/expansionbase.h>

#include <aros/host-conf.h>

#include <string.h>

#include "emul_handler_intern.h"

#include <string.h>

#include LC_LIBDEFS_FILE

/*********************************************************************************************/

/* Init DOSBase ourselves because emul_handler is initialized before dos.library */
static struct DosLibrary *DOSBase;
static APTR HostLibBase, KernelBase;
static struct EmulInterface *EmulIFace;
static struct KernelInterface *KernelIFace;

/*********************************** Support *******************************/

static void SendEvent(struct emulbase *emulbase, LONG event) {
  struct IOStdReq *InputRequest;
  struct MsgPort *InputPort;
  struct InputEvent *ie;
  D(bug("[emul] SendEvent\n"));
  if ((InputPort = (struct MsgPort*)CreateMsgPort())) {
	
	if ((InputRequest = (struct IOStdReq*)CreateIORequest(InputPort, sizeof(struct IOStdReq)))) {
	  
	  if (!OpenDevice("input.device", 0, (struct IORequest*)InputRequest, 0)) {
		
		if ((ie = AllocVec(sizeof(struct InputEvent), MEMF_PUBLIC|MEMF_CLEAR))) {
		  ie->ie_Class = event;
		  InputRequest->io_Command = IND_WRITEEVENT;
		  InputRequest->io_Data = ie;
		  InputRequest->io_Length = sizeof(struct InputEvent);
		  
		  DoIO((struct IORequest*)InputRequest);
		  
		  FreeVec(ie);
		}
		CloseDevice((struct IORequest*)InputRequest);
	  }
	  DeleteIORequest ((APTR)InputRequest);
	}
	DeleteMsgPort (InputPort);
  }
}

/*********************************************************************************************/

static APTR emul_malloc(struct emulbase *emulbase, ULONG size)
{
  ULONG *res;
  
  // kprintf("** emul_malloc: size = %d **\n",size);
  ObtainSemaphore(&emulbase->memsem);
  
  size += sizeof(ULONG);
  res = AllocPooled(emulbase->mempool, size);
  if (res) *res++ = size;
  
  ReleaseSemaphore(&emulbase->memsem);
  
  // kprintf("** emul_malloc: size = %d  result = %x **\n",size-sizeof(ULONG),res);
  
  return res;
}

/*********************************************************************************************/

static void emul_free(struct emulbase *emulbase, APTR mem)
{
  if (mem)
  {
	ULONG *m = (ULONG *)mem;
	ULONG size = *--m;
	
	// kprintf("** emul_free: size = %d  memory = %x **\n",size-sizeof(ULONG),mem);
	
	ObtainSemaphore(&emulbase->memsem);
	FreePooled(emulbase->mempool, m, size);
	ReleaseSemaphore(&emulbase->memsem);
  }
  	D(else kprintf("*** emul_handler: tried to free NULL mem ***\n");)
}

/*********************************************************************************************/

/* Create a plain path out of the supplied filename.
 Eg 'path1\path2\\path3\' becomes 'path1\path3'.
 */
BOOL shrink(struct emulbase *emulbase, char *filename)
{
  char *s, *s1,*s2;
  unsigned long len;

  /* We skip the first slash because it separates volume root prefix and the actual pathname */
  s = filename;
  if (*s == '\\')
      *s++;
  
  for(;;)
  {
	/* leading slashes? --> return FALSE. */
	if(*s == '\\') return FALSE;
	
	/* remove superflous paths (ie paths that are followed by '\') */
	s1 = strstr(s, "\\\\");
	if(s1==NULL)
	  break;
	s2=s1;
	while(s2 > s)
	{
	  if (s2[-1] == '\\') break;
	  s2--;
	}
	
	memmove(s2,s1+2,strlen(s1+1));
  }
  
  /* strip trailing slash */
  len = strlen(filename);
  if (len && (filename[len-1] == '\\'))
      filename[len-1]=0;

  return TRUE;
}

/*********************************************************************************************/

/* Allocate a buffer, in which the filename is appended to the pathname. */
static LONG makefilename(struct emulbase *emulbase, char **dest, char **part, struct filehandle * fh, STRPTR filename)
{
  LONG ret = 0;
  int len, flen, dirlen;
  char *c, *s;

  D(bug("[emul] makefilename(): directory \"%s\", file \"%s\")\n", fh->hostname, filename));
  dirlen = strlen(fh->hostname);
  flen = strlen(filename);
  len = flen + dirlen + 2;
  *dest=(char *)emul_malloc(emulbase, len);
  if ((*dest))
  {
	CopyMem(fh->hostname, *dest, dirlen);
	c = *dest + dirlen;
	if (flen) {
            *c++ = '\\';
            /* We are on Windows, so we have to revert slashes while copying filename */
            for (s = filename; *s; s++)
                *c++ = (*s == '/') ? '\\' : *s;
	}
	*c = 0;

	c = *dest + (fh->name - fh->hostname);
	D(bug("[emul] Shrinking filename: \"%s\"\n", c));
	if (!shrink(emulbase, c))
	{
	  emul_free(emulbase, *dest);
	  *dest = NULL;
	  ret = ERROR_OBJECT_NOT_FOUND;
	} else {
	  D(bug("[emul] resulting host filename: \"%s\"\n", *dest));
	  if (part) {
	      *part = c;
	      D(bug("[emul] resulting AROS filename: \"%s\"\n", c));
	  }
	}
  } else
	ret = ERROR_NO_FREE_STORE;
  return ret;
}

/*********************************************************************************************/

/* Make Windows protection bits out of AROS protection bits. */
ULONG prot_a2w(ULONG protect)
{
  ULONG uprot = 0;
  
  /* The following flags are low-active! */
  if ((protect & (FIBF_WRITE|FIBF_DELETE)) == (FIBF_WRITE|FIBF_DELETE))
	uprot = FILE_ATTRIBUTE_READONLY;
  /* The following flags are high-active again. */
  if (protect & FIBF_ARCHIVE)
      	uprot |= FILE_ATTRIBUTE_ARCHIVE;
  if (protect & FIBF_SCRIPT)
	uprot |= FILE_ATTRIBUTE_SYSTEM;

  /* TODO: 1) Support more NT-specific attributes ('Executable')
           2) Write complete AROS protection bits support using NTFS streams */
  
  return uprot;
}

/*********************************************************************************************/

/* Make AROS protection bits out of Windows protection bits. */
ULONG prot_w2a(ULONG protect)
{
  ULONG uprot = 0;
  
  /* The following three flags are low-active! */
  if (protect & FILE_ATTRIBUTE_READONLY)
      uprot = FIBF_WRITE|FIBF_DELETE;
  if (protect & FILE_ATTRIBUTE_DIRECTORY)
      uprot |= FIBF_EXECUTE;
  /* The following flags are high-active again. */
  if (protect & FILE_ATTRIBUTE_ARCHIVE)
      uprot |= FIBF_ARCHIVE;
  if (protect & FILE_ATTRIBUTE_SYSTEM)
      uprot |= FIBF_SCRIPT;

  /* TODO: 1) Support more NT-specific attributes ('Executable')
           2) Write complete AROS protection bits support using NTFS streams */

  return uprot;
}

/*********************************************************************************************/

static void FileTime2DateStamp(struct DateStamp *ds, UQUAD ft)
{
    UQUAD totalmins;

    /* Adjust from 01.01.1601 to 01.01.1978. This offset was calculated using a specially written program
       which puts "01.01.1978 00:00:00" into SYSTEMTIME structure and converts it into FILETIME. */
    ft -= 118969344000000000LL;
    /* Adjust interval from 100 ns to 1/50 sec */
    ft /= 200000;
    totalmins = ft / (60*50);
    ds->ds_Days = totalmins / (24*60);
    ds->ds_Minute = totalmins % (24*60);
    ds->ds_Tick = ft % (60*50);
}

/*********************************************************************************************/

/* Make an AROS error-code (<dos/dos.h>) out of a Windows error-code. */
static ULONG u2a[][2]=
{
  { ERROR_PATH_NOT_FOUND, ERROR_OBJECT_NOT_FOUND },
  { ERROR_ACCESS_DENIED, ERROR_OBJECT_WRONG_TYPE },
  { ERROR_NO_MORE_FILES, ERROR_NO_MORE_ENTRIES },
  { ERROR_NOT_ENOUGH_MEMORY, ERROR_NO_FREE_STORE },
  { ERROR_FILE_NOT_FOUND, ERROR_OBJECT_NOT_FOUND },
  { ERROR_FILE_EXISTS, ERROR_OBJECT_EXISTS },
  { ERROR_WRITE_PROTECT, ERROR_WRITE_PROTECTED },
  { WIN32_ERROR_DISK_FULL, ERROR_DISK_FULL },
  { ERROR_DIR_NOT_EMPTY, ERROR_DIRECTORY_NOT_EMPTY },
  { ERROR_SHARING_VIOLATION, ERROR_OBJECT_IN_USE },
  { ERROR_LOCK_VIOLATION, ERROR_OBJECT_IN_USE },
  { WIN32_ERROR_BUFFER_OVERFLOW, ERROR_OBJECT_TOO_LARGE },
  { ERROR_INVALID_NAME, ERROR_OBJECT_NOT_FOUND },
  { 0, 0 }
};

ULONG Errno_w2a(ULONG e)
{
  ULONG i;
  
  DERROR(printf("[EmulHandler] Windows error code: %lu\n", e));
  for(i=0;i<sizeof(u2a)/sizeof(u2a[0]);i++)
	if(u2a[i][0]==e) {
	  DERROR(printf("[EmulHandler] Translated to AROS error code: %lu\n", u2a[i][1]));
	  return u2a[i][1];
	}
  DERROR(printf("[EmulHandler] Unknown error code\n"));
  return ERROR_UNKNOWN;
}

#define Errno() Errno_w2a(GetLastError())

/*********************************************************************************************/

void *DoOpen(char *path, int mode, int protect)
{
  ULONG flags = 0;
  ULONG lock;
  ULONG create;
  void *res;
  
  D(bug("[emul] DoOpen(): mode 0x%08lX\n", mode));
  if (mode & FMF_WRITE)
      flags = GENERIC_WRITE;
  if (mode & FMF_READ)
      flags |= GENERIC_READ;
  /* FILE_SHARE_WRITE looks strange here, however without it i can't reopen file which
     is already open with MODE_OLDFILE, even just for reading with FMF_READ */
  lock = (mode & FMF_LOCK) ? 0 : FILE_SHARE_READ|FILE_SHARE_WRITE;
  if (mode & FMF_CREATE)
      create = (flags & FMF_CLEAR) ? CREATE_ALWAYS : OPEN_ALWAYS;
  else
      create = (flags & FMF_CLEAR) ? TRUNCATE_EXISTING : OPEN_EXISTING;
  D(bug("[emul] CreateFile: name \"%s\", flags 0x%08lX, lock 0x%08lX, create %lu\n", path, flags, lock, create));
  Forbid();
  res = OpenFile(path, flags, lock, NULL, create, prot_a2w(protect), NULL);
  Permit();
  DB2(bug("[emul] FileHandle = 0x%08lX\n", res));
  return res;
}

/*********************************************************************************************/

/* Free a filehandle */
static LONG free_lock(struct emulbase *emulbase, struct filehandle *current)
{
    D(bug("[emul] Lock type = %lu\n", current->type));
    switch(current->type)
    {
	case FHD_FILE:
	  if((current->fd != emulbase->stdin_handle) && (current->fd != emulbase->stdout_handle) &&
	     (current->fd != emulbase->stderr_handle))
	  {
	        DB2(bug("[emul] CloseHandle(), fd = 0x%08lX\n", current->fd));
	        Forbid();
		DoClose(current->fd);
		Permit();
	  }
	  break;
	case FHD_DIRECTORY:
	  if (current->fd != INVALID_HANDLE_VALUE)
	  {
	        D(bug("[emul] Closing directory search handle\n"));
	        Forbid();
		FindEnd(current->fd);
		Permit();
	  }
	  break;
    }
    if (current->pathname) {
	D(bug("[emul] Freeing pathname: \"%s\"\n", current->pathname));
	emul_free(emulbase, current->pathname);
    }
    D(bug("[emul] Freeing name: \"%s\"\n", current->hostname));
    emul_free(emulbase, current->hostname);
    D(bug("[emul] Freeing filehandle\n"));
    FreeMem(current, sizeof(struct filehandle));
    D(bug("[emul] Done\n"));
    return 0;
}

/*********************************************************************************************/

static LONG open_(struct emulbase *emulbase, struct filehandle **handle, STRPTR name, LONG mode, LONG protect, BOOL AllowDir)
{
  LONG ret = 0;
  struct filehandle *fh;
  
  fh=(struct filehandle *)AllocMem(sizeof(struct filehandle), MEMF_PUBLIC);
  if(fh!=NULL)
  {
	fh->pathname = NULL; /* just to make sure... */
	fh->dl = (*handle)->dl;
	/* If no filename is given and the file-descriptor is one of the
	 standard filehandles (stdin, stdout, stderr) ... */
	if((!name[0]) && ((*handle)->type == FHD_FILE) &&
	   (((*handle)->fd == emulbase->stdin_handle) || ((*handle)->fd == emulbase->stdout_handle) || ((*handle)->fd == emulbase->stderr_handle)))
	{
	  /* ... then just reopen that standard filehandle. */
	  fh->type       = FHD_FILE;
	  fh->fd         = (*handle)->fd;
	  fh->name       = NULL;
	  fh->hostname   = NULL;
	  fh->volumename = NULL;
	  *handle = fh;
	  return 0;
	}

	fh->volumename=(*handle)->volumename;
	
	ret = makefilename(emulbase, &fh->hostname, &fh->name, *handle, name);
	if (!ret)
	{
	    int kind = Stat(fh->hostname, NULL);

	    D(bug("[emul] object type: %ld\n", kind));
	    switch (kind) {
	    case ST_SOFTLINK:
	        ret = ERROR_IS_SOFT_LINK;
	        break;
	    case 0: /* Non-existing objects can be files opened for writing */
	    case ST_FILE:
		fh->type=FHD_FILE;
		fh->fd = DoOpen(fh->hostname, mode, protect);
		if(fh->fd != INVALID_HANDLE_VALUE)
		{
		    *handle=fh;
		    return 0;
		}
		ret = Errno();
		break;
	    case ST_USERDIR:
		/* file is a directory */
		if (AllowDir) {
		    fh->type   = FHD_DIRECTORY;
		    fh->fd     = INVALID_HANDLE_VALUE;
		    fh->dirpos = 0;
		    *handle=fh;
		    return 0;
		}
		ret = ERROR_OBJECT_WRONG_TYPE;
		break;
	    default:
		ret = ERROR_OBJECT_WRONG_TYPE;
	    }
	    D(bug("[emul] Freeing pathname\n"));
	    emul_free(emulbase, fh->hostname);
	}
	D(bug("[emul] Freeing filehandle\n"));
	FreeMem(fh, sizeof(struct filehandle));
  } else
	ret = ERROR_NO_FREE_STORE;
  D(bug("[emul] open_() returns %lu\n", ret));
  return ret;
}

/*********************************************************************************************/

static LONG create_dir(struct emulbase *emulbase, struct filehandle **handle,
					   STRPTR filename, IPTR protect)
{
  LONG ret = 0;
  struct filehandle *fh;
  
  fh = (struct filehandle *)AllocMem(sizeof(struct filehandle), MEMF_PUBLIC);
  if (fh)
  {
	fh->pathname   = NULL; /* just to make sure... */
	fh->type       = FHD_DIRECTORY;
	fh->fd         = INVALID_HANDLE_VALUE;
	fh->dirpos     = 0;
	fh->volumename = (*handle)->volumename;
	fh->dl	       = (*handle)->dl;
	
	ret = makefilename(emulbase, &fh->hostname, &fh->name, *handle, filename);
	if (!ret)
	{
	    Forbid();
	    ret = MKDir(fh->hostname, NULL);
	    Permit();
	    if (ret) {
	        *handle = fh;
	        Forbid();
	        Chmod(fh->hostname, prot_a2w(protect));
	        Permit();
	        return 0;
	    }
	    ret = Errno();
	}
	free_lock(emulbase, fh);
  } else
	ret = ERROR_NO_FREE_STORE;
  
  return ret;
}

/*********************************************************************************************/

static LONG delete_object(struct emulbase *emulbase, struct filehandle* fh,
						  STRPTR file)
{
  LONG ret = 0;
  char *filename = NULL;
  
  ret = makefilename(emulbase, &filename, NULL, fh, file);
  if (!ret)
  {
	if (!Delete(filename))
	    ret = Errno();
	emul_free(emulbase, filename);
  }
  
  return ret;
}

/*********************************************************************************************/

static LONG set_protect(struct emulbase *emulbase, struct filehandle* fh,
						STRPTR file, ULONG aprot)
{
  LONG ret = 0;
  char *filename = NULL;
  
  if ((ret = makefilename(emulbase, &filename, NULL, fh, file)))
	return ret;

  Forbid();
  ret = Chmod(filename, prot_a2w(aprot));
  Permit();
  ret = ret ? 0 : Errno();
  
  emul_free(emulbase, filename);
  
  return ret;
}

/*********************************************************************************************/

static void EmulIntHandler(struct AsyncReaderControl *msg, void *d)
{
    DASYNC(bug("[emul] Interrupt on request 0x%p, task 0x%p, signal 0x%08lX\n", msg, msg->task, msg->sig));
    Signal(msg->task, msg->sig);
}

/*********************************************************************************************/
static LONG startup(struct emulbase *emulbase)
{
  struct Library *ExpansionBase;
  struct filehandle *fhi = NULL;
  struct filehandle *fho = NULL;
  struct filehandle *fhe = NULL;
  struct filehandle *fhv;
  struct DeviceNode *dlv, *dlv2;
  LONG ret = ERROR_NO_FREE_STORE;
  ULONG res;
  
  D(kprintf("[Emulhandler] startup\n"));
  ExpansionBase = OpenLibrary("expansion.library",0);
  if(ExpansionBase != NULL)
  {
    D(kprintf("[Emulhandler] startup: got ExpansionBase\n"));	  
    fhv=(struct filehandle *)AllocMem(sizeof(struct filehandle) + 256 + AROS_WORSTALIGN, MEMF_PUBLIC);
    if(fhv != NULL)
    {
	D(kprintf("[Emulhandler] startup allocated fhv\n"));
	fhv->hostname = (char *)(fhv + 1);
	fhv->type     = FHD_DIRECTORY;
	fhv->fd       = INVALID_HANDLE_VALUE;
	fhv->dirpos   = 0;
	fhv->pathname = NULL; /* just to make sure... */

	Forbid();
	res = GetCWD(256, fhv->hostname);
	Permit();
	if (res > 256)
	    res = 0;
	if(res)
	{
	    D(bug("[Emulhandler] startup got directory %s\n", fhv->hostname));
	    fhv->name = fhv->hostname + res;
#define DEVNAME "EMU"
#define VOLNAME "System"
			  
	    static const char *devname = DEVNAME;
	    static const char *volname = VOLNAME;
			  
	    fhv->volumename = VOLNAME;

	    Forbid();
	    emulbase->stdin_handle = GetStdFile(STD_INPUT_HANDLE);
	    emulbase->stdout_handle = GetStdFile(STD_OUTPUT_HANDLE);
	    emulbase->stderr_handle = GetStdFile(STD_ERROR_HANDLE);
	    Permit();
	    if (!emulbase->stdin_handle)
		emulbase->stdin_handle = INVALID_HANDLE_VALUE;
	    if (!emulbase->stdout_handle)
		emulbase->stdout_handle = INVALID_HANDLE_VALUE;
	    if (!emulbase->stderr_handle)
		emulbase->stderr_handle = INVALID_HANDLE_VALUE;

	    if (emulbase->stdin_handle != INVALID_HANDLE_VALUE) {
		fhi=(struct filehandle *)AllocMem(sizeof(struct filehandle), MEMF_PUBLIC|MEMF_CLEAR);
		if(fhi!=NULL)
		{
		    D(kprintf("[Emulhandler] allocated fhi\n"));
		    fhi->type	    = FHD_FILE;
		    fhi->fd	    = emulbase->stdin_handle;
		    emulbase->eb_stdin = fhi;
		}
	    }
	    if (emulbase->stdout_handle != INVALID_HANDLE_VALUE) {
		fho=(struct filehandle *)AllocMem(sizeof(struct filehandle), MEMF_PUBLIC|MEMF_CLEAR);
		if(fho!=NULL)
		{
		    D(kprintf("[Emulhandler] startup allocated fho\n"));
		    fho->type	    = FHD_FILE;
		    fho->fd	    = emulbase->stdout_handle;
		    emulbase->eb_stdout = fho;
		}
	    }
	    if (emulbase->stderr_handle != INVALID_HANDLE_VALUE) {
		fhe=(struct filehandle *)AllocMem(sizeof(struct filehandle), MEMF_PUBLIC|MEMF_CLEAR);
		if(fhe!=NULL)
		{
		    D(kprintf("[Emulhandler] startup allocated fhe\n"));
		    fhe->type	    = FHD_FILE;
		    fhe->fd	    = emulbase->stderr_handle;
		    emulbase->eb_stderr = fhe;
		}
	    }

	    ret = ERROR_NO_FREE_STORE;

	    D(bug("[Emulhandler] Creating console reader\n"));
	    emulbase->ConsoleReader = InitNative();
	    if (emulbase->ConsoleReader) {
		D(bug("[Emulhandler] Created console reader %p\n", emulbase->ConsoleReader));
	        emulbase->ConsoleInt = KrnAddExceptionHandler(1, EmulIntHandler, emulbase->ConsoleReader, NULL);
	        D(bug("[Emulhandler] Added console interrupt %p\n", emulbase->ConsoleReader));
	        if (emulbase->ConsoleInt) {

		    /*
		       Allocate space for the string from same mem,
		       Use AROS_BSTR_MEMSIZE4LEN macro for space to
		       to allocate and add an extra 4 for alignment
		       purposes.
		    */

		    dlv = AllocMem(sizeof(struct DeviceNode) + 4 + AROS_BSTR_MEMSIZE4LEN(strlen(DEVNAME)), MEMF_CLEAR|MEMF_PUBLIC);
		    if (dlv) {
			dlv2 = AllocMem(sizeof(struct DeviceNode) + 4 + AROS_BSTR_MEMSIZE4LEN(strlen(VOLNAME)), MEMF_CLEAR|MEMF_PUBLIC);
			if(dlv2 != NULL)
			{
			    BSTR s;
			    BSTR s2;
			    WORD   i;

			    D(kprintf("[Emulhandler] startup allocated dlv/dlv2\n"));
			    /* We want s to point to the first 4-byte
			       aligned memory after the structure.
			     */
			    s = (BSTR)MKBADDR(((IPTR)dlv + sizeof(struct DeviceNode) + 3) & ~3);
			    s2 = (BSTR)MKBADDR(((IPTR)dlv2 + sizeof(struct DeviceNode) + 3) & ~3);
					
			    for(i = 0; i < sizeof(DEVNAME) - 1; i++)
			    {
				AROS_BSTR_putchar(s, i, devname[i]);
			    }
			    AROS_BSTR_setstrlen(s, sizeof(DEVNAME) - 1);
					
			    dlv->dn_Type    = DLT_DEVICE;
			    dlv->dn_Ext.dn_AROS.dn_Unit   = (struct Unit *)fhv;
			    dlv->dn_Ext.dn_AROS.dn_Device = &emulbase->device;
			    dlv->dn_Handler = NULL;
			    dlv->dn_Startup = NULL;
			    dlv->dn_Name    = s;
			    dlv->dn_Ext.dn_AROS.dn_DevName = AROS_BSTR_ADDR(dlv->dn_Name);
					
			    AddBootNode(5, 0, dlv, NULL);
					
					
			    /* Unfortunately, we cannot do the stuff below
			       as dos is not yet initialized... */
			    // AddDosEntry(MakeDosEntry("System", DLT_VOLUME));

			    for(i = 0; i < sizeof(VOLNAME) - 1; i++)
			    {
				AROS_BSTR_putchar(s2, i, volname[i]);
			    }
			    AROS_BSTR_setstrlen(s2, sizeof(VOLNAME) - 1);
					
			    dlv2->dn_Type   = DLT_VOLUME;
			    dlv2->dn_Ext.dn_AROS.dn_Unit   = (struct Unit *)fhv;
			    dlv2->dn_Ext.dn_AROS.dn_Device = &emulbase->device;
			    dlv2->dn_Handler = NULL;
			    dlv2->dn_Startup = NULL;
			    dlv2->dn_Name = s2;
			    dlv2->dn_Ext.dn_AROS.dn_DevName = AROS_BSTR_ADDR(dlv2->dn_Name);
					
			    /* Make sure this is not booted from */
			    AddBootNode(-128, 0, dlv2, NULL);
			    fhv->dl = dlv2;
			    
			    /* Increment our open counter because we use ourselves */
			    emulbase->device.dd_Library.lib_OpenCnt++;
			    return 0;
			}
			FreeMem(dlv, sizeof(struct DeviceNode) + 4 + AROS_BSTR_MEMSIZE4LEN(strlen(DEVNAME)));
		    }
		}
	    }
	    if (fhe)
		FreeMem(fhe, sizeof(struct filehandle));
	    if (fho)
		FreeMem(fho, sizeof(struct filehandle));
	    if (fhi)
		FreeMem(fhi, sizeof(struct filehandle));
	} /* valid directory */
	else
	{
	    Alert(AT_DeadEnd|AO_Unknown|AN_Unknown );
	}
	free_lock(emulbase, fhv);
    }
    CloseLibrary(ExpansionBase);
  }
  
  return ret;
}

/*********************************************************************************************/

static const ULONG sizes[]=
{ 0, offsetof(struct ExAllData,ed_Type), offsetof(struct ExAllData,ed_Size),
  offsetof(struct ExAllData,ed_Prot), offsetof(struct ExAllData,ed_Days),
  offsetof(struct ExAllData,ed_Comment), offsetof(struct ExAllData,ed_OwnerUID),
sizeof(struct ExAllData) };

/*********************************************************************************************/

/* Returns a emul_malloc()'ed buffer, containing a pathname, stripped by the
 filename.
 */
char *pathname_from_name (struct emulbase *emulbase, char *name)
{
  long len = strlen(name);
  long i = len;
  char *result = NULL;
  long c;

  /* look for the first '\' in the filename starting at the end */
  while (i != 0 && name[i] != '\\')
    i--;
  
  if (0 != i)
  {
    result = (char *)emul_malloc(emulbase, i + 1);
    if(!result)
      return NULL;

    for (c = 0; c < i; c++)
        result[c] = (name[c] == '\\') ? '/' : name[c];
    result[i]=0x0;
  }
  return result;
}

/*********************************************************************************************/

ULONG examine_start(struct emulbase *emulbase, struct filehandle *fh)
{
    char *c;
    ULONG len;

    if (fh->type != FHD_DIRECTORY)
	return ERROR_OBJECT_WRONG_TYPE;

    if (!fh->pathname) {
        len = strlen(fh->hostname);
        fh->pathname = emul_malloc(emulbase, len + 3);
        if (!fh->pathname)
            return ERROR_NO_FREE_STORE;
        CopyMem(fh->hostname, fh->pathname, len);
        c = fh->pathname + len;
        strcpy(c, "\\*");
    }
    D(bug("[emul] Created search path: %s\n", fh->pathname));
    return 0;
}

/*********************************************************************************************/

/* Resets dirpos in directory handle and close existing search handle */
static LONG CloseDir(struct filehandle *fh)
{
    ULONG r;

    if (fh->fd != INVALID_HANDLE_VALUE) {
        Forbid();
        r = FindEnd(fh->fd);
        Permit();
        if (!r)
            return Errno();
        fh->fd = INVALID_HANDLE_VALUE;
    }
    fh->dirpos = 0;
    return 0;
}

/*********************************************************************************************/

#define is_special_dir(x) (x[0] == '.' && (!x[1] || (x[1] == '.' && !x[2])))

/* Positions to dirpos in directory, retrieves next item in it and updates dirpos */
ULONG ReadDir(struct filehandle *fh, LPWIN32_FIND_DATA FindData, ULONG *dirpos)
{
  ULONG res;

  /*
   * Windows does not support positioning within directory. The only thing i can do is to
   * scan the directory in forward direction. In order to bypass this limitation we do the
   * following:
   * 1. Before starting we explicitly set current position (dirpos) to 0. Examine() will place
   *    it into our fib_DiskKey; in case of ExAll() this is eac_LastKey. We also maintain second
   *	directory position counter - in our directory handle. It reflects the real position of
   *	our file search handle.
   * 2. Here we compare position in dirpos with position in the handle. If dirpos is smaller than
   *    filehandle's counter, we have to rewind the directory. This is done by closing the search
   *    handle in order to be able to restart from the beginning and setting handle's counter to 0.
   */
  D(bug("[emul] Current dirpos %lu, requested %lu\n", fh->dirpos, *dirpos));
  if (fh->dirpos > *dirpos) {
      D(bug("[emul] Resetting search handle\n"));
      CloseDir(fh);
  }
  do
  {
  /* 
   * 3. Now we will scan the next directory entry until its index is greater than original index
   *    in dirpos. This means that we've repositioned and scanned the next entry. After this we
   *	update dirpos.
   */
      do {
          if (fh->fd == INVALID_HANDLE_VALUE) {
              D(bug("[emul] Finding first file\n"));
              Forbid();
              fh->fd = FindFirst(fh->pathname, FindData);
              Permit();
              res = (fh->fd != INVALID_HANDLE_VALUE);
          } else {
              Forbid();
              res = FindNext(fh->fd, FindData);
              Permit();
          }
          if (!res)
	      return Errno();
	  fh->dirpos++;
	  D(bug("[emul] Found %s, position %lu\n", FindData->cFileName, fh->dirpos));
      } while (fh->dirpos <= *dirpos);
      (*dirpos)++;
      D(bug("[emul] New dirpos: %lu\n", *dirpos));
  /*
   * We also skip "." and ".." entries (however we count their indexes - just in case), because
   * AmigaOS donesn't have them.
   */
  } while (is_special_dir(FindData->cFileName));

  return 0;
}

/*********************************************************************************************/

ULONG examine_entry_sub(struct emulbase *emulbase, struct filehandle *fh, STRPTR FoundName, WIN32_FILE_ATTRIBUTE_DATA *FIB, LONG *kind)
{
  STRPTR filename, name;
  ULONG plen, flen;
  ULONG error = 0;

  D(bug("[emul] examine_entry_sub(): filehandle's path: %s\n", fh->hostname));
  if (FoundName) {
      D(bug("[emul] ...containing object: %s\n", FoundName));
      plen = strlen(fh->hostname);
      flen = strlen(FoundName);
      name = emul_malloc(emulbase, plen + flen + 2);
      if (NULL == name)
	  return ERROR_NO_FREE_STORE;
      strcpy(name, fh->hostname);
      filename = name + plen;
      *filename++ = '\\';
      strcpy(filename, FoundName);
  } else
      name = fh->hostname;
  
  D(bug("[emul] Full name: %s\n", name));
  *kind = Stat(name, FIB);
  if (*kind == 0)
      error = Errno();
  if (FoundName) {
      D(bug("[emul] Freeing full name\n"));
      emul_free(emulbase, name);
  }
  return error;
}	

/*********************************************************************************************/

ULONG examine_entry(struct emulbase *emulbase, struct filehandle *fh, STRPTR FoundName,
		    struct ExAllData *ead, ULONG size, ULONG type)
{
  STRPTR next, last, end, name;
  WIN32_FILE_ATTRIBUTE_DATA FIB;
  LONG kind;
  ULONG error;

  /* Check, if the supplied buffer is large enough. */
  next=(STRPTR)ead+sizes[type];
  end =(STRPTR)ead+size;
  
  if(next>end) {
      D(bug("[emul] examine_entry(): end of buffer\n"));
      return ERROR_BUFFER_OVERFLOW;
  }

  error = examine_entry_sub(emulbase, fh, FoundName, &FIB, &kind);
  if (error)
      return error;

  D(bug("[emul] Filling in object information\n"));
  switch(type)
  {
	default:
	case ED_OWNER:
	  ead->ed_OwnerUID = 0;
	  ead->ed_OwnerGID = 0;
	case ED_COMMENT:
	  ead->ed_Comment = NULL;
	  /* TODO: Write Windows shell-compatible comments support using NTFS streams */
	case ED_DATE:
	  FileTime2DateStamp(&ead->ed_Days, FIB.ftLastWriteTime);
	case ED_PROTECTION:
	  ead->ed_Prot 	= prot_w2a(FIB.dwFileAttributes);
	case ED_SIZE:
	  ead->ed_Size	= FIB.nFileSizeLow;
	case ED_TYPE:
	  if ((kind == ST_USERDIR) && (!fh->name[0]))
	      kind = ST_ROOT;
	  ead->ed_Type = kind;
	case ED_NAME:
	  if (FoundName)
	      last = FoundName;
	  else if (*fh->name) {
	      name = fh->name;
	      last = name;
	      while(*name)
		if(*name++=='\\')
		  last = name;
	  } else
	      last = fh->volumename;
	  ead->ed_Name=next;
	  for(;;)
	  {
		if(next>=end)
		  return ERROR_BUFFER_OVERFLOW;
		if(!(*next++=*last++))
		  break;
	  }
	case 0:
	  ead->ed_Next=(struct ExAllData *)(((IPTR)next+AROS_PTRALIGN-1)&~(AROS_PTRALIGN-1));
	  
	  return 0;
  }
}

/*********************************************************************************************/

static LONG examine(struct emulbase *emulbase, struct filehandle *fh,
                    struct ExAllData *ead,
                    ULONG  size,
                    ULONG  type,
                    LONG  *dirpos)
{
  LONG error;

  /* Return an error, if supplied type is not supported. */
  if(type>ED_OWNER)
	return ERROR_BAD_NUMBER;

  /* Reset fh->dirpos to 0. If there is already a directory scan handle, it will be closed in order to start from the beginning */
  if (fh->type == FHD_DIRECTORY) {
      D(bug("[emul] examine(): Resetting search handle\n"));
      error = CloseDir(fh);
      if (error)
          return error;
  }
  *dirpos = 0;
  
  return examine_entry(emulbase, fh, NULL, ead, size, type);
}

/*********************************************************************************************/

static LONG examine_next(struct emulbase *emulbase,  struct filehandle *fh, struct FileInfoBlock *FIB)
{
  char *src, *dest;
  ULONG res;
  WIN32_FIND_DATA FindData;
  WIN32_FILE_ATTRIBUTE_DATA AttrData;

  res = examine_start(emulbase, fh);
  if (res)
      return res;

  res = ReadDir(fh, &FindData, &FIB->fib_DiskKey);
  if (!res)
      res = examine_entry_sub(emulbase, fh, FindData.cFileName, &AttrData, &FIB->fib_DirEntryType);
  if (res) {
      CloseDir(fh);
      return res;
  }

  FIB->fib_OwnerUID	  = 0;
  FIB->fib_OwnerGID	  = 0;
  FIB->fib_Comment[0]	  = '\0'; /* TODO: no comments available yet! */
  FIB->fib_Protection	  = prot_w2a(AttrData.dwFileAttributes);
  FIB->fib_Size           = AttrData.nFileSizeLow;
  FileTime2DateStamp(&FIB->fib_Date, AttrData.ftLastWriteTime);

  src  = FindData.cFileName;
  dest = FIB->fib_FileName;
  for (res = 0; res<MAXFILENAMELENGTH-1; res++)
  {
      if(! (*dest++=*src++) )
      {
	  break;
      }
  }

  return 0;
}

/*********************************************************************************************/

static LONG examine_all(struct emulbase *emulbase, struct filehandle *fh,
                        struct ExAllData *ead,
                        struct ExAllControl *eac,
                        ULONG  size,
                        ULONG  type)
{
  struct ExAllData *last = NULL;
  STRPTR end = (STRPTR)ead + size;
  LONG error;
  WIN32_FIND_DATA FindData;

  eac->eac_Entries = 0;
  error = examine_start(emulbase, fh);
  if (error)
      return error;

  for(;;)
  {
      error = ReadDir(fh, &FindData, &eac->eac_LastKey);
      if (error) {
          D(bug("[emul] ReadDir() returned %lu\n", error));
          break;
      }
      /* Try to match the filename, if required.  */
      D(bug("[emul] Checking against MatchString\n"));
      if (eac->eac_MatchString && !MatchPatternNoCase(eac->eac_MatchString, FindData.cFileName))
	  continue;
      D(bug("[emul] Examining object\n"));
      error = examine_entry(emulbase, fh, FindData.cFileName, ead, end-(STRPTR)ead, type);
      if(error)
	  break;
      /* Do some more matching... */
      if ((eac->eac_MatchFunc) && !CALLHOOKPKT(eac->eac_MatchFunc, ead, &type))
	  continue;
      eac->eac_Entries++;
      last = ead;
      ead = ead->ed_Next;
  }
  if (last!=NULL)
	last->ed_Next=NULL;
  if((error==ERROR_BUFFER_OVERFLOW)&&last!=NULL)
  {
	return 0;
  }
      
  return error;
}

/*********************************************************************************************/

static LONG create_hardlink(struct emulbase *emulbase, struct filehandle *handle, STRPTR name, struct filehandle *oldfile)
{
  LONG error;
  char *fn;
  
  if (!KernelIFace->CreateHardLink)
      return ERROR_ACTION_NOT_KNOWN;
  
  error = makefilename(emulbase, &fn, NULL, handle, name);
  if (!error)
  {
      D(bug("[emul] Creating hardlink %s to file %s\n", fn, oldfile->hostname));
      Forbid();
      error = Link(fn, oldfile->hostname, NULL);
      Permit();
      error = error ? 0 : Errno();
      emul_free(emulbase, fn);
  }

  return error;
}

/*********************************************************************************************/

static LONG create_softlink(struct emulbase * emulbase,
                            struct filehandle *handle, STRPTR name, STRPTR ref)
{
  LONG error=0L;
  char *src, *dest;
  
  /* TODO: implement symbolic links on earlier Windows versions using shell shortcuts */
  if (!KernelIFace->CreateSymbolicLink)
      return ERROR_ACTION_NOT_KNOWN;
  
  /* TODO: currently relative paths are converted to absolute one, this needs to be improved */
  error = makefilename(emulbase, &src, NULL, handle, name);
  if (!error)
  {
      error = makefilename(emulbase, &dest, NULL, handle, ref);
      if (!error) {
          Forbid();
	  error = SymLink(src, dest, 0);
	  Permit();
	  error = error ? 0 : Errno();
	  emul_free(emulbase, dest);
      }
      emul_free(emulbase, src);
  }
  
  return error;
}

/*********************************************************************************************/

static LONG rename_object(struct emulbase * emulbase,
						  struct filehandle *fh, STRPTR file, STRPTR newname)
{
  LONG ret = 0L;
  
  char *filename = NULL , *newfilename = NULL;
  
  ret = makefilename(emulbase, &filename, NULL, fh, file);
  if (!ret)
  {
	ret = makefilename(emulbase, &newfilename, NULL, fh, newname);
	if (!ret)
	{
	    Forbid();
	    ret = DoRename(filename,newfilename);
	    Permit();
	    ret = ret ? 0 : Errno();
	    emul_free(emulbase, newfilename);
	}
	emul_free(emulbase, filename);
  }
  
  return ret;
}


static LONG read_softlink(struct emulbase *emulbase,
                          struct filehandle *fh,
                          STRPTR buffer,
                          ULONG size)
{
/* TODO: implement symbolic links on pre-Vista using shell shortcuts
  
  if (DoReadLink(fh->name, buffer, size-1) == -1)
	return Errno();
  
  return 0L;*/
  return ERROR_ACTION_NOT_KNOWN;
}

/*********************************************************************************************/

ULONG parent_dir(struct emulbase *emulbase,
				 struct filehandle *fh,
				 char ** DirectoryName)
{
  *DirectoryName = pathname_from_name(emulbase, fh->name);
  D(bug("[emul] Parent directory: \"%s\"\n", *DirectoryName));
  return 0;
}

/*********************************************************************************************/

void parent_dir_post(struct emulbase *emulbase, char ** DirectoryName)
{
  /* free the previously allocated memory */
  emul_free(emulbase, *DirectoryName);
  **DirectoryName = 0;
}

/*********************************************************************************************/

/************************ Library entry points ************************/

int loadhooks(struct emulbase *emulbase);

static int GM_UNIQUENAME(Init)(LIBBASETYPEPTR emulbase)
{
  D(bug("Initializing emul_handler\n"));
  
  if (loadhooks(emulbase) != 0)
	  return FALSE;

  InitSemaphore(&emulbase->memsem);
  
  emulbase->mempool = CreatePool(MEMF_ANY, 4096, 2000);
  if (!emulbase->mempool) return FALSE;

  if(!startup(emulbase))
  {
	D(bug("emul_handler initialized OK\n"));
	return TRUE;
  }

  DeletePool(emulbase->mempool);
  
  D(bug("emul_handler startup failed\n"));
  
  return FALSE;
}

/*********************************************************************************************/

static BOOL new_volume(struct IOFileSys *iofs, struct emulbase *emulbase)
{
  struct filehandle 	*fhv;
  struct DosList  	*doslist;
  char    	    	*unixpath;
  
  /* Volume name and Unix path are encoded into DEVICE entry of
   MountList like this: <volumename>:<unixpath> */
  
  unixpath = iofs->io_Union.io_OpenDevice.io_DeviceName;
  unixpath = strchr(unixpath, ':');
  
  if (unixpath)
  {
	char *sp;
	
	*unixpath++ = '\0';
	
	if ((sp = strchr(unixpath, '~')))
	{
	  char home[260];
	  char *newunixpath = 0;
	  char *sp_end;
	  BOOL ok = FALSE;
	  WORD cmplen;
	  char tmp;
	  ULONG err;
	  
	  /* "~<name>" means home of user <name> */
		
	  for(sp_end = sp + 1;
		sp_end[0] != '\0' && sp_end[0] != '\\';
		sp_end++);

	  cmplen = sp_end - sp - 1;
	  /* temporariliy zero terminate name */
	  tmp = sp[cmplen+1]; sp[cmplen+1] = '\0';

	  err = GetHome(sp+1, home);
		
	  sp[cmplen+1] = tmp;
		
	  if (err)
	      err = Errno_w2a(err);
	  else {
		newunixpath = AllocVec(strlen(unixpath) + strlen(home) + 1, MEMF_CLEAR);
		if (newunixpath)
		{
		  strncpy(newunixpath, unixpath, sp - unixpath);
		  strcat(newunixpath, home);
		  strcat(newunixpath, sp_end);
		  
		  ok = TRUE;
		  unixpath = newunixpath;
		}
	  }
	  
	  if (!ok)
	  {
		unixpath = 0;
		if (newunixpath) FreeVec(newunixpath);
	  }
	}
	
	if (unixpath)
	{
	  if (Stat(unixpath, NULL)>0)
	  {
		fhv=(struct filehandle *)AllocMem(sizeof(struct filehandle), MEMF_PUBLIC);
		if (fhv != NULL)
		{
		  fhv->hostname   = unixpath;
		  fhv->name       = unixpath + strlen(unixpath);
		  fhv->type       = FHD_DIRECTORY;
		  fhv->pathname   = NULL; /* just to make sure... */
		  fhv->volumename = iofs->io_Union.io_OpenDevice.io_DeviceName;
		  
		  if ((doslist = MakeDosEntry(fhv->volumename, DLT_VOLUME)))
		  {
		      	fhv->dl = doslist;
			doslist->dol_Ext.dol_AROS.dol_Unit=(struct Unit *)fhv;
			doslist->dol_Ext.dol_AROS.dol_Device=&emulbase->device;
			AddDosEntry(doslist);
			
			iofs->IOFS.io_Unit   = (struct Unit *)fhv;
			iofs->IOFS.io_Device = &emulbase->device;
			
			SendEvent(emulbase, IECLASS_DISKINSERTED);
			
			return TRUE;
			
		  } /* if ((doslist = MakeDosEntry(fhv->volumename, DLT_VOLUME))) */
		  
		  FreeMem(fhv, sizeof(struct filehandle));
		  
		} /* if (fhv != NULL)*/
		
	  } /* if (!Stat(unixpath, &st)) */
	  
	} /* if (unixpath) */
	
  } /* if (unixpath) */
  return FALSE;
}

/*********************************************************************************************/

static int GM_UNIQUENAME(Open)
(
 LIBBASETYPEPTR emulbase,
 struct IOFileSys *iofs,
 ULONG unitnum,
 ULONG flags
)
{
  /* Keep compiler happy */
  unitnum=0;
  flags=0;
  
  if (DOSBase == NULL)
	DOSBase = (struct DosLibrary *)OpenLibrary("dos.library", 41);
  
  if (DOSBase == NULL || !new_volume(iofs, emulbase))
  {
	iofs->IOFS.io_Error = -1;
	return FALSE;
  }
  
  /* Set returncode */
  iofs->IOFS.io_Error=0;
  return TRUE;
}

/*********************************************************************************************/

ADD2INITLIB(GM_UNIQUENAME(Init), 0)
ADD2OPENDEV(GM_UNIQUENAME(Open), 0)

/*********************************************************************************************/

AROS_LH1(void, beginio,
		 AROS_LHA(struct IOFileSys *, iofs, A1),
		 struct emulbase *, emulbase, 5, emul_handler)
{
  AROS_LIBFUNC_INIT
  
  LONG error = 0;
  
  /* WaitIO will look into this */
  iofs->IOFS.io_Message.mn_Node.ln_Type=NT_MESSAGE;
  
  /*
   Do everything quick no matter what. This is possible
   because I never need to Wait().
   */
  switch(iofs->IOFS.io_Command)
  {
    case FSA_OPEN:
          D(bug("[emul] FSA_OPEN(\"%s\")\n", iofs->io_Union.io_OPEN.io_Filename));
	  error = open_(emulbase, (struct filehandle **)&iofs->IOFS.io_Unit,
			iofs->io_Union.io_OPEN.io_Filename, iofs->io_Union.io_OPEN.io_FileMode, 0, TRUE);
	  if (
		  (error == ERROR_WRITE_PROTECTED) &&
		  (iofs->io_Union.io_OPEN.io_FileMode & FMF_AMIGADOS)
		  )
	  {
	        D(bug("[emul] Retrying in read mode\n"));
		error = open_(emulbase, (struct filehandle **)&iofs->IOFS.io_Unit,
	                      iofs->io_Union.io_OPEN.io_Filename, iofs->io_Union.io_OPEN.io_FileMode & (~FMF_WRITE), 0, TRUE);
	  }
	  D(bug("[emul] FSA_OPEN returning %lu\n", error));
	  
	  break;
	  
	  case FSA_CLOSE:
	  D(bug("[emul] FSA_CLOSE\n"));
	  error = free_lock(emulbase, (struct filehandle *)iofs->IOFS.io_Unit);
	  D(bug("[emul] FSA_CLOSE returning %lu\n", error));
	  break;
	  
	  case FSA_READ:
	{
	  struct filehandle *fh = (struct filehandle *)iofs->IOFS.io_Unit;
	  
	  if (fh->type == FHD_FILE)
	  {
		if (fh->fd == emulbase->stdout_handle)
		    fh->fd = emulbase->stdin_handle;
		if (fh->fd == emulbase->stdin_handle) {
		    DASYNC(bug("[emul] Reading %lu bytes asynchronously \n", iofs->io_Union.io_READ.io_Length));
		    emulbase->ConsoleReader->fh = fh->fd;
		    emulbase->ConsoleReader->addr = iofs->io_Union.io_READ.io_Buffer;
		    emulbase->ConsoleReader->len = iofs->io_Union.io_READ.io_Length;
		    emulbase->ConsoleReader->sig = SIGF_DOS;
		    emulbase->ConsoleReader->task = FindTask(NULL);
		    emulbase->ConsoleReader->cmd = ASYNC_CMD_READ;
		    SetSignal(0, emulbase->ConsoleReader->sig);
		    if (RaiseEvent(emulbase->ConsoleReader->CmdEvent)) {
		    	Wait(emulbase->ConsoleReader->sig);
		    	DASYNC(bug("[emul] Read %ld bytes, error %lu\n", emulbase->EmulMsg.actual, emulbase->EmulMsg.error));
		    	iofs->io_Union.io_READ.io_Length = emulbase->ConsoleReader->actual;
		    	error = Errno_w2a(emulbase->ConsoleReader->error);
		    	if (!error) {
		            char *c, *d;

		            c = iofs->io_Union.io_READ.io_Buffer;
		            d = c;
		            while (*c) {
		            	if ((c[0] == '\r') && (c[1] == '\n')) {
		            	    c++;
		            	    iofs->io_Union.io_READ.io_Length--;
		            	}
		                *d++ = *c++;
		            }
		        }
		    } else {
		        DASYNC(bug("[emul] FSA_READ: RaiseEvent() failed!\n"));
		        error = ERROR_UNKNOWN;
		    }
		} else {
		    Forbid();
		    error = DoRead(fh->fd, iofs->io_Union.io_READ.io_Buffer, iofs->io_Union.io_READ.io_Length, &iofs->io_Union.io_READ.io_Length, NULL);
		    Permit();
		    error = error ? 0 : Errno();
		}
	  }
	  else
	  {
		error = ERROR_OBJECT_WRONG_TYPE;
	  }
	  
	  break;
	}
	  
	  case FSA_WRITE:
	{
	  struct filehandle *fh = (struct filehandle *)iofs->IOFS.io_Unit;
	  
	  if (fh->type == FHD_FILE)
	  {
		if (fh->fd == emulbase->stdin_handle)
		  fh->fd=emulbase->stdout_handle;
		Forbid();
		error = DoWrite(fh->fd, iofs->io_Union.io_WRITE.io_Buffer, iofs->io_Union.io_WRITE.io_Length, &iofs->io_Union.io_WRITE.io_Length, NULL);
		Permit();
		error = error ? 0 : Errno();
	  }
	  else
	  {
		error = ERROR_OBJECT_WRONG_TYPE;
	  }
	  
	  break;
	}
	  
	  case FSA_SEEK:
	{
	  struct filehandle *fh = (struct filehandle *)iofs->IOFS.io_Unit;
	  ULONG mode;
	  ULONG pos_high = 0;
	  UQUAD oldpos;
	  
	  D(bug("[emul] FSA_SEEK, mode %ld, offset %lu\n", iofs->io_Union.io_SEEK.io_SeekMode, iofs->io_Union.io_SEEK.io_Offset));
	  if (fh->type == FHD_FILE)
	  {
	        DB2(bug("[emul] LSeek() - getting current position\n"));
	        Forbid();
		oldpos = LSeek(fh->fd, 0, &pos_high, FILE_CURRENT);
		Permit();
		oldpos |= (UQUAD)pos_high << 32;
		D(bug("[emul] Original position: %llu\n", oldpos));
		
		switch(iofs->io_Union.io_SEEK.io_SeekMode) {
		case OFFSET_BEGINNING:
		  mode = FILE_BEGIN;
		  break;
		case OFFSET_CURRENT:
		  mode = FILE_CURRENT;
		  break;
		default:
		  mode = FILE_END;
		}
		pos_high = iofs->io_Union.io_SEEK.io_Offset >> 32;
		DB2(bug("[emul] LSeek() - setting new position\n"));
		Forbid();
		error = LSeek(fh->fd, iofs->io_Union.io_SEEK.io_Offset, &pos_high, mode);
		Permit();
		error = (error == (ULONG)-1) ? Errno() : 0;
		
		iofs->io_Union.io_SEEK.io_Offset = oldpos;
	  }
	  else
	  {
		error = ERROR_OBJECT_WRONG_TYPE;
	  }
	  D(bug("[emul] FSA_SEEK returning %lu\n", error));
	  break;
	}
	  
	  case FSA_SET_FILE_SIZE:
#warning FIXME: Implement FSA_SET_FILE_SIZE
	  /* We could manually change the size, but this is currently not
	   implemented. FIXME */
	  case FSA_WAIT_CHAR:
#warning FIXME: Implement FSA_WAIT_CHAR
	  /* We could manually wait for a character to arrive, but this is
	   currently not implemented. FIXME */
	  case FSA_FILE_MODE:
#warning FIXME: Implement FSA_FILE_MODE
	  error=ERROR_ACTION_NOT_KNOWN;
	  break;
	  
	  case FSA_IS_INTERACTIVE:
	{
	  struct filehandle *fh = (struct filehandle *)iofs->IOFS.io_Unit;
	  
	  if (fh->type == FHD_FILE)
	  {
	        DB2(bug("[emul] GetFileType()\n"));
	        Forbid();
		iofs->io_Union.io_IS_INTERACTIVE.io_IsInteractive = (GetFileType(fh->fd) == FILE_TYPE_CHAR) ? TRUE : FALSE;
		Permit();
	  }
	  else
	  {
		iofs->io_Union.io_IS_INTERACTIVE.io_IsInteractive = FALSE;
	  }
	  
	  break;
	}
	  
	  case FSA_SAME_LOCK:
	{
	  struct filehandle *lock1 = iofs->io_Union.io_SAME_LOCK.io_Lock[0],
	  *lock2 = iofs->io_Union.io_SAME_LOCK.io_Lock[1];
	  
	  if (strcmp(lock1->hostname, lock2->hostname))
	  {
		iofs->io_Union.io_SAME_LOCK.io_Same = LOCK_DIFFERENT;
	  }
	  else
	  {
		iofs->io_Union.io_SAME_LOCK.io_Same = LOCK_SAME;
	  }
	  
	  break;
	}
	  
	  case FSA_EXAMINE:
	  D(bug("[emul] FSA_EXAMINE\n"));
	  error = examine(emulbase,
					  (struct filehandle *)iofs->IOFS.io_Unit,
					  iofs->io_Union.io_EXAMINE.io_ead,
					  iofs->io_Union.io_EXAMINE.io_Size,
					  iofs->io_Union.io_EXAMINE.io_Mode,
					  &(iofs->io_DirPos));
	  break;
	  
	  case FSA_EXAMINE_NEXT:
	  D(bug("[emul] FSA_EXAMINE_NEXT\n"));
	  error = examine_next(emulbase,
						   (struct filehandle *)iofs->IOFS.io_Unit,
						   iofs->io_Union.io_EXAMINE_NEXT.io_fib);
	  break;
	  
	  case FSA_EXAMINE_ALL:
	  D(bug("[emul] FSA_EXAMINE_ALL\n"));
	  error = examine_all(emulbase,
						  (struct filehandle *)iofs->IOFS.io_Unit,
						  iofs->io_Union.io_EXAMINE_ALL.io_ead,
						  iofs->io_Union.io_EXAMINE_ALL.io_eac,
						  iofs->io_Union.io_EXAMINE_ALL.io_Size,
						  iofs->io_Union.io_EXAMINE_ALL.io_Mode);
	  break;
	  
	  case FSA_EXAMINE_ALL_END:
	  CloseDir((struct filehandle *)iofs->IOFS.io_Unit);
	  error = 0;
	  break;
	  
	  case FSA_OPEN_FILE:
	  D(bug("[emul] FSA_OPEN_FILE: name \"%s\", mode 0x%08lX)\n", iofs->io_Union.io_OPEN_FILE.io_Filename, iofs->io_Union.io_OPEN_FILE.io_FileMode));
	  error = open_(emulbase, (struct filehandle **)&iofs->IOFS.io_Unit,
			iofs->io_Union.io_OPEN_FILE.io_Filename, iofs->io_Union.io_OPEN_FILE.io_FileMode,
			iofs->io_Union.io_OPEN_FILE.io_Protection, FALSE);
	  if (
		  (error == ERROR_WRITE_PROTECTED) &&
		  (iofs->io_Union.io_OPEN_FILE.io_FileMode & FMF_AMIGADOS)
		  )
	  {
	        D(bug("[emul] Retrying in read-only mode\n"));
		error = open_(emulbase,(struct filehandle **)&iofs->IOFS.io_Unit,
			      iofs->io_Union.io_OPEN_FILE.io_Filename, iofs->io_Union.io_OPEN_FILE.io_FileMode & (~FMF_WRITE),
			      iofs->io_Union.io_OPEN_FILE.io_Protection, FALSE);
	  }
	  D(bug("[emul] FSA_OPEN_FILE returning %lu\n", error));
	  break;
	  
	  case FSA_CREATE_DIR:
	  error = create_dir(emulbase,
						 (struct filehandle **)&iofs->IOFS.io_Unit,
						 iofs->io_Union.io_CREATE_DIR.io_Filename,
						 iofs->io_Union.io_CREATE_DIR.io_Protection);
	  break;
	  
	  case FSA_CREATE_HARDLINK:
	  D(bug("[emul] FSA_CREATE_HARDLINK: link name \"%s\"\n", iofs->io_Union.io_CREATE_HARDLINK.io_Filename));
	  error = create_hardlink(emulbase,
							  (struct filehandle *)iofs->IOFS.io_Unit,
							  iofs->io_Union.io_CREATE_HARDLINK.io_Filename,
							  (struct filehandle *)iofs->io_Union.io_CREATE_HARDLINK.io_OldFile);
	  D(bug("[emul] FSA_CREATE_HARDLINK returning %lu\n", error));
	  break;
	  
	  case FSA_CREATE_SOFTLINK:
	  error = create_softlink(emulbase,
							  (struct filehandle *)&iofs->IOFS.io_Unit,
							  iofs->io_Union.io_CREATE_SOFTLINK.io_Filename,
							  iofs->io_Union.io_CREATE_SOFTLINK.io_Reference);
	  break;
	  
	  case FSA_RENAME:
	  error = rename_object(emulbase,
							(struct filehandle *)iofs->IOFS.io_Unit,
							iofs->io_Union.io_RENAME.io_Filename,
							iofs->io_Union.io_RENAME.io_NewName);
	  break;
	  
	  case FSA_READ_SOFTLINK:
	  error = read_softlink(emulbase,
							(struct filehandle *)iofs->IOFS.io_Unit,
							iofs->io_Union.io_READ_SOFTLINK.io_Buffer,
							iofs->io_Union.io_READ_SOFTLINK.io_Size);
	  break;
	  
	  case FSA_DELETE_OBJECT:
	  error = delete_object(emulbase,
							(struct filehandle *)iofs->IOFS.io_Unit,
							iofs->io_Union.io_DELETE_OBJECT.io_Filename);
	  break;
	  
	  case FSA_SET_PROTECT:
	  error = set_protect(emulbase,
						  (struct filehandle *)iofs->IOFS.io_Unit,
						  iofs->io_Union.io_SET_PROTECT.io_Filename,
						  iofs->io_Union.io_SET_PROTECT.io_Protection);
	  break;
	  
	  case FSA_PARENT_DIR:
	  /* error will always be 0 */
	  error = parent_dir(emulbase,
						 (struct filehandle *)iofs->IOFS.io_Unit,
						 &(iofs->io_Union.io_PARENT_DIR.io_DirName));
	  break;
	  
	  case FSA_PARENT_DIR_POST:
	  /* error will always be 0 */
	  error = 0;
	  parent_dir_post(emulbase, &(iofs->io_Union.io_PARENT_DIR.io_DirName));
	  break;    
	  
	  case FSA_IS_FILESYSTEM:
	  iofs->io_Union.io_IS_FILESYSTEM.io_IsFilesystem = TRUE;
	  error = 0;
	  break;
	  
	  case FSA_DISK_INFO:
	{
	  struct filehandle *fh = (struct filehandle *)iofs->IOFS.io_Unit;
	  struct InfoData *id = iofs->io_Union.io_INFO.io_Info;

	  error = StatFS(fh->hostname, id);
	  if (error)
	      error = Errno_w2a(error);
	  else
	      id->id_VolumeNode = fh->dl;
	  break;
	}
	  
	  case FSA_SET_COMMENT:
	  case FSA_SET_OWNER:
	  case FSA_SET_DATE:
	  case FSA_MORE_CACHE:
	  case FSA_MOUNT_MODE:
#warning FIXME: not supported yet
	  
	  default:
	  error = ERROR_ACTION_NOT_KNOWN;
	  break;
  }

  /* Set error code */
  iofs->io_DosError = error;
  
  /* If the quick bit is not set send the message to the port */
  if(!(iofs->IOFS.io_Flags & IOF_QUICK))
  {
	ReplyMsg(&iofs->IOFS.io_Message);
  }
  
  AROS_LIBFUNC_EXIT
}

/*********************************************************************************************/

AROS_LH1(LONG, abortio,
		 AROS_LHA(struct IOFileSys *, iofs, A1),
		 struct emulbase *, emulbase, 6, emul_handler)
{
  AROS_LIBFUNC_INIT
  
  /* Everything already done. */
  return 0;
  
  AROS_LIBFUNC_EXIT
}

/*********************************************************************************************/

const char *EmulSymbols[] = {
    "Emul_Init_Native",
    "EmulStat",
    "EmulDelete",
    "EmulGetHome",
    "EmulStatFS",
    NULL
};
    
const char *KernelSymbols[] = {
    "CreateFileA",
    "CloseHandle",
    "ReadFile",
    "WriteFile",
    "SetFilePointer",
    "GetFileType",
    "GetStdHandle",
    "MoveFileA",
    "GetCurrentDirectoryA",
    "FindFirstFileA",
    "FindNextFileA",
    "FindClose",
    "CreateDirectoryA",
    "SetFileAttributesA",
    "GetLastError",
    "CreateHardLinkA",
    "CreateSymbolicLinkA",
    "SetEvent",
    NULL
};

int loadhooks(struct emulbase *emulbase)
{
  ULONG r = 1;

  HostLibBase = OpenResource("hostlib.resource");
  if (!HostLibBase)
    return 1;
  D(kprintf("[EmulHandler] got hostlib.resource HostLibBase=%p\n", HostLibBase));

  emulbase->EmulHandle = HostLib_Open("Libs\\Host\\emul_handler.dll", NULL);
  if (emulbase->EmulHandle) {
    EmulIFace = (struct EmulInterface *)HostLib_GetInterface(emulbase->EmulHandle, EmulSymbols, &r);
    D(bug("[EmulHandler] Native library interface: 0x%08lX\n", EmulIFace));
    if (EmulIFace) {
        if (!r) {
            emulbase->KernelHandle = HostLib_Open("kernel32.dll", NULL);
            if (emulbase->KernelHandle) {
            	KernelIFace = (struct KernelInterface *)HostLib_GetInterface(emulbase->KernelHandle, KernelSymbols, &r);
            	if (KernelIFace) {
            	    D(bug("[EmulHandler] %lu unresolved symbols in kernel32.dll\n", r));
            	    if (r < 3) {
            	    	D(bug("[EmulHandler] CreateHardLink()     : 0x%08lX\n", KernelIFace->CreateHardLink));
            	    	D(bug("[EmulHandler] CreateSymbolicLink() : 0x%08lX\n", KernelIFace->CreateSymbolicLink));
            	    	KernelBase = OpenResource("kernel.resource");
            	    	if (KernelBase)
            	    	    return 0;
            	    }
            	    HostLib_DropInterface((APTR *)KernelIFace);
            	}
            	    D(else bug("[EmulHandler] Unable to get kernel32.dll interface!\n");)
            	HostLib_Close(emulbase->KernelHandle, NULL);
            }
        }
            D(else bug("[EmulHandler] %lu unresolved symbols!\n", r);)
        HostLib_DropInterface((APTR *)EmulIFace);
    }
        D(else bug("[EmulHandler] Unable go get host-side library interface!\n"));
    HostLib_Close(emulbase->EmulHandle, NULL);
  }
    D(else bug("[EmulHandler] Unable to open emul.handler host-side library!\n"));
  return 1;
}
