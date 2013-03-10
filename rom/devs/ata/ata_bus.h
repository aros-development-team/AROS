/*
    Copyright © 2004-2013, The AROS Development Team. All rights reserved.
    $Id$

    Desc: private inline stubs for calling DMA and PIO vectors
    Lang: English
*/

static inline VOID PIO_Out(struct ata_Bus* bus, UBYTE val, UWORD offset)
{
    register void *obj = bus->pioInterface;
    struct ATA_BusInterface *vec = obj - sizeof(struct ATA_BusInterface);

    vec->ata_out(obj, val, offset);
}

static inline UBYTE PIO_In(struct ata_Bus* bus, UWORD offset)
{
    register void *obj = bus->pioInterface;
    struct ATA_BusInterface *vec = obj - sizeof(struct ATA_BusInterface);

    return vec->ata_in(obj, offset);
}

static inline VOID PIO_OutAlt(struct ata_Bus* bus, UBYTE val, UWORD offset)
{
    register void *obj = bus->pioInterface;
    struct ATA_BusInterface *vec = obj - sizeof(struct ATA_BusInterface);

    vec->ata_out_alt(obj, val, offset);
}

static inline UBYTE PIO_InAlt(struct ata_Bus* bus, UWORD offset)
{
    register void *obj = bus->pioInterface;
    struct ATA_BusInterface *vec = obj - sizeof(struct ATA_BusInterface);

    return vec->ata_in_alt(obj, offset);
}

static inline BOOL DMA_Setup(struct ata_Bus *bus, APTR buffer, IPTR size, BOOL read)
{
    register void *obj = bus->dmaInterface;
    struct ATA_DMAInterface *vec = obj - sizeof(struct ATA_DMAInterface);
    
    return vec->dma_Prepare(obj, buffer, size, read);
}

static inline void DMA_Start(struct ata_Bus *bus)
{
    register void *obj = bus->dmaInterface;
    struct ATA_DMAInterface *vec = obj - sizeof(struct ATA_DMAInterface);

    vec->dma_Start(obj);
}

static inline void DMA_End(struct ata_Bus *bus, APTR addr, IPTR len, BOOL read)
{
    register void *obj = bus->dmaInterface;
    struct ATA_DMAInterface *vec = obj - sizeof(struct ATA_DMAInterface);
    
    vec->dma_End(obj, addr, len, read);
}

static inline ULONG DMA_GetResult(struct ata_Bus *bus)
{
    register void *obj = bus->dmaInterface;
    struct ATA_DMAInterface *vec = obj - sizeof(struct ATA_DMAInterface);
    
    return vec->dma_Result(obj);
}

/* Convert data instance pointer back to OOP object pointer */
#define OOP_OBJECT(cl, data) (((void *)data) - cl->InstOffset)

static inline void Unit_Enable32Bit(struct ata_Unit *unit)
{
    struct ata_Bus *bus = unit->au_Bus;

    unit->au_UseModes |= AF_XFER_PIO32;
    unit->au_ins       = bus->pioVectors->ata_insl;
    unit->au_outs      = bus->pioVectors->ata_outsl;
}

static inline void Unit_Disable32Bit(struct ata_Unit *unit)
{
    struct ata_Bus *bus = unit->au_Bus;

    unit->au_UseModes &= ~AF_XFER_PIO32;
    unit->au_ins       = bus->pioVectors->ata_insw;
    unit->au_outs      = bus->pioVectors->ata_outsw;
}

static inline void Unit_OutS(struct ata_Unit *unit, APTR data, ULONG length)
{
    unit->au_outs(unit->pioInterface, data, length);
}

static inline void Unit_InS(struct ata_Unit *unit, APTR data, ULONG length)
{
    unit->au_ins(unit->pioInterface, data, length);
}
