/*
    Copyright 1995-2001 AROS - The Amiga Research OS
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
#include <proto/exec.h>
#include <utility/tagitem.h>
#include <dos/dosextens.h>
#include <dos/filesystem.h>
#include <proto/dos.h>
#include <aros/libcall.h>
#ifdef __GNUC__
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

static const char end;

struct rootfshandle
{
    struct Device *device;
    struct Unit   *unit;
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

static const char version[]="$VER: rootfs-handler 41.1 (10.6.01)\r\n";

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

/* Strip off a possible volume name */
static STRPTR getName(STRPTR name)
{
    STRPTR idx = name;

    for (; *idx != ':' && *idx !='\0'; idx++);

    if (*idx == ':')
	return idx + 1;

    return name;
}

static LONG redirect(struct IOFileSys *iofs, CONST_STRPTR name, struct rootfsbase *rootfsbase)
{
    STRPTR volname;
    CONST_STRPTR pathname, s1 = NULL;
    BPTR lock = (BPTR)NULL;
    struct DosList *dl;
    struct Device *device;
    struct Unit *unit;
    struct FileHandle *fh;

    /* Copy volume name */

    s1 = name;
    while (*s1 != '/' && *s1 != '\0')
    	s1++;

    volname = (STRPTR)AllocVec(s1 - name + 1, MEMF_ANY);
    if (volname == NULL)
	return ERROR_NO_FREE_STORE;

    CopyMem(name, volname, s1 - name);
    volname[s1 - name] = '\0';
    pathname = s1 + (*s1 == '\0'?0:1);

    /* search for the volume */

    dl = LockDosList(LDF_ALL | LDF_READ);

    /* Find logical device */
    dl = FindDosEntry(dl, volname, LDF_ALL);

    if (dl == NULL)
    {
	UnLockDosList(LDF_ALL|LDF_READ);
	FreeVec(volname);

	return ERROR_DEVICE_NOT_MOUNTED;
    }
    else if (dl->dol_Type == DLT_LATE)
    {
    	lock = Lock(dl->dol_misc.dol_assign.dol_AssignName, SHARED_LOCK);
	UnLockDosList(LDF_ALL | LDF_READ);

	if (lock != NULL)
	{
	    AssignLock(volname, lock);
	    dl = LockDosList(LDF_ALL | LDF_READ);
	    dl = FindDosEntry(dl, volname, LDF_ALL);

	    if (dl == NULL)
	    {
		UnLockDosList(LDF_ALL | LDF_READ);
		FreeVec(volname);

		return ERROR_DEVICE_NOT_MOUNTED;
	    }

	    device = dl->dol_Device;
	    unit   = dl->dol_Unit;
	}
	else
	{
	    FreeVec(volname);

	    return IoErr();
	}
    }
    else if (dl->dol_Type == DLT_NONBINDING)
    {
	lock = Lock(dl->dol_misc.dol_assign.dol_AssignName, SHARED_LOCK);
	fh = (struct FileHandle *)BADDR(lock);

	if (fh != NULL)
	{
	    device = fh->fh_Device;
	    unit = fh->fh_Unit;
	}
	else
	{
	    UnLockDosList(LDF_ALL | LDF_READ);
	    FreeVec(volname);

	    return IoErr();
	}
    }
    else
    {
	device = dl->dol_Device;
	unit   = dl->dol_Unit;
    }

    iofs->IOFS.io_Device = device;
    iofs->IOFS.io_Unit = unit;
    iofs->io_Union.io_NamedFile.io_Filename = (STRPTR)pathname;

    /* Send the request. */

    /* Call BeginIO() vector */
    AROS_LVO_CALL1NR(
	AROS_LCA(struct IORequest *,&(iofs->IOFS),A1),
	struct Device *, iofs->IOFS.io_Device,5,
    );

    if (dl != NULL)
    {
	if (dl->dol_Type == DLT_NONBINDING)
	{
	    UnLock(lock);
	}
	UnLockDosList(LDF_ALL | LDF_READ);
    }

    if (volname != NULL)
	FreeVec(volname);

    return 0;
} /* DoName */
		   
static LONG examine(struct ExAllData *ead,
                    ULONG  size,
                    ULONG  type,
                    LONG   *dirpos)
{
    STRPTR next, end;
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

    if (type > ED_OWNER)
    {
	return ERROR_BAD_NUMBER;
    }

    next = (STRPTR)ead + sizes[type];
    end  = (STRPTR)ead + size;

    if(next>end) /* > is correct. Not >= */
	return ERROR_BUFFER_OVERFLOW;

    *dirpos = 0;

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
	    ead->ed_Name = "Root";
    }

    ead->ed_Next = (struct ExAllData *)(((IPTR)next + AROS_PTRALIGN - 1) & ~(AROS_PTRALIGN - 1));

    return 0;
}
		   /*
static LONG examine_next(struct FileInfoBlock *FIB)
{
    if (FIB->fib_DiskKey == 0)


    if (file->node.mln_Succ == NULL)
    {
	return ERROR_NO_MORE_ENTRIES;
    }

    FIB->fib_OwnerUID	    = 0;
    FIB->fib_OwnerGID	    = 0;

    FIB->fib_Date.ds_Days   = 0;
    FIB->fib_Date.ds_Minute = 0;
    FIB->fib_Date.ds_Tick   = 0;
    FIB->fib_Protection	    = file->protect;
    FIB->fib_Size	    = file->size;
    FIB->fib_DirEntryType   = file->type;

    strncpy(FIB->fib_FileName, file->name, MAXFILENAMELENGTH - 1);
    strncpy(FIB->fib_Comment, file->comment != NULL ? file->comment : "",
	    MAXCOMMENTLENGTH - 1);

    FIB->fib_DiskKey = (LONG)file->node.mln_Succ;

    return 0;
}
		     */
AROS_LH2(struct rootfsbase *, init,
 AROS_LHA(struct rootfsbase *, rootfsbase, D0),
 AROS_LHA(BPTR,                segList,    A0),
	   struct ExecBase *, sysBase, 0, rootfs_handler)
{
    AROS_LIBFUNC_INIT

    /* Store arguments */
    rootfsbase->sysbase=sysBase;
    rootfsbase->seglist=segList;

    rootfsbase->dosbase=(struct DosLibrary *)OpenLibrary("dos.library",39);
    if(!rootfsbase->dosbase)
	return NULL;

    return rootfsbase;
    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, open,
 AROS_LHA(struct IOFileSys *, iofs, A1),
 AROS_LHA(ULONG,              unitnum, D0),
 AROS_LHA(ULONG,              flags, D1),
	   struct rootfsbase *, rootfsbase, 1, rootfs_handler)
{
    AROS_LIBFUNC_INIT

    /* Get compiler happy */
    unitnum=flags=0;

    /* I have one more opener. */
    rootfsbase->device.dd_Library.lib_OpenCnt++;

    /* Mark Message as recently used. */
    iofs->IOFS.io_Message.mn_Node.ln_Type=NT_REPLYMSG;

    iofs->IOFS.io_Unit=(struct Unit *)NULL;
    iofs->IOFS.io_Device=&rootfsbase->device;
    rootfsbase->device.dd_Library.lib_Flags&=~LIBF_DELEXP;

    iofs->IOFS.io_Error=0;
    return;

    AROS_LIBFUNC_EXIT
}

AROS_LH1(BPTR, close,
 AROS_LHA(struct IOFileSys *, iofs, A1),
	   struct rootfsbase *, rootfsbase, 2, rootfs_handler)
{
    AROS_LIBFUNC_INIT
    ULONG *dev;

    dev=(ULONG *)iofs->IOFS.io_Unit;
    if(*dev)
    {
	iofs->io_DosError=ERROR_OBJECT_IN_USE;
	return 0;
    }

    /* Let any following attemps to use the device crash hard. */
    iofs->IOFS.io_Device=(struct Device *)-1;
    FreeMem(dev,sizeof(ULONG));
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
    LONG error=0;
    int redirected = 0;

    /*
	Do everything quick no matter what. This is possible
	because I never need to Wait().
    */

    switch(iofs->IOFS.io_Command)
    {

	case FSA_OPEN:
	case FSA_OPEN_FILE:
	    {
  	       /*
	         get handle on a file or directory
	         STRPTR name;   file- or directoryname
	        */
		STRPTR name = getName(iofs->io_Union.io_OPEN.io_Filename);
		if (name[0])
		{
                    error = redirect(iofs, name, rootfsbase);
		    if (!error) redirected = 1;
		}
	    }
	    break;

	case FSA_CLOSE:
	    /* do nothing */
	    break;
	case FSA_EXAMINE:
	     /*
	      Get information about the current object
	      struct ExAllData *ead; buffer to be filled
	      ULONG size;    size of the buffer
	      ULONG type;    type of information to get
	      iofs->io_DirPos; leave current position so
	      ExNext() knows where to find
	      next object
	     */

	    error = examine(iofs->io_Union.io_EXAMINE.io_ead,
			    iofs->io_Union.io_EXAMINE.io_Size,
			    iofs->io_Union.io_EXAMINE.io_Mode,
			    &(iofs->io_DirPos));
	    break;                  /*
	case FSA_EXAMINE_NEXT:          */
	    /*
	      Get information about the next object
	      struct FileInfoBlock *fib;
	    */           /*
	    error = examine_next(iofs->io_Union.io_EXAMINE_NEXT.io_fib);
			   */
	default:
	    error = ERROR_NOT_IMPLEMENTED;
	    break;
    }

    if (!redirected)
    {
    	/* Set error code */
    	iofs->io_DosError=error;

    	/* If the quick bit is not set send the message to the port */
    	if(!(iofs->IOFS.io_Flags&IOF_QUICK))
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

static const char end=0;
