/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Unix filedescriptor/socket IO
    Lang: english
*/

#include <exec/execbase.h>
#include <hidd/unixio.h>
#include <oop/oop.h>
#include <proto/oop.h>
#include <aros/debug.h>

/************
**  Stubs  **
************/

#define OOPBase (OOP_OCLASS(o)->OOPBasePtr)

IPTR Hidd_UnixIO_Wait(HIDD *o, ULONG fd, ULONG mode, APTR callback, APTR callbackdata, struct ExecBase * SysBase)
{
     static OOP_MethodID mid;
     struct uioMsg  	 p;
     
     if (!mid)
     	mid = OOP_GetMethodID(IID_Hidd_UnixIO, moHidd_UnixIO_Wait);
	
     p.um_MethodID  	= mid;
     p.um_Filedesc  	= fd;
     p.um_Mode	    	= mode;
     p.um_CallBack  	= callback;
     p.um_CallBackData  = callbackdata;
         
     return OOP_DoMethod((OOP_Object *)o, (OOP_Msg)&p);
}

IPTR Hidd_UnixIO_AsyncIO(HIDD *o, ULONG fd, ULONG fd_type, struct MsgPort * port, ULONG mode, struct ExecBase * SysBase)
{
     static OOP_MethodID    mid;
     struct uioMsgAsyncIO   p;
     
     if (!mid)
     	mid = OOP_GetMethodID(IID_Hidd_UnixIO, moHidd_UnixIO_AsyncIO);
	
     p.um_MethodID      = mid;
     p.um_Filedesc      = fd;
     p.um_Filedesc_Type = fd_type;
     p.um_ReplyPort     = port;
     p.um_Mode	        = mode;
     
     return OOP_DoMethod((OOP_Object *)o, (OOP_Msg)&p);
}


VOID Hidd_UnixIO_AbortAsyncIO(HIDD *o, ULONG fd, struct ExecBase * SysBase)
{
     static OOP_MethodID    	mid;
     struct uioMsgAbortAsyncIO  p;
     
     if (!mid)
     	mid = OOP_GetMethodID(IID_Hidd_UnixIO, moHidd_UnixIO_AbortAsyncIO);
     
     p.um_MethodID = mid;
     p.um_Filedesc = fd;
     
     OOP_DoMethod((OOP_Object *)o, (OOP_Msg)&p);
}

int Hidd_UnixIO_OpenFile(HIDD *o, const char *filename, int flags, int mode, int *errno_ptr)
{
     static OOP_MethodID    mid;
     struct uioMsgOpenFile  p;
     
     if (!mid)
     	mid = OOP_GetMethodID(IID_Hidd_UnixIO, moHidd_UnixIO_OpenFile);

     p.um_MethodID  = mid;
     p.um_FileName  = (STRPTR)filename;
     p.um_Flags     = (STACKULONG)flags;
     p.um_Mode      = (STACKULONG)mode;
     p.um_ErrNoPtr  = errno_ptr;
     
     return (int)OOP_DoMethod((OOP_Object *)o, (OOP_Msg)&p);
}

VOID Hidd_UnixIO_CloseFile(HIDD *o, int fd, int *errno_ptr)
{
     static OOP_MethodID    mid;
     struct uioMsgCloseFile p;
     
     if (!mid)
     	mid = OOP_GetMethodID(IID_Hidd_UnixIO, moHidd_UnixIO_CloseFile);
	
     p.um_MethodID  = mid;
     p.um_FD 	    = (APTR)fd;
     p.um_ErrNoPtr  = errno_ptr;
     
     OOP_DoMethod((OOP_Object *)o, (OOP_Msg)&p);
}

int Hidd_UnixIO_WriteFile(HIDD *o, int fd, const void *buffer, int count, int *errno_ptr)
{
     static OOP_MethodID    mid;
     struct uioMsgWriteFile p;
     
     if (!mid)
     	mid = OOP_GetMethodID(IID_Hidd_UnixIO, moHidd_UnixIO_WriteFile);
	
     p.um_MethodID  = mid;
     p.um_FD 	    = (APTR)fd;
     p.um_Buffer    = (APTR)buffer;
     p.um_Count     = (STACKULONG)count;
     p.um_ErrNoPtr  = errno_ptr;
     
     return (int)OOP_DoMethod((OOP_Object *)o, (OOP_Msg)&p);
}

int Hidd_UnixIO_IOControlFile(HIDD *o, int fd, int request, void *param, int *errno_ptr)
{
     static OOP_MethodID    	mid;
     struct uioMsgIOControlFile p;
     
     if (!mid)
     	mid = OOP_GetMethodID(IID_Hidd_UnixIO, moHidd_UnixIO_IOControlFile);

     p.um_MethodID  = mid;
     p.um_FD 	    = (APTR)fd;
     p.um_Request   = (STACKULONG)request;
     p.um_Param     = (APTR)param;
     p.um_ErrNoPtr  = errno_ptr;
     
     return (int)OOP_DoMethod((OOP_Object *)o, (OOP_Msg)&p);
}

/* The below function is just a hack to avoid
   name conflicts inside intuition_driver.c
*/

#undef OOPBase

HIDD *New_UnixIO(struct Library *OOPBase, struct ExecBase * SysBase)
{
   struct TagItem tags[] = {{ TAG_END, 0 }};
   
   return (HIDD)OOP_NewObject (NULL, CLID_Hidd_UnixIO, (struct TagItem *)tags);
}
