/*
    Copyright 1995-2001 AROS - The Amiga Research OS
    $Id$

    Desc: POSIX function readdir()
    Lang: english
*/

#include <dos/dos.h>
#include <proto/dos.h>

#include <errno.h>

#include "__open.h"

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

    HISTORY
	09.06.2001 falemagn created
******************************************************************************/
{
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

    if (ExNext(desc->fh, dir->priv))
    {
	int max = MAXFILENAMELENGTH > NAME_MAX ? NAME_MAX : MAXFILENAMELENGTH;
	strncpy
	(
	    dir->ent.d_name,
	    ((struct FileInfoBlock *)dir->priv)->fib_FileName,
	    max
        );

        return &(dir->ent);
    }

    if (IoErr() != ERROR_NO_MORE_ENTRIES)
    	errno = IoErr2errno(IoErr());

    return NULL;
}