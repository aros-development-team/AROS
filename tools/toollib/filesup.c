/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Functions to do interesting things with files.
*/

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>

#include <toollib/toollib.h>
#include <toollib/filesup.h>

int filesdiffer( char *file1, char *file2 )
{
    FILE *fd1, *fd2;
    int cnt1, cnt2;

    char buffer1[1], buffer2[1];
    int retval = 0;
    
    /* Do our files exist? */
    fd1 = fopen(file1,"rb");
    if(!fd1)
	return -1;
    fd2 = fopen(file2, "rb");
    if(!fd2)
	return -2;

    /* Read and compare, character by character */
    do
    {
	cnt1 = fread(buffer1, 1, 1, fd1);
	cnt2 = fread(buffer2, 1, 1, fd2);
    } while( cnt1 && cnt2 && buffer1[0] == buffer2[0] );

    /*
	If the sizes are different, then one of the files must have hit
	EOF, so they differ; otherwise it depends upon the data in the
	file.
    */
    if(buffer1[0] != buffer2[0] || cnt1 != cnt2)
	retval = 1;

    fclose(fd1);
    fclose(fd2);
    return retval;
}

int moveifchanged(char *old, char *new)
{
    struct stat *statold, *statnew;
    char *bakname;

    statold = calloc(1, sizeof(struct stat));
    statnew = calloc(1, sizeof(struct stat));

    if(stat(old, statold))
    {
	/* Couldn't stat old file, assume non-existant */
	return rename(new, old);
    }

    if(stat(new, statnew))
    {
	/* Couldn't stat() new file, this shouldn't happen */
	fprintf(stderr, "Couldn't stat() file %s\n", new);
	exit(-1);
    }

    /* Fill an array with the old.bak filename */
    bakname = malloc( (strlen(old) + 5) * sizeof(char) );
    sprintf( bakname, "%s.bak", old );

    if(statold->st_size != statnew->st_size)
    {
	rename(old, bakname);
	rename(new, old);
    }
    else if( filesdiffer(old, new) )
    {
	rename(old, bakname);
	rename(new, old);
    }
    else
	remove(new);

    free(bakname);
    return 0;
}

int copy(char *src, char *dest)
{
    FILE *in, *out;
    int count;
    char buffer[1024];

    in = fopen(src, "rb");
    if(!in)
	return -1;

    out = fopen(dest, "w");
    if(!out)
	return -1;

    do
    {
	count = fread(buffer, 1, 1024, in);
	fwrite(buffer, 1, count, out);
    } while( count == 1024 );

    fclose(in);
    fclose(out);

    return 0;
}
