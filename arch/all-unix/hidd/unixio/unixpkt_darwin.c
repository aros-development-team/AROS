/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Stubs for the UnixIO raw-packet methods on hosts that lack Linux-style
          PF_PACKET raw sockets (e.g. darwin). unixpkt_class.c is built from
          Linux-only networking APIs (PF_PACKET, sockaddr_ll, SIOCGIFHWADDR),
          none of which exist on macOS. Only network drivers use these methods;
          the filesystem path (emul-handler) uses the file/fd methods alone.
          These stubs exist solely to satisfy the UnixIO class method table.
*/

#define DEBUG 0
#include <aros/debug.h>

#define __OOP_NOATTRBASES__

#include <exec/types.h>
#include <hidd/unixio.h>
#include <aros/symbolsets.h>
#include <oop/oop.h>
#include <proto/oop.h>

#include "unixio.h"

#include LC_LIBDEFS_FILE

IPTR UXIO__Hidd_UnixIO__OpenPacket(OOP_Class *cl, OOP_Object *o, struct pHidd_UnixIO_OpenPacket *msg)
{
    return 0;
}

IPTR UXIO__Hidd_UnixIO__ClosePacket(OOP_Class *cl, OOP_Object *o, struct pHidd_UnixIO_ClosePacket *msg)
{
    return 0;
}

IPTR UXIO__Hidd_UnixIO__RecvPacket(OOP_Class *cl, OOP_Object *o, struct pHidd_UnixIO_RecvPacket *msg)
{
    return (IPTR)-1;
}

IPTR UXIO__Hidd_UnixIO__SendPacket(OOP_Class *cl, OOP_Object *o, struct pHidd_UnixIO_SendPacket *msg)
{
    return (IPTR)-1;
}

IPTR UXIO__Hidd_UnixIO__PacketGetFileDescriptor(OOP_Class *cl, OOP_Object *o, struct pHidd_UnixIO_PacketGetFileDescriptor *msg)
{
    return (IPTR)-1;
}

IPTR UXIO__Hidd_UnixIO__PacketGetMACAddress(OOP_Class *cl, OOP_Object *o, struct pHidd_UnixIO_PacketGetMACAddress *msg)
{
    return 0;
}
