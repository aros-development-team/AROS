/*
    (C) 1995-97 AROS - The Amiga Research OS
    $Id$
*/
#ifndef PROTO_ARP_H
#define PROTO_ARP_H

#ifndef AROS_SYSTEM_H
#include <aros/system.h>
#endif

#include <clib/arp_protos.h>

#if defined(_AMIGA) && defined(__GNUC__)
#include <inline/arp.h>
#else
#include <defines/arp.h>
#endif

#endif /* PROTO_ARP_H */
