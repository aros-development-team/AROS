/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$

    POSIX function readdir().
*/

#include "__arosc_privdata.h"

#include <dos/dos.h>
#include <proto/dos.h>

#include <string.h>
#include <errno.h>

#include "__errno.h"
#include "__open.h"
#include "__upath.h"

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
	telldir(), scandir()

    INTERNALS

******************************************************************************/
{
    int const max = MAXFILENAMELENGTH > NAME_MAX ? NAME_MAX : MAXFILENAMELENGTH;
    fdesc *desc;

    if (!dir)
    {
        errno = EFAULT;
	return NULL;
    }

    desc = __getfdesc(dir->fd);
    if (!desc)
    {
    	errno = EBADF;
    	return NULL;
    }

    if (__doupath && dir->pos == 0)
    {
        dir->ent.d_name[0]='.';
        dir->ent.d_name[1]='\0';
    } 
    else
    if (__doupath && dir->pos == 1)
    {
        dir->ent.d_name[0]='.';
    	dir->ent.d_name[1]='.';
    	dir->ent.d_name[2]='\0';
    }
    else
    while (TRUE)
    {
        if (!ExNext(desc->fh, dir->priv))
        {
	    dir->pos--;
	    if (IoErr() != ERROR_NO_MORE_ENTRIES)
    	        errno = IoErr2errno(IoErr());

    	    return NULL;
        }
        else
        {
            CONST_STRPTR name = ((struct FileInfoBlock *)dir->priv)->fib_FileName;
            
            if (__doupath && name[0] == '.')
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
            
            break;
        }
    }
    
    dir->pos++;
    return &(dir->ent);
}
