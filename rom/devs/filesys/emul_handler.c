/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.13  1996/10/21 20:57:50  aros
    ADE doesn't need to have the patch for timeval.

    Revision 1.12  1996/10/14 02:38:39  iaint
    FreeBSD patch no longer needed.

    Revision 1.11  1996/10/10 13:23:55  digulla
    Make handler work with timer (Fleischer)

    Revision 1.10  1996/09/13 17:57:07  digulla
    Use IPTR

    Revision 1.9  1996/09/13 04:23:23  aros
    Define FreeBSD should have been __FreeBSD__ (sorry my fault).

    Revision 1.8  1996/09/11 16:54:24  digulla
    Always use __AROS_SLIB_ENTRY() to access shared external symbols, because
	some systems name an external symbol "x" as "_x" and others as "x".
	(The problem arises with assembler symbols which might differ)

    Revision 1.7  1996/09/11 14:40:10  digulla
    Integrated patch by I. Templeton: Under FreeBSD, there is a clash with
	struct timeval

    Revision 1.6  1996/09/11 13:05:34  digulla
    Own function to open a file (M. Fleischer)

    Revision 1.5  1996/08/31 12:58:11  aros
    Merged in/modified for FreeBSD.

    Revision 1.4  1996/08/30 17:02:06  digulla
    Fixed a bug which caused the shell to exit if the timer sent a signal. This
	fix is a very bad hack :(

    Revision 1.3  1996/08/13 15:35:07  digulla
    Replaced __AROS_LA by __AROS_LHA

    Revision 1.2  1996/08/01 17:41:22  digulla
    Added standard header for all files

    Desc:
    Lang:
*/
#include <aros/system.h>
#ifndef _AMIGA  /* ADE <sys/time.h> has provisions for this */
#define DEVICES_TIMER_H /* avoid redefinition of struct timeval */
#endif /* _AMIGA */
#include <exec/resident.h>
#include <exec/memory.h>
#include <clib/exec_protos.h>
#include <utility/tagitem.h>
#include <dos/dosextens.h>
#include <dos/filesystem.h>
#include <dos/exall.h>
#include <dos/dosasl.h>
#include <clib/dos_protos.h>
#include <aros/libcall.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/time.h>
#ifdef __GNUC__
    #include "emul_handler_gcc.h"
#endif

static const char name[];
static const char version[];
static const APTR inittabl[4];
static void *const functable[];
static const UBYTE datatable;

struct emulbase * __AROS_SLIB_ENTRY(init,emul_handler) ();
void __AROS_SLIB_ENTRY(open,emul_handler) ();
BPTR __AROS_SLIB_ENTRY(close,emul_handler) ();
BPTR __AROS_SLIB_ENTRY(expunge,emul_handler) ();
int __AROS_SLIB_ENTRY(null,emul_handler) ();
void __AROS_SLIB_ENTRY(beginio,emul_handler) ();
LONG __AROS_SLIB_ENTRY(abortio,emul_handler) ();

static const char end;

struct filehandle
{
    char *name;
    int type;
    long fd;
};
#define FHD_FILE	0
#define FHD_DIRECTORY	1

int emul_handler_entry(void)
{
    /* If the device was executed by accident return error code. */
    return -1;
}

const struct Resident emul_handler_resident=
{
    RTC_MATCHWORD,
    (struct Resident *)&emul_handler_resident,
    (APTR)&end,
    RTF_AUTOINIT,
    1,
    NT_LIBRARY,
    0,
    (char *)name,
    (char *)&version[6],
    (ULONG *)inittabl
};

static const char name[]="emul.handler";

static const char version[]="$VER: emul_handler 1.0 (28.3.96)\n\015";

static const APTR inittabl[4]=
{
    (APTR)sizeof(struct emulbase),
    (APTR)functable,
    (APTR)&datatable,
    &__AROS_SLIB_ENTRY(init,emul_handler)
};

static void *const functable[]=
{
    &__AROS_SLIB_ENTRY(open,emul_handler),
    &__AROS_SLIB_ENTRY(close,emul_handler),
    &__AROS_SLIB_ENTRY(expunge,emul_handler),
    &__AROS_SLIB_ENTRY(null,emul_handler),
    &__AROS_SLIB_ENTRY(beginio,emul_handler),
    &__AROS_SLIB_ENTRY(abortio,emul_handler),
    (void *)-1
};

static const UBYTE datatable=0;

LONG u2a[][2]=
{
  { ENOMEM, ERROR_NO_FREE_STORE },
  { ENOENT, ERROR_OBJECT_NOT_FOUND },
  { 0, 0 }
};

LONG err_u2a(void)
{
    ULONG i;
    for(i=0;i<sizeof(u2a)/sizeof(u2a[0]);i++)
	if(u2a[i][0]==errno)
	    break;
    return u2a[i][1];
}

static void shrink(char *filename)
{
    char *s1,*s2;
    unsigned long len;
    for(;;)
    {
	while(*filename=='/')
	    memmove(filename,filename+1,strlen(filename));
	s1=strstr(filename,"//");
	if(s1==NULL)
	    break;
	s2=s1;
	while(s2>filename&&*--s2!='/')
	    ;
	memmove(s2,s1+2,strlen(s1+1));
    }
    len=strlen(filename);
    if(len&&filename[len-1]=='/')
	filename[len-1]=0;
}

static LONG open_(struct filehandle **handle,STRPTR name,LONG mode)
{
    LONG ret=ERROR_NO_FREE_STORE;
    struct filehandle *fh;
    struct stat st;
    long flags;
    fh=(struct filehandle *)malloc(sizeof(struct filehandle));
    if(fh!=NULL)
    {
	if(!*name&&(*handle)->type==FHD_FILE&&((*handle)->fd==STDIN_FILENO||
	   (*handle)->fd==STDOUT_FILENO||(*handle)->fd==STDERR_FILENO))
	{
	    fh->type=FHD_FILE;
	    fh->fd=(*handle)->fd;
	    fh->name="";
	    *handle=fh;
	    return 0;
	}
	fh->name=(char *)malloc(strlen((*handle)->name)+strlen(name)+2);
	if(fh->name!=NULL)
	{
	    strcpy(fh->name,(*handle)->name);
	    strcat(fh->name,"/");
	    strcat(fh->name,name);
	    shrink(fh->name);
	    if(!stat(*fh->name?fh->name:".",&st))
	    {
		if(S_ISREG(st.st_mode))
		{
		    fh->type=FHD_FILE;
		    flags=(mode&FMF_CREATE?O_CREAT:0)|
			  (mode&FMF_CLEAR?O_TRUNC:0);
		    if(mode&FMF_WRITE)
			flags|=mode&FMF_READ?O_RDWR:O_WRONLY;
		    else
			flags|=O_RDONLY;
		    fh->fd=open(*fh->name?fh->name:".",flags,0770);
		    if(fh->fd>=0)
		    {
			*handle=fh;
			return 0;
		    }
		}else if(S_ISDIR(st.st_mode))
		{
		    fh->type=FHD_DIRECTORY;
		    fh->fd=(long)opendir(*fh->name?fh->name:".");
		    if(fh->fd)
		    {
			*handle=fh;
			return 0;
		    }
		}else
		    errno=ENOENT;
	    }
	    ret=err_u2a();
	    free(fh->name);
	}
	free(fh);
    }
    return ret;
}

static LONG open_file(struct filehandle **handle,STRPTR name,LONG mode,LONG protect)
{
    LONG ret=ERROR_NO_FREE_STORE;
    struct filehandle *fh;
    long flags;
    fh=(struct filehandle *)malloc(sizeof(struct filehandle));
    if(fh!=NULL)
    {
	if(!*name&&(*handle)->type==FHD_FILE&&((*handle)->fd==STDIN_FILENO||
	   (*handle)->fd==STDOUT_FILENO||(*handle)->fd==STDERR_FILENO))
	{
	    fh->type=FHD_FILE;
	    fh->fd=(*handle)->fd;
	    fh->name="";
	    *handle=fh;
	    return 0;
	}
	fh->name=(char *)malloc(strlen((*handle)->name)+strlen(name)+2);
	if(fh->name!=NULL)
	{
	    strcpy(fh->name,(*handle)->name);
	    strcat(fh->name,"/");
	    strcat(fh->name,name);
	    shrink(fh->name);
	    fh->type=FHD_FILE;
	    flags=(mode&FMF_CREATE?O_CREAT:0)|
		  (mode&FMF_CLEAR?O_TRUNC:0);
	    if(mode&FMF_WRITE)
		flags|=mode&FMF_READ?O_RDWR:O_WRONLY;
	    else
		flags|=O_RDONLY;
	    fh->fd=open(fh->name,flags,0770);
	    if(fh->fd>=0)
	    {
		*handle=fh;
		return 0;
	    }
	    ret=err_u2a();
	    free(fh->name);
	}
	free(fh);
    }
    return ret;
}

static LONG free_lock(struct filehandle *current)
{
    switch(current->type)
    {
	case FHD_FILE:
	    if(current->fd!=STDIN_FILENO&&current->fd!=STDOUT_FILENO&&
	       current->fd!=STDERR_FILENO)
	    {
		close(current->fd);
		free(current->name);
	    }
	    break;
	case FHD_DIRECTORY:
	    closedir((DIR *)current->fd);
	    free(current->name);
	    break;
    }
    free(current);
    return 0;
}

static LONG startup(struct emulbase *emulbase)
{
    struct filehandle *fhi, *fho, *fhe, *fhv, *fhc, *fhs;
    struct DosList *dlv, *dlc;
    static struct filehandle sys={ "", FHD_DIRECTORY, 0 };
    LONG ret=ERROR_NO_FREE_STORE;

    fhi=(struct filehandle *)malloc(sizeof(struct filehandle));
    if(fhi!=NULL)
    {
	fho=(struct filehandle *)malloc(sizeof(struct filehandle));
	if(fho!=NULL)
	{
	    fhe=(struct filehandle *)malloc(sizeof(struct filehandle));
	    if(fhe!=NULL)
	    {
		fhv=&sys;
		ret=open_(&fhv,"",0);
		if(!ret)
		{
		    fhc=&sys;
		    ret=open_(&fhc,"",0);
		    if(!ret)
		    {
			fhs=&sys;
			ret=open_(&fhs,"",0);
			if(!ret)
			{
			    ret=ERROR_NO_FREE_STORE;
			    dlv=MakeDosEntry("Workbench",DLT_VOLUME);
			    if(dlv!=NULL)
			    {
				dlc=MakeDosEntry("SYS",DLT_DEVICE);
				if(dlc!=NULL)
				{
				    ret=ERROR_OBJECT_EXISTS;
				    dlv->dol_Unit  =(struct Unit *)fhv;
				    dlv->dol_Device=&emulbase->device;
				    dlc->dol_Unit  =(struct Unit *)fhc;
				    dlc->dol_Device=&emulbase->device;
				    fhi->type=FHD_FILE;
				    fhi->fd=STDIN_FILENO;
				    fhi->name="";
				    emulbase->stdin=(struct Unit *)fhi;
				    fho->type=FHD_FILE;
				    fho->fd=STDOUT_FILENO;
				    fho->name="";
				    emulbase->stdout=(struct Unit *)fho;
				    fhe->type=FHD_FILE;
				    fhe->fd=STDERR_FILENO;
				    fhe->name="";
				    emulbase->stderr=(struct Unit *)fhe;
				    if(AddDosEntry(dlv))
				    {
					if(AddDosEntry(dlc))
					{
					    DOSBase->dl_NulHandler=&emulbase->device;
					    DOSBase->dl_NulLock   =(struct Unit *)fhs;
					    return 0;
					}
					RemDosEntry(dlv);
				    }
				    FreeDosEntry(dlc);
				}
				FreeDosEntry(dlv);
			    }
			    free_lock(fhs);
			}
			free_lock(fhc);
		    }
		    free_lock(fhv);
		}
		free(fhe);
	    }
	    free(fho);
	}
	free(fhi);
    }
    return ret;
}

static const ULONG sizes[]=
{ 0, offsetof(struct ExAllData,ed_Type), offsetof(struct ExAllData,ed_Size),
  offsetof(struct ExAllData,ed_Prot), offsetof(struct ExAllData,ed_Days),
  offsetof(struct ExAllData,ed_Comment), offsetof(struct ExAllData,ed_OwnerUID),
  sizeof(struct ExAllData) };

static LONG examine(struct filehandle *fh,struct ExAllData *ead,ULONG size,ULONG type)
{
    STRPTR next, end, last, name;
    struct stat st;
    if(type>ED_OWNER)
	return ERROR_BAD_NUMBER;
    next=(STRPTR)ead+sizes[type];
    end =(STRPTR)ead+size;
    if(next>=end)
	return ERROR_BUFFER_OVERFLOW;
    if(lstat(*fh->name?fh->name:".",&st))
	return err_u2a();
    switch(type)
    {
	default:
	case ED_OWNER:
	    ead->ed_OwnerUID=st.st_uid;
	    ead->ed_OwnerGID=st.st_gid;
	case ED_COMMENT:
	    ead->ed_Comment=NULL;
	case ED_DATE:
	    ead->ed_Days=st.st_ctime/(60*60*24)-(6*365+2*366);
	    ead->ed_Mins=(st.st_ctime/60)%(60*24);
	    ead->ed_Ticks=(st.st_ctime%60)*TICKS_PER_SECOND;
	case ED_PROTECTION:
	    ead->ed_Prot=(st.st_mode&S_IRUSR?FIBF_READ:0)|
			 (st.st_mode&S_IWUSR?FIBF_WRITE:0)|
			 (st.st_mode&S_IXUSR?FIBF_EXECUTE:0)|
			 FIBF_SCRIPT|FIBF_DELETE;
	case ED_SIZE:
	    ead->ed_Size=st.st_size;
	case ED_TYPE:
	    ead->ed_Type=S_ISREG(st.st_mode)?ST_FILE:
			 S_ISDIR(st.st_mode)?(*fh->name?ST_USERDIR:ST_ROOT):0;
	case ED_NAME:
	    ead->ed_Name=next;
	    last=name=*fh->name?fh->name:"Workbench";
	    while(*name)
		if(*name++=='/')
		    last=name;
	    for(;;)
	    {
		if(next>=end)
		    return ERROR_BUFFER_OVERFLOW;
		if(!(*next++=*last++))
		    break;
	    }
	case 0:
	    ead->ed_Next=(struct ExAllData *)(((IPTR)next+PTRALIGN-1)&~(PTRALIGN-1));
	    return 0;
    }
}

static LONG examine_all(struct filehandle *fh,struct ExAllData *ead,ULONG size,ULONG type)
{
    struct ExAllData *last=NULL;
    STRPTR end=(STRPTR)ead+size, name, old;
    off_t oldpos;
    struct dirent *dir;
    LONG error;
    if(fh->type!=FHD_DIRECTORY)
	return ERROR_OBJECT_WRONG_TYPE;
    for(;;)
    {
	oldpos=telldir((DIR *)fh->fd);
	errno=0;
	dir=readdir((DIR *)fh->fd);
	if(dir==NULL)
	{
	    error=err_u2a();
	    break;
	}
	if(dir->d_name[0]=='.'&&(!dir->d_name[1]||(dir->d_name[1]=='.'&&!dir->d_name[2])))
	    continue;
	name=(STRPTR)malloc(strlen(fh->name)+strlen(dir->d_name)+2);
	if(name==NULL)
	{
	    error=ERROR_NO_FREE_STORE;
	    break;
	}
	strcpy(name,fh->name);
	if(*name)
	    strcat(name,"/");
	strcat(name,dir->d_name);
	old=fh->name;
	fh->name=name;
	error=examine(fh,ead,end-(STRPTR)ead,type);
	fh->name=old;
	free(name);
	if(error)
	    break;
	last=ead;
	ead=ead->ed_Next;
    }
    if((!error||error==ERROR_BUFFER_OVERFLOW)&&last!=NULL)
    {
	last->ed_Next=NULL;
	seekdir((DIR *)fh->fd,oldpos);
	return 0;
    }
    if(!error)
	error=ERROR_NO_MORE_ENTRIES;
    rewinddir((DIR *)fh->fd);
    return error;
}

__AROS_LH2(struct emulbase *, init,
 __AROS_LHA(struct emulbase *, emulbase, D0),
 __AROS_LHA(BPTR,              segList,   A0),
	   struct ExecBase *, sysBase, 0, emul_handler)
{
    __AROS_FUNC_INIT

    /* Store arguments */
    emulbase->sysbase=sysBase;
    emulbase->seglist=segList;
    emulbase->device.dd_Library.lib_OpenCnt=1;
    emulbase->dosbase=(struct DosLibrary *)OpenLibrary("dos.library",39);
    if(emulbase->dosbase!=NULL)
    {
	if(AttemptLockDosList(LDF_ALL|LDF_WRITE))
	{
	    if(!startup(emulbase))
	    {
		UnLockDosList(LDF_ALL|LDF_WRITE);
		return emulbase;
	    }
	    UnLockDosList(LDF_ALL|LDF_WRITE);
	}
	CloseLibrary((struct Library *)emulbase->dosbase);
    }

    return NULL;
    __AROS_FUNC_EXIT
}

__AROS_LH3(void, open,
 __AROS_LHA(struct IOFileSys *, iofs, A1),
 __AROS_LHA(ULONG,              unitnum, D0),
 __AROS_LHA(ULONG,              flags, D0),
	   struct emulbase *, emulbase, 1, emul_handler)
{
    __AROS_FUNC_INIT

    /* Keep compiler happy */
    unitnum=0;
    flags=0;

    /* I have one more opener. */
    emulbase->device.dd_Library.lib_Flags&=~LIBF_DELEXP;

    /* Set returncode */
    iofs->IOFS.io_Error=0;

    /* Mark Message as recently used. */
    iofs->IOFS.io_Message.mn_Node.ln_Type=NT_REPLYMSG;
    __AROS_FUNC_EXIT
}

__AROS_LH1(BPTR, close,
 __AROS_LHA(struct IOFileSys *, iofs, A1),
	   struct emulbase *, emulbase, 2, emul_handler)
{
    __AROS_FUNC_INIT

    /* Let any following attemps to use the device crash hard. */
    iofs->IOFS.io_Device=(struct Device *)-1;
    return 0;
    __AROS_FUNC_EXIT
}

__AROS_LH0(BPTR, expunge, struct emulbase *, emulbase, 3, emul_handler)
{
    __AROS_FUNC_INIT

    /* Do not expunge the device. Set the delayed expunge flag and return. */
    emulbase->device.dd_Library.lib_Flags|=LIBF_DELEXP;
    return 0;
    __AROS_FUNC_EXIT
}

__AROS_LH0I(int, null, struct emulbase *, emulbase, 4, emul_handler)
{
    __AROS_FUNC_INIT
    return 0;
    __AROS_FUNC_EXIT
}

__AROS_LH1(void, beginio,
 __AROS_LHA(struct IOFileSys *, iofs, A1),
	   struct emulbase *, emulbase, 5, emul_handler)
{
    __AROS_FUNC_INIT
    LONG error=0;

    /* WaitIO will look into this */
    iofs->IOFS.io_Message.mn_Node.ln_Type=NT_MESSAGE;

    Disable();

    /*
	Do everything quick no matter what. This is possible
	because I never need to Wait().
    */
    switch(iofs->IOFS.io_Command)
    {
	case FSA_OPEN:
	    error=open_((struct filehandle **)&iofs->IOFS.io_Unit,
			 (char *)iofs->io_Args[0],iofs->io_Args[1]);
	    break;

	case FSA_OPEN_FILE:
	    error=open_file((struct filehandle **)&iofs->IOFS.io_Unit,
			 (char *)iofs->io_Args[0],
			 iofs->io_Args[1],iofs->io_Args[2]);
	    break;

	case FSA_CLOSE:
	    error=free_lock((struct filehandle *)iofs->IOFS.io_Unit);
	    break;

	case FSA_IS_INTERACTIVE:
	{
	    struct filehandle *fh=(struct filehandle *)iofs->IOFS.io_Unit;
	    if(fh->type==FHD_FILE)
		iofs->io_Args[0]=isatty(fh->fd);
	    else
		iofs->io_Args[0]=0;
	    break;
	}

	case FSA_READ:
	{
	    struct filehandle *fh=(struct filehandle *)iofs->IOFS.io_Unit;
	    if(fh->type==FHD_FILE)
	    {
		if(fh->fd==STDOUT_FILENO)
		    fh->fd=STDIN_FILENO;
	        for(;;)
	        {
	            fd_set rfds;
	            struct timeval tv;
	            FD_ZERO(&rfds);
	            FD_SET(fh->fd,&rfds);
	            tv.tv_sec=0;
	            tv.tv_usec=100000;
	            if(select(fh->fd+1,&rfds,NULL,NULL,&tv))
	                break;
	            SysBase->ThisTask->tc_State=TS_READY;
	            AddTail(&SysBase->TaskReady,&SysBase->ThisTask->tc_Node);
	            Switch();
	        }
	        
		iofs->io_Args[1]=read(fh->fd,(APTR)iofs->io_Args[0],iofs->io_Args[1]);
		if(iofs->io_Args[1]<0)
		    error=err_u2a();
	    }else
		error=ERROR_OBJECT_WRONG_TYPE;
	    break;
	}

	case FSA_WRITE:
	{
	    struct filehandle *fh=(struct filehandle *)iofs->IOFS.io_Unit;
	    if(fh->type==FHD_FILE)
	    {
		if(fh->fd==STDIN_FILENO)
		    fh->fd=STDOUT_FILENO;
		iofs->io_Args[1]=write(fh->fd,(APTR)iofs->io_Args[0],iofs->io_Args[1]);
		if(iofs->io_Args[1]<0)
		    error=err_u2a();
	    }else
		error=ERROR_OBJECT_WRONG_TYPE;
	    break;
	}

	case FSA_SEEK:
	{
	    struct filehandle *fh=(struct filehandle *)iofs->IOFS.io_Unit;
	    LONG mode=iofs->io_Args[2];
	    LONG oldpos;
	    if(fh->type==FHD_FILE)
	    {
		oldpos=lseek(fh->fd,0,SEEK_CUR);
		if(lseek(fh->fd,iofs->io_Args[1],mode==OFFSET_BEGINNING?SEEK_SET:mode==OFFSET_CURRENT?SEEK_CUR:SEEK_END)<0)
		    error=err_u2a();
		iofs->io_Args[0]=0;
		iofs->io_Args[1]=oldpos;
	    }else
		error=ERROR_OBJECT_WRONG_TYPE;
	    break;
	}

	case FSA_EXAMINE:
	    error=examine((struct filehandle *)iofs->IOFS.io_Unit,
			  (struct ExAllData *)iofs->io_Args[0],
			  iofs->io_Args[1],iofs->io_Args[2]);
	    break;

	case FSA_EXAMINE_ALL:
	    error=examine_all((struct filehandle *)iofs->IOFS.io_Unit,
			  (struct ExAllData *)iofs->io_Args[0],
			  iofs->io_Args[1],iofs->io_Args[2]);
	    break;

	default:
	    error=ERROR_NOT_IMPLEMENTED;
	    break;
    }

    Enable();

    /* Set error code */
    iofs->io_DosError=error;

    /* If the quick bit is not set send the message to the port */
    if(!(iofs->IOFS.io_Flags&IOF_QUICK))
	ReplyMsg(&iofs->IOFS.io_Message);

    __AROS_FUNC_EXIT
}

__AROS_LH1(LONG, abortio,
 __AROS_LHA(struct IOFileSys *, iofs, A1),
	   struct emulbase *, emulbase, 6, emul_handler)
{
    __AROS_FUNC_INIT
    /* Everything already done. */
    return 0;
    __AROS_FUNC_EXIT
}

static const char end=0;
