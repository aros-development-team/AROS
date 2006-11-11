/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: a virtual filesystem that emulates the unixish root dir
    Lang: English
*/

#define  DEBUG 1
#include <aros/debug.h>

#include <exec/errors.h>
#include <exec/resident.h>
#include <exec/memory.h>
#include <exec/devices.h>
#include <exec/lists.h>
#include <exec/nodes.h>
#include <proto/exec.h>
#include <utility/tagitem.h>
#include <dos/dosextens.h>
#include <dos/filesystem.h>
#include <proto/dos.h>
#include <aros/libcall.h>
#include <aros/asmcall.h>
#if defined(__GNUC__) || defined(__INTEL_COMPILER)
#include "rootfs_handler_gcc.h"
#endif

#include <string.h>
#include <stddef.h>

static const char name[];
static const char version[];
static const APTR inittabl[4];
static void *const functable[];
struct rootfsbase *AROS_SLIB_ENTRY(init,rootfs_handler)();
void AROS_SLIB_ENTRY(open,rootfs_handler)();
BPTR AROS_SLIB_ENTRY(close,rootfs_handler)();
BPTR AROS_SLIB_ENTRY(expunge,rootfs_handler)();
int AROS_SLIB_ENTRY(null,rootfs_handler)();
void AROS_SLIB_ENTRY(beginio,rootfs_handler)();
LONG AROS_SLIB_ENTRY(abortio,rootfs_handler)();

AROS_UFH3(LONG, rootfsproc,
    AROS_UFHA(char *,argstr,A0),
    AROS_UFHA(ULONG,argsize,D0),
    AROS_UFHA(struct ExecBase *,sysbase,A6));

static const char end;

struct root
{
    ULONG openfiles;
}

struct filehandle
{
    struct root *root;
    struct Device *device;
    struct Unit *unit;
    ULONG  depth;
};

struct rootfsbase
{
    struct Device device;
    struct ExecBase *sysbase;
    struct DosLibrary *dosbase;
    struct Process *proc;
    BPTR seglist;
};


struct rootmessage
{
    struct Message msg;
    struct
    {
        struct IOFileSys newiofs;
        struct IOFileSys *oldiofs;
    } iofs;
};


int entry(void)
{
    /* If the handler was executed by accident return error code. */
    return -1;
}

const struct Resident rootfs_handler_resident=
{
    RTC_MATCHWORD,
    (struct Resident *)&rootfs_handler_resident,
    (APTR)&end,
    RTF_AUTOINIT,
    41,
    NT_DEVICE,
    0,
    (char *)name,
    (char *)&version[6],
    (ULONG *)inittabl
};

static const char name[]="rootfs.handler";

static const char version[]="$VER: rootfs-handler 41.1 (10.6.2001)\r\n";

static const APTR inittabl[4]=
{
    (APTR)sizeof(struct rootfsbase),
    (APTR)functable,
    NULL,
    &AROS_SLIB_ENTRY(init,rootfs_handler)
};

static void *const functable[]=
{
    &AROS_SLIB_ENTRY(open,rootfs_handler),
    &AROS_SLIB_ENTRY(close,rootfs_handler),
    &AROS_SLIB_ENTRY(expunge,rootfs_handler),
    &AROS_SLIB_ENTRY(null,rootfs_handler),
    &AROS_SLIB_ENTRY(beginio,rootfs_handler),
    &AROS_SLIB_ENTRY(abortio,rootfs_handler),
    (void *)-1
};

static inline void initIOFS(struct rootfsbase *rootfsbase, struct IOFileSys *iofs,
                            ULONG type)
{
    struct Process *me = (struct Process *)FindTask(NULL);

    iofs->IOFS.io_Message.mn_Node.ln_Type = NT_REPLYMSG;
    iofs->IOFS.io_Message.mn_ReplyPort    = &me->pr_MsgPort;
    iofs->IOFS.io_Message.mn_Length       = sizeof(struct IOFileSys);
    iofs->IOFS.io_Command                 = type;
    iofs->IOFS.io_Flags                   = 0;
}

static inline BOOL redirect(struct rootfsbase *rootfsbase, struct IOFileSys *iofs,
                     struct Device *device, struct Unit *unit, struct Unit **newunit)
{
    struct IOFileSys iofs2;

    /* Prepare I/O request. */
    initIOFS(rootfsbase, &iofs2, iofs->IOFS.io_Command);

    iofs2.IOFS.io_Device = device;
    iofs2.IOFS.io_Unit   = unit;

    iofs2.io_Union = iofs->io_Union;

    kprintf("Sending the request... Device = %s - Unit = %p\n", device->dd_Library.lib_Node.ln_Name, unit);
    DoIO(&iofs2.IOFS);
    kprintf("Done! Return Code: %d\n", iofs2.io_DosError);
    iofs->io_DosError = iofs2.io_DosError;
    iofs->io_Union = iofs2.io_Union;

    if (newunit)
        *newunit = iofs2.IOFS.io_Unit;

    return !iofs2.io_DosError;
}

#if 0
static BOOL redirect(struct rootfsbase *rootfsbase, struct IOFileSys *iofs,
                     struct Device *device, struct Unit *unit)
{
    const struct filehandle *handle = (struct filehandle *)iofs->IOFS.io_Unit;
    struct rootmessage *msg;

    kprintf(">>>>>>>>>>> In SEND REQUEST <<<<<<<<<\n");

    if (namePtr) kprintf("=== name: %s\n", *namePtr);


    msg = AllocVec(sizeof(struct rootmessage), MEMF_PUBLIC | MEMF_CLEAR);
    if (!msg)
    {
    	iofs->io_DosError = ERROR_NO_FREE_STORE;
        return FALSE;
    }

    kprintf(">>>>>>>>>>> In SEND REQUEST -  2  -  <<<<<<<<<\n");

    msg->msg.mn_Length = sizeof(struct rootmessage);

    if (iofs)
    {
	struct FileHandle *fh = (struct FileHandle *)BADDR(handle->lock);
	msg->iofs.oldiofs = iofs;
        kprintf(">>>>>>>>>>> In SEND REQUEST -  3  -  <<<<<<<<<\n");

	if (namePtr)
	    msg->iofs.newiofs.io_Union.io_NamedFile.io_Filename = *namePtr;
	kprintf(">>>>>>>>>>> In SEND REQUEST -  5  -  <<<<<<<<<\n");

	iofs->IOFS.io_Flags &= ~IOF_QUICK;
	iofs->IOFS.io_Message.mn_Node.ln_Type = NT_MESSAGE;
        msg->iofs.newiofs = *iofs;

	msg->iofs.newiofs.IOFS.io_Device = fh->fh_Device;
    	msg->iofs.newiofs.IOFS.io_Unit   = fh->fh_Unit;
    }

    PutMsg(&(rootfsbase->proc->pr_MsgPort), (struct Message *)msg);

    return TRUE;
}
#endif

static STRPTR myStrDup(struct rootfsbase *rootfsbase, STRPTR old)
{
    STRPTR new;
    int len = strlen(old);

    /* Use +2 instead of +1 because we migth want to hold also a ':' */
    new = AllocVec(len+2, MEMF_ANY);
    if (new)
    {
    	CopyMem(old, new, len);
	new[len]='\0';
	new[len+1]='\0';
    }

    return new;
}


static struct filehandle *allocFHandle(struct rootfsbase *rootfsbase, struct root *root,
                                       struct Device *device, struct Unit *unit,
				       ULONG depth)
{
    struct filehandle *handle = AllocVec(sizeof(struct filehandle), MEMF_ANY);
    if (handle)
    {
	handle->root   = root;
	handle->device = device;
	handle->unit   = unit;
	handle->depth  = depth;
	root->openfiles++;
    }

    return handle;
}

static void freeFHandle(struct rootfsbase *rootfsbase, struct filehandle *handle)
{
    handle->root->openfiles--;

    FreeVec(handle);
}

static STRPTR skipVol(STRPTR path)
{
    STRPTR ptr = path;

    while (*ptr != ':' && *ptr != '\0') ptr++;

    if (*ptr == ':') path = ptr+1;

    return path;
}

static struct filehandle * getFileHandle_1(struct rootfsbase * rootfsbase, struct dnode *curdir,
                                           STRPTR path, struct FileInfoBlock *fib,
					   struct IOFileSys *iofs, STRPTR tmp)
{
    struct filehandle *handle;
    BPTR olddirlock, lock;
    STRPTR s1 = path;

    kprintf("PATH requested: %s\n", s1);

    if (handle->depth == 0 && *path != '/')


    while (*path)
    {
	if (*path == '/')
	{
	    if (depth == 0)
	    {
		kprintf("OOPS... where the heck do you want to go, huh??\n");
		iofs->io_DosError = ERROR_OBJECT_NOT_FOUND;
		return NULL;
	    }
	    kprintf("ascending...\n");

	    path++;
	}
	else
	{
    	    struct dnode *child;

	    /* get next part in the path */
	    for (s1 = path; *s1 != '/' && *s1 != '\0'; s1++);
	    if (*s1 == '/') *s1++ = '\0';

            strcpy(tmp, path);

	    kprintf("Searching....\n");
	    for
	    (
		child = GetHead((struct List *)&curdir->children);
		child ;
		child = GetSucc(child)
	    )
	    {
		kprintf("Comparing: %s - %s\n", tmp, child->name);
		if (!strcasecmp(tmp, child->name)) break;
	    }
	    kprintf("....Search finished\n");

	    if (child)
	    {
	        itsadirectory = TRUE;
		itsinthelist  = TRUE;

	    /* if it's a device add the ':' to the name */
	    if (!curdir->parent)
	        strcat(tmp, ":");


	    kprintf("Trying to lock '%s'... ", tmp);
	    olddirlock = CurrentDir(curdir->lock);
	lock = Lock(tmp, SHARED_LOCK);
	(void)CurrentDir(olddirlock);

	if (!lock)
	{
	    kprintf("Failed :(\n", tmp);
	    iofs->io_DosError = IoErr();
	    return NULL;
	}
	kprintf("Succeeded!!\n", tmp);

        if (!Examine(lock, fib))
	{
	    int len = strlen(tmp);
	    /* if Examine() fails assume that the object is a plain file */
	    fib->fib_DirEntryType = ST_FILE;
	    if (tmp[len] == ':') tmp[len] = '\0';
        }
	else
	{
	    strcpy(tmp, fib->fib_FileName);
        }

        /* A file cannot be in the middle of a path */
	if (*s1 && fib->fib_DirEntryType <= 0)
	{
	    kprintf("AHA... what do you want to do, huh?\n");
	    UnLock(lock);
	    iofs->io_DosError = ERROR_DIR_NOT_FOUND;
	    return NULL;
 	}

	/* It's a directory or a device */
	if (fib->fib_DirEntryType > 0)
	{

	    if (child)
		curdir = child;
 	    else
	    {
		curdir = allocDNode(rootfsbase, curdir, tmp, lock);
		if (!curdir)
		{
		    UnLock(lock);
		    iofs->io_DosError = ERROR_NO_FREE_STORE;
		    return NULL;
		}
	    }
	}  /* Is a directory or a device */


	/*Is there somthing else in the path? */
	if (*s1)
	{
 	    kprintf("Recursiiiiiinggggggg......\n");
	    return getFileHandle_1(rootfsbase, curdir, s1, fib, iofs, tmp);
	}

	kprintf("Forwarding the request - Current directory is: %S\n", curdir->name);

	/* send the request to the proprer device */
	{
	    struct FileHandle *fh = (struct FileHandle *)BADDR(lock);
	    STRPTR oldfilename = iofs->io_Union.io_OPEN_FILE.io_Filename;
	    struct Unit   *unit;

            iofs->io_Union.io_OPEN_FILE.io_Filename = "";

	    redirect(rootfsbase, iofs, fh->fh_Device, fh->fh_Unit, &unit);

    	    iofs->io_Union.io_OPEN_FILE.io_Filename = oldfilename;

	    if (!iofs->io_DosError)
	    {

		handle = allocFHandle(rootfsbase, curdir, fh->fh_Device, unit);
                if (handle)
	        {
		    if (lock != curdir->lock)
		        UnLock(lock);

		    return handle;
		}

       	        iofs->io_DosError = ERROR_NO_FREE_STORE;

		/* close the file just opened */
		{
		    struct IOFileSys dummy;
		    dummy.IOFS.io_Command = FSA_CLOSE;
		    redirect(rootfsbase, &dummy, fh->fh_Device, fh->fh_Unit, NULL);
		}
	    }

	    /* Did we try to open a directory? */
	    if (lock == curdir->lock)
		freeDNode(rootfsbase, curdir);
	    else
	        UnLock(lock);

	    //Fault(iofs->io_DosError, "", tmp, MAXFILENAMELENGTH+1);
	    kprintf("Error trying to open the file: %d", iofs->io_DosError);
	    return NULL;
	}
	}
    }

    kprintf("Ok... this is the end!! %s\n", curdir->name);

    if (iofs->IOFS.io_Command == FSA_OPEN)
    {
	struct FileHandle *fh = (struct FileHandle *)BADDR(curdir->lock);
	struct Device *device = fh?fh->fh_Device:NULL;
	struct Unit   *unit   = fh?fh->fh_Unit  :NULL;

        handle = allocFHandle(rootfsbase, curdir, device, unit);

        if (handle)
	    return handle;

	iofs->io_DosError = ERROR_NO_FREE_STORE;
    }

    if (!iofs->io_DosError)
        iofs->io_DosError = ERROR_OBJECT_WRONG_TYPE;

    return NULL;
}

static struct filehandle * getFileHandle(struct rootfsbase * rootfsbase, ,
                                         STRPTR path, struct IOFileSys *iofs)
{
    struct FileInfoBlock *fib = NULL;
    UBYTE tmp[MAXFILENAMELENGTH+2];
    struct filehandle *handle;

    fib = AllocDosObject(DOS_FIB, NULL);
    if (!fib)
    {
	iofs->io_DosError = ERROR_NO_FREE_STORE;
	return NULL;
    }

    handle = getFileHandle_1(rootfsbase, dir, path, fib, iofs, tmp);

    FreeDosObject(DOS_FIB, fib);

    return handle;
}

static STRPTR getPath(STRPTR path, ULONG *depth)
{
    STRPTR ret  = myStrDup(skipVol(path));
    STRPTR ret2 = ret;

    kprintf("PATH requested: %s\n", path);

    if (!ret)
    {
        *depth = ERROR_NO_FREE_STORE;
	return NULL;
    }

    while (*ret)
    {
	if (*ret == '/')
	{
	    if (*depth == 0)
	    {
		*depth = ERROR_OBJECT_NOT_FOUND;
		return NULL;
	    }
	    *depth--;
	    ret++:
	}
	else
	{
	    for (; *ret != '/' && *ret != '\0'; ret++);
	    if (*depth == 0)
		*ret = ':';

	    ret++;
	}

	*depth++;
    }

    return ret2;
}

static BOOL open_(struct rootfsbase *rootfsbase, struct IOFileSys *iofs)
{
    STRPTR path;
    struct filehandle *handle = (struct filehandle *)iofs->IOFS.io_Unit;
    ULONG depth = handle->depth;
    BOOL redirected = FALSE;
    struct
    path = getPath(iofs->io_Union.io_OPEN.io_Filename, &depth);

    if (path)

    getFileHandle(rootfsbase, path, depth, iofs);

    if (handle)
        (struct filehandle *)iofs->IOFS.io_Unit = handle;

    FreeVec(path);

    return FALSE;
}

static BOOL close_(struct rootfsbase *rootfsbase, struct IOFileSys *iofs)
{
    BOOL redirected = FALSE;

    struct filehandle *handle = (struct filehandle *)iofs->IOFS.io_Unit;

    /* check we're not the root */
    if (handle->depth)
    {
	kprintf("Closing... Device = %p - Unit = %p\n", handle->device, handle->unit);
	redirect(rootfsbase, iofs, handle->device, handle->unit, NULL);
    }

    kprintf("CLOSE %p\n", iofs->IOFS.io_Unit);

    freeFHandle(rootfsbase, handle);

    return redirected;

}
#if 0
static BOOL examine(struct rootfsbase *rootfsbase, struct IOFileSys *iofs)
{
    struct ExAllData  *ead              = iofs->io_Union.io_EXAMINE.io_ead;
    const struct filehandle *handle     = (struct filehandle *)iofs->IOFS.io_Unit;
    const ULONG              type       = iofs->io_Union.io_EXAMINE.io_Mode;
    const ULONG              size       = iofs->io_Union.io_EXAMINE.io_Size;
    STRPTR                   next, end;

    static const ULONG sizes[]=
    {
    	0,
    	offsetof(struct ExAllData,ed_Type),
    	offsetof(struct ExAllData,ed_Size),
    	offsetof(struct ExAllData,ed_Prot),
    	offsetof(struct ExAllData,ed_Days),
    	offsetof(struct ExAllData,ed_Comment),
    	offsetof(struct ExAllData,ed_OwnerUID),
    	sizeof(struct ExAllData)
    };

    kprintf("In examine...\n");
    if (type > ED_OWNER)
    {
	return ERROR_BAD_NUMBER;
    }

    next = (STRPTR)ead + sizes[type];
    end  = (STRPTR)ead + size;

    if(next>end) /* > is correct. Not >= */
	return ERROR_BUFFER_OVERFLOW;

    iofs->io_DirPos = (LONG)handle->dir;

    /* it's not the root */
    if (handle->device)
    {
	/* Get pointer to I/O request. Use stackspace for now. */
	kprintf("*NOT* Examining the root\n");
	kprintf("Our parent is: %s\n", handle->dir->parent->name);
	kprintf("Our dir is: %s\n", handle->dir->name);

	redirect(rootfsbase, iofs, handle->device, handle->unit, NULL);

	kprintf("Redirection happened...\n");
	if (ead->ed_Type == ST_ROOT)
	    ead->ed_Type = ST_USERDIR;

	kprintf("Name: %s - Size: %d - Type: %d\n", ead->ed_Name, ead->ed_Size, ead->ed_Type);
    }
    else
    {
    kprintf("*Examining* the root\n");

    /* it's the root */
    switch(type)
    {
        case ED_OWNER:
	    ead->ed_OwnerUID = 0;
	    ead->ed_OwnerGID = 0;

	/* Fall through */
        case ED_COMMENT:
	    ead->ed_Comment = NULL;

	/* Fall through */
        case ED_DATE:
	    ead->ed_Days  = 0;
	    ead->ed_Mins  = 0;
	    ead->ed_Ticks = 0;

	/* Fall through */
        case ED_PROTECTION:
	    ead->ed_Prot = 0;

	/* Fall through */
        case ED_SIZE:
	    ead->ed_Size = 0;

	/* Fall through */
        case ED_TYPE:
	    ead->ed_Type = ST_ROOT;

	/* Fall through */
	case ED_NAME:
	    ead->ed_Name = handle->dir->name;
    }
    }
    ead->ed_Next = (struct ExAllData *)(((IPTR)next + AROS_PTRALIGN - 1) & ~(AROS_PTRALIGN - 1));
    kprintf("exiting from examine...\n");
    return FALSE;
}
#endif


AROS_UFH3(struct rootfsbase *, AROS_SLIB_ENTRY(init,rootfs_handler),
 AROS_UFHA(struct rootfsbase *, rootfsbase, D0),
 AROS_UFHA(BPTR,                segList,    A0),
 AROS_UFHA(struct ExecBase *,	sysBase, A6)
)
{
    AROS_USERFUNC_INIT

    /* Store arguments */
    rootfsbase->sysbase=sysBase;
    rootfsbase->seglist=segList;

    rootfsbase->dosbase = (struct DosLibrary *)OpenLibrary("dos.library",39);
    if(rootfsbase->dosbase)
    {
	struct TagItem taglist[]=
	{
	 {NP_Entry,              (IPTR)rootfsproc},
	 {NP_Name, (IPTR)"rootfs.handler process"},
	 {NP_UserData,           (IPTR)rootfsbase},
	 {TAG_DONE,                             0}
	};

	rootfsbase->proc = CreateNewProc(taglist);

       	if (rootfsbase->proc)
	    return rootfsbase;


        CloseLibrary((struct Library *)rootfsbase->dosbase);
    }

    return NULL;
    AROS_USERFUNC_EXIT
}

AROS_LH3(void, open,
 AROS_LHA(struct IOFileSys *, iofs, A1),
 AROS_LHA(ULONG,              unitnum, D0),
 AROS_LHA(ULONG,              flags, D1),
	   struct rootfsbase *, rootfsbase, 1, rootfs_handler)
{
    AROS_LIBFUNC_INIT

    struct root *root;

    /* Get compiler happy */
    unitnum=flags=0;

   /* Mark Message as recently used. */
    iofs->IOFS.io_Message.mn_Node.ln_Type=NT_REPLYMSG;

    iofs->IOFS.io_Device=&rootfsbase->device;
    iofs->IOFS.io_Error = 0;

    root = AllocVec(sizeof(struct root), MEMF_ANY | MEMF_CLEAR);
    if (root)
    {
	struct filehandle *handle;

	handle = allocFHandle(rootfsbase, root, NULL, NULL, 0);
	if (handle)
	{
	    /* I have one more opener. */
	    rootfsbase->device.dd_Library.lib_OpenCnt++;
	    rootfsbase->device.dd_Library.lib_Flags&=~LIBF_DELEXP;
	    (struct filehandle *)iofs->IOFS.io_Unit = handle;
	    return;
        }

	FreeVec(root);
    }

    iofs->IOFS.io_Error = ERROR_NO_FREE_STORE;
    return;

    AROS_LIBFUNC_EXIT
}

AROS_LH1(BPTR, close,
 AROS_LHA(struct IOFileSys *, iofs, A1),
	   struct rootfsbase *, rootfsbase, 2, rootfs_handler)
{
    AROS_LIBFUNC_INIT
    struct filehandle *handle;

    handle = (struct filehandle *)iofs->IOFS.io_Unit;

    if (handle->device)
    {
	iofs->io_DosError = ERROR_OBJECT_WRONG_TYPE;
	return 0;
    }

    if (handle->root->opencount)
    {
	iofs->io_DosError = ERROR_OBJECT_IN_USE;
	return 0;
    }

    freeFHandle(rootfsbase, handle);

    /* Let any following attemps to use the device crash hard. */
    iofs->IOFS.io_Device=(struct Device *)-1;

    iofs->io_DosError=0;

    /* I have one fewer opener. */
    if(!--rootfsbase->device.dd_Library.lib_OpenCnt)
    {
	/* Delayed expunge pending? */
	if(rootfsbase->device.dd_Library.lib_Flags&LIBF_DELEXP)
	    /* Then expunge the device */
	    return expunge();
    }
    return 0;
    AROS_LIBFUNC_EXIT
}

AROS_LH0(BPTR, expunge, struct rootfsbase *, rootfsbase, 3, rootfs_handler)
{
    AROS_LIBFUNC_INIT

    BPTR ret;
    /*
	This function is single-threaded by exec by calling Forbid.
	Never break the Forbid() or strange things might happen.
    */

    /* Test for openers. */
    if(rootfsbase->device.dd_Library.lib_OpenCnt)
    {
	/* Set the delayed expunge flag and return. */
	rootfsbase->device.dd_Library.lib_Flags|=LIBF_DELEXP;
	return 0;
    }


    /* Tell the helper process to die */
    //sendRequest(0, 0);

    /* Free all resources */
    CloseLibrary((struct Library *)rootfsbase->dosbase);

    /* Get rid of the device. Remove it from the list. */
    Remove(&rootfsbase->device.dd_Library.lib_Node);

    /* Get returncode here - FreeMem() will destroy the field. */
    ret=rootfsbase->seglist;

    /* Free the memory. */
    FreeMem((char *)rootfsbase-rootfsbase->device.dd_Library.lib_NegSize,
	    rootfsbase->device.dd_Library.lib_NegSize+rootfsbase->device.dd_Library.lib_PosSize);

    return ret;
    AROS_LIBFUNC_EXIT
}

AROS_LH0I(int, null, struct rootfsbase *, rootfsbase, 4, rootfs_handler)
{
    AROS_LIBFUNC_INIT
    return 0;
    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, beginio,
 AROS_LHA(struct IOFileSys *, iofs, A1),
	   struct rootfsbase *, rootfsbase, 5, rootfs_handler)
{
    AROS_LIBFUNC_INIT
    BOOL redirected = FALSE;

    /*
	Do everything quick no matter what. This is possible
	because I never need to Wait().
    */

    kprintf("COMMAND = %d\n", iofs->IOFS.io_Command);
    iofs->io_DosError = 0;

    switch(iofs->IOFS.io_Command)
    {

	case FSA_OPEN:
	case FSA_OPEN_FILE:
	    redirected = open_(rootfsbase, iofs);
	    kprintf("OPEN %p\n", iofs->IOFS.io_Unit);
	    break;

	case FSA_CLOSE:
	    redirected = close_(rootfsbase, iofs);
	    break;/*
	case FSA_EXAMINE:
	    redirected = examine(rootfsbase, iofs);
	    break;  */                    /*
	case FSA_EXAMINE_NEXT:          */
	    /*
	      Get information about the next object
	      struct FileInfoBlock *fib;
	    */           /*
	    error = examine_next(iofs->io_Union.io_EXAMINE_NEXT.io_fib);
			   */
	default:
	    iofs->io_DosError = ERROR_ACTION_NOT_KNOWN;
	    break;
    }

    /* If the quick bit is not set send the message to the port */
    if(!(iofs->IOFS.io_Flags&IOF_QUICK) && !redirected)
    {
    	kprintf("Che ci faccio qui??\n");
	ReplyMsg(&iofs->IOFS.io_Message);
    }

   AROS_LIBFUNC_EXIT
}

AROS_LH1(LONG, abortio,
 AROS_LHA(struct IOFileSys *, iofs, A1),
	   struct rootfsbase *, rootfsbase, 6, rootfs_handler)
{
    AROS_LIBFUNC_INIT
    /* Everything already done. */
    return 0;
    AROS_LIBFUNC_EXIT
}

#undef SysBase
AROS_UFH3(LONG, rootfsproc,
    AROS_UFHA(char *,argstr,A0),
    AROS_UFHA(ULONG,argsize,D0),
    AROS_UFHA(struct ExecBase *,SysBase,A6))
{
    struct Process *me = (struct Process *)FindTask(0);
    struct rootmessage *msg;
    BOOL cont = TRUE;

    do
    {
    	WaitPort(&(me->pr_MsgPort));

	while
	(
	    (msg =(struct rootmessage *)GetMsg(&(me->pr_MsgPort))) &&
	    (cont = (msg->iofs.oldiofs != 0))
	)
	{
	    if (msg->msg.mn_Node.ln_Type == NT_REPLYMSG)
	    {
		struct filehandle *handle;

		msg = ((struct rootmessage *)(((char *)(msg)) - offsetof(struct rootmessage, iofs.newiofs)));

		kprintf("Hurray!! We've received the message back :)\n");

		handle = (struct filehandle *)msg->iofs.oldiofs->IOFS.io_Unit;

		msg->iofs.oldiofs->io_DosError = msg->iofs.newiofs.io_DosError;
		msg->iofs.oldiofs->io_Union    = msg->iofs.newiofs.io_Union;

		ReplyMsg(&(msg->iofs.oldiofs->IOFS.io_Message));
 	        FreeVec(msg);
 	    }
	    else
	    {
	    	struct filehandle *handle;

	    	handle = (struct filehandle *)msg->iofs.oldiofs->IOFS.io_Unit;
	    	kprintf("GOT A MESSAGE: command = %d -\n", msg->iofs.newiofs.IOFS.io_Command);

	        msg->iofs.newiofs.IOFS.io_Message.mn_ReplyPort = &(me->pr_MsgPort);

		/* Call BeginIO() vector */
		AROS_LVO_CALL1NR(
	            AROS_LCA(struct IORequest *,&(msg->iofs.newiofs.IOFS),A1),
	            struct Device *, msg->iofs.newiofs.IOFS.io_Device,5,
		);
	    }
	}
    } while (cont);

    return 0;


}


static const char end=0;
