/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <stdio.h>
#include "types.h"
#include "lists.h"

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

int main (int argc, char ** argv)
{
    int a;
    short b;
    APTR ptr;
    WORD c;
    struct List l;
    ListPtr lptr;
    struct Node n;

    /* Store an int in a BE 16bit data type */
    c = 15;
    a = c;  // Check if all conversions work
    b = c;

    // Try to print the data
    printf ("Must be 15 15 15: %d %d %d\n", a, b, (int)c);
    putchar ('\n');

    // Same with a pointer
    ptr = &lptr;

    // Note that the pointer must be casted but the compiler will print
    // a warning if the cast is missing:
    //	warning: cannot pass objects of type `APTR' through `...'
    // These three lines must print the same values.
    printf ("APTR %p %p\n", &lptr, (void *)ptr);
    hexdump (&ptr, sizeof (ptr));
    ptr.print (); putchar ('\n');
    putchar ('\n');

    // Same with a pointer
    char * p1;
    STRPTR p2;

    p1 = "hello";
    p2 = p1;

    // Note that the pointer must be casted but the compiler will print
    // a warning if the cast is missing:
    //	warning: cannot pass objects of type `STRPTR' through `...'
    // The first line must print two equal pointers and the second line
    // must print two times "hello".
    printf ("string %p %p\n", p1, (void *)p2);
    printf ("%s %s\n", p1, (const char *)p2);
    putchar ('\n');

    // Show the contents of the memory (to prove that the actual data is BE)
    printf ("Contents of p1 (host endianess) and p2 (big endian):\n");
    hexdump (&p1, sizeof (p1));
    hexdump (&p2, sizeof (p2));
    putchar ('\n');

    // Same with a structure
    lptr = &l;
    // Print address of list header
    printf ("&lptr %p\n", &lptr);
    // Print list pointers (host and BE) which must be equal (otherwise the
    // BE pointer is not converted correctly).
    printf ("List %p %p\n", &l, (void *)lptr);
    // Show that it's really a BE pointer
    hexdump (&lptr, sizeof (lptr));
    // Print the real contents of the variable in host endianess
    lptr.print ();
    putchar ('\n');
    putchar ('\n');

    // Try some functions on the list
    NEWLIST(lptr);
    printf ("NewList %p %p %p\n", (void *)(l.lh_Head), (void *)l.lh_Tail, (void *)l.lh_TailPred);
    printf ("NewList %p %p %p\n", (void *)(lptr->lh_Head), (void *)lptr->lh_Tail, (void *)lptr->lh_TailPred);
    hexdump (&l, sizeof (struct List));
    putchar ('\n');

    ADDHEAD(lptr, &n);
    printf ("&Node %p\n", &n);
    printf ("AddHead %p %p %p\n", (void *)l.lh_Head, (void *)l.lh_Tail, (void *)l.lh_TailPred);
    putchar ('\n');

    return 0;
}
