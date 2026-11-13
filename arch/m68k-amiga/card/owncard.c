/*
    Copyright (C) 1995-2014, The AROS Development Team. All rights reserved.

    Desc: OwnCard() function.
*/

#include <proto/exec.h>

#include "card_intern.h"

/*****************************************************************************
    NAME
        struct CardHandle * OwnCard(

    SYNOPSIS
        struct CardHandle *handle)

    FUNCTION
        Request ownership of the m68k-Amiga PCMCIA slot, or queue a handle
        for a later card.

    INPUTS
        handle - Writable, caller-owned CardHandle not already linked into
        the resource. Initialize its node priority, flags and callback
        pointers; use NULL for absent callbacks. Keep the handle, callback
        structures, code and callback data alive while registered.

    RESULT
        With CARDF_DELAYOWNERSHIP, returns (struct CardHandle *)-1 after
        queueing the handle, even if ownership and its insertion callback
        occur immediately. Otherwise returns -1 when the card is marked
        removed. With CARDF_IFAVAILABLE and an existing owner, returns that
        owner's borrowed handle. Other paths return NULL; see BUGS.

    NOTES
        CARDF_DELAYOWNERSHIP takes precedence over CARDF_IFAVAILABLE and
        queues by node priority. An immediate claim adds the handle at the
        head of the queue without calling its insertion callback.
        CARDF_RESETREMOVE selects machine-reset behaviour on card-detect
        changes when ownership is granted. CARDF_POSTSTATUS requests a
        second status callback with mask zero after interrupt
        acknowledgement.

        Insertion and removal callbacks execute with task switching
        forbidden, from resource-task or caller context. They must not wait.
        The status callback executes in interrupt context; it returns the
        interrupt mask to acknowledge. Use the AROS_CARDH/AROS_CARDFUNC and
        AROS_CARDC conventions from resources/card.h.

        Remove a registered handle with ReleaseCard(handle,
        CARDF_REMOVEHANDLE) before freeing its storage. Serialize
        registration and release; the ownership decision and registration
        are not one atomic operation.

    BUGS
        Without CARDF_IFAVAILABLE or CARDF_DELAYOWNERSHIP, an existing owner
        causes NULL to be returned without registering or acquiring the
        supplied handle. NULL alone therefore does not prove ownership;
        BeginCardAccess() checks the current handle. Re-registering a linked
        handle can corrupt the queue.

*****************************************************************************/
AROS_LH1(struct CardHandle*, OwnCard,
         AROS_LHA(struct CardHandle*, handle, A1),
         struct CardResource*, CardResource, 1, Card)
{
    AROS_LIBFUNC_INIT
    
    struct CardHandle *ret = NULL;

    CARDDEBUG(bug("OwnCard(%p,%08x)\n", handle, handle->cah_CardFlags));

    handle->cah_CardFlags &= ~CARDF_USED;
    if (handle->cah_CardFlags & CARDF_DELAYOWNERSHIP) {
        Forbid();
        Enqueue(&CardResource->handles, &handle->cah_CardNode);
        Permit();
        pcmcia_newowner(CardResource, TRUE);
        ret = (struct CardHandle*)-1;
    } else if (handle->cah_CardFlags & CARDF_IFAVAILABLE) {
        if (CardResource->removed)
            ret = (struct CardHandle*)-1;
        else if (CardResource->ownedcard)
            ret = CardResource->ownedcard;
    }

    if (ret == NULL) {
        if (CardResource->removed)
            ret = (struct CardHandle*)-1;
        else if (CardResource->ownedcard == NULL) {
            Forbid();
            AddHead(&CardResource->handles, &handle->cah_CardNode);
            Permit();
            pcmcia_newowner(CardResource, FALSE);
        } else
            ret = 0;
    }
        
    CARDDEBUG(bug("=%p\n", ret));

    return ret;

    AROS_LIBFUNC_EXIT
}
