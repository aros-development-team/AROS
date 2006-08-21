/*
    Copyright © 1995-2006, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Filesystem that accesses an underlying POSIX filesystem.
    Lang: english
*/

/* Implementing this handler is quite complicated as it uses AROS system-calls
   as well as POSIX calls of the underlying operating system. This easily
   leads to complications. So take care, when updating this handler!

   Please always update the version-string below, if you modify the code!
*/

/*********************************************************************************************/

/* AROS includes */

# define  DEBUG  1
# include <aros/debug.h>


#include <aros/system.h>
#include <aros/options.h>
#include <aros/symbolsets.h>
#include <exec/resident.h>
#include <exec/memory.h>
#include <exec/alerts.h>
#include <proto/exec.h>
#include <utility/tagitem.h>
#include <dos/filesystem.h>
#include <dos/exall.h>
#include <dos/dosasl.h>
#include <dos/bptr.h>
#define __DOS_NOLIBBASE__
#include <proto/dos.h>
#include <proto/expansion.h>
#include <libraries/expansion.h>
#include <libraries/configvars.h>
#include <libraries/expansionbase.h>
#include <proto/oop.h>
#include <oop/oop.h>

#include <aros/host-conf.h>

/* POSIX includes */
#define timeval sys_timeval
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <string.h>
#include <stdlib.h>
#include <stddef.h>
#include <errno.h>
#include <sys/stat.h>
#ifdef HAVE_SYS_STATFS_H
#   include <sys/statfs.h>
#endif
#include <sys/time.h>
#include <pwd.h>

#undef timeval

#ifdef HAVE_SYS_VFS_H
#   include <sys/vfs.h>
#endif
#ifdef HAVE_SYS_PARAM_H
#   include <sys/param.h>
#endif
#ifdef HAVE_SYS_MOUNT_H
#   include <sys/mount.h>
#endif

/* Underlying OS's rename() Clib function prototype */
#include <stdio.h>

#include "emul_handler_intern.h"
#if defined(__GNUC__) || defined(__INTEL_COMPILER)
#   include "emul_handler_gcc.h"
#endif

#include <aros/debug.h>
#include <string.h>

#include "/usr/include/signal.h"

#include LC_LIBDEFS_FILE

/*********************************************************************************************/

/* Init DOSBase ourselves because emul_handler is initialized before dos.library */
static struct DosLibrary *DOSBase;

#define NO_CASE_SENSITIVITY 1

#define malloc you_must_change_malloc_to__emul_malloc
#define free you_must_change_free_to__emul_free

/*********************************** Support *******************************/

/* Make an AROS error-code (<dos/dos.h>) out of an unix error-code. */
static LONG u2a[][2]=
{
  { ENOMEM, ERROR_NO_FREE_STORE },
  { ENOENT, ERROR_OBJECT_NOT_FOUND },
  { EEXIST, ERROR_OBJECT_EXISTS },
  { EACCES, ERROR_WRITE_PROTECTED }, /* AROS distinguishes between different
                                        kinds of privelege violation. Therefore
                                        a routine using err_u2a() should check
                                        for ERROR_WRITE_PROTECTED and replace
                                        it by a different constant, if
                                        necessary. */
  { ENOTDIR, ERROR_DIR_NOT_FOUND },
  { ENOSPC, ERROR_DISK_FULL },
  { ENOTEMPTY, ERROR_DIRECTORY_NOT_EMPTY },
  { EISDIR, ERROR_OBJECT_WRONG_TYPE },
  { ETXTBSY, ERROR_OBJECT_IN_USE },
  { ENAMETOOLONG, ERROR_OBJECT_TOO_LARGE },
  { EROFS, ERROR_WRITE_PROTECTED },
  { 0, ERROR_UNKNOWN }
};

/*********************************************************************************************/

static APTR emul_malloc(struct emulbase *emulbase, ULONG size)
{
    ULONG *res;

    // kprintf("** emul_malloc: size = %d **\n",size);
    ObtainSemaphore(&emulbase->memsem);

    size += sizeof(ULONG);
    res = AllocPooled(emulbase->mempool, size);
    if (res) *res++ = size;

    ReleaseSemaphore(&emulbase->memsem);

    // kprintf("** emul_malloc: size = %d  result = %x **\n",size-sizeof(ULONG),res);

    return res;
}

/*********************************************************************************************/

static void emul_free(struct emulbase *emulbase, APTR mem)
{
    if (!mem)
    {
        kprintf("*** emul_handler: tried to free NULL mem ***\n");
    } else {
        ULONG *m = (ULONG *)mem;
        ULONG size = *--m;

        // kprintf("** emul_free: size = %d  memory = %x **\n",size-sizeof(ULONG),mem);

	ObtainSemaphore(&emulbase->memsem);
	FreePooled(emulbase->mempool, m, size);
	ReleaseSemaphore(&emulbase->memsem);
    }
}

/*********************************************************************************************/

static BOOL is_root_filename(char *filename)
{
    BOOL result = FALSE;
    
    if ((*filename == '\0') ||
        (!strcmp(filename, ".")) ||
	(!strcmp(filename, "./")))
    {
        result = TRUE;
    }
    
    return result;
}

/*********************************************************************************************/

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

/*********************************************************************************************/

/* Create a plain path out of the supplied filename.
   Eg 'path1/path2//path3/' becomes 'path1/path3'.
*/
BOOL shrink(struct emulbase *emulbase, char *filename)
{
    char	  *s1,*s2;
    unsigned long len;

    if (filename[0] == '.')
        if (filename[1] == '/') filename += 2;
	
    for(;;)
    {
	/* leading slashes? --> return FALSE. */
	if(*filename=='/') return FALSE;

	/* remove superflous paths (ie paths that are followed by '//') */
	s1=strstr(filename,"//");
	if(s1==NULL)
	    break;
	s2=s1;
	while(s2 > filename)
	{
	    if (s2[-1] == '/') break;
	    s2--;
	}
	
	memmove(s2,s1+2,strlen(s1+1));
    }

    /* strip trailing slash */
    len=strlen(filename);
    if(len&&filename[len-1]=='/')
	filename[len-1]=0;

   return TRUE;
}

/*********************************************************************************************/

/* Allocate a buffer, in which the filename is appended to the pathname. */
static LONG makefilename(struct emulbase *emulbase,
			 char **dest, STRPTR dirname, STRPTR filename)
{
    LONG ret = 0;
    int len, dirlen;
    dirlen = strlen(dirname) + 1;
    len = strlen(filename) + dirlen + 1 + /*safety*/ 1;
    *dest=(char *)emul_malloc(emulbase, len);
    if ((*dest))
    {
	CopyMem(dirname, *dest, dirlen);
	if (dirlen > 1)
	{
	    if ((*dest)[dirlen - 2] != '/') strcat(*dest, "/");
	}

	strcat(*dest, filename);

	if (!shrink(emulbase, *dest))
	{
	    emul_free(emulbase, *dest);
	    *dest = NULL;
	    ret = ERROR_OBJECT_NOT_FOUND;
	}
    } else
	ret = ERROR_NO_FREE_STORE;

    return ret;
}

/*********************************************************************************************/

#if NO_CASE_SENSITIVITY

static void fixcase(struct emulbase *emulbase, char *pathname)
{
    struct dirent 	*de;
    struct stat		st;
    DIR			*dir;
    char		*pathstart, *pathend;
    BOOL		dirfound;
    
    pathstart = pathname;
    
    if (stat((const char *)pathname, &st) != 0)
    {
        /* file/dir called pathname does not exist */
	
	while((pathstart = strchr(pathstart, '/')))
	{
	    pathstart++;
	    
	    pathend = strchr(pathstart, '/');
	    if (pathend) *pathend = '\0';

	    dirfound = TRUE;
	    
	    if (stat((const char *)pathname, &st) != 0)
	    {
	    	dirfound = FALSE;

        	pathstart[-1] = '\0';
		dir = opendir(pathname);
		pathstart[-1] = '/';		
		
		if (dir)
		{
		    while((de = readdir(dir)))
		    {
        		if (strcasecmp(de->d_name, pathstart) == 0)
			{
			    dirfound = TRUE;
			    strcpy(pathstart, de->d_name);
			    break;
			}
		    }	    
		    closedir(dir);

		} /* if ((dir = opendir(pathname))) */

	    } /* if (stat((const char *)pathname, &st) != 0) */
	    
	    if (pathend) *pathend = '/';			    

	    if (!dirfound) break;

	} /* while((pathpos = strchr(pathpos, '/))) */
	
    } /* if (stat((const char *)pathname, &st) != 0) */

}

/*-------------------------------------------------------------------------------------------*/

static int nocase_open(struct emulbase *emulbase, char *pathname, int flags, mode_t mode)
{
    int	result;

    fixcase(emulbase, pathname);
    result = open((const char *)pathname, flags, mode);
        
    return result;        
}

/*-------------------------------------------------------------------------------------------*/

static DIR *nocase_opendir(struct emulbase *emulbase, char *name)
{
    DIR *result;
    
    fixcase(emulbase, name);
    result = opendir((const char *)name);
    
    return result;
}

/*-------------------------------------------------------------------------------------------*/

static int nocase_stat(struct emulbase *emulbase, char *file_name, struct stat *buf)
{
    int result;
    
    fixcase(emulbase, file_name);
    result = stat((const char *)file_name, buf);
    
    return result;
}
  
/*-------------------------------------------------------------------------------------------*/

static int nocase_lstat(struct emulbase *emulbase, char *file_name, struct stat *buf)
{
    int result;
    
    fixcase(emulbase, file_name);
    result = lstat((const char *)file_name, buf);
    
    return result;
}

/*-------------------------------------------------------------------------------------------*/

static int nocase_unlink(struct emulbase *emulbase, char *pathname)
{
    int result;
    
    fixcase(emulbase, pathname);
    result = unlink((const char *)pathname);
    
    return result;
}

/*-------------------------------------------------------------------------------------------*/

static int nocase_mkdir(struct emulbase *emulbase, char *pathname, mode_t mode)
{
    int result;
    
    fixcase(emulbase, pathname);
    result = mkdir((const char *)pathname, mode);
    
    return result;
}


/*-------------------------------------------------------------------------------------------*/

static int nocase_rmdir(struct emulbase *emulbase, char *pathname)
{
    int result;
    
    fixcase(emulbase, pathname);
    result = rmdir((const char *)pathname);
    
    return result;
}

/*-------------------------------------------------------------------------------------------*/

static int nocase_link(struct emulbase *emulbase, char *oldpath, char *newpath)
{
    int result;
    
    fixcase(emulbase, oldpath);
    fixcase(emulbase, newpath);
    result = link((const char *)oldpath, (const char *)newpath);
    
    return result;
}

/*-------------------------------------------------------------------------------------------*/

static int nocase_symlink(struct emulbase *emulbase, char *oldpath, char *newpath)
{ 
    int result;
    
    fixcase(emulbase, oldpath);
    fixcase(emulbase, newpath);
    result = symlink((const char *)oldpath, (const char *)newpath);
    
    return result;
}

/*-------------------------------------------------------------------------------------------*/

static int nocase_rename(struct emulbase *emulbase, char *oldpath, char *newpath)
{
    int result;
    
    fixcase(emulbase, oldpath);
    fixcase(emulbase, newpath);
    result = rename((const char *)oldpath, (const char *)newpath);
    
    return result;
    
}

/*-------------------------------------------------------------------------------------------*/

#undef open
#define open(a,b,c) nocase_open(emulbase, a, b, c)

#undef opendir
#define opendir(a) nocase_opendir(emulbase, a)

#undef stat
#define stat(a,b) nocase_stat(emulbase, a, b)

#undef lstat
#define lstat(a,b) nocase_lstat(emulbase, a, b)

#undef unlink
#define unlink(a) nocase_unlink(emulbase, a)

#undef mkdir
#define mkdir(a,b) nocase_mkdir(emulbase, a, b)

#undef rmdir
#define rmdir(a) nocase_rmdir(emulbase,a)

#undef link
#define link(a,b) nocase_link(emulbase, a, b)

#undef symlink
#define symlink(a,b) nocase_symlink(emulbase, a, b)

#undef rename
#define rename(a,b) nocase_rename(emulbase, a, b)

#endif /* NO_CASE_SENSITIVITY */

/*********************************************************************************************/

/* Make unix protection bits out of AROS protection bits. */
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

/*********************************************************************************************/

/* Make AROS protection bits out of unix protection bits. */
ULONG prot_u2a(mode_t protect)
{
    ULONG aprot = FIBF_SCRIPT;

    /* The following three (AROS) flags are low-active! */
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

/*********************************************************************************************/

static BOOL check_volume(struct filehandle *fh, struct emulbase *emulbase)
{
    if (fh->volume == emulbase->current_volume) return TRUE;
    
    if (chdir(fh->volume) == 0)
    {
    	emulbase->current_volume = fh->volume;
	return TRUE;
    }
    
    return FALSE;
}

/*********************************************************************************************/

/* Free a filehandle */
static LONG free_lock(struct emulbase *emulbase, struct filehandle *current)
{
    switch(current->type)
    {
	case FHD_FILE:
	    if(current->fd!=STDIN_FILENO&&current->fd!=STDOUT_FILENO&&
	       current->fd!=STDERR_FILENO)
	    {
		close(current->fd);
		emul_free(emulbase, current->name);

		if (current->pathname)
		{
		  emul_free(emulbase, current->pathname);
		}

		if (current->DIR)
		{
		  closedir(current->DIR);
		}
	    }
	    break;
	case FHD_DIRECTORY:
            if (current->fd)
            {
	      closedir((DIR *)current->fd);
	    }
	    
	    if (current->name)
	        emul_free(emulbase, current->name);
	    break;
    }
    FreeMem(current, sizeof(struct filehandle));
    return 0;
}

/*********************************************************************************************/

static LONG open_(struct emulbase *emulbase, struct filehandle **handle,STRPTR name,LONG mode)
{
    LONG ret = 0;
    struct filehandle *fh;
    struct stat st;
    long flags;

    if (!check_volume(*handle, emulbase)) return ERROR_OBJECT_NOT_FOUND;
    
    fh=(struct filehandle *)AllocMem(sizeof(struct filehandle), MEMF_PUBLIC);
    if(fh!=NULL)
    {
        fh->pathname = NULL; /* just to make sure... */
        fh->DIR      = NULL;
	/* If no filename is given and the file-descriptor is one of the
	   standard filehandles (stdin, stdout, stderr) ... */
	if((!name[0]) && ((*handle)->type == FHD_FILE) &&
	   (((*handle)->fd == STDIN_FILENO) || ((*handle)->fd == STDOUT_FILENO) || ((*handle)->fd == STDERR_FILENO)))
	{
	    /* ... then just reopen that standard filehandle. */
	    fh->type=FHD_FILE;
	    fh->fd=(*handle)->fd;
	    fh->name="";
	    fh->volume=NULL;
	    fh->volumename=NULL;
	    *handle=fh;
	    return 0;
	}

	fh->volume=(*handle)->volume;
    	fh->volumename=(*handle)->volumename;
	
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

	    emul_free(emulbase, fh->name);
	}
	FreeMem(fh, sizeof(struct filehandle));
    } else
	ret = ERROR_NO_FREE_STORE;
    return ret;
}

/*********************************************************************************************/

static LONG open_file(struct emulbase *emulbase, struct filehandle **handle,STRPTR name,LONG mode,LONG protect)
{
    LONG ret=ERROR_NO_FREE_STORE;
    struct filehandle *fh;
    mode_t prot;
    long flags;

    if (!check_volume(*handle, emulbase)) return ERROR_OBJECT_NOT_FOUND;

    fh=(struct filehandle *)AllocMem(sizeof(struct filehandle), MEMF_PUBLIC);
    if(fh!=NULL)
    {
        fh->pathname = NULL; /* just to make sure... */
        fh->DIR      = NULL;
	/* If no filename is given and the file-descriptor is one of the
	   standard filehandles (stdin, stdout, stderr) ... */
	if ((!name[0]) && ((*handle)->type==FHD_FILE) &&
	    (((*handle)->fd==STDIN_FILENO) || ((*handle)->fd==STDOUT_FILENO) || ((*handle)->fd==STDERR_FILENO)))
	{
	    /* ... then just reopen that standard filehandle. */
	    fh->type=FHD_FILE;
	    fh->fd=(*handle)->fd;
	    fh->name="";
	    fh->volume=0;
	    *handle=fh;
	    return 0;
	}

	fh->volume=(*handle)->volume;
    	fh->volumename=(*handle)->volumename;

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
	    emul_free(emulbase, fh->name);
	}
	FreeMem(fh, sizeof(struct filehandle));
    }
    return ret;
}

/*********************************************************************************************/

static LONG create_dir(struct emulbase *emulbase, struct filehandle **handle,
		       STRPTR filename, IPTR protect)
{
    mode_t prot;
    LONG ret = 0;
    struct filehandle *fh;

    if (!check_volume(*handle, emulbase)) return ERROR_OBJECT_NOT_FOUND;

    fh = (struct filehandle *)AllocMem(sizeof(struct filehandle), MEMF_PUBLIC);
    if (fh)
    {
        fh->pathname   = NULL; /* just to make sure... */
	fh->name       = NULL;
        fh->DIR        = NULL;
	fh->fd         = 0;
	fh->volume     = (*handle)->volume;
    	fh->volumename =(*handle)->volumename;
	
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
	free_lock(emulbase, fh);
    } else
	ret = ERROR_NO_FREE_STORE;

    return ret;
}

/*********************************************************************************************/

static LONG delete_object(struct emulbase *emulbase, struct filehandle* fh,
			  STRPTR file)
{
    LONG ret = 0;
    char *filename = NULL;
    struct stat st;

    if (!check_volume(fh, emulbase)) return ERROR_OBJECT_NOT_FOUND;

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
	emul_free(emulbase, filename);
    }

    return ret;
}

/*********************************************************************************************/

static LONG startup(struct emulbase *emulbase)
{
    struct Library *ExpansionBase;
    struct filehandle *fhi, *fho, *fhe, *fhv;
    struct DeviceNode *dlv, *dlv2;
    LONG ret = ERROR_NO_FREE_STORE;

    ExpansionBase = OpenLibrary("expansion.library",0);
    if(ExpansionBase != NULL)
    {
	fhi=(struct filehandle *)AllocMem(sizeof(struct filehandle), MEMF_PUBLIC);
	if(fhi!=NULL)
	{
            fhi->pathname   = NULL; /* just to make sure... */
            fhi->DIR        = NULL;
	    fhi->volume     = NULL;
	    fhi->volumename = NULL;
	    
	    fho=(struct filehandle *)AllocMem(sizeof(struct filehandle), MEMF_PUBLIC);
	    if(fho!=NULL)
	    {
                fho->pathname   = NULL; /* just to make sure... */
                fho->DIR        = NULL;
    	    	fho->volume     = NULL;
		fho->volumename = NULL;
		
		fhe=(struct filehandle *)AllocMem(sizeof(struct filehandle), MEMF_PUBLIC);
		if(fhe!=NULL)
		{
                    fhe->pathname   = NULL; /* just to make sure... */
                    fhe->DIR        = NULL;
		    fhe->volume     = NULL;
		    fhe->volumename = NULL;

		    fhv=(struct filehandle *)AllocMem(sizeof(struct filehandle) + 256 + AROS_WORSTALIGN, MEMF_PUBLIC);
		    if(fhv != NULL)
		    {
			struct stat st;

			fhv->name = ".";
			fhv->type = FHD_DIRECTORY;
                        fhv->pathname = NULL; /* just to make sure... */
                        fhv->DIR      = NULL;
    	    	    	fhv->volume=(char *)(fhv + 1);
			
			/* Make sure that the root directory is valid */
			if(getcwd(fhv->volume, 256) && !stat(fhv->name,&st) && S_ISDIR(st.st_mode))
			{
			    #define DEVNAME "EMU"
			    #define VOLNAME "System"
			    
			    static const char *devname = DEVNAME;
			    static const char *volname = VOLNAME;

    	    	    	    fhv->volumename = VOLNAME;
			    emulbase->current_volume = fhv->volume;
			    
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

			    emulbase->eb_stdin  = (struct Unit *)fhi;
			    emulbase->eb_stdout = (struct Unit *)fho;
			    emulbase->eb_stderr = (struct Unit *)fhe;

			    /*
				Allocate space for the string from same mem
				Add 1 for BSTR size.
				Add an extra 4 for alignment purposes.
			    */
			    ret = ERROR_NO_FREE_STORE;

			    dlv = AllocMem(sizeof(struct DeviceNode) + 4 + sizeof(DEVNAME) + 1,
					   MEMF_CLEAR | MEMF_PUBLIC);

			    dlv2 = AllocMem(sizeof(struct DeviceNode) + 4 + sizeof(VOLNAME) + 1,
					    MEMF_CLEAR | MEMF_PUBLIC);

			    if(dlv != NULL && dlv2 != NULL)
			    {
				STRPTR s;
				STRPTR s2;
    	    	    	    	WORD   i;

				/*  We want s to point to the first 4-byte
				    aligned memory after the structure.
				*/
				s = (STRPTR)(((IPTR)dlv + sizeof(struct DeviceNode) + 4) & ~3);
				s2 = (STRPTR)(((IPTR)dlv2 + sizeof(struct DeviceNode) + 4) & ~3);
				
    	    	    	    	for(i = 0; i < sizeof(DEVNAME) - 1; i++)
				{
				    AROS_BSTR_putchar(s, i, devname[i]);
				}
				AROS_BSTR_setstrlen(s, sizeof(DEVNAME) - 1);
				
				dlv->dn_Type    = DLT_DEVICE;
				dlv->dn_Unit    = (struct Unit *)fhv;
				dlv->dn_Device  = &emulbase->device;
				dlv->dn_Handler = NULL;
				dlv->dn_Startup = NULL;
				dlv->dn_OldName = MKBADDR(s);
				dlv->dn_NewName = AROS_BSTR_ADDR(dlv->dn_OldName);

				AddBootNode(0, 0, dlv, NULL);


				/* Unfortunately, we cannot do the stuff below
				   as dos is not yet initialized... */
				// AddDosEntry(MakeDosEntry("System", 
				//			    DLT_VOLUME));

   	    	    	    	for(i = 0; i < sizeof(VOLNAME) - 1; i++)
				{
				    AROS_BSTR_putchar(s2, i, volname[i]);
				}
				AROS_BSTR_setstrlen(s2, sizeof(VOLNAME) - 1);

				dlv2->dn_Type    = DLT_VOLUME;
				dlv2->dn_Unit    = (struct Unit *)fhv;
				dlv2->dn_Device  = &emulbase->device;
				dlv2->dn_Handler = NULL;
				dlv2->dn_Startup = NULL;
				dlv2->dn_OldName = MKBADDR(s2);
				dlv2->dn_NewName = AROS_BSTR_ADDR(dlv2->dn_OldName);

				/* Make sure this is not booted from */
				AddBootNode(-128, 0, dlv2, NULL);

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
			free_lock(emulbase, fhv);
		    }
		    FreeMem(fhe, sizeof(struct filehandle));
		}
		FreeMem(fho, sizeof(struct filehandle));
	    }
	    FreeMem(fhi, sizeof(struct filehandle));
	}
    }
    CloseLibrary(ExpansionBase);

    return ret;
}

/*********************************************************************************************/

static const ULONG sizes[]=
{ 0, offsetof(struct ExAllData,ed_Type), offsetof(struct ExAllData,ed_Size),
  offsetof(struct ExAllData,ed_Prot), offsetof(struct ExAllData,ed_Days),
  offsetof(struct ExAllData,ed_Comment), offsetof(struct ExAllData,ed_OwnerUID),
  sizeof(struct ExAllData) };

/*********************************************************************************************/

/* Returns a emul_malloc()'ed buffer, containing a pathname, stripped by the
   filename.
*/
char * pathname_from_name (struct emulbase *emulbase, char * name)
{
  long len = strlen(name);
  long i = len;
  char * result = NULL;
  /* look for the first '/' in the filename starting at the end */
  while (i != 0 && name[i] != '/')
    i--;

  if (0 != i)
  {
    result = (char *)emul_malloc(emulbase, i+1);
    if(!result)
      return NULL;
    strncpy(result, name, i);
    result[i]=0x0;
  }
  return result;
}

/*********************************************************************************************/

/* Returns a emul_malloc()'ed buffer, containing the filename without its path. */
char * filename_from_name(struct emulbase *emulbase, char * name)
{
  long len = strlen(name);
  long i = len;
  char * result = NULL;
  /* look for the first '/' in the filename starting at the end */
  while (i != 0 && name[i] != '/')
    i--;

  if (0 != i)
  {
    result = (char *)emul_malloc(emulbase, len-i);
    if(!result)
      return NULL;
    strncpy(result, &name[i+1], len-i);
    result[len-i-1]=0x0;
  }
  return result;
}

/*********************************************************************************************/

static LONG examine(struct emulbase *emulbase,
		    struct filehandle *fh,
                    struct ExAllData *ead,
                    ULONG  size,
                    ULONG  type,
                    off_t  *dirpos)
{
    STRPTR next, end, last, name;
    struct stat st;

    /* Return an error, if supplied type is not supported. */
    if(type>ED_OWNER)
	return ERROR_BAD_NUMBER;

    /* Check, if the supplied buffer is large enough. */
    next=(STRPTR)ead+sizes[type];
    end =(STRPTR)ead+size;
    
    if(next>end) /* > is correct. Not >= */
	return ERROR_BUFFER_OVERFLOW;

    if (!check_volume(fh, emulbase)) return ERROR_OBJECT_NOT_FOUND;

    if(lstat(*fh->name?fh->name:".",&st))
      return err_u2a();
    
    if (FHD_FILE == fh->type)
       /* What we have here is a file, so it's no that easy to
          deal with it when the user is calling ExNext() after
          Examine(). So I better prepare it now. */
    {
      /* We're going to opendir the directory where the file is in
         and then actually start searching for the file. Yuk! */
      if (NULL == fh->pathname)
      {
        struct dirent * dirEnt;
        char * filename;
        fh->pathname = pathname_from_name(emulbase, fh->name);
        filename     = filename_from_name(emulbase, fh->name);
        if(!fh->pathname || !filename)
        {
          emul_free(emulbase, filename);
          return ERROR_NO_FREE_STORE;
        }
        fh->DIR      = opendir(fh->pathname);
        if(!fh->DIR)
        {
          emul_free(emulbase, filename);
          return err_u2a();
        }
        do 
        {
          errno = 0;
          dirEnt = readdir(fh->DIR);
        }
        while (NULL != dirEnt &&
               0    != strcmp(dirEnt->d_name, filename));
        emul_free(emulbase, filename);
        if(!dirEnt)
        {
          if(!errno)
            return ERROR_NO_MORE_ENTRIES;
          else
            return err_u2a();
        }

        *dirpos = (LONG)telldir(fh->DIR);
        
      }
    }
    else
    {
      *dirpos = (LONG)telldir((DIR *)fh->fd);
    }
    switch(type)
    {
	default:
	case ED_OWNER:
	    ead->ed_OwnerUID	= st.st_uid;
	    ead->ed_OwnerGID	= st.st_gid;
	case ED_COMMENT:
	    ead->ed_Comment=NULL;
	case ED_DATE:
	    ead->ed_Days	= st.st_ctime/(60*60*24)-(6*365+2*366);
	    ead->ed_Mins	= (st.st_ctime/60)%(60*24);
	    ead->ed_Ticks	= (st.st_ctime%60)*TICKS_PER_SECOND;
	case ED_PROTECTION:
	    ead->ed_Prot 	= prot_u2a(st.st_mode);
	case ED_SIZE:
	    ead->ed_Size	= st.st_size;
	case ED_TYPE:
            if (S_ISDIR(st.st_mode))
	    {
	        if (is_root_filename(fh->name))
		{
                   ead->ed_Type = ST_ROOT;
		} else {
		   ead->ed_Type = ST_USERDIR;
		}
	    } else {
	        ead->ed_Type 	= ST_FILE;
	    }
	    
	case ED_NAME:
	    ead->ed_Name=next;
	    last=name=is_root_filename(fh->name)?fh->volumename:fh->name;

            /* do not show the "." but "" instead */
	    if (last[0] == '.' && last[1] == '\0')
	      last=name="";

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

/*********************************************************************************************/

static LONG examine_next(struct emulbase *emulbase, 
			 struct filehandle *fh,
                         struct FileInfoBlock *FIB)
{
    int		i;
    struct stat    st;
    struct dirent *dir;
    char 	  *name, *src, *dest, *pathname;
    DIR		  *ReadDIR;
    /* first of all we have to go to the position where Examine() or
       ExNext() stopped the previous time so we can read the next entry! */
 
    if (!check_volume(fh, emulbase)) return ERROR_OBJECT_NOT_FOUND;

    switch(fh->type)
    {
    case FHD_DIRECTORY:
        seekdir((DIR *)fh->fd, FIB->fib_DiskKey);
        pathname = fh->name; /* it's just a directory! */
        ReadDIR  = (DIR *)fh->fd;
	break;
	
    case FHD_FILE:
        seekdir(fh->DIR, FIB->fib_DiskKey);
        pathname = fh->pathname;
        ReadDIR  = fh->DIR;
	break; 
    }
    /* hm, let's read the data now! 
       but skip '.' and '..' (they're not available on Amigas and
       Amiga progs wouldn't know how to treat '.' and '..', i.e. they
       might want to scan recursively the directory and end up scanning
       ./././ etc. */
    /*#undef kprintf*/
    do
    {
	dir = readdir(ReadDIR);

	if (NULL == dir)
	    return ERROR_NO_MORE_ENTRIES;  
	
    } while ( 0 == strcmp(dir->d_name,"." ) || 
	      0 == strcmp(dir->d_name,"..") ); 
    
    
    name = (STRPTR)emul_malloc(emulbase,
			       strlen(pathname) + strlen(dir->d_name) + 2);
    
    if (NULL == name)
	return ERROR_NO_FREE_STORE;
  
    strcpy(name, pathname);
    
    if (*name)
	strcat(name, "/");
    
    strcat(name, dir->d_name);
    
    if (stat(name, &st))
    {
	D(bug("stat() failed for %s\n", name));
	      
	emul_free(emulbase, name);

	return err_u2a();
    }
    
    emul_free(emulbase, name);
    
    FIB->fib_OwnerUID	    = st.st_uid;
    FIB->fib_OwnerGID	    = st.st_gid;
    FIB->fib_Comment[0]	    = '\0'; /* no comments available yet! */
    FIB->fib_Date.ds_Days   = st.st_ctime/(60*60*24) - (6*365 + 2*366);
    FIB->fib_Date.ds_Minute = (st.st_ctime/60)%(60*24);
    FIB->fib_Date.ds_Tick   = (st.st_ctime%60)*TICKS_PER_SECOND;
    FIB->fib_Protection	    = prot_u2a(st.st_mode);
    FIB->fib_Size           = st.st_size;

    if (S_ISDIR(st.st_mode))
    {
	FIB->fib_DirEntryType = ST_USERDIR; /* S_ISDIR(st.st_mode)?(*fh->name?ST_USERDIR:ST_ROOT):0*/
    }
    else
    {
	FIB->fib_DirEntryType = ST_FILE;
    }
    
    /* fast copying of the filename */
    src  = dir->d_name;
    dest = FIB->fib_FileName;

    for (i =0; i<MAXFILENAMELENGTH-1;i++)
    {
	if(! (*dest++=*src++) )
	{
	    break;
	}
    }
    
    FIB->fib_DiskKey = (LONG)telldir(ReadDIR);

    return 0;
}

/*********************************************************************************************/

static LONG examine_all(struct emulbase *emulbase,
			struct filehandle *fh,
                        struct ExAllData *ead,
                        struct ExAllControl *eac,
                        ULONG  size,
                        ULONG  type)
{
    struct ExAllData *last=NULL;
    STRPTR end=(STRPTR)ead+size, name, old;
    off_t oldpos;
    struct dirent *dir;
    LONG error;
    off_t dummy; /* not anything is done with this value but passed to examine */

    eac->eac_Entries = 0;
    if(fh->type!=FHD_DIRECTORY)
	return ERROR_OBJECT_WRONG_TYPE;

    if (!check_volume(fh, emulbase)) return ERROR_OBJECT_NOT_FOUND;

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
	name=(STRPTR)emul_malloc(emulbase, strlen(fh->name)+strlen(dir->d_name)+2);
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
	error=examine(emulbase,fh,ead,end-(STRPTR)ead,type,&dummy);
	fh->name=old;
	emul_free(emulbase, name);
	if(error)
	    break;
	eac->eac_Entries++;
	last=ead;
	ead=ead->ed_Next;
    }
    if (last!=NULL)
	last->ed_Next=NULL;
    if((error==ERROR_BUFFER_OVERFLOW)&&last!=NULL)
    {
	seekdir((DIR *)fh->fd,oldpos);
	return 0;
    }
    if(!error)
	error=ERROR_NO_MORE_ENTRIES;
    rewinddir((DIR *)fh->fd);
    return error;
}

/*********************************************************************************************/

static LONG create_hardlink(struct emulbase *emulbase,
			    struct filehandle **handle,STRPTR name,struct filehandle *oldfile)
{
    LONG error=0L;
    struct filehandle *fh;

    if (!check_volume(*handle, emulbase)) return ERROR_OBJECT_NOT_FOUND;

    fh = AllocMem(sizeof(struct filehandle), MEMF_PUBLIC);
    if (!fh)
      return ERROR_NO_FREE_STORE;

    fh->pathname = NULL; /* just to make sure... */
    fh->DIR      = NULL;
    
    error = makefilename(emulbase, &fh->name, (*handle)->name, name);
    if (!error)
    {
        if (!link(oldfile->name, fh->name))
            *handle = fh;
        else
            error = err_u2a();
    } else
    {
        error = ERROR_NO_FREE_STORE;
        FreeMem(fh, sizeof(struct filehandle));
    }

    return error;
}

/*********************************************************************************************/

static LONG create_softlink(struct emulbase * emulbase,
                            struct filehandle **handle, STRPTR name, STRPTR ref)
{
    LONG error=0L;
    struct filehandle *fh;

    if (!check_volume(*handle, emulbase)) return ERROR_OBJECT_NOT_FOUND;

    fh = AllocMem(sizeof(struct filehandle), MEMF_PUBLIC);
    if(!fh)
      return ERROR_NO_FREE_STORE;

    fh->pathname = NULL; /* just to make sure... */
    fh->DIR      = NULL;
    
    error = makefilename(emulbase, &fh->name, (*handle)->name, name);
    if (!error)
    {
        if (!symlink(ref, fh->name))
            *handle = fh;
        else
            error = err_u2a();
    } else
    {
        error = ERROR_NO_FREE_STORE;
        FreeMem(fh, sizeof(struct filehandle));
    }

    return error;
}

/*********************************************************************************************/

static LONG rename_object(struct emulbase * emulbase,
                            struct filehandle *fh, STRPTR file, STRPTR newname)
{
    LONG ret = 0L;

    char *filename = NULL , *newfilename = NULL;

    if (!check_volume(fh, emulbase)) return ERROR_OBJECT_NOT_FOUND;

    ret = makefilename(emulbase, &filename, fh->name, file);
    if (!ret)
    {
	ret = makefilename(emulbase, &newfilename, fh->name, newname);
	if (!ret)
	{
	    if (rename(filename,newfilename))
		ret = err_u2a();
	    emul_free(emulbase, newfilename);
	}
	emul_free(emulbase, filename);
    }

    return ret;
}


static LONG read_softlink(struct emulbase *emulbase,
                          struct filehandle *fh,
                          STRPTR buffer,
                          ULONG size)
{
    if (!check_volume(fh, emulbase)) return ERROR_OBJECT_NOT_FOUND;

    if (readlink(fh->name, buffer, size-1) == -1)
        return err_u2a();

    return 0L;
}

/*********************************************************************************************/

ULONG parent_dir(struct emulbase *emulbase,
		 struct filehandle *fh,
	         char ** DirName)
{
  *DirName = pathname_from_name(emulbase, fh->name);
  return 0;
}

/*********************************************************************************************/

void parent_dir_post(struct emulbase *emulbase, char ** DirName)
{
  /* free the previously allocated memory */
  emul_free(emulbase, *DirName);
  **DirName = 0;
}

/*********************************************************************************************/

/************************ Library entry points ************************/

static int GM_UNIQUENAME(Init)(LIBBASETYPEPTR emulbase)
{
    static const struct TagItem tags[] = {{ TAG_END, 0 }};

    D(bug("Initializing emul_handler\n"));

    InitSemaphore(&emulbase->sem);
    InitSemaphore(&emulbase->memsem);
    
    emulbase->mempool = CreatePool(MEMF_ANY, 4096, 2000);
    if (!emulbase->mempool) return FALSE;
    
    emulbase->unixio = OOP_NewObject (NULL, CLID_Hidd_UnixIO, (struct TagItem *)tags);

    if (!emulbase->unixio)
    {
	D(bug("Could not get UnixIO object\n"));
        DeletePool(emulbase->mempool);
	return FALSE;
    }

    if(!startup(emulbase))
    {
	D(bug("emul_handler initialized OK\n"));
	return TRUE;
    }

    OOP_DisposeObject (emulbase->unixio);
    DeletePool(emulbase->mempool);

    D(bug("emul_handler startup failed\n"));
       
    return FALSE;
}

/*********************************************************************************************/

static BOOL new_volume(struct IOFileSys *iofs, struct emulbase *emulbase)
{
    struct filehandle 	*fhv;
    struct DosList  	*doslist;
    struct stat     	 st;
    char    	    	*unixpath;
    
    ObtainSemaphore(&emulbase->sem);

    /* Volume name and Unix path are encoded into DEVICE entry of
       MountList like this: <volumename>:<unixpath> */
       
    unixpath = iofs->io_Union.io_OpenDevice.io_DeviceName;
    unixpath = strchr(unixpath, ':');
    
    if (unixpath)
    {
    	char *sp;
	
    	*unixpath++ = '\0';
	
	if ((sp = strchr(unixpath, '~')))
	{
	    char *home = NULL;
	    char *newunixpath = 0;
	    char *sp_end;
	    BOOL do_endpwent = FALSE;
	    BOOL ok = FALSE;
	    
	    /* "~<name>" means home of user <name> */
	     
	    if ((sp[1] == '\0') || (sp[1] == '/'))
	    {
	    	sp_end = sp + 1;
		home = getenv("HOME");
	    }
	    else
	    {
	    	struct passwd *pwd;
		WORD  	       cmplen;
		
		for(sp_end = sp + 1;
		    sp_end[0] != '\0' && sp_end[0] != '/';
		    sp_end++)
		{
		}
		
	    
		cmplen = sp_end - sp - 1;

		while((pwd = getpwent()))
		{
		    if(memcmp(pwd->pw_name, sp + 1, cmplen) == 0)
		    {
	    	    	if (pwd->pw_name[cmplen] == '\0')
			{
		    	    home = pwd->pw_dir;
			    break;
			}
		    }
		}
		
		do_endpwent = TRUE;
	    }
	    
	    if (home)
	    {
	    	newunixpath = AllocVec(strlen(unixpath) + strlen(home) + 1, MEMF_CLEAR);
	    	if (newunixpath)
	    	{
		    strncpy(newunixpath, unixpath, sp - unixpath);
		    strcat(newunixpath, home);
		    strcat(newunixpath, sp_end);
		    
		    ok = TRUE;
		    unixpath = newunixpath;
	    	}
	    }
	    
	    if (do_endpwent) endpwent();
	    
	    if (!ok)
	    {
	    	unixpath = 0;
		if (newunixpath) FreeVec(newunixpath);
	    }
	}
	
	if (unixpath)
	{
	    if (!stat(unixpath, &st))
	    {
		fhv=(struct filehandle *)AllocMem(sizeof(struct filehandle), MEMF_PUBLIC);
		if (fhv != NULL)
		{
		    fhv->name       = ".";
		    fhv->type       = FHD_DIRECTORY;
        	    fhv->pathname   = NULL; /* just to make sure... */
        	    fhv->DIR        = NULL;
    		    fhv->volume     = unixpath;
		    fhv->volumename = iofs->io_Union.io_OpenDevice.io_DeviceName;

		    if ((doslist = MakeDosEntry(fhv->volumename, DLT_VOLUME)))
		    {
			doslist->dol_Unit=(struct Unit *)fhv;
			doslist->dol_Device=&emulbase->device;
			AddDosEntry(doslist);

			iofs->IOFS.io_Unit   = (struct Unit *)fhv;
			iofs->IOFS.io_Device = &emulbase->device;

    			ReleaseSemaphore(&emulbase->sem);

			return TRUE;

		    } /* if ((doslist = MakeDosEntry(fhv->volumename, DLT_VOLUME))) */

		    FreeMem(fhv, sizeof(struct filehandle));

		} /* if (fhv != NULL)*/

	    } /* if (!stat(unixpath, &st)) */
	    
	} /* if (unixpath) */

    } /* if (unixpath) */
    
    ReleaseSemaphore(&emulbase->sem);
    
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

/*********************************************************************************************/

ADD2INITLIB(GM_UNIQUENAME(Init), 0)
ADD2OPENDEV(GM_UNIQUENAME(Open), 0)

/*********************************************************************************************/

AROS_LH1(void, beginio,
 AROS_LHA(struct IOFileSys *, iofs, A1),
	   struct emulbase *, emulbase, 5, emul_handler)
{
    AROS_LIBFUNC_INIT

    LONG error = 0;

    /* WaitIO will look into this */
    iofs->IOFS.io_Message.mn_Node.ln_Type=NT_MESSAGE;

    /* Disable(); */
    ObtainSemaphore(&emulbase->sem);

    /*
      Do everything quick no matter what. This is possible
      because I never need to Wait().
    */
    switch(iofs->IOFS.io_Command)
    {
    case FSA_OPEN:
	error = open_(emulbase,
		      (struct filehandle **)&iofs->IOFS.io_Unit,
		      iofs->io_Union.io_OPEN.io_Filename,
		      iofs->io_Union.io_OPEN.io_FileMode);
	if (
			(error == ERROR_WRITE_PROTECTED) &&
			(iofs->io_Union.io_OPEN.io_FileMode & FMF_AMIGADOS)
		)
	{
		error = open_(emulbase,
                      (struct filehandle **)&iofs->IOFS.io_Unit,
                      iofs->io_Union.io_OPEN.io_Filename,
                      iofs->io_Union.io_OPEN.io_FileMode & (~FMF_WRITE));
	}

	break;

    case FSA_CLOSE:
	error = free_lock(emulbase, (struct filehandle *)iofs->IOFS.io_Unit);
	break;
	
    case FSA_READ:
	{
	    struct filehandle *fh = (struct filehandle *)iofs->IOFS.io_Unit;
	    
	    if (fh->type == FHD_FILE)
	    {
		if (fh->fd == STDOUT_FILENO)
		{
		    fh->fd = STDIN_FILENO;
		}

		error = Hidd_UnixIO_Wait(emulbase->unixio, fh->fd,
					 vHidd_UnixIO_Read, NULL, NULL,
					 SysBase);
		
		if (error == 0)
		{
		    iofs->io_Union.io_READ.io_Length = read(fh->fd, iofs->io_Union.io_READ.io_Buffer, iofs->io_Union.io_READ.io_Length);
		    
		    if (iofs->io_Union.io_READ.io_Length < 0)
		    {
			error = err_u2a();
		    }
		}
		else
		{
		    errno = error;
		    error = err_u2a();
		}
	    }
	    else
	    {
		error = ERROR_OBJECT_WRONG_TYPE;
	    }
	    
	    break;
	}
	
    case FSA_WRITE:
	{
	    struct filehandle *fh = (struct filehandle *)iofs->IOFS.io_Unit;

	    if (fh->type == FHD_FILE)
	    {
		if (fh->fd == STDIN_FILENO)
		{
		    fh->fd=STDOUT_FILENO;
		}

		iofs->io_Union.io_WRITE.io_Length = write(fh->fd, iofs->io_Union.io_WRITE.io_Buffer, iofs->io_Union.io_WRITE.io_Length);

		if (iofs->io_Union.io_WRITE.io_Length < 0)
		{
		    error = err_u2a();
		}
	    }
	    else
	    {
		error = ERROR_OBJECT_WRONG_TYPE;
	    }
	    
	    break;
	}
	
    case FSA_SEEK:
	{
	    struct filehandle *fh = (struct filehandle *)iofs->IOFS.io_Unit;
	    LONG mode = iofs->io_Union.io_SEEK.io_SeekMode;
	    LONG oldpos;

	    if (fh->type == FHD_FILE)
	    {
		oldpos = lseek(fh->fd, 0, SEEK_CUR);

		if (mode == OFFSET_BEGINNING)
		{
		    mode = SEEK_SET;
		}
		else if (mode == OFFSET_CURRENT)
		{
		    mode = SEEK_CUR;
		}
		else
		{
		    mode = SEEK_END;
		}
		
		if (lseek(fh->fd, iofs->io_Union.io_SEEK.io_Offset, mode) < 0)
		{
		    error = err_u2a();
		}

		iofs->io_Union.io_SEEK.io_Offset = oldpos;
	    }
	    else
	    {
		error = ERROR_OBJECT_WRONG_TYPE;
	    }

	    break;
	}
	
    case FSA_SET_FILE_SIZE:
#warning FIXME: Implement FSA_SET_FILE_SIZE
	/* We could manually change the size, but this is currently not
	   implemented. FIXME */
    case FSA_WAIT_CHAR:
#warning FIXME: Implement FSA_WAIT_CHAR
	/* We could manually wait for a character to arrive, but this is
	   currently not implemented. FIXME */
    case FSA_FILE_MODE:
#warning FIXME: Implement FSA_FILE_MODE
	error=ERROR_ACTION_NOT_KNOWN;
	break;

    case FSA_IS_INTERACTIVE:
	{
	    struct filehandle *fh = (struct filehandle *)iofs->IOFS.io_Unit;

	    if (fh->type == FHD_FILE)
	    {
		iofs->io_Union.io_IS_INTERACTIVE.io_IsInteractive = isatty(fh->fd);
	    }
	    else
	    {
		iofs->io_Union.io_IS_INTERACTIVE.io_IsInteractive = 0;
	    }
	    
	    break;
	}
	
    case FSA_SAME_LOCK:
	{
	    struct filehandle *lock1 = iofs->io_Union.io_SAME_LOCK.io_Lock[0],
			      *lock2 = iofs->io_Union.io_SAME_LOCK.io_Lock[1];

	    if ((lock1->volume != lock2->volume) || strcmp(lock1->name, lock2->name))
	    {
		iofs->io_Union.io_SAME_LOCK.io_Same = LOCK_DIFFERENT;
	    }
	    else
	    {
		iofs->io_Union.io_SAME_LOCK.io_Same = LOCK_SAME;
	    }

	    break;
	}

    case FSA_EXAMINE:
	error = examine(emulbase,
			(struct filehandle *)iofs->IOFS.io_Unit,
			iofs->io_Union.io_EXAMINE.io_ead,
			iofs->io_Union.io_EXAMINE.io_Size,
			iofs->io_Union.io_EXAMINE.io_Mode,
			&(iofs->io_DirPos));
	break;
	
    case FSA_EXAMINE_NEXT:
	error = examine_next(emulbase,
			     (struct filehandle *)iofs->IOFS.io_Unit,
			     iofs->io_Union.io_EXAMINE_NEXT.io_fib);
	break;
	
    case FSA_EXAMINE_ALL:
	error = examine_all(emulbase,
			    (struct filehandle *)iofs->IOFS.io_Unit,
			    iofs->io_Union.io_EXAMINE_ALL.io_ead,
			    iofs->io_Union.io_EXAMINE_ALL.io_eac,
			    iofs->io_Union.io_EXAMINE_ALL.io_Size,
			    iofs->io_Union.io_EXAMINE_ALL.io_Mode);
	break;
	
    case FSA_EXAMINE_ALL_END:
	error = 0;
	break;
	
    case FSA_OPEN_FILE:
	error = open_file(emulbase,
			  (struct filehandle **)&iofs->IOFS.io_Unit,
			  iofs->io_Union.io_OPEN_FILE.io_Filename,
			  iofs->io_Union.io_OPEN_FILE.io_FileMode,
			  iofs->io_Union.io_OPEN_FILE.io_Protection);
	if (
			(error == ERROR_WRITE_PROTECTED) &&
			(iofs->io_Union.io_OPEN_FILE.io_FileMode & FMF_AMIGADOS)
		)
	{
		error = open_file(emulbase,
			 (struct filehandle **)&iofs->IOFS.io_Unit,
			 iofs->io_Union.io_OPEN_FILE.io_Filename,
			 iofs->io_Union.io_OPEN_FILE.io_FileMode & (~FMF_WRITE),
			 iofs->io_Union.io_OPEN_FILE.io_Protection);
	}
	break;
	
    case FSA_CREATE_DIR:
	error = create_dir(emulbase,
			   (struct filehandle **)&iofs->IOFS.io_Unit,
			   iofs->io_Union.io_CREATE_DIR.io_Filename,
			   iofs->io_Union.io_CREATE_DIR.io_Protection);
	break;
	
    case FSA_CREATE_HARDLINK:
	error = create_hardlink(emulbase,
				(struct filehandle **)&iofs->IOFS.io_Unit,
				iofs->io_Union.io_CREATE_HARDLINK.io_Filename,
				(struct filehandle *)iofs->io_Union.io_CREATE_HARDLINK.io_OldFile);
	    break;
	    
    case FSA_CREATE_SOFTLINK:
	error = create_softlink(emulbase,
				(struct filehandle **)&iofs->IOFS.io_Unit,
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
	error = read_softlink(emulbase,
			      (struct filehandle *)iofs->IOFS.io_Unit,
			      iofs->io_Union.io_READ_SOFTLINK.io_Buffer,
			      iofs->io_Union.io_READ_SOFTLINK.io_Size);
	break;
	
    case FSA_DELETE_OBJECT:
	error = delete_object(emulbase,
			      (struct filehandle *)iofs->IOFS.io_Unit,
			      iofs->io_Union.io_DELETE_OBJECT.io_Filename);
	break;
	
    case FSA_PARENT_DIR:
	/* error will always be 0 */
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
	{
	    struct InfoData *id = iofs->io_Union.io_INFO.io_Info;
	    struct statfs    buf;
	    
	    statfs(".", &buf);
	    
	    id->id_NumSoftErrors = 0;
	    id->id_UnitNumber = 0;
	    id->id_DiskState = ID_VALIDATED;
	    id->id_NumBlocks = buf.f_blocks;
	    id->id_NumBlocksUsed = buf.f_blocks - buf.f_bavail;
	    id->id_BytesPerBlock = buf.f_bsize;
	    id->id_DiskType = ID_DOS_DISK; /* Well, not really true... */
	    id->id_VolumeNode = NULL;
	    id->id_InUse = TRUE;
	    
	    break;
	}
	
    case FSA_SET_COMMENT:
    case FSA_SET_PROTECT:
    case FSA_SET_OWNER:
    case FSA_SET_DATE:
    case FSA_MORE_CACHE:
    case FSA_FORMAT:
    case FSA_MOUNT_MODE:
#warning FIXME: not supported yet
	
    default:
	error = ERROR_ACTION_NOT_KNOWN;
	break;
    }
    
    /*Enable();*/
    ReleaseSemaphore(&emulbase->sem);
    
    /* Set error code */
    iofs->io_DosError = error;
    
    /* If the quick bit is not set send the message to the port */
    if(!(iofs->IOFS.io_Flags & IOF_QUICK))
    {
	ReplyMsg(&iofs->IOFS.io_Message);
    }

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

/*********************************************************************************************/
