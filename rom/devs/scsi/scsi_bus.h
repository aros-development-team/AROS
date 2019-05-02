/*
    Copyright © 2019, The AROS Development Team. All rights reserved.
    $Id$

    Desc: private inline stubs for calling DMA and PIO vectors
    Lang: English
*/

static inline VOID PIO_Out(struct scsi_Bus* bus, UBYTE val, UWORD offset)
{
    register void *obj = bus->pioInterface;
    struct SCSI_BusInterface *vec = obj - sizeof(struct SCSI_BusInterface);

    vec->scsi_out(obj, val, offset);
}

static inline UBYTE PIO_In(struct scsi_Bus* bus, UWORD offset)
{
    register void *obj = bus->pioInterface;
    struct SCSI_BusInterface *vec = obj - sizeof(struct SCSI_BusInterface);

    return vec->scsi_in(obj, offset);
}

static inline VOID PIO_OutAlt(struct scsi_Bus* bus, UBYTE val, UWORD offset)
{
    register void *obj = bus->pioInterface;
    struct SCSI_BusInterface *vec = obj - sizeof(struct SCSI_BusInterface);

    vec->scsi_out_alt(obj, val, offset);
}

static inline UBYTE PIO_InAlt(struct scsi_Bus* bus, UWORD offset)
{
    register void *obj = bus->pioInterface;
    struct SCSI_BusInterface *vec = obj - sizeof(struct SCSI_BusInterface);

    return vec->scsi_in_alt(obj, offset);
}

static inline BOOL DMA_Setup(struct scsi_Bus *bus, APTR buffer, IPTR size, BOOL read)
{
    register void *obj = bus->dmaInterface;
    struct SCSI_DMAInterface *vec = obj - sizeof(struct SCSI_DMAInterface);
    
    return vec->dma_Prepare(obj, buffer, size, read);
}

static inline void DMA_Start(struct scsi_Bus *bus)
{
    register void *obj = bus->dmaInterface;
    struct SCSI_DMAInterface *vec = obj - sizeof(struct SCSI_DMAInterface);

    vec->dma_Start(obj);
}

static inline void DMA_End(struct scsi_Bus *bus, APTR addr, IPTR len, BOOL read)
{
    register void *obj = bus->dmaInterface;
    struct SCSI_DMAInterface *vec = obj - sizeof(struct SCSI_DMAInterface);
    
    vec->dma_End(obj, addr, len, read);
}

static inline ULONG DMA_GetResult(struct scsi_Bus *bus)
{
    register void *obj = bus->dmaInterface;
    struct SCSI_DMAInterface *vec = obj - sizeof(struct SCSI_DMAInterface);
    
    return vec->dma_Result(obj);
}

/* Convert data instance pointer back to OOP object pointer */
#define OOP_OBJECT(cl, data) (((void *)data) - cl->InstOffset)

static inline void Unit_Enable32Bit(struct scsi_Unit *unit)
{
    struct scsi_Bus *bus = unit->su_Bus;

    unit->su_UseModes |= AF_XFER_PIO32;
    unit->su_ins       = bus->pioVectors->scsi_insl;
    unit->su_outs      = bus->pioVectors->scsi_outsl;
}

static inline void Unit_Disable32Bit(struct scsi_Unit *unit)
{
    struct scsi_Bus *bus = unit->su_Bus;

    unit->su_UseModes &= ~AF_XFER_PIO32;
    unit->su_ins       = bus->pioVectors->scsi_insw;
    unit->su_outs      = bus->pioVectors->scsi_outsw;
}

static inline void Unit_OutS(struct scsi_Unit *unit, APTR data, ULONG length)
{
    unit->su_outs(unit->pioInterface, data, length);
}

static inline void Unit_InS(struct scsi_Unit *unit, APTR data, ULONG length)
{
    unit->su_ins(unit->pioInterface, data, length);
}
