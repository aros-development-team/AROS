#ifndef HIDD_UNIXIO_H
#define HIDD_UNIXIO_H

/*
    Copyright (C) 1997 AROS - The Amiga Replacement OS
    $Id$

    Desc: Unix filedescriptor/socket IO Include File
    Lang: english
*/

#ifndef HIDD_HIDD_H
#include <hidd/hidd.h>
#endif

#define UNIXIOCLASS		"unixioclass"

#define HIDDM_UnixIO_Wait   (HIDDM_UncommonMethodBase + 1)  /* LONG M ( uioMsg *)		*/
#define HIDDM_UnixIO_Select (HIDDM_UncommonMethodBase + 2)  /* for async IO, unused		*/

/* obsolete */
#define HIDDM_WaitForIO     HIDDM_UnixIO_Wait

struct uioMsg
{
    STACKULONG um_MethodID;
    STACKULONG um_Filedesc;
    STACKULONG um_Mode;
};

/* UnixIO HIDD Values */
#define HIDDV_UnixIO_Read       0x1
#define HIDDV_UnixIO_Write      0x2

#endif /* HIDD_UNIXIO_H */
