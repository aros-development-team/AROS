#ifndef HIDD_UNIXIO_H
#define HIDD_UNIXIO_H

/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Unix filedescriptor/socket IO Include File
    Lang: english
*/

#include <exec/libraries.h>
#include <exec/ports.h>
#include <oop/oop.h>
#include <interface/Hidd_UnixIO.h>

#define CLID_Hidd_UnixIO "unixio.hidd"

struct uioInterrupt
{
    struct MinNode Node;
    int		   fd;
    int		   mode;
    void	   (*handler)(int, int, void *);
    void	   *handlerData;
};

/*
 * (Semi)-public part of unixio.hidd library base.
 * Normally you don't need to access it at all. However it can be
 * useful if you want to use more specific operations on filedescriptors.
 * For example emul-handler needs this.
 */
struct UnixIOBase
{
    struct Library uio_Library;		/* Library node				  */
    APTR	   uio_LibcHandle;	/* hostlib.resource's handle to host libc */
    int		  *uio_ErrnoPtr;	/* Pointer to host's errno variable	  */
};

/* I/O mode flags */
#define vHidd_UnixIO_Read       0x1
#define vHidd_UnixIO_Write      0x2
#define vHidd_UnixIO_RW         (vHidd_UnixIO_Read | vHidd_UnixIO_Write)
#define vHidd_UnixIO_Error	0x10

#endif /* HIDD_UNIXIO_H */
