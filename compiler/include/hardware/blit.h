#ifndef HARDWARE_BLIT_H
#define HARDWARE_BLIT_H

/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Amiga bit blitter
    Lang: english
*/

/* Note: bare support for needed stuff. This file needs to be completed! */

/* blitter minterms */
#define ABC     0x80
#define ABNC    0x40
#define ANBC    0x20
#define ANBNC   0x10
#define NABC    0x08
#define NABNC   0x04
#define NANBC   0x02
#define NANBNC  0x01

/* common minterm operations */
#define A_XOR_C   NABC|ABNC|NANBC|ANBNC

struct bltnode
{
  struct bltnode * n;
  int    (*function) ();
  char   stat;
  short  bltsize;
  short  beamsync;
  int    (*cleanup) ();
};

/* cxref mixes up with the function pointers in the previous struct */
extern int __cxref_bug_blit;

#define CLEANUP 0x40
#define CLEANME 0x40

#endif /* HARDWARE_BLIT_H */
