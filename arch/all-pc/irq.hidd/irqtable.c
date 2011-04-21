/*
    Copyright © 2011, The AROS Development Team. All rights reserved.
    $Id: $

    Desc: IBM PC Motherboard resources table
    Lang: english
*/

/* The constants are in reverse order! */
const char IRQ_Table[15] =
{
    14,	/* Total number of entries in the table */

    12,	/* Mouse	*/
    -1,	/* Ethernet	*/
    15, /* HDD2		*/
    14,	/* HDD1		*/
    13,	/* FPU		*/
    8,	/* RTC		*/
    7,	/* Parallel2	*/
    5,	/* Parallel1	*/
    6,	/* Floppy	*/
    -1,	/* Audio	*/
    3,	/* Serial2	*/
    4,	/* Serial1	*/
    1,	/* Keyboard	*/
    0	/* Timer	*/
};
