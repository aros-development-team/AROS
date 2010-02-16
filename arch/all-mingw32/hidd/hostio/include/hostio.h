#ifndef HIDD_HOSTIO_H
#define HIDD_HOSTIO_H

/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id: unixio.h 30792 2009-03-07 22:40:04Z neil $

    Desc: Host OS filedescriptor/socket IO Include File
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
#ifndef EXEC_EXECBASE_H
#   include <exec/execbase.h>
#endif

#define CLID_Hidd_HostIO "hostio.hidd"
#define IID_Hidd_HostIO	"I_Hidd_HostIO"

struct hioMessage
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
    moHidd_HostIO_Wait = 0,	/* LONG M ( hioMsg *)		*/
    moHidd_HostIO_AsyncIO,	/* 	*/
    moHidd_HostIO_AbortAsyncIO,
    moHidd_HostIO_OpenFile,
    moHidd_HostIO_CloseFile,
    moHidd_HostIO_WriteFile,
    moHidd_HostIO_IOControlFile,    
    moHidd_HostIO_ReadFile,
    moHidd_HostIO_CloneHandle,
    nhm_Hidd_HostIO_Attrs
    
};

struct hioMsg
{
    STACKULONG hm_MethodID;
    APTR       hm_FD;
    APTR       hm_CallBack;
    APTR       hm_CallBackData;
    int        *hm_ErrNoPtr;
    int	       *hm_RawErrNoPtr; /* Raw untranslated host error code */
};

struct hioMsgAsyncIO
{
    STACKULONG hm_MethodID;
    STACKULONG hm_Filedesc;
    STACKULONG hm_Filedesc_Type;
    STACKULONG hm_Mode;
    struct MsgPort * hm_ReplyPort;
};

struct hioMsgAbortAsyncIO
{
    STACKULONG hm_MethodID;
    STACKULONG hm_Filedesc;
};

struct hioMsgOpenFile
{
    STACKULONG  hm_MethodID;
    STRPTR      hm_FileName;	/* File name */
    STACKULONG  hm_Flags;	/* Flags, the same as for open() */
    STACKULONG  hm_Mode;	/* Mode, the same as for open()  */
    int        *hm_ErrNoPtr;
    int	       *hm_RawErrNoPtr; /* Raw untranslated host error code */
};

struct hioMsgCloneHandle
{
    STACKULONG  hm_MethodID;
    APTR        hm_FD;		/* File descriptor */
    int        *hm_ErrNoPtr;
    int	       *hm_RawErrNoPtr; /* Raw untranslated host error code */
};

struct hioMsgCloseFile
{
    STACKULONG  hm_MethodID;
    APTR        hm_FD;		/* File descriptor */
    int        *hm_ErrNoPtr;
    int	       *hm_RawErrNoPtr; /* Raw untranslated host error code */
};

struct hioMsgWriteFile
{
    STACKULONG  hm_MethodID;
    APTR        hm_FD;		/* File descriptor	*/
    APTR        hm_Buffer;	/* Buffer		*/
    STACKULONG  hm_Count;	/* Buffer length	*/
    int        *hm_ErrNoPtr;
    int	       *hm_RawErrNoPtr; /* Raw untranslated host error code */
};

struct hioMsgIOControlFile
{
    STACKULONG  hm_MethodID;
    APTR        hm_FD;		/* File descriptor		    */
    STACKULONG  hm_Request;	/* Request code			    */
    APTR    	hm_Param;	/* Input parameter buffer	    */
    STACKULONG	hm_ParamLen;    /* Input buffer size		    */
    APTR	hm_Output;	/* Output buffer		    */
    STACKULONG  hm_OutputLen;	/* Output buffer size		    */
    int        *hm_ErrNoPtr;
    int	       *hm_RawErrNoPtr; /* Raw untranslated host error code */
};

struct hioMsgReadFile
{
    STACKULONG  hm_MethodID;
    APTR        hm_FD;		/* File descriptor	*/
    APTR        hm_Buffer;	/* Buffer		*/
    STACKULONG  hm_Count;	/* Buffer length	*/
    int        *hm_ErrNoPtr;
    int	       *hm_RawErrNoPtr; /* Raw untranslated host error code */
};

/* HostIO HIDD Values */
#define vHidd_HostIO_Read       0x1
#define vHidd_HostIO_Write      0x2
#define vHidd_HostIO_RW         (vHidd_HostIO_Read | vHidd_HostIO_Write)
#define vHidd_HostIO_Abort	0x4
#define vHidd_HostIO_Keep       0x8

/* Types of Filedescriptors */
#define vHidd_HostIO_Terminal   0x1
#define vHidd_HostIO_Socket     0x2

#define vHidd_HostIO_Invalid_Handle ((APTR)-1)

/* Stubs */
IPTR Hidd_HostIO_Wait(HIDD o, APTR fd, APTR callback, APTR callbackdata, int *errno_ptr, int *raw_errno_ptr);
IPTR Hidd_HostIO_AsyncIO(HIDD h, ULONG fd, ULONG fd_type, struct MsgPort *port, ULONG mode, struct ExecBase *);
VOID Hidd_HostIO_AbortAsyncIO(HIDD h, ULONG fd, struct ExecBase *);

APTR Hidd_HostIO_OpenFile(HIDD o, const char *filename, int flags, int mode, int *errno_ptr, int *raw_errno_ptr);
VOID Hidd_HostIO_CloseFile(HIDD o, APTR fd, int *errno_ptr, int *raw_errno_ptr);
APTR Hidd_HostIO_CloneHandle(HIDD o, APTR fd, int *errno_ptr, int *raw_errno_ptr);
int Hidd_HostIO_ReadFile(HIDD o, APTR fd, void *buffer, int count, int *errno_ptr, int *raw_errno_ptr);
int Hidd_HostIO_WriteFile(HIDD o, APTR fd, void *buffer, int count, int *errno_ptr, int *raw_errno_ptr);
int Hidd_HostIO_IOControlFile(HIDD o, APTR fd, int request, void *param, int count, void *output, int outlen, int *errno_ptr, int *raw_errno_ptr);

#endif /* HIDD_UNIXIO_H */
