/*
    $Id$
    $Log$
    Revision 1.1  1996/07/28 16:37:22  digulla
    Initial revision

    Desc:
    Lang: english
*/
#include <exec/memory.h>
#include <clib/exec_protos.h>
#include <utility/tagitem.h>
#include <dos/dosextens.h>
#include <dos/filesystem.h>
#include <clib/dos_protos.h>
#include <clib/utility_protos.h>
#include "dos_intern.h"

#define NEWLIST(l)                          \
((l)->lh_Head=(struct Node *)&(l)->lh_Tail, \
 (l)->lh_Tail=NULL,                         \
 (l)->lh_TailPred=(struct Node *)(l))

/*****************************************************************************

    NAME */
	#include <clib/dos_protos.h>

	__AROS_LH2(BPTR, Open,

/*  SYNOPSIS */
	__AROS_LA(STRPTR, name,       D1),
	__AROS_LA(LONG,   accessMode, D2),

/*  LOCATION */
	struct DosLibrary *, DOSBase, 5, Dos)

/*  FUNCTION
	Opens a file for read and/or write depending on the accessmode given.

    INPUTS
	name       - NUL terminated name of the file.
	accessMode - One of MODE_OLDFILE   - open existing file
			    MODE_NEWFILE   - delete old, create new file
			    		     exclusive lock
			    MODE_READWRITE - open new one if it doesn't exist

    RESULT
	Handle to the file or 0 if the file couldn't be opened.
	IoErr() gives additional information in that case.

    NOTES
	This function is identical to Lock().

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY
	29-10-95    digulla automatically created from
			    dos_lib.fd and clib/dos_protos.h

*****************************************************************************/

/*****************************************************************************

    NAME
	#include <clib/dos_protos.h>

	__AROS_LH2(BPTR, Lock,

    SYNOPSIS
	__AROS_LA(STRPTR, name,       D1),
	__AROS_LA(LONG,   accessMode, D2),

    LOCATION
	struct DosLibrary *, DOSBase, 14, Dos)

    FUNCTION
	Gets a lock on a file or directory. There may be more than one
	shared lock on a file but only one if it is an exclusive one.
	Locked files or directories may not be deleted.

    INPUTS
	name       - NUL terminated name of the file or directory.
	accessMode - One of SHARED_LOCK
			    EXCLUSIVE_LOCK

    RESULT
	Handle to the file or directory or 0 if the object couldn't be locked.
	IoErr() gives additional information in that case.

    NOTES
	The lock structure returned by this function is different
	from that of AmigaOS. Do not try to read any internal fields.
	This function is identical to Open().

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY
	29-10-95    digulla automatically created from
			    dos_lib.fd and clib/dos_protos.h

*****************************************************************************/

{
    __AROS_FUNC_INIT
    __AROS_BASE_EXT_DECL(struct DosLibrary *,DOSBase)
    ULONG flags;
    LONG error;
    STRPTR s1, pathname, volname;
    struct DosList *dl=NULL;
    struct Device *device;
    struct Unit *unit;
    BPTR con=0, ast=0, cur;

    /* Get pointer to filehandle */
    struct FileHandle *fh, *ret;

    /* Get pointer to process structure */
    struct Process *me=(struct Process *)FindTask(NULL);

    /* Get pointer to I/O request. Use stackspace for now. */
    struct IOFileSys io,*iofs=&io;

    struct MsgPort mp,*msgport=&mp;
    
    static const struct TagItem tags[]={ { TAG_END, 0 } };

    /* Prepare message port */
    if(me->pr_Task.tc_Node.ln_Type==NT_PROCESS)
	msgport=&me->pr_MsgPort;
    else
    {
	msgport->mp_Node.ln_Type=NT_MSGPORT;
	msgport->mp_Flags=PA_SIGNAL;
	msgport->mp_SigBit=SIGB_DOS;
	msgport->mp_SigTask=&me->pr_Task;
	NEWLIST(&msgport->mp_MsgList);
    }

    ret=(struct FileHandle *)AllocDosObject(DOS_FILEHANDLE,(struct TagItem *)tags);
    if(ret==NULL)
    {
	me->pr_Result2=ERROR_NO_FREE_STORE;
	return 0;
    }

    /* Prepare I/O request. */
    iofs->IOFS.io_Message.mn_Node.ln_Type=NT_REPLYMSG;
    iofs->IOFS.io_Message.mn_ReplyPort   =msgport;
    iofs->IOFS.io_Message.mn_Length      =sizeof(struct IOFileSys);
    iofs->IOFS.io_Flags=0;

    switch(accessMode)
    {
	case MODE_OLDFILE:
	    flags=LMF_FILE|LMF_WRITE|LMF_READ;
	    ast=con=me->pr_CIS;
	    break;
	case MODE_NEWFILE:
	    flags=LMF_FILE|LMF_LOCK|LMF_CREATE|LMF_CLEAR|LMF_WRITE|LMF_READ;
	    con=me->pr_COS;
	    ast=me->pr_CES?me->pr_CES:me->pr_COS;
	    break;
	case MODE_READWRITE:
	    flags=LMF_FILE|LMF_CREATE|LMF_WRITE|LMF_READ;
	    con=me->pr_COS;
	    ast=me->pr_CES?me->pr_CES:me->pr_COS;
	    break;
	case SHARED_LOCK:
	    flags=0;
	    break;
	case EXCLUSIVE_LOCK:
	    flags=LMF_LOCK;
	    break;
	default:
	    me->pr_Result2=ERROR_ACTION_NOT_KNOWN;
	    return 0;
    }

    if(!Stricmp(name,"CONSOLE:"))
    {
        cur=con;
        volname=NULL;
        pathname="";
    }else if(!Stricmp(name,"*"))
    {
    	cur=ast;
    	volname=NULL;
        pathname="";
    }else if(!Strnicmp(name,"PROGDIR:",8))
    {
        cur=me->pr_HomeDir;
        volname=NULL;
        pathname=name+8;
    }else
    {
        /* Copy volume name */
        cur=me->pr_CurrentDir;
        s1=name;
        pathname=name;
        volname=NULL;
        while(*s1)
    	    if(*s1++==':')
    	    {
	        volname=(STRPTR)AllocMem(s1-name,MEMF_ANY);
	        if(volname==NULL)
	        {
	            me->pr_Result2=ERROR_NO_FREE_STORE;
		    return 0;
	        }
	        CopyMem(name,volname,s1-name-1);
	        volname[s1-name-1]=0;
	        pathname=s1;
	        break;
	    }
    }

    dl=LockDosList(LDF_ALL|LDF_READ);
    if(volname!=NULL)
    {
	/* Find logical device */
	dl=FindDosEntry(dl,volname,LDF_DEVICES|LDF_VOLUMES|LDF_ASSIGNS);
	if(dl==NULL)
	{
	    UnLockDosList(LDF_ALL|LDF_READ);
	    FreeMem(volname,s1-name);
	    FreeDosObject(DOS_FILEHANDLE,ret);
	    me->pr_Result2=ERROR_DEVICE_NOT_MOUNTED;
	    return 0;
	}
	device=dl->dol_Device;
	unit  =dl->dol_Unit;
    }else if(cur)
    {
	fh=(struct FileHandle *)BADDR(cur);
	device=fh->fh_Device;
	unit  =fh->fh_Unit;
    }else
    {
	device=DOSBase->dl_NulHandler;
	unit  =DOSBase->dl_NulLock;
    }

    iofs->IOFS.io_Device =device;
    iofs->IOFS.io_Unit   =unit;
    iofs->IOFS.io_Command=FSA_OPEN;
    iofs->io_Args[0]=(LONG)pathname;
    iofs->io_Args[1]=flags;
    iofs->io_Args[2]=0xf;

    /* Send the request. */
    DoIO(&iofs->IOFS);
    
    error=iofs->io_DosError;

    if(dl!=NULL)
	UnLockDosList(LDF_ALL|LDF_READ);

    if(volname!=NULL)
	FreeMem(volname,s1-name);

    if(error)
    {
	me->pr_Result2=error;
	FreeDosObject(DOS_FILEHANDLE,ret);
	return 0;
    }

    ret->fh_Device=iofs->IOFS.io_Device;
    ret->fh_Unit  =iofs->IOFS.io_Unit;
    return MKBADDR(ret);
    __AROS_FUNC_EXIT
} /* Open */
