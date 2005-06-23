#ifndef DEVICES_SANA2SPECIALSTATS_H
#define DEVICES_SANA2SPECIALSTATS_H

/*
    Copyright (C) 2005 Neil Cafferkey
    $Id$

    Desc: Special statistics definitions for SANA-II devices
    Lang: english
*/

#include <devices/sana2.h>


/* Constants */
/* ========= */

/* Ethernet */

#define S2SS_ETHERNET_BADMULTICAST   (S2WireType_Ethernet << 16 | 0)
#define S2SS_ETHERNET_RETRIES        (S2WireType_Ethernet << 16 | 1)
#define S2SS_ETHERNET_FIFO_UNDERRUNS (S2WireType_Ethernet << 16 | 2)

#endif
