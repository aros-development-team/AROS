/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.3  1996/09/13 17:57:05  digulla
    Use IPTR

    Revision 1.2  1996/09/12 14:52:01  digulla
    Use correct way to access external names (was missing)

    Revision 1.1  1996/09/11 12:52:53  digulla
    Two new devices by M. Fleischer: RAM: and NIL:

    Desc:
    Lang:
*/
#include <exec/errors.h>
#include <exec/types.h>
#include <exec/resident.h>
#include <exec/memory.h>
#include <exec/semaphores.h>
#include <clib/exec_protos.h>
#include <utility/tagitem.h>
#include <clib/utility_protos.h>
#include <dos/dosextens.h>
#include <dos/dosasl.h>
#include <dos/exall.h>
#include <dos/filesystem.h>
#include <clib/dos_protos.h>
#include <aros/libcall.h>
#ifdef __GNUC__
    #include "ram_handler_gcc.h"
#endif
#include "machine.h"

#define NEWLIST(l)                          \
((l)->lh_Head=(struct Node *)&(l)->lh_Tail, \
 (l)->lh_Tail=NULL,                         \
 (l)->lh_TailPred=(struct Node *)(l))

extern const char name[];
extern const char version[];
extern const APTR inittabl[4];
extern void *const functable[];
extern const UBYTE datatable;
extern struct rambase *__AROS_SLIB_ENTRY(init,ramdev)();
extern void __AROS_SLIB_ENTRY(open,ramdev)();
extern BPTR __AROS_SLIB_ENTRY(close,ramdev)();
extern BPTR __AROS_SLIB_ENTRY(expunge,ramdev)();
extern int __AROS_SLIB_ENTRY(null,ramdev)();
extern void __AROS_SLIB_ENTRY(beginio,ramdev)();
extern LONG __AROS_SLIB_ENTRY(abortio,ramdev)();
extern void deventry();
extern const char end;

/* Device node */
struct cnode
{
    struct MinNode node;
    LONG type;			/* ST_LINKDIR */
    char *name; 		/* Link's name */
    struct cnode *self; 	/* Pointer to top of structure */
    struct hnode *link; 	/* NULL */
    LONG usecount;		/* >0 usecount locked:+(~0ul/2+1) */
    ULONG protect;		/* 0 */
    char *comment;		/* NULL */
    struct vnode *volume;	/* Pointer to volume */
    struct DosList *doslist;	/* Pointer to doslist entry */
};

/* Volume node */
struct vnode
{
    struct MinNode node;
    LONG type;			/* ST_USERDIR */
    char *name; 		/* Directory name */
    struct vnode *self; 	/* Points to top of structure */
    struct hnode *link; 	/* This one is linked to me */
    LONG usecount;		/* >0 usecount locked:+(~0ul/2+1) */
    ULONG protect;		/* 0 */
    char *comment;		/* NULL */
    struct MinList list;	/* Contents of directory */
    ULONG volcount;		/* number of handles on this volume */
    struct DosList *doslist;	/* Pointer to doslist entry */
};

/* Directory node */
struct dnode
{
    struct MinNode node;
    LONG type;			/* ST_USERDIR */
    char *name; 		/* Directory name */
    struct vnode *volume;	/* Volume's root directory */
    struct hnode *link; 	/* This one is linked to me */
    LONG usecount;		/* >0 usecount locked:+(~0ul/2+1) */
    ULONG protect;		/* protection bits */
    char *comment;		/* Some comment */
    struct MinList list;	/* Contents of directory */
};

/* File node */
struct fnode
{
    struct MinNode node;
    LONG type;			/* ST_FILE */
    char *name; 		/* Filename */
    struct vnode *volume;	/* Volume's root directory */
    struct hnode *link; 	/* This one is linked to me */
    LONG usecount;		/* >0 usecount locked:+(~0ul/2+1) */
    ULONG protect;		/* protection bits */
    char *comment;		/* Some file comment */
    LONG size;			/* Filesize */
    UBYTE *blocks[16];		/* Upto 0x1000 bytes */
    UBYTE **iblocks[4]; 	/* Upto 0x41000 bytes */
    UBYTE ***i2block;		/* Upto 0x1041000 bytes */
    UBYTE ****i3block;		/* Upto 0x101041000 bytes */
};

/* Softlink node */
struct snode
{
    struct MinNode node;
    LONG type;			/* ST_SOFTLINK */
    char *name; 		/* Link's name */
    struct vnode *volume;	/* Volume's root directory */
    struct hnode *link; 	/* This one is hardlinked to me */
    LONG usecount;		/* >0 usecount locked:+(~0ul/2+1) */
    ULONG protect;		/* protection bits */
    char *comment;		/* Some file comment */
    char *contents;		/* Contents of soft link */
};

/* Hardlink node */
struct hnode
{
    struct MinNode node;
    LONG type;			/* ST_LINKDIR */
    char *name; 		/* Link's name */
    struct vnode *volume;	/* Volume's root directory */
    struct hnode *link; 	/* This one is hardlinked to me */
    LONG usecount;		/* >0 usecount locked:+(~0ul/2+1) */
    ULONG protect;		/* protection bits */
    char *comment;		/* Some file comment */
    struct hnode *orig; 	/* original object */
};

#define BLOCKSIZE	256
#define PBLOCKSIZE	(256*sizeof(UBYTE *))

struct filehandle
{
    struct dnode *node;
    IPTR position;
};

int entry(void)
{
    /* If the device was executed by accident return error code. */
    return -1;
}

const struct Resident resident=
{
    RTC_MATCHWORD,
    (struct Resident *)&resident,
    (APTR)&end,
    RTF_AUTOINIT,
    1,
    NT_LIBRARY,
    0,
    (char *)name,
    (char *)&version[6],
    (ULONG *)inittabl
};

const char name[]="ram.handler";

const char version[]="$VER: ram handler 1.0 (28.3.96)\n\015";

const APTR inittabl[4]=
{
    (APTR)sizeof(struct rambase),
    (APTR)functable,
    (APTR)&datatable,
    &__AROS_SLIB_ENTRY(init,ramdev)
};

void *const functable[]=
{
    &__AROS_SLIB_ENTRY(open,ramdev),
    &__AROS_SLIB_ENTRY(close,ramdev),
    &__AROS_SLIB_ENTRY(expunge,ramdev),
    &__AROS_SLIB_ENTRY(null,ramdev),
    &__AROS_SLIB_ENTRY(beginio,ramdev),
    &__AROS_SLIB_ENTRY(abortio,ramdev),
    (void *)-1
};

const UBYTE datatable=0;

__AROS_LH2(struct rambase *, init,
 __AROS_LHA(struct rambase *, rambase, D0),
 __AROS_LHA(BPTR,             segList,   A0),
	   struct ExecBase *, SysBase, 0, ramdev)
{
    __AROS_FUNC_INIT

    /* This function is single-threaded by exec by calling Forbid. */

    struct MsgPort *port;
    struct Task *task;
    struct SignalSemaphore *semaphore;
    APTR stack;

    /* Store arguments */
    rambase->sysbase=SysBase;
    rambase->seglist=segList;
    NEWLIST((struct List *)&rambase->waitdoslist);
    rambase->dosbase=(struct DosLibrary *)OpenLibrary("dos.library",39);
    if(rambase->dosbase!=NULL)
    {
	rambase->utilitybase=(struct UtilityBase *)OpenLibrary("utility.library",39);
	if(rambase->utilitybase!=NULL)
	{
	    port=(struct MsgPort *)AllocMem(sizeof(struct MsgPort),MEMF_PUBLIC|MEMF_CLEAR);
	    if(port!=NULL)
	    {
		rambase->port=port;
		NEWLIST(&port->mp_MsgList);
		port->mp_Node.ln_Type=NT_MSGPORT;
		port->mp_SigBit=SIGB_SINGLE;

		task=(struct Task *)AllocMem(sizeof(struct Task),MEMF_PUBLIC|MEMF_CLEAR);
		if(task!=NULL)
		{
		    port->mp_SigTask=task;
		    port->mp_Flags=PA_IGNORE;
		    NEWLIST(&task->tc_MemEntry);
		    task->tc_Node.ln_Type=NT_TASK;
		    task->tc_Node.ln_Name="ram.handler task";

		    stack=AllocMem(2048,MEMF_PUBLIC);
		    if(stack!=NULL)
		    {
			task->tc_SPLower=stack;
			task->tc_SPUpper=(BYTE *)stack+2048;
#if STACK_GROWS_DOWNWARDS
			task->tc_SPReg=(BYTE *)task->tc_SPUpper-SP_OFFSET-sizeof(APTR);
			((APTR *)task->tc_SPUpper)[-1]=rambase;
#else
			task->tc_SPReg=(BYTE *)task->tc_SPLower-SP_OFFSET+sizeof(APTR);
			*(APTR *)task->tc_SPLower=rambase;
#endif

			semaphore=(struct SignalSemaphore *)AllocMem(sizeof(struct SignalSemaphore),MEMF_PUBLIC|MEMF_CLEAR);
			if(semaphore!=NULL)
			{
			    rambase->sigsem=semaphore;
			    InitSemaphore(semaphore);

			    if(AddTask(task,deventry,NULL)!=NULL)
				return rambase;

			    FreeMem(semaphore,sizeof(struct SignalSemaphore));
			}
			FreeMem(stack,2048);
		    }
		    FreeMem(task,sizeof(struct Task));
		}
		FreeMem(port,sizeof(struct MsgPort));
	    }
	    CloseLibrary((struct Library *)rambase->utilitybase);
	}
	CloseLibrary((struct Library *)rambase->dosbase);
    }

    return NULL;
    __AROS_FUNC_EXIT
}

/* Use This from now on */
#ifdef SysBase
    #undef SysBase
#endif
#ifdef DOSBase
    #undef DOSBase
#endif
#ifdef UtilityBase
    #undef UtilityBase
#endif
#define SysBase rambase->sysbase
#define DOSBase rambase->dosbase
#define UtilityBase rambase->utilitybase

__AROS_LH3(void, open,
 __AROS_LHA(struct IOFileSys *, iofs, A1),
 __AROS_LHA(ULONG,              unitnum, D0),
 __AROS_LHA(ULONG,              flags, D0),
	   struct rambase *, rambase, 1, ramdev)
{
    __AROS_FUNC_INIT
    /*
	This function is single-threaded by exec by calling Forbid.
	If you break the Forbid() another task may enter this function
	at the same time. Take care.
    */

    /* Keep compiler happy */
    unitnum=0;
    flags=0;

    /* I have one more opener. */
    rambase->device.dd_Library.lib_OpenCnt++;
    rambase->device.dd_Library.lib_Flags&=~LIBF_DELEXP;

    /* Set returncode */
    iofs->IOFS.io_Error=0;

    /* Mark Message as recently used. */
    iofs->IOFS.io_Message.mn_Node.ln_Type=NT_REPLYMSG;
    __AROS_FUNC_EXIT
}

__AROS_LH1(BPTR, close,
 __AROS_LHA(struct IOFileSys *, iofs, A1),
	   struct rambase *, rambase, 2, ramdev)
{
    __AROS_FUNC_INIT
    /*
	This function is single-threaded by exec by calling Forbid.
	If you break the Forbid() another task may enter this function
	at the same time. Take care.
    */

    /* Let any following attemps to use the device crash hard. */
    iofs->IOFS.io_Device=(struct Device *)-1;

    /* I have one fewer opener. */
    if(!--rambase->device.dd_Library.lib_OpenCnt)
    {
	/* Delayed expunge pending? */
	if(rambase->device.dd_Library.lib_Flags&LIBF_DELEXP)
	    /* Then expunge the device */
	    return expunge();
    }
    return 0;
    __AROS_FUNC_EXIT
}

__AROS_LH0(BPTR, expunge, struct rambase *, rambase, 3, ramdev)
{
    __AROS_FUNC_INIT

    BPTR ret;
    /*
	This function is single-threaded by exec by calling Forbid.
	Never break the Forbid() or strange things might happen.
    */

    /* Test for openers. */
    if(rambase->device.dd_Library.lib_OpenCnt)
    {
	/* Set the delayed expunge flag and return. */
	rambase->device.dd_Library.lib_Flags|=LIBF_DELEXP;
	return 0;
    }

    /* Kill device task and free all resources */
    RemTask(rambase->port->mp_SigTask);
    FreeMem(rambase->sigsem,sizeof(struct SignalSemaphore));
    FreeMem(((struct Task *)rambase->port->mp_SigTask)->tc_SPLower,2048);
    FreeMem(rambase->port->mp_SigTask,sizeof(struct Task));
    FreeMem(rambase->port,sizeof(struct MsgPort));
    CloseLibrary((struct Library *)rambase->utilitybase);
    CloseLibrary((struct Library *)rambase->dosbase);

    /* Get rid of the device. Remove it from the list. */
    Remove(&rambase->device.dd_Library.lib_Node);

    /* Get returncode here - FreeMem() will destroy the field. */
    ret=rambase->seglist;

    /* Free the memory. */
    FreeMem((char *)rambase-rambase->device.dd_Library.lib_NegSize,
	    rambase->device.dd_Library.lib_NegSize+rambase->device.dd_Library.lib_PosSize);

    return ret;
    __AROS_FUNC_EXIT
}

__AROS_LH0I(int, null, struct rambase *, rambase, 4, ramdev)
{
    __AROS_FUNC_INIT
    return 0;
    __AROS_FUNC_EXIT
}

__AROS_LH1(void, beginio,
 __AROS_LHA(struct IOFileSys *, iofs, A1),
	   struct rambase *, rambase, 5, ramdev)
{
    __AROS_FUNC_INIT

    /* WaitIO will look into this */
    iofs->IOFS.io_Message.mn_Node.ln_Type=NT_MESSAGE;

    /* Nothing is done quick */
    iofs->IOFS.io_Flags&=~IOF_QUICK;

    /* So let the device task do it */
    PutMsg(rambase->port,&iofs->IOFS.io_Message);

    __AROS_FUNC_EXIT
}

__AROS_LH1(LONG, abortio,
 __AROS_LHA(struct IOFileSys *, iofs, A1),
	   struct rambase *, rambase, 6, ramdev)
{
    __AROS_FUNC_INIT
    return 0;
    __AROS_FUNC_EXIT
}

static STRPTR Strdup(struct rambase *rambase, STRPTR string)
{
    STRPTR s2=string,s3;
    while(*s2++)
	;
    s3=(STRPTR)AllocMem(s2-string,MEMF_ANY);
    if(s3!=NULL)
	CopyMem(string,s3,s2-string);
    return s3;
}

static void Strfree(struct rambase *rambase, STRPTR string)
{
    STRPTR s2=string;
    if(string==NULL)
	return;
    while(*s2++)
	;
    FreeMem(string,s2-string);
}

static LONG startup(struct rambase *rambase, STRPTR name, struct TagItem *args)
{
    struct filehandle *fhv, *fhc;
    struct DosList *dlv, *dlc;
    struct cnode *dev;
    struct vnode *vol;

    /* Get compiler happy */
    args=NULL;

    fhv=(struct filehandle*)AllocMem(sizeof(struct filehandle),MEMF_CLEAR);
    if(fhv!=NULL)
    {
	vol=(struct vnode *)AllocMem(sizeof(struct vnode),MEMF_CLEAR);
	if(vol!=NULL)
	{
	    vol->name=Strdup(rambase,"Ram Disk");
	    if(vol->name!=NULL)
	    {
		dlv=MakeDosEntry("Ram Disk",DLT_VOLUME);
		if(dlv!=NULL)
		{
		    fhc=(struct filehandle *)AllocMem(sizeof(struct filehandle),MEMF_CLEAR);
		    if(fhc!=NULL)
		    {
			dev=(struct cnode *)AllocMem(sizeof(struct cnode),MEMF_CLEAR);
			if(dev!=NULL)
			{
			    dev->name=Strdup(rambase,name);
			    if(dev->name!=NULL)
			    {
				dlc=MakeDosEntry(name,DLT_DEVICE);
				if(dlc!=NULL)
				{
				    vol->type=ST_USERDIR;
				    vol->protect=FMF_READ|FMF_WRITE;
				    vol->self=vol;
				    vol->doslist=dlv;
				    NEWLIST((struct List *)&vol->list);
				    fhv->node=(struct dnode *)vol;
				    dlv->dol_Unit  =(struct Unit *)fhv;
				    dlv->dol_Device=&rambase->device;
				    dev->type=ST_LINKDIR;
				    dev->self=dev;
				    dev->volume=vol;
				    dev->doslist=dlc;
				    fhc->node=(struct dnode *)dev;
				    dlc->dol_Unit  =(struct Unit *)fhc;
				    dlc->dol_Device=&rambase->device;
				    if(AddDosEntry(dlv))
				    {
					if(AddDosEntry(dlc))
					{
					    rambase->unitcount++;
					    return 0;
					}
					RemDosEntry(dlv);
				    }
				    FreeDosEntry(dlc);
				}
				Strfree(rambase,dev->name);
			    }
			    FreeMem(dev,sizeof(struct cnode));
			}
			FreeMem(fhc,sizeof(struct filehandle));
		    }
		    FreeDosEntry(dlv);
		}
		Strfree(rambase,vol->name);
	    }
	    FreeMem(vol,sizeof(struct vnode));
	}
	FreeMem(fhv,sizeof(struct filehandle));
    }
    return ERROR_NO_FREE_STORE;
}

static LONG getblock(struct rambase *rambase, struct fnode *file, LONG block, int mode, UBYTE **result)
{
    ULONG a, i;
    UBYTE **p, **p2;

    if(block<0x10)
    {
	p=&file->blocks[block];
	block=0;
	i=0;
    }else if(block<0x410)
    {
	block-=0x10;
	p=(UBYTE **)&file->iblocks[block/0x100];
	block&=0xff;
	i=1;
    }else if(block<0x10410)
    {
	block-=0x410;
	p=(UBYTE **)&file->i2block;
	i=2;
    }else
    {
	block-=0x10410;
	p=(UBYTE **)&file->i3block;
	i=3;
    }
    switch(mode)
    {
	case -1:
	    p2=(UBYTE **)*p;
	    if(!block)
		*p=NULL;
	    p=p2;
	    while(i--&&p!=NULL)
	    {
		a=(block>>i*8)&0xff;
		p2=(UBYTE **)p[a];
		if(!(block&((1<<i*8)-1)))
		{
		    p[a]=NULL;
		    if(!a)
			FreeMem(p,PBLOCKSIZE);
		}
		p=p2;
	    }
	    if(p!=NULL)
		FreeMem(p,BLOCKSIZE);
	    break;
	case 0:
	    p=(UBYTE **)*p;
	    while(i--&&p!=NULL)
		p=((UBYTE ***)p)[(block>>i*8)&0xff];
	    *result=(UBYTE *)p;
	    break;
	case 1:
	    while(i--)
	    {
		if(*p==NULL)
		{
		    *p=AllocMem(PBLOCKSIZE,MEMF_CLEAR);
		    if(*p==NULL)
			return ERROR_NO_FREE_STORE;
		}
		p=(UBYTE **)*p+((block>>i*8)&0xff);
	    }
	    if(*p==NULL)
	    {
		*p=AllocMem(BLOCKSIZE,MEMF_CLEAR);
		if(*p==NULL)
		    return ERROR_NO_FREE_STORE;
	    }
	    *result=*p;
	    break;
    }
    return 0;
}

static void zerofill(UBYTE *address, ULONG size)
{
    while(size--)
	*address++=0;
}

static void shrinkfile(struct rambase *rambase, struct fnode *file, LONG size)
{
    ULONG blocks, block;
    UBYTE *p;

    blocks=(size+BLOCKSIZE-1)/BLOCKSIZE;
    block =(file->size+BLOCKSIZE-1)/BLOCKSIZE;
    for(;block-->blocks;)
	(void)getblock(rambase,file,block,-1,&p);
    if(size&0xff)
    {
	(void)getblock(rambase,file,size,0,&p);
	if(p!=NULL)
	    zerofill(p+(size&0xff),-size&0xff);
    }
    file->size=size;
}

static void delete(struct rambase *rambase, struct fnode *file)
{
    struct hnode *link, *new, *more;
    struct Node *node;

    Strfree(rambase,file->name);
    Strfree(rambase,file->comment);
    Remove((struct Node *)file);

    if(file->type==ST_LINKDIR)
    {
	/* It is a link. Remove it from the chain. */
	link=((struct hnode *)file)->orig;
	((struct hnode *)file)->orig=NULL;
	file->type=link->type;
	while((struct fnode *)link->link!=file)
	    link=link->link;
	link->link=file->link;
    }else if(file->link!=NULL)
    {
	/* If there is a hard link to the object make the link the original */
	link=file->link;
	link->type=file->type;
	more=new->link;
	while(more!=NULL)
	{
	    more->orig=new;
	    more=more->link;
	}
	switch(file->type)
	{
	    case ST_USERDIR:
		while((node=RemHead((struct List *)&((struct dnode *)file)->list))!=NULL)
		    AddTail((struct List *)&((struct dnode *)file)->list,node);
		break;
	    case ST_FILE:
		CopyMemQuick(&file->size,&((struct fnode *)new)->size,sizeof(struct fnode)-offsetof(struct fnode,size));
		zerofill((UBYTE *)&file->size,sizeof(struct fnode)-offsetof(struct fnode,size));
		break;
	    case ST_SOFTLINK:
		((struct snode *)new)->contents=((struct snode *)file)->contents;
		((struct snode *)file)->contents=NULL;
		break;
	}
    }
    switch(file->type)
    {
	case ST_USERDIR:
	    FreeMem(file,sizeof(struct dnode));
	    return;
	case ST_FILE:
	    shrinkfile(rambase,file,0);
	    FreeMem(file,sizeof(struct fnode));
	    return;
	case ST_SOFTLINK:
	    Strfree(rambase,((struct snode *)file)->contents);
	    FreeMem(file,sizeof(struct snode));
	    return;
    }
}

static int fstrcmp(struct rambase *rambase, char *s1, char *s2)
{
    for(;;)
    {
	if(ToLower(*s1)!=ToLower(*s2))
	    return *s1||*s2!='/';
	if(!*s1)
	    return 0;
	s1++; s2++;
    }
}

static LONG findname(struct rambase *rambase, STRPTR *name, struct dnode **dnode)
{
    struct dnode *cur=*dnode;
    char *rest=*name;

    for(;;)
    {
	if(cur->type==ST_LINKDIR)
	    cur=(struct dnode *)((struct hnode *)cur)->orig;
	if(!*rest)
	    break;
	if(*rest=='/')
	{
	    if((struct dnode *)cur->volume==cur)
		return ERROR_OBJECT_NOT_FOUND;
	    while(cur->node.mln_Pred!=NULL)
		cur=(struct dnode *)cur->node.mln_Pred;
	    cur=(struct dnode *)((BYTE *)cur-offsetof(struct dnode,list));
	}else
	{
	    if(cur->type==ST_SOFTLINK)
	    {
		*dnode=cur;
		*name=rest;
		return ERROR_IS_SOFT_LINK;
	    }
	    if(cur->type!=ST_USERDIR)
		return ERROR_DIR_NOT_FOUND;
	    *dnode=cur;
	    cur=(struct dnode *)cur->list.mlh_Head;
	    for(;;)
	    {
		if(cur->node.mln_Succ==NULL)
		{
		    *name=rest;
		    return ERROR_OBJECT_NOT_FOUND;
		}
		if(!fstrcmp(rambase,cur->name,rest))
		    break;
		cur=(struct dnode *)cur->node.mln_Succ;
	    }
	}
	while(*rest)
	    if(*rest++=='/')
		    break;
    }
    *dnode=cur;
    return 0;
}

static LONG set_file_size(struct rambase *rambase, struct filehandle *handle, LONG *offl, LONG *offh, LONG mode)
{
    struct fnode *file=(struct fnode *)handle->node;
    LONG size=*offl;

    if((size<0?-1:0)!=*offh)
	return ERROR_SEEK_ERROR;

    if((size<0?-1:0)!=*offh)
	return ERROR_SEEK_ERROR;

    if(file->type!=ST_FILE)
	return ERROR_OBJECT_WRONG_TYPE;
    switch(mode)
    {
	case OFFSET_BEGINNING:	break;
	case OFFSET_CURRENT:	size+=handle->position; break;
	case OFFSET_END:	size+=file->size; break;
	default:		return ERROR_NOT_IMPLEMENTED;
    }
    if(size<0)
	return ERROR_SEEK_ERROR;
    if(size<file->size)
	shrinkfile(rambase,file,size);
    file->size=*offl=size;
    *offh=0;
    return 0;
}

static LONG read(struct rambase *rambase, struct filehandle *handle, APTR buffer, LONG *numbytes)
{
    struct fnode *file=(struct fnode *)handle->node;
    ULONG num =*numbytes;
    ULONG size=file->size;
    ULONG block, offset;
    UBYTE *buf=buffer, *p;

    if(handle->position>=size)
	num=0;
    else if(handle->position+num>size)
	num=size-handle->position;
    block =handle->position/BLOCKSIZE;
    offset=handle->position&(BLOCKSIZE-1);
    size  =BLOCKSIZE-offset;
    while(num)
    {
	if(size>num)
	    size=num;
	(void)getblock(rambase,file,block,0,&p);
	if(p!=NULL)
	    CopyMem(p+offset,buffer,size);
	else
	    zerofill(buffer,size);
	buffer+=size;
	num   -=size;
	block++;
	offset=0;
	size=BLOCKSIZE;
    }
    *numbytes=(UBYTE *)buffer-buf;
    handle->position+=*numbytes;
    return 0;
}

static LONG write(struct rambase *rambase, struct filehandle *handle, UBYTE *buffer, LONG *numbytes)
{
    struct fnode *file=(struct fnode *)handle->node;
    ULONG num =*numbytes;
    ULONG size=file->size;
    ULONG block, offset;
    UBYTE *buf=buffer, *p;
    LONG error=0;

    if((LONG)(handle->position+num)<0)
	return ERROR_OBJECT_TOO_LARGE;
    block =handle->position/BLOCKSIZE;
    offset=handle->position&(BLOCKSIZE-1);
    size  =BLOCKSIZE-offset;
    while(num)
    {
	if(size>num)
	    size=num;
	error=getblock(rambase,file,block,1,&p);
	if(error)
	    break;
	CopyMem(buffer,p+offset,size);
	buffer+=size;
	num   -=size;
	block++;
	offset=0;
	size=BLOCKSIZE;
    }
    *numbytes=(UBYTE *)buffer-buf;
    handle->position+=*numbytes;
    if(handle->position>file->size)
	file->size=handle->position;
    return error;
}

static LONG lock(struct dnode *dir, ULONG mode)
{
    if((mode&FMF_EXECUTE)&&!(dir->protect&FMF_EXECUTE))
	return ERROR_NOT_EXECUTABLE;
    if((mode&FMF_WRITE)&&!(dir->protect&FMF_WRITE))
	return ERROR_WRITE_PROTECTED;
    if((mode&FMF_READ)&&!(dir->protect&FMF_READ))
	return ERROR_READ_PROTECTED;
    if(mode&FMF_LOCK)
    {
	if(dir->usecount)
	    return ERROR_OBJECT_IN_USE;
	dir->usecount=~0ul/2+1;
    }else
	if(dir->usecount<0)
	    return ERROR_OBJECT_IN_USE;
    dir->usecount++;
    dir->volume->volcount++;
    return 0;
}

static LONG open_(struct rambase *rambase, struct filehandle **handle, STRPTR name, ULONG mode)
{
    struct dnode *dir=(*handle)->node;
    struct filehandle *fh;
    LONG error;

    fh=(struct filehandle *)AllocMem(sizeof(struct filehandle),MEMF_CLEAR);
    if(fh!=NULL)
    {
	error=findname(rambase,&name,&dir);
	if(!error)
	{
	    error=lock(dir,mode);
	    if(!error)
	    {
		fh->node=dir;
		*handle=fh;
		return 0;
	    }
	}
	FreeMem(fh,sizeof(struct filehandle));
    }else
	error=ERROR_NO_FREE_STORE;
    return error;
}

static LONG open_file(struct rambase *rambase, struct filehandle **handle, STRPTR name, ULONG mode, ULONG protect)
{
    struct dnode *dir=(*handle)->node;
    struct filehandle *fh;
    LONG error;

    fh=AllocMem(sizeof(struct filehandle),MEMF_CLEAR);
    if(fh!=NULL)
    {
	error=findname(rambase,&name,&dir);
	if((mode&FMF_CREATE)&&error==ERROR_OBJECT_NOT_FOUND)
	{
	    char *s=name;
	    struct fnode *file;
	    while(*s)
		if(*s++=='/')
		    return error;
	    file=(struct fnode *)AllocMem(sizeof(struct fnode),MEMF_CLEAR);
	    if(file!=NULL)
	    {
		file->name=Strdup(rambase,name);
		if(file->name!=NULL)
		{
		    file->type=ST_FILE;
		    file->protect=protect;
		    file->volume=dir->volume;
		    AddTail((struct List *)&dir->list,(struct Node *)file);
		    error=lock((struct dnode *)file,mode);
		    if(!error)
		    {
			fh->node=dir;
			*handle=fh;
			return 0;
		    }
		    Strfree(rambase,file->name);
		}
		FreeMem(file,sizeof(struct fnode));
	    }
	    error=ERROR_NO_FREE_STORE;
	}else if(!error)
	{
	    if(dir->type!=ST_FILE)
		error=ERROR_OBJECT_WRONG_TYPE;
	    else
	    {
		error=lock(dir,mode);
		if(!error)
		{
		    fh->node=dir;
		    *handle=fh;
		    return 0;
		}
	    }
	}
	FreeMem(fh,sizeof(struct filehandle));
    }
    return error;
}

static LONG create_dir(struct rambase *rambase, struct filehandle **handle, STRPTR name, ULONG protect)
{
    struct dnode *dir=(*handle)->node, *new;
    struct filehandle *fh;
    STRPTR s;
    LONG error;

    error=findname(rambase,&name,&dir);
    if(!error)
	return ERROR_OBJECT_EXISTS;
    if(error!=ERROR_OBJECT_NOT_FOUND)
	return error;
    s=name;
    while(*s)
	if(*s++=='/')
	    return error;
    fh=AllocMem(sizeof(struct filehandle),MEMF_CLEAR);
    if(fh!=NULL)
    {
	new=(struct dnode *)AllocMem(sizeof(struct dnode),MEMF_CLEAR);
	if(new!=NULL)
	{
	    new->name=Strdup(rambase,name);
	    if(new->name!=NULL)
	    {
		new->type=ST_USERDIR;
		new->protect=protect;
		new->volume=dir->volume;
		new->volume->volcount++;
		new->usecount=~0ul/2+2;
		NEWLIST((struct List *)&new->list);
		AddTail((struct List *)&dir->list,(struct Node *)new);
		fh->node=new;
		*handle=fh;
		return 0;
	    }
	    FreeMem(new,sizeof(struct dnode));
	}
	FreeMem(fh,sizeof(struct filehandle));
    }
    return ERROR_NO_FREE_STORE;
}

static LONG free_lock(struct rambase *rambase, struct filehandle *filehandle)
{
    struct dnode *dnode=filehandle->node;
    dnode->usecount=(dnode->usecount-1)&~0ul/2;
    FreeMem(filehandle,sizeof(struct filehandle));
    dnode->volume->volcount--;
    return 0;
}

static LONG seek(struct rambase *rambase, struct filehandle *filehandle, LONG *posh, LONG *posl, LONG mode)
{
    struct fnode *file=(struct fnode *)filehandle->node;
    LONG pos=*posl;

    if((pos<0?-1:0)!=*posh)
	return ERROR_SEEK_ERROR;
    if(file->type!=ST_FILE)
	return ERROR_OBJECT_WRONG_TYPE;
    switch(mode)
    {
	case OFFSET_BEGINNING:	break;
	case OFFSET_CURRENT:	pos+=filehandle->position; break;
	case OFFSET_END:	pos+=file->size; break;
	default:		return ERROR_NOT_IMPLEMENTED;
    }
    if(pos<0)
	return ERROR_SEEK_ERROR;
    *posh=0;
    *posl=filehandle->position;
    filehandle->position=pos;
    return 0;
}

static const ULONG sizes[]=
{ 0, offsetof(struct ExAllData,ed_Type), offsetof(struct ExAllData,ed_Size),
  offsetof(struct ExAllData,ed_Prot), offsetof(struct ExAllData,ed_Days),
  offsetof(struct ExAllData,ed_Comment), offsetof(struct ExAllData,ed_OwnerUID),
  sizeof(struct ExAllData)
};

static LONG examine(struct fnode *file, struct ExAllData *ead, ULONG size, ULONG type)
{
    STRPTR next, end, name;
    if(type>ED_OWNER)
	return ERROR_BAD_NUMBER;
    next=(STRPTR)ead+sizes[type];
    end=(STRPTR)ead+size;
    switch(type)
    {
	case ED_OWNER:
	    ead->ed_OwnerUID=0;
	    ead->ed_OwnerGID=0;
	case ED_COMMENT:
	    if(file->comment!=NULL)
	    {
		ead->ed_Comment=next;
		name=file->comment;
		for(;;)
		{
		    if(next>=end)
			return ERROR_BUFFER_OVERFLOW;
		    if(!(*next++=*name++))
			break;
		}
	    }else
		ead->ed_Comment=NULL;
	case ED_DATE:
	    ead->ed_Days=0;
	    ead->ed_Mins=0;
	    ead->ed_Ticks=0;
	case ED_PROTECTION:
	    ead->ed_Prot=file->protect;
	case ED_SIZE:
	    ead->ed_Size=file->size;
	case ED_TYPE:
	    ead->ed_Type=file->type;
	    if(((struct vnode *)file)->self==(struct vnode *)file)
		ead->ed_Type=ST_ROOT;
	case ED_NAME:
	    ead->ed_Name=next;
	    name=file->name;
	    for(;;)
	    {
		if(next>=end)
		    return ERROR_BUFFER_OVERFLOW;
		if(!(*next++=*name++))
		    break;
	    }
	case 0:
	    ead->ed_Next=(struct ExAllData *)(((IPTR)next+PTRALIGN-1)&~(PTRALIGN-1));
    }
    return 0;
}

static LONG examine_all(struct filehandle *dir, struct ExAllData *ead, ULONG size, ULONG type)
{
    STRPTR end;
    struct ExAllData *last=NULL;
    struct fnode *ent;
    LONG error;
    end=(STRPTR)ead+size;
    if(dir->node->type!=ST_USERDIR)
	return ERROR_OBJECT_WRONG_TYPE;
    ent=(struct fnode *)dir->position;
    if(ent==NULL)
    {
	ent=(struct fnode *)dir->node->list.mlh_Head;
	ent->usecount++;
    }
    if(ent->node.mln_Succ==NULL)
	return ERROR_NO_MORE_ENTRIES;
    ent->usecount--;
    do
    {
	error=examine(ent,ead,end-(STRPTR)ead,type);
	if(error==ERROR_BUFFER_OVERFLOW)
	{
	    if(last==NULL)
		return error;
	    ent->usecount++;
	    last->ed_Next=NULL;
	    dir->position=(IPTR)ent;
	    return 0;
	}
	last=ead;
	ead=ead->ed_Next;
	ent=(struct fnode *)ent->node.mln_Succ;
    }while(ent->node.mln_Succ!=NULL);
    last->ed_Next=NULL;
    dir->position=(IPTR)ent;
    return 0;
}

static LONG delete_object(struct rambase *rambase, struct filehandle *filehandle, STRPTR name)
{
    struct dnode *file=filehandle->node;
    LONG error;
    error=findname(rambase,&name,&file);
    if(error)
	return error;
    if((struct dnode *)file->volume==file)
	return ERROR_OBJECT_WRONG_TYPE;
    if(file->usecount)
	return ERROR_OBJECT_IN_USE;
    if(!(file->protect&FIBF_DELETE))
	return ERROR_DELETE_PROTECTED;
    if(file->type==ST_USERDIR&&file->list.mlh_Head->mln_Succ!=NULL)
	return ERROR_DIRECTORY_NOT_EMPTY;
    delete(rambase,(struct fnode *)file);
    return 0;
}

LONG die(struct rambase *rambase, struct filehandle *handle)
{
    struct cnode *dev;
    struct vnode *vol;
    struct dnode *dir;
    struct fnode *file;

    dev=(struct cnode *)handle->node;
    free_lock(rambase,handle);
    if(dev->type!=ST_LINKDIR||dev->self!=dev)
	return ERROR_OBJECT_WRONG_TYPE;
    vol=dev->volume;
    if(vol->volcount)
	return ERROR_OBJECT_IN_USE;

    RemDosEntry(vol->doslist);
    FreeDosEntry(vol->doslist);
    RemDosEntry(dev->doslist);
    FreeDosEntry(dev->doslist);

    while(vol->list.mlh_Head->mln_Succ!=NULL)
    {
	dir=(struct dnode *)vol->list.mlh_Head;
	if(dir->type==ST_USERDIR)
	    while((file=(struct fnode *)RemHead((struct List *)&dir->list))!=NULL)
		AddTail((struct List *)&vol->list,(struct Node *)dir);
	delete(rambase,(struct fnode *)dir);
    }
    Strfree(rambase,vol->name);
    FreeMem(vol,sizeof(struct vnode));

    Strfree(rambase,dev->name);
    FreeMem(dev,sizeof(struct cnode));
    rambase->unitcount--;
    return 0;
}

void deventry(struct rambase *rambase)
{
    struct IOFileSys *iofs;
    struct dnode *dir;
    LONG error=0;
    /*
	Init device port. AllocSignal() cannot fail because this is a
	freshly created task with all signal bits still free.
    */
    rambase->port->mp_SigBit=AllocSignal(-1);
    rambase->port->mp_Flags=PA_SIGNAL;

    /* Get and process the messages. */
    for(;;)
    {
	while((iofs=(struct IOFileSys *)GetMsg(rambase->port))!=NULL)
	{
	    switch(iofs->IOFS.io_Command)
	    {
		case FSA_MOUNT:
		    /*
			mount a new dos device (a new filesystem)
			Unit *root;    root handle on return
			STRPTR name;   device name without colon
			ULONG *args;   further arguments
		    */
		    AddTail((struct List *)&rambase->waitdoslist,&iofs->IOFS.io_Message.mn_Node);
		    continue;

		case FSA_DISMOUNT:
		    /*
			Free the lock then try to dismount dos device
			Unit *root;    handle to device's root directory
		    */
		    AddTail((struct List *)&rambase->waitdoslist,&iofs->IOFS.io_Message.mn_Node);
		    continue;

		case FSA_OPEN:
		    /*
			get handle on a file or directory
			Unit *current; current directory / new handle on return
			STRPTR name;   file- or directoryname
			LONG mode;     open mode
		    */
		    error=open_(rambase,(struct filehandle **)&iofs->IOFS.io_Unit,
				      (STRPTR)iofs->io_Args[0], iofs->io_Args[1]);
		    break;

		case FSA_OPEN_FILE:
		    /*
			open a file or create a new one
			Unit *current; current directory / new handle on return
			STRPTR name;   file- or directoryname
			LONG mode;     open mode
			LONG protect;  protection flags if a new file is created
		    */
		    error=open_file(rambase,(struct filehandle **)&iofs->IOFS.io_Unit,
				      (STRPTR)iofs->io_Args[0], iofs->io_Args[1],
				      iofs->io_Args[2]);
		    break;

		case FSA_READ:
		    /*
			read a number of bytes from a file
			Unit *current; filehandle
			APTR buffer;   data
			LONG numbytes; number of bytes to read /
				       number of bytes read on return,
				       0 if there are no more bytes in the file
		    */
		    error=read(rambase,(struct filehandle *)iofs->IOFS.io_Unit,
			     (APTR)iofs->io_Args[0], &iofs->io_Args[1]);
		    break;

		case FSA_WRITE:
		    /*
			write a number of bytes to a file
			Unit *current; filehandle
			APTR buffer;   data
			LONG numbytes; number of bytes to write /
				       number of bytes written on return
		    */
		    error=write(rambase,(struct filehandle *)iofs->IOFS.io_Unit,
			      (APTR)iofs->io_Args[0], &iofs->io_Args[1]);
		    break;

		case FSA_SEEK:
		    /*
			set / read position in file
			Unit *current; filehandle
			LONG posh;
			LONG posl;     relative position /
				       old position on return
			LONG mode;     one of OFFSET_BEGINNING, OFFSET_CURRENT,
				       OFFSET_END
		    */
		    error=seek(rambase,(struct filehandle *)iofs->IOFS.io_Unit,
			     &iofs->io_Args[0], &iofs->io_Args[1], iofs->io_Args[2]);
		    break;

		case FSA_CLOSE:
		    /*
			get rid of a handle
			Unit *current; filehandle
		    */
		    error=free_lock(rambase,(struct filehandle *)iofs->IOFS.io_Unit);
		    break;

		case FSA_EXAMINE:
		    /*
			Get information about the current object
			Unit *current; current object
			struct ExAllData *ead; buffer to be filled
			ULONG size;    size of the buffer
			ULONG type;    type of information to get
		    */
		    error=examine((struct fnode *)((struct filehandle *)iofs->IOFS.io_Unit)->node,
				  (struct ExAllData*)iofs->io_Args[0],
				  iofs->io_Args[1], iofs->io_Args[2]);
		    break;

		case FSA_EXAMINE_ALL:
		    /*
			Read the current directory
			Unit *current; current directory
			struct ExAllData *ead; buffer to be filled
			ULONG size;    size of the buffer
			ULONG type;    type of information to get
		    */
		    error=examine_all((struct filehandle *)iofs->IOFS.io_Unit,
				      (struct ExAllData*)iofs->io_Args[0],
				      iofs->io_Args[1], iofs->io_Args[2]);
		    break;

		case FSA_CREATE_DIR:
		    /*
			Build lock and open a new directory
			Unit *current; current directory
			STRPTR name;   name of the dir to create
			LONG protect;  Protection flags for the new dir
		    */
		    error=create_dir(rambase,(struct filehandle **)&iofs->IOFS.io_Unit,
				     (STRPTR)iofs->io_Args[0], iofs->io_Args[1]);
		    break;

		case FSA_DELETE_OBJECT:
		    /*
			Delete file or directory
			Unit *current; current directory
			STRPTR name;   filename
		    */
		    error=delete_object(rambase,(struct filehandle *)iofs->IOFS.io_Unit,
					(STRPTR)iofs->io_Args[0]);
		    break;

		case FSA_SET_PROTECT:
		    /*
			Set protection bits for a certain file or directory.
			Unit *current; current directory
			STRPTR name;   filename
			ULONG protect; new protection bits
		    */
		    dir=((struct filehandle *)iofs->IOFS.io_Unit)->node;
		    error=findname(rambase,(STRPTR *)&iofs->io_Args[0],&dir);
		    if(!error)
			dir->protect=iofs->io_Args[1];
		    break;

		case FSA_SET_OWNER:
		    /*
			Set owner and group of the file or directory
			Unit *current; current directory
			STRPTR name;   filename
			ULONG UID;
			ULONG GID;
		    */
		    dir=((struct filehandle *)iofs->IOFS.io_Unit)->node;
		    error=findname(rambase,(STRPTR *)&iofs->io_Args[0],&dir);
		    if(!error)
		    {
		    }
		    break;

		case FSA_SET_DATE:
		    /*
			Set creation date of the file
			Unit *current; current directory
			STRPTR name;   filename
			ULONG days;
			ULONG mins;
			ULONG ticks;   timestamp
		    */
		    dir=((struct filehandle *)iofs->IOFS.io_Unit)->node;
		    error=findname(rambase,(STRPTR *)&iofs->io_Args[0],&dir);
		    if(!error)
		    {
		    }
		    break;

		case FSA_SET_COMMENT:
		    /*
			Set a comment for the file or directory;
			Unit *current; current directory
			STRPTR name;   filename
			STRPTR comment; NUL terminated C string or NULL.
		    */
		    dir=((struct filehandle *)iofs->IOFS.io_Unit)->node;
		    error=findname(rambase,(STRPTR *)&iofs->io_Args[0],&dir);
		    if(!error)
		    {
			if(iofs->io_Args[1])
			{
			    STRPTR s=Strdup(rambase,(STRPTR)iofs->io_Args[0]);
			    if(s!=NULL)
			    {
				Strfree(rambase,dir->comment);
				dir->comment=s;
			    }else
				error=ERROR_NO_FREE_STORE;
			}else
			{
			    Strfree(rambase,dir->comment);
			    dir->comment=NULL;
			}
		    }
		    break;

		case FSA_SET_FILE_SIZE:
		    /*
			Set a new size for the file.
			Unit *file;    filehandle
			LONG offh;
			LONG offl;     offset to current position/
				       new size on return
			LONG mode;     relative to what (see Seek)
		    */
		    error=set_file_size(rambase,(struct filehandle *)iofs->IOFS.io_Unit,
					&iofs->io_Args[0],&iofs->io_Args[1],iofs->io_Args[2]);
		    break;

		default:
		    error=ERROR_NOT_IMPLEMENTED;
		    break;
/*
  FSA_FILE_MODE
    Change or read the mode of a single filehandle
    Unit *current; filehandle to change
    ULONG newmode; new mode/old mode on return
    ULONG mask;    bits affected

  FSA_MOUNT_MODE
    Change or read the mode of the filesystem
    Unit *current; filesystem to change
    ULONG newmode; new mode/old mode on return
    ULONG mask;    bits affected
    STRPTR passwd; password for MMF_LOCKED

  FSA_MAKE_HARDLINK
    Create a hard link on a file, directory or soft link
    Unit *current; current directory
    STRPTR name;   softlink name
    Unit *target;  target handle

  FSA_MAKE_SOFTLINK
    Create a soft link to another object
    Unit *current; current directory
    STRPTR name;   softlink name
    STRPTR target; target name

  FSA_RENAME
  FSA_READ_LINK
  FSA_DISK_INFO
  FSA_SERIALIZE_DISK
  FSA_WAIT_CHAR
  FSA_INFO
  FSA_TIMER
  FSA_DISK_TYPE
  FSA_DISK_CHANGE
  FSA_SAME_LOCK
  FSA_CHANGE_SIGNAL
  FSA_FORMAT
  FSA_IS_FILESYSTEM
  FSA_EXAMINE_ALL
  FSA_EXAMINE_FH
  FSA_ADD_NOTIFY
  FSA_REMOVE_NOTIFY
  FSA_EXAMINE_ALL_END

*/
	    }
	    iofs->io_DosError=error;
	    ReplyMsg(&iofs->IOFS.io_Message);
	}
	if(rambase->waitdoslist.mlh_Head->mln_Succ!=NULL)
	{
	    if(!AttemptLockDosList(LDF_DEVICES|LDF_VOLUMES|LDF_WRITE))
		Delay(TICKS_PER_SECOND/2);
	    else
	    {
		while((iofs=(struct IOFileSys *)RemHead((struct List *)&rambase->waitdoslist))!=NULL)
		{
		    if(iofs->IOFS.io_Command==FSA_MOUNT)
			error=startup(rambase,(STRPTR)iofs->io_Args[0],
				      (struct TagItem *)iofs->io_Args[1]);
		    else
			error=die(rambase,(struct filehandle *)iofs->IOFS.io_Unit);
		    iofs->io_DosError=error;
		    ReplyMsg(&iofs->IOFS.io_Message);
		}
		UnLockDosList(LDF_DEVICES|LDF_VOLUMES|LDF_WRITE);
	    }
	}
#if 0
	if(rambase->iofs!=NULL)
	{
	    iofs=rambase->iofs;
	    if(iofs->IOFS.io_Message.mn_Node.ln_Type==NT_MESSAGE)
	    {
		abort_notify(rambase,iofs);
		iofs->io_DosError=ERROR_BREAK;
		rambase->iofs=NULL;
		ReplyMsg(&iofs->IOFS.io_Message);
	    }else
	    {
		rambase->iofs=NULL;
		Signal(1,0);
	    }
	}
#endif
	Wait(1<<rambase->port->mp_SigBit);
    }
}

const char end=0;
