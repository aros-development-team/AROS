/*
    Copyright © 1995-2004, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <stdio.h>
#include <stddef.h>
#include <exec/types.h>
#include <aros/config.h>

struct __aros_longalign
{
    char dummy;
    LONG offset;
};

struct __aros_wordalign
{
    char dummy;
    WORD offset;
};

struct __aros_ptralign
{
    char dummy;
    APTR offset;
};

struct __aros_iptralign
{
    char dummy;
    IPTR offset;
};

struct __aros_doublealign
{
    char dummy;
    double offset;
};

long sub (void)
{
    char b; /* This MUST NOT BE static ! */
    long adr = (long)&b;

    return adr;
}

int main (void)
{
    char a;
    long adr1, adr2;
    long val;
    char * first_byte;
    int wordalign;
    int longalign;
    int ptralign;
    int iptralign;
    int doublealign;
    int worstalign;

    /* Calculate the addresses of two *local* variables */
    adr1 = (long)&a;
    adr2 = sub();

    /* If this is a normal stack, memory looks like this:

	    adr2 (b)
	    ...
	    adr1 (a)

       because adr1 (a) is pushed on the stack and the stackpointer
       is decreased (*--sp=a). Otherwise it looks like:

	    adr1 (a)
	    ...
	    adr2 (b)
    */

    /*
	The difference between big and little endian is this:

	Big endian stores the value 0x11223344 as { 0x11, 0x22, 0x33, 0x44 }
	(ie. the first byte in memory is the most significant byte of the
	variable).

	Little endian stores the value 0x11223344 as { 0x44, 0x33, 0x22, 0x11 }
	(ie. the first byte in memory is the least significant byte of the
	variable).
    */

    val = 0x11223344;

    first_byte = (char *)&val; /* Check if the first byte is 0x11 */

    wordalign	= offsetof(struct __aros_wordalign,    offset);
    longalign	= offsetof(struct __aros_longalign,    offset);
    ptralign	= offsetof(struct __aros_ptralign,     offset);
    iptralign	= offsetof(struct __aros_iptralign,    offset);
    doublealign = offsetof(struct __aros_doublealign,  offset);

    worstalign = wordalign;
    if (worstalign < longalign)   worstalign = longalign;
    if (worstalign < ptralign)    worstalign = ptralign;
    if (worstalign < iptralign)   worstalign = iptralign;
    if (worstalign < doublealign) worstalign = doublealign;

#if (AROS_FLAVOUR & AROS_FLAVOUR_BINCOMPAT)
    if (worstalign < 8) worstalign = 8;
#endif

    printf ("#define AROS_STACK_GROWS_DOWNWARDS %d /* Stack direction */\n", (adr2 < adr1));
    printf ("#define AROS_BIG_ENDIAN            %d /* Big or little endian */\n", (*first_byte == 0x11));
    printf ("#define AROS_SIZEOFULONG           %ld /* Size of an ULONG */\n", (unsigned long)sizeof (ULONG));
    printf ("#define AROS_WORDALIGN             %d /* Alignment for WORD */\n",   wordalign);
    printf ("#define AROS_LONGALIGN             %d /* Alignment for LONG */\n",   longalign);
    printf ("#define AROS_PTRALIGN              %d /* Alignment for PTR */\n",    ptralign);
    printf ("#define AROS_IPTRALIGN             %d /* Alignment for IPTR */\n",   iptralign);
    printf ("#define AROS_DOUBLEALIGN           %d /* Alignment for double */\n", doublealign);
    printf ("#define AROS_WORSTALIGN            %d /* Worst case alignment */\n", worstalign);

    return 0;
}

