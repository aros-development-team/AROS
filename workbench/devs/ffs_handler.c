/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Amiga FastFileSystem handler
    Lang: english
*/

/****************************************************************************************/

#include <devices/trackdisk.h>
#include <exec/errors.h>
#include <exec/types.h>
#include <exec/resident.h>
#include <exec/memory.h>
#include <exec/semaphores.h>
#include <proto/exec.h>
#include <utility/tagitem.h>
#include <proto/utility.h>
#include <dos/dosextens.h>
#include <dos/dosasl.h>
#include <dos/exall.h>
#include <dos/filesystem.h>
#include <proto/dos.h>
#include <aros/libcall.h>
#include <aros/asmcall.h>
#include <aros/macros.h>
#if defined(__GNUC__) || defined(__INTEL_COMPILER)
#include "ffs_handler_gcc.h"
#endif
#include <stddef.h>
#include <string.h>

#define DEBUG 1
#include <aros/debug.h>

/****************************************************************************************/

/*
 * The amiga (fast) filing system:
 *
 * Unlike some Un*x filesystems the amiga ffs is not split into a fixed inode
 * and data section but instead the inode information is scattered across
 * the entire disk. Each "inode" lives in it's own disk block (usually near
 * the rest of the file) containing enough information to recover the entire
 * file including name, parent directory, creation date and protection bits.
 *
 * The root directory block can be found in the middle of the partition.
 * It contains a hashtable pointing to chained linear lists of file header
 * or further (user) directory blocks. A file header block points to a
 * number of simple data blocks and a list of further file list blocks
 * containing pointers to data blocks.
 *
 * data blocks in the old amiga filing system consist of a small header
 * and the data. data blocks in the fast filing system just consist of
 * the stale data.
 */

/* All information in the amiga FS is big endian */

#if AROS_BIG_ENDIAN
#define EC(a) (a)
#else
#define EC(a) (((a)>>24)|(((a)&0xff0000)>>8)|(((a)&0xff00)<<8)|((a)<<24))
#endif

/*
 * Structure describing a single block of the amiga filesystem. The
 * fb_hashtable field in the middle of the structure is sized so that
 * the entire structure fills a complete block. Therefore all following fields
 * don't live at a fixed offset and are just defines with HASHSIZE being
 * the number of hashtable entries.
 */

struct filesysblock
{
    ULONG fb_type;		/* block types (see below) */
    ULONG fb_own;		/* own block number */
    ULONG fb_blocks;		/* the number of blocks in the file */
    ULONG fb_hashsize;		/* size of the hashtable */
    ULONG fb_firstdata;		/* first data block */
    ULONG fb_chksum;		/* sums the block to 0 */
    ULONG fb_hashtable[122];	/* blocknumbers of blocksize/4-56 blocks */
#define    fb_data		/* data for data blocks, data block numbers else */ fb_hashtable
#define    fb_validated		/* ~0: disk is validated */	fb_hashtable[HASHSIZE]
#define    fb_bam		/* 25 block allocation map blocks */ fb_hashtable+HASHSIZE+1
#define    fb_bam_extend	/* first bam extend block */	fb_hashtable[HASHSIZE+26]
#define    fb_days		/* modification date... */	fb_hashtable[HASHSIZE+27]
#define    fb_mins		/* ...and time... */		fb_hashtable[HASHSIZE+28]
#define    fb_ticks		/* ...dito */			fb_hashtable[HASHSIZE+29]
#define    fb_name		/* file/disk/dir name */	fb_hashtable+HASHSIZE+30
#define    fb_lastlink		/* chained links to file/dir */	fb_hashtable[HASHSIZE+38]
#define    fb_nextlink						fb_hashtable[HASHSIZE+39]
#define    fb_vdays		/* volume altered time */	fb_hashtable[HASHSIZE+40]
#define    fb_vmins						fb_hashtable[HASHSIZE+41]
#define    fb_vticks						fb_hashtable[HASHSIZE+42]
#define    fb_cdays		/* volume creation time */	fb_hashtable[HASHSIZE+43]
#define    fb_cmins						fb_hashtable[HASHSIZE+44]
#define    fb_cticks						fb_hashtable[HASHSIZE+45]
#define    fb_nexthash		/* next hash chain entry */	fb_hashtable[HASHSIZE+46]
#define    fb_parent		/* parent directory */		fb_hashtable[HASHSIZE+47]
#define    fb_extend		/* next file list block */	fb_hashtable[HASHSIZE+48]
#define    fb_sectype		/* secondary type */		fb_hashtable[HASHSIZE+49]
#define    fb_owner		/* UID<<16+GID */		fb_hashtable[HASHSIZE+1]
#define    fb_protect		/* protection bits^0xf */	fb_hashtable[HASHSIZE+2]
#define    fb_size		/* size of the file in bytes */	fb_hashtable[HASHSIZE+3]
#define    fb_comment		/* comment */			fb_hashtable+HASHSIZE+4
#define    fb_id		/* filesystem magic number */	fb_type
};

/* Block types */
#define BT_STRUCT	2	/* describes the structure of the filesystem */
#define BT_DATA		8	/* file data block */
#define BT_FILELIST	16	/* file list block */

/* Secondary types (see dos/dosextens.h) */
#if 0
#define ST_ROOT		1	/* Root directory */
#define ST_USERDIR	2	/* Normal directory */
#define ST_FILE		-3	/* file header/list block */
#endif

#define HASHSIZE dev->hashsize

#define NEWLIST(l)                          \
((l)->lh_Head=(struct Node *)&(l)->lh_Tail, \
 (l)->lh_Tail=NULL,                         \
 (l)->lh_TailPred=(struct Node *)(l))

/****************************************************************************************/

extern const char name[];
extern const char version[];
extern const APTR inittabl[4];
extern void *const functable[];
extern const UBYTE datatable;
extern struct ffsbase *AROS_SLIB_ENTRY(init,ffsdev)();
extern void AROS_SLIB_ENTRY(open,ffsdev)();
extern BPTR AROS_SLIB_ENTRY(close,ffsdev)();
extern BPTR AROS_SLIB_ENTRY(expunge,ffsdev)();
extern int AROS_SLIB_ENTRY(null,ffsdev)();
extern void AROS_SLIB_ENTRY(beginio,ffsdev)();
extern LONG AROS_SLIB_ENTRY(abortio,ffsdev)();
extern void deventry();
extern const char end;

/****************************************************************************************/

/* Structure describing a cache block */
struct cinfo
{
    ULONG lastacc;		/* time index for LRU cache */
    ULONG num;			/* blocknumber of this block (-1: invalid) */
    struct filesysblock *data;	/* Pointer to block */
    LONG synced;		/* 0: needs to be written */
};

/* Device node: one mounted drive */
struct dev
{
    struct MinNode node;
    ULONG numbuffers;		/* number of buffers */
    UBYTE *cache;		/* cached blocks (contiguous to allow
				 * reading of more than one buffer per
				 * device access). */
    struct cinfo *cinfos;	/* array of cinfo structures */
    struct IOExtTD *iotd;	/* I/O request for this device */
    ULONG accindex;		/* actual access index */
    struct vol *vol;		/* Pointer to volume node */
    LONG error;			/* ERROR_NO_DISK, ERROR_NOT_A_DOS_DISK or 0 */
    
    /* Describing the partition */
    ULONG poffset;		/* number of the first partition block */
    ULONG psize;		/* number of blocks of this partition */
    ULONG reserved;		/* Reserved blocks */
    ULONG bsize;		/* size of the blocks */
    ULONG rnum;			/* number of the root block */
    ULONG hashsize;		/* number of hashtable entries */
    struct fh *fh;		/* Device handle */

    ULONG bmext;		/* current bitmap extend block */
    ULONG fblock;		/* first free block */
};

/* Volume node: one mounted disk */
struct vol
{
    struct MinNode node;
    struct dev *dev;		/* drive where the disk is inserted or NULL */
    struct MinList files;	/* All filehandles for this disk */
    struct MinList dirs;	/* All directory handles */

    /* Describing the partition */
    ULONG psize;		/* See above */
    ULONG reserved;
    ULONG bsize;
    ULONG rnum;
    struct DosList *dlist;	/* pointer to dos list entry */

    ULONG cdays, cmins, cticks;
    ULONG vdays, vmins, vticks;
    
    ULONG id;			/* filesystem type */
};

/* A handle for a device, file or directory */
struct fh
{
    struct MinNode node;
    struct vol *vol;	/* Volume or device node */
    ULONG block;	/* FIB or 0 for device handles */
    LONG locked;	/* Object is locked */
    /* actual Seek position */
    ULONG current;	/* actual FIB for dirs, block for files */
    ULONG blocknr;	/* the Nth block in the file */
    ULONG index;	/* Hash chain# for dirs, the Nth byte in the block for files */
};

/****************************************************************************************/

int entry(void)
{
    /* If the device was executed by accident return error code. */
    return -1;
}

/****************************************************************************************/

const struct Resident resident=
{
    RTC_MATCHWORD,
    (struct Resident *)&resident,
    (APTR)&end,
    RTF_AUTOINIT,
    41,
    NT_DEVICE,
    0,
    (char *)name,
    (char *)&version[6],
    (ULONG *)inittabl
};

const char name[]="ffs.handler";

const char version[]="$VER: ffs-handler 41.2 (9.10.97)\r\n";

const APTR inittabl[4]=
{
    (APTR)sizeof(struct ffsbase),
    (APTR)functable,
    (APTR)&datatable,
    &AROS_SLIB_ENTRY(init,ffsdev)
};

void *const functable[]=
{
    &AROS_SLIB_ENTRY(open,ffsdev),
    &AROS_SLIB_ENTRY(close,ffsdev),
    &AROS_SLIB_ENTRY(expunge,ffsdev),
    &AROS_SLIB_ENTRY(null,ffsdev),
    &AROS_SLIB_ENTRY(beginio,ffsdev),
    &AROS_SLIB_ENTRY(abortio,ffsdev),
    (void *)-1
};

const UBYTE datatable=0;

/****************************************************************************************/

#undef SysBase
struct ExecBase *SysBase;

/****************************************************************************************/

AROS_UFH3(struct ffsbase *, AROS_SLIB_ENTRY(init,ffsdev),
 AROS_UFHA(struct ffsbase *, ffsbase, D0),
 AROS_UFHA(BPTR,             segList,   A0),
 AROS_UFHA(struct ExecBase *, sysbase,	A6)
)
{
    AROS_USERFUNC_INIT
    
    /* This function is single-threaded by exec by calling Forbid. */

    struct Task *task;
    APTR stack;

    /* Store arguments */
    SysBase=sysbase;
    ffsbase->seglist=segList;
    NEWLIST((struct List *)&ffsbase->inserted);
    NEWLIST((struct List *)&ffsbase->mounted);
    NEWLIST((struct List *)&ffsbase->removed);
    DOSBase=(struct DosLibrary *)OpenLibrary("dos.library",39);
    if(DOSBase!=NULL)
    {
	NEWLIST(&ffsbase->port.mp_MsgList);
	ffsbase->port.mp_Node.ln_Type=NT_MSGPORT;
	ffsbase->port.mp_Flags=PA_IGNORE;
	NEWLIST(&ffsbase->dport.mp_MsgList);
	ffsbase->dport.mp_Node.ln_Type=NT_MSGPORT;
	ffsbase->dport.mp_Flags=PA_IGNORE;
	NEWLIST(&ffsbase->rport.mp_MsgList);
	ffsbase->rport.mp_Node.ln_Type=NT_MSGPORT;
	ffsbase->rport.mp_Flags=PA_SIGNAL;
	ffsbase->rport.mp_SigBit=SIGB_SINGLE;
	InitSemaphore(&ffsbase->sigsem);

	task=(struct Task *)AllocMem(sizeof(struct Task),MEMF_PUBLIC|MEMF_CLEAR);
	if(task!=NULL)
	{
	    ffsbase->port.mp_SigTask=task;
	    ffsbase->dport.mp_SigTask=task;
	    NEWLIST(&task->tc_MemEntry);
	    task->tc_Node.ln_Type=NT_TASK;
	    task->tc_Node.ln_Name="ffs.handler task";

	    stack=AllocMem(AROS_STACKSIZE,MEMF_PUBLIC);
	    if(stack!=NULL)
	    {
		task->tc_SPLower=stack;
		task->tc_SPUpper=(BYTE *)stack+AROS_STACKSIZE;
#if AROS_STACK_GROWS_DOWNWARDS
		task->tc_SPReg=(BYTE *)task->tc_SPUpper-SP_OFFSET-sizeof(APTR);
		((APTR *)task->tc_SPUpper)[-1]=ffsbase;
#else
		task->tc_SPReg=(BYTE *)task->tc_SPLower-SP_OFFSET+sizeof(APTR);
		*(APTR *)task->tc_SPLower=ffsbase;
#endif

	        if(AddTask(task,deventry,NULL)!=NULL)
		    return ffsbase;

		FreeMem(stack,2048);
            }
	    FreeMem(task,sizeof(struct Task));
	}
	CloseLibrary((struct Library *)DOSBase);
    } /* if(DOSBase!=NULL) */

    return NULL;
    AROS_USERFUNC_EXIT
}

/****************************************************************************************/

AROS_LH3(void, open,
 AROS_LHA(struct IOFileSys *, iofs, A1),
 AROS_LHA(ULONG,              unitnum, D0),
 AROS_LHA(ULONG,              flags, D1),
	   struct ffsbase *, ffsbase, 1, ffsdev)
{
    AROS_LIBFUNC_INIT

    /* Keep compiler happy */
    unitnum=flags=0;

    /* I have one more opener. */
    ffsbase->device.dd_Library.lib_OpenCnt++;

    /* Send message to device task */
    ObtainSemaphore(&ffsbase->sigsem);
    ffsbase->rport.mp_SigTask=FindTask(NULL);
    iofs->IOFS.io_Command=-1;
    PutMsg(&ffsbase->port,&iofs->IOFS.io_Message);
    WaitPort(&ffsbase->rport);
    (void)GetMsg(&ffsbase->rport);
    ReleaseSemaphore(&ffsbase->sigsem);

    if(!iofs->io_DosError)
    {
        iofs->IOFS.io_Error=0;
        ffsbase->device.dd_Library.lib_Flags&=~LIBF_DELEXP;
        return;
    }

    /* set secondary error code and return */
    iofs->IOFS.io_Error=IOERR_OPENFAIL;
    ffsbase->device.dd_Library.lib_OpenCnt--;
        
    AROS_LIBFUNC_EXIT
}

/****************************************************************************************/

AROS_LH1(BPTR, close,
 AROS_LHA(struct IOFileSys *, iofs, A1),
	   struct ffsbase *, ffsbase, 2, ffsdev)
{
    AROS_LIBFUNC_INIT
    /*
	This function is single-threaded by exec by calling Forbid.
	If you break the Forbid() another task may enter this function
	at the same time. Take care.
    */

    /* Send message to device task */
    ObtainSemaphore(&ffsbase->sigsem);
    ffsbase->rport.mp_SigTask=FindTask(NULL);
    iofs->IOFS.io_Command=-2;
    PutMsg(&ffsbase->port,&iofs->IOFS.io_Message);
    WaitPort(&ffsbase->rport);
    ReleaseSemaphore(&ffsbase->sigsem);

    if(iofs->io_DosError)
        return 0;

    /* Let any following attemps to use the device crash hard. */
    iofs->IOFS.io_Device=(struct Device *)-1;

    /* I have one fewer opener. */
    if(!--ffsbase->device.dd_Library.lib_OpenCnt)
    {
	/* Delayed expunge pending? */
	if(ffsbase->device.dd_Library.lib_Flags&LIBF_DELEXP)
	    /* Then expunge the device */
	    return expunge();
    }
    return 0;
    AROS_LIBFUNC_EXIT
}

/****************************************************************************************/

AROS_LH0(BPTR, expunge, struct ffsbase *, ffsbase, 3, ffsdev)
{
    AROS_LIBFUNC_INIT

    BPTR ret;
    /*
	This function is single-threaded by exec by calling Forbid.
	Never break the Forbid() or strange things might happen.
    */

    /* Test for openers. */
    if(ffsbase->device.dd_Library.lib_OpenCnt)
    {
	/* Set the delayed expunge flag and return. */
	ffsbase->device.dd_Library.lib_Flags|=LIBF_DELEXP;
	return 0;
    }

    /* Kill device task and free all resources */
    RemTask(ffsbase->port.mp_SigTask);
    FreeMem(((struct Task *)ffsbase->port.mp_SigTask)->tc_SPLower,2048);
    FreeMem(ffsbase->port.mp_SigTask,sizeof(struct Task));
    CloseLibrary((struct Library *)ffsbase->dosbase);

    /* Get rid of the device. Remove it from the list. */
    Remove(&ffsbase->device.dd_Library.lib_Node);

    /* Get returncode here - FreeMem() will destroy the field. */
    ret=ffsbase->seglist;

    /* Free the memory. */
    FreeMem((char *)ffsbase-ffsbase->device.dd_Library.lib_NegSize,
	    ffsbase->device.dd_Library.lib_NegSize+ffsbase->device.dd_Library.lib_PosSize);

    return ret;
    AROS_LIBFUNC_EXIT
}

/****************************************************************************************/

AROS_LH0I(int, null, struct ffsbase *, ffsbase, 4, ffsdev)
{
    AROS_LIBFUNC_INIT
    return 0;
    AROS_LIBFUNC_EXIT
}

/****************************************************************************************/

AROS_LH1(void, beginio,
 AROS_LHA(struct IOFileSys *, iofs, A1),
	   struct ffsbase *, ffsbase, 5, ffsdev)
{
    AROS_LIBFUNC_INIT

    /* Nothing is done quick */
    iofs->IOFS.io_Flags&=~IOF_QUICK;
    
    /* So let the device task do it */
    PutMsg(&ffsbase->port,&iofs->IOFS.io_Message);
    
    AROS_LIBFUNC_EXIT
}

/****************************************************************************************/

AROS_LH1(LONG, abortio,
 AROS_LHA(struct IOFileSys *, iofs, A1),
	   struct ffsbase *, ffsbase, 6, ffsdev)
{
    AROS_LIBFUNC_INIT
    return 0;
    AROS_LIBFUNC_EXIT
}

/****************************************************************************************/

#define touch_read(dev,cinfo) ((cinfo)->lastacc=((dev)->accindex)++)
#define touch_write(dev,cinfo) ((cinfo)->lastacc=((dev)->accindex)++,(cinfo)->synced=0)

/****************************************************************************************/

static LONG write_block(struct ffsbase *ffsbase, struct dev *dev, struct cinfo *cinfo)
{
    cinfo->synced=1;
    dev->iotd->iotd_Req.io_Command=CMD_WRITE;
    dev->iotd->iotd_Req.io_Data   =cinfo->data;
    dev->iotd->iotd_Req.io_Offset =(dev->poffset+cinfo->num)*dev->bsize;
    dev->iotd->iotd_Req.io_Length =dev->bsize;
    
    return DoIO((struct IORequest *)dev->iotd);
}

/****************************************************************************************/

static LONG get_buffer(struct ffsbase *ffsbase, struct dev *dev, struct cinfo **cinfo)
{
    LONG 		ret=0;
    struct cinfo 	*buf;
    ULONG 		max,i;
    
    buf=dev->cinfos;
    max=dev->accindex-buf->lastacc;
    for(i=1;i<dev->numbuffers;i++)
        if(dev->accindex-dev->cinfos[i].lastacc>max)
        {
            buf=&dev->cinfos[i];
            max=dev->accindex-buf->lastacc;
        }
    if(!buf->synced)
        ret=write_block(ffsbase,dev,buf);
    *cinfo=buf;
    
    return ret;
}

/****************************************************************************************/

LONG get_block(struct ffsbase *ffsbase, struct dev *dev, struct cinfo **cinfo, ULONG num)
{
    ULONG 	i;
    int 	ret;
    
    if(dev==NULL)
        return ERROR_DEVICE_NOT_MOUNTED;
	
    for(i=0;i<dev->numbuffers;i++)
	if(dev->cinfos[i].num==num)
	{
	    *cinfo=&dev->cinfos[i];
	    return 0;
	}
    ret=get_buffer(ffsbase,dev,cinfo);
    if(ret)
        return ret;
	
    (*cinfo)->num=~0;
    (*cinfo)->synced=0;
    dev->iotd->iotd_Req.io_Command=CMD_READ;
    dev->iotd->iotd_Req.io_Data   =(*cinfo)->data;
    dev->iotd->iotd_Req.io_Offset =(dev->poffset+num)*dev->bsize;
    dev->iotd->iotd_Req.io_Length =dev->bsize;
    ret=DoIO((struct IORequest *)dev->iotd);
    if(ret)
        return 1; /* I/O Error */
	
    (*cinfo)->num=num;
    (*cinfo)->synced=1;
    
    return 0;
}

/****************************************************************************************/

ULONG checksum(struct dev *dev, struct cinfo *cinfo)
{
    ULONG i;
    
    ULONG *p=(ULONG *)cinfo->data, sum=0;
    
    for(i=0;i<dev->bsize/4;i++)
    {
	sum += AROS_LONG2BE(*p);
	p++;
    }
    
    return -sum;
    
}

/****************************************************************************************/

LONG read_block_chk(struct ffsbase *ffsbase, struct dev *dev, struct cinfo **cinfo, ULONG num)
{
    int ret;
    
    ret=get_block(ffsbase,dev,cinfo,num);
    if(ret)
        return ret;
	
    if(checksum(dev,*cinfo))
    {kprintf("%s\n",(*cinfo)->data->fb_name);
	return ERROR_BAD_NUMBER; /* Checksum error */}
	
    return 0;
}

/****************************************************************************************/

void dump(struct ffsbase *ffsbase, struct cinfo *block)
{
    int i;
    for(i=0;i<128;i++)
        kprintf("%lx ",EC(((ULONG *)block->data)[i]));
    kprintf("\n");
}

/****************************************************************************************/

static LONG alloc_block(struct ffsbase *ffsbase, struct dev *dev, ULONG *newblk)
{
    struct cinfo *block;
    ULONG a, b;
    ULONG i, imax, k, kmax, kmax2;
    ULONG bpb, bblock, btotal, next;
    LONG error;
    
    bpb=8*(dev->bsize-4);
    bblock=dev->fblock/bpb;
    btotal=(dev->psize-dev->reserved+bpb-1)/bpb;
    k=1+(dev->fblock%bpb)/32;
    for(;;)
    {
	if(bblock<25)
	{
	    i=7+HASHSIZE+bblock;
	    imax=7+25+HASHSIZE;
	}else
	{
	    i=(bblock-25)%(dev->bsize/4-1);
	    imax=dev->bsize/4-1;
	}
	for(;i<imax;i++)
	{
	    if(bblock>=btotal)
	        return ERROR_DISK_FULL;
		
	    if(bblock<25)
	        error=read_block_chk(ffsbase,dev,&block,dev->bmext);
	    else
	        error=get_block(ffsbase,dev,&block,dev->bmext);
	    if(error)
	        return error;
		
	    next=EC(((ULONG *)block->data)[imax]);
	    touch_read(dev,block);
	    error=read_block_chk(ffsbase,dev,&block,EC(((ULONG *)block->data)[i]));
	    if(error)
	        return error;
		
	    kmax=dev->bsize/4;
	    kmax2=1+(dev->psize-dev->reserved-bblock*bpb+31)/32;
	    if(kmax>kmax2)
	        kmax=kmax2;
	    touch_read(dev,block);
	    for(;k<kmax;k++)
	        if(((ULONG *)block->data)[k])
	        {
	            a=EC(((ULONG *)block->data)[k]);
	            b=a&-a;
	            a&=~b;
	            b=(b&0xffff0000?16:0)+(b&0xff00ff00?8:0)+(b&0xf0f0f0f0?4:0)+
	              (b&0xcccccccc?2:0) +(b&0xaaaaaaaa?1:0);
	            dev->fblock=bblock*bpb+(k-1)*32+b;
	            if(dev->fblock+dev->reserved>=dev->psize)
	                return ERROR_DISK_FULL;
			
	            *newblk=dev->fblock+dev->reserved;
	            ((ULONG *)block->data)[k]=EC(a);
	            *(ULONG *)block->data=0;
	            a=checksum(dev,block);
	            *(ULONG *)block->data=EC(a);
	            touch_write(dev,block);
		    
	            return 0;
	        }
		
	    bblock++;
	    dev->fblock=bblock*bpb;
	    k=1;
	    
	} /* for(;i<imax;i++) */
	dev->bmext=next;
	
    } /* for(;;) */
}

/****************************************************************************************/

static LONG free_block(struct ffsbase *ffsbase, struct dev *dev, ULONG blk)
{
    struct cinfo *block;
    ULONG 	 a, k, old, oldbblock, bpb, bblock, btotal, next;
    LONG 	 error;

    bpb=8*(dev->bsize-4);
    oldbblock=dev->fblock/bpb;
    btotal=(dev->psize-dev->reserved+bpb-1)/bpb;
    bblock=blk/bpb;
    k=blk%bpb;
    
    if(bblock<oldbblock)
    {
        dev->bmext=dev->rnum;
        if(bblock>25)
        {
	    error=read_block_chk(ffsbase,dev,&block,dev->bmext);
            if(error)
                return error;
            dev->bmext=EC(((ULONG *)block->data)[7+25+HASHSIZE]);
            touch_read(dev,block);
            old=(old-25)/(dev->bsize/4-1);
            while(old--)
            {
	        error=get_block(ffsbase,dev,&block,dev->bmext);
                if(error)
                    return error;
	        dev->bmext=EC(((ULONG *)block->data)[dev->bsize/4-1]);
	        touch_read(dev,block);
            }
        }
    }
    
    if(dev->fblock>blk)
        dev->fblock=blk;
    if(bblock<25)
    {
	error=read_block_chk(ffsbase,dev,&block,dev->bmext);
	next=EC(((ULONG *)block->data)[7+HASHSIZE+bblock]);
    }else
    {
    	error=get_block(ffsbase,dev,&block,dev->bmext);
	next=EC(((ULONG *)block->data)[(bblock-25)%(dev->bsize/4-1)]);
    }
    if(error)
        return error;
	
    touch_read(dev,block);
    error=read_block_chk(ffsbase,dev,&block,next);
    if(error)
        return error;
	
    a=EC(((ULONG *)block->data)[k/32+1]);
    a|=1<<(k&31);
    ((ULONG *)block->data)[k/32+1]=EC(a);
    touch_write(dev,block);
    
    return 0;
}

/****************************************************************************************/

static void zerofill(UBYTE *address, ULONG size)
{
    while(size--)
	*address++=0;
}

/****************************************************************************************/

LONG mount(struct ffsbase *ffsbase, struct fh **fh, STRPTR name, LONG unit, IPTR *envec)
{
    struct dev 	*dev;
    ULONG 	i,tracksize;
    
    dev=(struct dev *)AllocMem(sizeof(struct dev),MEMF_CLEAR);
    if(dev!=NULL)
    {
        dev->fh=(struct fh *)AllocMem(sizeof(struct fh),MEMF_CLEAR);
        if(dev->fh!=NULL)
        {
            dev->fh->vol=(struct vol *)dev;
            dev->cache=(UBYTE *)AllocMem(envec[DE_NUMBUFFERS]*envec[DE_BLOCKSIZE],MEMF_CLEAR);
            if(dev->cache!=NULL)
            {
                dev->cinfos=(struct cinfo *)AllocMem(sizeof(struct cinfo)*envec[DE_NUMBUFFERS],MEMF_CLEAR);
                if(dev->cinfos!=NULL)
                {
                    for(i=0;i<envec[DE_NUMBUFFERS];i++)
                    {
                        dev->cinfos[i].data=(struct filesysblock *)&dev->cache[envec[DE_BLOCKSIZE]*i];
                        dev->cinfos[i].num=-1;
                        dev->cinfos[i].synced=1;
                    }
                    tracksize=envec[DE_NUMHEADS]*envec[DE_BLKSPERTRACK];
                    dev->numbuffers=envec[DE_NUMBUFFERS];
                    dev->poffset=tracksize*envec[DE_LOWCYL];
                    dev->psize=tracksize*(envec[DE_HIGHCYL]-envec[DE_LOWCYL]+1);
                    dev->reserved=envec[DE_RESERVEDBLKS];
                    dev->bsize=envec[DE_BLOCKSIZE];
                    dev->rnum=(dev->psize-dev->reserved-1)/2+dev->reserved;
                    dev->hashsize=(dev->bsize-224)/4;
		    
		    D(bug("ffs_handler/mount: tracksize  = %d\n", tracksize));
		    D(bug("ffs_handler/mount: numbuffers = %d\n", dev->numbuffers));
		    D(bug("ffs_handler/mount: poffset    = %d\n", dev->poffset));
		    D(bug("ffs_handler/mount: psize      = %d\n", dev->psize));
		    D(bug("ffs_handler/mount: reserved   = %d\n", dev->reserved));
		    D(bug("ffs_handler/mount: bsize      = %d\n", dev->bsize));
		    D(bug("ffs_handler/mount: rnum       = %d\n", dev->rnum));
		    D(bug("ffs_handler/mount: hashsize   = %d\n", dev->hashsize));
		    
                    dev->iotd=(struct IOExtTD *)CreateIORequest(&ffsbase->dport,sizeof(struct IOExtTD));
 
                    if(dev->iotd!=NULL)
		    {
                        if(!OpenDevice(name,unit,(struct IORequest *)dev->iotd,0))
                        {
                            *fh=dev->fh;
                            return 0;
                        }
                        DeleteIORequest((struct IORequest *)dev->iotd);
                    }
                    FreeMem(dev->cinfos,sizeof(struct cinfo)*envec[DE_NUMBUFFERS]);
		    
                } /* if(dev->cinfos!=NULL) */
                FreeMem(dev->cache,envec[DE_NUMBUFFERS]*envec[DE_BLOCKSIZE]);
		
            } /* if(dev->cache!=NULL) */
            FreeMem(dev->fh,sizeof(struct fh));
	    
        } /* if(dev->fh!=NULL) */
        FreeMem(dev,sizeof(struct dev));
	
    } /* if(dev!=NULL) */
    
    return ERROR_NO_FREE_STORE;
}

/****************************************************************************************/

void flush(struct dev *dev)
{
    ULONG i;
    
    for(i=0;i<dev->numbuffers;i++)
    {
        dev->cinfos[i].num=-1;
        dev->cinfos[i].synced=1;
    }
}

/****************************************************************************************/

LONG disk_change(struct ffsbase *ffsbase, struct dev *dev)
{
    struct cinfo 	*root, *boot;
    ULONG 		id;
    LONG 		error;
    struct vol 		*vol;
    struct fh 		*fh;

    D(bug("ffs_handler/disk_change. Flushing and reading in boot block\n"));
   
    flush(dev);
    error=get_block(ffsbase,dev,&boot,0);
    if(error)
        return dev->error=error;

    D(bug("ffs_handler/disk_change: Checking ID\n"));
    
    id=EC(boot->data->fb_id);

    if(id!=ID_DOS_DISK&&id!=ID_FFS_DISK)
    {
         D(bug("ffs_handler/disk_change: bad disk ID (%c%c%c%c). Returning ERROR_NOT_A_DOS_DISK\n", id >> 24, id >> 16, id >> 8, id));

         return dev->error=ERROR_NOT_A_DOS_DISK;
    }
    
    D(bug("ffs_handler/disk_change: ID okay. Now reading in root block\n"));
	
    error=read_block_chk(ffsbase,dev,&root,dev->rnum);

    D(bug("ffs_handler/disk_change: read_block_chk returned %d\n", error));

    if(error==ERROR_BAD_NUMBER||EC(root->data->fb_hashsize)!=dev->hashsize||
        root->data->fb_type!=EC(BT_STRUCT)||root->data->fb_sectype!=EC(ST_ROOT)||
        root->data->fb_validated!=EC(~0ul))
    {
        D(bug("ffs_handler/disk_change: Something wrong with rootblock:\n"));
 	D(bug(" ffs_handler/disk_change: hashsize     = %d <--> %d\n",EC(root->data->fb_hashsize),dev->hashsize));
	D(bug(" ffs_handler/disk_change: fb_type      = %d <--> %d\n",EC(root->data->fb_type), BT_STRUCT));
	D(bug(" ffs_handler/disk_change: fb_sectype   = %d <--> %d\n",EC(root->data->fb_sectype), ST_ROOT)); 
	D(bug(" ffs_handler/disk_change: fb_validated = %d <--> %d\n",EC(root->data->fb_validated), ~0UL));
        D(bug("ffs_handler/disk_change: Returning ERROR_NOT_A_DOS_DISK\n"));
        return dev->error=ERROR_NOT_A_DOS_DISK;
    }
    
#if 0
    for(dev=ffsbase->volumes.mlh_Head;
	dev->node.mln_Succ!=NULL;
	dev=dev->node.mln_Succ)
	if(dev->cdays ==root->data->fb_cdays &&
	   dev->cmins ==root->data->fb_cmins &&
	   dev->cticks==root->data->fb_cticks&&
	   dev->vdays ==root->data->fb_vdays &&
	   dev->vmins ==root->data->fb_vmins &&
	   dev->vticks==root->data->fb_vticks&&
    {
    }
#endif

    vol=AllocMem(sizeof(struct vol),MEMF_PUBLIC);
    if(vol!=NULL)
    {
        fh=AllocMem(sizeof(struct fh),MEMF_CLEAR);

        if(fh!=NULL)
        {
            UBYTE buf[32];
            ULONG l;
            fh->vol  =vol;
            fh->block=dev->rnum;
            vol->id      =id;
            vol->bsize   =dev->bsize;
    	    vol->psize   =dev->psize;
    	    vol->reserved=dev->reserved;
            vol->cdays =EC(root->data->fb_cdays);
            vol->cmins =EC(root->data->fb_cmins);
            vol->cticks=EC(root->data->fb_cticks);
            vol->vdays =EC(root->data->fb_vdays);
            vol->vmins =EC(root->data->fb_vmins);
            vol->vticks=EC(root->data->fb_vticks);
            NEWLIST((struct List *)&vol->files);
            NEWLIST((struct List *)&vol->dirs);
            l=*(UBYTE *)(root->data->fb_name);
 
            if(l<31)
            {
                buf[l]=0;
                CopyMem((STRPTR)(root->data->fb_name)+1,buf,l);
                vol->dlist=MakeDosEntry(buf,DLT_VOLUME);
                if(vol->dlist!=NULL)
                {
                    vol->dlist->dol_Device=&ffsbase->device;
                    vol->dlist->dol_Unit=(struct Unit *)fh;
                    dev->vol=vol;
                    vol->dev=dev;
                    dev->bmext=dev->rnum;
                    dev->fblock=0;
                    AddTail((struct List *)&ffsbase->inserted,(struct Node *)vol);
                    ffsbase->dlflag=1;
                    return 0;
                }
		
            } /* if (l<31) */
            FreeMem(fh,sizeof(struct fh));
	    
        } /* if(fh!=NULL) */
        FreeMem(dev,sizeof(struct dev));
	
    } /* if(vol!=NULL) */
    
    return ERROR_NO_FREE_STORE;
}

/****************************************************************************************/

#define toupper_ffs(c) ((c)>='a'&&(c)<='z'?(c)-'a'+'A':(c))

/****************************************************************************************/

static int namechk(struct dev *dev, struct cinfo *cur, STRPTR name)
{
    STRPTR 	s1=(STRPTR)(cur->data->fb_name);
    ULONG 	l=*s1++;
    
    if(l)
	do
	{
            if(toupper_ffs(*s1)!=toupper_ffs(*name)||*name=='/')
            	return 1;
            s1++, name++;
        }
        while(--l);
	
    return *name;
}

/****************************************************************************************/

static void setname(struct dev *dev, struct cinfo *block, STRPTR name)
{
    STRPTR 	s1=(STRPTR)(block->data->fb_name)+1;
    ULONG 	l;
    
    for(l=0;l<30;l++)
    {
	if(!*name)
	    break;
	*s1++=*name++;
    }
    
    *(STRPTR)(block->data->fb_name)=l;
    
    for(;l<31;l++)
        *s1++=0;
}

/****************************************************************************************/

static ULONG hash(struct dev *dev, STRPTR name)
{
    ULONG 	l, h=0;
    STRPTR 	s2=name;

    while(*s2&&*s2!='/')
        s2++;
    l=s2-name>30?30:s2-name;
    h=l;

    while(l--)
    {
        h=(h*13+toupper_ffs(*name))&0x7ff;
        name++;
    }
    
    return h%HASHSIZE;
}

/****************************************************************************************/

static LONG findname(struct ffsbase *ffsbase, struct fh *fh, STRPTR *name, struct cinfo **dir)
{
    LONG 		error;
    struct cinfo 	*cur=*dir;
    char 		*rest=*name;
    ULONG 		block;
    struct dev 		*dev;

    block=fh->block;
    if(!block)
    {
        dev=(struct dev *)fh->vol;
        block=dev->rnum;
    }else
        dev=fh->vol->dev;
    error=read_block_chk(ffsbase,dev,&cur,block);
    if(error)
	return error;

    while(*rest)
    {
	touch_read(dev,cur);
	if(*rest=='/')
	{
	    if(cur->data->fb_sectype==EC(ST_ROOT))
	    {
	        *name=rest;
	        *dir=cur;
		return ERROR_OBJECT_NOT_FOUND;
	    }
	    error=read_block_chk(ffsbase,dev,&cur,EC(cur->data->fb_parent));
	    if(error)
	        return error;
	}
	else
	{
	    if(cur->data->fb_sectype!=EC(ST_USERDIR)&&
	       cur->data->fb_sectype!=EC(ST_ROOT))
	    {
	        *name=rest;
	        *dir=cur;
		return ERROR_DIR_NOT_FOUND;
	    }

	    *dir=cur;
	    block=hash(dev,rest);
	    block=EC(cur->data->fb_hashtable[block]);

	    for(;;)
	    {
    	        if(!block)
		{
		    *name=rest;
		    *dir=cur;
		    return ERROR_OBJECT_NOT_FOUND;
		}
		
    	        error=read_block_chk(ffsbase,dev,&cur,block);
    	        if(error)
    	            return error;
		    
		touch_read(dev,cur);
		if(!namechk(dev,cur,rest))
		    break;
		    
		block=EC(cur->data->fb_nexthash);
	    }
	}
	
	while(*rest)
	    if(*rest++=='/')
		    break;
		    
    } /* while(*rest)*/
    *dir=cur;
    
    return 0;
}

/****************************************************************************************/

static LONG lock(struct dev *dev, struct cinfo *block, ULONG mode)
{
    ULONG 	protect;
    struct fh 	*fh;
    
    for(fh=(struct fh *)dev->vol->files.mlh_Head;
	fh->node.mln_Succ!=NULL;
	fh=(struct fh *)fh->node.mln_Succ)
	if(fh->block==block->num)
	    if(mode&FMF_LOCK||fh->locked)
	        return ERROR_OBJECT_IN_USE;
		
    touch_read(dev,block);
    protect=EC(block->data->fb_protect); /* ^0xf; */

    /* protect bits in data block are 1 if something is forbidden and 0,
      if something is allowed! */
    
    if((mode&FMF_EXECUTE)&&(protect&FMF_EXECUTE))
        return ERROR_NOT_EXECUTABLE;
	
    if((mode&FMF_WRITE)&&(protect&FMF_WRITE))
	return ERROR_WRITE_PROTECTED;
	
    if((mode&FMF_READ)&&(protect&FMF_READ))
	return ERROR_READ_PROTECTED;
	
    return 0;
}

/****************************************************************************************/

static LONG open(struct ffsbase *ffsbase, struct fh **fh, STRPTR name, ULONG mode)
{
    struct cinfo *dir;
    struct fh	 *new;
    LONG 	 error;
    struct dev 	 *dev;
    
    dev=(*fh)->block?(*fh)->vol->dev:(struct dev *)(*fh)->vol;

    new=(struct fh *)AllocMem(sizeof(struct fh),MEMF_CLEAR);
    if(new!=NULL)
    {
        error=findname(ffsbase,*fh,&name,&dir);
        if(!error)
        {
            error=lock(dev,dir,mode);
            if(!error)
            {
                new->vol=dev->vol;
		new->block=dir->num;
		if(mode&FMF_LOCK)
		    new->locked=1;
        	AddHead((struct List *)(dir->data->fb_sectype==EC(ST_FILE)?
        		&dev->vol->files:&dev->vol->dirs),
        		(struct Node *)&new->node);
                *fh=new;
                return 0;
            }
        }
        FreeMem(new,sizeof(struct fh));
    }else
        error=ERROR_NO_FREE_STORE;
	
    return error;
}

/****************************************************************************************/

static LONG create_object
(struct ffsbase *ffsbase, struct dev *dev, ULONG *blocknr,
 struct cinfo *block, STRPTR name, ULONG protect, ULONG type)
{
    STRPTR 	s2;
    ULONG 	hashnr, pnum, next, a;
    LONG 	error;

    s2=name;
    while(*s2)
        if(*s2++=='/')
            return ERROR_DIR_NOT_FOUND;
	    
    pnum=block->num;
    hashnr=hash(dev,name);
    next=block->data->fb_hashtable[hashnr];
    error=alloc_block(ffsbase,dev,blocknr);
    
    if(!error)
    {
        error=get_buffer(ffsbase,dev,&block);
        if(!error)
        {
            block->num=*blocknr;
            zerofill((STRPTR)block->data,dev->bsize);
            block->data->fb_type    =EC(BT_STRUCT);
            block->data->fb_own     =EC(*blocknr);
            block->data->fb_protect =EC(protect); // ^0xf);
#warning TODO: set creation date
            block->data->fb_nexthash=next;
            block->data->fb_parent  =EC(pnum);
            block->data->fb_sectype =EC(type);
            setname(dev,block,name);
            block->data->fb_chksum  =0;
            a=checksum(dev,block);
            block->data->fb_chksum  =EC(a);
            touch_write(dev,block);
            error=read_block_chk(ffsbase,dev,&block,pnum);
            if(!error)
            {
                block->data->fb_hashtable[hashnr]=EC(*blocknr);
                block->data->fb_chksum=0;
                a=checksum(dev,block);
                block->data->fb_chksum=EC(a);
                touch_write(dev,block);
                return 0;
            }
	    
        } /* if(!error) */
        (void)free_block(ffsbase,dev,*blocknr);
	
    } /* if(!error) */
    
    return error;
}

/****************************************************************************************/

static LONG open_file(struct ffsbase *ffsbase, struct fh **fh, STRPTR name, ULONG mode, ULONG protect)
{
    struct cinfo 	*dir;
    struct fh 		*new;
    LONG 		error;
    struct dev 		*dev;

    dev=(*fh)->block?(*fh)->vol->dev:(struct dev *)(*fh)->vol;

    new=AllocMem(sizeof(struct fh),MEMF_CLEAR);
    if(new!=NULL)
    {
        error=findname(ffsbase,*fh,&name,&dir);
        if((mode&FMF_CREATE)&&error==ERROR_OBJECT_NOT_FOUND)
            error=create_object(ffsbase,dev,&new->block,dir,name,protect,ST_FILE);
        else if(!error)
        {
            touch_read(dev,dir);
            if(dir->data->fb_sectype!=EC((ULONG)ST_FILE))
                error=ERROR_OBJECT_WRONG_TYPE;
            else
                error=lock(dev,dir,mode);
        }
        if(!error)
        {
            new->vol  =dev->vol;
	    new->block=dir->num;
	    if(mode&FMF_LOCK)
		new->locked=1;
  	    AddHead((struct List *)&dev->vol->files,(struct Node *)new);
            *fh=new;
            return 0;
        }
        FreeMem(new,sizeof(struct fh));
	
    } /* if(new!=NULL)*/
    
    return error;
}

/****************************************************************************************/

static LONG read(struct ffsbase *ffsbase, struct fh *fh, APTR buf, ULONG *numbytes)
{
    struct cinfo 	*block;
    ULONG 		bpb,total,pos,rest,size,num=*numbytes;
    STRPTR 		buffer=(STRPTR)buf;
    struct dev 		*dev;
    LONG 		error;

    dev=fh->block?fh->vol->dev:(struct dev *)fh->vol;
    error=read_block_chk(ffsbase,dev,&block,fh->block);
    if(error)
        return error;
	
    bpb=fh->vol->id==ID_DOS_DISK?fh->vol->bsize-24:fh->vol->bsize;
    total=EC(block->data->fb_size);
    touch_read(dev,block);
    pos=fh->blocknr*bpb+fh->index;
    rest=total>pos?total-pos:0;
    num=num>rest?rest:num;
    size=bpb-fh->index;

    while(num)
    {
        if(size>num)
            size=num;
        if(!fh->current)
        {
            fh->current=fh->block;
            rest=fh->blocknr;
            while(rest--)
            {
                error=read_block_chk(ffsbase,dev,&block,fh->current);
                if(error)
                    return error;
		    
                fh->current=EC(block->data->fb_extend);
                touch_read(dev,block);
            }
        }

        error=read_block_chk(ffsbase,dev,&block,fh->current);
        if(error)
            return error;
	    
        touch_read(dev,block);

        if(fh->vol->id==ID_DOS_DISK)
        {
            error=read_block_chk(ffsbase,dev,&block,EC(block->data->fb_data[HASHSIZE-1-fh->blocknr%(HASHSIZE)]));
            if(error)
                return error;
            CopyMem((STRPTR)(block->data->fb_data)+fh->index,buffer,size);
        }
	else
        {
            error=get_block(ffsbase,dev,&block,EC(block->data->fb_data[fh->blocknr%(HASHSIZE)]));
            if(error)
                return error;
            CopyMem((STRPTR)(block->data)+fh->index,buffer,size);
        }

        touch_read(dev,block);
        buffer+=size;
        num-=size;
        fh->index+=size;
        if(fh->index==bpb)
 
        {
            fh->index=0;
            fh->blocknr++;
        }

        if(!fh->blocknr%(HASHSIZE))
        {
            error=read_block_chk(ffsbase,dev,&block,fh->current);
            if(error)
                return error;
		
            fh->current=EC(block->data->fb_extend);
            touch_read(dev,block);
        }
        size=bpb;
	
    } /* while(num) */
    
    *numbytes=buffer-(STRPTR)buf;
    
    return 0;
}

/****************************************************************************************/

static LONG free_lock(struct ffsbase *ffsbase, struct fh *fh)
{
    Remove((struct Node *)fh);
    FreeMem(fh,sizeof(struct fh));
#warning TODO: Dismount removed and unused disk
    return 0;
}

/****************************************************************************************/

static LONG create_dir(struct ffsbase *ffsbase, struct fh **fh, STRPTR name, ULONG protect)
{
    struct cinfo *block;
    struct dev 	 *dev;
    struct fh 	 *new;
    LONG 	 error;
    
    dev=(*fh)->block?(*fh)->vol->dev:(struct dev *)(*fh)->vol;

    error=findname(ffsbase,*fh,&name,&block);
    if(!error)
        return ERROR_OBJECT_EXISTS;
	
    if(error!=ERROR_OBJECT_NOT_FOUND)
        return error;
	
    new=AllocMem(sizeof(struct fh),MEMF_CLEAR);
    if(new!=NULL)
    {
        error=create_object(ffsbase,dev,&new->block,block,name,protect,ST_USERDIR);
        if(!error)
        {
            new->vol=dev->vol;
            new->locked=1;
            AddHead((struct List *)&dev->vol->dirs,(struct Node *)new);
            *fh=new;
            return 0;
        }
        FreeMem(new,sizeof(struct fh));
    }
    
    return error;
}

/****************************************************************************************/

static const ULONG sizes[]=
{ 0, offsetof(struct ExAllData,ed_Type), offsetof(struct ExAllData,ed_Size),
  offsetof(struct ExAllData,ed_Prot), offsetof(struct ExAllData,ed_Days),
  offsetof(struct ExAllData,ed_Comment), offsetof(struct ExAllData,ed_OwnerUID),
  sizeof(struct ExAllData)
}; 

/****************************************************************************************/

static LONG examine_fib(struct ffsbase *ffsbase, struct dev *dev, ULONG block, struct ExAllData *ead, ULONG size, ULONG type)
{
    struct cinfo 	*file;
    STRPTR 		next, end, name;
    ULONG 		l;
    LONG 		error;
    
    if(type>ED_OWNER)
        return ERROR_BAD_NUMBER;
	
    next=(STRPTR)ead+sizes[type];
    end=(STRPTR)ead+size;
    error=read_block_chk(ffsbase,dev,&file,block);
    if(error)
	return error;
	
    touch_read(dev,file);
    
    switch(type)
    {
	case ED_OWNER:
	    ead->ed_OwnerUID=EC(file->data->fb_owner)>>16;
	    ead->ed_OwnerGID=EC(file->data->fb_owner)&0xffff;
	    
	case ED_COMMENT:
	    name=(STRPTR)(file->data->fb_comment);
	    l=*name++;
	    if(l)
	    {
	        ead->ed_Comment=next;
	        if(next+l+1>=end)
	            return ERROR_BUFFER_OVERFLOW;
	        do
	            *next++=*name++;
	        while(--l);
	        *next++='\0';
	    }else
	        ead->ed_Comment=NULL;
		
	case ED_DATE:
	    ead->ed_Days=EC(file->data->fb_days);
	    ead->ed_Mins=EC(file->data->fb_mins);
	    ead->ed_Ticks=EC(file->data->fb_ticks);
	    
	case ED_PROTECTION:
	    ead->ed_Prot=EC(file->data->fb_protect);//^0xf;
	    
	case ED_SIZE:
	    ead->ed_Size=EC(file->data->fb_size);
	    
	case ED_TYPE:
	    ead->ed_Type=EC(file->data->fb_sectype);
	    
	case ED_NAME:
	    name=(STRPTR)(file->data->fb_name);
	    l=*name++;
	    ead->ed_Name=next;
	    if(next+l+1>=end)
	        return ERROR_BUFFER_OVERFLOW;
	    if(l)
	        do
	            *next++=*name++;
	        while(--l);
	    *next++='\0';
	    
	case 0:
	    ead->ed_Next=(struct ExAllData *)(((ULONG)next+AROS_PTRALIGN-1)&~(AROS_PTRALIGN-1));
	    
    } /* switch(type) */
    
    return 0;
}

/****************************************************************************************/

static LONG examine(struct ffsbase *ffsbase, struct fh *fh, struct ExAllData *ead, ULONG size, ULONG type)
{
    return examine_fib(ffsbase,fh->vol->dev,fh->block,ead,size,type);
}

/****************************************************************************************/

static LONG examine_all(struct ffsbase *ffsbase, struct fh *dir, struct ExAllData *ead, struct ExAllControl *eac, ULONG size, ULONG type)
{
    STRPTR 		end;
    struct ExAllData 	*last=NULL;
    struct cinfo 	*block;
    struct dev 		*dev;
    LONG 		error;
    
    eac->eac_Entries = 0;
    dev=dir->block?dir->vol->dev:(struct dev *)dir->vol;
    end=(STRPTR)ead+size;
    error=read_block_chk(ffsbase,dev,&block,dir->block);
    if(error)
	return error;
	
    touch_read(dev,block);
    if(block->data->fb_sectype!=EC(ST_USERDIR)&&
       block->data->fb_sectype!=EC(ST_ROOT))
        return ERROR_OBJECT_WRONG_TYPE;
	
    if(!dir->blocknr)
        for(;dir->index<HASHSIZE;dir->index++)
            if(block->data->fb_hashtable[dir->index])
            {
                dir->blocknr=EC(block->data->fb_hashtable[dir->index]);
                break;
            }

    if(dir->index>=HASHSIZE)
    {
        dir->index=0;
        return ERROR_NO_MORE_ENTRIES;
    }

    do
    {
        error=examine_fib(ffsbase,dev,dir->blocknr,ead,end-(STRPTR)ead,type);
        if(error==ERROR_BUFFER_OVERFLOW)
        {
            if(last==NULL)
                return error;
		
            last->ed_Next=NULL;
	    
            return 0;
        }
	eac->eac_Entries++;
        last=ead;
        ead=ead->ed_Next;
        error=read_block_chk(ffsbase,dev,&block,dir->blocknr);
        if(error)
             return error;
	     
        touch_read(dev,block);
        dir->blocknr=EC(block->data->fb_nexthash);

        if(!dir->blocknr)
        {
            error=read_block_chk(ffsbase,dev,&block,dir->block);
            if(error)
                 return error;
            touch_read(dev,block);
            for(dir->index++;dir->index<HASHSIZE;dir->index++)
                if(block->data->fb_hashtable[dir->index])
                {
                    dir->blocknr=EC(block->data->fb_hashtable[dir->index]);
                    break;
                }
        }
	
    } while(dir->index<HASHSIZE);
    
    last->ed_Next=NULL;
    
    return 0;
}

/****************************************************************************************/

void deventry(struct ffsbase *ffsbase)
{
    struct IOFileSys *iofs;
    struct vol 	     *vol;

    LONG error = 0;

    /*
    	Init device ports. AllocSignal() cannot fail because this is a
	freshly created task with all signal bits still free. 
    */
    ffsbase->port.mp_SigBit = AllocSignal(-1);
    ffsbase->port.mp_Flags = PA_SIGNAL;
    ffsbase->dport.mp_SigBit = AllocSignal(-1);
    ffsbase->dport.mp_Flags = PA_SIGNAL;

    /* Get and process the messages. */
    for (;;)
    {
    	while((iofs=(struct IOFileSys *)GetMsg(&ffsbase->port))!=NULL)
    	{
    	    switch(iofs->IOFS.io_Command)
    	    {
	    case (UWORD)-1:
		error = mount(ffsbase, (struct fh **)&iofs->IOFS.io_Unit,
			      iofs->io_Union.io_OpenDevice.io_DeviceName,
			      iofs->io_Union.io_OpenDevice.io_Unit,
			      iofs->io_Union.io_OpenDevice.io_Environ);
		iofs->io_DosError = error;
		
		if (!error)
		{
		    (void)disk_change(ffsbase,(struct dev *)((struct fh *)iofs->IOFS.io_Unit)->vol);
		}
		
		PutMsg(&ffsbase->rport, &iofs->IOFS.io_Message);
		continue;
		
	    case FSA_OPEN:
		error = open(ffsbase,(struct fh **)&iofs->IOFS.io_Unit,
			     iofs->io_Union.io_OPEN.io_Filename,
			     iofs->io_Union.io_OPEN.io_FileMode);
		break;
		
	    case FSA_OPEN_FILE:
		error = open_file(ffsbase, (struct fh **)&iofs->IOFS.io_Unit,
				  iofs->io_Union.io_OPEN_FILE.io_Filename,
				  iofs->io_Union.io_OPEN_FILE.io_FileMode,
				  iofs->io_Union.io_OPEN_FILE.io_Protection);
		break;
		
	    case FSA_READ:
		error = read(ffsbase, (struct fh *)iofs->IOFS.io_Unit,
			     iofs->io_Union.io_READ.io_Buffer,
			     &iofs->io_Union.io_READ.io_Length);
		break;
		
	    case FSA_CLOSE:
		error = free_lock(ffsbase, (struct fh *)iofs->IOFS.io_Unit);
		break;
		
	    case FSA_CREATE_DIR:
		error = create_dir(ffsbase, (struct fh **)&iofs->IOFS.io_Unit,
				   iofs->io_Union.io_CREATE_DIR.io_Filename,
				   iofs->io_Union.io_CREATE_DIR.io_Protection);
		break;
		
	    case FSA_EXAMINE:
		error = examine(ffsbase, (struct fh *)iofs->IOFS.io_Unit,
				iofs->io_Union.io_EXAMINE.io_ead,
				iofs->io_Union.io_EXAMINE.io_Size,
				iofs->io_Union.io_EXAMINE.io_Mode);
		break;
		
	    case FSA_EXAMINE_ALL:
		error = examine_all(ffsbase,(struct fh *)iofs->IOFS.io_Unit,
				    iofs->io_Union.io_EXAMINE_ALL.io_ead,
				    iofs->io_Union.io_EXAMINE_ALL.io_eac,
				    iofs->io_Union.io_EXAMINE_ALL.io_Size,
				    iofs->io_Union.io_EXAMINE_ALL.io_Mode);
		break;
		
	    default:
		error = ERROR_ACTION_NOT_KNOWN;
		break;
		    
    	    } /* switch(iofs->IOFS.io_Command) */
	    
    	    iofs->io_DosError = error;
    	    ReplyMsg(&iofs->IOFS.io_Message);
	    
    	} /* while((iofs=(struct IOFileSys *)GetMsg(&ffsbase->port))!=NULL) */
	
    	if (ffsbase->dlflag)
    	{
    	    if (AttemptLockDosList(LDF_VOLUMES | LDF_WRITE) != NULL)
    	    {
    	        while ((vol = (struct vol *)RemHead((struct List *)&ffsbase->removed)) != NULL)
    	        {
    	            (void)RemDosEntry(vol->dlist);
    	            FreeDosEntry(vol->dlist);
    	            FreeMem(vol, sizeof(struct vol));
    	        }

    	        while ((vol = (struct vol *)RemHead((struct List *)&ffsbase->inserted)) != NULL)
    	        {
    	            AddHead((struct List *)&ffsbase->mounted,(struct Node *)vol);
    	            (void)AddDosEntry(vol->dlist);
    	        }
		
    	        UnLockDosList(LDF_VOLUMES | LDF_WRITE);
    	        ffsbase->dlflag = 0;
    	    }
	    else
    	    {
    	        /* Wait some time then try again */
    	        Delay(TICKS_PER_SECOND/10);
    	        continue;
    	    }
	    
    	} /* if(ffsbase->dlflag) */

    	WaitPort(&ffsbase->port);
    } /* for(;;) */
}

/****************************************************************************************/

const char end = 0;

/****************************************************************************************/
