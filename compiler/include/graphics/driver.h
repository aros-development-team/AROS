#ifndef GRAPHICS_DRIVER_H
#define GRAPHICS_DRIVER_H

/*
    Copyright (C) 2010-2026, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Display driver definitions.
    Lang: english
    
    Information contained in this file is AROS-specific.
*/

#ifndef EXEC_TYPES_H
#   include <exec/types.h>
#endif
#ifndef UTILITY_TAGITEM_H
#   include <utility/tagitem.h>
#endif

/* Tags for AddDisplayDriverA() */

#define DDRV_BootMode	  (TAG_USER + 0x01)	/* BOOL    Boot mode driver which will be unloaded when any next driver comes in, default = FALSE */
#define DDRV_MonitorID	  (TAG_USER + 0x02)	/* ULONG   Monitor ID for this driver, default = next available */
#define DDRV_ReserveIDs	  (TAG_USER + 0x03)	/* ULONG   How many monitor IDs to reserve, default = 1 */
#define DDRV_KeepBootMode (TAG_USER + 0x04)	/* BOOL    Do not shut down boot mode drivers, default = FALSE */
#define DDRV_ResultID	  (TAG_USER + 0x05)	/* ULONG * Obtain assigned monitor ID */
#define DDRV_IDMask	  (TAG_USER + 0x06)	/* ULONG   Use own mask for monitor ID separation */
#define DDRV_HWRanges	  (TAG_USER + 0x07)	/* struct DisplayRange * Hardware this driver writes to, default = NULL */

/*
 * A region of the CPU address space a display driver writes to - for a
 * boot-mode driver, the firmware framebuffer it inherited; for a native
 * driver, the apertures of the card it drives.
 *
 * Addresses are always CPU addresses, never bus/PCI ones. The two sides
 * are compared numerically, so a driver holding a PCI resource must
 * translate it first - HIDD_PCIDriver_PCItoCPU() translates without
 * establishing a mapping, unlike MapPCI()/ioremap(). On machines where a
 * BAR is not identity-mapped, the untranslated address matches nothing.
 *
 * An array is terminated by an entry with dr_Size == 0.
 */
struct DisplayRange
{
    APTR		  dr_Base;
    IPTR		  dr_Size;
};

/*
 * Handover interface. graphics.library places a pointer to one of these
 * in the attribute taglist it passes to a non-boot-mode driver's New(),
 * under DDRVA_Handover.
 *
 * A driver that is about to take ownership of display hardware uses it to
 * find, and then tear down, any boot-mode driver still writing to that
 * hardware - typically a firmware framebuffer living in a BAR the new
 * driver is about to reprogram, whose subsequent redraws would otherwise
 * land in whatever those addresses mean under the new owner.
 *
 * Both calls are only valid from within New(), where graphics.library
 * holds the display database locked on the caller's behalf.
 *
 *   dho_FindDisplay(ctx, ranges)
 *     Returns an opaque handle for one boot-mode display whose hardware
 *     overlaps ranges, or NULL when none is left. A boot-mode driver that
 *     declared no DDRV_HWRanges is assumed to conflict, so drivers that
 *     never described their hardware keep behaving as they always did.
 *
 *   dho_ExpungeDisplay(ctx, handle)
 *     Shuts the display down for good: it is unlinked from the display
 *     database, Intuition is notified, its modes stop being enumerated
 *     and its driver object is disposed. Returns FALSE, changing nothing,
 *     if the display is still in use - a handle that fails this way is
 *     not returned by dho_FindDisplay() again, so a find/expunge loop
 *     always terminates.
 */
struct DisplayHandover
{
    APTR		  dho_Context;
    APTR		(*dho_FindDisplay)(APTR ctx, const struct DisplayRange *ranges);
    BOOL		(*dho_ExpungeDisplay)(APTR ctx, APTR handle);
};

/* Tags injected by graphics.library into the driver class attribute taglist */

#define DDRVA_Handover	  (TAG_USER + 0x1000)	/* struct DisplayHandover * */

/* Return codes */

#define DD_OK	     	0  /* No error					  */
#define DD_NO_MEM	1  /* Out of memory				  */
#define DD_ID_EXISTS	2  /* Specified MonitorID is already allocated	  */
#define DD_IN_USE    	3  /* One of boot mode drivers can't be shut down */
#define DD_DRIVER_ERROR 4  /* Failed to create driver object		  */

/* This structure is subject to change! Private! */
struct MonitorHandle
{
    struct MonitorHandle *next;
    ULONG		  id;
    ULONG		  mask;
    ULONG		  pad;
    APTR		  display;
    APTR		  gfxhidd;
};

#endif
