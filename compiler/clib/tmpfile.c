/*
    This file has been released into the Public Domain.
    $Id:  $

    POSIX function tmpfile().
*/

#include <sys/param.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <proto/dos.h>
#include <unistd.h>

/*****************************************************************************

    NAME */
#include <stdio.h>

        FILE * tmpfile(

/*  SYNOPSIS */
	void)

/*  FUNCTION 
        The tmpfile() function returns a pointer to a stream
        associated with a file descriptor returned by the routine
        mkstemp(3).  The created file is unlinked before tmpfile()
        returns, causing the file to be automatically deleted when the
        last reference to it is closed.  The file is opened with the
        access value `w+'.  The file is created in the directory
        determined by the environment variable TMPDIR if set.  The
        default location if TMPDIR is not set is /tmp.


    INPUTS
  
        
    RESULT
        The tmpfile() function returns a pointer to an open file stream on 
	success. On error, a NULL pointer is returned and errno is set 
	appropriately.

    ERRORS 
        The tmpfile() function may fail and set the global variable
        errno for any of the errors specified for the library functions
        fdopen() or mkstemp().

    NOTES

    EXAMPLE
        #include <errno.h>
	#include <stdio.h>
	#include <string.h>

	main()
	{
	  FILE * fp;

	  fp = tmpfile();
	  if ( fp == NULL)
	  {
	    perror(strerror(errno));
	    return;
	  }

	  fprintf(fp, "do a bit of writing to the temp file");
	}

    BUGS
        The temporary file is neither closed nor deleted.

	Does not respect TMPDIR or /tmp; uses current directory instead.

    SEE ALSO
        fdopen(), mkstemp()

    INTERNALS

******************************************************************************/
{
  #define TEMPLATE "temp.XXXXXX"
  char * filename;

  filename = (char *)malloc(MAXPATHLEN);
  if (!filename) {return NULL;}
  strcpy(filename, TEMPLATE);

  mktemp(filename);
  puts(filename);
  free(filename);
  return fopen(filename, "w+");

  /* FIXME implement properly
     A better way to implement this function would be along the lines of:
     int fd;
     fd  = mkstemp(filename);
     unlink(filename);
     return fdopen(fd, "w+");

     However, it appears that fdopen returns NULL when you try to do this, and 
     I don't understand why. It has nothing to do with unlink(); because even 
     if you comment it out, the problem still occurs.
  */

 
} /* tmpfile() */
