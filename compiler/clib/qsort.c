/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$

    ANSI C function qsort().
*/
/* Original source from NetBSD */
#include <stdlib.h>

static inline const char *med3 (const char *, const char *, const char *, int (*)());
static inline void	 swapfunc (char *, char *, int, int);

#define min(a, b)       (a) < (b) ? a : b

/*
 * Qsort routine from Bentley & McIlroy's "Engineering a Sort Function".
 */
#define swapcode(TYPE, parmi, parmj, n) {               \
	long i = (n) / sizeof (TYPE);                   \
	register TYPE *pi = (TYPE *) (parmi);           \
	register TYPE *pj = (TYPE *) (parmj);           \
	do {						\
		register TYPE	t = *pi;		\
		*pi++ = *pj;				\
		*pj++ = t;				\
	} while (--i > 0);                              \
}

#define SWAPINIT(a, es) swaptype = ((char *)a - (char *)0) % sizeof(long) || \
	es % sizeof(long) ? 2 : es == sizeof(long)? 0 : 1;

static inline void
swapfunc (char *a, char *b, int n, int swaptype)
{
	if(swaptype <= 1)
		swapcode(long, a, b, n)
	else
		swapcode(char, a, b, n)
}

#define swap(a, b)                                      \
	if (swaptype == 0) {                            \
		long t = *(long *)(a);                  \
		*(long *)(a) = *(long *)(b);            \
		*(long *)(b) = t;                       \
	} else						\
		swapfunc(a, b, es, swaptype)

#define vecswap(a, b, n)        if ((n) > 0) swapfunc(a, b, n, swaptype)

static inline const char *
med3(const char *a, const char *b, const char *c, int (*cmp)(const void *, const void *))
{
	return cmp(a, b) < 0 ?
	       (cmp(b, c) < 0 ? b : (cmp(a, c) < 0 ? c : a ))
	      :(cmp(b, c) > 0 ? b : (cmp(a, c) < 0 ? a : c ));
}

/*****************************************************************************

    NAME */

	void qsort (

/*  SYNOPSIS */
	void * a,
	size_t n,
	size_t es,
	int (* cmp)(const void *, const void *))

/*  FUNCTION
	Sort the array a. It contains n elements of the size es. Elements
	are compares using the function cmp().

    INPUTS
	a - The array to sort
	n - The number of elements in the array
	es - The size of a single element in the array
	cmp - The function which is called when two elements must be
		compared. The function gets the addresses of two elements
		of the array and must return 0 is both are equal, < 0 if
		the first element is less than the second and > 0 otherwise.

    RESULT
	None.

    NOTES

    EXAMPLE
	// Use this function to compare to stringpointers
	int cmp_strptr (const char ** sptr1, const char ** sptr2)
	{
	    return strcmp (*sptr1, *sptr2);
	}

	// Sort an array of strings
	char ** strings;

	// fill the array
	strings = malloc (sizeof (char *)*4);
	strings[0] = strdup ("h");
	strings[1] = strdup ("a");
	strings[2] = strdup ("f");
	strings[3] = strdup ("d");

	// Sort it
	qsort (strings, sizeof (char *), 4, (void *)cmp_strptr);

    BUGS

    SEE ALSO
	strcmp(), strncmp(), memcmp(), strcasecmp(), strncasecmp()

    INTERNALS

******************************************************************************/
{
	char *pa, *pb, *pc, *pd, *pl, *pm, *pn;
	int d, r, swaptype, swap_cnt;

	
loop:	SWAPINIT(a, es);
	swap_cnt = 0;
	if (n < 7) {
		for (pm = a + es; pm < (char *) a + n * es; pm += es)
			for (pl = pm; pl > (char *) a && cmp(pl - es, pl) > 0;
			     pl -= es)
				swap(pl, pl - es);
		return;
	}
	pm = a + (n / 2) * es;
	if (n > 7) {
		pl = a;
		pn = a + (n - 1) * es;
		if (n > 40) {
			d = (n / 8) * es;
			pl = (char *)med3(pl, pl + d, pl + 2 * d, cmp);
			pm = (char *)med3(pm - d, pm, pm + d, cmp);
			pn = (char *)med3(pn - 2 * d, pn - d, pn, cmp);
		}
		pm = (char *)med3(pl, pm, pn, cmp);
	}
	swap(a, pm);
	pa = pb = a + es;

	pc = pd = a + (n - 1) * es;
	for (;;) {
		while (pb <= pc && (r = cmp(pb, a)) <= 0) {
			if (r == 0) {
				swap_cnt = 1;
				swap(pa, pb);
				pa += es;
			}
			pb += es;
		}
		while (pb <= pc && (r = cmp(pc, a)) >= 0) {
			if (r == 0) {
				swap_cnt = 1;
				swap(pc, pd);
				pd -= es;
			}
			pc -= es;
		}
		if (pb > pc)
			break;
		swap(pb, pc);
		swap_cnt = 1;
		pb += es;
		pc -= es;
	}
	if (swap_cnt == 0) {  /* Switch to insertion sort */
		for (pm = a + es; pm < (char *) a + n * es; pm += es)
			for (pl = pm; pl > (char *) a && cmp(pl - es, pl) > 0;
			     pl -= es)
				swap(pl, pl - es);
		return;
	}

	pn = a + n * es;
	r = min(pa - (char *)a, pb - pa);
	vecswap(a, pb - r, r);
	r = min(pd - pc, pn - pd - es);
	vecswap(pb, pn - r, r);
	if ((r = pb - pa) > es)
		qsort(a, r / es, es, cmp);
	if ((r = pd - pc) > es) {
		/* Iterate rather than recurse to save stack space */
		a = pn - r;
		n = r / es;
		goto loop;
	}
/*		qsort(pn - r, r / es, es, cmp);*/
}
