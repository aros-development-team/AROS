/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    HID-over-GATT (HOGP) device-class emulation for the virtual controller:
    the report maps and the simulated input reports for the LE mouse and the
    LE keyboard. Selected per device by GAP appearance (keyboard 0x03c1, mouse
    0x03c2); anything else advertising the HID service (0x1812) is treated as a
    mouse.
*/

#include <proto/exec.h>
#include <string.h>

#include "vbthci_intern.h"
#include "vbthci_devclass.h"

/* HID report descriptor of a boot mouse (buttons byte + X + Y). */
static const UBYTE mouse_reportdesc[] = {
    0x05, 0x01, 0x09, 0x02, 0xa1, 0x01, 0x09, 0x01,
    0xa1, 0x00, 0x05, 0x09, 0x19, 0x01, 0x29, 0x03,
    0x15, 0x00, 0x25, 0x01, 0x95, 0x03, 0x75, 0x01,
    0x81, 0x02, 0x95, 0x01, 0x75, 0x05, 0x81, 0x03,
    0x05, 0x01, 0x09, 0x30, 0x09, 0x31, 0x15, 0x81,
    0x25, 0x7f, 0x75, 0x08, 0x95, 0x02, 0x81, 0x06,
    0xc0, 0xc0
};

/* HID report descriptor of a boot keyboard: 1 modifier byte, 1 reserved byte,
   then 6 key codes (8-byte input report). */
static const UBYTE kbd_reportdesc[] = {
    0x05, 0x01, 0x09, 0x06, 0xa1, 0x01,             /* Desktop, Keyboard, Collection(App) */
    0x05, 0x07, 0x19, 0xe0, 0x29, 0xe7,             /* Keycodes, Usage Min E0 .. Max E7 (modifiers) */
    0x15, 0x00, 0x25, 0x01, 0x75, 0x01, 0x95, 0x08, /* logical 0..1, size 1, count 8 */
    0x81, 0x02,                                     /* Input(Data,Var,Abs) - modifier byte */
    0x95, 0x01, 0x75, 0x08, 0x81, 0x01,             /* count 1, size 8, Input(Const) - reserved */
    0x95, 0x06, 0x75, 0x08, 0x15, 0x00, 0x25, 0x65, /* count 6, size 8, logical 0..101 */
    0x05, 0x07, 0x19, 0x00, 0x29, 0x65,             /* Keycodes, Usage Min 0 .. Max 101 */
    0x81, 0x00,                                     /* Input(Data,Array) - 6 key codes */
    0xc0
};

#define VBT_APPEARANCE_KEYBOARD 0x03c1
#define VBT_APPEARANCE_MOUSE    0x03c2
#define VBT_UUID_HID_LE         0x1812   /* HID over GATT (HOGP) */
#define VBT_UUID_HID_CLASSIC    0x1124   /* HID over L2CAP (HIDP) */

/* Is this device a keyboard? LE devices are identified by GAP appearance,
   classic devices by the Class-of-Device peripheral minor (keyboard). */
static BOOL vbtp_HidIsKeyboard(const struct VBTFakeDevice *fd)
{
    if(!fd) {
        return FALSE;
    }
    if(fd->fd_IsLE) {
        return (fd->fd_Appearance == VBT_APPEARANCE_KEYBOARD) ? TRUE : FALSE;
    }
    /* classic: CoD peripheral minor bits 6..7 == 1 -> keyboard */
    return ((((fd->fd_CoD >> 2) & 0x3f) >> 4) == 1) ? TRUE : FALSE;
}

/* /// "vbtp_IsHid()" */
BOOL vbtp_IsHid(const struct VBTFakeDevice *fd)
{
    return (fd && ((fd->fd_ServiceUUID == VBT_UUID_HID_LE) ||
                   (fd->fd_ServiceUUID == VBT_UUID_HID_CLASSIC))) ? TRUE : FALSE;
}
/* \\\ */

/* /// "vbtp_HidReportDesc()" */
const UBYTE *vbtp_HidReportDesc(const struct VBTFakeDevice *fd, ULONG *len)
{
    if(vbtp_HidIsKeyboard(fd)) {
        *len = sizeof(kbd_reportdesc);
        return kbd_reportdesc;
    }
    *len = sizeof(mouse_reportdesc);
    return mouse_reportdesc;
}
/* \\\ */

/* /// "vbtp_HidInputReport()" */
ULONG vbtp_HidInputReport(const struct VBTFakeDevice *fd, ULONG step, UBYTE *buf)
{
    if(vbtp_HidIsKeyboard(fd)) {
        /* keyboard: type the letters a, b, c, ... one at a time, alternating a
           key-press report with a key-release report so input.device sees a
           clean make/break for every character. HID keycode 0x04 == 'a'. */
        memset(buf, 0, 8);
        if(!(step & 1)) {
            buf[2] = (UBYTE)(0x04 + ((step >> 1) % 26));
        }
        return 8;
    } else {
        /* mouse: wiggle in a small square. Report is buttons, dX, dY. */
        static const BYTE dx[4] = { 4, 0, -4, 0 };
        static const BYTE dy[4] = { 0, 4, 0, -4 };
        buf[0] = 0;
        buf[1] = (UBYTE) dx[step & 3];
        buf[2] = (UBYTE) dy[step & 3];
        return 3;
    }
}
/* \\\ */
