/*
    Copyright (C) 2011-2014, The AROS Development Team. All rights reserved.

    Desc: dummy file
*/

#include "card_intern.h"

/*****************************************************************************
    NAME
        struct CardHandle * OwnCard(

    SYNOPSIS
        struct CardHandle *handle)

    FUNCTION
        Generic fallback: leave card ownership unchanged.

    INPUTS
        handle - Unused; not read, modified or retained.

    RESULT
        Always NULL. This does not indicate acquired ownership.

    BUGS
        This implementation does not register the handle or grant card
        access. Do not interpret NULL using the m68k-Amiga ownership
        convention.

*****************************************************************************/
AROS_LH1(struct CardHandle*, OwnCard,
         AROS_LHA(struct CardHandle*, handle, A1),
         struct CardResource*, CardResource, 1, Card)
{
    AROS_LIBFUNC_INIT

    return NULL;

    AROS_LIBFUNC_EXIT
}

/*****************************************************************************
    NAME
        void ReleaseCard(

    SYNOPSIS
        struct CardHandle *handle, ULONG flags)

    FUNCTION
        Generic fallback: perform no release operation.

    INPUTS
        handle - Unused.
        flags - Unused.

    RESULT
        None.

*****************************************************************************/
AROS_LH2(void, ReleaseCard,
         AROS_LHA(struct CardHandle*, handle, A1),
         AROS_LHA(ULONG, flags, D0),
         struct CardResource*, CardResource, 2, Card)
{
    AROS_LIBFUNC_INIT

    AROS_LIBFUNC_EXIT
}

/*****************************************************************************
    NAME
        struct CardMemoryMap * GetCardMap(

    SYNOPSIS
        )

    FUNCTION
        Return the memory-map structure embedded in the generic resource.

    RESULT
        A borrowed pointer to the resource-owned CardMemoryMap.

    NOTES
        Do not modify or free the structure. Its lifetime is that of the
        resource. This implementation does not populate hardware addresses
        or sizes; the returned pointer does not establish an accessible card
        mapping. The m68k-Amiga resource supplies a separate hardware map.

*****************************************************************************/
AROS_LH0(struct CardMemoryMap*, GetCardMap,
        struct CardResource*, CardResource, 3, Card)
{
    AROS_LIBFUNC_INIT

    return &CardResource->cmm;

    AROS_LIBFUNC_EXIT
}

/*****************************************************************************
    NAME
        BOOL BeginCardAccess(

    SYNOPSIS
        struct CardHandle *handle)

    FUNCTION
        Generic fallback: report that card access cannot begin.

    INPUTS
        handle - Unused.

    RESULT
        Always FALSE.

    NOTES
        No ownership check, locking or hardware access is performed.

*****************************************************************************/
AROS_LH1(BOOL, BeginCardAccess,
        AROS_LHA(struct CardHandle*, handle, A1),
        struct CardResource*, CardResource, 4, Card)
{
    AROS_LIBFUNC_INIT

    return FALSE;

    AROS_LIBFUNC_EXIT
}

/*****************************************************************************
    NAME
        BOOL EndCardAccess(

    SYNOPSIS
        struct CardHandle *handle)

    FUNCTION
        Generic fallback: report that card access cannot end.

    INPUTS
        handle - Unused.

    RESULT
        Always FALSE.

    NOTES
        No ownership check, locking or hardware access is performed.

*****************************************************************************/
AROS_LH1(BOOL, EndCardAccess,
        AROS_LHA(struct CardHandle*, handle, A1),
        struct CardResource*, CardResource, 5, Card)
{
    AROS_LIBFUNC_INIT

    return FALSE;

    AROS_LIBFUNC_EXIT
}

/*****************************************************************************
    NAME
        UBYTE ReadCardStatus(

    SYNOPSIS
        )

    FUNCTION
        Generic fallback: return no card-status bits.

    RESULT
        Always zero. No hardware status is read.

*****************************************************************************/
AROS_LH0(UBYTE, ReadCardStatus,
         struct CardResource*, CardResource, 6, Card)
{
    AROS_LIBFUNC_INIT

    return 0;

    AROS_LIBFUNC_EXIT
}

/*****************************************************************************
    NAME
        BOOL CardResetRemove(

    SYNOPSIS
        struct CardHandle *handle, ULONG flag)

    FUNCTION
        Generic fallback: leave removal-reset behaviour unchanged.

    INPUTS
        handle - Unused.
        flag - Unused.

    RESULT
        Always FALSE.

*****************************************************************************/
AROS_LH2(BOOL, CardResetRemove,
        AROS_LHA(struct CardHandle*, handle, A1),
        AROS_LHA(ULONG, flag, D0),
        struct CardResource*, CardResource, 7, Card)
{
    AROS_LIBFUNC_INIT

    return FALSE;

    AROS_LIBFUNC_EXIT
}

/*****************************************************************************
    NAME
        UBYTE CardMiscControl(

    SYNOPSIS
        struct CardHandle *handle, UBYTE control_bits)

    FUNCTION
        Generic fallback: leave card controls unchanged.

    INPUTS
        handle - Unused.
        control_bits - Unused.

    RESULT
        Always zero.

*****************************************************************************/
AROS_LH2(UBYTE, CardMiscControl,
        AROS_LHA(struct CardHandle*, handle, A1),
        AROS_LHA(UBYTE, control_bits, D1),
        struct CardResource*, CardResource, 8, Card)
{
    AROS_LIBFUNC_INIT

    return 0;

    AROS_LIBFUNC_EXIT
}

/*****************************************************************************
    NAME
        ULONG CardAccessSpeed(

    SYNOPSIS
        struct CardHandle *handle, ULONG nanoseconds)

    FUNCTION
        Generic fallback: reject a card-access timing request.

    INPUTS
        handle - Unused.
        nanoseconds - Unused.

    RESULT
        Always zero. No access speed is selected.

*****************************************************************************/
AROS_LH2(ULONG, CardAccessSpeed,
        AROS_LHA(struct CardHandle*, handle, A1),
        AROS_LHA(ULONG, nanoseconds, D0),
        struct CardResource*, CardResource, 9, Card)
{
    AROS_LIBFUNC_INIT

    return 0;

    AROS_LIBFUNC_EXIT
}

/*****************************************************************************
    NAME
        LONG CardProgramVoltage(

    SYNOPSIS
        struct CardHandle *handle, ULONG voltage)

    FUNCTION
        Generic fallback: reject a programming-voltage request.

    INPUTS
        handle - Unused.
        voltage - Unused.

    RESULT
        Always -1. No voltage is changed.

*****************************************************************************/
AROS_LH2(LONG, CardProgramVoltage,
        AROS_LHA(struct CardHandle*, handle, A1),
        AROS_LHA(ULONG, voltage, D0),
        struct CardResource*, CardResource, 10, Card)
{
    AROS_LIBFUNC_INIT

    return -1;

    AROS_LIBFUNC_EXIT
}

/*****************************************************************************
    NAME
        BOOL CardResetCard(

    SYNOPSIS
        struct CardHandle *handle)

    FUNCTION
        Generic fallback: perform no card reset.

    INPUTS
        handle - Unused.

    RESULT
        Always FALSE.

*****************************************************************************/
AROS_LH1(BOOL, CardResetCard,
        AROS_LHA(struct CardHandle*, handle, A1),
        struct CardResource*, CardResource, 11, Card)
{
    AROS_LIBFUNC_INIT

    return FALSE;

    AROS_LIBFUNC_EXIT
}

/*****************************************************************************
    NAME
        BOOL CopyTuple(

    SYNOPSIS
        struct CardHandle *handle, UBYTE *buffer, ULONG tuplecode, ULONG size)

    FUNCTION
        Generic fallback: copy no card tuple.

    INPUTS
        handle - Unused.
        buffer - Unused; not written.
        tuplecode - Unused.
        size - Unused.

    RESULT
        Always FALSE. The output buffer is unchanged.

*****************************************************************************/
AROS_LH4(BOOL, CopyTuple,
        AROS_LHA(struct CardHandle*, handle, A1),
        AROS_LHA(UBYTE*, buffer, A0),
        AROS_LHA(ULONG, tuplecode, D1),
        AROS_LHA(ULONG, size, D0),
        struct CardResource*, CardResource, 12, Card)
{
    AROS_LIBFUNC_INIT

    return FALSE;

    AROS_LIBFUNC_EXIT
}

/*****************************************************************************
    NAME
        ULONG DeviceTuple(

    SYNOPSIS
        UBYTE *tuple_data, struct DeviceTData *storage)

    FUNCTION
        Generic fallback: decode no device tuple.

    INPUTS
        tuple_data - Unused; not read.
        storage - Unused; not written.

    RESULT
        Always zero. The output structure is unchanged.

*****************************************************************************/
AROS_LH2(BOOL, DeviceTuple,
        AROS_LHA(UBYTE*, tuple_data, A0),
        AROS_LHA(struct DeviceTData*, storage, A1),
        struct CardResource*, CardResource, 13, Card)
{
    AROS_LIBFUNC_INIT

    return FALSE;

    AROS_LIBFUNC_EXIT
}

/*****************************************************************************
    NAME
        struct Resident * IfAmigaXIP(

    SYNOPSIS
        struct CardHandle *handle)

    FUNCTION
        Generic fallback: return no execute-in-place resident.

    INPUTS
        handle - Unused.

    RESULT
        Always NULL.

*****************************************************************************/
AROS_LH1(struct Resident*, IfAmigaXIP,
        AROS_LHA(struct CardHandle*, handle, A2),
        struct CardResource*, CardResource, 14, Card)
{
    AROS_LIBFUNC_INIT

    return NULL;

    AROS_LIBFUNC_EXIT
}

/*****************************************************************************
    NAME
        ULONG CardChangeCount(

    SYNOPSIS
        )

    FUNCTION
        Generic fallback: return no recorded card changes.

    RESULT
        Always zero. No change counter is maintained.

*****************************************************************************/
AROS_LH0(ULONG, CardChangeCount,
        struct CardResource*, CardResource, 16, Card)
{
    AROS_LIBFUNC_INIT

    return 0;

    AROS_LIBFUNC_EXIT
}

/*****************************************************************************
    NAME
        BOOL CardForceChange(

    SYNOPSIS
        )

    FUNCTION
        Generic fallback: generate no card-change notification.

    RESULT
        Always FALSE.

*****************************************************************************/
AROS_LH0(BOOL, CardForceChange,
        struct CardResource*, CardResource, 15, Card)
{
    AROS_LIBFUNC_INIT

    return FALSE;

    AROS_LIBFUNC_EXIT
}

/*****************************************************************************
    NAME
        ULONG CardInterface(

    SYNOPSIS
        )

    FUNCTION
        Identify the generic resource as having no supported card interface.

    RESULT
        (ULONG)-1. This is not CARD_INTERFACE_AMIGA_0.

*****************************************************************************/
AROS_LH0(ULONG, CardInterface,
        struct CardResource*, CardResource, 17, Card)
{
    AROS_LIBFUNC_INIT

    return -1;

    AROS_LIBFUNC_EXIT
}

