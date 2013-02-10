#ifndef HIDD_ATA_H
#define HIDD_ATA_H

/*
    Copyright © 2013, The AROS Development Team. All rights reserved.
    $Id$

    Desc: ATA bus driver HIDD definitions
    Lang: english
*/

struct ata_BusDriver
{
    VOID  (*ata_out     )(UBYTE val, UWORD offset, struct ata_BusDriver *self);
    UBYTE (*ata_in      )(UWORD offset, struct ata_BusDriver *self);
    VOID  (*ata_outsw   )(APTR address, ULONG count, struct ata_BusDriver *self);
    VOID  (*ata_insw    )(APTR address, ULONG count, struct ata_BusDriver *self);
    /*
     * The following two are optional.
     * If they are NULL, our bus doesn't support 32-bit transfers.
     */
    VOID  (*ata_outsl   )(APTR address, ULONG count, struct ata_BusDriver *self);
    VOID  (*ata_insl    )(APTR address, ULONG count, struct ata_BusDriver *self);

    VOID  (*ata_out_alt )(UBYTE val, UWORD offset, struct ata_BusDriver *self);
    UBYTE (*ata_in_alt  )(UWORD offset, struct ata_BusDriver *self);
    /*
     * The following four are optional.
     * If they are NULL, our bus doesn't support DMA transfers
     */
    VOID  (*ata_out_dma )(UBYTE val, UWORD offset, struct ata_BusDriver *self);
    UBYTE (*ata_in_dma  )(UWORD offset, struct ata_BusDriver *self);
    VOID  (*ata_outl_dma)(ULONG val, UWORD offset, struct ata_BusDriver *self);
    ULONG (*ata_inl_dma )(UWORD offset, struct ata_BusDriver *self);
};

#include <interface/Hidd_ATA.h>

#endif
