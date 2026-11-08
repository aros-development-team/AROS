/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/

#ifndef DNSSD_H
#define DNSSD_H

#include <exec/types.h>

#define NAME_MAX        256
#define TXT_MAX         1300    /* a TXT record as large as one mDNS packet carries (RFC 6763 6.2) */

/* A printer that answered: IPP (_ipp._tcp) or raw socket (_pdl-datastream._tcp). */
struct DNSSDPrinter {
    char dp_Name[NAME_MAX];             /* the instance's first label */
    char dp_URI[NAME_MAX];              /* ipp://host:port/rp or socket://host:port */
    char dp_Formats[TXT_MAX];           /* pdl */
    char dp_UUID[64];
    BOOL dp_IPP;
};

/* Asks the link for printers, listening ms milliseconds; returns how many answered (at
   most max), or -1 with the socket error in *error when no query could be sent. */
LONG DNSSD_FindPrinters(struct Library *socketbase, struct Device *timerbase, LONG ms,
    struct DNSSDPrinter *printers, LONG max, LONG *error);

#endif
