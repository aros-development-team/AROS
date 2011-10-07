/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Generic AROS APIC definitions.
    Lang: english
*/

ULONG core_APIC_Wake(APTR start_addr, UBYTE id, IPTR base);
IPTR  core_APIC_GetBase(void);
UBYTE core_APIC_GetID(IPTR base);
BOOL  core_APIC_Init(IPTR base);
void  core_APIC_AckIntr(void);
