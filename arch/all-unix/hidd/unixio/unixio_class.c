/*
    Copyright � 1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Unix filedescriptor/socket IO
    Lang: english
*/

/* Unix includes */
#define timeval sys_timeval /* We don't want the unix timeval to interfere with the AROS one */
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#undef timeval

#include <exec/types.h>
#include <exec/lists.h>
#include <exec/interrupts.h>
#include <exec/execbase.h>
#include <exec/semaphores.h>
#include <exec/memory.h>
#include <exec/resident.h>
#include <exec/tasks.h>
#include <exec/ports.h>
#include <exec/nodes.h>
#include <intuition/intuition.h>
#include <utility/tagitem.h>
#include <utility/hooks.h>
#include <hidd/unixio.h>
#include <aros/asmcall.h>
#include <aros/symbolsets.h>

#include <oop/oop.h>
#include <proto/exec.h>
#include <proto/hostlib.h>
#include <proto/intuition.h>
#include <proto/kernel.h>
#include <proto/oop.h>
#include <proto/utility.h>
#include <proto/alib.h>

#include <devices/timer.h>

#include "unixio.h"

#include LC_LIBDEFS_FILE

#include <aros/asmcall.h>

#define SDEBUG 0
#define DEBUG 0
#include <aros/debug.h>

/************************************************************************/

static void SigIO_IntServer(struct uio_data *ud, void *unused)
{
    struct uioInterrupt *intnode;

    /* Walk through the list of installed handlers and de-multiplex our SIGIO */
    for (intnode = (struct uioInterrupt *)ud->intList.mlh_Head; intnode->Node.mln_Succ;
    	 intnode = (struct uioInterrupt *)intnode->Node.mln_Succ)
    {
    	struct pollfd pfd = {intnode->fd, 0, 0};

	if (intnode->mode & vHidd_UnixIO_Read)
	    pfd.events |= POLLIN;
	if (intnode->mode & vHidd_UnixIO_Write)
	    pfd.events |= POLLOUT;

	if (ud->SysIFace->poll(&pfd, 1, 0) > 0)
	{
	    int mode = 0;

	    if (pfd.revents & POLLIN)
	    	mode |= vHidd_UnixIO_Read;
	    if (pfd.revents & POLLOUT)
	    	mode |= vHidd_UnixIO_Write;
	    if (pfd.revents & (POLLERR|POLLHUP))
	    	mode |= vHidd_UnixIO_Error;

	    D(bug("[UnixIO] Events 0x%02X for fd %d\n", mode, intnode->fd));

	    intnode->handler(intnode->fd, mode, intnode->handlerData);
	}
    }
}

static void WaitIntHandler(int fd, int mode, struct UnixIO_Waiter *w)
{
    Signal(w->task, 1 << w->signal);
}

static BOOL CheckArch(APTR KernelBase, STRPTR Component, STRPTR MyArch)
{
    STRPTR arg[3] = {Component, MyArch, NULL};

    arg[2] = (STRPTR)KrnGetSystemAttr(KATTR_Architecture);
    D(bug("[UnixIO] My architecture: %s, kernel architecture: %s\n", arg[1], arg[2]));
    if (!arg[2])
	return FALSE;

    if (strcmp(arg[1], arg[2]))
    {
	struct IntuitionBase *IntuitionBase;

	IntuitionBase = (struct IntuitionBase *)OpenLibrary("intuition.library", 36);
	if (IntuitionBase)
	{
            struct EasyStruct es = {
        	sizeof (struct EasyStruct),
        	0,
        	"Incompatible architecture",
		"Used version of %s is built for use\n"
		"with %s architecture, but your\n"
		"system architecture is %s.",
        	"Ok",
	    };

	    EasyRequestArgs(NULL, &es, NULL, (IPTR *)arg);

	    CloseLibrary(&IntuitionBase->LibNode);
	}
	return FALSE;
    }

    D(bug("[UnixIO] Architecture check done\n"));
    return TRUE;
}

#define KernelBase  LIBBASE->KernelBase
#define HostLibBase LIBBASE->HostLibBase

/* The following are methods of the UnixIO HIDD class. */

/********************
**  UnixIO::New()  **
********************/
OOP_Object *UXIO__Root__New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    struct uio_data *data = UD(cl);
    EnterFunc(bug("UnixIO::New(cl=%s)\n", cl->ClassNode.ln_Name));

    /* We are a true singletone */
    ObtainSemaphore(&data->sem);

    if (!data->obj)
    {
    	D(bug("[UnixIO] Creating object\n"));
    	data->obj = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    }

    ReleaseSemaphore(&data->sem);

    ReturnPtr("UnixIO::New", OOP_Object *, data->obj);
}

/***********************
**  UnixIO::Dispose()  **
***********************/
void UXIO__Root__Dispose(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    /*
     * Do nothing here. 
     * We can't just omit this method because in this case Dispose() will be called
     * on our superclass, which is not what we want.
     */
}

/*****************************************************************************************

    NAME
        moHidd_UnixIO_Wait

    SYNOPSIS
        OOP_DoMethod(OOP_Object *obj, struct uioMsg *msg);

        IPTR Hidd_UnixIO_Wait(OOP_Object *obj, ULONG fd, ULONG mode);

    LOCATION
        unixio.hidd

    FUNCTION
        Wait for an event on the file descriptor.

    INPUTS
        obj          - A pointer to a UnixIO object
        fd           - A file descriptor to wait on
        mode	     - A combination of two flags:
        		- vHidd_UnixIO_Read  - to request waiting until read is permitted
        		- vHidd_UnixIO_Write - to request waiting until write is permitted

    RESULT
    	0 in case of success or UNIX errno value in case if exception happens on the
    	socket (select() call sets corresponding flag in third fd_set).

    NOTES

    EXAMPLE

    BUGS
    	Callback routine is called only for read events (if they were requested)

    SEE ALSO

    INTERNALS

    TODO

*****************************************************************************************/
IPTR UXIO__Hidd_UnixIO__Wait(OOP_Class *cl, OOP_Object *o, struct uioMsg *msg)
{
    IPTR retval = 0UL;
    struct uioInterrupt myInt;
    struct UnixIO_Waiter w;

    w.signal = AllocSignal(-1);
    if (w.signal == -1)
    	return ENOMEM;

    w.task = FindTask(NULL);

    myInt.fd   = msg->um_Filedesc;
    myInt.mode = msg->um_Mode;
    myInt.handler = WaitIntHandler;
    myInt.handlerData = &w;

    D(bug("[UnixIO] Adding interrupt 0x%P\n", &myInt));

    retval = Hidd_UnixIO_AddInterrupt(o, &myInt);
    D(bug("[UnixIO] Result: %d\n", retval));

    if (!retval)
    {
    	D(bug("[UnixIO] Waiting for signal...\n"));
    	Wait(1 << w.signal);
    	Hidd_UnixIO_RemInterrupt(o, &myInt);
    }

    FreeSignal(w.signal);

    return retval;
}

/************************
**  UnixIO::AsyncIO()  **
************************/
IPTR UXIO__Hidd_UnixIO__AsyncIO(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    /* Obsolete and not implemented */
    return ENOSYS;
}

/*****************************************************************************************

    NAME
        moHidd_UnixIO_OpenFile

    SYNOPSIS
        OOP_DoMethod(OOP_Object *obj, struct uioMsgOpenFile *msg);

        int Hidd_UnixIO_OpenFile (OOP_Object *obj, const char *filename, int flags, int mode, int *errno_ptr);

    LOCATION
        unixio.hidd

    FUNCTION
        Open a UNIX file descriptor

    INPUTS
        obj       - An pointer to a UnixIO object
        filename  - File name to open. File name should meet host OS conventions.
	flags     - Flags specifying open mode. These are the same flags as for
		    open() C function. Note that this value is passed directly to
		    the host OS, and its definition can differ from AROS one.
	errno_ptr - An optional pointer to a location where error code (value of
		    UNIX errno variable) will be written

    RESULT
	A number of the opened file descriptor or -1 for an error. 

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	moHidd_UnixIO_CloseFile

    INTERNALS

    TODO

*****************************************************************************************/
APTR UXIO__Hidd_UnixIO__OpenFile(OOP_Class *cl, OOP_Object *o, struct uioMsgOpenFile *msg)
{
    struct uio_data *data = UD(cl);
    APTR retval;

    D(bug("[UnixIO] OpenFile(%s, 0x%04X, %o)\n", msg->um_FileName, msg->um_Flags, msg->um_Mode));

    ObtainSemaphore(&data->sem);

    retval = (APTR)data->SysIFace->open(msg->um_FileName, (int)msg->um_Flags, (int)msg->um_Mode);
    AROS_HOST_BARRIER

    if (msg->um_ErrNoPtr)
    	*msg->um_ErrNoPtr = *data->errnoPtr;

    ReleaseSemaphore(&data->sem);

    D(bug("[UnixIO] FD is %d, errno is %d\n", retval, *data->errnoPtr));

    return retval;
}

/*****************************************************************************************

    NAME
        moHidd_UnixIO_CloseFile

    SYNOPSIS
        OOP_DoMethod(OOP_Object *obj, struct uioMsgCloseFile *msg);

        int Hidd_UnixIO_CloseFile (OOP_Object *obj, int fd, int *errno_ptr);

    LOCATION
        unixio.hidd

    FUNCTION
        Close a UNIX file descriptor.

    INPUTS
        obj	  - A pointer to a UnixIO object.
        fd        - A file descriptor to close.
	errno_ptr - An optional pointer to a location where error code (a value of UNIX
		    errno variable) will be written.

    RESULT
	0 in case of success and -1 on failure.

    NOTES
	Despite there's no return value, error code still can be set.

    EXAMPLE

    BUGS

    SEE ALSO
	moHidd_UnixIO_OpenFile

    INTERNALS

    TODO

*****************************************************************************************/
int UXIO__Hidd_UnixIO__CloseFile(OOP_Class *cl, OOP_Object *o, struct uioMsgCloseFile *msg)
{
    struct uio_data *data = UD(cl);
    int ret = 0;

    if (msg->um_FD != (APTR)-1)
    {
    	ObtainSemaphore(&data->sem);

    	ret = data->SysIFace->close((int)msg->um_FD);
    	AROS_HOST_BARRIER

    	if (msg->um_ErrNoPtr)
    	    *msg->um_ErrNoPtr = *data->errnoPtr;

    	ReleaseSemaphore(&data->sem);
    }

    return ret;
}

/*****************************************************************************************

    NAME
        moHidd_UnixIO_ReadFile

    SYNOPSIS
        OOP_DoMethod(OOP_Object *obj, struct uioMsgReadFile *msg);

        int Hidd_UnixIO_ReadFile(OOP_Object *obj, int fd, void *buffer, int count, int *errno_ptr);

    LOCATION
        unixio.hidd

    FUNCTION
        Read data from a UNIX file descriptor.

    INPUTS
        obj	  - A pointer to a UnixIO object.
        fd        - A file descriptor to read from.
        buffer    - A pointer to a buffer for data.
        count     - Number of bytes to read.
	errno_ptr - An optional pointer to a location where error code (a value of UNIX
		    errno variable) will be written.

    RESULT
	Number of bytes actually read or -1 if error happened.

    NOTES
	If there's no errno pointer supplied read operation will be automatically repeated if one
	of EINTR or EAGAIN error happens. If you supplied valid own errno_ptr you should be ready
	to handle these conditions yourself.

    EXAMPLE

    BUGS

    SEE ALSO
	moHidd_UnixIO_WriteFile

    INTERNALS

    TODO

*****************************************************************************************/
IPTR UXIO__Hidd_UnixIO__ReadFile(OOP_Class *cl, OOP_Object *o, struct uioMsgReadFile *msg)
{
    struct uio_data *data = UD(cl);
    int retval = -1;
    volatile int err;

    if (msg->um_FD != (APTR)-1)
    {
    	ObtainSemaphore(&data->sem);

    	do
	{
    	    retval = data->SysIFace->read((int)msg->um_FD, (void *)msg->um_Buffer, (size_t)msg->um_Count);
    	    AROS_HOST_BARRIER

    	    err = *data->errnoPtr;
    	    D(kprintf(" UXIO__Hidd_UnixIO__ReadFile: retval %d errno %d  buff %x  count %d\n", retval, err, msg->um_Buffer, msg->um_Count));

    	    if (msg->um_ErrNoPtr)
    	    	break;

	} while((err == EINTR) || (err == EAGAIN));

	ReleaseSemaphore(&data->sem);
    }

    if (msg->um_ErrNoPtr)
    	*msg->um_ErrNoPtr = err;
    
    D(if (retval == -1) kprintf("UXIO__Hidd_UnixIO__ReadFile: errno %d  buff %x  count %d\n", err, msg->um_Buffer, msg->um_Count));
    
    return retval;
}

/*****************************************************************************************

    NAME
        moHidd_UnixIO_WriteFile

    SYNOPSIS
        OOP_DoMethod(OOP_Object *obj, struct uioMsgWriteFile *msg);

        int Hidd_UnixIO_WriteFile(OOP_Object *obj, int fd, void *buffer, int count, int *errno_ptr);

    LOCATION
        unixio.hidd

    FUNCTION
        Write data to a UNIX file descriptor.

    INPUTS
        obj	  - A pointer to a UnixIO object.
        fd        - A file descriptor to write to.
        buffer    - A pointer to a buffer containing data.
        count     - Number of bytes to write.
	errno_ptr - An optional pointer to a location where error code (a value of UNIX
		    errno variable) will be written.

    RESULT
	Number of bytes actually written or -1 if error happened.

    NOTES
	If there's no errno pointer supplied read operation will be automatically repeated if one
	of EINTR or EAGAIN error happens. If you supplied valid own errno_ptr you should be ready
	to handle these conditions yourself.

    EXAMPLE

    BUGS

    SEE ALSO
	moHidd_UnixIO_ReadFile

    INTERNALS

    TODO

*****************************************************************************************/
IPTR UXIO__Hidd_UnixIO__WriteFile(OOP_Class *cl, OOP_Object *o, struct uioMsgWriteFile *msg)
{
    struct uio_data *data = UD(cl);
    int retval = -1;
    volatile int err;

    if (msg->um_FD != (APTR)-1)
    {
	ObtainSemaphore(&data->sem);

    	do
	{
    	    retval = data->SysIFace->write((int)msg->um_FD, (const void *)msg->um_Buffer, (size_t)msg->um_Count);
    	    AROS_HOST_BARRIER

	    err = *data->errnoPtr;
    	    D(kprintf(" UXIO__Hidd_UnixIO__WriteFile: retval %d errno %d  buff %x  count %d\n", retval, err, msg->um_Buffer, msg->um_Count));

    	    if (msg->um_ErrNoPtr)
    	    	break;

	} while((retval < 1) && ((err == EINTR) || (err == EAGAIN) || (err == 0)));
	
	ReleaseSemaphore(&data->sem);
    }

    if (msg->um_ErrNoPtr)
	*msg->um_ErrNoPtr = err;
    
    D(if (retval == -1) kprintf("UXIO__Hidd_UnixIO__WriteFile: errno %d  buff %x  count %d\n", err, msg->um_Buffer, msg->um_Count));

    return retval;
}

/*****************************************************************************************

    NAME
        moHidd_UnixIO_ReadFile

    SYNOPSIS
        OOP_DoMethod(OOP_Object *obj, struct uioMsgIOControlFile *msg);

        int Hidd_UnixIO_IOControlFile(OOP_Object *obj, int fd, int request, void *param, int *errno_ptr);

    LOCATION
        unixio.hidd

    FUNCTION
        Perform a special operation (ioctl) on a UNIX file descriptor.

    INPUTS
        obj	  - A pointer to a UnixIO object.
        fd        - A file descriptor to operate on.
        request   - A device-specific operation code.
        param     - A pointer to a request-specific parameter block.
	errno_ptr - An optional pointer to a location where error code (a value of UNIX
		    errno variable) will be written.

    RESULT
	Operation-specific value (actually a return value of ioctl() function called).

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    TODO

*****************************************************************************************/
IPTR UXIO__Hidd_UnixIO__IOControlFile(OOP_Class *cl, OOP_Object *o, struct uioMsgIOControlFile *msg)
{
    struct uio_data *data = UD(cl);
    int retval = -1;

    if (msg->um_FD != (APTR)-1)
    {
    	ObtainSemaphore(&data->sem);

    	retval = data->SysIFace->ioctl((int)msg->um_FD, (int)msg->um_Request, msg->um_Param);
    	AROS_HOST_BARRIER

    	if (msg->um_ErrNoPtr)
	    *msg->um_ErrNoPtr = *data->errnoPtr;

	ReleaseSemaphore(&data->sem);
    }

    return retval;
}

/*****************************************************************************************

    NAME
        moHidd_UnixIO_AddInterrupt

    SYNOPSIS
        OOP_DoMethod(OOP_Object *obj, struct uioMsgAddInterrupt *msg);

        int Hidd_UnixIO_AddInterrupt(OOP_Object *obj, struct uioInterrupt *interrupt);

    LOCATION
        unixio.hidd

    FUNCTION
        Install a filedescriptor-specific event interrupt handler

    INPUTS
        obj       - An pointer to a UnixIO object
        interrupt - A pointer to an interrupt descriptor structure initialized as follows:
        	      fd          - Number of file descriptor to watch
        	      mode        - one or more of mode flags
        	      handler     - A pointer to a handler routine.
        	      handlerData - User-specified data for the interrupt handler

        	    The interrupt handler routine will be called using C calling convention:

        	    void handler(int fd, int mode, void *data)

        	    where:
        	      fd   - File descriptor number
        	      mode - Flags reflecting set of occured events
        	      data - User data (specified in handlerData member of uioInterrupt structure)

    RESULT
	Zero if interrupt was succesfully installed and UNIX errno value if there was en error
	during setting up the filedescriptor.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	moHidd_UnixIO_RemInterrupt

    INTERNALS

    TODO

*****************************************************************************************/
int UXIO__Hidd_UnixIO__AddInterrupt(OOP_Class *cl, OOP_Object *o, struct uioMsgAddInterrupt *msg)
{
    struct uio_data *data = UD(cl);
    int res;
    int err;

    Disable();
    AddTail((struct List *)&data->intList, (struct Node *)msg->um_Int);
    Enable();

    /* Now own the filedescriptor and enable SIGIO on it */
    ObtainSemaphore(&data->sem);

    res = data->SysIFace->fcntl(msg->um_Int->fd, F_SETOWN, data->aros_PID);
    AROS_HOST_BARRIER

    if (res != -1)
    {
	res = data->SysIFace->fcntl(msg->um_Int->fd, F_GETFL);
	AROS_HOST_BARRIER
	res = data->SysIFace->fcntl(msg->um_Int->fd, F_SETFL, res|O_ASYNC);
	AROS_HOST_BARRIER
    }
    err = *data->errnoPtr;

    ReleaseSemaphore(&data->sem);

    if (res != -1)
    	return 0;

    /* Remove the interrupt if something went wrong */
    Hidd_UnixIO_RemInterrupt(o, msg->um_Int);

    return err;
}

/*****************************************************************************************

    NAME
        moHidd_UnixIO_RemInterrupt

    SYNOPSIS
        OOP_DoMethod(OOP_Object *obj, struct uioMsgRemInterrupt *msg);

        void Hidd_UnixIO_RemInterrupt(OOP_Object *obj, struct uioInterrupt *interrupt);

    LOCATION
        unixio.hidd

    FUNCTION
        Remove previously installed file descriptor event interrupt structure

    INPUTS
        obj       - An pointer to a UnixIO object
        interrupt - A pointer to a previously installed interrupt descriptor structure

    RESULT
	None.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	moHidd_UnixIO_AddInterrupt

    INTERNALS

    TODO

*****************************************************************************************/
void UXIO__Hidd_UnixIO__RemInterrupt(OOP_Class *cl, OOP_Object *o, struct uioMsgRemInterrupt *msg)
{
    Disable();

    Remove((struct Node *)msg->um_Int);

    Enable();
}

/* This is the initialisation code for the HIDD class itself. */

static const char *libc_symbols[] = {
    "open",
    "close",
    "ioctl",
    "fcntl",
    "poll",
    "read",
    "write",
    "getpid",
#ifdef HOST_OS_linux
    "__errno_location",
#else
    "__error",
#endif
    NULL
};

static int UXIO_Init(LIBBASETYPEPTR LIBBASE)
{
    ULONG i;

    D(bug("[UnixIO] Init\n"));

    KernelBase = OpenResource("kernel.resource");
    if (!KernelBase)
	return FALSE;

    HostLibBase = OpenResource("hostlib.resource");
    if (!HostLibBase)
    	return FALSE;

    if (!CheckArch(KernelBase, "unixio.hidd", AROS_ARCHITECTURE))
    	return FALSE;

    LIBBASE->uio_csd.UnixIOAB = OOP_ObtainAttrBase(IID_Hidd_UnixIO);
    if (!LIBBASE->uio_csd.UnixIOAB)
    	return FALSE;

    LIBBASE->libcHandle = HostLib_Open(LIBC_NAME, NULL);
    if (!LIBBASE->libcHandle)
    	return FALSE;

    LIBBASE->uio_csd.SysIFace = (struct LibCInterface *)HostLib_GetInterface(LIBBASE->libcHandle, libc_symbols, &i);
    if ((!LIBBASE->uio_csd.SysIFace) || i)
	return FALSE;

    LIBBASE->irqHandle = KrnAddIRQHandler(SIGIO, SigIO_IntServer, &LIBBASE->uio_csd, NULL);
    if (!LIBBASE->irqHandle)
    	return FALSE;

    InitSemaphore(&LIBBASE->uio_csd.sem);
    NewList((struct List *)&LIBBASE->uio_csd.intList);

    LIBBASE->uio_csd.errnoPtr = LIBBASE->uio_csd.SysIFace->__error();
    AROS_HOST_BARRIER
    LIBBASE->uio_csd.aros_PID  = LIBBASE->uio_csd.SysIFace->getpid();
    AROS_HOST_BARRIER

    return TRUE;
}

static int UXIO_Cleanup(struct unixio_base *LIBBASE)
{
    D(bug("[UnixIO] Expunging\n"));

    if ((!KernelBase) || (!HostLibBase))
    	return TRUE;

    if (LIBBASE->irqHandle)
	KrnRemIRQHandler(LIBBASE->irqHandle);

    if (LIBBASE->uio_csd.SysIFace)
	HostLib_DropInterface ((APTR *)LIBBASE->uio_csd.SysIFace);

    if (LIBBASE->libcHandle)
    	HostLib_Close(LIBBASE->libcHandle, NULL);

    if (LIBBASE->uio_csd.UnixIOAB)
    	OOP_ReleaseAttrBase(IID_Hidd_UnixIO);

    return TRUE;
}

/* The singleton gets really disposed only when we expunge its library */
static int UXIO_Dispose(struct unixio_base *LIBBASE)
{
    ULONG mid = OOP_GetMethodID(IID_Root, moRoot_Dispose);

    D(bug("[UnixIO] Disposing object\n"));

    OOP_DoSuperMethod(LIBBASE->uio_unixioclass, LIBBASE->uio_csd.obj, (OOP_Msg)&mid);
    LIBBASE->uio_csd.obj = NULL;

    return TRUE;
}

ADD2INITLIB(UXIO_Init, 0)
ADD2EXPUNGELIB(UXIO_Cleanup, 0)
ADD2SET(UXIO_Dispose, classesexpunge, 0)
