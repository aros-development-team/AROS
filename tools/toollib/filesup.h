#ifndef TOOLLIB_FILESUP_H
#define TOOLLIB_FILESUP_H

/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Some nifty functions to help with files.
*/

#ifndef TOOLLIB_TOOLLIB_H
#include <toollib/toollib.h>
#endif

/*
    Compares two files
    returns:	0 for equal files,
		1 for different files,
		-1 for file 1 not present,
		-2 for file 2 not present.
*/
extern int filesdiffer PARAMS(( char * file1, char * file2 ));

/*
    Compares old and new file. If the old file is not found, or
    it is different from the new file, the new file will be
    renamed to the name of the old file, and the old files will
    be renamed to old.bak

    Returns 0 on success
*/
extern int moveifchanged PARAMS(( char *old, char *new ));

/*
    Copy file src to file dest, returns 0 on success
*/
extern int copy PARAMS(( char *src, char *dest ));

#endif /* TOOLLIB_FILESUP_H */
