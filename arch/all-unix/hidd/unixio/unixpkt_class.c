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

#define SDEBUG 0
#define DEBUG 0
#include <aros/debug.h>

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

#define HostLibBase data->HostLibBase
#define KernelBase  data->KernelBase

struct uioPacket {
    int fd;
    struct sockaddr ifaddr;
    short ifindex;
};

/*****************************************************************************************

    NAME
        moHidd_UnixIO_OpenPacket

    SYNOPSIS
        OOP_DoMethod(OOP_Object *obj, struct pHidd_UnixIO_OpenPacket *msg);

        int Hidd_UnixIO_OpenPacket (OOP_Object *obj, const char *Interface, int *errno_ptr);

    LOCATION
        unixio.hidd

    FUNCTION
        Open a UNIX packet descriptor to a raw network interface

    INPUTS
        obj       - An pointer to a UnixIO object
        interface - Name of a network interace (ie eth0)
        errno_ptr - An optional pointer to a location where error code (value of
                UNIX errno variable) will be written

    RESULT
        A number of the opened packet descriptor or -1 for an error. 

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        moHidd_UnixIO_ClosePacket

    INTERNALS

    TODO

*****************************************************************************************/

#ifdef HOST_OS_linux
#include <net/if.h>
#include <linux/if_ether.h>
#include <linux/if_packet.h>
#include <stdio.h> // for snprintf

#define htons(x)    AROS_WORD2BE(x)

struct uioPacket *linux_OpenPacket(struct LibCInterface *SysIFace, const char *interface)
{
    struct ifreq ifr = {};
    struct uioPacket *pd;

    D(bug("%s: interface='%s'\n", __func__, interface));
    if (strlen(interface) <= (sizeof(ifr.ifr_name)-1)) {
        if ((pd = AllocVec(sizeof(*pd), MEMF_ANY | MEMF_CLEAR))) {
            int sd;
            sd = SysIFace->socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL));

            pd->fd = sd;

            D(bug("%s: sd=%d\n", __func__, sd));
            if (sd >= 0) {

                snprintf(ifr.ifr_name, sizeof(ifr.ifr_name), "%s", interface);
                ifr.ifr_name[sizeof(ifr.ifr_name)-1]=0;

                if (SysIFace->ioctl(sd, SIOCGIFFLAGS, &ifr) >= 0) {
                    ifr.ifr_flags |= IFF_PROMISC;
                    if (SysIFace->ioctl(sd, SIOCSIFFLAGS, &ifr) >= 0) {
                        if (SysIFace->ioctl(sd, SIOCGIFINDEX, &ifr) >= 0) {
                            pd->ifindex = ifr.ifr_ifindex;
                            if (SysIFace->ioctl(sd, SIOCGIFHWADDR, &ifr) >= 0) {
                                pd->ifaddr = ifr.ifr_hwaddr;
                                struct sockaddr_ll sll = {};
                                sll.sll_family = AF_PACKET;
                                sll.sll_ifindex = pd->ifindex;
                                sll.sll_protocol = htons(ETH_P_ALL);
                                if (SysIFace->bind(sd, (struct sockaddr *)&sll, sizeof(sll)) >= 0) {
                                    return pd;
                                } else {
                                    D(bug("%s: sd=%d bind\n", __func__, sd));
                                }
                            } else {
                                D(bug("%s: sd=%d SIOCGIFHWADDR\n", __func__, sd));
                            }
                        } else {
                            D(bug("%s: sd=%d SIOCGIFINDEX, errno=%d\n", __func__, sd, *(SysIFace->__error())));
                        }
                    } else {
                        D(bug("%s: sd=%d SIOCSIFFLAGS, errno=%d\n", __func__, sd, *(SysIFace->__error())));
                    }
                } else {
                    D(bug("%s: sd=%d SIOCGIFFLAGS, errno=%d\n", __func__, sd, *(SysIFace->__error())));
                }
                SysIFace->close(sd);
            }
            FreeVec(pd);
        }
    }

    return NULL;
}
#endif

IPTR UXIO__Hidd_UnixIO__OpenPacket(OOP_Class *cl, OOP_Object *o, struct pHidd_UnixIO_OpenPacket *msg)
{
    struct unixio_base *data = UD(cl);
    IPTR retval;

    D(bug("[UnixIO] OpenFile(%s, 0x%04X, %o)\n", msg->FileName, msg->Flags, msg->Mode));

    HostLib_Lock();

#ifdef HOST_OS_linux
    retval = (IPTR)linux_OpenPacket(data->SysIFace, msg->Interface);
    AROS_HOST_BARRIER
#else
    retval = (IPTR)NULL;
#endif

    if (msg->ErrNoPtr)
        *msg->ErrNoPtr = *data->uio_Public.uio_ErrnoPtr;

    HostLib_Unlock();

    D(bug("[UnixIO] PD is %d, errno is %d\n", retval, *data->uio_Public.uio_ErrnoPtr));

    return retval;
}

/*****************************************************************************************

    NAME
        moHidd_UnixIO_ClosePacket

    SYNOPSIS
        OOP_DoMethod(OOP_Object *obj, struct pHidd_UnixIO_ClosePacket *msg);

        int Hidd_UnixIO_ClosePacket (OOP_Object *obj, int fd, int *errno_ptr);

    LOCATION
        unixio.hidd

    FUNCTION
        Close a UNIX packet descriptor.

    INPUTS
        obj       - A pointer to a UnixIO object.
        pd        - A packet descriptor to close.
        errno_ptr - An optional pointer to a location where error code (a value of UNIX
                    errno variable) will be written.

    RESULT
        0 in case of success and -1 on failure.

    NOTES
        Despite there's no return value, error code still can be set.

    EXAMPLE

    BUGS

    SEE ALSO
        moHidd_UnixIO_OpenPacket

    INTERNALS

    TODO

*****************************************************************************************/
IPTR UXIO__Hidd_UnixIO__ClosePacket(OOP_Class *cl, OOP_Object *o, struct pHidd_UnixIO_ClosePacket *msg)
{
    struct unixio_base *data = UD(cl);
    int ret = 0;

    if (msg->PD)
    {
        struct uioPacket *pd = msg->PD;

        HostLib_Lock();

        ret = data->SysIFace->close(pd->fd);
        AROS_HOST_BARRIER

        if (msg->ErrNoPtr)
            *msg->ErrNoPtr = *data->uio_Public.uio_ErrnoPtr;

        HostLib_Unlock();

        FreeVec(pd);
    }

    return ret;
}

/*****************************************************************************************

    NAME
        moHidd_UnixIO_RecvPacket

    SYNOPSIS
        OOP_DoMethod(OOP_Object *obj, struct pHidd_UnixIO_RecvPacket *msg);

        int Hidd_UnixIO_RecvPacket(OOP_Object *obj, int fd, void *buffer, int count, int *errno_ptr);

    LOCATION
        unixio.hidd

    FUNCTION
        Read packet from a Unix packet descriptor

    INPUTS
        obj       - A pointer to a UnixIO object.
        pd        - A packet descriptor to read from.
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
        moHidd_UnixIO_SendPacket

    INTERNALS

    TODO

*****************************************************************************************/
IPTR UXIO__Hidd_UnixIO__RecvPacket(OOP_Class *cl, OOP_Object *o, struct pHidd_UnixIO_RecvPacket *msg)
{
    struct unixio_base *data = UD(cl);
    int retval = -1;
    volatile int err = EINVAL;

    if (msg->PD)
    {
        struct uioPacket *pd = msg->PD;
        int user = !KrnIsSuper();

        if (user)
            HostLib_Lock();

        do
        {
            retval = data->SysIFace->recvfrom(pd->fd, (void *)msg->Buffer, (size_t)msg->Length, MSG_DONTWAIT, NULL, NULL);
            AROS_HOST_BARRIER

            err = *data->uio_Public.uio_ErrnoPtr;
            D(kprintf(" UXIO__Hidd_UnixIO__RecvPacket: retval %d errno %d  buff %x  count %d\n", retval, err, msg->Buffer, msg->Length));

            if (msg->ErrNoPtr)
                break;

        } while((err == EINTR) || (err == EAGAIN));

        if (user)
            HostLib_Unlock();
    }

    if (msg->ErrNoPtr)
        *msg->ErrNoPtr = err;
    
    D(if (retval == -1) kprintf("UXIO__Hidd_UnixIO__RecvPacket: errno %d  buff %x  count %d\n", err, msg->Buffer, msg->Length));
    
    return retval;
}

/*****************************************************************************************

    NAME
        moHidd_UnixIO_SendPacket

    SYNOPSIS
        OOP_DoMethod(OOP_Object *obj, struct pHidd_UnixIO_SendPacket *msg);

        int Hidd_UnixIO_SendPacket(OOP_Object *obj, int fd, void *buffer, int count, int *errno_ptr);

    LOCATION
        unixio.hidd

    FUNCTION
        Write data to a UNIX packet descriptor.

    INPUTS
        obj       - A pointer to a UnixIO object.
        pd        - A packet descriptor to write to.
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
        moHidd_UnixIO_RecvPacket

    INTERNALS

    TODO

*****************************************************************************************/
IPTR UXIO__Hidd_UnixIO__SendPacket(OOP_Class *cl, OOP_Object *o, struct pHidd_UnixIO_SendPacket *msg)
{
    struct unixio_base *data = UD(cl);
    int retval = -1;
    volatile int err = EINVAL;
    struct sockaddr_ll device = {};

    if (msg->PD)
    {
        struct uioPacket *pd = msg->PD;
        int user = !KrnIsSuper();

        device.sll_ifindex = pd->ifindex;
        device.sll_family = AF_PACKET;
        memcpy(device.sll_addr, pd->ifaddr.sa_data, 6);
        device.sll_halen = htons(6);

        if (user)
            HostLib_Lock();

        do
        {
            retval = data->SysIFace->sendto(pd->fd, (const void *)msg->Buffer, (size_t)msg->Length, 0, (struct sockaddr *)&device, sizeof(device));
            AROS_HOST_BARRIER

            err = *data->uio_Public.uio_ErrnoPtr;
            D(kprintf(" UXIO__Hidd_UnixIO__SendPacket: retval %d errno %d  buff %x  count %d\n", retval, err, msg->Buffer, msg->Length));

            if (msg->ErrNoPtr)
                break;

        } while((retval < 1) && ((err == EINTR) || (err == EAGAIN) || (err == 0)));

        if (user)
            HostLib_Unlock();
    }

    if (msg->ErrNoPtr)
        *msg->ErrNoPtr = err;
    
    D(if (retval == -1) kprintf("UXIO__Hidd_UnixIO__SendPacket: errno %d  buff %x  count %d\n", err, msg->Buffer, msg->Length));

    return retval;
}

IPTR UXIO__Hidd_UnixIO__PacketGetFileDescriptor(OOP_Class *cl, OOP_Object *o, struct pHidd_UnixIO_PacketGetFileDescriptor *msg)
{
    if (msg->PD)
    {
        struct uioPacket *pd = msg->PD;

        return pd->fd;
    }

    return -1;
}

IPTR UXIO__Hidd_UnixIO__PacketGetMACAddress(OOP_Class *cl, OOP_Object *o, struct pHidd_UnixIO_PacketGetMACAddress *msg)
{
    if (msg->PD)
    {
        struct uioPacket *pd = msg->PD;

        if (msg->MACAddress)
            memcpy(msg->MACAddress, pd->ifaddr.sa_data, 6);

        return 6;
    }

    return -1;
}
