/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

/* misc.c -- here are all miscellaneous functions for global use */

#include "Installer.h"
#include "cleanup.h"


/*
 * Break string into array of strings at LINEFEEDs
 * **outarr must be obtained via malloc!
 */
int strtostrs(char * in, char ***outarr)
{
int i = 0, j = 0, k = 0;
char **out = *outarr;

    while (*in)
    {
	i++;
	/* allocate space for next string */
	out = ReAllocVec(out, (i+1) * sizeof(char *), MEMF_PUBLIC);
	outofmem(out);
	for ( j = 0 ; in[j] && in[j]!=LINEFEED ; j++ );
	out[i-1] = AllocVec((j+1) * sizeof(char), MEMF_PUBLIC);
	outofmem(out[i-1]);
	for ( k = 0 ; k < j ; k++ )
	{
	    /* save char to string */
	    out[i-1][k] = *in;
	    in++;
	}
	/* NULL-terminate string */
	out[i-1][j] = 0;
	if (*in)
	    in++;
    }
    /* NULL-terminate array */
    out[i] = NULL;
    *outarr = out;

return i;
}

/*
 * Collate array of strings with glueing LINEFEEDs
 */
char *collatestrings(int n, char ** instrs)
{
char *retval;
int len = 0, i, j, k;

    for ( i = 0 ; i < n ; i++ )
    {
	/* Add length of line to total length of text,
	   plus additional LINEFEED (NULL for last item) */
	len += strlen(instrs[i]) + 1;
    }
    retval = AllocVec(sizeof(char) * len, MEMF_PUBLIC);
    if (retval != NULL)
    {
	j = 0;
	for ( i = 0 ; i < n ; i++ )
	{
	    k = 0;
	    for ( len = strlen(instrs[i]) ; len > 0 ; len-- )
	    {
		retval[j] = instrs[i][k];
		j++;
		k++;
	    }
	    retval[j] = LINEFEED;
	    j++;
	}
	retval[j-1] = 0;
    }

return retval;
}

/*
 * Add surrounding quotes to string
 * Creates a copy of input string
 */
char *addquotes(char * string)
{
char *retval;
int c;

    /* Add surrounding quotes */
    c = (string == NULL) ? 0 : strlen(string);
    retval = AllocVec(c+3, MEMF_PUBLIC);
    outofmem(retval);
    retval[0] = DQUOTE;
    strcpy(retval+1, string);
    retval[c+1] = DQUOTE;
    retval[c+2] = 0;

return retval;
}

/*
 * FreeVec() array of strings (eg. allocated by strtostrs())
 */
void freestrlist(STRPTR *array)
{
int i=0;

    while (array[i])
    {
	FreeVec(array[i]);
	i++;
    }
    FreeVec(array);
}

