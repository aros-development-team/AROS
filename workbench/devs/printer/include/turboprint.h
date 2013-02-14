/*
 * Copyright (C) 2013, The AROS Development Team
 * All right reserved.
 * Author: Jason S. McMullan <jason.mcmullan@gmail.com>
 *
 * Licensed under the AROS PUBLIC LICENSE (APL) Version 1.1
 */

#ifndef TURBOPRINT_H
#define TURBOPRINT_H

#ifndef DEVICES_PRINTER_H
#include <devices/printer.h>
#endif /* DEVICES_PRINTER_H */

#define TPFMT_BitPlanes     0x0000
#define TPFMT_Chunky8       0x0001
#define TPFMT_BGR15         0x0002
#define TPFMT_BGR16         0x0003
#define TPFMT_BGR24         0x0004
#define TPFMT_RGB15         0x0012
#define TPFMT_RGB16         0x0013
#define TPFMT_RGB24         0x0014

#define TPFMT_HAM           0x0800
#define TPFMT_EHB           0x0080
#define TPFMT_CyberGraphX   0x0400

#define TPMATCHWORD         0xf10a57ef

#define PRD_TPEXTDUMPRPORT  (PRD_DUMPRPORT | 0x80)

/* struct IODRPReq io_Modes must point to this structure */

struct TPExtIODRP {
    UWORD   PixAspX;        /* X Aspect */
    UWORD   PixAspY;        /* Y Aspect - together they make the acpect ratio */
    UWORD   Mode;           /* TPFMT_* mode */
};

#endif /* TURBOPRINT_H */
