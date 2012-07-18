#ifndef HIDD_UNIXIO_H
#define HIDD_UNIXIO_H

/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Unix filedescriptor/socket IO Include File
    Lang: english
*/

#include <exec/libraries.h>
#include <exec/ports.h>
#include <oop/oop.h>

#define CLID_Hidd_UnixIO "unixio.hidd"
#define IID_Hidd_UnixIO	"I_Hidd_UnixIO"

/* Attrbases */
#define HiddUnixIOAttrBase      __IHidd_UnixIO

#ifndef __OOP_NOATTRBASES__
extern OOP_AttrBase HiddUnixIOAttrBase;
#endif

struct uioInterrupt
{
    struct MinNode Node;
    int		   fd;
    int		   mode;
    void	   (*handler)(int, int, void *);
    void	   *handlerData;
};

/*
 * (Semi)-public part of unixio.hidd library base.
 * Normally you don't need to access it at all. However it can be
 * useful if you want to use more specific operations on filedescriptors.
 * For example emul-handler needs this.
 */
struct UnixIOBase
{
    struct Library uio_Library;		/* Library node				  */
    APTR	   uio_LibcHandle;	/* hostlib.resource's handle to host libc */
    int		  *uio_ErrnoPtr;	/* Pointer to host's errno variable	  */
};

enum {
    moHidd_UnixIO_Wait = 0,	/* LONG M ( uioMsg *)		*/
    moHidd_UnixIO_AsyncIO,	/* Obsolete and reserved	*/
    moHidd_UnixIO_AbortAsyncIO, /* Obsolete and reserved	*/
    moHidd_UnixIO_OpenFile,
    moHidd_UnixIO_CloseFile,
    moHidd_UnixIO_WriteFile,
    moHidd_UnixIO_IOControlFile,    
    moHidd_UnixIO_ReadFile,
    moHidd_UnixIO_AddInterrupt,
    moHidd_UnixIO_RemInterrupt,
    moHidd_UnixIO_Poll,
    num_Hidd_UnixIO_Methods
};

enum
{
    aoHidd_UnixIO_Opener,	    	/* [I..] Opener name		    */
    aoHidd_UnixIO_Architecture,		/* [I..] Opener's architecture name */
    num_Hidd_UnixIO_Attrs
};

#define aHidd_UnixIO_Opener		(HiddUnixIOAttrBase + aoHidd_UnixIO_Opener)
#define aHidd_UnixIO_Architecture	(HiddUnixIOAttrBase + aoHidd_UnixIO_Architecture)

struct uioMsg
{
    STACKED ULONG um_MethodID;
    STACKED ULONG um_Filedesc;
    STACKED ULONG um_Filedesc_Type;
    STACKED ULONG um_Mode;
};

struct uioMsgOpenFile
{
    STACKED ULONG  um_MethodID;
    STACKED CONST_STRPTR um_FileName;
    STACKED ULONG  um_Flags;
    STACKED ULONG  um_Mode;
    STACKED int   *um_ErrNoPtr;
};

struct uioMsgCloseFile
{
    STACKED ULONG  um_MethodID;
    STACKED APTR   um_FD;
    STACKED int   *um_ErrNoPtr;
};

struct uioMsgWriteFile
{
    STACKED ULONG  um_MethodID;
    STACKED APTR   um_FD;
    STACKED CONST_APTR   um_Buffer;
    STACKED ULONG  um_Count;
    STACKED int   *um_ErrNoPtr;
};

struct uioMsgIOControlFile
{
    STACKED ULONG  um_MethodID;
    STACKED APTR   um_FD;
    STACKED ULONG  um_Request;
    STACKED APTR   um_Param;
    STACKED int   *um_ErrNoPtr;
};

struct uioMsgReadFile
{
    STACKED ULONG  um_MethodID;
    STACKED APTR um_FD;
    STACKED APTR um_Buffer;
    STACKED ULONG  um_Count;
    STACKED int *um_ErrNoPtr;
};

struct uioMsgAddInterrupt
{
    STACKED ULONG                um_MethodID;
    STACKED struct uioInterrupt *um_Int;
};

struct uioMsgRemInterrupt
{
    STACKED ULONG                um_MethodID;
    STACKED struct uioInterrupt *um_Int;
};

struct uioMsgPoll
{
    STACKED ULONG  um_MethodID;
    STACKED APTR   um_FD;
    STACKED ULONG  um_Mode;
    STACKED int   *um_ErrNoPtr;
};

/* I/O mode flags */
#define vHidd_UnixIO_Read       0x1
#define vHidd_UnixIO_Write      0x2
#define vHidd_UnixIO_RW         (vHidd_UnixIO_Read | vHidd_UnixIO_Write)
#define vHidd_UnixIO_Error	0x10

#endif /* HIDD_UNIXIO_H */


