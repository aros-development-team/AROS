/*
    Copyright (C) 1995-2014, The AROS Development Team. All rights reserved.

    Desc: ReleaseCard() function.
*/

#include <proto/exec.h>

#include "card_intern.h"

/*****************************************************************************
    NAME
        void ReleaseCard(

    SYNOPSIS
        struct CardHandle *handle, ULONG flags)

    FUNCTION
        Release a handle's m68k-Amiga card ownership and optionally unlink
        the handle.

    INPUTS
        handle - Valid, caller-owned CardHandle. For CARDF_REMOVEHANDLE, it
        must currently be linked into this resource.
        flags - CARDF_REMOVEHANDLE unlinks the handle; other bits are
        ignored.

    RESULT
        None.

    NOTES
        If handle owns the card, ownership is cleared and card configuration
        and interrupts are reset. When a card remains inserted, another
        eligible handle can receive ownership and its insertion callback
        before this call returns. Without CARDF_REMOVEHANDLE, the released
        handle remains queued but is marked used for the current card.

        No handle or callback storage is freed. Serialize release with other
        operations on the handle. Call only after registration is known to
        have occurred; a failed OwnCard() attempt does not necessarily
        register it.

        A removal callback must arrange to release ownership: notifying a
        client of removal does not itself clear the resource's owner
        pointer.

    BUGS
        Unlinking is unconditional when CARDF_REMOVEHANDLE is set. Passing
        an unregistered or already removed handle can corrupt a list.

*****************************************************************************/
AROS_LH2(void, ReleaseCard,
         AROS_LHA(struct CardHandle*, handle, A1),
         AROS_LHA(ULONG, flags, D0),
         struct CardResource*, CardResource, 2, Card)
{
    AROS_LIBFUNC_INIT

    CARDDEBUG(bug("ReleaseCard(%p,%08x) flags=%02x owned=%p removed=%d\n",
        handle, flags, handle->cah_CardFlags, CardResource->ownedcard, CardResource->removed));

    handle->cah_CardFlags &= ~CARDF_USED;
    if (CardResource->ownedcard == handle) {
        if (CardResource->removed == FALSE)
            handle->cah_CardFlags |= CARDF_USED;
        CardResource->ownedcard = NULL;
        pcmcia_reset(CardResource);
        pcmcia_enable_interrupts();

        if (CardResource->removed == FALSE)
            pcmcia_newowner(CardResource, TRUE);
    }

    if (flags & CARDF_REMOVEHANDLE) {
        Forbid();
        Remove(&handle->cah_CardNode);
        Permit();
        handle->cah_CardFlags &= ~CARDF_USED;
    }

    AROS_LIBFUNC_EXIT
}
