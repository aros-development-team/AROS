/*
    Copyright © 1995-2006, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Unix filedescriptor/socket IO include file
*/

#ifndef UXIO_H
#define UXIO_H

#include <exec/types.h>

#include <exec/tasks.h>
#include <exec/ports.h>
#include <exec/libraries.h>

#include <dos/bptr.h>

#include <oop/oop.h>

#include <proto/exec.h>


/* instance data for the unixioclass */
struct UnixIOData
{
    struct MsgPort		* uio_ReplyPort;
};

/* static data for the unixioclass */
struct uio_data
{
    struct Task 		* ud_WaitForIO;
    struct MsgPort		* ud_Port;
};

struct unixio_base
{
    struct Library		  uio_lib;
    struct ExecBase		* uio_SysBase;
    BPTR			  uio_SegList;
    
    OOP_Class			* uio_unixioclass;
    struct uio_data		  uio_csd;
};

#define UD(cl) (&((struct unixio_base *)cl->UserData)->uio_csd)

#endif /* UXIO */
