/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Host OS file I/O, Windows-hosted version
    Lang: english
*/

#define DEBUG 1

#include <aros/debug.h>
#include <exec/alerts.h>
#include <exec/tasks.h>
#include <hidd/hostio.h>
#include <oop/oop.h>
#include <proto/exec.h>
#include <proto/kernel.h>
#include <proto/oop.h>
#include <fcntl.h>
#include <errno.h>

#include LC_LIBDEFS_FILE

#include "hostio.h"

#define DERROR(x) x

/************************************************************************/

static ULONG errtab[][2]=
{
  { ERROR_PATH_NOT_FOUND	, ENOENT    },
  { ERROR_ACCESS_DENIED		, EACCES    },
  { ERROR_NO_MORE_FILES		, ENOENT    },
  { ERROR_NOT_ENOUGH_MEMORY	, ENOMEM    },
  { ERROR_FILE_NOT_FOUND	, ENOENT    },
  { ERROR_FILE_EXISTS		, EEXIST    },
  { ERROR_WRITE_PROTECT		, EACCES    },
  { WIN32_ERROR_DISK_FULL	, ENOSPC    },
  { ERROR_DIR_NOT_EMPTY		, ENOTEMPTY },
  { ERROR_SHARING_VIOLATION	, EBUSY     },
  { ERROR_LOCK_VIOLATION	, EBUSY     },
  { ERROR_BUFFER_OVERFLOW	, ENOBUFS   },
  { ERROR_INVALID_NAME		, EINVAL    },
  { ERROR_INVALID_PARAMETER	, EINVAL    },
  { ERROR_IO_PENDING		, EAGAIN    },
  { ERROR_HANDLE_EOF		, 0	    },
  { 0				, 0	    }
};

ULONG Errno_win2std(ULONG e)
{
    ULONG i;
  
    DERROR(bug("[HostIO] Windows error code: %lu\n", e));
    for (i=0; i < sizeof(errtab)/sizeof(errtab[0]); i++) {
	if(errtab[i][0] == e) {
	    DERROR(bug("[HostIO] Translated to standard error code: %lu\n", errtab[i][1]));
	    return errtab[i][1];
	}
    }
    DERROR(bug("[HostIO] Unknown error code\n"));
    return -1;
}

#define SetError(err)				\
if (msg->hm_RawErrNoPtr)			\
    *msg->hm_RawErrNoPtr = err;			\
if (msg->hm_ErrNoPtr)				\
    *msg->hm_ErrNoPtr = Errno_win2std(err)

/************************************************************************/

void IoIntHandler(struct File_Handle *fh, struct Task *task)
{
    if (fh->io.Internal != STATUS_PENDING)
        Signal(task, SIGF_SINGLE);
}

/************************************************************************/

/********************
**  HostIO::New()  **
********************/
OOP_Object *HIO__Root__New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    EnterFunc(bug("HostIO::New(cl=%s)\n", cl->ClassNode.ln_Name));

    o =(OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);

    if (o)
    {
	struct HostIOData *id;
	ULONG dispose_mid;
	
	id = OOP_INST_DATA(cl, o);
	D(bug("inst: %p, o: %p\n", id, o));

	id->irq = KrnAllocIRQ();
	D(bug("[HostIO] Allocated IRQ %d\n", id->irq));
	if (id->irq != -1) {
	    id->irqobj = KrnGetIRQObject(id->irq);
	    ReturnPtr("HostIO::New", OOP_Object *, o);
	}

	dispose_mid = OOP_GetMethodID(IID_Root, moRoot_Dispose);
	OOP_CoerceMethod(cl, o, (OOP_Msg)&dispose_mid);
    }
    ReturnPtr("HostIO::New", OOP_Object *, NULL);
}

/***********************
**  HostIO::Dispose()  **
***********************/
IPTR HIO__Root__Dispose(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    struct HostIOData *id = OOP_INST_DATA(cl, o);

    if (id->irq != -1)
	KrnFreeIRQ(id->irq);

    return OOP_DoSuperMethod(cl, o, msg);
}

/*****************************************************************************************

    NAME
        moHidd_HostIO_Wait

    SYNOPSIS
        OOP_DoMethod(OOP_Object *obj, struct hioMsg *msg);

        IPTR Hidd_HostIO_Wait (OOP_Object *obj, APTR fd, APTR callback, APTR callbackdata, int *errno_ptr, int *raw_errno_ptr);

    LOCATION
        hostio.hidd

    FUNCTION
        Wait for asynchronous I/O to complete.

	The user may supply a callback function which will be called when the operation
	completes. The function is called using C calling convention and should be
	declared as:

	  int callback(APTR fh, APTR data);

	Parameters of the callback are:
	  fh   - A HostIO file handle.
	  data - User-defined data specified for the method.

	The callback is expected to return nonzero value in order to terminate the
	operation and zero in order to continue it. For example, this can be used in
	order to read data in portions until some condition is met. Note that it's
	up to callback function to request another operation on the handle before
	returning zero, otherwise the process will lock up.

    INPUTS
        obj           - An object to operate on
        fd            - A file descriptor to operate on
	callback      - An optional callback which will be called when the operation
		        completes
	callbackdata  - User-defined data which will be passed to a callback
	errno_ptr     - An optional pointer to a location where standard error code will
			be written
	raw_errno_ptr - An optional pointer to a location where host-specific error code
			will be written

    RESULT
	A number of bytes succesfully transferred during last operation or -1 in case of
	error. 0 means end of file.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    TODO

*****************************************************************************************/

IPTR HIO__Hidd_HostIO__Wait(OOP_Class *cl, OOP_Object *o, struct hioMsg *msg)
{
    LONG retval = -1;
    struct HostIOData *id = OOP_INST_DATA(cl, o);
    struct File_Handle *fh = msg->hm_FD;
    ULONG res;
    ULONG err;
    int done;
    struct Task *me = FindTask(NULL);
    void *ih = KrnAddIRQHandler(id->irq, IoIntHandler, msg->hm_FD, me);
    
    do
    {
	/* This comparison is what HasOverlappedIoCompleted() does */
	while (fh->io.Internal == STATUS_PENDING)
	{
	    if (!ih) {
		SetError(ERROR_NOT_ENOUGH_MEMORY);
		return -1;
            }

            SetSignal(0, SIGF_SINGLE);
            Wait(SIGF_SINGLE);
	}

	Forbid();
	res = GetOverlappedResult(fh->handle, &fh->io, &retval, FALSE);
	err = GetLastError();
	Permit();

	/* First check for EOF and reset error condition in the case */
	if (!res)
	{
	    if (err == ERROR_HANDLE_EOF)
	        res = TRUE;
	    else
	        retval = -1;
	}
	done = 1;

	/* If we succeeded, we may have a callback. If the callback returns 0, we should
	   continue looping. It's assumed that the callback has requested another operation. */
	if (res)
	{
	    int (*cb)(void *, void *) = msg->hm_CallBack;

	    if (cb)
		done = cb(fh, msg->hm_CallBackData);
	}
    } while (!done);

    if (ih)
	KrnRemIRQHandler(ih);

    SetError(err);
    return retval;
}

/************************
**  HostIO::AsyncIO()  **
************************/
IPTR HIO__Hidd_HostIO__AsyncIO(OOP_Class *cl, OOP_Object *o, struct hioMsgAsyncIO *msg)
{
    IPTR retval = 0UL;
#if 0 /* TODO */
    struct hioMessage * umsg = AllocMem (sizeof (struct hioMessage), MEMF_CLEAR|MEMF_PUBLIC);
    struct MsgPort  * port = msg->hm_ReplyPort;
    struct hio_data *hd = HD(cl);

    if (umsg)
    {
    	/* nlorentz: What action should be taken on reply of this message
	   should be the choice of the caller. (The caller might want
	   a signal instead of a softint)
	   
	port->mp_Flags   = PA_SOFTINT;
	
	*/

	umsg->Message.mn_ReplyPort = port;
	umsg->fd      = msg->hm_Filedesc;
	umsg->fd_type = msg->hm_Filedesc_Type;
	umsg->mode    = msg->hm_Mode;
	umsg->callback = NULL;
	umsg->callbackdata = NULL;

	D(bug("Sending msg fd=%ld mode=%ld to port %x\n", umsg->fd, umsg->mode, hd->hd_Port));

        /*
        ** Just send the message and leave
        ** When the message arrives on the port the user must free
        ** the message!
        */
	PutMsg (hd->hd_Port, (struct Message *)umsg);

    }
    else
	retval = ENOMEM;
#endif
    return retval;
}


/*****************************
**  HostIO::AbortAsyncIO()  **
*****************************/
VOID HIO__Hidd_HostIO__AbortAsyncIO(OOP_Class *cl, OOP_Object *o, struct hioMsgAbortAsyncIO *msg)
{
/* TODO
    struct hioMessage * umsg = AllocMem (sizeof (struct hioMessage), MEMF_CLEAR|MEMF_PUBLIC);
    struct hio_data *hd = HD(cl);
    struct MsgPort  * port = CreateMsgPort();

    if (umsg  && port)
    {
	umsg->Message.mn_ReplyPort = port;
	umsg->fd   = ((struct hioMsg *)msg)->hm_Filedesc;
	umsg->mode = vHidd_HostIO_Abort;

	PutMsg (hd->hd_Port, (struct Message *)umsg);

	WaitPort (port);
	GetMsg (port);

    }
    
    if (umsg)
	FreeMem (umsg, sizeof (struct hioMessage));

    if (port)
        DeleteMsgPort(port);
*/
}

/*****************************************************************************************

    NAME
        moHidd_HostIO_OpenFile

    SYNOPSIS
        OOP_DoMethod(OOP_Object *obj, struct hioMsgOpenFile *msg);

        APTR Hidd_HostIO_OpenFile (OOP_Object *obj, const char *filename, int flags, int mode, int *errno_ptr, int *raw_errno_ptr);

    LOCATION
        hostio.hidd

    FUNCTION
        Open a file on host OS side.

    INPUTS
        obj           - An object to operate on
        filename      - File name to open. File name should meet host OS conventions.
	flags         - Flags specifying open mode. These are the same flags as for
			open() C function.
	errno_ptr     - An optional pointer to a location where standard error code will
			be written
	raw_errno_ptr - An optional pointer to a location where host-specific error code
			will be written

    RESULT
	An opaque file handle or vHidd_HostIO_Invalid_Handle in case of error.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	moHidd_HostIO_CloseFile

    INTERNALS

    TODO

*****************************************************************************************/

APTR HIO__Hidd_HostIO__OpenFile(OOP_Class *cl, OOP_Object *o, struct hioMsgOpenFile *msg)
{
    struct HostIOData *id = OOP_INST_DATA(cl, o);
    struct File_Handle *fh = AllocMem(sizeof(struct File_Handle), MEMF_ANY|MEMF_CLEAR);

    D(bug("[HostIO] OpenFile(\"%s\", 0x%04lX, %o)\n", msg->hm_FileName, msg->hm_Flags, msg->hm_Mode));
    if (fh) {
        ULONG access = 0;
        ULONG share = FILE_SHARE_READ;
        ULONG create = OPEN_EXISTING;
        ULONG flags = 0;
	ULONG error;

	if (msg->hm_Flags & O_RDONLY)
	    access = GENERIC_READ;
	if (msg->hm_Flags & O_WRONLY) {
	    access |= GENERIC_WRITE;
	    share = 0;
	}

	if (msg->hm_Flags & O_CREAT)
	    create = (msg->hm_Flags & O_EXCL) ? CREATE_NEW : OPEN_ALWAYS;
	else if (msg->hm_Flags & O_TRUNC)
	    create = CREATE_ALWAYS;

	if (msg->hm_Flags & O_DSYNC)
	    flags = FILE_FLAG_WRITE_THROUGH;
	if (msg->hm_Flags & O_NONBLOCK)
	    flags |= FILE_FLAG_OVERLAPPED;
	if (!msg->hm_Mode & S_IWUSR)
	    flags |= FILE_ATTRIBUTE_READONLY;
	/* TODO: process O_APPEND and set file pointer in OVERLAPPED structure */

	D(bug("[HostIO] Translated access: 0x%08lX, share: 0x%08lX, create: %u, flags: 0x%08lX\n", access, share, create, flags));
	Forbid();
	fh->handle = CreateFile(msg->hm_FileName, access, share, NULL, create, flags, NULL);
	error = GetLastError();
	Permit();
	SetError(error);

	if (fh->handle == INVALID_HANDLE_VALUE) {
	    D(bug("[HostIO] Error opening file\n"));
	    FreeMem(fh, sizeof(struct File_Handle));
	    fh = vHidd_HostIO_Invalid_Handle;
	} else
	    fh->io.hEvent = id->irqobj;
    }
    else
    {
        fh = vHidd_HostIO_Invalid_Handle;
	SetError(ERROR_NOT_ENOUGH_MEMORY);
    }
    return fh;
}

/*****************************************************************************************

    NAME
        moHidd_HostIO_CloneHandle

    SYNOPSIS
        OOP_DoMethod(OOP_Object *obj, struct hioMsgCloneHandle *msg);

        APTR Hidd_HostIO_CloneHandle (OOP_Object *obj, APTR fd, int *errno_ptr, int *raw_errno_ptr);

    LOCATION
        hostio.hidd

    FUNCTION
        Clone a host file handle. This may be needed if the user wants to run more than
	one operation on an object concurrently.

    INPUTS
        obj           - An object to operate on.
        fd            - An original handle to clone.
	errno_ptr     - An optional pointer to a location where standard error code will
			be written
	raw_errno_ptr - An optional pointer to a location where host-specific error code
			will be written

    RESULT
	A duplicated file handle or vHidd_HostIO_Invalid_Handle in case of error.

    NOTES
	All cloned handles internally refer to the same object. If you close the original
	handle, underlying object will also be closed and all cloned handles
	automatically become inoperative. You won't be able to perform any operations on
	them, however you will still need to close them.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    TODO

*****************************************************************************************/

APTR HIO__Hidd_HostIO__CloneHandle(OOP_Class *cl, OOP_Object *o, struct hioMsgCloneHandle *msg)
{
    struct HostIOData *id = OOP_INST_DATA(cl, o);
    struct File_Handle *fh = msg->hm_FD;
    ULONG err = 0;
    struct File_Handle *fh2 = AllocMem(sizeof(struct File_Handle), MEMF_ANY);

    if (fh2) {
        CopyMem(fh, fh2, sizeof(struct File_Handle));
	fh2->io.hEvent = id->irqobj; /* This allows you to clone handles created by other units */
	fh2->flags |= HANDLE_CLONED;
    } else {
        fh2 = vHidd_HostIO_Invalid_Handle;
	err = ERROR_NOT_ENOUGH_MEMORY;
    }
    SetError(err);
    return fh2;
}

/*****************************************************************************************

    NAME
        moHidd_HostIO_GetRawHandle

    SYNOPSIS
        OOP_DoMethod(OOP_Object *obj, struct hioMsgGetRawHandle *msg);

        APTR Hidd_HostIO_GetRawHandle (OOP_Object *obj, APTR fd);

    LOCATION
        hostio.hidd

    FUNCTION
        Extract raw host OS file object handle.

    INPUTS
        obj - An object to operate on.
        fd  - A hostio.hidd file handle from which you want to extract
	      the host OS handle.

    RESULT
	Raw untranslated host OS handle of the object.

    NOTES
	The raw handle will be closed when the user invokes moHidd_HostIO_CloseFile on the
	object. Do not use it after this.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    TODO

*****************************************************************************************/

APTR HIO__Hidd_HostIO__GetRawHandle(OOP_Class *cl, OOP_Object *o, struct hioMsgGetRawHandle *msg)
{
    struct File_Handle *fh = msg->hm_FD;

    return fh->handle;
}

/*****************************************************************************************

    NAME
        moHidd_HostIO_CloseFile

    SYNOPSIS
        OOP_DoMethod(OOP_Object *obj, struct hioMsgCloseFile *msg);

        void Hidd_HostIO_CloseFile (OOP_Object *obj, APTR fd, int *errno_ptr, int *raw_errno_ptr);

    LOCATION
        hostio.hidd

    FUNCTION
        Close the host-side file.

    INPUTS
        obj	     - An object to operate on.
        fd            - A file handle to close.
	errno_ptr     - An optional pointer to a location where standard error code will
			be written
	raw_errno_ptr - An optional pointer to a location where host-specific error code
			will be written

    RESULT
	None.

    NOTES
	Closing cloned handles does not actually close underlying host OS object. It
	will be closed only when you close the primary handle obtained using
	moHidd_HostIO_OpenFile.

	Despite there's no return code, error code still can be set, depending on the
	host OS.

    EXAMPLE

    BUGS

    SEE ALSO
	moHidd_HostIO_OpenFile

    INTERNALS

    TODO

*****************************************************************************************/

VOID HIO__Hidd_HostIO__CloseFile(OOP_Class *cl, OOP_Object *o, struct hioMsgCloseFile *msg)
{
    struct File_Handle *fh = msg->hm_FD;
    ULONG error = 0;
    
    if (fh != vHidd_HostIO_Invalid_Handle)
    {
	if (!(fh->flags & HANDLE_CLONED)) {
	    Forbid();
	    CloseHandle(fh->handle);
	    error = GetLastError();
	    Permit();
	}
	FreeMem(fh, sizeof(struct File_Handle));
    }
    SetError(error);
}

/*****************************
**  HostIO::ReadFile()     **
*****************************/
IPTR HIO__Hidd_HostIO__ReadFile(OOP_Class *cl, OOP_Object *o, struct hioMsgReadFile *msg)
{
    struct File_Handle *fh = msg->hm_FD;
    LONG retval = -1;
    ULONG err = 0;
    
    if (fh != vHidd_HostIO_Invalid_Handle)
    {
	ULONG res;

	Forbid();
	res = ReadFile(fh->handle, msg->hm_Buffer, msg->hm_Count, &retval, &fh->io);
	err = GetLastError();
	Permit();
	D(bug("[HostIO] Read result: %d, bytes count: %d\n", res, retval));
	if (!res) {
	    /* EOF is an error in Windows but not an error in libc */
	    if (err != ERROR_HANDLE_EOF)
	        retval = -1;
	}
    }
    SetError(err);    
    return retval;
}

/*****************************
**  HostIO::WriteFile()     **
*****************************/
IPTR HIO__Hidd_HostIO__WriteFile(OOP_Class *cl, OOP_Object *o, struct hioMsgWriteFile *msg)
{
    struct File_Handle *fh = msg->hm_FD;
    LONG retval = -1;
    ULONG err = 0;
    
    if (fh != vHidd_HostIO_Invalid_Handle)
    {
	ULONG res;

	Forbid();
	res = WriteFile(fh->handle, msg->hm_Buffer, msg->hm_Count, &retval, &fh->io);
	err = GetLastError();
	Permit();
	if (!res)
	    retval = -1;
    }
    SetError(err);    
    return retval;
}

/*****************************
**  HostIO::IOControlFile() **
*****************************/
IPTR HIO__Hidd_HostIO__IOControlFile(OOP_Class *cl, OOP_Object *o, struct hioMsgIOControlFile *msg)
{
    struct File_Handle *fh = msg->hm_FD;
    LONG retval = -1;
    ULONG err = 0;

    if (fh != vHidd_HostIO_Invalid_Handle)
    {
	ULONG res;

	Forbid();
    	res = DeviceIoControl(fh->handle, msg->hm_Request, msg->hm_Param, msg->hm_ParamLen, msg->hm_Output, msg->hm_OutputLen, &retval, &fh->io);
	err = GetLastError();
	Permit();
	if (!res)
	    retval = -1;
    }
    SetError(err);
    return retval;
}
