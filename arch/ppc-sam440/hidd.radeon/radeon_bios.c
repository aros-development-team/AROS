/*
    Copyright © 2004-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: ATI radeon driver. BIOS part.
    Lang: English
*/

#include <exec/types.h>

/* Read the Video BIOS block and the FP registers (if applicable). */
BOOL RADEONGetBIOSInfo(struct ati_staticdata *sd)
{
    return FALSE;
}

BOOL RADEONGetConnectorInfoFromBIOS(struct ati_staticdata *sd)
{
    return FALSE;
}

/* Read PLL parameters from BIOS block.  Default to typical values if there
   is no BIOS. */
BOOL RADEONGetClockInfoFromBIOS(struct ati_staticdata *sd)
{
    return FALSE;
}
