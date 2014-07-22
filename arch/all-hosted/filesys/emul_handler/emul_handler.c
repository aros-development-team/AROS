/*
 Copyright  1995-2014, The AROS Development Team. All rights reserved.
 $Id$
 
 Desc: Filesystem that accesses an underlying host OS filesystem.
 Lang: english
 */

/*********************************************************************************************/

#define DEBUG 0
#define DCHDIR(x)
#define DCMD(x)
#define DDEL(x)
#define DERROR(x)
#define DEXAM(x)
#define DFNAME(x)
#define DFSIZE(x)
#define DLINK(x)
#define DLOCK(x)
#define DMOUNT(x)
#define DOPEN(x)
#define DSAME(x)
#define DSEEK(x)

#include <aros/debug.h>
#include <aros/symbolsets.h>
#include <exec/memory.h>
#include <devices/input.h>
#include <devices/inputevent.h>
#include <libraries/expansion.h>
#include <proto/exec.h>
#include <utility/tagitem.h>
#include <dos/exall.h>
#include <dos/dosasl.h>
#include <proto/arossupport.h>
#include <proto/dos.h>
#include <proto/expansion.h>

#include "emul_intern.h"

#include <limits.h>
#include <string.h>
#include <stddef.h>

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

    /*
     * dos.library will give us whatever the user typed. It won't strip away device prefix.
     * Here we have to do it ourselves.
     */
    c = strrchr(filename, ':');
    if (c)
    	filename = c + 1;

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

static LONG open_(struct emulbase *emulbase, struct filehandle *fhv, struct filehandle **handle, const char *name, LONG access, LONG mode, LONG protect, BOOL AllowDir)
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
            /* If the name is empty, this is an alias of the root
             * volume's handle.
             */
            if (fh->name[0] == 0) {
                FreeVecPooled(emulbase->mempool, fh->hostname);
                FreeMem(fh, sizeof(*fh));
                if (!AllowDir) {
                    *handle = 0;
                    return ERROR_OBJECT_WRONG_TYPE;
                }
                *handle = fhv;
                return 0;
            }

            ret = DoOpen(emulbase, fh, access, mode, protect, AllowDir);
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

    DDEL(bug("[emul] Deleting object %s in handle 0x%p (%s)\n", file, fh, fh->hostname));

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

static SIPTR examine(struct emulbase *emulbase, struct filehandle *fh,
             struct FileInfoBlock *fib)
{
    UBYTE buff[sizeof(struct ExAllData) +
               sizeof(fib->fib_FileName) + 1 +
               sizeof(fib->fib_Comment) + 1];
    struct ExAllData *ead = (APTR)&buff[0];
    SIPTR err;

    err = DoExamineEntry(emulbase, fh, NULL, ead, sizeof(buff), ED_OWNER);
    if (err)
        return err;

    memset(fib, 0, sizeof(*fib));

    if (ead->ed_Name) {
        fib->fib_FileName[0] = strlen(ead->ed_Name) + 1;
        if (fib->fib_FileName[0] > sizeof(fib->fib_FileName)-1)
            fib->fib_FileName[0] = sizeof(fib->fib_FileName)-1;
        CopyMem(ead->ed_Name, &fib->fib_FileName[1], fib->fib_FileName[0]);
    }

    if (ead->ed_Comment) {
        fib->fib_Comment[0] = strlen(ead->ed_Comment) + 1;
        if (fib->fib_Comment[0] > sizeof(fib->fib_Comment)-1)
            fib->fib_Comment[0] = sizeof(fib->fib_Comment)-1;
        CopyMem(ead->ed_Comment, &fib->fib_Comment[1], fib->fib_Comment[0]);
    }

    fib->fib_DiskKey = 0;
    fib->fib_DirEntryType = ead->ed_Type;
    fib->fib_Protection = ead->ed_Prot;
    fib->fib_EntryType = ead->ed_Type;
    fib->fib_Size = ead->ed_Size;
    fib->fib_NumBlocks = (ead->ed_Size + 512 - 1) / 512;
    fib->fib_Date.ds_Days = ead->ed_Days;
    fib->fib_Date.ds_Minute = ead->ed_Mins;
    fib->fib_Date.ds_Tick = ead->ed_Ticks;
    fib->fib_OwnerUID = ead->ed_OwnerUID;
    fib->fib_OwnerGID = ead->ed_OwnerGID;
    
    return 0; 
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
    char *dest;

    DLINK(bug("[emul] Creating softlink %s to file %s\n", name, ref));
    DLINK(bug("[emul] Handle 0x%p, pathname %s\n", handle, handle->hostname));
    error = makefilename(emulbase, &dest, NULL, handle, name);
    if (!error)
    {
        char *src = AllocVecPooled(emulbase->mempool, strlen(ref)+1);
        if (src)
        {
            strcpy(src, ref);
            DLINK(bug("[emul] Link host name: %s\n", dest));
            error = DoSymLink(emulbase, src, dest);
            DLINK(bug("[emul] Error: %d\n", error));

            FreeVecPooled(emulbase->mempool, src);
        }
        else
            error = ERROR_NO_FREE_STORE;

        FreeVecPooled(emulbase->mempool, dest);
    }

    return error;
}

/*********************************************************************************************/

static LONG rename_object(struct emulbase * emulbase, struct filehandle *fh,
        		  const char *file, struct filehandle *fh2, const char *newname)
{
  LONG ret = 0L;
  
  char *filename = NULL , *newfilename = NULL;
 
  /* FIXME: fh2 is unused! */
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

static LONG read_softlink(struct emulbase *emulbase, struct filehandle *fh, CONST_STRPTR link,
                          STRPTR buffer, SIPTR *size, struct DosLibrary *DOSBase)
{
    char *ln;
    LONG ret = 0;
    char *filename = NULL;
    long l = strlen(link) + 1;

    DLINK(bug("read_softlink: link %s len %d\n", link, l));

    /* don't mess with link itself */
    ln = AllocVecPooled(emulbase->mempool, l);

    if (!ln)
        return ERROR_NO_FREE_STORE;

    CopyMem(link, ln, l);

    ret = makefilename(emulbase, &filename, NULL, fh, ln);
    if (!ret)
    {
        int targetlen = DoReadLink(emulbase, filename, buffer, *size, &ret);
        DLINK(bug("read_softlink: targetlen %d\n", targetlen));

        FreeVecPooled(emulbase->mempool, filename);

        if (targetlen < 0)
            *size = targetlen;
        else
        {
            buffer[targetlen] = '\0';
            DLINK(bug("read_softlink: buffer after DoReadLink %s\n", buffer));
            if (strchr(buffer, ':') == NULL)
            {
                STRPTR source = FilePart(ln);

                /* strip file part of link */
                *source = '\0';
                if (strlen(ln) + targetlen >= *size)
                {
                    DLINK(bug("read_softlink: buffer too small %d>=%u\n", strlen(ln) + targetlen, *size));
                    /* Buffer was too small */
                    *size = -2;
                }
                else
                {
                    /* copy buffer to create resolved link path in it */
                    char* target = AllocVecPooled(emulbase->mempool, targetlen+1);
                    if (target)
                    {
                        strcpy(target, buffer);
                        if (shrink(target))
                        {
                            strcpy(buffer, ln);
                            strcat(buffer, target);
                            *size = strlen(buffer);
                        }

                        FreeVecPooled(emulbase->mempool, target);
                    }
                    else
                        ret = ERROR_NO_FREE_STORE;
                }
            }
            else
            {
                *size = targetlen >= *size ? -2 : strlen(buffer);
            }
        }
    }

    DLINK(if (!ret) bug("read_softlink: buffer %s\n", buffer));
    FreeVecPooled(emulbase->mempool, ln);

    return ret;
}

/*********************************************************************************************/

static SIPTR parent_dir(struct emulbase *emulbase, 
                                struct filehandle *fhv,
                                struct filehandle **fhp)
{
    SIPTR err;

    DCHDIR(bug("[emul] Original directory: \"%s\"\n", (*fhp)->name));
    err = open_(emulbase, fhv, fhp, "/", ACCESS_READ, MODE_OLDFILE, 0, TRUE);
    DCHDIR(bug("[emul] Parent directory: \"%s\"\n", err ? NULL : (*fhp)->name));

    return err;
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

static LONG disk_info(struct emulbase *emulbase, struct filehandle *fh, struct InfoData *id)
{
    LONG Res2 = DoStatFS(emulbase, fh->hostname, id);

    if (!Res2)
    {
        /* Fill in host-independent part */
        id->id_UnitNumber = 0;
        id->id_DiskType   = ID_DOS_DISK; /* Well, not really true... */
        id->id_VolumeNode = MKBADDR(fh->dl);
    	id->id_InUse      = TRUE; /* Perhaps we should count locks? */
    }

    return Res2;
}

/*********************************************************************************************/

#define VOLNAME	    "System"
#define VOLNAME_LEN  6

static struct filehandle *new_volume(struct emulbase *emulbase, const char *path, struct MsgPort *mp, struct DosLibrary *DOSBase)
{
    struct filehandle *fhv;
    struct DosList *doslist;
    char *unixpath;
    const char *vol;
    int vol_len = 0;
    char *sp;

    /*
     * MakeDosNode() creates zero-length fssm_Device instead of BNULL pointer when ParamPkt[1] is zero.
     * CHECKME: is this correct, or MakeDosNode() needs to be fixed?
     */
    if (path && path[0])
    {
        DMOUNT(bug("[emul] Mounting volume %s\n", path));

	/*
	 * Volume name and Unix path are encoded into DEVICE entry of
	 * MountList like this: <volumename>:<unixpath>
	 */
	vol = path;
	do
	{
	    if (*path == 0)
		return NULL;

	    vol_len++;
	} while (*path++ != ':');
	DMOUNT(bug("[emul] Host path: %s, volume name length %u\n", path, vol_len));

        sp = strchr(path, '~');
        if (sp)
        {
            unixpath = GetHomeDir(emulbase, sp + 1);
            if (!unixpath)
                return NULL;
        } else {
            unixpath = AllocVecPooled(emulbase->mempool, strlen(path)+1);
            if (!unixpath)
                return NULL;

            CopyMem(path, unixpath, strlen(path)+1);
        }
    }
    else
    {
        ULONG res;

        DMOUNT(bug("[emul] Mounting root volume\n"));

        unixpath = AllocVecPooled(emulbase->mempool, PATH_MAX);
        if (!unixpath)
            return NULL;

        res = GetCurrentDir(emulbase, unixpath, PATH_MAX);
        DMOUNT(bug("[emul] GetCurrentDir() returned %d\n", res));
        if(!res)
        {
            FreeVec(unixpath);

            return NULL;
        }
        D(bug("[emul] startup directory %s\n", unixpath));

	vol = VOLNAME;
	vol_len = VOLNAME_LEN + 1;
    }

    DMOUNT(bug("[emul] Resolved host path: %s\n", unixpath));

    if (CheckDir(emulbase, unixpath))
    {
        FreeVecPooled(emulbase->mempool, unixpath);
        return NULL;
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
        if (!DoOpen(emulbase, fhv, ACCESS_READ, MODE_OLDFILE, 0, TRUE)) {
            DMOUNT(bug("[emul] Making volume node %s\n", volname));

            doslist = MakeDosEntry(volname, DLT_VOLUME);
            DMOUNT(bug("[emul] Volume node 0x%p\n", doslist));
            if (doslist)
            {
                fhv->dl = doslist;
                doslist->dol_Task = mp;
                AddDosEntry(doslist);

                SendEvent(emulbase, IECLASS_DISKINSERTED);

                DMOUNT(bug("[emul] Mounting done\n"));
                return fhv;
            }
            DMOUNT(bug("[emul] DOS Volume add failed, freeing volume node\n"));
        }

        DMOUNT(bug("[emul] Failed, freeing volume node\n"));
        FreeVecPooled(emulbase->mempool, unixpath);
        FreeMem(fhv, sizeof(struct filehandle) + vol_len);
    }

    DMOUNT(bug("[emul] Mounting failed\n"));
    return NULL;
}

#define FH_FROM(x) ((struct filehandle *)(x))
#define FH_FROM_LOCK(x)	\
    	({ BPTR _x = (BPTR)x; \
    	   APTR _fh; \
    	   if (_x == BNULL) { \
    	     _fh = fhv; \
    	   } else { \
    	     _fh = (APTR)(((struct FileLock *)BADDR(_x))->fl_Key); \
    	   } \
    	   (struct filehandle *)_fh;\
    	 })

static void handlePacket(struct emulbase *emulbase, struct filehandle *fhv, struct MsgPort *mp, struct DosPacket *dp, struct DosLibrary *DOSBase)
{
    SIPTR Res1 = DOSFALSE;
    SIPTR Res2 = ERROR_UNKNOWN;
    struct filehandle *fh, *fh2;
    struct FileHandle *f;
    struct FileLock *fl, *fl2;
    struct InfoData *id;
  
    DB2(bug("[emul] Got command %u\n", dp->dp_Type));

    switch(dp->dp_Type)
    {
    case ACTION_FINDINPUT:
    case ACTION_FINDOUTPUT:
    case ACTION_FINDUPDATE:
    	f = BADDR(dp->dp_Arg1);
    	fh2 = FH_FROM_LOCK(dp->dp_Arg2);

        DCMD(bug("[emul] %p ACTION_FIND%s: %p, %p, %b\n", fhv, (dp->dp_Type == ACTION_FINDINPUT) ? "INPUT" : ((dp->dp_Type == ACTION_FINDOUTPUT) ? "OUTPUT" : "UPDATE"), fh, fh2, dp->dp_Arg3));
        Res2 = open_(emulbase, fhv, &fh2, AROS_BSTR_ADDR(dp->dp_Arg3), ACCESS_WRITE, dp->dp_Type, 0, FALSE);

        if (Res2 == 0)
        {
            f->fh_Arg1 = (SIPTR)fh2;
            if (fh2 != fhv)
                fh2->locks++;
            Res1 = DOSTRUE;
        }
        else
            Res1 = DOSFALSE;

        break;

    case ACTION_END:
        fh = FH_FROM(dp->dp_Arg1);
        DCMD(bug("[emul] %p ACTION_END\n", fhv, fh));
        if (fh != fhv) {
            fh->locks--;
            if (!fh->locks)
                free_lock(emulbase, fh);
        }
        Res2 = 0;
        Res1 = DOSTRUE;
        break;

    case ACTION_READ:
        fh = FH_FROM(dp->dp_Arg1);
        DCMD(bug("[emul] %p ACTION_READ\n", fhv, fh));

        if (fh->type & FHD_FILE)
        {
            Res1 = DoRead(emulbase, fh, (APTR)dp->dp_Arg2, dp->dp_Arg3, &Res2);
        }
        else {
            Res1 = -1;
            Res2 = ERROR_OBJECT_WRONG_TYPE;
        }
        break;

    case ACTION_WRITE:
        fh = FH_FROM(dp->dp_Arg1);
        DCMD(bug("[emul] %p ACTION_WRITE\n", fhv, fh));

        if (fh->type & FHD_FILE)
        {
            Res1 = DoWrite(emulbase, fh, (APTR)dp->dp_Arg2, dp->dp_Arg3, &Res2);
        } else {
            Res1 = -1;
            Res2 = ERROR_OBJECT_WRONG_TYPE;
        }
        break;

    case ACTION_SEEK:
        fh = FH_FROM(dp->dp_Arg1);
        DCMD(bug("[emul] %p ACTION_SEEK %p, mode %ld, offset %lu\n", fhv, fh, dp->dp_Arg3, dp->dp_Arg2));

        if (fh->type == FHD_FILE)
            Res1 = DoSeek(emulbase, fh, dp->dp_Arg2, dp->dp_Arg3, &Res2);
        else {
            Res1 = DOSFALSE;
            Res2 = ERROR_OBJECT_WRONG_TYPE;
        }

        break;

    case ACTION_SET_FILE_SIZE:
        fh = FH_FROM(dp->dp_Arg1);
        DCMD(bug("[emul] %p ACTION_SET_FILE_SIZE: %p, mode %ld, offset %llu\n", fhv, fh, dp->dp_Arg2, dp->dp_Arg3));

        Res1 = DoSetSize(emulbase, fh, dp->dp_Arg2, dp->dp_Arg3, &Res2);
        if (Res2 != 0) {
           Res1 = -1;
        }
        break;

    case ACTION_SAME_LOCK:
        fh = FH_FROM_LOCK(dp->dp_Arg1);
        fh2 = FH_FROM_LOCK(dp->dp_Arg2);

        DCMD(bug("[emul] %p ACTION_SAME_LOCK: %p, %p\n", fhv, fh, fh2));
	DSAME(bug("[emul] Paths: %s, %s\n", fh->hostname, fh2->hostname));

	Res2 = 0;
	/* DOSTRUE means 'Same', DOSFALSE means 'Different' */
	Res1 = strcasecmp(fh->hostname, fh2->hostname) ? DOSFALSE : DOSTRUE;

        DSAME(bug("[emul] Replying with 0x%p, %ld\n", Res1, Res2));
        break;

    case ACTION_EXAMINE_FH:
        fh = FH_FROM(dp->dp_Arg1);
        DCMD(bug("[emul] %p ACTION_EXAMINE_FH: %p, fib %p\n", fhv, fh, BADDR(dp->dp_Arg2)));
        Res2 = examine(emulbase, fh, (struct FileInfoBlock *)BADDR(dp->dp_Arg2));
        Res1 = (Res2 == 0) ? DOSTRUE : DOSFALSE;
        break;

    case ACTION_EXAMINE_OBJECT:
        fh = FH_FROM_LOCK(dp->dp_Arg1);
        DCMD(bug("[emul] %p ACTION_EXAMINE_OBJECT: %p, fib %p\n", fhv, fh, BADDR(dp->dp_Arg2)));
        Res2 = examine(emulbase, fh, (struct FileInfoBlock *)BADDR(dp->dp_Arg2));
        Res1 = (Res2 == 0) ? DOSTRUE : DOSFALSE;
        break;

    case ACTION_EXAMINE_NEXT:
        fh = FH_FROM_LOCK(dp->dp_Arg1);
        DCMD(bug("[emul] %p ACTION_EXAMINE_NEXT: %p, fib %p (key %d)\n", fhv, fh, BADDR(dp->dp_Arg2), ((struct FileInfoBlock *)BADDR(dp->dp_Arg2))->fib_DiskKey));
        Res2 = DoExamineNext(emulbase, (struct filehandle *)fh, (struct FileInfoBlock *)BADDR(dp->dp_Arg2));
        Res1 = (Res2 == 0) ? DOSTRUE : DOSFALSE;
        break;

    case ACTION_EXAMINE_ALL:
        fh = FH_FROM_LOCK(dp->dp_Arg1);
        DCMD(bug("[emul] %p ACTION_EXAMINE_ALL: %p\n", fhv, fh));
        Res2 = DoExamineAll(emulbase, fh, (APTR)dp->dp_Arg2, BADDR(dp->dp_Arg5),
                            dp->dp_Arg3, dp->dp_Arg4, DOSBase);
        Res1 = (Res2 == 0) ? DOSTRUE : DOSFALSE;
        break;
          
    case ACTION_EXAMINE_ALL_END:
        /* Just rewind */
        fh = FH_FROM_LOCK(dp->dp_Arg1);
        DCMD(bug("[emul] %p ACTION_EXAMINE_ALL_END: %p\n", fhv, fh));
        Res2 = DoRewindDir(emulbase, (struct filehandle *)fh);
        Res1 = (Res2 == 0) ? DOSTRUE : DOSFALSE;
        break;
          
    case ACTION_CREATE_DIR:
        fh = FH_FROM_LOCK(dp->dp_Arg1);
        DCMD(bug("[emul] %p ACTION_CREATE_DIR: %p, %b\n", fhv, fh, dp->dp_Arg2));

        fl = AllocMem(sizeof(*fl), MEMF_ANY | MEMF_CLEAR);
        if (!fl) {
            Res2 = ERROR_NO_FREE_STORE;
            Res1 = DOSFALSE;
            break;
        }

        Res2 = create_dir(emulbase, &fh, AROS_BSTR_ADDR(dp->dp_Arg2),
                  FIBF_GRP_EXECUTE | FIBF_GRP_READ |
                  FIBF_OTR_EXECUTE | FIBF_OTR_READ);
        Res1 = (Res2 == 0) ? DOSTRUE : DOSFALSE;
        if (Res1 != DOSTRUE) {
            FreeMem(fl, sizeof(*fl));
            break;
        }

        /* Make a lock */
        fl->fl_Link   = BNULL;
        fl->fl_Key    = (IPTR)fh;
        fl->fl_Access = ACCESS_READ;
        fl->fl_Task   = mp;
        fl->fl_Volume = MKBADDR(fh->dl);
        if (fh != fhv)
            fh->locks++;
        Res2 = 0;
        Res1 = (SIPTR)fl;
        break;
    case ACTION_LOCATE_OBJECT:
        fh = FH_FROM_LOCK(dp->dp_Arg1);
        DCMD(bug("[emul] %p ACTION_LOCATE_OBJECT: %p, %b\n", fhv, fh, dp->dp_Arg2));

        fl = AllocMem(sizeof(*fl), MEMF_ANY | MEMF_CLEAR);
        if (!fl) {
            Res2 = ERROR_NO_FREE_STORE;
            Res1 = DOSFALSE;
            break;
        }

        Res2 = open_(emulbase, fhv, &fh, AROS_BSTR_ADDR(dp->dp_Arg2), dp->dp_Arg3, MODE_OLDFILE, 0755, TRUE);
        if (Res2) {
            Res1 = DOSFALSE;
            FreeMem(fl, sizeof(*fl));
            break;
        }

        /* Make a lock */
        fl->fl_Link   = BNULL;
        fl->fl_Key    = (IPTR)fh;
        fl->fl_Access = dp->dp_Arg3;
        fl->fl_Task   = mp;
        fl->fl_Volume = MKBADDR(fh->dl);
        if (fh != fhv)
            fh->locks++;
        Res2 = 0;
        Res1 = (SIPTR)fl;
        break;

    case ACTION_FH_FROM_LOCK:
        f = BADDR(dp->dp_Arg1);
        fh2 = FH_FROM_LOCK(dp->dp_Arg2);
        fl = BADDR(dp->dp_Arg2);
        DCMD(bug("[emul] %p ACTION_FH_FROM_LOCK: %p, lock %p\n", fhv, fh, fh2));

        f->fh_Arg1 = (SIPTR)fh2;

        if (fl)
            FreeMem(fl, sizeof(*fl));

        Res2 = 0;
        Res1 = DOSTRUE;
        break;

    case ACTION_COPY_DIR: /* DupLock */
        fh = FH_FROM_LOCK(dp->dp_Arg1);
        fl = BADDR(dp->dp_Arg1);
        DCMD(bug("[emul] %p ACTION_COPY_DIR: %p\n", fhv, fh));

        fl2 = AllocMem(sizeof(*fl2), MEMF_ANY | MEMF_CLEAR);
        if (!fl2) {
            Res2 = ERROR_NO_FREE_STORE;
            Res1 = DOSFALSE;
            break;
        }

        CopyMem(fl, fl2, sizeof(*fl2));
        if (fh != fhv)
            fh->locks++;

        Res2 = 0;
        Res1 = (SIPTR)MKBADDR(fl2);
        break;

    case ACTION_COPY_DIR_FH: /* Dup */
        fh = FH_FROM(dp->dp_Arg1);
        DCMD(bug("[emul] %p ACTION_COPY_DIR_FH: %p\n", fhv, fh));

        fl = AllocMem(sizeof(*fl), MEMF_ANY | MEMF_CLEAR);
        if (!fl) {
            Res2 = ERROR_NO_FREE_STORE;
            Res1 = DOSFALSE;
            break;
        }

        /* Make a lock */
        fl->fl_Link   = BNULL;
        fl->fl_Key    = (IPTR)fh;
        fl->fl_Access = ACCESS_READ;
        fl->fl_Task   = mp;
        fl->fl_Volume = MKBADDR(fh->dl);
        if (fh != fhv)
            fh->locks++;

        Res2 = 0;
        Res1 = (SIPTR)MKBADDR(fl);
        break;

    case ACTION_FREE_LOCK:
        fh = FH_FROM_LOCK(dp->dp_Arg1);
        fl = BADDR(dp->dp_Arg1);
        DCMD(bug("[emul] %p ACTION_FREE_LOCK: %p\n", fhv, fh));

        FreeMem(fl, sizeof(*fl));
        if (fh != fhv) {
            fh->locks--;
            if (!fh->locks)
                free_lock(emulbase, fh);
        }

        Res2 = 0;
        Res1 = DOSTRUE;
        break;

    case ACTION_MAKE_LINK:
        fh = FH_FROM_LOCK(dp->dp_Arg1);

        Res2 = ERROR_UNKNOWN;
        if (dp->dp_Arg4 == LINK_SOFT) {
                DCMD(bug("[emul] %p ACTION_MAKE_LINK: %p, dest \"%b\", src \"%b\"\n", fhv, fh, dp->dp_Arg2, dp->dp_Arg3));
                Res2 = create_softlink(emulbase, fh, AROS_BSTR_ADDR(dp->dp_Arg2), AROS_BSTR_ADDR(dp->dp_Arg3));
        } else if (dp->dp_Arg4 == LINK_HARD) {
                fh2 = FH_FROM_LOCK(dp->dp_Arg3);
                DCMD(bug("[emul] %p ACTION_MAKE_LINK: %p, dest \"%b\", src %p\n", fhv, fh, dp->dp_Arg2, fh2));
                Res2 = create_hardlink(emulbase, fh, AROS_BSTR_ADDR(dp->dp_Arg2), fh2);
        }

        Res1 = (Res2 == 0) ? DOSTRUE : DOSFALSE;
        break;
          
    case ACTION_RENAME_OBJECT:
        fh = FH_FROM_LOCK(dp->dp_Arg1);
        fh2 = FH_FROM_LOCK(dp->dp_Arg3);
        DCMD(bug("[emul] %p ACTION_RENAME_OBJECT: %p, \"%b\" => %p, \"%b\"\n", fhv, fh, dp->dp_Arg2, fh2, dp->dp_Arg4));
        Res2 = rename_object(emulbase, fh, AROS_BSTR_ADDR(dp->dp_Arg2), fh2, AROS_BSTR_ADDR(dp->dp_Arg4));

        Res1 = (Res2 == 0) ? DOSTRUE : DOSFALSE;
        break;
          
    case ACTION_READ_LINK:
        fh = FH_FROM_LOCK(dp->dp_Arg1);
        DCMD(bug("[emul] %p ACTION_READ_LINK: %p\n", fhv, fh));
        Res1 = dp->dp_Arg4;
        Res2 = read_softlink(emulbase, fh, (APTR)dp->dp_Arg2, (APTR)dp->dp_Arg3, &Res1, DOSBase);
        break;
          
    case ACTION_DELETE_OBJECT:
        fh = FH_FROM_LOCK(dp->dp_Arg1);
        DCMD(bug("[emul] %p ACTION_DELETE_OBJECT: %p\n", fhv, fh));
        Res2 = delete_object(emulbase, fh, AROS_BSTR_ADDR(dp->dp_Arg2));
        Res1 = (Res2 == 0) ? DOSTRUE : DOSFALSE;
        break;
          
    case ACTION_SET_PROTECT:
        /* dp_Arg1 is unused */
        fh = FH_FROM_LOCK(dp->dp_Arg2);
        DCMD(bug("[emul] %p ACTION_SET_PROTECT: %p\n", fhv, fh));
        Res2 = set_protect(emulbase, fh, AROS_BSTR_ADDR(dp->dp_Arg3), dp->dp_Arg4);
        Res1 = (Res2 == 0) ? DOSTRUE : DOSFALSE;
        break;
          
    case ACTION_PARENT:
        fh = FH_FROM_LOCK(dp->dp_Arg1);
        DCMD(bug("[emul] %p ACTION_PARENT: %p\n", fhv, fh));

        if (fh == fhv) {
            Res1 = 0;
            Res2 = 0;
            break;
        }

        fl = AllocMem(sizeof(*fl), MEMF_ANY | MEMF_CLEAR);
        if (!fl) {
            Res2 = ERROR_NO_FREE_STORE;
            Res1 = DOSFALSE;
            break;
        }

        Res2 = parent_dir(emulbase, fhv, &fh);
        if (Res2) {
            FreeMem(fl, sizeof(*fl));
            Res1 = DOSFALSE;
            break;
        }

        /* Make a lock */
        fl->fl_Link   = BNULL;
        fl->fl_Key    = (IPTR)fh;
        fl->fl_Access = ACCESS_READ;
        fl->fl_Task   = mp;
        fl->fl_Volume = MKBADDR(fh->dl);
        if (fh != fhv)
            fh->locks++;

        Res1 = (SIPTR)MKBADDR(fl);
        Res2 = 0;
        break;
          
    case ACTION_PARENT_FH:
        fh = FH_FROM(dp->dp_Arg1);
        DCMD(bug("[emul] %p ACTION_PARENT_FH: %p\n", fhv, fh));

        if (fh == fhv) {
            Res1 = 0;
            Res2 = 0;
            break;
        }

        fl = AllocMem(sizeof(*fl), MEMF_ANY | MEMF_CLEAR);
        if (!fl) {
            Res2 = ERROR_NO_FREE_STORE;
            Res1 = DOSFALSE;
            break;
        }

        Res2 = parent_dir(emulbase, fhv, &fh);
        if (Res2) {
            FreeMem(fl, sizeof(*fl));
            Res1 = DOSFALSE;
            break;
        }

        /* Make a lock */
        fl->fl_Link   = BNULL;
        fl->fl_Key    = (IPTR)fh;
        fl->fl_Access = ACCESS_READ;
        fl->fl_Task   = mp;
        fl->fl_Volume = MKBADDR(fh->dl);
        if (fh != fhv)
            fh->locks++;

        Res1 = (SIPTR)MKBADDR(fl);
        Res2 = 0;
        break;

    case ACTION_IS_FILESYSTEM:
        DCMD(bug("[emul] %p ACTION_IS_FILESYSTEM:\n", fhv));
        Res2 = 0;
        Res1 = DOSTRUE;
        break;
  
    case ACTION_INFO:
        fh = FH_FROM_LOCK(dp->dp_Arg1);
    	id = BADDR(dp->dp_Arg2);
    	DCMD(bug("[emul] %p ACTION_INFO:\n", fhv));
  
        Res2 = disk_info(emulbase, fh, id);
        Res1 = Res2 ? DOSFALSE : DOSTRUE;
        break;
  
    case ACTION_DISK_INFO:
        id = (struct InfoData *)BADDR(dp->dp_Arg1);
        DCMD(bug("[emul] %p ACTION_DISK_INFO:\n", fhv));

        Res2 = disk_info(emulbase, fhv, id);
        Res1 = Res2 ? DOSFALSE : DOSTRUE;
        break;

    case ACTION_SET_DATE:
        /* dp_Arg1 is unused */
        fh = FH_FROM_LOCK(dp->dp_Arg2);
        DCMD(bug("[emul] %p ACTION_SET_DATE: %p\n", fhv, fh));
        Res2 = set_date(emulbase, fh, AROS_BSTR_ADDR(dp->dp_Arg3), (struct DateStamp *)dp->dp_Arg4);
        Res1 = (Res2 == 0) ? DOSTRUE : DOSFALSE;
        break;

    case ACTION_SET_OWNER:
        /* pretend to have changed owner, avoids tons of error messages from e.g. tar */
        Res1 = DOSTRUE;
        break;

/* FIXME: not supported yet
    case ACTION_SET_COMMENT:
    case ACTION_MORE_CACHE:
    case ACTION_WAIT_CHAR:
  */
    default:
        DCMD(bug("[emul] Unknown action %lu\n", dp->dp_Type));
        Res2 = ERROR_ACTION_NOT_KNOWN;
        Res1 = DOSFALSE;
        break;
    }

    /* Set error code */
    DCMD(bug("[emul] Replying with 0x%p, %ld\n", Res1, Res2));

    ReplyPkt(dp, Res1, Res2);
}

static void EmulHandler_work(struct ExecBase *SysBase)
{
    struct DosLibrary *DOSBase;
    struct DosPacket *dp;
    struct DeviceNode *dn;
    struct FileSysStartupMsg *fssm;
    STRPTR devpath = NULL;
    struct MsgPort *mp;
    struct filehandle *fhv;
    struct emulbase *emulbase;
    const STRPTR hname = "emul-handler";

    DOSBase = (APTR)OpenLibrary("dos.library", 0);
    if (!DOSBase)
        return;

    mp = &((struct Process *)FindTask(NULL))->pr_MsgPort;

    /* Wait for startup message. */
    D(bug("EMUL: Waiting for startup...\n"));
    WaitPort(mp);
    dp = (struct DosPacket *)(GetMsg(mp)->mn_Node.ln_Name);

    D(bug("EMUL: Open resource\n"));
    emulbase = OpenResource(hname);
    if (!emulbase)
    {
        D(bug("EMUL: FATAL - can't find myself\n"));
        ReplyPkt(dp, DOSFALSE, ERROR_INVALID_RESIDENT_LIBRARY);
        return;
    }

    /* emul-handler is really a .resource. This causes the problem that the startup code is not put
     * at the beginning of module, making RunHandler not beeing able to "start" the handler if it
     * is loaded at later date (for example by "mount home:"). Making the module resident is
     * a workaround for this problem. The full solution would most likely mean separating most of
     * the code to real .resource and then building a thin "real handler" emul-handler on top of
     * that resource.
     */
    if (FindSegment(hname, NULL, TRUE) == BNULL)
        AddSegment(hname, CreateSegList(EmulHandlerMain), CMD_INTERNAL);


    fssm = BADDR(dp->dp_Arg2);
    if (fssm)
    	devpath = AROS_BSTR_ADDR(fssm->fssm_Device);

    fhv = new_volume(emulbase, devpath, mp, DOSBase);
    if (!fhv)
    {
        D(bug("EMUL: FATAL - can't create the inital volume \"%s\"\n", devpath));
        ReplyPkt(dp, DOSFALSE, ERROR_NO_FREE_STORE);
        return;
    }

    /* Now, once we know all is well with the world,
     * we tell DOS that we're the handler for this
     * DeviceNode
     */
    dn = BADDR(dp->dp_Arg3);
    dn->dn_Task = mp;

    ReplyPkt(dp, DOSTRUE, 0);

    fhv->locks = 1;
    while (fhv->locks)
    {
        dp = WaitPkt();
        handlePacket(emulbase, fhv, mp, dp, DOSBase);
    }

    D(bug("EMUL: Closing volume %s\n", fhv->volumename));
    RemDosEntry(fhv->dl);
    free_lock(emulbase, fhv);
    CloseLibrary((APTR)DOSBase);
}

AROS_PROCH(EmulHandlerMain, argptr, argstr, SysBase)
{
    AROS_PROCFUNC_INIT

    EmulHandler_work(SysBase);

    return RETURN_OK;

    AROS_PROCFUNC_EXIT
}

