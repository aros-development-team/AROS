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
        access value `w+'.  The file is created in the T: directory,
        which is the standard AROS temp directory.


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
        BUG1: The temporary file is neither closed nor deleted. Ideally,
        unlink() could be used to mark the temp file for removal (see
        BUG1 in the source code) - but I suspect a bug in unlink() itself,
        whereby it tries to remove the file straight away, rather than
        waiting for all references to it to be closed. The bug is not too
        serious, because all temp files are written to the T: directory,
        which get zapped when AROS is closed down. However, problems may
        exist when you start creating over 26 temp files with the same PID.


    SEE ALSO
        fopen(), mkstemp()

    INTERNALS

******************************************************************************/
{
  #define TEMPLATE "T:temp.XXXXXX"
  char * filename;
  FILE *fp;

  filename = (char *)malloc(MAXPATHLEN);
  if (!filename) { puts("FIXME: mktemp() malloc failed"); return NULL;}
  strcpy(filename, TEMPLATE);

  mktemp(filename);
  fp = fopen(filename, "w+");
  /* unlink(filename); -- see BUG1 in BUGS section */
  free(filename);
  return fp;
 
} /* tmpfile() */
