#ifndef HIDD_DISPLAYHW_H
#define HIDD_DISPLAYHW_H

/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Display hardwired.
    Lang: English.
*/

/* I have to move away this defines - should be in separate file */

/* This structure keeps contents of mode registers including palette */

struct DisplayHWRec
{
  char NoClock;                 /* number of selected clock */
};

#endif /* HIDD_DISPLAYHW_H */
