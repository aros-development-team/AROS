/*
    Copyright © 2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: SAM440EP motherboard resources table
    Lang: english
*/

#include <asm/amcc440.h>

/* The constants are in reverse order! */
const char IRQ_Table[15] =
{
    14,		/* Total number of entries in the table */

    -1,		/* Mouse	*/
    -1,		/* Ethernet	*/
    -1, 	/* HDD2		*/
    -1,		/* HDD1		*/
    -1,		/* FPU		*/
    -1,		/* RTC		*/
    -1,		/* Parallel2	*/
    -1,		/* Parallel1	*/
    -1,		/* Floppy	*/
    -1,		/* Audio	*/
    -1,		/* Serial2	*/
    -1,		/* Serial1	*/
    -1,		/* Keyboard	*/
    INTR_GDP	/* Timer	*/
};
