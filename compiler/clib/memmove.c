/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: ANSI C function memmove()
    Lang: english
*/

/*****************************************************************************

    NAME */
#include <string.h>

	void * memmove (

/*  SYNOPSIS */
	void *	     dest,
	const void * src,
	size_t	     count)

/*  FUNCTION
	Copy the contents of a part of memory to another. Both areas
	may overlap.

    INPUTS
	dest - The first byte of the destination area in memory
	src - The first byte of the source area in memory
	count - How many bytes to copy

    RESULT
	dest.

    NOTES

    EXAMPLE
	See below.

    BUGS

    SEE ALSO
	memcpy()

    INTERNALS

    HISTORY
	24-12-95    digulla created

******************************************************************************/
{
    char * d = (char *)dest,
	 * s = (char *)src;
    size_t mis,
	   low,
	   high;


    if (s < d)
    {
	d += count;
	s += count;

#if 0 /* stegerg: I think this is the wrong way round (and even then buggy) */
	mis = sizeof (long) - ((long)s & (sizeof (long) - 1));
#else
	mis = (long)s & (sizeof(long) - 1);
#endif
	if (mis > count)
	    mis = count;

	count -= mis;

	if (mis)
	{
	    while (mis--)
		*--d = *--s;
	}

	if (!((long)d & (sizeof (long) - 1)) )
	{
	    long * dl = (long *)d;
	    long * sl = (long *)s;
	    size_t longs;

	    longs = count / sizeof (long);

	    low  = longs & 7;
	    high = longs >> 3;

	    while (low--)
		*--dl = *--sl;

	    while (high--)
	    {
		*--dl = *--sl;
		*--dl = *--sl;
		*--dl = *--sl;
		*--dl = *--sl;

		*--dl = *--sl;
		*--dl = *--sl;
		*--dl = *--sl;
		*--dl = *--sl;
	    }

	    #if 0 /* stegerg: this is slower than */
	    count -= longs * sizeof (long);
	    #else /* this, which should also work (CopyMem does the same) */
	    count &= (sizeof(long) - 1);
	    #endif
	    
	    /* stegerg: the following 2 lines were missing */
	    d = (char *)dl;
	    s = (char *)sl;
	    
	}

	low  = count & 7;
	high = count >> 3;

	while (low--)
	    *--d = *--s;

	while (high--)
	{
	    *--d = *--s;
	    *--d = *--s;
	    *--d = *--s;
	    *--d = *--s;

	    *--d = *--s;
	    *--d = *--s;
	    *--d = *--s;
	    *--d = *--s;
	}
    }
    else
    {
#if 0   /* stegerg: I think this is the wrong way round */
	mis = (long)s & (sizeof (long) - 1);
#else
	mis = (sizeof(long) - 1) - (((long)s - 1) & (sizeof(long) - 1));
#endif
	if (mis > count)
	    mis = count;

	count -= mis;

	if (mis)
	{
	    while (mis--)
		*d++ = *s++;
	}

	if (!((long)d & (sizeof (long) - 1)) )
	{
	    long * dl = (long *)d;
	    long * sl = (long *)s;
	    size_t longs;

	    longs = count / sizeof (long);

	    low  = longs & 7;
	    high = longs >> 3;

	    while (low--)
		*dl++ = *sl++;

	    while (high--)
	    {
		*dl++ = *sl++;
		*dl++ = *sl++;
		*dl++ = *sl++;
		*dl++ = *sl++;

		*dl++ = *sl++;
		*dl++ = *sl++;
		*dl++ = *sl++;
		*dl++ = *sl++;
	    }

	    #if 0 /* stegerg: this is slower than */
	    count -= longs * sizeof (long);
	    #else /* this, which should also work (CopyMem does the same) */
	    count &= (sizeof(long) - 1);
	    #endif
	    
	    /* stegerg: the following 2 lines were missing */
	    d = (char *)dl;
	    s = (char *)sl;
	}

	low  = count & 7;
	high = count >> 3;

	while (low--)
	    *d++ = *s++;

	while (high--)
	{
	    *d++ = *s++;
	    *d++ = *s++;
	    *d++ = *s++;
	    *d++ = *s++;

	    *d++ = *s++;
	    *d++ = *s++;
	    *d++ = *s++;
	    *d++ = *s++;
	}
    }

    return dest;
} /* memmove */

#ifdef TEST
#include <stdio.h>

unsigned char src[64];
unsigned char dst[64+8];

void showresult (void)
{
    int t;

    printf ("    %02x%02x%02x%02x,\n", dst[0], dst[1], dst[2], dst[2]);

    for (t=0; t<64; t++)
    {
	if ((t&15)==0)
	    printf ("    ");

	printf ("%02lx", dst[t+4]);

	if ((t&15)==15)
	    printf ("\n");
	else if ((t&3)==3)
	    printf (" ");
    }

    printf ("    %02x%02x%02x%02x\n", dst[68], dst[69], dst[70], dst[71]);
}

int main (int argc, char ** argv)
{
    char * s = src;
    char * d = &dst[4];
    int t;

    for (t=0; t<64; t++)
	src[t] = t+1;

    printf ("Initial state:\n");
    showresult ();

    printf ("Full copy:\n");
    memmove (d, s, 64);
    showresult ();

    printf ("Shift down:\n");
    memmove (d, s, 64);
    memmove (d, d+1, 63);
    showresult ();

    printf ("Shift up:\n");
    memmove (d, s, 64);
    memmove (d+1, d, 63);
    showresult ();

} /* main */

#endif /* TEST */
