/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/

#ifndef IPP_INTERN_H
#define IPP_INTERN_H

#include <exec/devices.h>
#include <exec/semaphores.h>
#include <dos/dos.h>

#define IPP_UNITS       10      /* as many as printer.device has printers */

#define IPP_URI_MAX     256
#define IPP_TEXT_MAX    128

/* A unit's printer, from ENV:IPP/Unit<n> (see ipp_config.c). */
struct IPPConfig {
    char ic_URI[IPP_URI_MAX];           /* ipp://host[:port]/path or socket://host[:port] */
    char ic_Format[IPP_TEXT_MAX];       /* document-format; application/octet-stream when unset */
    char ic_Name[IPP_TEXT_MAX];         /* what the printer calls itself */
};

/* Where a URI points. */
struct IPPTarget {
    BOOL it_IPP;                        /* FALSE: socket://, the job's bytes as they are */
    char it_Host[IPP_URI_MAX];
    UWORD it_Port;
    char it_Path[IPP_URI_MAX];
};

/* One job: it runs from the first open of the unit to the last close. The
   open connects; the request goes out with the first write, so a unit that
   is opened and closed without writing sends the printer nothing. */
struct IPPUnit {
    struct SignalSemaphore iu_Lock;
    ULONG iu_Opens;
    ULONG iu_Number;
    struct Library *iu_SocketBase;      /* the opener's, as sockets belong to a task */
    LONG iu_Socket;
    ULONG iu_JobId;
    BOOL iu_Started;                    /* the request is on its way: something was written */
    BOOL iu_Failed;
    ULONG iu_Bytes;
    struct IPPConfig iu_Config;
    struct IPPTarget iu_Target;
    char iu_Error[160];
};

struct IPPBase {
    struct Device ib_Device;
    struct IPPUnit ib_Unit[IPP_UNITS];
    ULONG ib_NextJob;
    struct Library *ib_DOSBase;
};

#define DOSBase IPPBase->ib_DOSBase

/* ipp_config.c */
BOOL IPP_ReadConfig(struct IPPBase *IPPBase, ULONG unit, struct IPPConfig *ic);
BOOL IPP_ParseURI(const char *uri, struct IPPTarget *t);
void IPP_AppendNumber(char *s, ULONG n);
void IPP_SetStatus(struct IPPBase *IPPBase, struct IPPUnit *iu, const char *text);

/* ipp_net.c */
BOOL IPP_BeginJob(struct IPPBase *IPPBase, struct IPPUnit *iu, ULONG id);
BOOL IPP_Send(struct IPPBase *IPPBase, struct IPPUnit *iu, const UBYTE *data, LONG len);
BOOL IPP_EndJob(struct IPPBase *IPPBase, struct IPPUnit *iu);
void IPP_Disconnect(struct IPPBase *IPPBase, struct IPPUnit *iu);

#endif /* IPP_INTERN_H */
