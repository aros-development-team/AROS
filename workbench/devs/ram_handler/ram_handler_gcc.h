/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang:
*/
#ifndef RAM_HANDLER_GCC_H
#define RAM_HANDLER_GCC_H
#include  <aros/libcall.h>
#include  <exec/execbase.h>
#include  <exec/io.h>
#include  <exec/devices.h>
#include  <dos/dos.h>
#include  <dos/filesystem.h>
#include  "HashTable.h"

struct vnode;			/* Predeclaration */

struct rambase
{
    struct Device device;
    struct ExecBase *sysbase;
    struct DosLibrary *dosbase;
    struct UtilityBase *utilitybase;
    BPTR seglist;
    struct MsgPort *port;		/* Port to put IORequests to */
    struct SignalSemaphore *sigsem;	/* Semaphore for iofs */
    struct IOFileSys *iofs;		/* IORequest to be aborted or NULL */
    struct vnode *root;		        /* Root of the filesystem */
    HashTable *notifications;           /* Notification requests corresponding
					   to files and directories that
					   currently not exist */
    struct MsgPort *notifyPort;	        /* Notification messages will be
					   replied here */
};

#define init(rambase, segList) \
AROS_LC2(struct rambase *, init, AROS_LCA(struct rambase *, rambase, D0), AROS_LCA(BPTR, segList, A0), struct ExecBase *, SysBase, 0, ram)

#define open(iob, unitnum, flags) \
AROS_LC3(void, open, AROS_LCA(struct ramrequest *, iob, A1), AROS_LCA(ULONG, unitnum, D0), AROS_LCA(ULONG, flags, D0), struct rambase *, rambase, 1, ram)

#define close(iob) \
AROS_LC1(BPTR, close, AROS_LCA(struct ramrequest *, iob, A1), struct rambase *, rambase, 2, ram)

#define expunge() \
AROS_LC0(BPTR, expunge, struct rambase *, rambase, 3, ram)

#define null() \
AROS_LC0(int, null, struct rambase *, rambase, 4, ram)

#define beginio(iob) \
AROS_LC1(void, beginio, AROS_LCA(struct ramrequest *, iob, A1), struct rambase *, rambase, 5, ram)

#define abortio(iob) \
AROS_LC1(LONG, abortio, AROS_LCA(struct ramrequest *, iob, A1), struct rambase *, rambase, 6, ram)

#endif

