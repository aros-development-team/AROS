/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
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

#define __OOP_NOATTRBASES__

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
#include <hidd/unixio_inline.h>
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

/*****************************************************************************************

    NAME
	--background--

    LOCATION
	unixio.hidd

    NOTES
	unixio.hidd is a simple driver for host-side I/O on UNIX system. Its primary
	purpose is to handle non-blocking I/O on AROS task level. Also it provides
	common file access operations (open, close, read, write and ioctl) in order
	to avoid code duplication.

	I/O operations you perform must never block. The whole AROS with all its tasks
	is just one process from host OS' point of view, so blocking operation would
	halt all the system. In order to avoid this you need to make sure that the
	file descriptor is actually ready to perform I/O. If this is not the case,
	your task needs to wait until file descriptor becomes available. unixio.hidd
	offers a simple way of doing it by adding an interrupt handler to the file
	descriptor using moHidd_UnixIO_AddInterrupt method. The interrupt handler
	will be called whenever SIGIO arrives from the specified descriptor and specified
	conditions are met. You do not need to explicitly enable asynchronous I/O
	on the file descriptor, unixio.hidd takes care about all this itself.
	
	There's also a convenience moHidd_UnixIO_Wait method. It allows you to simulate
	a normal blocking I/O in a simple way.

    	Starting from v42 unixio.hidd is a singletone. This means that all calls to
    	OOP_NewObject() will actually return the same object which is never really
    	disposed. This object pointer can be freely transferred between tasks. It's
    	not necessary t call OOP_DisposeObject() on it. It is safe, but will do nothing.
	Usage counter is maintained by OpenLibrary()/CloseLibrary() calls.

	Remember that all values (like file mode flags and errno values) are host-specific!
	Different hosts may use different values, and even different structure layouts
	(especially this affects ioctl). When opening unixio.hidd it is adviced to check
	that host OS matches what is expected (what your client program/driver/whatever
	is compiled for). Use aoHidd_UnixIO_Architecture attribute for this.

*****************************************************************************************/

static int poll_fd(int fd, int req_mode, struct unixio_base *ud)
{
    struct pollfd pfd = {fd, 0, 0};
    int mode = 0;
    int res;

    if (req_mode & vHidd_UnixIO_Read)
	pfd.events |= POLLIN;
    if (req_mode & vHidd_UnixIO_Write)
	pfd.events |= POLLOUT;

    res = ud->SysIFace->poll(&pfd, 1, 0);
    AROS_HOST_BARRIER

    if (res > 0)
    {
	if (pfd.revents & POLLIN)
	    mode |= vHidd_UnixIO_Read;
	if (pfd.revents & POLLOUT)
	    mode |= vHidd_UnixIO_Write;
	if (pfd.revents & (POLLERR|POLLHUP))
	    mode |= vHidd_UnixIO_Error;
    }
    else if (res < 0)
	mode = -1;

    return mode;
}

static void SigIO_IntServer(struct unixio_base *ud, void *unused)
{
    struct uioInterrupt *intnode;

    /* Walk through the list of installed handlers and de-multiplex our SIGIO */
    for (intnode = (struct uioInterrupt *)ud->intList.mlh_Head; intnode->Node.mln_Succ;
    	 intnode = (struct uioInterrupt *)intnode->Node.mln_Succ)
    {
 	int mode = poll_fd(intnode->fd, intnode->mode, ud);

	if (mode > 0)
	{
	    D(bug("[UnixIO] Events 0x%02X for fd %d\n", mode, intnode->fd));

	    intnode->handler(intnode->fd, mode, intnode->handlerData);
	}
    }
}

static void WaitIntHandler(int fd, int mode, void *data)
{
    struct UnixIO_Waiter *w = data;
    Signal(w->task, 1 << w->signal);
}

static BOOL CheckArch(struct unixio_base *data, STRPTR Component, STRPTR MyArch)
{
    STRPTR arg[3] = {Component, MyArch, data->SystemArch};

    D(bug("[UnixIO] My architecture: %s, kernel architecture: %s\n", arg[1], arg[2]));

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

#define HostLibBase data->HostLibBase
#define KernelBase  data->KernelBase

#undef HiddUnixIOAttrBase
#define HiddUnixIOAttrBase data->UnixIOAB

/*****************************************************************************************

    NAME
        aoHidd_UnixIO_Opener

    SYNOPSIS
        [I..], STRPTR

    LOCATION
        unixio.hidd

    FUNCTION
        Specifiers opener name for architecture check routine.

    NOTES
    	This attribute's sole purpose is to be presented to the user in an error requester
    	if the architecture check fails. For example if you specify "tap.device" here,
    	the user will see a requester telling that "This version of tap.device is built
    	for XXX architecture, while current system architecture is YYY".

    	If this attribute is not specified, but architecture check is requested using
    	aoHidd_UnixIO_Architecture, current task's name will be used. This can be not
    	always approptiate, so it's adviced to always specify your driver or program
    	name here.

    EXAMPLE

    BUGS

    SEE ALSO
	aoHidd_UnixIO_Architecture

    INTERNALS

*****************************************************************************************/
/*****************************************************************************************

    NAME
        aoHidd_UnixIO_Architecture

    SYNOPSIS
        [I..], STRPTR

    LOCATION
        unixio.hidd

    FUNCTION
        Specifiers architecture name to match against current system's architecture.
        Architecture name needs to be supplied in the form "arch-cpu", for example
        "linux-ppc" or "darwin-i386". Usually this comes from a definition when
        you compile your module.

    NOTES
    	This attribute allows you to ensure that your module is running on the same
    	architecture it was compiled for. This is needed because unixio.hidd by its
    	nature works with host OS structures and values (especially ioctl operation).
    	Different host OSes (for example Linux and Darwin) are not binary compatible
    	even on the same CPU. This is why the architecture check is generally needed,
    	especially for disk-based components.

	It is adviced to specify your module name using aoHidd_UnixIO_Opener. This needed
	in order to display the correct name to the user if the check fails, so the user
	will see what module causes the error.

    EXAMPLE

	struct TagItem tags = {
	    {aHidd_UnixIO_Opener, "tap.device"},
	    {aHidd_UnixIO_Architecture, "linux-i386"},
	    {TAG_DONE, 0}
	};
	uio = OOP_NewObject(CLID_Hidd_UnixIO, tags);
	// If uio == NULL, the system you're running on is not linux-i386. The error
	// requester has been already presented to the user.

    BUGS

    SEE ALSO
	aoHidd_UnixIO_Opener

    INTERNALS

*****************************************************************************************/
/* The following are methods of the UnixIO HIDD class. */

/********************
**  UnixIO::New()  **
********************/
OOP_Object *UXIO__Root__New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    struct unixio_base *data = UD(cl);
    STRPTR archName;

    EnterFunc(bug("UnixIO::New(cl=%s)\n", cl->ClassNode.ln_Name));

    archName = (STRPTR)GetTagData(aHidd_UnixIO_Architecture, 0, msg->attrList);
    if (archName)
    {
        struct Task *t = FindTask(NULL);
    	STRPTR moduleName = (STRPTR)GetTagData(aHidd_UnixIO_Opener, (IPTR)t->tc_Node.ln_Name, msg->attrList);
    	
    	if (!CheckArch(data, moduleName, archName))
    	    return NULL;
    }

    /* We are a true singletone */
    ObtainSemaphore(&data->lock);

    if (!data->obj)
    {
    	D(bug("[UnixIO] Creating object\n"));
    	data->obj = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    }

    ReleaseSemaphore(&data->lock);

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
    	0 in case of success or UNIX errno value in case if the operation failed.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    TODO

*****************************************************************************************/
IPTR UXIO__Hidd_UnixIO__Wait(OOP_Class *cl, OOP_Object *o, struct uioMsg *msg)
{
    int retval = 0;
    int mode;
    struct uioInterrupt myInt;
    struct UnixIO_Waiter w;

    /* Check if the fd is already ready. In this case we don't need to wait for anything. */
    mode = Hidd_UnixIO_Poll(o, msg->um_Filedesc, msg->um_Mode, &retval);
    if (mode)
    	return (mode == -1) ? retval : 0;

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
    struct unixio_base *data = UD(cl);
    APTR retval;

    D(bug("[UnixIO] OpenFile(%s, 0x%04X, %o)\n", msg->um_FileName, msg->um_Flags, msg->um_Mode));

    HostLib_Lock();

    retval = (APTR)(unsigned long)data->SysIFace->open(msg->um_FileName, (int)msg->um_Flags, (int)msg->um_Mode);
    AROS_HOST_BARRIER

    if (msg->um_ErrNoPtr)
    	*msg->um_ErrNoPtr = *data->uio_Public.uio_ErrnoPtr;

    HostLib_Unlock();

    D(bug("[UnixIO] FD is %d, errno is %d\n", retval, *data->uio_Public.uio_ErrnoPtr));

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
    struct unixio_base *data = UD(cl);
    int ret = 0;

    if (msg->um_FD != (APTR)-1)
    {
    	HostLib_Lock();

    	ret = data->SysIFace->close((long)msg->um_FD);
    	AROS_HOST_BARRIER

    	if (msg->um_ErrNoPtr)
    	    *msg->um_ErrNoPtr = *data->uio_Public.uio_ErrnoPtr;

    	HostLib_Unlock();
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

	This method can be called from within interrupts.

    EXAMPLE

    BUGS

    SEE ALSO
	moHidd_UnixIO_WriteFile

    INTERNALS

    TODO

*****************************************************************************************/
IPTR UXIO__Hidd_UnixIO__ReadFile(OOP_Class *cl, OOP_Object *o, struct uioMsgReadFile *msg)
{
    struct unixio_base *data = UD(cl);
    int retval = -1;
    volatile int err = EINVAL;

    if (msg->um_FD != (APTR)-1)
    {
        int user = !KrnIsSuper();

    	if (user)
    	    HostLib_Lock();

    	do
	{
    	    retval = data->SysIFace->read((long)msg->um_FD, (void *)msg->um_Buffer, (size_t)msg->um_Count);
    	    AROS_HOST_BARRIER

    	    err = *data->uio_Public.uio_ErrnoPtr;
    	    D(kprintf(" UXIO__Hidd_UnixIO__ReadFile: retval %d errno %d  buff %x  count %d\n", retval, err, msg->um_Buffer, msg->um_Count));

    	    if (msg->um_ErrNoPtr)
    	    	break;

	} while((err == EINTR) || (err == EAGAIN));

	if (user)
	    HostLib_Unlock();
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

	This method can be called from within interrupts.

    EXAMPLE

    BUGS

    SEE ALSO
	moHidd_UnixIO_ReadFile

    INTERNALS

    TODO

*****************************************************************************************/
IPTR UXIO__Hidd_UnixIO__WriteFile(OOP_Class *cl, OOP_Object *o, struct uioMsgWriteFile *msg)
{
    struct unixio_base *data = UD(cl);
    int retval = -1;
    volatile int err = EINVAL;

    if (msg->um_FD != (APTR)-1)
    {
	int user = !KrnIsSuper();

    	if (user)
	    HostLib_Lock();

    	do
	{
    	    retval = data->SysIFace->write((long)msg->um_FD, (const void *)msg->um_Buffer, (size_t)msg->um_Count);
    	    AROS_HOST_BARRIER

	    err = *data->uio_Public.uio_ErrnoPtr;
    	    D(kprintf(" UXIO__Hidd_UnixIO__WriteFile: retval %d errno %d  buff %x  count %d\n", retval, err, msg->um_Buffer, msg->um_Count));

    	    if (msg->um_ErrNoPtr)
    	    	break;

	} while((retval < 1) && ((err == EINTR) || (err == EAGAIN) || (err == 0)));

	if (user)
	    HostLib_Unlock();
    }

    if (msg->um_ErrNoPtr)
	*msg->um_ErrNoPtr = err;
    
    D(if (retval == -1) kprintf("UXIO__Hidd_UnixIO__WriteFile: errno %d  buff %x  count %d\n", err, msg->um_Buffer, msg->um_Count));

    return retval;
}

/*****************************************************************************************

    NAME
        moHidd_UnixIO_IOControlFile

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
    	This method can be called from within interrupts.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    TODO

*****************************************************************************************/
IPTR UXIO__Hidd_UnixIO__IOControlFile(OOP_Class *cl, OOP_Object *o, struct uioMsgIOControlFile *msg)
{
    struct unixio_base *data = UD(cl);
    int err = EINVAL;
    int retval = -1;

    if (msg->um_FD != (APTR)-1)
    {
        int user = !KrnIsSuper();

	if (user)
    	    HostLib_Lock();

    	retval = data->SysIFace->ioctl((long)msg->um_FD, (int)msg->um_Request, msg->um_Param);
    	AROS_HOST_BARRIER

	err = *data->uio_Public.uio_ErrnoPtr;

	if (user)
	    HostLib_Unlock();
    }

    if (msg->um_ErrNoPtr)
	*msg->um_ErrNoPtr = err;

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
    struct unixio_base *data = UD(cl);
    int res;
    int err;

    Disable();
    AddTail((struct List *)&data->intList, (struct Node *)msg->um_Int);
    Enable();

    /* Now own the filedescriptor and enable SIGIO on it */
    HostLib_Lock();

    res = data->SysIFace->fcntl(msg->um_Int->fd, F_SETOWN, data->aros_PID);
    AROS_HOST_BARRIER

    if (res != -1)
    {
	res = data->SysIFace->fcntl(msg->um_Int->fd, F_GETFL);
	AROS_HOST_BARRIER
	res = data->SysIFace->fcntl(msg->um_Int->fd, F_SETFL, res|O_ASYNC);
	AROS_HOST_BARRIER
    }
    err = *data->uio_Public.uio_ErrnoPtr;

    HostLib_Unlock();

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
    /*
     * We do not disable O_ASYNC because theoretically we can have more
     * than one interrupt on a single fd.
     * Anyway typically removing interrupt handler means the fd is not
     * going to be used any more and will be closed soon.
     */
}

/*****************************************************************************************

    NAME
        moHidd_UnixIO_Poll

    SYNOPSIS
        OOP_DoMethod(OOP_Object *obj, struct uioMsgPoll *msg);

        int Hidd_UnixIO_Poll(OOP_Object *obj, int fd, int mode, int *errno_ptr);

    LOCATION
        unixio.hidd

    FUNCTION
        Check current status of UNIX file descriptor or -1 if an error occured.

    INPUTS
        obj	  - A pointer to a UnixIO object.
        fd        - A file descriptor to check.
        mode      - Mask of modes we are interested in.
	errno_ptr - An optional pointer to a location where error code (a value of UNIX
		    errno variable) will be written.

    RESULT
	Current set of filedescriptor modes.

    NOTES
    	This method can be called from within interrupts.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    TODO

*****************************************************************************************/
ULONG UXIO__Hidd_UnixIO__Poll(OOP_Class *cl, OOP_Object *o, struct uioMsgPoll *msg)
{
    struct unixio_base *data = UD(cl);
    int user = !KrnIsSuper();
    int ret;
 
    if (user)
    	HostLib_Lock();

    ret = poll_fd((int)(IPTR)msg->um_FD, msg->um_Mode, data);
    if (msg->um_ErrNoPtr)
	*msg->um_ErrNoPtr = *data->uio_Public.uio_ErrnoPtr;

    if (user)
    	HostLib_Unlock();

    return ret;
}

/*****************************************************************************************

    NAME
        moHidd_UnixIO_MemoryMap

    SYNOPSIS
        OOP_DoMethod(OOP_Object *obj, struct uioMsgMemoryMap *msg);

        int Hidd_UnixIO_MemoryMap(OOP_Object *obj, OOP_Object *o, void *addr, int len, int prot, int flags, int fd, int offset, int *errno_ptr);

    LOCATION
        unixio.hidd

    FUNCTION
        Maps address into file descriptor.

    INPUTS
        obj   - A pointer to a UnixIO object.
        fd    - A file descriptor to check.
    errno_ptr - An optional pointer to a location where error code (a value of UNIX
            errno variable) will be written.

    RESULT
        Actuall mapping address or MAP_FAILED for errors.

    NOTES
        This method can be called from within interrupts.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    TODO

*****************************************************************************************/
APTR UXIO__Hidd_UnixIO__MemoryMap(OOP_Class *cl, OOP_Object *o, struct uioMsgMemoryMap *msg)
{
    struct unixio_base *data = UD(cl);
    int user = !KrnIsSuper();
    APTR ret;

    if (user)
        HostLib_Lock();

    ret = data->SysIFace->mmap(msg->um_Address, msg->um_Length, msg->um_Prot, msg->um_Flags,  (int)(IPTR)msg->um_FD, msg->um_Offset);
    if (msg->um_ErrNoPtr)
        *msg->um_ErrNoPtr = *data->uio_Public.uio_ErrnoPtr;

    if (user)
        HostLib_Unlock();

    return ret;
}

/*****************************************************************************************

    NAME
        moHidd_UnixIO_MemoryUnMap

    SYNOPSIS
        OOP_DoMethod(OOP_Object *obj, struct uioMsgMemoryUnMap *msg);

        int Hidd_UnixIO_MemoryUnMap(OOP_Object *obj, OOP_Object *o, void *addr, int len, int *errno_ptr);

    LOCATION
        unixio.hidd

    FUNCTION
        Unmaps memory

    INPUTS
        obj   - A pointer to a UnixIO object.
    errno_ptr - An optional pointer to a location where error code (a value of UNIX
            errno variable) will be written.

    RESULT
        0 for success, -1 for failure

    NOTES
        This method can be called from within interrupts.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    TODO

*****************************************************************************************/
IPTR UXIO__Hidd_UnixIO__MemoryUnMap(OOP_Class *cl, OOP_Object *o, struct uioMsgMemoryUnMap *msg)
{
    struct unixio_base *data = UD(cl);
    int user = !KrnIsSuper();
    IPTR ret;

    if (user)
        HostLib_Lock();

    ret = data->SysIFace->munmap(msg->um_Address, msg->um_Length);
    if (msg->um_ErrNoPtr)
        *msg->um_ErrNoPtr = *data->uio_Public.uio_ErrnoPtr;

    if (user)
        HostLib_Unlock();

    return ret;
}


/* This is the initialisation code for the HIDD class itself. */

static const char *libc_symbols[] =
{
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
#ifdef HOST_OS_android
    "__errno",
#else
    "__error",
#endif
#endif
    "mmap",
    "munmap",
    NULL
};

#undef HostLibBase
#undef KernelBase
#define KernelBase  LIBBASE->KernelBase
#define HostLibBase LIBBASE->HostLibBase

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

    LIBBASE->SystemArch = (STRPTR)KrnGetSystemAttr(KATTR_Architecture);
    if (!LIBBASE->SystemArch)
    	return FALSE;

    if (!CheckArch(LIBBASE, "unixio.hidd", AROS_ARCHITECTURE))
    	return FALSE;

    LIBBASE->UnixIOAB = OOP_ObtainAttrBase(IID_Hidd_UnixIO);
    if (!LIBBASE->UnixIOAB)
    	return FALSE;

    LIBBASE->uio_Public.uio_LibcHandle = HostLib_Open(LIBC_NAME, NULL);
    if (!LIBBASE->uio_Public.uio_LibcHandle)
    	return FALSE;

    LIBBASE->SysIFace = (struct LibCInterface *)HostLib_GetInterface(LIBBASE->uio_Public.uio_LibcHandle, libc_symbols, &i);
    if ((!LIBBASE->SysIFace) || i)
	return FALSE;

    LIBBASE->irqHandle = KrnAddIRQHandler(SIGIO, SigIO_IntServer, LIBBASE, NULL);
    if (!LIBBASE->irqHandle)
    	return FALSE;

    NewList((struct List *)&LIBBASE->intList);
    InitSemaphore(&LIBBASE->lock);

    LIBBASE->uio_Public.uio_ErrnoPtr = LIBBASE->SysIFace->__error();
    AROS_HOST_BARRIER
    LIBBASE->aros_PID  = LIBBASE->SysIFace->getpid();
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

    if (LIBBASE->SysIFace)
	HostLib_DropInterface ((APTR *)LIBBASE->SysIFace);

    if (LIBBASE->uio_Public.uio_LibcHandle)
    	HostLib_Close(LIBBASE->uio_Public.uio_LibcHandle, NULL);

    if (LIBBASE->UnixIOAB)
    	OOP_ReleaseAttrBase(IID_Hidd_UnixIO);

    return TRUE;
}

/* The singleton gets really disposed only when we expunge its library */
static int UXIO_Dispose(struct unixio_base *LIBBASE)
{
    if (LIBBASE->obj)
    {
	OOP_MethodID mid = OOP_GetMethodID(IID_Root, moRoot_Dispose);

	D(bug("[UnixIO] Disposing object\n"));

	OOP_DoSuperMethod(LIBBASE->uio_unixioclass, LIBBASE->obj, &mid);
    	LIBBASE->obj = NULL;
    }

    return TRUE;
}

ADD2INITLIB(UXIO_Init, 0)
ADD2EXPUNGELIB(UXIO_Cleanup, 0)
ADD2SET(UXIO_Dispose, CLASSESEXPUNGE, 0)
