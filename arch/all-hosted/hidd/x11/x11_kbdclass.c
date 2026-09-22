/*
    Copyright (C) 1995-2025, The AROS Development Team. All rights reserved.

    Desc: X11 hidd handling keypresses.
*/

#define DEBUG 0
#include "x11_debug.h"

#define __OOP_NOATTRBASES__

#include <proto/utility.h>
#include <string.h>
#include <signal.h>
#include <devices/inputevent.h>
#include <hidd/keyboard.h>

#include "x11_keytable.h"

#include LC_LIBDEFS_FILE
#include "x11_hostlib.h"

/****************************************************************************************/

WORD xkey2hidd (XKeyEvent *xk, struct x11_staticdata *xsd);

static VOID x11kbd_cancel(struct x11kbd_data *data);

static OOP_AttrBase HiddInputAB;

static struct OOP_ABDescr attrbases[] =
{
    { IID_Hidd_Input  , &HiddInputAB    },
    { NULL          , NULL          }
};

/****************************************************************************************/
/* Include the required x11 keytable translations                                        */
/****************************************************************************************/

/* include the "generic" keyboard translation .. */
#include "x11_keytable_default.c"

/****************************************************************************************/

/* include the default keyboard translation .. */
#ifdef KEYTABLE_NAME
#  undef KEYTABLE_NAME
#endif
#define KEYTABLE_NAME builtin_keytable

#ifndef KEYTABLE_DEFAULT
#  define KEYTABLE_DEFAULT "x11_keytable-en_gb.c"
#endif
#include KEYTABLE_DEFAULT

/****************************************************************************************/
/* Kbd Hidd methods ...                                                                 */
/****************************************************************************************/

OOP_Object * X11Kbd__Root__New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    BOOL                has_kbd_hidd = FALSE;
    struct TagItem      *tag, *tstate;
    InputIrqCallBack_t  callback = NULL;

    D(bug("[X11:Kbd] %s()\n", __func__));

    ObtainSemaphoreShared( &XSD(cl)->sema);
 
    if (XSD(cl)->kbdhidd)
        has_kbd_hidd = TRUE;

    ReleaseSemaphore( &XSD(cl)->sema);
 
    if (has_kbd_hidd) { /* Cannot open twice */
        D(bug("[X11:Kbd] %s: Attempt to create second instance\n", __func__));
        return NULL; /* Should have some error code here */
    }

    tstate = msg->attrList;
    D(bug("[X11:Kbd] %s: tstate: %p, tag=%x\n", __func__, tstate, tstate->ti_Tag));
    
    while ((tag = NextTagItem(&tstate))) {
        ULONG idx;

        if (IS_HIDDINPUT_ATTR(tag->ti_Tag, idx)) {
            D(bug("[X11:Kbd] %s: Kbd hidd tag\n", __func__));
            switch (idx) {
                case aoHidd_Input_IrqHandler:
                    callback = (APTR)tag->ti_Data;
                    D(bug("[X11:Kbd] %s:   Got callback @ 0x%p\n", __func__, (APTR)tag->ti_Data));
                    break;
            }
        } else {
            D(bug("[X11:Kbd] %s: Got tag %d, data %x\n", __func__, tag->ti_Tag, tag->ti_Data));
        }
    } /* while (tags to process) */
    
    if (NULL == callback) {
        D(bug("[X11:Kbd] %s: returning %d\n", __func__, 0));

        return NULL; /* Should have some error code here */
    }

    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    if (o) {
        struct x11kbd_data *data = OOP_INST_DATA(cl, o);

        data->kbd_callback = callback;
        OOP_GetAttr(o, aHidd_Input_IrqHandlerData, (IPTR *)&data->callbackdata);
        D(bug("[X11:Kbd] %s: callback data = %p\n", __func__, (APTR)data->callbackdata));
        memset(&data->keys, 0, sizeof(data->keys));
        memset(data->f12, 0, sizeof(data->f12));
        data->f12_count = 0;
        data->active = FALSE;
        data->await_keymap = FALSE;

        ObtainSemaphore( &XSD(cl)->sema);
        XSD(cl)->kbdhidd = o;
        ReleaseSemaphore( &XSD(cl)->sema);
    }

    D(bug("[X11:Kbd] %s: returning 0x%p\n", __func__, o));

    return o;
}

/****************************************************************************************/

VOID X11Kbd__Root__Dispose(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    D(bug("[X11:Kbd] %s()\n", __func__));

    ObtainSemaphore( &XSD(cl)->sema);
    x11kbd_cancel(OOP_INST_DATA(cl, o));
    XSD(cl)->kbdhidd = NULL;
    ReleaseSemaphore( &XSD(cl)->sema);
    OOP_DoSuperMethod(cl, o, msg);
}

/****************************************************************************************/

/* HandleEvent has a single writer (the X11 task) under the shared semaphore;
 * disposal and table replacement take it exclusively.
 * These helpers run under xsd->sema, including disposal. Only this backend's
 * contributions are released; the shared keyboard device is not reset.
 */
static VOID x11kbd_emit(struct x11kbd_data *data, int code)
{
    if (code >= 0)
    {
        struct pHidd_Kbd_Event event;
        event.flags = 0;
        event.code = code;
        data->kbd_callback(data->callbackdata, &event);
    }
}

static VOID x11kbd_release(struct x11kbd_data *data, unsigned int key)
{
    if (data->f12[key])
    {
        data->f12[key] = FALSE;
        --data->f12_count;
    }
    x11kbd_emit(data, x11_key_release(&data->keys, key));
}

static VOID x11kbd_cancel(struct x11kbd_data *data)
{
    unsigned int key;
    for (key = 0; key < 256; ++key)
        x11kbd_release(data, key);
    data->active = FALSE;
    data->await_keymap = FALSE;
}

VOID X11Kbd__Hidd_Kbd_X11__HandleEvent(OOP_Class *cl, OOP_Object *o, struct pHidd_Kbd_X11_HandleEvent *msg)
{
    struct x11kbd_data *data = OOP_INST_DATA(cl, o);
    XEvent *event = msg->event;
    XKeyEvent *xk = &event->xkey;
    unsigned int key;
    KeySym sym;

    if (event->type == FocusOut)
    {
        x11kbd_cancel(data);
        return;
    }
    if (event->type == FocusIn)
    {
        data->active = TRUE;
        data->await_keymap = TRUE;
        return;
    }
    if (event->type == KeymapNotify)
    {
        if (!data->active || !data->await_keymap)
            return;
        data->await_keymap = FALSE;
        /* KeymapStateMask supplies the server's ordered focus-entry snapshot.
         * Do not query current state later: it may include subsequently typed keys.
         */
        for (key = 0; key < 256; ++key)
        {
            if (!(event->xkeymap.key_vector[key / 8] & (1U << (key % 8))))
                x11kbd_release(data, key);
            else if (!data->keys.key[key])
            {
                XKeyEvent lookup = {0};
                int raw;
                lookup.type = KeyPress;
                lookup.display = XSD(cl)->display;
                lookup.keycode = key;
                raw = xkey2hidd(&lookup, XSD(cl));
                /* Restore momentary modifiers only. Caps Lock is guest-owned;
                 * other held keys must be released before they can act again.
                 */
                if (raw < 0x60 || raw > 0x67 || raw == 0x62)
                    raw = -1;
                x11kbd_emit(data, x11_key_press(&data->keys, key, raw));
            }
        }
        return;
    }
    if (!data->active || (event->type != KeyPress && event->type != KeyRelease)
        || xk->keycode >= 256)
        return;
    key = xk->keycode;
    if (event->type == KeyRelease)
    {
        x11kbd_release(data, key);
        return;
    }
    if (data->keys.key[key])
        return;

    LOCK_X11
    sym = XCALL(XLookupKeysym, xk, 0);
    UNLOCK_X11
    /* Latch the host shortcut role too, even for an unmapped guest key. */
    if (sym == XK_F12)
    {
        data->f12[key] = TRUE;
        ++data->f12_count;
    }
    else if (data->f12_count && (sym == XK_Q || sym == XK_q))
    {
        LOCK_X11
        CCALL(raise, SIGINT);
        UNLOCK_X11
    }
    x11kbd_emit(data, x11_key_press(&data->keys, key, xkey2hidd(xk, XSD(cl))));
}

/****************************************************************************************/

#undef XSD
#define XSD(cl) xsd

/****************************************************************************************/

WORD lookup_keytable(KeySym *ks, const struct _keytable *keytable)
{
    short t;
    WORD  result = -1;
    
    for (t = 0; keytable[t].hiddcode != -1; t++) {
        if (*ks == keytable[t].keysym) {
            D(bug("[X11:Kbd] %s: found in key table\n", __func__));
            result = keytable[t].hiddcode;
            break;
        }
    }
    
    return result;
}

/****************************************************************************************/

WORD xkey2hidd (XKeyEvent *xk, struct x11_staticdata *xsd)
{
    XKeyEvent lookup = *xk;
    char    buffer[10];
    KeySym  ks;
    D(int     count;)
    LONG    result;

    D(bug("[X11:Kbd] %s()\n", __func__));
    D(bug("[X11:Kbd] %s: xsd @ 0x%p\n", __func__, xsd));

    if ((xsd->xtd) && (xsd->xtd->havetable)) {
        D(bug("[X11:Kbd] %s: using loaded X key table\n", __func__));

        result = -1;

        if (xk->keycode < 256) {
            result = xsd->xtd->keycode2rawkey[xk->keycode];
            if (result == 255) result = -1;
        }
        
        return result;
    }
    
    LOCK_X11
    lookup.state = 0;
    D(count =) XCALL(XLookupString, &lookup, buffer, 10, &ks, NULL);
    UNLOCK_X11

    D(bug("[X11:Kbd] %s: Code %d (0x%x). Event was decoded into %d chars: %d (0x%x)\n", __func__,xk->keycode, xk->keycode, count,ks,ks));

    result = lookup_keytable(&ks, keytable);
    if (result == -1) result = lookup_keytable(&ks, builtin_keytable);

    D(bug("[X11:Kbd] %s: returning %d\n", __func__, result));

    return result;
    
} /* XKeyToAmigaCode */

/****************************************************************************************/

AROS_LH1(void , x11kdb_LoadkeyTable,
         AROS_LHA(APTR, table, A0),
         struct x11clbase *, X11Base, 5, X11Cl)
{
    AROS_LIBFUNC_INIT

    D(bug("[X11:Kbd] %s(0x%p)\n", __func__, table));

    if (X11Base->xsd.xtd) {
        D(bug("[X11:Kbd] %s: Copying Table Data\n", __func__));
        ObtainSemaphore(&X11Base->xsd.sema);
        CopyMem(table, X11Base->xsd.xtd->keycode2rawkey, 256);
        X11Base->xsd.xtd->havetable = TRUE;
        ReleaseSemaphore(&X11Base->xsd.sema);
    }
    AROS_LIBFUNC_EXIT
}


/****************************************************************************************/

#undef XSD
#define XSD(cl) (&LIBBASE->xsd)

/****************************************************************************************/

static int kbd_init(LIBBASETYPEPTR LIBBASE)
{

    return OOP_ObtainAttrBases(attrbases);
}

/****************************************************************************************/

static int kbd_expunge(LIBBASETYPEPTR LIBBASE)
{
    OOP_ReleaseAttrBases(attrbases);
    return TRUE;
}

/****************************************************************************************/

ADD2INITLIB(kbd_init, 0);
ADD2EXPUNGELIB(kbd_expunge, 0);

/****************************************************************************************/
