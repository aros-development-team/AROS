#ifndef HIDD_UNIXIO_H
#define HIDD_UNIXIO_H

/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Unix filedescriptor/socket IO Include File
    Lang: english
*/

#include <exec/ports.h>
#include <oop/oop.h>

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

struct uioInterrupt
{
    struct MinNode Node;
    int		   fd;
    int		   mode;
    void	   (*handler)(int, int, void *);
    void	   *handlerData;
};

enum {
    moHidd_UnixIO_Wait = 0,	/* LONG M ( uioMsg *)		*/
    moHidd_UnixIO_AsyncIO,	/* 	*/
    moHidd_UnixIO_AbortAsyncIO,
    moHidd_UnixIO_OpenFile,
    moHidd_UnixIO_CloseFile,
    moHidd_UnixIO_WriteFile,
    moHidd_UnixIO_IOControlFile,    
    moHidd_UnixIO_ReadFile,
    moHidd_UnixIO_AddInterrupt,
    moHidd_UnixIO_RemInterrupt,
    num_Hidd_UnixIO_Attrs
    
};

struct uioMsg
{
    STACKULONG um_MethodID;
    STACKULONG um_Filedesc;
    STACKULONG um_Filedesc_Type;
    STACKULONG um_Mode;
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

struct uioMsgReadFile
{
    STACKULONG  um_MethodID;
    APTR        um_FD;
    APTR        um_Buffer;
    STACKULONG  um_Count;
    int        *um_ErrNoPtr;
};

struct uioMsgAddInterrupt
{
    STACKULONG	         um_MethodID;
    struct uioInterrupt *um_Int;
};

struct uioMsgRemInterrupt
{
    STACKULONG	         um_MethodID;
    struct uioInterrupt *um_Int;
};

/* UnixIO OOP_Object *Values */
#define vHidd_UnixIO_Read       0x1
#define vHidd_UnixIO_Write      0x2
#define vHidd_UnixIO_RW         (vHidd_UnixIO_Read | vHidd_UnixIO_Write)
#define vHidd_UnixIO_Abort	0x4
#define vHidd_UnixIO_Keep       0x8
#define vHidd_UnixIO_Error	0x10

/* Types of Filedescriptors */
#define vHidd_UnixIO_Terminal   0x1
#define vHidd_UnixIO_Socket     0x2

/* Stubs */
IPTR Hidd_UnixIO_Wait(OOP_Object *h, ULONG fd, ULONG mode);
IPTR Hidd_UnixIO_AsyncIO(OOP_Object *h, ULONG fd, ULONG fd_type, struct MsgPort *port, ULONG mode, struct ExecBase *);
VOID Hidd_UnixIO_AbortAsyncIO(OOP_Object *h, ULONG fd, struct ExecBase *);

int Hidd_UnixIO_OpenFile(OOP_Object *o, const char *filename, int flags, int mode, int *errno_ptr);
VOID Hidd_UnixIO_CloseFile(OOP_Object *o, int fd, int *errno_ptr);
int Hidd_UnixIO_ReadFile(OOP_Object *o, int fd, void *buffer, int count, int *errno_ptr);
int Hidd_UnixIO_WriteFile(OOP_Object *o, int fd, const void *buffer, int count, int *errno_ptr);
int Hidd_UnixIO_IOControlFile(OOP_Object *o, int fd, int request, void *param, int *errno_ptr);
int Hidd_UnixIO_AddInterrupt(OOP_Object *o, struct uioInterrupt *interrupt);
void Hidd_UnixIO_RemInterrupt(OOP_Object *o, struct uioInterrupt *interrupt);

#endif /* HIDD_UNIXIO_H */


