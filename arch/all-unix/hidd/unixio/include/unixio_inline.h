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
#define HiddUnixIOMethodBase Hidd_UnixIO_GetMethodBase(__obj)

static inline OOP_MethodID Hidd_UnixIO_GetMethodBase(OOP_Object *obj)
{
    static OOP_MethodID base;

    if (!base)
    {
        struct Library *OOPBase = (struct Library *)OOP_OCLASS(obj)->OOPBasePtr;

    	base = OOP_GetMethodID(IID_Hidd_UnixIO, 0);
    }

    return base;
}
#endif

static inline IPTR __inline_Hidd_UnixIO_Wait(OOP_MethodID base, OOP_Object *o, ULONG fd, ULONG mode)
{
    struct uioMsg p;
	
    p.um_MethodID = base + moHidd_UnixIO_Wait;
    p.um_Filedesc = fd;
    p.um_Mode	  = mode;

    return OOP_DoMethod(o, (OOP_MethodID *)&p.um_MethodID);
}

#define Hidd_UnixIO_Wait(o, fd, mode) \
    ({OOP_Object *__obj = o; __inline_Hidd_UnixIO_Wait(HiddUnixIOMethodBase, __obj, fd, mode); })

static inline int __inline_Hidd_UnixIO_OpenFile(OOP_MethodID base, OOP_Object *o, const char *filename, int flags, int mode, int *errno_ptr)
{
    struct uioMsgOpenFile p;
	
    p.um_MethodID = base + moHidd_UnixIO_OpenFile;
    p.um_FileName = filename;
    p.um_Flags    = flags;
    p.um_Mode     = mode;
    p.um_ErrNoPtr = errno_ptr;

    return OOP_DoMethod(o, (OOP_MethodID *)&p.um_MethodID);
}

#define Hidd_UnixIO_OpenFile(o, filename, flags, mode, errno_ptr) \
    ({OOP_Object *__obj = o; __inline_Hidd_UnixIO_OpenFile(HiddUnixIOMethodBase, __obj, filename, flags, mode, errno_ptr); })

static inline int __inline_Hidd_UnixIO_CloseFile(OOP_MethodID base, OOP_Object *o, int fd, int *errno_ptr)
{
    struct uioMsgCloseFile p;
	
    p.um_MethodID = base + moHidd_UnixIO_CloseFile;
    p.um_FD 	  = (APTR)fd;
    p.um_ErrNoPtr = errno_ptr;

    return OOP_DoMethod(o, (OOP_MethodID *)&p.um_MethodID);
}

#define Hidd_UnixIO_CloseFile(o, fd, errno_ptr) \
    ({OOP_Object *__obj = o; __inline_Hidd_UnixIO_CloseFile(HiddUnixIOMethodBase, __obj, fd, errno_ptr); })

static inline int __inline_Hidd_UnixIO_ReadFile(OOP_MethodID base, OOP_Object *o, int fd, void *buffer, int count, int *errno_ptr)
{
    struct uioMsgReadFile p;
	
    p.um_MethodID = base + moHidd_UnixIO_ReadFile;
    p.um_FD 	  = (APTR)fd;
    p.um_Buffer   = buffer;
    p.um_Count    = count;
    p.um_ErrNoPtr = errno_ptr;

    return OOP_DoMethod(o, (OOP_MethodID *)&p.um_MethodID);
}

#define Hidd_UnixIO_ReadFile(o, fd, buffer, count, errno_ptr) \
    ({OOP_Object *__obj = o; __inline_Hidd_UnixIO_ReadFile(HiddUnixIOMethodBase, __obj, fd, buffer, count, errno_ptr); })

static inline int __inline_Hidd_UnixIO_WriteFile(OOP_MethodID base, OOP_Object *o, int fd, const void *buffer, int count, int *errno_ptr)
{
    struct uioMsgWriteFile p;
	
    p.um_MethodID = base + moHidd_UnixIO_WriteFile;
    p.um_FD 	  = (APTR)fd;
    p.um_Buffer   = buffer;
    p.um_Count    = count;
    p.um_ErrNoPtr = errno_ptr;

    return OOP_DoMethod(o, (OOP_MethodID *)&p.um_MethodID);
}

#define Hidd_UnixIO_WriteFile(o, fd, buffer, count, errno_ptr) \
    ({OOP_Object *__obj = o; __inline_Hidd_UnixIO_WriteFile(HiddUnixIOMethodBase, __obj, fd, buffer, count, errno_ptr); })

static inline int __inline_Hidd_UnixIO_IOControlFile(OOP_MethodID base, OOP_Object *o, int fd, int request, void *param, int *errno_ptr)
{
    struct uioMsgIOControlFile p;

    p.um_MethodID = base + moHidd_UnixIO_IOControlFile;
    p.um_FD 	  = (APTR)fd;
    p.um_Request  = request;
    p.um_Param    = param;
    p.um_ErrNoPtr = errno_ptr;

    return OOP_DoMethod(o, (OOP_MethodID *)&p.um_MethodID);
}

#define Hidd_UnixIO_IOControlFile(o, fd, request, param, errno_ptr) \
    ({OOP_Object *__obj = o; __inline_Hidd_UnixIO_IOControlFile(HiddUnixIOMethodBase, __obj, fd, request, param, errno_ptr); })

static inline int __inline_Hidd_UnixIO_AddInterrupt(OOP_MethodID base, OOP_Object *o, struct uioInterrupt *interrupt)
{
    struct uioMsgAddInterrupt p;

    p.um_MethodID = base + moHidd_UnixIO_AddInterrupt;
    p.um_Int	  = interrupt;

    return OOP_DoMethod(o, (OOP_MethodID *)&p.um_MethodID);
}

#define Hidd_UnixIO_AddInterrupt(o, interrupt) \
    ({OOP_Object *__obj = o; __inline_Hidd_UnixIO_AddInterrupt(HiddUnixIOMethodBase, __obj, interrupt); })

static inline void __inline_Hidd_UnixIO_RemInterrupt(OOP_MethodID base, OOP_Object *o, struct uioInterrupt *interrupt)
{
    struct uioMsgRemInterrupt p;

    p.um_MethodID = base + moHidd_UnixIO_RemInterrupt;
    p.um_Int	  = interrupt;

    OOP_DoMethod(o, (OOP_MethodID *)&p.um_MethodID);
}

#define Hidd_UnixIO_RemInterrupt(o, interrupt) \
    ({OOP_Object *__obj = o; __inline_Hidd_UnixIO_RemInterrupt(HiddUnixIOMethodBase, __obj, interrupt); })

static inline int __inline_Hidd_UnixIO_Poll(OOP_MethodID base, OOP_Object *o, int fd, int mode, int *errno_ptr)
{
    struct uioMsgPoll p;

    p.um_MethodID = base + moHidd_UnixIO_Poll;
    p.um_FD       = (APTR)fd;
    p.um_Mode     = mode;
    p.um_ErrNoPtr = errno_ptr;

    return OOP_DoMethod(o, (OOP_MethodID *)&p.um_MethodID);
}

#define Hidd_UnixIO_Poll(o, fd, mode, errno_ptr) \
    ({OOP_Object *__obj = o; __inline_Hidd_UnixIO_Poll(HiddUnixIOMethodBase, __obj, fd, mode, errno_ptr); })

static inline void * __inline_Hidd_UnixIO_MemoryMap(OOP_MethodID base, OOP_Object *o, void * addr, int len, int prot, int flags, int fd, int offset, int *errno_ptr)
{
    struct uioMsgMemoryMap p;

    p.um_MethodID = base + moHidd_UnixIO_MemoryMap;
    p.um_FD       = (APTR)fd;
    p.um_Address  = addr;
    p.um_Length   = len;
    p.um_Prot     = prot;
    p.um_Flags    = flags;
    p.um_Offset   = offset;
    p.um_ErrNoPtr = errno_ptr;

    return OOP_DoMethod(o, (OOP_MethodID *)&p.um_MethodID);
}

#define Hidd_UnixIO_MemoryMap(o, addr, len, prot, flags, fd, offset, errno_ptr) \
    ({OOP_Object *__obj = o; __inline_Hidd_UnixIO_MemoryMap(HiddUnixIOMethodBase, __obj, addr, len, prot, flags, fd, offset, errno_ptr); })

static inline int __inline_Hidd_UnixIO_MemoryUnMap(OOP_MethodID base, OOP_Object *o, void * addr, int len, int *errno_ptr)
{
    struct uioMsgMemoryUnMap p;

    p.um_MethodID = base + moHidd_UnixIO_MemoryUnMap;
    p.um_Address  = addr;
    p.um_Length   = len;
    p.um_ErrNoPtr = errno_ptr;

    return OOP_DoMethod(o, (OOP_MethodID *)&p.um_MethodID);
}

#define Hidd_UnixIO_MemoryUnMap(o, addr, len, errno_ptr) \
    ({OOP_Object *__obj = o; __inline_Hidd_UnixIO_MemoryUnMap(HiddUnixIOMethodBase, __obj, addr, len, errno_ptr); })

#endif
