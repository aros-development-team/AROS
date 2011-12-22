/*
    Copyright ? 2010-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Declarations of filesystem control routines
    Lang: english
*/

int SetRootDirectory(void);

/*
 * Windows CE has no notion of "current directory".  We have to emulate it,
 * so here comes this wrapper.
 */
#ifdef UNDER_CE
FILE *file_open (const char *filename, const char *mode);
#else
#define file_open fopen
#endif
