/*
    Copyright © 2010-2013, The AROS Development Team. All rights reserved. 
    $Id$

    C99 function tmpfile().
    This function is based on the public domain libnix code
*/
#include "__stdio.h"

/*****************************************************************************

    NAME */
#include <stdio.h>

        FILE * tmpfile(

/*  SYNOPSIS */
	void)

/*  FUNCTION 
        The tmpfile() function creates a temporary file that is different from
        any other existing file and that will automatically be removed when
        it is closed or at program termination. The file is opened for update
        with "wb+" mode.

    INPUTS
  
        
    RESULT
        Pointer to the stream that was created. On error NULL is returned.

    NOTES
        This function uses tmpnam(NULL) to get the file name.

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

    SEE ALSO
        fopen(), tmpnam()

    INTERNALS

******************************************************************************/
{
    char *s;
    FILE *f = NULL;

    do {
        s = tmpnam(NULL);

        if (s == NULL)
            return NULL;

        f = fopen(s, "wb+");
    } while (f == NULL);

    f->flags |= __STDCIO_STDIO_TMP;

    return f;
} /* tmpfile() */
