/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.1  1996/09/11 12:52:54  digulla
    Two new devices by M. Fleischer: RAM: and NIL:

    Revision 1.2  1996/08/01 17:41:23  digulla
    Added standard header for all files

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
#include <dos/filesystem.h>
#include <clib/dos_protos.h>
#include <aros/libcall.h>
#ifdef __GNUC__
    #include "ramdev_gcc.h"
#endif

#define NEWLIST(l)                          \
((l)->lh_Head=(struct Node *)&(l)->lh_Tail, \
 (l)->lh_Tail=NULL,                         \
 (l)->lh_TailPred=(struct Node *)(l))

extern const char name[];
extern const char version[];
extern const APTR inittabl[4];
extern void *const functable[];
extern const UBYTE datatable;
extern struct rambase *ramdev_init();
extern void ramdev_open();
extern BPTR ramdev_close();
extern BPTR ramdev_expunge();
extern int ramdev_null();
extern void ramdev_beginio();
extern LONG ramdev_abortio();
extern void deventry();
extern const char end;

/* Device node */
struct cnode
{
    struct MinNode node;
    LONG type;			/* ST_LINKDIR */
    char *name;			/* Link's name */
    struct cnode *self;		/* Pointer to top of structure */
    struct hnode *link;		/* NULL */
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
    char *name;			/* Directory name */
    struct vnode *self;		/* Points to top of structure */
    struct hnode *link;		/* This one is linked to me */
    LONG usecount;		/* >0 usecount locked:+(~0ul/2+1) */
    ULONG protect;		/* 0 */
    char *comment;		/* NULL */
    struct MinList list;	/* Contents of directory */
    struct MinList waiting;	/* IORequests waiting on dismountage */
    ULONG volcount;		/* number of handles on this volume */
    struct DosList *doslist;	/* Pointer to doslist entry */
};

/* Directory node */
struct dnode
{
    struct MinNode node;
    LONG type;			/* ST_USERDIR */
    char *name;			/* Directory name */
    struct vnode *volume;	/* Volume's root directory */
    struct hnode *link;		/* This one is linked to me */
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
    char *name;			/* Filename */
    struct vnode *volume;	/* Volume's root directory */
    struct hnode *link;		/* This one is linked to me */
    LONG usecount;		/* >0 usecount locked:+(~0ul/2+1) */
    ULONG protect;		/* protection bits */
    char *comment;		/* Some file comment */
    LONG size;			/* Filesize */
    UBYTE *blocks[16];		/* Upto 0x1000 bytes */
    UBYTE **iblocks[4];		/* Upto 0x41000 bytes */
    UBYTE ***i2block;		/* Upto 0x1041000 bytes */
    UBYTE ****i3block;		/* Upto 0x101041000 bytes */
};

/* Softlink node */
struct snode
{
    struct MinNode node;
    LONG type;			/* ST_SOFTLINK */
    char *name;			/* Link's name */
    struct vnode *volume;	/* Volume's root directory */
    struct hnode *link;		/* This one is hardlinked to me */
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
    char *name;			/* Link's name */
    struct vnode *volume;	/* Volume's root directory */
    struct hnode *link;		/* This one is hardlinked to me */
    LONG usecount;		/* >0 usecount locked:+(~0ul/2+1) */
    ULONG protect;		/* protection bits */
    char *comment;		/* Some file comment */
    struct hnode *orig;		/* original object */
};

#define BLOCKSIZE	256
#define PBLOCKSIZE	(256*sizeof(UBYTE *))

struct filehandle
{
    struct dnode *node;
    ULONG position;
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
    &ramdev_init
};

void *const functable[]=
{
    &ramdev_open,
    &ramdev_close,
    &ramdev_expunge,
    &ramdev_null,
    &ramdev_beginio,
    &ramdev_abortio,
    (void *)-1
};

const UBYTE datatable=0;

__AROS_LH2(struct rambase *, init,
 __AROS_LA(struct rambase *, rambase, D0),
 __AROS_LA(BPTR,             segList,   A0),
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
		port->mp_SigBit=SIGF_SINGLE;

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
 __AROS_LA(struct IOFileSys *, iofs, A1),
 __AROS_LA(ULONG,              unitnum, D0),
 __AROS_LA(ULONG,              flags, D0),
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
 __AROS_LA(struct IOFileSys *, iofs, A1),
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
 __AROS_LA(struct IOFileSys *, iofs, A1),
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
 __AROS_LA(struct IOFileSys *, iofs, A1),
	   struct rambase *, rambase, 6, ramdev)
{
    __AROS_FUNC_INIT
#if 0
    if(iofs->IOFS.io_Command==FSA_NOTIFY)
    {
	    ObtainSemaphore(rambase->sigsem);
	    rambase->iofs=iofs;
	    Signal(rambase->port->mp_SigTask,rambase->port->mp_SigBit);
	    while(rambase->iofs!=NULL)
	        Wait(1<<iofs->IOFS.io_Message.mn_ReplyPort->mp_SigBit);
	    ReleaseSemaphore(rambase->sigsem);
    }
#endif
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
	            *p=AllocMem(PBLOCKSIZE,MEMF_ANY);
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
    struct hnode *link;
    
    if(file->type!=ST_LINKDIR&&file->link!=NULL)
    {
        link=file->link;
    	Strfree(rambase,file->name);
    	Strfree(rambase,file->comment);
    	file->name=link->name;
    	file->link=link->link;
    	file->usecount=link->usecount;
    	file->protect=link->protect;
    	file->comment=link->comment;
	Remove((struct Node *)file);
	Insert(NULL,(struct Node *)file,(struct Node *)link);
	Remove((struct Node *)link);
	FreeMem(link,sizeof(struct hnode));
	return;
    }
    Remove((struct Node *)file);
    Strfree(rambase,file->name);
    Strfree(rambase,file->comment);
    switch(file->type)
    {
        case ST_USERDIR:
            FreeMem(file,sizeof(struct dnode));
            return;
        case ST_FILE:
            shrinkfile(rambase,file,0);
            FreeMem(file,sizeof(struct fnode));
            return;
        case ST_LINKDIR:
            link=((struct hnode *)file)->orig;
            while((struct fnode *)link->link!=file)
                link=link->link;
            link->link=file->link;
            FreeMem(file,sizeof(struct hnode));
            return;
        case ST_SOFTLINK:
            Strfree(rambase,((struct snode *)file)->contents);
            return;
    }
}

static int fstrcmp(char *s1, char *s2)
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

static LONG findname(STRPTR *name, struct dnode **dnode)
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
	    cur=(struct dnode *)cur->list.mlh_Head;
	    for(;;)
	    {
		if(cur->node.mln_Succ==NULL)
		    return ERROR_OBJECT_NOT_FOUND;
		if(fstrcmp(cur->name,rest))
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

static LONG set_file_size(struct rambase *rambase, struct filehandle *handle, LONG *offset, LONG mode)
{
    struct fnode *file=(struct fnode *)handle->node;
    LONG size=*offset;
    
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
    file->size=*offset=size;
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

    if(handle->position+num<0)
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
    if(*numbytes)
	return 0;
    return error;
}

static LONG locate_object(struct rambase *rambase, struct filehandle **handle, STRPTR name, ULONG mode)
{
    struct dnode *dir=(*handle)->node;
    struct filehandle *fh;
    LONG error;

    error=findname(&name,&dir);
    if((mode&LMF_CREATE)&&error==ERROR_OBJECT_NOT_FOUND)
    {
        char *s=name;
        struct fnode *file;
	while(*s)
	    if(*s=='/')
		return error;
        fh=AllocMem(sizeof(struct filehandle),MEMF_CLEAR);
        if(fh!=NULL)
        {
            file=(struct fnode *)AllocMem(sizeof(struct fnode),MEMF_CLEAR);
            if(file!=NULL)
            {
                file->name=Strdup(rambase,name);
                if(file->name!=NULL)
                {
                    file->type=ST_FILE;
                    file->protect=FIBF_READ|FIBF_WRITE|FIBF_EXECUTE|FIBF_DELETE;
                    file->volume=dir->volume;
                    file->volume->volcount++;
                    if(mode&LMF_LOCK)
                        file->usecount=~0ul/2+1;
                    file->usecount++;
                    AddTail((struct List *)&dir->list,(struct Node *)file);
                    fh->node=(struct dnode *)file;
                    *handle=fh;
                    return 0;
                }
                FreeMem(file,sizeof(struct fnode));
            }
            FreeMem(fh,sizeof(struct filehandle));
        }
        return ERROR_NO_FREE_STORE;
    }
    if(error)
        return error;
    if((mode&LMF_EXECUTE)&&!(dir->protect&LMF_EXECUTE))
        return ERROR_NOT_EXECUTABLE;
    if((mode&LMF_WRITE)&&!(dir->protect&LMF_WRITE))
	return ERROR_WRITE_PROTECTED;
    if((mode&LMF_READ)&&!(dir->protect&LMF_READ))
	return ERROR_READ_PROTECTED;
    if((mode&LMF_FILE)&&dir->type!=ST_FILE)
	return ERROR_OBJECT_WRONG_TYPE;
    if(mode&LMF_LOCK)
    {
	if(dir->usecount)
	    return ERROR_OBJECT_IN_USE;
	dir->usecount=~0ul/2+1;
    }else
        if(dir->usecount<0)
            return ERROR_OBJECT_IN_USE;
    dir->usecount++;
    fh=(struct filehandle *)AllocMem(sizeof(struct filehandle),MEMF_CLEAR);
    if(fh==NULL)
    {
        dir->usecount=(dir->usecount-1)&~0ul/2;
        return ERROR_NO_FREE_STORE;
    }
    if(mode&LMF_CLEAR)
        shrinkfile(rambase,(struct fnode *)dir,0);
    dir->volume->volcount++;
    fh->node=dir;
    *handle=fh;
    return 0;
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
#if 0
static LONG examine_node(struct fnode *file, ULONG **buffer, ULONG *size, ULONG type, ULONG namesize, ULONG commsize)
{
    ULONG *buf=*buffer;
    ULONG siz=*size/sizeof(ULONG),bsize;
    STRPTR s1=NULL, s2;
    int i, k;
    
    for(i=1;type;i++)
    {
	if(type&1)
	{
	    if(!siz)
	        return ERROR_BUFFER_OVERFLOW;
	    switch(i)
	    {
		case ETB_COMMENT:
		    s1=file->comment;
		    if(s1==NULL)
		    {
		        *buffer++=0;
		        break;
		    }
		    bsize=commsize;
		    /* Fall through */
		case ETB_NAME:
		    if(i==ETB_NAME)
		    {
			s1=file->name;
			bsize=namesize;
		    }
		    s2=(STRPTR)buf;
		    if(bsize)
		    {
		        /* Fixed sized field */
			if(bsize/sizeof(ULONG)>siz)
			    return ERROR_BUFFER_OVERFLOW;
			siz-=bsize/sizeof(ULONG)-1;
			do
			    if(!(*s2++=*s1++))
			        break;
			while(--bsize);
			s2[-1]=0;
		    }else
		        /* Variable sized field */
			for(;;)
			{
		    	    for(k=0;k<sizeof(ULONG);k++)
		    	        if(!(*s2++=*s1++))
		    	        {
		    	            for(;k<sizeof(ULONG);k++)
		    	                *s2++=0;
		    	            break;
		    	        }
		    	    if(!--siz)
		    	        return ERROR_BUFFER_OVERFLOW;
		        }
		    buf=(ULONG*)s2;
		    break;
		case ETB_TYPE:
		    *buf++=file->type;
		    break;
		case ETB_PROTECT:
		    *buf++=file->protect;
		    break;
		case ETB_SIZE:
		    *buf++=file->size;
		    break;
		case ETB_DOSLIST:
		case ETB_SIZEH:
		case ETB_BLOCKSUSED:
		case ETB_BLOCKSUSEDH:
		case ETB_DAYS:
		case ETB_MINUTES:
		case ETB_TICKS:
		case ETB_OWNERIDS:
		case ETB_DISKSTATE:
		case ETB_BYTESPERBLK:
		case ETB_DISKTYPE:
		    *buf++=0;
		    break;
	    }
	    siz--;
	}
	type<<=1;
    }
    *buffer=buf;
    *size=siz*sizeof(ULONG);
    return 0;
}

static LONG examine(struct filehandle *handle, ULONG *buffer, ULONG size, struct TagItem *tags)
{
    ULONG type[16], filesize=0, commsize=0;
    while(tags->ti_Tag!=TAG_END)
    {
        switch(tags->ti_Tag)
        {
            case EXA_TYPE:
                type=tags->ti_Data;
                break;
            case EXA_NAMESIZE:
                filesize=tags->ti_Data;
                break;
            case EXA_COMMENTSIZE:
                commsize=tags->ti_Data;
                break;
            default:
                return ERROR_NOT_IMPLEMENTED;
        }
        tags++;
    }
    return examine_node((struct fnode *)handle->node, &buffer, &size, type, namesize, commsize);
}
#endif
static LONG delete_object(struct rambase *rambase, struct filehandle *filehandle, char *name)
{
    struct fnode *file=(struct fnode *)filehandle->node;
    if((struct fnode *)file->volume==file)
        return ERROR_OBJECT_WRONG_TYPE;
    if(file->usecount)
	return ERROR_OBJECT_IN_USE;
    if(!(file->protect&FIBF_DELETE))
	return ERROR_DELETE_PROTECTED;
    if(file->type==ST_USERDIR&&((struct dnode *)file)->list.mlh_Head->mln_Succ!=NULL)
	return ERROR_DIRECTORY_NOT_EMPTY;
    delete(rambase,file);
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
		case FSA_STARTUP:
		    /*
			mount a new dos device (a new filesystem)
			Unit *root;    root handle on return
			STRPTR name;   device name without colon
			struct TagItem *args; further arguments
		    */
		    AddTail((struct List *)&rambase->waitdoslist,&iofs->IOFS.io_Message.mn_Node);
		    continue;

		case FSA_DIE:
		    /*
			Free the lock then try to dismount dos device
			Unit *root;    handle to device's root directory
		    */
		    AddTail((struct List *)&rambase->waitdoslist,&iofs->IOFS.io_Message.mn_Node);
		    continue;

		case FSA_LOCATE_OBJECT:
		    /*
			get handle on a file or directory
			Unit *current; current directory / new handle on return
			STRPTR name;   file- or directoryname
			LONG mode;     locking mode
			LONG protect;  protection flags if a new file is created
		    */
		    error=locate_object(rambase,(struct filehandle **)&iofs->IOFS.io_Unit,
		    		      (STRPTR)iofs->io_Args[0], iofs->io_Args[1]);
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
		    
		case FSA_FREE_LOCK:
		    /*
			get rid of a handle
			Unit *current; filehandle
		    */
		    error=free_lock(rambase,(struct filehandle *)iofs->IOFS.io_Unit);
		    break;
#if 0
		case FSA_EXAMINE:
		    /*
			Get information about the current object
			Unit *current; current object
			ULONG *buffer;  to be filled
			ULONG size;    size of the buffer
			ULONG type;    type of information to get
		    */
		    error=examine((struct filehandle *)iofs->IOFS.io_Unit,
		                  (ULONG *)iofs->io_Args[0], iofs->io_Args[1],
		                  iofs->io_Args[2]);
		    break;
#endif
/*
  FSA_DELETE_OBJECT - delete file or directory
    Unit *current; current directory
    STRPTR name;   filename

  FSA_MAKE_LINK
    Unit *current; current directory, untouched by the handler
    STRPTR name;   filename
    LONG target;   target handle for hardlinks/target name for softlinks
    LONG mode;     !=0 softlink

  FSA_CREATE_DIR
  FSA_RENAME
  FSA_SET_PROTECT
  FSA_SET_OWNER
  FSA_SET_DATE
  FSA_READ_LINK
  FSA_DISK_INFO
  FSA_SERIALIZE_DISK
  FSA_MORE_CACHE
  FSA_WAIT_CHAR
  FSA_INFO
  FSA_FLUSH
  FSA_SET_COMMENT
  FSA_TIMER
  FSA_INHIBIT
  FSA_DISK_TYPE
  FSA_DISK_CHANGE
  FSA_SCREEN_MODE
  FSA_READ_RETURN
  FSA_WRITE_RETURN

  FSA_SET_FILE_SIZE
    LONG current;
    LONG pos;
    LONG mode;

  FSA_WRITE_PROTECT
  FSA_SAME_LOCK
  FSA_CHANGE_SIGNAL
  FSA_FORMAT
  FSA_IS_FILESYSTEM
  FSA_CHANGE_MODE
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
		    if(iofs->IOFS.io_Command==FSA_STARTUP)
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
