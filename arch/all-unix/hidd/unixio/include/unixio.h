#ifndef HIDD_UNIXIO_H
#define HIDD_UNIXIO_H

/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
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
#ifndef EXEC_PORTS_H
#   include <exec/ports.h>
#endif

#define CLID_Hidd_UnixIO "unixio.hidd"
#define IID_Hidd_UnixIO	"I_Hidd_UnixIO"

struct uioMessage
{
    struct Message Message;
    int 	   fd;
    int 	   mode;
    int 	   result;
    void *callback;
    void *callbackdata;
};


enum {
    moHidd_UnixIO_Wait = 0,	/* LONG M ( uioMsg *)		*/
    moHidd_UnixIO_AsyncIO,	/* 	*/
    moHidd_UnixIO_AbortAsyncIO,
    num_Hidd_UnixIO_Attrs
    
};

struct uioMsg
{
    STACKULONG um_MethodID;
    STACKULONG um_Filedesc;
    STACKULONG um_Mode;
    APTR um_CallBack;
    APTR um_CallBackData;
};

struct uioMsgAsyncIO
{
    STACKULONG um_MethodID;
    STACKULONG um_Filedesc;
    STACKULONG um_Mode;
    struct MsgPort * um_ReplyPort;
};

struct uioMsgAbortAsyncIO
{
    STACKULONG um_MethodID;
    STACKULONG um_Filedesc;
};

/* UnixIO HIDD Values */
#define vHidd_UnixIO_Read       0x1
#define vHidd_UnixIO_Write      0x2
#define vHidd_UnixIO_RW         (vHidd_UnixIO_Read | vHidd_UnixIO_Write)
#define vHidd_UnixIO_Abort	0x4

/* Stubs */
IPTR Hidd_UnixIO_Wait(HIDD *h, ULONG fd, ULONG mode, APTR callback,  APTR callbackdata, struct ExecBase *);
HIDD *New_UnixIO(struct Library * /* OOPBase */, struct ExecBase *);
IPTR Hidd_UnixIO_AsyncIO(HIDD *h, ULONG fd, struct MsgPort *port, ULONG mode, struct ExecBase *);
VOID Hidd_UnixIO_AbortAsyncIO(HIDD *h, ULONG fd, struct ExecBase *);

#endif /* HIDD_UNIXIO_H */


