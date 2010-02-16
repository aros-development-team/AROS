/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id: hostio_stubs.c 30792 2009-03-07 22:40:04Z neil $

    Desc: Host OS filedescriptor/socket IO
    Lang: english
*/

#include <exec/execbase.h>
#include <hidd/hostio.h>
#include <oop/oop.h>
#include <proto/oop.h>
#include <aros/debug.h>

/************
**  Stubs  **
************/

#define OOPBase (OOP_OCLASS(o)->OOPBasePtr)

IPTR Hidd_HostIO_Wait(HIDD o, APTR fd, ULONG mode, APTR callback, APTR callbackdata, int *errno_ptr, int *raw_errno_ptr)
{
     static OOP_MethodID mid;
     struct hioMsg  	 p, *msg = &p;
     
     if (!mid)
     	mid = OOP_GetMethodID(IID_Hidd_HostIO, moHidd_HostIO_Wait);
	
     p.hm_MethodID  	= mid;
     p.hm_FD	  	= fd;
     p.hm_CallBack  	= callback;
     p.hm_CallBackData  = callbackdata;
     p.hm_ErrNoPtr	= errno_ptr;
     p.hm_RawErrNoPtr	= raw_errno_ptr;

     return OOP_DoMethod((OOP_Object *)o, (OOP_Msg)msg);
}

IPTR Hidd_HostIO_AsyncIO(HIDD o, ULONG fd, ULONG fd_type, struct MsgPort * port, ULONG mode, struct ExecBase * SysBase)
{
     static OOP_MethodID    mid;
     struct hioMsgAsyncIO   p, *msg = &p;
     
     if (!mid)
     	mid = OOP_GetMethodID(IID_Hidd_HostIO, moHidd_HostIO_AsyncIO);
	
     p.hm_MethodID      = mid;
     p.hm_Filedesc      = fd;
     p.hm_Filedesc_Type = fd_type;
     p.hm_ReplyPort     = port;
     p.hm_Mode	        = mode;
     
     return OOP_DoMethod((OOP_Object *)o, (OOP_Msg)msg);
}


VOID Hidd_HostIO_AbortAsyncIO(HIDD o, ULONG fd, struct ExecBase * SysBase)
{
     static OOP_MethodID    	mid;
     struct hioMsgAbortAsyncIO  p, *msg = &p;
     
     if (!mid)
     	mid = OOP_GetMethodID(IID_Hidd_HostIO, moHidd_HostIO_AbortAsyncIO);
     
     p.hm_MethodID = mid;
     p.hm_Filedesc = fd;
     
     OOP_DoMethod((OOP_Object *)o, (OOP_Msg)msg);
}

APTR Hidd_HostIO_OpenFile(HIDD o, const char *filename, int flags, int mode, int *errno_ptr, int *raw_errno_ptr)
{
     static OOP_MethodID    mid;
     struct hioMsgOpenFile  p, *msg = &p;
     
     if (!mid)
     	mid = OOP_GetMethodID(IID_Hidd_HostIO, moHidd_HostIO_OpenFile);

     p.hm_MethodID  = mid;
     p.hm_FileName  = (STRPTR)filename;
     p.hm_Flags     = (STACKULONG)flags;
     p.hm_Mode      = (STACKULONG)mode;
     p.hm_ErrNoPtr  = errno_ptr;
     p.hm_RawErrNoPtr = raw_errno_ptr;
     
     return (APTR)OOP_DoMethod((OOP_Object *)o, (OOP_Msg)msg);
}

APTR Hidd_HostIO_CloneHandle(HIDD o, APTR fd, int *errno_ptr, int *raw_errno_ptr)
{
     static OOP_MethodID    mid;
     struct hioMsgCloneHandle p, *msg = &p;
     
     if (!mid)
     	mid = OOP_GetMethodID(IID_Hidd_HostIO, moHidd_HostIO_CloneHandle);

     p.hm_MethodID  = mid;
     p.hm_FD 	    = fd;
     p.hm_ErrNoPtr  = errno_ptr;
     p.hm_RawErrNoPtr = raw_errno_ptr;
 
     return (APTR)OOP_DoMethod((OOP_Object *)o, (OOP_Msg)msg);
}

VOID Hidd_HostIO_CloseFile(HIDD o, APTR fd, int *errno_ptr, int *raw_errno_ptr)
{
     static OOP_MethodID    mid;
     struct hioMsgCloseFile p, *msg = &p;
     
     if (!mid)
     	mid = OOP_GetMethodID(IID_Hidd_HostIO, moHidd_HostIO_CloseFile);
	
     p.hm_MethodID  = mid;
     p.hm_FD 	    = fd;
     p.hm_ErrNoPtr  = errno_ptr;
     p.hm_RawErrNoPtr = raw_errno_ptr;
     
     OOP_DoMethod((OOP_Object *)o, (OOP_Msg)msg);
}

int Hidd_HostIO_ReadFile(HIDD o, APTR fd, void *buffer, int count, int *errno_ptr, int *raw_errno_ptr)
{
     static OOP_MethodID    mid;
     struct hioMsgReadFile p, *msg = &p;
     
     if (!mid)
     	mid = OOP_GetMethodID(IID_Hidd_HostIO, moHidd_HostIO_ReadFile);
	
     p.hm_MethodID  = mid;
     p.hm_FD 	    = fd;
     p.hm_Buffer    = buffer;
     p.hm_Count     = count;
     p.hm_ErrNoPtr  = errno_ptr;
     p.hm_RawErrNoPtr = raw_errno_ptr;
     
     return OOP_DoMethod((OOP_Object *)o, (OOP_Msg)msg);
}

int Hidd_HostIO_WriteFile(HIDD o, APTR fd, void *buffer, int count, int *errno_ptr, int *raw_errno_ptr)
{
     static OOP_MethodID    mid;
     struct hioMsgWriteFile p, *msg = &p;
     
     if (!mid)
     	mid = OOP_GetMethodID(IID_Hidd_HostIO, moHidd_HostIO_WriteFile);
	
     p.hm_MethodID  = mid;
     p.hm_FD 	    = fd;
     p.hm_Buffer    = buffer;
     p.hm_Count     = count;
     p.hm_ErrNoPtr  = errno_ptr;
     p.hm_RawErrNoPtr = raw_errno_ptr;
     
     return OOP_DoMethod((OOP_Object *)o, (OOP_Msg)msg);
}

int Hidd_HostIO_IOControlFile(HIDD o, APTR fd, int request, void *param, int count, void *output, int outlen, int *errno_ptr, int *raw_errno_ptr)
{
     static OOP_MethodID    	mid;
     struct hioMsgIOControlFile p, *msg = &p;
     
     if (!mid)
     	mid = OOP_GetMethodID(IID_Hidd_HostIO, moHidd_HostIO_IOControlFile);

     p.hm_MethodID  = mid;
     p.hm_FD 	    = fd;
     p.hm_Request   = request;
     p.hm_Param     = param;
     p.hm_ParamLen  = count;
     p.hm_Output    = output;
     p.hm_OutputLen = outlen;
     p.hm_ErrNoPtr  = errno_ptr;
     p.hm_RawErrNoPtr = raw_errno_ptr;
     
     return OOP_DoMethod((OOP_Object *)o, (OOP_Msg)msg);
}
