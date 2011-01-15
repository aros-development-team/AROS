/*
 Copyright  1995-2010, The AROS Development Team. All rights reserved.
 $Id$
 
 Desc: Filesystem that accesses an underlying host OS filesystem.
 Lang: english
 */

/*********************************************************************************************/

#define DEBUG 0
#define DCHDIR(x)
#define DCMD(x)
#define DERROR(x)
#define DEXAM(x)
#define DFNAME(x)
#define DFSIZE(x)
#define DLINK(x)
#define DLOCK(x)
#define DMOUNT(x)
#define DOPEN(x)
#define DSEEK(x)

#include <aros/debug.h>
#include <aros/symbolsets.h>
#include <exec/memory.h>
#include <devices/input.h>
#include <devices/inputevent.h>
#include <proto/exec.h>
#include <utility/tagitem.h>
#include <dos/exall.h>
#include <dos/dosasl.h>
#include <proto/dos.h>
#include <proto/expansion.h>

#include <limits.h>
#include <string.h>

#include "emul_intern.h"

#include LC_LIBDEFS_FILE

#ifdef AROS_FAST_BSTR

#define bstrcpy strcpy

#else

#define bstrcpy(d, s) \
    d = (d + 3) & ~3; \
    strcpy(d + 1, s); \
    d[0] = strlen(s);

#endif

/*********************************************************************************************/

static void SendEvent(struct emulbase *emulbase, LONG event)
{
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

/* Allocate a buffer, in which the filename is appended to the pathname. */
static LONG makefilename(struct emulbase *emulbase, char **dest, char **part, struct filehandle *fh, const char *filename)
{
    LONG ret = 0;
    int len, flen, dirlen;
    char *c;

    DFNAME(bug("[emul] makefilename(): directory \"%s\", file \"%s\")\n", fh->hostname, filename));

    ret = validate(filename);
    if (ret)
    	return ret;
  
    dirlen = strlen(fh->hostname);
    flen = strlen(filename);
    len = flen + dirlen + 2;

    *dest = AllocVecPooled(emulbase->mempool, len);
    if ((*dest))
    {
	CopyMem(fh->hostname, *dest, dirlen);
	c = *dest + dirlen;
	if (flen)
	    c = append(c, filename);
	*c = 0;

	c = *dest + (fh->name - fh->hostname);
	DFNAME(bug("[emul] Shrinking filename: \"%s\"\n", c));
	if (!shrink(c))
	{
	    FreeVecPooled(emulbase->mempool, *dest);
	    *dest = NULL;
	    ret = ERROR_OBJECT_NOT_FOUND;
	} else {
	    DFNAME(bug("[emul] resulting host filename: \"%s\"\n", *dest));
	    if (part) {
	        *part = c;
	        DFNAME(bug("[emul] resulting AROS filename: \"%s\"\n", c));
	    }
	}
    } else
	ret = ERROR_NO_FREE_STORE;
    return ret;
}

/*********************************************************************************************/

/* Free a filehandle */
static void free_lock(struct emulbase *emulbase, struct filehandle *current)
{
    DLOCK(bug("[emul] Lock type = %lu\n", current->type));
    DoClose(emulbase, current);

    DLOCK(bug("[emul] Freeing name: \"%s\"\n", current->hostname));
    FreeVecPooled(emulbase->mempool, current->hostname);

    DLOCK(bug("[emul] Freeing filehandle\n"));
    FreeMem(current, sizeof(struct filehandle));

    DLOCK(bug("[emul] Done\n"));
}

/*********************************************************************************************/

static LONG open_(struct emulbase *emulbase, struct filehandle **handle, const char *name, LONG mode, LONG protect, BOOL AllowDir)
{
    LONG ret = 0;
    struct filehandle *fh;

    DOPEN(bug("[emul] open_(\"%s\", 0x%lx), directories allowed: %lu\n", name, mode, AllowDir));
  
    fh = (struct filehandle *)AllocMem(sizeof(struct filehandle), MEMF_PUBLIC|MEMF_CLEAR);
    if (fh)
    {
	fh->dl = (*handle)->dl;

	/* If no filename is given and the file-descriptor is one of the
	 standard filehandles (stdin, stdout, stderr) ... */
	if((!name[0]) && ((*handle)->type & FHD_STDIO))
	{
	    /* ... then just reopen that standard filehandle. */
	    fh->type       = FHD_FILE|FHD_STDIO;
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
	    ret = DoOpen(emulbase, fh, mode, protect, AllowDir);
	    if (!ret)
	    {
	    	*handle = fh;
	    	return 0;
	    }

	    DOPEN(bug("[emul] Freeing pathname\n"));
	    FreeVecPooled(emulbase->mempool, fh->hostname);
	}
	DOPEN(bug("[emul] Freeing filehandle\n"));
	FreeMem(fh, sizeof(struct filehandle));
    } else
	ret = ERROR_NO_FREE_STORE;
    DOPEN(bug("[emul] open_() returns %lu\n", ret));
    return ret;
}


/*********************************************************************************************/

static LONG create_dir(struct emulbase *emulbase, struct filehandle **handle,
		       const char *filename, ULONG protect)
{
  LONG ret = 0;
  struct filehandle *fh;
  
  fh = (struct filehandle *)AllocMem(sizeof(struct filehandle), MEMF_PUBLIC|MEMF_CLEAR);
  if (fh)
  {
	fh->type       = FHD_DIRECTORY;
	fh->volumename = (*handle)->volumename;
	fh->dl	       = (*handle)->dl;
	
	ret = makefilename(emulbase, &fh->hostname, &fh->name, *handle, filename);
	if (!ret)
	{
	    ret = DoMkDir(emulbase, fh, protect);
	    if (!ret)
	    {
	        *handle = fh;
	        return 0;
	    }
	}
	free_lock(emulbase, fh);
  } else
	ret = ERROR_NO_FREE_STORE;
  
  return ret;
}

/*********************************************************************************************/

static LONG delete_object(struct emulbase *emulbase, struct filehandle* fh, const char *file)
{
    LONG ret = 0;
    char *filename = NULL;

    ret = makefilename(emulbase, &filename, NULL, fh, file);
    if (!ret)
    {
	ret = DoDelete(emulbase, filename);
	FreeVecPooled(emulbase->mempool, filename);
    }

    return ret;
}

/*********************************************************************************************/

static LONG set_protect(struct emulbase *emulbase, struct filehandle* fh,
			const char *file, ULONG aprot)
{
    LONG ret = 0;
    char *filename = NULL;

    if ((ret = makefilename(emulbase, &filename, NULL, fh, file)))
	return ret;

    ret = DoChMod(emulbase, filename, aprot);

    FreeVecPooled(emulbase->mempool, filename);
    return ret;
}

/*********************************************************************************************/

#define DEVNAME	    "EMU"
#define VOLNAME	    "System"
#define HANDLERNAME "emul.handler"

#define DEVNAME_LEN	3
#define VOLNAME_LEN     6
#define HANDLERNAME_LEN 12

static LONG startup(struct emulbase *emulbase)
{
    struct Library *ExpansionBase;
    struct DeviceNode *dlv;

    D(kprintf("[Emulhandler] startup\n"));

    emulbase->mempool = CreatePool(MEMF_ANY|MEMF_SEM_PROTECTED, 4096, 2000);
    if (!emulbase->mempool)
	return FALSE;

    ExpansionBase = OpenLibrary("expansion.library",0);
    if (!ExpansionBase)
	return FALSE;

    D(kprintf("[Emulhandler] startup: got ExpansionBase\n"));	  

    /*
     * Allocate space for the string from same mem,
     * Use AROS_BSTR_MEMSIZE4LEN macro for space to
     * to allocate and add an extra 4 for alignment
     * purposes.
     */
    dlv = AllocMem(sizeof(struct DeviceNode) + 6 +
		   AROS_BSTR_MEMSIZE4LEN(DEVNAME_LEN) +
		   AROS_BSTR_MEMSIZE4LEN(HANDLERNAME_LEN), MEMF_CLEAR|MEMF_PUBLIC);
    if (dlv)
    {
	STRPTR str;

	D(kprintf("[Emulhandler] startup allocated dlv\n"));
	/* We want str to point to the first 4-byte aligned memory after the structure */
	str = (STRPTR)(((IPTR)dlv + sizeof(struct DeviceNode) + 3) & ~3);

	bstrcpy(str, DEVNAME);
	dlv->dn_Name = MKBADDR(str);
	dlv->dn_Ext.dn_AROS.dn_DevName = str;

	str = (STRPTR)(((IPTR)str + AROS_BSTR_MEMSIZE4LEN(DEVNAME_LEN) + 3) & ~3);
	bstrcpy(str, HANDLERNAME);
	dlv->dn_Handler = MKBADDR(str);

	dlv->dn_Type    = DLT_DEVICE;
	AddBootNode(5, 0, dlv, NULL);
    }

    CloseLibrary(ExpansionBase);

    return dlv ? TRUE : FALSE;
}

ADD2INITLIB(startup, 10)

/*********************************************************************************************/

static LONG cleanup(struct emulbase *emulbase)
{
    if (emulbase->mempool)
    	DeletePool(emulbase->mempool);

    return TRUE;
}

ADD2EXPUNGELIB(cleanup, 10);

/*********************************************************************************************/

const ULONG sizes[] = {
    0,
    offsetof(struct ExAllData,ed_Type),
    offsetof(struct ExAllData,ed_Size),
    offsetof(struct ExAllData,ed_Prot),
    offsetof(struct ExAllData,ed_Days),
    offsetof(struct ExAllData,ed_Comment),
    offsetof(struct ExAllData,ed_OwnerUID),
    sizeof(struct ExAllData)
};

LONG examine(struct emulbase *emulbase, struct filehandle *fh,
             struct ExAllData *ead, ULONG size, ULONG type,
             LONG *dirpos)
{
    LONG err;

    if (fh->type == FHD_DIRECTORY)
    {
	DEXAM(bug("[emul] examine(): Resetting search handle\n"));
	err = DoRewindDir(emulbase, fh);
	if (err)
	    return err;
    }
    /* Directory search position has been reset */
    *dirpos = 0;

    return examine_entry(emulbase, fh, NULL, ead, size, type);
}

/*********************************************************************************************/

/* Returns an allocated buffer, containing a pathname, stripped by the filename. */
char *pathname_from_name (struct emulbase *emulbase, char *name)
{
    long len = strlen(name);
    long i;
    char *result = NULL;

    /* look for the first '\' in the filename starting at the end */
    i = startpos(name, len);
  
    if (0 != i)
    {
    	result = AllocVecPooled(emulbase->mempool, i + 1);
    	if (!result)
      	    return NULL;

	copyname(result, name, i);
	result[i]=0x0;
    }
    return result;
}

/*********************************************************************************************/

static LONG create_hardlink(struct emulbase *emulbase, struct filehandle *handle, const char *name, struct filehandle *oldfile)
{
    LONG error;
    char *fn;

    DLINK(bug("[emul] Creating hardlink %s to file %s\n", name, oldfile->hostname));
    error = makefilename(emulbase, &fn, NULL, handle, name);
    if (!error)
    {
	DLINK(bug("[emul] Host name of the link: %s\n", fn));
        error = DoHardLink(emulbase, fn, oldfile->hostname);
	FreeVecPooled(emulbase->mempool, fn);
    }

    return error;
}

/*********************************************************************************************/

static LONG create_softlink(struct emulbase * emulbase,
                            struct filehandle *handle, const char *name, const char *ref)
{
    LONG error;
    char *src, *dest;

    DLINK(bug("[emul] Creating softlink %s to file %s\n", name, ref));
    DLINK(bug("[emul] Handle 0x%p, pathname %s\n", handle, handle->hostname));
    error = makefilename(emulbase, &dest, NULL, handle, name);
    if (!error)
    {
	DLINK(bug("[emul] Link host name: %s\n", dest));
	error = makefilename(emulbase, &src, NULL, handle, ref);
	if (!error)
	{
	    DLINK(bug("[emul] File host name: %s\n", src));
	    error = DoSymLink(emulbase, dest, src);
	    DLINK(bug("[emul] Error: %d\n", error));

	    FreeVecPooled(emulbase->mempool, dest);
	}
	FreeVecPooled(emulbase->mempool, src);
    }

    return error;
}

/*********************************************************************************************/

static LONG rename_object(struct emulbase * emulbase, struct filehandle *fh,
			  const char *file, const char *newname)
{
  LONG ret = 0L;
  
  char *filename = NULL , *newfilename = NULL;
  
  ret = makefilename(emulbase, &filename, NULL, fh, file);
  if (!ret)
  {
	ret = makefilename(emulbase, &newfilename, NULL, fh, newname);
	if (!ret)
	{
	    ret = DoRename(emulbase, filename, newfilename);
	    FreeVecPooled(emulbase->mempool, newfilename);
	}
	FreeVecPooled(emulbase->mempool, filename);
  }
  
  return ret;
}

/*********************************************************************************************/

static LONG read_softlink(struct emulbase *emulbase,
                          struct filehandle *fh,
                          CONST_STRPTR link,
                          STRPTR buffer,
                          ULONG *size)
{
    char *ln;
    LONG ret = 0;
    char *filename = NULL;
    long l = strlen(link) + 1;

    /* don't mess with link itself */
    ln = AllocPooled(emulbase->mempool, l);

    if (!ln)
        return ERROR_NO_FREE_STORE;

    CopyMem(link, ln, l);

    ret = makefilename(emulbase, &filename, NULL, fh, ln);
    if (!ret)
    {
        int targetlen = DoReadLink(emulbase, filename, buffer, *size, &ret);

        FreeVecPooled(emulbase->mempool, filename);

	if (targetlen < 0)
	    *size = targetlen;
	else
	{
            if (strchr(buffer, ':') == NULL)
            {
                STRPTR source = FilePart(ln);

                /* strip file part of link */
                *source = '\0';
                if (strlen(ln) + targetlen >= *size)
                {
                    /* Buffer was too small */
                    *size = -2;
                }
                else
                {
                    char* target;

                    /* copy buffer to create resolved link path in it */
                    targetlen++;
                    target = AllocVecPooled(emulbase->mempool, targetlen);
                    if (target)
                    {
                    	CopyMem(buffer, target, targetlen);
                    	if (shrink(target))
                    	{
                            strcpy(buffer, ln);
                            strcat(buffer, target);
                            *size = strlen(buffer);
                        }

                        FreeVecPooled(emulbase->mempool, target);
                    }
                }
            }
        }
    }

    FreePooled(emulbase->mempool, ln, l);

    return ret;
}

/*********************************************************************************************/

static ULONG parent_dir(struct emulbase *emulbase,
				 struct filehandle *fh,
				 char ** DirectoryName)
{
    *DirectoryName = pathname_from_name(emulbase, fh->name);
    DCHDIR(bug("[emul] Parent directory: \"%s\"\n", *DirectoryName));

    return (*DirectoryName) ? 0 : ERROR_NO_FREE_STORE;
}

/*********************************************************************************************/

static void parent_dir_post(struct emulbase *emulbase, char ** DirectoryName)
{
    /* free the previously allocated memory */
    FreeVecPooled(emulbase->mempool, *DirectoryName);
    **DirectoryName = 0;
}

/*********************************************************************************************/

static LONG set_date(struct emulbase *emulbase, struct filehandle *fh,
		     const char *FileName, struct DateStamp *date)
{
    char *fullname;
    LONG ret;

    ret = makefilename(emulbase, &fullname, NULL, fh, FileName);
    if (!ret)
    {
    	ret = DoSetDate(emulbase, fullname, date);

	FreeVecPooled(emulbase->mempool, fullname);
    }
    return ret;
}

/*********************************************************************************************/

static BOOL new_volume(struct IOFileSys *iofs, struct emulbase *emulbase)
{
    struct filehandle *fhv;
    struct DosList *doslist;
    char *unixpath;
    int vol_len = 0;
    char *sp;
    char *vol;

    unixpath = (char *)iofs->io_Union.io_OpenDevice.io_DeviceName;
    if (unixpath)
    {
	DMOUNT(bug("[emul] Mounting volume %s\n", unixpath));

	/* Volume name and Unix path are encoded into DEVICE entry of
	   MountList like this: <volumename>:<unixpath> */
	vol = unixpath;
	do {
	    if (*unixpath == 0)
		return FALSE;

	    vol_len++;
	} while (*unixpath++ != ':');
	DMOUNT(bug("[emul] Host path: %s, volume name length %u\n", unixpath, vol_len));

	sp = strchr(unixpath, '~');
	if (sp)
	{
            unixpath = GetHomeDir(emulbase, sp + 1);
	    if (!unixpath)
		return FALSE;
	}
    }
    else
    {
        ULONG res;

	DMOUNT(bug("[emul] Mounting root volume\n"));

        unixpath = AllocVec(PATH_MAX, MEMF_PUBLIC);
	if (!unixpath)
	    return FALSE;

	res = GetCurrentDir(emulbase, unixpath, PATH_MAX);
	DMOUNT(bug("[emul] GetCurrentDir() returned %d\n", res));
	if(!res)
	{
	    FreeVec(unixpath);

	    return FALSE;
	}
	D(bug("[emul] startup directory %s\n", unixpath));

	vol = VOLNAME;
	vol_len = VOLNAME_LEN + 1;
    }

    if (CheckDir(emulbase, unixpath))
    {
	FreeVec(unixpath);
	return FALSE;
    }

    fhv = AllocMem(sizeof(struct filehandle) + vol_len, MEMF_PUBLIC|MEMF_CLEAR);
    DMOUNT(bug("[emul] Volume file handle: 0x%p\n", fhv));
    if (fhv)
    {
	char *volname = (char *)fhv + sizeof(struct filehandle);

	CopyMem(vol, volname, vol_len - 1);
	volname[vol_len - 1] = 0;

	fhv->hostname   = unixpath;
	fhv->name       = unixpath + strlen(unixpath);
	fhv->type       = FHD_DIRECTORY;
	fhv->volumename = volname;
	AllocMem(12, MEMF_PUBLIC);
	DMOUNT(bug("[emul] Making volume node %s\n", volname));

	doslist = MakeDosEntry(volname, DLT_VOLUME);
	DMOUNT(bug("[emul] Volume node 0x%p\n", doslist));
	if (doslist)
	{
	    fhv->dl = doslist;
	    doslist->dol_Ext.dol_AROS.dol_Unit=(struct Unit *)fhv;
	    doslist->dol_Ext.dol_AROS.dol_Device=&emulbase->device;
	    AddDosEntry(doslist);

	    iofs->IOFS.io_Unit   = (struct Unit *)fhv;
	    iofs->IOFS.io_Device = &emulbase->device;

	    SendEvent(emulbase, IECLASS_DISKINSERTED);

	    DMOUNT(bug("[emul] Mounting done\n"));
	    return TRUE;		
	}

	DMOUNT(bug("[emul] Failed, freeing volume node\n"));
        FreeMem(fhv, sizeof(struct filehandle) + vol_len);
    }

    DMOUNT(bug("[emul] Mounting failed\n"));
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

ADD2OPENDEV(GM_UNIQUENAME(Open), 0)

/*********************************************************************************************/

AROS_LH1(void, beginio,
		 AROS_LHA(struct IOFileSys *, iofs, A1),
		 struct emulbase *, emulbase, 5, emul_handler)
{
    AROS_LIBFUNC_INIT
  
    LONG error = 0;
    struct filehandle *fh, *fh2;
    struct InfoData *id;
  
    /* WaitIO will look into this */
    iofs->IOFS.io_Message.mn_Node.ln_Type=NT_MESSAGE;
  
    /*
     Do everything quick no matter what. This is possible
     because I never need to Wait().
     */
    DB2(bug("[emul] Got command %u\n", iofs->IOFS.io_Command));

    switch(iofs->IOFS.io_Command)
    {
    case FSA_OPEN:
        DCMD(bug("[emul] FSA_OPEN(\"%s\")\n", iofs->io_Union.io_OPEN.io_Filename));
	error = open_(emulbase, (struct filehandle **)&iofs->IOFS.io_Unit,
		      iofs->io_Union.io_OPEN.io_Filename, iofs->io_Union.io_OPEN.io_FileMode, 0, TRUE);
	break;

    case FSA_CLOSE:
	DCMD(bug("[emul] FSA_CLOSE\n"));
	free_lock(emulbase, (struct filehandle *)iofs->IOFS.io_Unit);
	break;

    case FSA_READ:
	fh = (struct filehandle *)iofs->IOFS.io_Unit;  

	if (fh->type & FHD_FILE)
	{
	    BOOL async = FALSE;

	    error = DoRead(emulbase, iofs, &async);
	    if (async)
	    {
	    	/* Asynchronous request sent, reset QUICK flag and return */
		iofs->IOFS.io_Flags &= ~IOF_QUICK;
		return;
	    }
	}
	else
	    error = ERROR_OBJECT_WRONG_TYPE;
	break;

    case FSA_WRITE:
	fh = (struct filehandle *)iofs->IOFS.io_Unit;

	if (fh->type & FHD_FILE)
	{
	    BOOL async = FALSE;

	    error = DoWrite(emulbase, iofs, &async);
	    if (async)
	    {
	    	/* Asynchronous request sent, reset QUICK flag and return */
		iofs->IOFS.io_Flags &= ~IOF_QUICK;
		return;
	    }
	}
	else
	    error = ERROR_OBJECT_WRONG_TYPE;
	break;

    case FSA_SEEK:
	fh = (struct filehandle *)iofs->IOFS.io_Unit;

    	DCMD(bug("[emul] FSA_SEEK, mode %ld, offset %llu\n", iofs->io_Union.io_SEEK.io_SeekMode, iofs->io_Union.io_SEEK.io_Offset));

	if (fh->type == FHD_FILE)
	    error = DoSeek(emulbase, fh->fd, &iofs->io_Union.io_SEEK.io_Offset, iofs->io_Union.io_SEEK.io_SeekMode);
	else
	    error = ERROR_OBJECT_WRONG_TYPE;

	DSEEK(bug("[emul] FSA_SEEK returning %lu\n", error));
	break;

    case FSA_SET_FILE_SIZE:
        fh = (struct filehandle *)iofs->IOFS.io_Unit;
        
        DCMD(bug("[emul] FSA_SET_FILE_SIZE, mode %ld, offset %llu\n", iofs->io_Union.io_SET_FILE_SIZE.io_SeekMode, iofs->io_Union.io_SET_FILE_SIZE.io_Offset));
 
 	error = DoSetSize(emulbase, fh, &iofs->io_Union.io_SEEK);
	DFSIZE(bug("[emul] FSA_SET_FILE_SIZE returning %lu\n", error));
	break;

    case FSA_IS_INTERACTIVE:
	fh = (struct filehandle *)iofs->IOFS.io_Unit;
	  
	if (fh->type & FHD_FILE)
	    iofs->io_Union.io_IS_INTERACTIVE.io_IsInteractive = DoGetType(emulbase, fh->fd);
	else
	     iofs->io_Union.io_IS_INTERACTIVE.io_IsInteractive = FALSE;
	break;

    case FSA_SAME_LOCK:
	fh = iofs->io_Union.io_SAME_LOCK.io_Lock[0],
	fh2 = iofs->io_Union.io_SAME_LOCK.io_Lock[1];
	  
	if (strcmp(fh->hostname, fh2->hostname))
	    iofs->io_Union.io_SAME_LOCK.io_Same = LOCK_DIFFERENT;
	else
	    iofs->io_Union.io_SAME_LOCK.io_Same = LOCK_SAME;
	  
	break;

    case FSA_EXAMINE:
	DCMD(bug("[emul] FSA_EXAMINE\n"));
	error = examine(emulbase, (struct filehandle *)iofs->IOFS.io_Unit,
			iofs->io_Union.io_EXAMINE.io_ead,
			iofs->io_Union.io_EXAMINE.io_Size,
			iofs->io_Union.io_EXAMINE.io_Mode,
			&(iofs->io_DirPos));
	break;

    case FSA_EXAMINE_NEXT:
	DCMD(bug("[emul] FSA_EXAMINE_NEXT\n"));
	error = examine_next(emulbase, (struct filehandle *)iofs->IOFS.io_Unit,
			     iofs->io_Union.io_EXAMINE_NEXT.io_fib);
	break;

    case FSA_EXAMINE_ALL:
	DCMD(bug("[emul] FSA_EXAMINE_ALL\n"));
	error = examine_all(emulbase,
						  (struct filehandle *)iofs->IOFS.io_Unit,
						  iofs->io_Union.io_EXAMINE_ALL.io_ead,
						  iofs->io_Union.io_EXAMINE_ALL.io_eac,
						  iofs->io_Union.io_EXAMINE_ALL.io_Size,
						  iofs->io_Union.io_EXAMINE_ALL.io_Mode);
	break;
	  
    case FSA_EXAMINE_ALL_END:
        /* Just rewind */
	error = DoRewindDir(emulbase, (struct filehandle *)iofs->IOFS.io_Unit);
	break;
	  
    case FSA_OPEN_FILE:
	DCMD(bug("[emul] FSA_OPEN_FILE: name \"%s\", mode 0x%08lX)\n", iofs->io_Union.io_OPEN_FILE.io_Filename, iofs->io_Union.io_OPEN_FILE.io_FileMode));
	error = open_(emulbase, (struct filehandle **)&iofs->IOFS.io_Unit,
			iofs->io_Union.io_OPEN_FILE.io_Filename, iofs->io_Union.io_OPEN_FILE.io_FileMode,
			iofs->io_Union.io_OPEN_FILE.io_Protection, FALSE);
	break;

    case FSA_CREATE_DIR:
	error = create_dir(emulbase,
						 (struct filehandle **)&iofs->IOFS.io_Unit,
						 iofs->io_Union.io_CREATE_DIR.io_Filename,
						 iofs->io_Union.io_CREATE_DIR.io_Protection);
	break;
	  
    case FSA_CREATE_HARDLINK:
	DCMD(bug("[emul] FSA_CREATE_HARDLINK: link name \"%s\"\n", iofs->io_Union.io_CREATE_HARDLINK.io_Filename));
	error = create_hardlink(emulbase,
							  (struct filehandle *)iofs->IOFS.io_Unit,
							  iofs->io_Union.io_CREATE_HARDLINK.io_Filename,
							  (struct filehandle *)iofs->io_Union.io_CREATE_HARDLINK.io_OldFile);
	DLINK(bug("[emul] FSA_CREATE_HARDLINK returning %lu\n", error));
	break;
	  
    case FSA_CREATE_SOFTLINK:
	error = create_softlink(emulbase,
							  (struct filehandle *)iofs->IOFS.io_Unit,
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
	error = read_softlink(emulbase, (struct filehandle *)iofs->IOFS.io_Unit,
			      iofs->io_Union.io_READ_SOFTLINK.io_Filename,
			      iofs->io_Union.io_READ_SOFTLINK.io_Buffer,
			      &iofs->io_Union.io_READ_SOFTLINK.io_Size);
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
	fh = (struct filehandle *)iofs->IOFS.io_Unit;
	id = iofs->io_Union.io_INFO.io_Info;

	error = DoStatFS(emulbase, fh->hostname, id);
	if (!error)
	{
	    /* Fill in host-independent part */
	    id->id_UnitNumber = 0;
    	    id->id_DiskType   = ID_DOS_DISK; /* Well, not really true... */
	    id->id_VolumeNode = MKBADDR(fh->dl);
    	    id->id_InUse      = TRUE; /* Perhaps we should count locks? */
	}

	break;

    case FSA_SET_DATE:
	error = set_date(emulbase, (struct filehandle *)iofs->IOFS.io_Unit
			 , iofs->io_Union.io_SET_DATE.io_Filename, &iofs->io_Union.io_SET_DATE.io_Date);
	break;

/* FIXME: not supported yet
    case FSA_SET_COMMENT:
    case FSA_SET_OWNER:
    case FSA_MORE_CACHE:
    case FSA_MOUNT_MODE:
    case FSA_WAIT_CHAR:
    case FSA_FILE_MODE:*/
    default:
        DCMD(bug("[emul] Unknown action %lu\n", iofs->IOFS.io_Command));
	error = ERROR_ACTION_NOT_KNOWN;
	break;
    }

    /* Set error code */
    iofs->io_DosError = error;
    DB2(bug("[emul] Replying with error %u\n", error));

    /* If the quick bit is not set send the message to the port */
    if(!(iofs->IOFS.io_Flags & IOF_QUICK))
	ReplyMsg(&iofs->IOFS.io_Message);

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
