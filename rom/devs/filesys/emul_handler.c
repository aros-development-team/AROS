/*
    Copyright (C) 1995-1997 AROS - The Amiga Replacement OS
    $Id$

    Desc: Filesystem that accesses an underlying unix filesystem.
    Lang: english
*/
/* AROS includes */
#include <aros/system.h>
#include <aros/options.h>
#include <aros/libcall.h>
#include <exec/resident.h>
#include <exec/memory.h>
#include <exec/types.h>
#include <exec/alerts.h>
#include <proto/exec.h>
#include <utility/tagitem.h>
#include <dos/dosextens.h>
#include <dos/filesystem.h>
#include <dos/exall.h>
#include <dos/dosasl.h>
#include <proto/dos.h>
#include <proto/expansion.h>
#include <aros/libcall.h>
#include <aros/asmcall.h>
#include <libraries/expansion.h>
#include <libraries/configvars.h>
#include <libraries/expansionbase.h>
#include <aros/debug.h>

/* Unix includes */
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <string.h>
#include <stdlib.h>
#include <stddef.h>
#include <errno.h>
#define timeval sys_timeval
#include <sys/stat.h>
#include <sys/time.h>
#undef timeval
#ifdef __GNUC__
#   include "emul_handler_gcc.h"
#endif
#undef DOSBase

static const char name[];
static const char version[];
static const APTR inittabl[4];
static void *const functable[];
static const UBYTE datatable;

struct emulbase * AROS_SLIB_ENTRY(init,emul_handler) ();
void AROS_SLIB_ENTRY(open,emul_handler) ();
BPTR AROS_SLIB_ENTRY(close,emul_handler) ();
BPTR AROS_SLIB_ENTRY(expunge,emul_handler) ();
int AROS_SLIB_ENTRY(null,emul_handler) ();
void AROS_SLIB_ENTRY(beginio,emul_handler) ();
LONG AROS_SLIB_ENTRY(abortio,emul_handler) ();

static const char end;

struct filehandle
{
    char * name;
    int    type;
    long   fd;
};
#define FHD_FILE      0
#define FHD_DIRECTORY 1

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
    RTF_AUTOINIT | RTF_COLDSTART,
    41,
    NT_DEVICE,
    0,
    (char *)name,
    (char *)&version[6],
    (ULONG *)inittabl
};

static const char name[]="emul.handler";

static const char version[]="$VER: emul_handler 41.2 (14.7.1997)\r\n";

static const APTR inittabl[4]=
{
    (APTR)sizeof(struct emulbase),
    (APTR)functable,
    (APTR)&datatable,
    &AROS_SLIB_ENTRY(init,emul_handler)
};

static void *const functable[]=
{
    &AROS_SLIB_ENTRY(open,emul_handler),
    &AROS_SLIB_ENTRY(close,emul_handler),
    &AROS_SLIB_ENTRY(expunge,emul_handler),
    &AROS_SLIB_ENTRY(null,emul_handler),
    &AROS_SLIB_ENTRY(beginio,emul_handler),
    &AROS_SLIB_ENTRY(abortio,emul_handler),
    (void *)-1
};

static const UBYTE datatable=0;

/* Make an AROS filenumber out of an unix filenumber. */
LONG u2a[][2]=
{
  { ENOMEM, ERROR_NO_FREE_STORE },
  { ENOENT, ERROR_OBJECT_NOT_FOUND },
  { EEXIST, ERROR_OBJECT_EXISTS },
  { EACCES, ERROR_WRITE_PROTECTED },
  { ENOTDIR, ERROR_DIR_NOT_FOUND },
  { ENOSPC, ERROR_DISK_FULL },
  { ENOTEMPTY, ERROR_DIRECTORY_NOT_EMPTY },
  { 0, ERROR_UNKNOWN }
};

static LONG err_u2a(void)
{
    ULONG i;
    for(i=0;i<sizeof(u2a)/sizeof(u2a[0]);i++)
	if(u2a[i][0]==errno)
	    return u2a[i][1];
#if PassThroughErrnos
    return errno+PassThroughErrnos;
#else
    return ERROR_UNKNOWN;
#endif
}


/* Make unix protection bits out of amiga protection bits. */
mode_t prot_a2u(ULONG protect)
{
    mode_t uprot = 0000;

    if ((protect & FIBF_SCRIPT))
	uprot |= 0111;
    /* The following three flags are low-active! */
    if (!(protect & FIBF_EXECUTE))
	uprot |= 0100;
    if (!(protect & FIBF_WRITE))
	uprot |= 0200;
    if (!(protect & FIBF_READ))
	uprot |= 0400;
    if ((protect & FIBF_GRP_EXECUTE))
	uprot |= 0010;
    if ((protect & FIBF_GRP_WRITE))
	uprot |= 0020;
    if ((protect & FIBF_GRP_READ))
	uprot |= 0040;
    if ((protect & FIBF_OTR_EXECUTE))
	uprot |= 0001;
    if ((protect & FIBF_OTR_WRITE))
	uprot |= 0002;
    if ((protect & FIBF_OTR_READ))
	uprot |= 0004;

    return uprot;
}

/* Make amiga protection bits out of unix protection bits. */
ULONG prot_u2a(mode_t protect)
{
    ULONG aprot = FIBF_SCRIPT;

    /* The following three (amiga) flags are low-active! */
    if (!(protect & S_IRUSR))
	aprot |= FIBF_READ;
    if (!(protect & S_IWUSR))
	aprot |= FIBF_WRITE;
    if (!(protect & S_IXUSR))
	aprot |= FIBF_EXECUTE;

    /* The following flags are high-active again. */
    if ((protect & S_IRGRP))
	aprot |= FIBF_GRP_READ;
    if ((protect & S_IWGRP))
	aprot |= FIBF_GRP_WRITE;
    if ((protect & S_IXGRP))
	aprot |= FIBF_GRP_EXECUTE;
    if ((protect & S_IROTH))
	aprot |= FIBF_OTR_READ;
    if ((protect & S_IWOTH))
	aprot |= FIBF_OTR_WRITE;
    if ((protect & S_IXOTH))
	aprot |= FIBF_OTR_EXECUTE;

    return aprot;
}


/* Makes a direct path out of the supplied filename.
   Eg 'path1/path2//path3/' becomes 'path1/path3'.
*/
static void shrink(char *filename)
{
    char *s1,*s2;
    unsigned long len;
    for(;;)
    {
	/* strip all leading slashes */
	while(*filename=='/')
	    memmove(filename,filename+1,strlen(filename));

	/* remove superflous paths (ie paths that are followed by '//') */
	s1=strstr(filename,"//");
	if(s1==NULL)
	    break;
	s2=s1;
	while((s2 > filename) && (*--s2 != '/'))
	    ;
	memmove(s2,s1+2,strlen(s1+1));
    }

    /* strip trailing slash */
    len=strlen(filename);
    if(len&&filename[len-1]=='/')
	filename[len-1]=0;
}

static LONG makefilename(struct emulbase *emulbase,
			 char **dest, STRPTR dirname, STRPTR filename)
{
    LONG ret = 0;
    int len, dirlen;

    dirlen = strlen(dirname) + 1;
    len = strlen(filename) + dirlen + 1;
    *dest=(char *)malloc(len);
    if ((*dest))
    {
	CopyMem(dirname, *dest, dirlen);
	if (AddPart(*dest, filename, len))
	    shrink(*dest);
	else {
	    free(*dest);
	    *dest = NULL;
	    ret = ERROR_OBJECT_TOO_LARGE;
	}
    } else
	ret = ERROR_NO_FREE_STORE;

    return ret;
}


static LONG open_(struct emulbase *emulbase, struct filehandle **handle,STRPTR name,LONG mode)
{
    LONG ret = 0;
    struct filehandle *fh;
    struct stat st;
    long flags;

    fh=(struct filehandle *)malloc(sizeof(struct filehandle));
    if(fh!=NULL)
    {
	/* If no filename is given and the file-descriptor is one of the
	   standard filehandles (stdin, stdout, stderr) ... */
	if((!name[0]) && ((*handle)->type == FHD_FILE) &&
	   (((*handle)->fd == STDIN_FILENO) || ((*handle)->fd == STDOUT_FILENO) || ((*handle)->fd == STDERR_FILENO)))
	{
	    /* ... then just reopen that standard filehandle. */
	    fh->type=FHD_FILE;
	    fh->fd=(*handle)->fd;
	    fh->name="";
	    *handle=fh;
	    return 0;
	}

	ret = makefilename(emulbase, &fh->name, (*handle)->name, name);
	if (!ret)
	{
	    if(!stat(*fh->name?fh->name:".",&st))
	    {
		if(S_ISREG(st.st_mode))
		{
		    /* file is a plain file */
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
		    /* file is a directory */
		    fh->type=FHD_DIRECTORY;
		    fh->fd=(long)opendir(*fh->name?fh->name:".");
		    if(fh->fd)
		    {
			*handle=fh;
			return 0;
		    }
		}else
		  ret = ERROR_OBJECT_WRONG_TYPE;
	    }
	    /* stat() failed. If ret is unset, generate it from errno. */
	    if (!ret)
		ret = err_u2a();
	}
	free(fh);
    } else
	ret = ERROR_NO_FREE_STORE;
    return ret;
}

static LONG open_file(struct emulbase *emulbase, struct filehandle **handle,STRPTR name,LONG mode,LONG protect)
{
    LONG ret=ERROR_NO_FREE_STORE;
    struct filehandle *fh;
    mode_t prot;
    long flags;

    fh=(struct filehandle *)malloc(sizeof(struct filehandle));
    if(fh!=NULL)
    {
	/* If no filename is given and the file-descriptor is one of the
	   standard filehandles (stdin, stdout, stderr) ... */
	if ((!name[0]) && ((*handle)->type==FHD_FILE) &&
	    (((*handle)->fd==STDIN_FILENO) || ((*handle)->fd==STDOUT_FILENO) || ((*handle)->fd==STDERR_FILENO)))
	{
	    /* ... then just reopen that standard filehandle. */
	    fh->type=FHD_FILE;
	    fh->fd=(*handle)->fd;
	    fh->name="";
	    *handle=fh;
	    return 0;
	}

	ret = makefilename(emulbase, &fh->name, (*handle)->name, name);
	if (!ret)
	{
	    fh->type=FHD_FILE;
	    flags=(mode&FMF_CREATE?O_CREAT:0)|
		  (mode&FMF_CLEAR?O_TRUNC:0);
	    if(mode&FMF_WRITE)
		flags|=mode&FMF_READ?O_RDWR:O_WRONLY;
	    else
		flags|=O_RDONLY;
	    prot = prot_a2u((ULONG)protect);
	    fh->fd=open(fh->name,flags,prot);
	    if (fh->fd != -1)
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

static LONG create_dir(struct emulbase *emulbase, struct filehandle **handle,
		       STRPTR filename, IPTR protect)
{
    mode_t prot;
    LONG ret = 0;
    struct filehandle *fh;

    fh = (struct filehandle *)malloc(sizeof(struct filehandle));
    if (fh)
    {
	ret = makefilename(emulbase, &fh->name, (*handle)->name, filename);
	if (!ret)
	{
	    fh->type = FHD_DIRECTORY;
	    prot = prot_a2u((ULONG)protect);
	    if (!mkdir(fh->name, prot))
	    {
		*handle = fh;
		(*handle)->fd = (long)opendir((*handle)->name);
		if ((*handle)->fd)
		    return 0;
	    }
	    ret = err_u2a();
	}
	free(fh);
    } else
	ret = ERROR_NO_FREE_STORE;

    return ret;
}

static LONG delete_object(struct emulbase *emulbase, struct filehandle* fh,
			  STRPTR file)
{
    LONG ret = 0;
    char *filename;
    struct stat st;

    ret = makefilename(emulbase, &filename, fh->name, file);
    if (!ret)
    {
	if (!lstat(filename, &st))
	{
	    if (S_ISDIR(st.st_mode))
	    {
		if (rmdir(filename))
		    ret = err_u2a();
	    }
	    else
	    {
		if (unlink(filename))
		    ret = err_u2a();
	    }
	} else
	    ret = err_u2a();
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
    struct Library *ExpansionBase;
    struct filehandle *fhi, *fho, *fhe, *fhv;
    struct DeviceNode *dlv;
    LONG ret=ERROR_NO_FREE_STORE;

    ExpansionBase = OpenLibrary("expansion.library",0);
    if(ExpansionBase != NULL)
    {
    	fhi=(struct filehandle *)malloc(sizeof(struct filehandle));
      	if(fhi!=NULL)
	{
    	    fho=(struct filehandle *)malloc(sizeof(struct filehandle));
    	    if(fho!=NULL)
    	    {
    		fhe=(struct filehandle *)malloc(sizeof(struct filehandle));
    		if(fhe!=NULL)
    		{
    		    fhv=(struct filehandle *)malloc(sizeof(struct filehandle));
    		    if(fhv != NULL)
    		    {
    			struct stat st;
    
    			fhv->name = ".";
    			fhv->type = FHD_DIRECTORY;
    
    			/* Make sure that the root directory is valid */
    			if(!stat(fhv->name,&st) && S_ISDIR(st.st_mode))
    			{
    			    fhv->fd = (long)opendir(fhv->name);
    
    			    fhi->type = FHD_FILE;
    			    fhi->fd   = STDIN_FILENO;
    			    fhi->name = "";
    			    fho->type = FHD_FILE;
    			    fho->fd   = STDOUT_FILENO;
    			    fho->name = "";
    			    fhe->type = FHD_FILE;
    			    fhe->fd   = STDERR_FILENO;
    			    fhe->name = "";
    
    			    emulbase->stdin  = (struct Unit *)fhi;
    			    emulbase->stdout = (struct Unit *)fho;
    			    emulbase->stderr = (struct Unit *)fhe;
    
    			    /*
    				Allocate space for the string from same mem
    				"Workbench" total 11 bytes (9 + NULL + length)
				Add an extra 4 for alignment purposes.
    			    */
    			    ret=ERROR_NO_FREE_STORE;
    			    dlv = AllocMem(sizeof(struct DeviceNode) + 15, MEMF_CLEAR|MEMF_PUBLIC);
    			    if(dlv!=NULL)
    			    {
    				STRPTR s;
   
				/*  We want s to point to the first 4-byte 
				    aligned memory after the structure. 
				*/
				s = (STRPTR)(((IPTR)dlv + sizeof(struct DeviceNode) + 4) & ~3);
    				CopyMem("Workbench", &s[1], 9);
    				*s = 9;
    
    				dlv->dn_Type   = DLT_DEVICE;
    				dlv->dn_Unit   = (struct Unit *)fhv;
    				dlv->dn_Device = &emulbase->device;
    				dlv->dn_Handler = NULL;
    				dlv->dn_Startup = NULL;
    				dlv->dn_OldName = MKBADDR(s);
    				dlv->dn_NewName = &s[1];
    
    				AddBootNode( 0, 0, dlv, NULL);
    				return 0;
    			    }
    			} /* valid directory */
    			else
    			{
    			    /* If this was under config/ I could
    			       actually print out a message, but
    			       alas I don't have that liberty...
    
    			       It'd be nice to be able to add some
    			       extra alert definitions though...
    			    */
    			    Alert(AT_DeadEnd|AO_Unknown|AN_Unknown );
    			}
    			free_lock(fhv);
    		    }
    		    free(fhe);
    		}
    		free(fho);
    	    }
    	    free(fhi);
    	}
    }
    CloseLibrary(ExpansionBase);

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
	    ead->ed_Prot = prot_u2a(st.st_mode);
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
	    ead->ed_Next=(struct ExAllData *)(((IPTR)next+AROS_PTRALIGN-1)&~(AROS_PTRALIGN-1));
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
	    if (errno)
		error=err_u2a();
	    else
		error = 0;
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


AROS_LH2(struct emulbase *, init,
 AROS_LHA(struct emulbase *, emulbase, D0),
 AROS_LHA(BPTR,              segList,   A0),
	   struct ExecBase *, sysBase, 0, emul_handler)
{
    AROS_LIBFUNC_INIT
    static const struct TagItem tags[] = {{ TAG_END, 0 }};

    /* Store arguments */
    emulbase->sysbase=sysBase;
    emulbase->seglist=segList;
    emulbase->device.dd_Library.lib_OpenCnt=1;

    BOOPSIBase = OpenLibrary (BOOPSINAME,0);

    if (!BOOPSIBase)
	return NULL;

    emulbase->unixio = NewObjectA (NULL, UNIXIOCLASS, (struct TagItem *)tags);

    if (!emulbase->unixio)
    {
	CloseLibrary (BOOPSIBase);
	return NULL;
    }

    if(!startup(emulbase))
	return emulbase;

    DisposeObject (emulbase->unixio);
    CloseLibrary (BOOPSIBase);

    return NULL;
    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, open,
 AROS_LHA(struct IOFileSys *, iofs, A1),
 AROS_LHA(ULONG,              unitnum, D0),
 AROS_LHA(ULONG,              flags, D1),
	   struct emulbase *, emulbase, 1, emul_handler)
{
    AROS_LIBFUNC_INIT

    /* Keep compiler happy */
    unitnum=0;
    flags=0;

    if(emulbase->dosbase == NULL)
    {
	emulbase->dosbase = (struct DosLibrary *)OpenLibrary("dos.library", 0);
	if( emulbase->dosbase == NULL )
	{
		iofs->IOFS.io_Error = -1;
		return;
	} 
    }

   /* I have one more opener. */
    emulbase->device.dd_Library.lib_Flags&=~LIBF_DELEXP;

    /* Set returncode */
    iofs->IOFS.io_Error=0;

    /* Mark Message as recently used. */
    iofs->IOFS.io_Message.mn_Node.ln_Type=NT_REPLYMSG;
    AROS_LIBFUNC_EXIT
}

AROS_LH1(BPTR, close,
 AROS_LHA(struct IOFileSys *, iofs, A1),
	   struct emulbase *, emulbase, 2, emul_handler)
{
    AROS_LIBFUNC_INIT

    /* Let any following attemps to use the device crash hard. */
    iofs->IOFS.io_Device=(struct Device *)-1;
    return 0;
    AROS_LIBFUNC_EXIT
}

AROS_LH0(BPTR, expunge, struct emulbase *, emulbase, 3, emul_handler)
{
    AROS_LIBFUNC_INIT

    /* Do not expunge the device. Set the delayed expunge flag and return. */
    emulbase->device.dd_Library.lib_Flags|=LIBF_DELEXP;
    return 0;
    AROS_LIBFUNC_EXIT
}

AROS_LH0I(int, null, struct emulbase *, emulbase, 4, emul_handler)
{
    AROS_LIBFUNC_INIT
    return 0;
    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, beginio,
 AROS_LHA(struct IOFileSys *, iofs, A1),
	   struct emulbase *, emulbase, 5, emul_handler)
{
    AROS_LIBFUNC_INIT
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
	    error=open_(emulbase,
			(struct filehandle **)&iofs->IOFS.io_Unit,
			(char *)iofs->io_Args[0],iofs->io_Args[1]);
	    break;

	case FSA_OPEN_FILE:
	    error=open_file(emulbase,
			    (struct filehandle **)&iofs->IOFS.io_Unit,
			    (char *)iofs->io_Args[0],
			    iofs->io_Args[1],
			    iofs->io_Args[2]);
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
		if (fh->fd==STDOUT_FILENO)
		    fh->fd=STDIN_FILENO;

		error = DoMethod (emulbase->unixio,
		    HIDDM_WaitForIO, fh->fd, HIDDV_UnixIO_Read);

		if (error == 0) {
		    iofs->io_Args[1] = read (fh->fd,(APTR)iofs->io_Args[0],iofs->io_Args[1]);

		    if (iofs->io_Args[1]<0)
			error=err_u2a();
		}
		else
		{
		    errno = error;
		    error = err_u2a();
		}
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

		if (mode == OFFSET_BEGINNING)
		    mode = SEEK_SET;
		else if (mode == OFFSET_CURRENT)
		    mode = SEEK_CUR;
		else
		    mode = SEEK_END;

		if(lseek(fh->fd,iofs->io_Args[1],mode)<0)
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

	case FSA_SAME_LOCK: {
	    struct filehandle *lock1 = iofs->io_Union.io_SAME_LOCK.io_Lock[0],
			      *lock2 = iofs->io_Union.io_SAME_LOCK.io_Lock[1];

	    if (strcmp(lock1->name, lock2->name))
		iofs->io_Union.io_SAME_LOCK.io_Same = LOCK_DIFFERENT;
	    else
		iofs->io_Union.io_SAME_LOCK.io_Same = LOCK_SAME;
	    break;
	}

	case FSA_CREATE_DIR:
	    error = create_dir(emulbase,
			       (struct filehandle **)&iofs->IOFS.io_Unit,
			       iofs->io_Union.io_CREATE_DIR.io_Filename,
			       iofs->io_Union.io_CREATE_DIR.io_Protection);
	    break;

	case FSA_DELETE_OBJECT:
	    error = delete_object(emulbase,
				  (struct filehandle *)iofs->IOFS.io_Unit,
				  iofs->io_Union.io_DELETE_OBJECT.io_Filename);
	    break;

	default:
	    error=ERROR_ACTION_NOT_KNOWN;
	    break;
    }

    Enable();

    /* Set error code */
    iofs->io_DosError=error;

    /* If the quick bit is not set send the message to the port */
    if(!(iofs->IOFS.io_Flags&IOF_QUICK))
	ReplyMsg(&iofs->IOFS.io_Message);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(LONG, abortio,
 AROS_LHA(struct IOFileSys *, iofs, A1),
	   struct emulbase *, emulbase, 6, emul_handler)
{
    AROS_LIBFUNC_INIT
    /* Everything already done. */
    return 0;
    AROS_LIBFUNC_EXIT
}

static const char end=0;
