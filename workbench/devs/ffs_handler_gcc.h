/*
    (C) 1995-96 AROS - The Amiga Research OS
    $Id$
    $Log$
    Revision 1.4  2000/10/11 17:18:47  stegerg
    fixed a bug in (disk-block) protection bit handling.
    source cleanup. added some debug output.

    Revision 1.3  1998/10/20 16:47:25  hkiel
    Amiga Research OS

    Revision 1.2  1998/08/19 18:08:37  bernie
    changed module definition to FFS_HANDLER_GCC_H

    Revision 1.1  1996/11/14 08:53:30  aros
    First attempt for a real fastfilesystem
    (only directoryscans for now)


    Desc:
    Lang:
*/
#ifndef FFS_HANDLER_GCC_H
#define FFS_HANDLER_GCC_H
#include <aros/libcall.h>
#include <exec/execbase.h>
#include <exec/io.h>
#include <exec/devices.h>
#include <dos/dos.h>
#include <dos/filesystem.h>

struct ffsbase
{
    struct Device 		device;
    struct ExecBase 		*sysbase;
    struct DosLibrary 		*dosbase;
    BPTR 			seglist;
    ULONG 			unitcount;
    struct MsgPort 		port;		/* Port to put IORequests to */
    struct MsgPort 		dport;		/* reply port for exec device */
    struct SignalSemaphore 	sigsem;		/* Semaphore for rport */
    struct MsgPort 		rport;		/* Message reply port */
    struct IOFileSys 		*iofs;		/* IORequest to be aborted or NULL */
    LONG 			dlflag;		/* have to change dos list */
    struct MinList 		inserted;
    struct MinList 		mounted;
    struct MinList 		removed;
};

#define expunge() \
AROS_LC0(BPTR, expunge, struct rambase *, ffsbase, 3, ffs)

#define SysBase ffsbase->sysbase
#define DOSBase ffsbase->dosbase

#endif

