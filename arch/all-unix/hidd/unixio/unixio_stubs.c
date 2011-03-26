/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Unix filedescriptor/socket IO
    Lang: english
*/

#include <exec/execbase.h>
#include <hidd/unixio.h>
#include <oop/oop.h>
#include <proto/oop.h>

/************
**  Stubs  **
************/

#define OOPBase (OOP_OCLASS(o)->OOPBasePtr)

IPTR Hidd_UnixIO_Wait(OOP_Object *o, ULONG fd, ULONG mode)
{
     static OOP_MethodID mid;
     struct uioMsg  	 p, *msg = &p;
     
     if (!mid)
     	mid = OOP_GetMethodID(IID_Hidd_UnixIO, moHidd_UnixIO_Wait);
	
     p.um_MethodID  	= mid;
     p.um_Filedesc  	= fd;
     p.um_Mode	    	= mode;
         
     return OOP_DoMethod((OOP_Object *)o, (OOP_Msg)msg);
}

int Hidd_UnixIO_OpenFile(OOP_Object *o, const char *filename, int flags, int mode, int *errno_ptr)
{
     static OOP_MethodID    mid;
     struct uioMsgOpenFile  p, *msg = &p;
     
     if (!mid)
     	mid = OOP_GetMethodID(IID_Hidd_UnixIO, moHidd_UnixIO_OpenFile);

     p.um_MethodID  = mid;
     p.um_FileName  = (STRPTR)filename;
     p.um_Flags     = (STACKULONG)flags;
     p.um_Mode      = (STACKULONG)mode;
     p.um_ErrNoPtr  = errno_ptr;
     
     return (int)OOP_DoMethod((OOP_Object *)o, (OOP_Msg)msg);
}

int Hidd_UnixIO_CloseFile(OOP_Object *o, int fd, int *errno_ptr)
{
     static OOP_MethodID    mid;
     struct uioMsgCloseFile p, *msg = &p;
     
     if (!mid)
     	mid = OOP_GetMethodID(IID_Hidd_UnixIO, moHidd_UnixIO_CloseFile);
	
     p.um_MethodID  = mid;
     p.um_FD 	    = (APTR)fd;
     p.um_ErrNoPtr  = errno_ptr;
     
     return OOP_DoMethod((OOP_Object *)o, (OOP_Msg)msg);
}

int Hidd_UnixIO_ReadFile(OOP_Object *o, int fd, void *buffer, int count, int *errno_ptr)
{
     static OOP_MethodID    mid;
     struct uioMsgReadFile p, *msg = &p;
     
     if (!mid)
     	mid = OOP_GetMethodID(IID_Hidd_UnixIO, moHidd_UnixIO_ReadFile);
	
     p.um_MethodID  = mid;
     p.um_FD 	    = (APTR)fd;
     p.um_Buffer    = (APTR)buffer;
     p.um_Count     = (STACKULONG)count;
     p.um_ErrNoPtr  = errno_ptr;
     
     return (int)OOP_DoMethod((OOP_Object *)o, (OOP_Msg)msg);
}

int Hidd_UnixIO_WriteFile(OOP_Object *o, int fd, const void *buffer, int count, int *errno_ptr)
{
     static OOP_MethodID    mid;
     struct uioMsgWriteFile p, *msg = &p;
     
     if (!mid)
     	mid = OOP_GetMethodID(IID_Hidd_UnixIO, moHidd_UnixIO_WriteFile);
	
     p.um_MethodID  = mid;
     p.um_FD 	    = (APTR)fd;
     p.um_Buffer    = (APTR)buffer;
     p.um_Count     = (STACKULONG)count;
     p.um_ErrNoPtr  = errno_ptr;
     
     return (int)OOP_DoMethod((OOP_Object *)o, (OOP_Msg)msg);
}

int Hidd_UnixIO_IOControlFile(OOP_Object *o, int fd, int request, void *param, int *errno_ptr)
{
     static OOP_MethodID    	mid;
     struct uioMsgIOControlFile p, *msg = &p;
     
     if (!mid)
     	mid = OOP_GetMethodID(IID_Hidd_UnixIO, moHidd_UnixIO_IOControlFile);

     p.um_MethodID  = mid;
     p.um_FD 	    = (APTR)fd;
     p.um_Request   = (STACKULONG)request;
     p.um_Param     = (APTR)param;
     p.um_ErrNoPtr  = errno_ptr;
     
     return (int)OOP_DoMethod((OOP_Object *)o, (OOP_Msg)msg);
}

int Hidd_UnixIO_AddInterrupt(OOP_Object *o, struct uioInterrupt *interrupt)
{
    static OOP_MethodID mid;
    struct uioMsgAddInterrupt p, *msg = &p;
    
    if (!mid)
	mid = OOP_GetMethodID(IID_Hidd_UnixIO, moHidd_UnixIO_AddInterrupt);

    p.um_MethodID  = mid;
    p.um_Int	    = interrupt;

    return OOP_DoMethod(o, (OOP_Msg)msg);
}

void Hidd_UnixIO_RemInterrupt(OOP_Object *o, struct uioInterrupt *interrupt)
{
    static OOP_MethodID mid;
    struct uioMsgRemInterrupt p, *msg = &p;

    if (!mid)
     	mid = OOP_GetMethodID(IID_Hidd_UnixIO, moHidd_UnixIO_RemInterrupt);

    p.um_MethodID  = mid;
    p.um_Int	    = interrupt;
     
    OOP_DoMethod(o, (OOP_Msg)msg);
}
