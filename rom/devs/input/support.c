/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$

    Desc: 
    Lang: english
*/


#include <proto/exec.h>
#include <exec/tasks.h>
#include <exec/lists.h>
#include <exec/memory.h>
#include <devices/rawkeycodes.h>

#include "input_intern.h"

/***************************
**  GetEventsFromQueue()  **
***************************/
struct InputEvent *GetEventsFromQueue(struct inputbase *InputDevice)
{
    struct InputEvent *ie;

    ie = InputDevice->EventQueueHead;

    /* No more events in the queue */
    InputDevice->EventQueueHead = NULL;
    InputDevice->EventQueueTail = NULL;

    return (ie);
}



/******************
**  AddEQTail()  **
******************/
/* Adds a chian of InputEvents to the eventqueue */
VOID AddEQTail(struct InputEvent *ie, struct inputbase *InputDevice)
{
    UWORD code;

    /* Perform any necessary modifications to event */
    switch (ie->ie_Class)
    {
    case IECLASS_RAWKEY:
        /* Add previous codes and qualifiers to the event for dead key
           handling */
        ie->ie_Prev1DownCode = InputDevice->Prev1DownCode;
        ie->ie_Prev1DownQual = InputDevice->Prev1DownQual;
        ie->ie_Prev2DownCode = InputDevice->Prev2DownCode;
        ie->ie_Prev2DownQual = InputDevice->Prev2DownQual;

        /* Cache/rotate old codes and qualifiers for next event */
        if ((ie->ie_Code & IECODE_UP_PREFIX) == 0
            && !(ie->ie_Code >= RAWKEY_LSHIFT
                && ie->ie_Code <= RAWKEY_RAMIGA))
        {
            InputDevice->Prev2DownCode = InputDevice->Prev1DownCode;
            InputDevice->Prev2DownQual = InputDevice->Prev1DownQual;
            InputDevice->Prev1DownCode = (UBYTE) ie->ie_Code;
            InputDevice->Prev1DownQual = (UBYTE) ie->ie_Qualifier;
        }
        break;

        /* FIXME: add more event classes to which this is applicable */
    case IECLASS_RAWMOUSE:
        /* Experimental */
        if (InputDevice->pub.id_Flags & IDF_SWAP_BUTTONS)
        {
            code = ie->ie_Code & ~IECODE_UP_PREFIX;

            switch (code)
            {
            case IECODE_LBUTTON:
                code = IECODE_RBUTTON;
                break;
            case IECODE_RBUTTON:
                code = IECODE_LBUTTON;
                break;
            }
            ie->ie_Code = code | (ie->ie_Code & IECODE_UP_PREFIX);
        }
    }

    if (!InputDevice->EventQueueHead)   /* Empty queue? */
    {
        InputDevice->EventQueueHead = ie;
    }
    else
    {
        InputDevice->EventQueueTail->ie_NextEvent = ie;
    }

    /* Get last event in eventchain to add */
    while (ie->ie_NextEvent)
        ie = ie->ie_NextEvent;

    InputDevice->EventQueueTail = ie;

    return;
}

/*********************
**  IsQualifierKey  **
*********************/
BOOL IsQualifierKey(UWORD key)
{
    BOOL result = FALSE;

    key &= ~IECODE_UP_PREFIX;

    if ((key >= 0x60) && (key <= 0x67))
    {
        result = TRUE;
    }

    return result;
}

/************************
**  IsKeyRepeatable()  **
************************/
BOOL IsRepeatableKey(UWORD key)
{
    BOOL result = TRUE;

    key &= ~IECODE_UP_PREFIX;

    /* stegerg: It looks like this is really all so no need to
       check the keymap->repeatable tables. I have checked this
       on the Amiga. All keys except the qualifier keys are
       treated as repeatable raw (!!!) keys. Here in input.device
       we have only raw keys. For vanilla keys there's MapRawKey()
       and this function takes care of filtering out repeated
       raw keys if the corresponding KeyMap->repeatable table says
       that this key is not repeatable */

    if (IsQualifierKey(key))
    {
        result = FALSE;
    }

    return result;
}
