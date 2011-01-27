#ifndef GRAPHICS_DRIVER_H
#define GRAPHICS_DRIVER_H

/*
    Copyright © 2010-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Display driver definitions.
    Lang: english
    
    Information contained in this file is AROS-specific.
*/

/* Tags for AddDisplayDriverA() */

#define DDRV_BootMode	  (TAG_USER + 0x01)	/* BOOL    Boot mode driver which will be unloaded when any next driver comes in, default = FALSE */
#define DDRV_MonitorID	  (TAG_USER + 0x02)	/* ULONG   Monitor ID for this driver, default = next available */
#define DDRV_ReserveIDs	  (TAG_USER + 0x03)	/* ULONG   How many monitor IDs to reserve, default = 1 */
#define DDRV_KeepBootMode (TAG_USER + 0x04)	/* BOOL    Do not shut down boot mode drivers, default = FALSE */
#define DDRV_ResultID	  (TAG_USER + 0x05)	/* ULONG * Obtain assigned monitor ID */
#define DDRV_IDMask	  (TAG_USER + 0x06)	/* ULONG   Use own mask for monitor ID separation */

/* Return codes */

#define DD_OK	     0	/* No error */
#define DD_NO_MEM    1	/* Out of memory */
#define DD_ID_EXISTS 2	/* Specified MonitorID is already allocated */

/* This structure is subject to change! Private! */
struct MonitorHandle
{
    struct MonitorHandle *next;
    ULONG		  id;
    ULONG		  mask;
    APTR		  gfxhidd;
};

#endif
