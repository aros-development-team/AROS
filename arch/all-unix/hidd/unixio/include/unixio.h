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
    STACKULONG um_MethodID;
    STACKULONG um_Filedesc;
    STACKULONG um_Filedesc_Type;
    STACKULONG um_Mode;
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

struct uioMsgPoll
{
    STACKULONG  um_MethodID;
    APTR        um_FD;
    STACKULONG  um_Mode;
    int        *um_ErrNoPtr;
};

/* I/O mode flags */
#define vHidd_UnixIO_Read       0x1
#define vHidd_UnixIO_Write      0x2
#define vHidd_UnixIO_RW         (vHidd_UnixIO_Read | vHidd_UnixIO_Write)
#define vHidd_UnixIO_Error	0x10

#endif /* HIDD_UNIXIO_H */


