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
    int            fd_type;
    int 	   mode;
    int 	   result;
    void *callback;
    void *callbackdata;
};


enum {
    moHidd_UnixIO_Wait = 0,	/* LONG M ( uioMsg *)		*/
    moHidd_UnixIO_AsyncIO,	/* 	*/
    moHidd_UnixIO_AbortAsyncIO,
    moHidd_UnixIO_OpenFile,
    moHidd_UnixIO_CloseFile,
    moHidd_UnixIO_WriteFile,
    moHidd_UnixIO_IOControlFile,    
    num_Hidd_UnixIO_Attrs
    
};

struct uioMsg
{
    STACKULONG um_MethodID;
    STACKULONG um_Filedesc;
    STACKULONG um_Filedesc_Type;
    STACKULONG um_Mode;
    APTR um_CallBack;
    APTR um_CallBackData;
};

struct uioMsgAsyncIO
{
    STACKULONG um_MethodID;
    STACKULONG um_Filedesc;
    STACKULONG um_Filedesc_Type;
    STACKULONG um_Mode;
    struct MsgPort * um_ReplyPort;
};

struct uioMsgAbortAsyncIO
{
    STACKULONG um_MethodID;
    STACKULONG um_Filedesc;
};

struct uioMsgOpenFile
{
    STACKULONG  um_MethodID;
    STRPTR      um_FileName;
    STACKULONG  um_Flags;
    STACKULONG  um_Mode;
    int        *um_ErrNoPtr;
};

struct uioMsgCloseFile
{
    STACKULONG  um_MethodID;
    APTR        um_FD;
    int        *um_ErrNoPtr;
};

struct uioMsgWriteFile
{
    STACKULONG  um_MethodID;
    APTR        um_FD;
    APTR        um_Buffer;
    STACKULONG  um_Count;
    int        *um_ErrNoPtr;
};

struct uioMsgIOControlFile
{
    STACKULONG  um_MethodID;
    APTR        um_FD;
    STACKULONG  um_Request;
    APTR    	um_Param;
    int        *um_ErrNoPtr;
};

/* UnixIO HIDD Values */
#define vHidd_UnixIO_Read       0x1
#define vHidd_UnixIO_Write      0x2
#define vHidd_UnixIO_RW         (vHidd_UnixIO_Read | vHidd_UnixIO_Write)
#define vHidd_UnixIO_Abort	0x4
#define vHidd_UnixIO_Keep       0x8

/* Types of Filedescriptors */
#define vHidd_UnixIO_Terminal   0x1
#define vHidd_UnixIO_Socket     0x2

/* Stubs */
IPTR Hidd_UnixIO_Wait(HIDD *h, ULONG fd, ULONG mode, APTR callback,  APTR callbackdata, struct ExecBase *);
HIDD *New_UnixIO(struct Library * /* OOPBase */, struct ExecBase *);
IPTR Hidd_UnixIO_AsyncIO(HIDD *h, ULONG fd, ULONG fd_type, struct MsgPort *port, ULONG mode, struct ExecBase *);
VOID Hidd_UnixIO_AbortAsyncIO(HIDD *h, ULONG fd, struct ExecBase *);

int Hidd_UnixIO_OpenFile(HIDD *o, const char *filename, int flags, int mode, int *errno_ptr);
VOID Hidd_UnixIO_CloseFile(HIDD *o, int fd, int *errno_ptr);
int Hidd_UnixIO_WriteFile(HIDD *o, int fd, const void *buffer, int count, int *errno_ptr);
int Hidd_UnixIO_IOControlFile(HIDD *o, int fd, int request, void *param, int *errno_ptr);



#endif /* HIDD_UNIXIO_H */


