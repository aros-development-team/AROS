/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    $Id$

    POSIX.1-2008 function readdir().
*/

#include "__posixc_intbase.h"

#include <dos/dos.h>
#include <proto/dos.h>

#include <string.h>
#include <errno.h>

#include "__fdesc.h"
#include "__upath.h"
#include "__dirdesc.h"

#define DEBUG 0
#include <aros/debug.h>

/*****************************************************************************

    NAME */
#include <dirent.h>

	struct dirent *readdir(

/*  SYNOPSIS */
	DIR *dir)

/*  FUNCTION
	 Reads a directory

    INPUTS
	dir - the directory stream pointing to the directory being read

    RESULT
	The  readdir()  function  returns  a  pointer  to a dirent
        structure, or NULL if an error occurs  or  end-of-file  is
        reached.

	The data returned by readdir() is  overwritten  by  subse­
        quent calls to readdir() for the same directory stream.

	According  to POSIX, the dirent structure contains a field
        char d_name[] of unspecified size, with at  most  NAME_MAX
        characters  preceding the terminating null character.  Use
        of other fields will harm the  portability  of  your  pro­
        grams.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
 	read(), opendir(), closedir(), rewinddir(), seekdir(),
	telldir()

    INTERNALS

******************************************************************************/
{
    struct PosixCIntBase *PosixCBase =
        (struct PosixCIntBase *)__aros_getbase_PosixCBase();
    int const max = MAXFILENAMELENGTH > NAME_MAX ? NAME_MAX : MAXFILENAMELENGTH;
    fdesc *desc;

    D(bug("readdir("));

    if (!dir)
    {
        D(bug("null)=EFAULT\n"));
        errno = EFAULT;
	return NULL;
    }

    desc = __getfdesc(dir->fd);
    if (!desc)
    {
        D(bug("fd=%d)=EBADF\n", (int)dir->fd));
    	errno = EBADF;
    	return NULL;
    }

    if (PosixCBase->doupath && dir->pos == 0)
    {
        dir->ent.d_type = DT_DIR;
        dir->ent.d_name[0]='.';
        dir->ent.d_name[1]='\0';
        dir->ent.d_reclen = 1;
    } 
    else
    if (PosixCBase->doupath && dir->pos == 1)
    {
        dir->ent.d_type = DT_DIR;
        dir->ent.d_name[0]='.';
    	dir->ent.d_name[1]='.';
    	dir->ent.d_name[2]='\0';
        dir->ent.d_reclen = 2;
    }
    else
    {
        struct FileInfoBlock *fib = (struct FileInfoBlock *)dir->priv;

        if (!ExNext(desc->fcb->handle, fib))
        {
	    dir->pos--;
	    if (IoErr() != ERROR_NO_MORE_ENTRIES)
	    {
                errno = __stdc_ioerr2errno(IoErr());
		D(bug(") errno=%d\n", (int)errno));
            }
	    D(else
		bug("NO_MORE_ENTRIES)\n"));
    	    return NULL;
        }

        CONST_STRPTR name = fib->fib_FileName;
        while (TRUE)
        {

            if (PosixCBase->doupath && name[0] == '.')
            { 
                if (name[1] == '.')
                {      
                    if (name[2] == '\0')
                        continue;
                }
                else
                if (name[1] == '\0')
                    continue;
            }
          
            strncpy(dir->ent.d_name, name, max);
            dir->ent.d_reclen = strlen(name);

            switch (fib->fib_DirEntryType)
            {
            case ST_FILE:
              dir->ent.d_type = DT_REG; 
              break;
            case ST_ROOT:
            case ST_USERDIR:
              dir->ent.d_type = DT_DIR; 
              break;
            case ST_SOFTLINK:
            case ST_LINKFILE:
            case ST_LINKDIR:
              dir->ent.d_type = DT_LNK; 
              break;
            case ST_PIPEFILE:
              dir->ent.d_type = DT_FIFO; 
              break;
            default:
              dir->ent.d_type = DT_UNKNOWN; 
              break;
            }
            
            break;
        }
    }
   
    D(bug("%s) d_type=%d\n", dir->ent.d_name, (int)dir->ent.d_type));
    dir->pos++;
    return &(dir->ent);
}
