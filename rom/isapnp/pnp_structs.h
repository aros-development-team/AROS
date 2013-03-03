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

#ifndef	ISA_PNP_pnp_structs_h
#define ISA_PNP_pnp_structs_h

#include "CompilerSpecific.h"

#include <exec/types.h>

struct ISAPNPBase;

struct ISAPNP_Card;
struct ISAPNP_Device;
struct ISAPNP_ResourceGroup;
struct ISAPNP_Resource;

AROS_LD0(struct ISAPNP_Card *, ISAPNP_AllocCard,
         struct ISAPNPBase *, res, 18, ISAPNP);

AROS_LD1(void, ISAPNP_FreeCard,
         AROS_LHA(struct ISAPNP_Card *, card, A0),
         struct ISAPNPBase *, res, 19, ISAPNP);

AROS_LD0(struct ISAPNP_Device *, ISAPNP_AllocDevice,
         struct ISAPNPBase *, res, 20, ISAPNP);

AROS_LD1(void, ISAPNP_FreeDevice,
         AROS_LHA(struct ISAPNP_Device *, dev, A0),
         struct ISAPNPBase *, res, 21, ISAPNP);

AROS_LD1(struct ISAPNP_ResourceGroup *, ISAPNP_AllocResourceGroup,
         AROS_LHA(UBYTE, pri, D0),
         struct ISAPNPBase *, res, 22, ISAPNP);

AROS_LD1(void, ISAPNP_FreeResourceGroup,
         AROS_LHA(struct ISAPNP_ResourceGroup *, rg, A0),
         struct ISAPNPBase *, res, 23, ISAPNP);

AROS_LD1(struct ISAPNP_Resource *, ISAPNP_AllocResource,
         AROS_LHA(UBYTE, type, D0),
         struct ISAPNPBase *, res, 24, ISAPNP);

AROS_LD1(void, ISAPNP_FreeResource,
         AROS_LHA(struct ISAPNP_Resource *, r, A0),
         struct ISAPNPBase *, res, 25, ISAPNP);

static inline struct ISAPNP_Card *ISAPNP_AllocCard(struct ISAPNPBase *res)
{
        return AROS_LC0(struct ISAPNP_Card *, ISAPNP_AllocCard,
                        struct ISAPNPBase *, res, 18, ISAPNP);
}

static inline void ISAPNP_FreeCard(struct ISAPNP_Card *card, struct ISAPNPBase *res)
{
        AROS_LC1(void, ISAPNP_FreeCard,
                 AROS_LHA(struct ISAPNP_Card *, card, A0),
                 struct ISAPNPBase *, res, 19, ISAPNP);
}

static inline struct ISAPNP_Device *ISAPNP_AllocDevice(struct ISAPNPBase *res)
{
        return AROS_LC0(struct ISAPNP_Device *, ISAPNP_AllocDevice,
                        struct ISAPNPBase *, res, 20, ISAPNP);
}

static inline void ISAPNP_FreeDevice(struct ISAPNP_Device *dev,
                                     struct ISAPNPBase *res)
{
        return AROS_LC1(void, ISAPNP_FreeDevice,
                        AROS_LHA(struct ISAPNP_Device *, dev, A0),
                        struct ISAPNPBase *, res, 21, ISAPNP);
}

static inline struct ISAPNP_ResourceGroup *ISAPNP_AllocResourceGroup(UBYTE pri,
                                                                     struct ISAPNPBase *res)
{
        return AROS_LC1(struct ISAPNP_ResourceGroup *, ISAPNP_AllocResourceGroup,
                        AROS_LHA(UBYTE, pri, D0),
                        struct ISAPNPBase *, res, 22, ISAPNP);
}

static inline void ISAPNP_FreeResourceGroup(struct ISAPNP_ResourceGroup *rg,
                                            struct ISAPNPBase *res)
{
        AROS_LC1(void, ISAPNP_FreeResourceGroup,
                 AROS_LHA(struct ISAPNP_ResourceGroup *, rg, A0),
                 struct ISAPNPBase *, res, 23, ISAPNP);
}

static inline struct ISAPNP_Resource *ISAPNP_AllocResource(UBYTE type,
                                                           struct ISAPNPBase *res)
{
        return AROS_LC1(struct ISAPNP_Resource *, ISAPNP_AllocResource,
                        AROS_LHA(UBYTE, type, D0),
                        struct ISAPNPBase *, res, 24, ISAPNP);
}

static inline void ISAPNP_FreeResource(struct ISAPNP_Resource *r,
                                       struct ISAPNPBase *res)
{
        AROS_LC1(void, ISAPNP_FreeResource,
                 AROS_LHA(struct ISAPNP_Resource *, r, A0),
                 struct ISAPNPBase *, res, 25, ISAPNP);
}

#endif /* ISA_PNP_pnp_structs_h */
