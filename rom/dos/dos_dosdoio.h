/*
    Copyright (C) 2001 AROS - The Amiga Research OS
    $Id$

    Desc: Internal proto and define for DosDoIO
    Lang: English
*/
#ifndef DOS_DOSDOIO_H
#define DOS_DOSDOIO_H

#include <exec/io.h>


BYTE DosDoIO(struct IORequest * iORequest,
	     struct ExecBase * SysBase);

#define DosDoIO(iorequest) DosDoIO(iorequest, SysBase)


#endif /* DOS_DOSDOIO_H */
