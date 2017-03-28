/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <stdio.h>
#include <assert.h>

#include "aros_types.h"

void hexdump (const void * start, int size);

/* Make sure that all variables contain the same values */
#define __check(v1,op,v2) \
    if (debug) \
    { \
	printf ("Check: " #v1 " " #op " " #v2 "\n"); \
	hexdump (&v1, sizeof (v1)); \
	hexdump (&v2, sizeof (v2)); \
    } \
    if (!(v1 op v2) ) \
    { \
	printf ("%s:%d: Check failed: " #v1 " " #op " " #v2 " (%d - %d)\n", \
		__FILE__, __LINE__, (int)v1, (int)v2); \
    }
#define check(op) \
    do { \
	__check (s,op,w) \
	__check (us,op,uw) \
	__check (l,op,L) \
	__check (ul,op,UL) \
    } \
    while (0)

#define unoptest(op) \
    s op; us op; l op; ul op; \
    w op; uw op; L op; UL op; \
    check (==)

#define binoptest(op) \
    s = s op; us = us op; l = l op; ul = ul op; \
    w = w op; uw = uw op; L = L op; UL = UL op; \
    check (==)

#define cmptest(v1,op,v2) \
    s = v1; us = v1; l = v1; ul = v1; \
    w = v2; uw = v2; L = v2; UL = v2; \
    check (op)
    
int main (int argc, char ** argv)
{
    short	   s;
    unsigned short us;
    long	   l;
    unsigned long  ul;
    WORD	   w;
    UWORD	   uw;
    LONG	   L;
    ULONG	   UL;

    int debug = (argc != 1);

    int test = 0x123456;
    hexdump (&test, sizeof (test));
    
    printf ("Check assignments\n");
    unoptest (= 0);
    unoptest (= 0x1234);
    unoptest (= 0x12345678);
    unoptest (= 0x92345678);
    w = s; uw = us; L = l; UL = ul; check (==);
    s = w; us = uw; l = L; ul = UL; check (==);

    printf ("Check + operator\n");
    binoptest (+ 1);
    binoptest (+ 0x1234);
    binoptest (+ 0x9234);
    binoptest (+ 0x12345678);
    binoptest (+ 0x92345678);

    printf ("Check - operator\n");
    binoptest (- 1);
    binoptest (- 0x1234);
    binoptest (- 0x9234);
    binoptest (- 0x12345678);
    binoptest (- 0x92345678);

    printf ("Check * operator\n");
    binoptest (* 2);
    binoptest (* 0x1234);
    binoptest (* 0x9234);
    binoptest (* 0x12345678);
    binoptest (* 0x92345678);

    printf ("Check / operator\n");
    binoptest (/ 2);
    binoptest (/ 0x1234);
    binoptest (/ 0x9234);
    binoptest (/ 0x12345678);
    binoptest (/ 0x92345678);

    printf ("Check %% operator\n");
    binoptest (% 2);
    binoptest (% 0x1234);
    binoptest (% 0x9234);
    binoptest (% 0x12345678);
    binoptest (% 0x92345678);

    printf ("Check >> operator\n");
    unoptest (= 0x12345678);
    binoptest (>> 2);

    printf ("Check << operator\n");
    unoptest (= 0x12345678);
    binoptest (<< 2);

    printf ("Check < comparison\n");
    cmptest (0x12345678, <, 0x12345679);
    cmptest (0x12345678, <=, 0x12345679);
    cmptest (0x12345678, <=, 0x12345678);
    
    printf ("Check > comparison\n");
    cmptest (0x12345679, >, 0x12345678);
    cmptest (0x12345679, >=, 0x12345678);
    cmptest (0x12345678, >=, 0x12345678);
    
    printf ("All tests succeeded\n");
    exit (0);
}

/* Print the contents of a piece of memory. */
void hexdump (const void * start, int size)
{
    int t;
    const unsigned char * ptr = (const unsigned char *)start;

    for (t=0; size > 0; t++, size--)
    {
        if (!(t & 15)) printf ("%08lx: ", ((long)ptr));
        printf ("%02x", *ptr++);
        if ((t & 3) == 3) putchar (' ');
        if ((t & 15) == 15) putchar ('\n');
    }

    if (t & 15) putchar ('\n');
}

