#ifndef HIDD_UNIXIO_H
#define HIDD_UNIXIO_H

/*
    Copyright (C) 1997 AROS - The Amiga Replacement OS
    $Id$

    Desc: Unix filedescriptor/socket IO Include File
    Lang: english
*/

#ifndef HIDD_HIDD_H
#   include <hidd/hidd.h>
#endif
#ifndef EXEC_LIBRARIES_H
#   include <exec/libraries.h>
#endif

#define CLID_UnixIO_Hidd	"unixioclass"
#define IID_UnixIO	"I_UnixIO"


enum {
    HIDDMIDX_UnixIO_Wait = 0,	/* LONG M ( uioMsg *)		*/
    HIDDMIDX_UnixIO_Select,	/* for async IO, unused		*/
    
    NUM_M_UnixIO
    
};

struct uioMsg
{
    STACKULONG um_MethodID;
    STACKULONG um_Filedesc;
    STACKULONG um_Mode;
};

/* UnixIO HIDD Values */
#define HIDDV_UnixIO_Read       0x1
#define HIDDV_UnixIO_Write      0x2

/* Stubs */
IPTR HIDD_UnixIO_Wait(HIDD *h, ULONG fd, ULONG mode);
HIDD *New_UnixIO(struct Library * /* OOPBase */);


#endif /* HIDD_UNIXIO_H */


