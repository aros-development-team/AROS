/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Inline stubs for Unix filedescriptor/socket IO
    Lang: english
*/

#ifndef HIDD_UNIXIO_INLINE_H
#define HIDD_UNIXIO_INLINE_H

#include <exec/execbase.h>
#include <hidd/unixio.h>
#include <oop/oop.h>
#include <proto/oop.h>

#ifndef HiddUnixIOMethodBase
#define HiddUnixIOMethodBase Hidd_UnixIO_GetMethodBase(OOPBase)

static inline OOP_MethodID Hidd_UnixIO_GetMethodBase(struct Library *OOPBase)
{
    static OOP_MethodID base;

    if (!base)
    	base = OOP_GetMethodID(IID_Hidd_UnixIO, 0);

    return base;
}
#endif

static inline IPTR Hidd_UnixIO_Wait(OOP_Object *o, ULONG fd, ULONG mode)
{
    struct Library *OOPBase = OOP_OCLASS(o)->OOPBasePtr;
    struct uioMsg p;
	
    p.um_MethodID = HiddUnixIOMethodBase + moHidd_UnixIO_Wait;
    p.um_Filedesc = fd;
    p.um_Mode	  = mode;

    return OOP_DoMethod(o, (OOP_Msg)&p);
}

static inline int Hidd_UnixIO_OpenFile(OOP_Object *o, const char *filename, int flags, int mode, int *errno_ptr)
{
    struct Library *OOPBase = OOP_OCLASS(o)->OOPBasePtr;
    struct uioMsgOpenFile p;
	
    p.um_MethodID = HiddUnixIOMethodBase + moHidd_UnixIO_OpenFile;
    p.um_FileName = filename;
    p.um_Flags    = flags;
    p.um_Mode     = mode;
    p.um_ErrNoPtr = errno_ptr;

    return OOP_DoMethod((OOP_Object *)o, (OOP_Msg)&p);
}

static inline int Hidd_UnixIO_CloseFile(OOP_Object *o, int fd, int *errno_ptr)
{
    struct Library *OOPBase = OOP_OCLASS(o)->OOPBasePtr;
    struct uioMsgCloseFile p;
	
    p.um_MethodID = HiddUnixIOMethodBase + moHidd_UnixIO_CloseFile;
    p.um_FD 	  = (APTR)fd;
    p.um_ErrNoPtr = errno_ptr;

    return OOP_DoMethod(o, (OOP_Msg)&p);
}

static inline int Hidd_UnixIO_ReadFile(OOP_Object *o, int fd, void *buffer, int count, int *errno_ptr)
{
    struct Library *OOPBase = OOP_OCLASS(o)->OOPBasePtr;
    struct uioMsgReadFile p;
	
    p.um_MethodID = HiddUnixIOMethodBase + moHidd_UnixIO_ReadFile;
    p.um_FD 	  = (APTR)fd;
    p.um_Buffer   = buffer;
    p.um_Count    = count;
    p.um_ErrNoPtr = errno_ptr;

    return OOP_DoMethod(o, (OOP_Msg)&p);
}

static inline int Hidd_UnixIO_WriteFile(OOP_Object *o, int fd, const void *buffer, int count, int *errno_ptr)
{
    struct Library *OOPBase = OOP_OCLASS(o)->OOPBasePtr;
    struct uioMsgWriteFile p;
	
    p.um_MethodID = HiddUnixIOMethodBase + moHidd_UnixIO_WriteFile;
    p.um_FD 	  = (APTR)fd;
    p.um_Buffer   = buffer;
    p.um_Count    = count;
    p.um_ErrNoPtr = errno_ptr;

    return OOP_DoMethod(o, (OOP_Msg)&p);
}

static inline int Hidd_UnixIO_IOControlFile(OOP_Object *o, int fd, int request, void *param, int *errno_ptr)
{
    struct Library *OOPBase = OOP_OCLASS(o)->OOPBasePtr;
    struct uioMsgIOControlFile p;

    p.um_MethodID = HiddUnixIOMethodBase + moHidd_UnixIO_IOControlFile;
    p.um_FD 	  = (APTR)fd;
    p.um_Request  = request;
    p.um_Param    = param;
    p.um_ErrNoPtr = errno_ptr;

    return OOP_DoMethod((OOP_Object *)o, (OOP_Msg)&p);
}

static inline int Hidd_UnixIO_AddInterrupt(OOP_Object *o, struct uioInterrupt *interrupt)
{
    struct Library *OOPBase = OOP_OCLASS(o)->OOPBasePtr;
    struct uioMsgAddInterrupt p;

    p.um_MethodID = HiddUnixIOMethodBase + moHidd_UnixIO_AddInterrupt;
    p.um_Int	  = interrupt;

    return OOP_DoMethod(o, (OOP_Msg)&p);
}

static inline void Hidd_UnixIO_RemInterrupt(OOP_Object *o, struct uioInterrupt *interrupt)
{
    struct Library *OOPBase = OOP_OCLASS(o)->OOPBasePtr;
    struct uioMsgRemInterrupt p;

    p.um_MethodID = HiddUnixIOMethodBase + moHidd_UnixIO_RemInterrupt;
    p.um_Int	  = interrupt;

    OOP_DoMethod(o, (OOP_Msg)&p);
}

#endif
