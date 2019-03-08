#ifndef DEVICES_ATA_H
#define DEVICES_ATA_H

/*
    Copyright © 2019, The AROS Development Team. All rights reserved.
    $Id$

    Desc: ATA Device standard
    Lang: english
*/

#include <devices/smart.h>

#define HD_SMARTCMD				(CMD_NONSTD + 22)
#define HD_TRIMCMD				(CMD_NONSTD + 23)

/* Commands (io_Offset) */
#define ATAFEATURE_TEST_AVAIL	0x54535446              /* TSTF */

#define SMART_MAGIC_ID 			0x534D5254				/* SMRT */
#define TRIM_MAGIC_ID 			0x5452494D				/* TRIM */

#endif /* DEVICES_ATA_H */
