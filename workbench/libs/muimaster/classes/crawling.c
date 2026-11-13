/*
    Copyright (C) 2002-2026, The AROS Development Team. All rights reserved.
*/

#define MUIMASTER_YES_INLINE_STDARG

#include <exec/memory.h>
#include <clib/alib_protos.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/utility.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>

#include <string.h>
#include <stdio.h>

#include "mui.h"
#include "muimaster_intern.h"
#include "support.h"
#include "support_classes.h"
#include "debug.h"
#include "crawling_private.h"

/****** Crawling.mui/MUIC_Crawling ************
*
*   NAME
*       MUIC_Crawling
*
*   SUPERCLASS
*       Virtgroup.mui
*
*   DESCRIPTION
*       Scroll a virtual group upward by one pixel for each received Intuition
*       tick after an initial delay. The class name is Crawling.mcc. No
*       Crawling-specific attributes or methods are declared by its header;
*       child ownership and layout follow Virtgroup/Group.
*       Setup delegates to Virtgroup and sets the countdown to fifty tick
*       messages, even if superclass setup reports failure. Show delegates,
*       enables IDCMP_INTUITICKS and registers a priority-zero event handler
*       even when superclass Show returns zero. Hide removes that handler,
*       clears the event bit and delegates. Pair Show and Hide and keep the
*       parent Window valid through removal; this class has no separate
*       Cleanup/Dispose override to remove its handler.
*       Each IDCMP_INTUITICKS message decrements a nonzero countdown. On the
*       message that reaches zero, and every subsequent tick, the handler reads
*       Virtgroup_Top and Virtgroup_Height. If virtual height is at least the
*       inner object height, it adds one to Top and wraps to zero when the
*       viewport would extend beyond the virtual height. Equal heights therefore
*       keep setting zero; shorter content is left unchanged.
*       The delay counts delivered tick messages, not DOS Delay ticks or a
*       guaranteed wall-clock duration. It resets on Setup, not Show or each
*       wrap. HandleEvent returns zero for all messages, including handled
*       ticks. Scroll setters can notify synchronously; avoid disposal or
*       conflicting mutations from active callbacks.
*
*/

extern struct Library *MUIMasterBase;

/*
 * Create Virtgroup and initialize an event handler with no event bits and
 * priority zero. Return object or zero from superclass construction. Own no
 * separate resources beyond superclass children; register handler later in
 * Show.
 */
IPTR Crawling__OM_NEW(struct IClass *cl, Object *obj, struct opSet *msg)
{
    obj = (Object *) DoSuperNewTags
        (cl, obj, NULL, TAG_MORE, (IPTR) msg->ops_AttrList);

    if (obj)
    {
        struct Crawling_DATA *data = INST_DATA(cl, obj);

        data->ehn.ehn_Events = 0;
        data->ehn.ehn_Priority = 0;
        data->ehn.ehn_Flags = 0;
        data->ehn.ehn_Object = obj;
        data->ehn.ehn_Class = cl;

    }

    return (IPTR) obj;
}

/*
 * Delegate Virtgroup Setup, reset ticker to fifty even on failure, and
 * return superclass result. Requires valid lifecycle message. Does not
 * register an event handler or reset scroll position.
 */
IPTR Crawling__MUIM_Setup(struct IClass *cl, Object *obj,
    struct MUIP_Setup *msg)
{
    struct Crawling_DATA *data = INST_DATA(cl, obj);
    IPTR retval;

    retval = DoSuperMethodA(cl, obj, (Msg) msg);

    data->ticker = CRAWLING_INITIAL_DELAY;

    return retval;
}

/*
 * Delegate Show, enable INTUITICKS and register handler on live parent
 * Window regardless of superclass result; return that result. Requires
 * paired Hide and valid connected window. Does not reset countdown or check
 * registration status.
 */
IPTR Crawling__MUIM_Show(struct IClass *cl, Object *obj,
    struct MUIP_Show *msg)
{
    struct Crawling_DATA *data = INST_DATA(cl, obj);
    IPTR retval;

    retval = DoSuperMethodA(cl, obj, (Msg) msg);

    data->ehn.ehn_Events |= IDCMP_INTUITICKS;
    DoMethod(_win(obj), MUIM_Window_AddEventHandler, (IPTR) & data->ehn);

    return retval;
}

/*
 * Remove handler from live parent Window, clear INTUITICKS bit, then return
 * Virtgroup Hide result. Requires paired Show and no concurrent handler
 * dispatch; no other local teardown path removes this handler.
 */
IPTR Crawling__MUIM_Hide(struct IClass *cl, Object *obj,
    struct MUIP_Hide *msg)
{
    struct Crawling_DATA *data = INST_DATA(cl, obj);

    DoMethod(_win(obj), MUIM_Window_RemEventHandler, (IPTR) & data->ehn);
    data->ehn.ehn_Events &= ~IDCMP_INTUITICKS;

    return DoSuperMethodA(cl, obj, (Msg) msg);

}

/*
 * For an INTUITICKS message, count down then increment/wrap virtual Top
 * when content height is at least viewport height. Requires live rendered
 * object; imsg may be NULL. Get statuses ignored with zero-initialized
 * storage; Set can notify synchronously. Return zero for all events; no
 * wall-clock timing guarantee.
 */
IPTR Crawling__MUIM_HandleEvent(struct IClass *cl, Object *obj,
    struct MUIP_HandleEvent *msg)
{
    if (msg->imsg)
    {
        if (msg->imsg->Class == IDCMP_INTUITICKS)
        {
            struct Crawling_DATA *data = INST_DATA(cl, obj);

            if (data->ticker)
                data->ticker--;

            if (data->ticker == 0)
            {
                IPTR virty = 0, virth = 0;

                get(obj, MUIA_Virtgroup_Top, &virty);
                get(obj, MUIA_Virtgroup_Height, &virth);

                if (virth >= _mheight(obj))
                {
                    virty++;
                    if (virty + _mheight(obj) > virth)
                        virty = 0;

                    set(obj, MUIA_Virtgroup_Top, virty);
                }
            }
        }
    }

    return 0;
}

#if ZUNE_BUILTIN_CRAWLING
/*
 * Dispatch local overrides and return handler results; delegate other
 * messages to the declared superclass. Requires correct method ABI and live
 * object except construction. No implicit additional ownership,
 * synchronization or conversion is supplied by this dispatcher.
 */
BOOPSI_DISPATCHER(IPTR, Crawling_Dispatcher, cl, obj, msg)
{
    switch (msg->MethodID)
    {
    case OM_NEW:
        return Crawling__OM_NEW(cl, obj, (struct opSet *)msg);
    case MUIM_Setup:
        return Crawling__MUIM_Setup(cl, obj, (APTR) msg);
    case MUIM_Show:
        return Crawling__MUIM_Show(cl, obj, (APTR) msg);
    case MUIM_Hide:
        return Crawling__MUIM_Hide(cl, obj, (APTR) msg);
    case MUIM_HandleEvent:
        return Crawling__MUIM_HandleEvent(cl, obj, (APTR) msg);
    default:
        return DoSuperMethodA(cl, obj, msg);
    }
}
BOOPSI_DISPATCHER_END

const struct __MUIBuiltinClass _MUI_Crawling_desc =
{
    MUIC_Crawling,
    MUIC_Virtgroup,
    sizeof(struct Crawling_DATA),
    (void *) Crawling_Dispatcher
};
#endif /* ZUNE_BUILTIN_CRAWLING */
