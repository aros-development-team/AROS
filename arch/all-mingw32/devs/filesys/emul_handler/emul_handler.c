/*
 Copyright © 1995-2008, The AROS Development Team. All rights reserved.
 $Id$
 
 Desc: Filesystem that accesses an underlying Windows filesystem.
 Lang: english

 Please always update the version-string below, if you modify the code!
 */

/*********************************************************************************************/

#define DEBUG 0
#define DASYNC(x)

#include <aros/debug.h>
#include <aros/hostthread.h>
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
#define __DOS_NOLIBBASE__
#include <proto/dos.h>
#include <proto/arossupport.h>
#include <proto/expansion.h>
#include <proto/hostlib.h>
#include <proto/hostthread.h>
#include <libraries/expansion.h>
#include <libraries/configvars.h>
#include <libraries/expansionbase.h>

#include <aros/host-conf.h>

#include <string.h>

#include "emul_handler_intern.h"
#include "winapi.h"

#include <string.h>

#include LC_LIBDEFS_FILE

/*********************************************************************************************/

/* Init DOSBase ourselves because emul_handler is initialized before dos.library */
static struct DosLibrary *DOSBase;
static APTR HostLibBase, HostThreadBase;
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

static BOOL is_root_filename(char *filename)
{
  BOOL result = FALSE;
  
  if ((*filename == '\0') ||
	  (!strcmp(filename, ".")) ||
	  (!strcmp(filename, ".\\")))
  {
	result = TRUE;
  }
  
  return result;
}

/*********************************************************************************************/

/* Create a plain path out of the supplied filename.
 Eg 'path1\path2\\path3\' becomes 'path1\path3'.
 */
BOOL shrink(struct emulbase *emulbase, char *filename)
{
  char	  *s1,*s2;
  unsigned long len;
  
  if (filename[0] == '.')
	if (filename[1] == '/') filename += 2;
  
  for(;;)
  {
	/* leading slashes? --> return FALSE. */
	if(*filename=='/') return FALSE;
	
	/* remove superflous paths (ie paths that are followed by '/') */
	s1=strstr(filename,"//");
	if(s1==NULL)
	  break;
	s2=s1;
	while(s2 > filename)
	{
	  if (s2[-1] == '/') break;
	  s2--;
	}
	
	memmove(s2,s1+2,strlen(s1+1));
  }
  
  /* strip trailing slash */
  len=strlen(filename);
  if(len&&filename[len-1]=='/')
	filename[len-1]=0;
  
  return TRUE;
}

/*********************************************************************************************/

/* Allocate a buffer, in which the filename is appended to the pathname. */
static LONG makefilename(struct emulbase *emulbase,
						 char **dest, STRPTR dirname, STRPTR filename)
{
  LONG ret = 0;
  int len, flen, dirlen;
  char *c;

  D(bug("[emul] makefilename(\"%s\", \"%s\")\n", dirname, filename));
  dirlen = strlen(dirname) + 1;
  flen = strlen(filename);
  len = flen + dirlen + 1 + /*safety*/ 1;
  *dest=(char *)emul_malloc(emulbase, len);
  if ((*dest))
  {
	CopyMem(dirname, *dest, dirlen);
	if ((dirlen > 1) && flen)
	{
	  if ((*dest)[dirlen - 2] != '/') strcat(*dest, "/");
	}
	
	strcat(*dest, filename);
	
	if (!shrink(emulbase, *dest))
	{
	  emul_free(emulbase, *dest);
	  *dest = NULL;
	  ret = ERROR_OBJECT_NOT_FOUND;
	} else {
	  /* We are on Windows, so we have to revert slashes */
	  for (c = *dest; *c; c++) {
	      if (*c == '/')
	          *c = '\\';
	  }
	  D(bug("[emul] resulting filename: %s\n", *dest));
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
  
  /* The following three flags are low-active! */
  if (protect & FIBF_WRITE)
	uprot |= FILE_ATTRIBUTE_READONLY;
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

void *DoOpen(const char *path, int mode, int protect)
{
  ULONG flags = 0;
  ULONG lock;
  ULONG create;
  void *res;
  
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
  DB2(bug("[emul] CreateFile: name \"%s\", flags 0x%08lX, lock 0x%08lX, create %lu\n", path, flags, lock, create));
  res = OpenFile(path, flags, lock, NULL, create, prot_a2w(protect), NULL);
  DB2(bug("[emul] FileHandle = 0x%08lX\n", res));
  return res;
}

/*********************************************************************************************/

static BOOL check_volume(struct filehandle *fh, struct emulbase *emulbase)
{
  if (fh->volume == emulbase->current_volume) return TRUE;
  
  if (ChDir(fh->volume))
  {
	emulbase->current_volume = fh->volume;
	return TRUE;
  }
  
  return FALSE;
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
		DoClose(current->fd);
		D(bug("[emul] Freeing name: \"%s\"\n", current->name));
		emul_free(emulbase, current->name);
		
		if (current->pathname)
		{
		  D(bug("[emul] Freeing pathname: \"%s\"\n", current->pathname));
		  emul_free(emulbase, current->pathname);
		}
		
		if (current->DIR)
		{
		  D(bug("[emul] Closing DIR\n"));
		  CloseDir(current->DIR);
		}
	  }
	  break;
	  case FHD_DIRECTORY:
	  if (current->fd)
	  {
		CloseDir(current->fd);
	  }
	  
	  if (current->name)
		emul_free(emulbase, current->name);
	  break;
  }
  D(bug("[emul] Freeing filehandle\n"));
  FreeMem(current, sizeof(struct filehandle));
  D(bug("[emul] Done\n"));
  return 0;
}

/*********************************************************************************************/

static LONG open_(struct emulbase *emulbase, struct filehandle **handle,STRPTR name,LONG mode)
{
  LONG ret = 0;
  struct filehandle *fh;
  
  if (!check_volume(*handle, emulbase)) return ERROR_OBJECT_NOT_FOUND;
  
  fh=(struct filehandle *)AllocMem(sizeof(struct filehandle), MEMF_PUBLIC);
  if(fh!=NULL)
  {
	fh->pathname = NULL; /* just to make sure... */
	fh->DIR      = NULL;
	fh->dl = (*handle)->dl;
	/* If no filename is given and the file-descriptor is one of the
	 standard filehandles (stdin, stdout, stderr) ... */
	if((!name[0]) && ((*handle)->type == FHD_FILE) &&
	   (((*handle)->fd == emulbase->stdin_handle) || ((*handle)->fd == emulbase->stdout_handle) || ((*handle)->fd == emulbase->stderr_handle)))
	{
	  /* ... then just reopen that standard filehandle. */
	  fh->type=FHD_FILE;
	  fh->fd=(*handle)->fd;
	  fh->name="";
	  fh->volume=NULL;
	  fh->volumename=NULL;
	  *handle=fh;
	  return 0;
	}
	
	fh->volume=(*handle)->volume;
	fh->volumename=(*handle)->volumename;
	
	ret = makefilename(emulbase, &fh->name, (*handle)->name, name);
	if (!ret)
	{
	  int kind;
	  if(0<(kind = Stat(*fh->name?fh->name:".",0)))
	  {
	        D(bug("[emul] object type: %ld\n", kind));
		if(kind == 1)
		{
		  fh->type=FHD_FILE;
		  fh->fd = DoOpen(*fh->name ? fh->name : ".", mode, 0770);
		  if(fh->fd != INVALID_HANDLE_VALUE)
		  {
			*handle=fh;
			return 0;
		  }
		}else if(kind == 2)
		{
		  /* file is a directory */
		  fh->type=FHD_DIRECTORY;
		  fh->fd = OpenDir(*fh->name?fh->name:".");
		  if(fh->fd)
		  {
			*handle=fh;
			return 0;
		  }
		}else
		  ret = ERROR_OBJECT_WRONG_TYPE;
	  }
	  /* Stat() failed. If ret is unset, generate it from errno. */
	  if (!ret) {
	        D(bug("[emul] Retrieving error code\n"));
		ret = Errno();
	  }
	  
	  D(bug("[emul] Freeing pathname\n"));
	  emul_free(emulbase, fh->name);
	}
	D(bug("[emul] Freeing filehandle\n"));
	FreeMem(fh, sizeof(struct filehandle));
  } else
	ret = ERROR_NO_FREE_STORE;
  D(bug("[emul] open_() returns %lu\n", ret));
  return ret;
}

/*********************************************************************************************/

static LONG open_file(struct emulbase *emulbase, struct filehandle **handle,STRPTR name,LONG mode,LONG protect)
{
  LONG ret=ERROR_NO_FREE_STORE;
  struct filehandle *fh;
  
  if (!check_volume(*handle, emulbase)) return ERROR_OBJECT_NOT_FOUND;
  
  fh=(struct filehandle *)AllocMem(sizeof(struct filehandle), MEMF_PUBLIC);
  if(fh!=NULL)
  {
	fh->pathname = NULL; /* just to make sure... */
	fh->DIR      = NULL;
	fh->dl = (*handle)->dl;
	/* If no filename is given and the file-descriptor is one of the
	 standard filehandles (stdin, stdout, stderr) ... */
	if ((!name[0]) && ((*handle)->type==FHD_FILE) &&
	    (((*handle)->fd==emulbase->stdin_handle) || ((*handle)->fd==emulbase->stdout_handle) || ((*handle)->fd==emulbase->stderr_handle)))
	{
	  /* ... then just reopen that standard filehandle. */
	  fh->type=FHD_FILE;
	  fh->fd=(*handle)->fd;
	  fh->name="";
	  fh->volume=0;
	  *handle=fh;
	  return 0;
	}
	
	fh->volume=(*handle)->volume;
	fh->volumename=(*handle)->volumename;
	
	ret = makefilename(emulbase, &fh->name, (*handle)->name, name);
	if (!ret)
	{
	  fh->type=FHD_FILE;
	  fh->fd=DoOpen(fh->name,mode,protect);
	  if (fh->fd != INVALID_HANDLE_VALUE)
	  {
		*handle=fh;
		return 0;
	  }
	  ret=Errno();
	  emul_free(emulbase, fh->name);
	}
	FreeMem(fh, sizeof(struct filehandle));
  }
  return ret;
}

/*********************************************************************************************/

static LONG create_dir(struct emulbase *emulbase, struct filehandle **handle,
					   STRPTR filename, IPTR protect)
{
  LONG ret = 0;
  struct filehandle *fh;
  
  if (!check_volume(*handle, emulbase)) return ERROR_OBJECT_NOT_FOUND;
  
  fh = (struct filehandle *)AllocMem(sizeof(struct filehandle), MEMF_PUBLIC);
  if (fh)
  {
	fh->pathname   = NULL; /* just to make sure... */
	fh->name       = NULL;
	fh->type       = FHD_DIRECTORY;
	fh->DIR        = NULL;
	fh->fd         = NULL;
	fh->volume     = (*handle)->volume;
	fh->volumename = (*handle)->volumename;
	fh->dl	       = (*handle)->dl;
	
	ret = makefilename(emulbase, &fh->name, (*handle)->name, filename);
	if (!ret)
	{
	  
	  if (!MKDir(fh->name, protect))
	  {
		*handle = fh;
		(*handle)->fd = OpenDir((*handle)->name);
		if ((*handle)->fd)
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
  
  if (!check_volume(fh, emulbase)) return ERROR_OBJECT_NOT_FOUND;
  
  ret = makefilename(emulbase, &filename, fh->name, file);
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
  
  if (!check_volume(fh, emulbase)) return ERROR_OBJECT_NOT_FOUND;
  
  if ((ret = makefilename(emulbase, &filename, fh->name, file)))
	return ret;
  
  if (Chmod(filename, aprot))
	ret = Errno();
  
  emul_free(emulbase, filename);
  
  return ret;
}

/*********************************************************************************************/

AROS_UFH3(static int, EmulIntHandler,
	  AROS_UFCA(struct EmulThreadMessage *, msg, A0),
	  AROS_UFCA(APTR, data, A1),
	  AROS_UFCA(APTR, code, A5))
{
    AROS_USERFUNC_INIT

    DASYNC(bug("[emul] Interrupt on request 0x%p, task 0x%p\n", msg, msg->task));
    Signal(msg->task, SIGF_BLIT);
    return 1;

    AROS_USERFUNC_EXIT
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
  int kind;
  
  D(kprintf("[Emulhandler] startup\n"));
  ExpansionBase = OpenLibrary("expansion.library",0);
  if(ExpansionBase != NULL)
  {
    D(kprintf("[Emulhandler] startup: got ExpansionBase\n"));	  
    fhv=(struct filehandle *)AllocMem(sizeof(struct filehandle) + 256 + AROS_WORSTALIGN, MEMF_PUBLIC);
    if(fhv != NULL)
    {
	D(kprintf("[Emulhandler] startup allocated fhv\n"));
	fhv->name = ".";
	fhv->type = FHD_DIRECTORY;
	fhv->pathname = NULL; /* just to make sure... */
	fhv->DIR      = NULL;
	fhv->volume=(char *)(fhv + 1);
			
	/* Make sure that the root directory is valid */
	res = GetCWD(256, fhv->volume);
	kind=Stat(fhv->name,0);
	if (res > 256)
	    res = 0;
	if(res && (kind == 2))
	{
	    D(kprintf("[Emulhandler] startup got valid directory\n"));
#define DEVNAME "EMU"
#define VOLNAME "System"
			  
	    static const char *devname = DEVNAME;
	    static const char *volname = VOLNAME;
			  
	    fhv->volumename = VOLNAME;
	    emulbase->current_volume = fhv->volume;
			  
	    fhv->fd = OpenDir(fhv->name);

	    DB2(bug("[emul] GetStdFile(STD_INPUT_HANDLE)\n"));
	    emulbase->stdin_handle = GetStdFile(STD_INPUT_HANDLE);
	    if (!emulbase->stdin_handle)
		emulbase->stdin_handle = INVALID_HANDLE_VALUE;
	    DB2(bug("[emul] GetStdFile(STD_OUTPUT_HANDLE)\n"));
	    emulbase->stdout_handle = GetStdFile(STD_OUTPUT_HANDLE);
	    if (!emulbase->stdout_handle)
		emulbase->stdout_handle = INVALID_HANDLE_VALUE;
	    DB2(bug("[emul] GetStdFile(STD_ERROR_HANDLE)\n"));
	    emulbase->stderr_handle = GetStdFile(STD_ERROR_HANDLE);
	    if (!emulbase->stderr_handle)
		emulbase->stderr_handle = INVALID_HANDLE_VALUE;

	    if (emulbase->stdin_handle != INVALID_HANDLE_VALUE) {
		fhi=(struct filehandle *)AllocMem(sizeof(struct filehandle), MEMF_PUBLIC);
		if(fhi!=NULL)
		{
		    D(kprintf("[Emulhandler] allocated fhi\n"));
		    fhi->pathname   = NULL; /* just to make sure... */
		    fhi->DIR        = NULL;
		    fhi->volume     = NULL;
		    fhi->volumename = NULL;
		    fhi->dl	    = NULL;
		    fhi->type = FHD_FILE;
		    fhi->fd   = emulbase->stdin_handle;
		    fhi->name = "";
		    emulbase->eb_stdin  = fhi;
		}
	    }
	    if (emulbase->stdout_handle != INVALID_HANDLE_VALUE) {
		fho=(struct filehandle *)AllocMem(sizeof(struct filehandle), MEMF_PUBLIC);
		if(fho!=NULL)
		{
		    D(kprintf("[Emulhandler] startup allocated fho\n"));
		    fho->pathname   = NULL; /* just to make sure... */
		    fho->DIR        = NULL;
		    fho->volume     = NULL;
		    fho->volumename = NULL;
		    fho->dl	    = NULL;
		    fho->type = FHD_FILE;
		    fho->fd   = emulbase->stdout_handle;
		    fho->name = "";
		    emulbase->eb_stdout = fho;
		}
	    }
	    if (emulbase->stderr_handle != INVALID_HANDLE_VALUE) {
		fhe=(struct filehandle *)AllocMem(sizeof(struct filehandle), MEMF_PUBLIC);
		if(fhe!=NULL)
		{
		    D(kprintf("[Emulhandler] startup allocated fhe\n"));
		    fhe->pathname   = NULL; /* just to make sure... */
		    fhe->DIR        = NULL;
		    fhe->volume     = NULL;
		    fhe->volumename = NULL;
		    fhe->dl	    = NULL;
		    fhe->type = FHD_FILE;
		    fhe->fd   = emulbase->stderr_handle;
		    fhe->name = "";
		    emulbase->eb_stderr = fhe;
		}
	    }

	    ret = ERROR_NO_FREE_STORE;

	    emulbase->EmulInt.is_Code = EmulIntHandler;
	    D(bug("[Emulhandler] Creating host thread\n"));
	    emulbase->HostThread = HT_CreateNewThread(EmulIFace->EmulThread, NULL);
	    if (emulbase->HostThread) {
	      D(bug("[Emulhandler] Created host thread 0x%08lX, handle 0x%08lX, ID %lu\n", emulbase->HostThread, emulbase->HostThread->handle, emulbase->HostThread->id));
	      HT_AddIntServer(&emulbase->EmulInt, emulbase->HostThread);
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
				
		    return 0;
		}
		FreeMem(dlv, sizeof(struct DeviceNode) + 4 + AROS_BSTR_MEMSIZE4LEN(strlen(DEVNAME)));
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
char * pathname_from_name (struct emulbase *emulbase, char * name)
{
  long len = strlen(name);
  long i = len;
  char * result = NULL;
  /* look for the first '\' in the filename starting at the end */
  while (i != 0 && name[i] != '\\')
    i--;
  
  if (0 != i)
  {
    result = (char *)emul_malloc(emulbase, i+1);
    if(!result)
      return NULL;
    strncpy(result, name, i);
    result[i]=0x0;
  }
  return result;
}

/*********************************************************************************************/

/* Returns a emul_malloc()'ed buffer, containing the filename without its path. */
char * filename_from_name(struct emulbase *emulbase, char * name)
{
  long len = strlen(name);
  long i = len;
  char * result = NULL;
  /* look for the first '\' in the filename starting at the end */
  while (i != 0 && name[i] != '\\')
    i--;
  
  if (0 != i)
  {
    result = (char *)emul_malloc(emulbase, len-i);
    if(!result)
      return NULL;
    strncpy(result, &name[i+1], len-i);
    result[len-i-1]=0x0;
  }
  return result;
}

/*********************************************************************************************/

static LONG examine(struct emulbase *emulbase,
					struct filehandle *fh,
                    struct ExAllData *ead,
                    ULONG  size,
                    ULONG  type,
                    LONG  *dirpos)
{
  STRPTR next, end, last, name;
  
  /* Return an error, if supplied type is not supported. */
  if(type>ED_OWNER)
	return ERROR_BAD_NUMBER;
  
  /* Check, if the supplied buffer is large enough. */
  next=(STRPTR)ead+sizes[type];
  end =(STRPTR)ead+size;
  
  if(next>end) /* > is correct. Not >= */
	return ERROR_BUFFER_OVERFLOW;
  
  if (!check_volume(fh, emulbase)) return ERROR_OBJECT_NOT_FOUND;

  struct FileInfoBlock FIB;
/* TODO: Symbolic links are not supported under Windows. We can either leave it as is or implement them using .lnk files
  int kind = LStat(*fh->name?fh->name:".",&FIB); */
  int kind = Stat(*fh->name?fh->name:".",&FIB);
  if (kind < 0)
	return Errno();
  
  if (FHD_FILE == fh->type)
  /* What we have here is a file, so it's no that easy to
   deal with it when the user is calling ExNext() after
   Examine(). So I better prepare it now. */
  {
	/* We're going to opendir the directory where the file is in
	 and then actually start searching for the file. Yuk! */
	if (NULL == fh->pathname)
	{
	  char * filename;
	  const char * dirname;
	  fh->pathname = pathname_from_name(emulbase, fh->name);
	  filename     = filename_from_name(emulbase, fh->name);
	  if(!fh->pathname || !filename)
	  {
		emul_free(emulbase, filename);
		return ERROR_NO_FREE_STORE;
	  }
	  fh->DIR      = OpenDir(fh->pathname);
	  if(!fh->DIR)
	  {
		emul_free(emulbase, filename);
		return Errno();
	  }
	  do 
	  {
		dirname = DirName(fh->DIR);
	  }
	  while (NULL != dirname &&
			 0    != strcmp(dirname, filename));
	  emul_free(emulbase, filename);
	  if(!dirname)
	  {
		int errno = Errno();
		if(!errno)
		  return ERROR_NO_MORE_ENTRIES;
		else
		  return errno;
	  }
	  
	  *dirpos = (LONG)TellDir(fh->DIR);
	  
	}
  }
  else
  {
	*dirpos = (LONG)TellDir(fh->fd);
  }
  switch(type)
  {
	default:
	case ED_OWNER:
	  ead->ed_OwnerUID	= FIB.fib_OwnerUID;
	  ead->ed_OwnerGID	= FIB.fib_OwnerGID;
	case ED_COMMENT:
	  ead->ed_Comment=NULL;
	case ED_DATE:
	  ead->ed_Days	= FIB.fib_Date.ds_Days;
	  ead->ed_Mins	= FIB.fib_Date.ds_Minute;
	  ead->ed_Ticks	= FIB.fib_Date.ds_Tick;
	case ED_PROTECTION:
	  ead->ed_Prot 	= FIB.fib_Protection;
	case ED_SIZE:
	  ead->ed_Size	= FIB.fib_Size;
	case ED_TYPE:
	  if (kind == 2)
	  {
		if (is_root_filename(fh->name))
		{
		  ead->ed_Type = ST_ROOT;
		} else {
		  ead->ed_Type = ST_USERDIR;
		}
	  } else {
		ead->ed_Type 	= ST_FILE;
	  }
	  
	  case ED_NAME:
	  ead->ed_Name=next;
	  last=name=is_root_filename(fh->name)?fh->volumename:fh->name;
	  
	  /* do not show the "." but "" instead */
	  if (last[0] == '.' && last[1] == '\0')
		last=name="";
	  
	  while(*name)
		if(*name++=='\\')
		  last=name;
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

static LONG examine_next(struct emulbase *emulbase, 
						 struct filehandle *fh,
                         struct FileInfoBlock *FIB)
{
  int		i;
  const char * dirname;
  char 	  *name, *src, *dest, *pathname;
  void *ReadDIR;
  /* first of all we have to go to the position where Examine() or
   ExamineNext() stopped the previous time so we can read the next entry! */
  
  if (!check_volume(fh, emulbase)) return ERROR_OBJECT_NOT_FOUND;
  
  switch(fh->type)
  {
    case FHD_DIRECTORY:
	  SeekDir(fh->fd, FIB->fib_DiskKey);
	  pathname = fh->name; /* it's just a directory! */
	  ReadDIR  = fh->fd;
	  break;
	  
    case FHD_FILE:
	  SeekDir((void *)fh->DIR, FIB->fib_DiskKey);
	  pathname = fh->pathname;
	  ReadDIR  = (void *)fh->DIR;
	  break; 
  }
  /* hm, let's read the data now! 
   but skip '.' and '..' (they're not available on Amigas and
   Amiga progs wouldn't know how to treat '.' and '..', i.e. they
   might want to scan recursively the directory and end up scanning
   ./././ etc. */
  /*#undef kprintf*/
  do
  {
	dirname = DirName(ReadDIR);
	
	if (NULL == dirname)
	  return ERROR_NO_MORE_ENTRIES;  
	
  } while ( 0 == strcmp(dirname,"." ) || 
		   0 == strcmp(dirname,"..") ); 
  
  
  name = (STRPTR)emul_malloc(emulbase, strlen(pathname) + strlen(dirname) + 2);
  
  if (NULL == name)
	return ERROR_NO_FREE_STORE;
  
  strcpy(name, pathname);
  
  if (*name)
	strcat(name, "\\");
  
  strcat(name, dirname);
  
  if (Stat(name,FIB) < 0)
  {
	D(bug("Stat() failed for %s\n", name));
	
	emul_free(emulbase, name);
	
	return Errno();
  }
  
  emul_free(emulbase, name);
    
  /* fast copying of the filename with slashes conversion */
  src  = dirname;
  dest = FIB->fib_FileName;
  
  for (i =0; i<MAXFILENAMELENGTH-1;i++)
  {
      if (*src == '\\')
          *dest = *src;
      else
          *dest = '/';
      if (!*src)
          break;
  }
  
  FIB->fib_DiskKey = (LONG)TellDir(ReadDIR);
  
  return 0;
}

/*********************************************************************************************/

static LONG examine_all(struct emulbase *emulbase,
						struct filehandle *fh,
                        struct ExAllData *ead,
                        struct ExAllControl *eac,
                        ULONG  size,
                        ULONG  type)
{
  struct ExAllData *last=NULL;
  STRPTR end=(STRPTR)ead+size, name, old;
  ULONG oldpos;
  const char * dirname;
  LONG error;
  LONG dummy; /* not anything is done with this value but passed to examine */
  
  eac->eac_Entries = 0;
  if(fh->type!=FHD_DIRECTORY)
	return ERROR_OBJECT_WRONG_TYPE;
  
  if (!check_volume(fh, emulbase)) return ERROR_OBJECT_NOT_FOUND;
  
  for(;;)
  {
	oldpos=TellDir(fh->fd);
	dirname=DirName(fh->fd);
	error = Errno();
	if(dirname==NULL)
	{
	  break;
	}
	if(dirname[0]=='.'&&(!dirname[1]||(dirname[1]=='.'&&!dirname[2])))
	  continue;
	name=(STRPTR)emul_malloc(emulbase, strlen(fh->name)+strlen(dirname)+2);
	if(name==NULL)
	{
	  error=ERROR_NO_FREE_STORE;
	  break;
	}
	strcpy(name,fh->name);
	if(*name)
	  strcat(name,"\\");
	strcat(name,dirname);
	old=fh->name;
	fh->name=name;
	error=examine(emulbase,fh,ead,end-(STRPTR)ead,type,&dummy);
	fh->name=old;
	emul_free(emulbase, name);
	if(error)
	  break;
	eac->eac_Entries++;
	last=ead;
	ead=ead->ed_Next;
  }
  if (last!=NULL)
	last->ed_Next=NULL;
  if((error==ERROR_BUFFER_OVERFLOW)&&last!=NULL)
  {
	SeekDir(fh->fd,oldpos);
	return 0;
  }
  if(!error)
	error=ERROR_NO_MORE_ENTRIES;
  RewindDir(fh->fd);
  return error;
}

/*********************************************************************************************/

static LONG create_hardlink(struct emulbase *emulbase,
							struct filehandle **handle,STRPTR name,struct filehandle *oldfile)
{
  LONG error=0L;
  struct filehandle *fh;
  
  if (!KernelIFace->CreateHardLink)
      return ERROR_ACTION_NOT_KNOWN;
  
  if (!check_volume(*handle, emulbase)) return ERROR_OBJECT_NOT_FOUND;
  
  fh = AllocMem(sizeof(struct filehandle), MEMF_PUBLIC);
  if (!fh)
	return ERROR_NO_FREE_STORE;
  
  fh->pathname = NULL; /* just to make sure... */
  fh->DIR      = NULL;
  fh->dl       = (*handle)->dl;
  
  error = makefilename(emulbase, &fh->name, (*handle)->name, name);
  if (!error)
  {
	if (Link(oldfile->name, fh->name, NULL))
	  *handle = fh;
	else
	  error = Errno();
  } else
  {
	error = ERROR_NO_FREE_STORE;
	FreeMem(fh, sizeof(struct filehandle));
  }
  
  return error;
}

/*********************************************************************************************/

static LONG create_softlink(struct emulbase * emulbase,
                            struct filehandle **handle, STRPTR name, STRPTR ref)
{
  LONG error=0L;
  struct filehandle *fh;
  
  /* TODO: implement symbolic links on earlier Windows versions using shell shortcuts */
  if (!KernelIFace->CreateSymbolicLink)
      return ERROR_ACTION_NOT_KNOWN;
  
  if (!check_volume(*handle, emulbase)) return ERROR_OBJECT_NOT_FOUND;
  
  fh = AllocMem(sizeof(struct filehandle), MEMF_PUBLIC);
  if(!fh)
	return ERROR_NO_FREE_STORE;
  
  fh->pathname = NULL; /* just to make sure... */
  fh->DIR      = NULL;
  fh->dl       = (*handle)->dl;
  
  error = makefilename(emulbase, &fh->name, (*handle)->name, name);
  if (!error)
  {
	if (SymLink(ref, fh->name, 0))
	  *handle = fh;
	else
	  error = Errno();
  } else
  {
	error = ERROR_NO_FREE_STORE;
	FreeMem(fh, sizeof(struct filehandle));
  }
  
  return error;
}

/*********************************************************************************************/

static LONG rename_object(struct emulbase * emulbase,
						  struct filehandle *fh, STRPTR file, STRPTR newname)
{
  LONG ret = 0L;
  
  char *filename = NULL , *newfilename = NULL;
  
  if (!check_volume(fh, emulbase)) return ERROR_OBJECT_NOT_FOUND;
  
  ret = makefilename(emulbase, &filename, fh->name, file);
  if (!ret)
  {
	ret = makefilename(emulbase, &newfilename, fh->name, newname);
	if (!ret)
	{
	  if (!DoRename(filename,newfilename))
		ret = Errno();
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
  if (!check_volume(fh, emulbase)) return ERROR_OBJECT_NOT_FOUND;
  
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

  InitSemaphore(&emulbase->sem);
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
  
  ObtainSemaphore(&emulbase->sem);
  
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
		
	  if (!err)
	  {
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
		  fhv->name       = ".";
		  fhv->type       = FHD_DIRECTORY;
		  fhv->pathname   = NULL; /* just to make sure... */
		  fhv->DIR        = NULL;
		  fhv->volume     = unixpath;
		  fhv->volumename = iofs->io_Union.io_OpenDevice.io_DeviceName;
		  
		  if ((doslist = MakeDosEntry(fhv->volumename, DLT_VOLUME)))
		  {
		      	fhv->dl = doslist;
			doslist->dol_Ext.dol_AROS.dol_Unit=(struct Unit *)fhv;
			doslist->dol_Ext.dol_AROS.dol_Device=&emulbase->device;
			AddDosEntry(doslist);
			
			iofs->IOFS.io_Unit   = (struct Unit *)fhv;
			iofs->IOFS.io_Device = &emulbase->device;
			
			ReleaseSemaphore(&emulbase->sem);
			SendEvent(emulbase, IECLASS_DISKINSERTED);
			
			return TRUE;
			
		  } /* if ((doslist = MakeDosEntry(fhv->volumename, DLT_VOLUME))) */
		  
		  FreeMem(fhv, sizeof(struct filehandle));
		  
		} /* if (fhv != NULL)*/
		
	  } /* if (!Stat(unixpath, &st)) */
	  
	} /* if (unixpath) */
	
  } /* if (unixpath) */
  
  ReleaseSemaphore(&emulbase->sem);
  
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
  
  /* Disable(); */
  ObtainSemaphore(&emulbase->sem);
  
  /*
   Do everything quick no matter what. This is possible
   because I never need to Wait().
   */
  switch(iofs->IOFS.io_Command)
  {
    case FSA_OPEN:
          D(bug("[emul] FSA_OPEN(\"%s\")\n", iofs->io_Union.io_OPEN.io_Filename));
	  error = open_(emulbase,
					(struct filehandle **)&iofs->IOFS.io_Unit,
					iofs->io_Union.io_OPEN.io_Filename,
					iofs->io_Union.io_OPEN.io_FileMode);
	  if (
		  (error == ERROR_WRITE_PROTECTED) &&
		  (iofs->io_Union.io_OPEN.io_FileMode & FMF_AMIGADOS)
		  )
	  {
	        D(bug("[emul] Retrying in read mode\n"));
		error = open_(emulbase,
                      (struct filehandle **)&iofs->IOFS.io_Unit,
                      iofs->io_Union.io_OPEN.io_Filename,
                      iofs->io_Union.io_OPEN.io_FileMode & (~FMF_WRITE));
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
		{
		  fh->fd = emulbase->stdin_handle;
		}
		DASYNC(bug("[emul] Reading %lu bytes\n", iofs->io_Union.io_READ.io_Length));
		/* TODO: This stuff is a quick hack made to get things up and running quickly.
		 * The whole handler needs total reengineering. */
		emulbase->EmulMsg.op = EMUL_CMD_READ;
		emulbase->EmulMsg.fh = fh->fd;
		emulbase->EmulMsg.addr = iofs->io_Union.io_READ.io_Buffer;
		emulbase->EmulMsg.len = iofs->io_Union.io_READ.io_Length;
		emulbase->EmulMsg.task = FindTask(NULL);
		if (HT_PutMsg(emulbase->HostThread, &emulbase->EmulMsg)) {
		    Wait(SIGF_BLIT);
		    DASYNC(bug("[emul] Read %ld bytes, error %lu\n", emulbase->EmulMsg.actual, emulbase->EmulMsg.error));
		    iofs->io_Union.io_READ.io_Length = emulbase->EmulMsg.actual;
		    error = emulbase->EmulMsg.error;
		    if ((!error) && (fh->fd == emulbase->stdin_handle)) {
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
		    DASYNC(bug("[emul] FSA_READ: HT_PutMsg failed!\n"));
		    error = ERROR_UNKNOWN;
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
		{
		  fh->fd=emulbase->stdout_handle;
		}
		DASYNC(bug("[emul] Writing %lu bytes at 0x%p\n", iofs->io_Union.io_WRITE.io_Length, iofs->io_Union.io_WRITE.io_Buffer));
		emulbase->EmulMsg.op = EMUL_CMD_WRITE;
		emulbase->EmulMsg.fh = fh->fd;
		emulbase->EmulMsg.addr = iofs->io_Union.io_WRITE.io_Buffer;
		emulbase->EmulMsg.len = iofs->io_Union.io_WRITE.io_Length;
		emulbase->EmulMsg.task = FindTask(NULL);
		if (HT_PutMsg(emulbase->HostThread, &emulbase->EmulMsg)) {
		    Wait(SIGF_BLIT);
		    DASYNC(bug("[emul] Wrote %ld bytes, error %lu\n", emulbase->EmulMsg.actual, emulbase->EmulMsg.error));
		    iofs->io_Union.io_WRITE.io_Length = emulbase->EmulMsg.actual;
		    error = emulbase->EmulMsg.error;
		} else {
		    DASYNC(bug("[emul] FSA_WRITE: HT_PutMsg failed!\n"));
		    error = ERROR_UNKNOWN;
		}
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
		oldpos = LSeek(fh->fd, 0, &pos_high, FILE_CURRENT);
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
		if (LSeek(fh->fd, iofs->io_Union.io_SEEK.io_Offset, &pos_high, mode) == (ULONG)-1)
		{
		  error = Errno();
		}
		
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
		iofs->io_Union.io_IS_INTERACTIVE.io_IsInteractive = (GetFileType(fh->fd) == FILE_TYPE_CHAR) ? TRUE : FALSE;
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
	  
	  if ((lock1->volume != lock2->volume) || strcmp(lock1->name, lock2->name))
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
	  error = examine_next(emulbase,
						   (struct filehandle *)iofs->IOFS.io_Unit,
						   iofs->io_Union.io_EXAMINE_NEXT.io_fib);
	  break;
	  
	  case FSA_EXAMINE_ALL:
	  error = examine_all(emulbase,
						  (struct filehandle *)iofs->IOFS.io_Unit,
						  iofs->io_Union.io_EXAMINE_ALL.io_ead,
						  iofs->io_Union.io_EXAMINE_ALL.io_eac,
						  iofs->io_Union.io_EXAMINE_ALL.io_Size,
						  iofs->io_Union.io_EXAMINE_ALL.io_Mode);
	  break;
	  
	  case FSA_EXAMINE_ALL_END:
	  error = 0;
	  break;
	  
	  case FSA_OPEN_FILE:
	  D(bug("[emul] FSA_OPEN_FILE: name \"%s\", mode 0x%08lX)\n", iofs->io_Union.io_OPEN_FILE.io_Filename, iofs->io_Union.io_OPEN_FILE.io_FileMode));
	  error = open_file(emulbase,
						(struct filehandle **)&iofs->IOFS.io_Unit,
						iofs->io_Union.io_OPEN_FILE.io_Filename,
						iofs->io_Union.io_OPEN_FILE.io_FileMode,
						iofs->io_Union.io_OPEN_FILE.io_Protection);
	  if (
		  (error == ERROR_WRITE_PROTECTED) &&
		  (iofs->io_Union.io_OPEN_FILE.io_FileMode & FMF_AMIGADOS)
		  )
	  {
	        D(bug("[emul] Retrying in read-only mode\n"));
		error = open_file(emulbase,
						  (struct filehandle **)&iofs->IOFS.io_Unit,
						  iofs->io_Union.io_OPEN_FILE.io_Filename,
						  iofs->io_Union.io_OPEN_FILE.io_FileMode & (~FMF_WRITE),
						  iofs->io_Union.io_OPEN_FILE.io_Protection);
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
	  error = create_hardlink(emulbase,
							  (struct filehandle **)&iofs->IOFS.io_Unit,
							  iofs->io_Union.io_CREATE_HARDLINK.io_Filename,
							  (struct filehandle *)iofs->io_Union.io_CREATE_HARDLINK.io_OldFile);
	  break;
	  
	  case FSA_CREATE_SOFTLINK:
	  error = create_softlink(emulbase,
							  (struct filehandle **)&iofs->IOFS.io_Unit,
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

	  if (check_volume(fh, emulbase)) {
	      if (StatFS(".", id)) {
	      	id->id_VolumeNode = fh->dl;
	      	error = 0;
	      } else
	          error = Errno();
	  } else
	      error = ERROR_OBJECT_NOT_FOUND;
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
  
  /*Enable();*/
  ReleaseSemaphore(&emulbase->sem);
  
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
    "EmulThread",
    "EmulOpenDir",
    "EmulCloseDir",
    "EmulStat",
    "EmulDirName",
    "EmulTellDir",
    "EmulSeekDir",
    "EmulRewindDir",
    "EmulDelete",
    "EmulGetHome",
    "EmulStatFS",
    "EmulChmod",
    "EmulMKDir",
    "EmulErrno",
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
    "SetCurrentDirectoryA",
    "CreateHardLinkA",
    "CreateSymbolicLinkA",
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
            	    	HostThreadBase = OpenResource("hostthread.resource");
            	    	if (HostThreadBase)
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
