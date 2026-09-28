#ifndef _VIDEOCOREGFX_DDC_H
#define _VIDEOCOREGFX_DDC_H
/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/

#include <exec/types.h>

#define VCGFX_DDC_EDID      0x50        /* 7-bit slave addresses */
#define VCGFX_DDC_SCDC      0x54

BOOL vcgfx_ddc_connected(ULONG port);
BOOL vcgfx_ddc_read(ULONG port, UBYTE slave, UBYTE offset, UBYTE *buf, ULONG len);
BOOL vcgfx_ddc_write(ULONG port, UBYTE slave, const UBYTE *buf, ULONG len);

#endif /* _VIDEOCOREGFX_DDC_H */
