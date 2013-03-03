/* $Id$ */

/*
     ISA-PnP -- A Plug And Play ISA software layer for AmigaOS.
     Copyright (C) 2001 Martin Blom <martin@blom.org>
     Copyright (C) 2009-2013 The AROS Development Team

     This library is free software; you can redistribute it and/or
     modify it under the terms of the GNU Library General Public
     License as published by the Free Software Foundation; either
     version 2 of the License, or (at your option) any later version.
     
     This library is distributed in the hope that it will be useful,
     but WITHOUT ANY WARRANTY; without even the implied warranty of
     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
     Library General Public License for more details.
     
     You should have received a copy of the GNU Library General Public
     License along with this library; if not, write to the
     Free Software Foundation, Inc., 59 Temple Place - Suite 330, Cambridge,
     MA 02139, USA.
*/

#ifndef	ISA_PNP_devices_h
#define ISA_PNP_devices_h

#include "CompilerSpecific.h"

#include <exec/types.h>

struct ISAPNPBase;
struct ISAPNP_Card;
struct ISAPNP_Device;

AROS_LD5(struct ISAPNP_Card *, ISAPNP_FindCard,
         AROS_LHA(struct ISAPNP_Card *, last_card, A0), 
         AROS_LHA(LONG, manufacturer, D0),
         AROS_LHA(WORD, product, D1),
         AROS_LHA(BYTE, revision, D2),
         AROS_LHA(LONG, serial, D3),
         struct ISAPNPBase *, res, 28, ISAPNP);

AROS_LD4(struct ISAPNP_Device *, ISAPNP_FindDevice,
         AROS_LHA(struct ISAPNP_Device *, last_device, A0), 
         AROS_LHA(LONG, manufacturer, D0),
         AROS_LHA(WORD, product, D1),
         AROS_LHA(BYTE, revision, D2),
         struct ISAPNPBase *, res, 29, ISAPNP);

AROS_LD2(APTR, ISAPNP_LockCardsA,
         AROS_LHA(ULONG, flags, D0),
         AROS_LHA(struct ISAPNP_Card **, cards, A0),
         struct ISAPNPBase *, res, 30, ISAPNP);

AROS_LD1(void, ISAPNP_UnlockCards,
         AROS_LHA(APTR, card_lock_handle, A0),
         struct ISAPNPBase *, res, 31, ISAPNP);

AROS_LD2(APTR, ISAPNP_LockDevicesA,
         AROS_LHA(ULONG, flags, D0),
         AROS_LHA(struct ISAPNP_Device **, devices, A0),
         struct ISAPNPBase *, res, 32, ISAPNP);

AROS_LD1(void, ISAPNP_UnlockDevices,
         AROS_LHA(APTR, device_lock_handle, A0),
         struct ISAPNPBase *, res , 33, ISAPNP);

static inline struct ISAPNP_Device *ISAPNP_FindDevice(struct ISAPNP_Device *last_device,
                                                      LONG manufacturer, WORD product,
                                                      BYTE revision, struct ISAPNPBase *res)
{
        return AROS_LC4(struct ISAPNP_Device *, ISAPNP_FindDevice,
                        AROS_LHA(struct ISAPNP_Device *, last_device, A0), 
                        AROS_LHA(LONG, manufacturer, D0),
                        AROS_LHA(WORD, product, D1),
                        AROS_LHA(BYTE, revision, D2),
                        struct ISAPNPBase *, res, 29, ISAPNP);
}

#endif /* ISA_PNP_devices_h */
