/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/debug.h>
#include <dos/dos.h>

#include <string.h>

/*********************************************************************************************/

/*
 * Create a plain path out of the supplied filename.
 * Eg 'path1/path2//path3/' becomes 'path1/path3'.
 */
BOOL shrink(char *filename)
{
    char *s, *s1, *s2;
    unsigned long len;

    /* We skip the first slash because it separates volume root prefix and the actual pathname */
    s = filename;
    if (*s == '/')
	s++;

    for(;;)
    {
	/* leading slashes? --> return FALSE. */
	if (*s == '/')
	    return FALSE;

	/* remove superflous paths (ie paths that are followed by '//') */
	s1 = strstr(s, "//");
	if (s1 == NULL)
	    break;
	s2 = s1;
	while (s2 > filename)
	{
	    if (s2[-1] == '/')
		break;
	    s2--;
	}

	memmove(s2, s1+2, strlen(s1+1));
    }

    /* strip trailing slash */
    len=strlen(filename);
    if (len && filename[len-1]=='/')
	filename[len-1]=0;

   return TRUE;
}

/* Validate file name */
ULONG validate(char *filename)
{
    char *s = filename;

    while (*s)
    {
	if (*s == '.') {
	    do {
		s++;
	    } while (*s == '.');
	    if ((*s == '/') || (!*s)) {
	        D(bug("[emul] Bad file name, contains dots-only component\n"));
		return ERROR_INVALID_COMPONENT_NAME;
	    }
	}
	do {
	    s++;
	} while ((*s != '/') && *s);
	while (*s == '/')
	    s++;
    }
    
    return 0;
}

/* Append file name to an existing path */
char *append(char *c, char *filename)
{
    char *s;

    *c++ = '/';
    for (s = filename; *s; s++)
	*c++ = *s;
    return c;
}

/* Find start position of the file name in path string */
long startpos(char *name, long i)
{
    /* look for the first '/' in the filename starting at the end */
    while (i != 0 && name[i] != '/')
	i--;

    return i;
}

/* Copy file name with possible conversion */
void copyname(char *result, char *name, long i)
{
    strncpy(result, name, i);
}

/* Go to next part in path string */
char *nextpart(char *sp)
{
    char *sp_end;

    for(sp_end = sp + 1; sp_end[0] != '\0' && sp_end[0] != '/'; sp_end++);

    return sp_end;
}
