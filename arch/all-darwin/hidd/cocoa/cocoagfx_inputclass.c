/*
 * cocoagfx_inputclass.c - Mouse and Keyboard input for Cocoa HIDD
 *
 * Polls the lock-free ring buffer in HostInterface with atomic barriers
 * and translates macOS events to AmigaOS input.device events.
 */

#include <aros/debug.h>
#include <hidd/gfx.h>
#include <hidd/input.h>
#include <hidd/mouse.h>
#include <hidd/keyboard.h>
#include <oop/oop.h>
#include <proto/exec.h>
#include <proto/oop.h>
#include <proto/utility.h>
#include <devices/timer.h>

#include "cocoa_intern.h"
#include "hostinterface.h"

/* AttrBase for input - defined in startup.c */
extern OOP_AttrBase __abHidd_Input;
#define HiddInputAB __abHidd_Input

/*
 * macOS Virtual KeyCode -> Amiga RawKey Code Translation Table
 * (ISO/ANSI Apple Keyboard Layout mapping to Amiga RawKey values)
 */
static UBYTE mac_keycode_to_rawkey(int keycode)
{
    switch (keycode) {
    /* Letters */
    case 0x00: return 0x20; /* A */
    case 0x01: return 0x21; /* S */
    case 0x02: return 0x22; /* D */
    case 0x03: return 0x23; /* F */
    case 0x04: return 0x25; /* H */
    case 0x05: return 0x24; /* G */
    case 0x06: return 0x31; /* Z */
    case 0x07: return 0x32; /* X */
    case 0x08: return 0x33; /* C */
    case 0x09: return 0x34; /* V */
    case 0x0B: return 0x35; /* B */
    case 0x0C: return 0x10; /* Q */
    case 0x0D: return 0x11; /* W */
    case 0x0E: return 0x12; /* E */
    case 0x0F: return 0x13; /* R */
    case 0x10: return 0x15; /* Y */
    case 0x11: return 0x14; /* T */
    case 0x1F: return 0x18; /* O */
    case 0x20: return 0x16; /* U */
    case 0x22: return 0x17; /* I */
    case 0x23: return 0x19; /* P */
    case 0x25: return 0x28; /* L */
    case 0x26: return 0x26; /* J */
    case 0x28: return 0x27; /* K */
    case 0x2D: return 0x36; /* N */
    case 0x2E: return 0x37; /* M */

    /* Digits */
    case 0x12: return 0x01; /* 1 */
    case 0x13: return 0x02; /* 2 */
    case 0x14: return 0x03; /* 3 */
    case 0x15: return 0x04; /* 4 */
    case 0x17: return 0x05; /* 5 */
    case 0x16: return 0x06; /* 6 */
    case 0x1A: return 0x07; /* 7 */
    case 0x1C: return 0x08; /* 8 */
    case 0x19: return 0x09; /* 9 */
    case 0x1D: return 0x0A; /* 0 */

    /* Punctuation / Symbols */
    case 0x18: return 0x0C; /* = / + */
    case 0x1B: return 0x0B; /* - / _ */
    case 0x1E: return 0x1B; /* ] / } */
    case 0x21: return 0x1A; /* [ / { */
    case 0x27: return 0x2B; /* ' / " */
    case 0x29: return 0x2A; /* ; / : */
    case 0x2A: return 0x0D; /* \ / | */
    case 0x2B: return 0x38; /* , / < */
    case 0x2C: return 0x3A; /* / / ? */
    case 0x2F: return 0x39; /* . / > */
    case 0x32: return 0x00; /* ` / ~ */

    /* Control / Modifiers */
    case 0x24: return 0x44; /* Return */
    case 0x30: return 0x42; /* Tab */
    case 0x31: return 0x40; /* Space */
    case 0x33: return 0x41; /* Backspace / Delete */
    case 0x35: return 0x45; /* Escape */
    case 0x37: return 0x66; /* Left Command -> Left Amiga */
    case 0x36: return 0x67; /* Right Command -> Right Amiga */
    case 0x38: return 0x60; /* Left Shift */
    case 0x3C: return 0x61; /* Right Shift */
    case 0x39: return 0x62; /* Caps Lock */
    case 0x3A: return 0x64; /* Left Option -> Left Alt */
    case 0x3D: return 0x65; /* Right Option -> Right Alt */
    case 0x3B: return 0x63; /* Left Control */
    case 0x3E: return 0x63; /* Right Control */

    /* Arrow / Cursor Keys */
    case 0x7B: return 0x4F; /* Left */
    case 0x7C: return 0x4E; /* Right */
    case 0x7D: return 0x4D; /* Down */
    case 0x7E: return 0x4C; /* Up */

    /* Function Keys */
    case 0x7A: return 0x50; /* F1 */
    case 0x78: return 0x51; /* F2 */
    case 0x63: return 0x52; /* F3 */
    case 0x76: return 0x53; /* F4 */
    case 0x60: return 0x54; /* F5 */
    case 0x61: return 0x55; /* F6 */
    case 0x62: return 0x56; /* F7 */
    case 0x64: return 0x57; /* F8 */
    case 0x65: return 0x58; /* F9 */
    case 0x6D: return 0x59; /* F10 */

    default:
        return 0xFF; /* Unknown / unmapped */
    }
}

/* ======== Mouse class ======== */

struct CocoaMouseData {
    void (*callback)(void *data, struct pHidd_Mouse_Event *ev);
    void *callbackdata;
};

OOP_Object *CocoaMouse__Root__New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    if (o) {
        struct CocoaMouseData *data = OOP_INST_DATA(cl, o);
        data->callback = (void *)GetTagData(aHidd_Input_IrqHandler, 0, msg->attrList);
        data->callbackdata = (void *)GetTagData(aHidd_Input_IrqHandlerData, 0, msg->attrList);
    }
    return o;
}

static struct OOP_MethodDescr CocoaMouse_Root_descr[] = {
    { (OOP_MethodFunc)CocoaMouse__Root__New, moRoot_New },
    { NULL, 0 }
};

struct OOP_InterfaceDescr CocoaMouse_ifdescr[] = {
    { CocoaMouse_Root_descr, IID_Root, 1 },
    { NULL, NULL }
};

/* ======== Keyboard class ======== */

struct CocoaKbdData {
    void (*callback)(void *data, UWORD keycode);
    void *callbackdata;
};

OOP_Object *CocoaKbd__Root__New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    if (o) {
        struct CocoaKbdData *data = OOP_INST_DATA(cl, o);
        data->callback = (void *)GetTagData(aHidd_Input_IrqHandler, 0, msg->attrList);
        data->callbackdata = (void *)GetTagData(aHidd_Input_IrqHandlerData, 0, msg->attrList);
    }
    return o;
}

static struct OOP_MethodDescr CocoaKbd_Root_descr[] = {
    { (OOP_MethodFunc)CocoaKbd__Root__New, moRoot_New },
    { NULL, 0 }
};

struct OOP_InterfaceDescr CocoaKbd_ifdescr[] = {
    { CocoaKbd_Root_descr, IID_Root, 1 },
    { NULL, NULL }
};

/* ======== Event polling with memory barriers ======== */

static OOP_Object *g_mouseobj = NULL;
static OOP_Object *g_kbdobj = NULL;

void cocoa_input_poll(struct HostInterface *hif)
{
    if (!hif) return;

    int rd = __atomic_load_n(&hif->cocoa_event_read, __ATOMIC_RELAXED);
    int wr = __atomic_load_n(&hif->cocoa_event_write, __ATOMIC_ACQUIRE);

    while (rd != wr) {
        typeof(hif->cocoa_events[0]) *ev = &hif->cocoa_events[rd];

        if (g_mouseobj && (ev->type == COCOA_EVENT_MOUSE_MOVE ||
                           ev->type == COCOA_EVENT_MOUSE_PRESS ||
                           ev->type == COCOA_EVENT_MOUSE_RELEASE)) {
            struct CocoaMouseData *md = OOP_INST_DATA(OOP_OCLASS(g_mouseobj), g_mouseobj);
            if (md->callback) {
                struct pHidd_Mouse_Event me;
                me.x = ev->x;
                me.y = ev->y;
                if (ev->type == COCOA_EVENT_MOUSE_MOVE) {
                    me.type = vHidd_Mouse_Motion;
                    me.button = vHidd_Mouse_NoButton;
                } else {
                    me.type = (ev->type == COCOA_EVENT_MOUSE_PRESS) ?
                              vHidd_Mouse_Press : vHidd_Mouse_Release;
                    me.button = (ev->button == 1) ? vHidd_Mouse_Button1 :
                                (ev->button == 2) ? vHidd_Mouse_Button2 :
                                vHidd_Mouse_Button3;
                }
                md->callback(md->callbackdata, &me);
            }
        }

        if (g_kbdobj && (ev->type == COCOA_EVENT_KEY_PRESS ||
                         ev->type == COCOA_EVENT_KEY_RELEASE)) {
            struct CocoaKbdData *kd = OOP_INST_DATA(OOP_OCLASS(g_kbdobj), g_kbdobj);
            if (kd->callback) {
                UBYTE rawkey = mac_keycode_to_rawkey(ev->keycode);
                if (rawkey != 0xFF) {
                    UWORD code = rawkey;
                    if (ev->type == COCOA_EVENT_KEY_RELEASE)
                        code |= 0x80; /* IECODE_UP_PREFIX */
                    kd->callback(kd->callbackdata, code);
                }
            }
        }

        rd = (rd + 1) % COCOA_EVENT_RING_SIZE;
        __atomic_store_n(&hif->cocoa_event_read, rd, __ATOMIC_RELEASE);
        wr = __atomic_load_n(&hif->cocoa_event_write, __ATOMIC_ACQUIRE);
    }
}

void cocoa_input_set_objects(OOP_Object *mouse, OOP_Object *kbd)
{
    g_mouseobj = mouse;
    g_kbdobj = kbd;
}
