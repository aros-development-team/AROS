#ifndef LIBRARIES_EXPANSION_H
#define LIBRARIES_EXPANSION_H

/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: expansion.library external definitions
    Lang: english
*/

#define EXPANSIONNAME   "expansion.library"

/* Flags for the AddDosNode() call */

/* Start FileSystem process. */
#define ADNB_STARTPROC      0
#define ADNF_STARTPROC      (1L<< ADNB_STARTPROC)

/* This is supposed to tell the boot code that we boot without a ConfigDev */
#define ADNB_NOCONFIGDEV    8
#define ADNF_NOCONFIGDEV    (1L<< ADNB_NOCONFIGDEV)

#endif /* LIBRARIES_EXPANSION_H */
