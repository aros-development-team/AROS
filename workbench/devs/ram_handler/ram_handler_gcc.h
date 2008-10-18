/*
    Copyright © 1995-2008, The AROS Development Team. All rights reserved.
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

#endif

