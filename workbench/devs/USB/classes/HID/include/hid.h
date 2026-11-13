#ifndef USB_HID_H
#define USB_HID_H

/*
    Copyright (C) 2006 by Michal Schulz
    $Id$

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU Library General Public License as 
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU Library General Public
    License along with this program; if not, write to the
    Free Software Foundation, Inc.,
    59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include <stdint.h>
#include <oop/oop.h>
#include <usb/usb.h>

#define CLID_Hidd_USBHID "Bus::USB::Hid"
#define IID_Hidd_USBHID  "IBus::USB::Hid"

#define HiddUSBHIDAttrBase __IHidd_USBHID

enum {
    moHidd_USBHID_GetReportDescriptor,
    moHidd_USBHID_GetHidDescriptor,
    moHidd_USBHID_SetIdle,
    moHidd_USBHID_SetProtocol,
    moHidd_USBHID_SetReport,
    moHidd_USBHID_ParseReport,

    NUM_HIDD_USBHID_METHODS
};

struct pHidd_USBHID_GetReportDescriptor {
    OOP_MethodID        mID;
    uint16_t            length;
    void                *buffer;
};

struct pHidd_USBHID_GetHidDescriptor {
    OOP_MethodID        mID;
};

struct pHidd_USBHID_SetIdle {
    OOP_MethodID        mID;
    uint8_t             duration;
    uint8_t             id;
};

struct pHidd_USBHID_ParseReport {
    OOP_MethodID        mID;
    uint8_t             id;
    void                *report;
    uint32_t            report_length;
};

struct pHidd_USBHID_SetProtocol {
    OOP_MethodID        mID;
    uint8_t             protocol;
};

struct pHidd_USBHID_SetReport {
    OOP_MethodID        mID;
    uint8_t             type;
    uint8_t             id;
    void                *report;
    uint16_t            length;
};

#define CLID_Hidd_USBMouse "Bus::USB::Hid::Mouse"
#define IID_Hidd_USBMouse  "IBus::USB::Hid::Mouse"

#define CLID_Hidd_USBKeyboard   "Bus::USB::Hid::Keyboard"
#define IID_Hidd_USBKeyboard    "IBus::USB::Hid::Keyboard"

#define UR_GET_HID_DESCRIPTOR   0x06
#define  UDESC_HID              0x21
#define  UDESC_REPORT           0x22
#define  UDESC_PHYSICAL         0x23
#define UR_SET_HID_DESCRIPTOR   0x07
#define UR_GET_REPORT           0x01
#define UR_SET_REPORT           0x09
#define UR_GET_IDLE             0x02
#define UR_SET_IDLE             0x0a
#define UR_GET_PROTOCOL         0x03
#define UR_SET_PROTOCOL         0x0b

typedef struct usb_hid_descriptor {
    uint8_t     bLength;
    uint8_t     bDescriptorType;
    uint16_t    bcdHID;
    uint8_t     bCountryCode;
    uint8_t     bNumDescriptors;
    struct {
        uint8_t     bDescriptorType;
        uint16_t    wDescriptorLength;
    } __attribute__((packed)) descrs[1];
} __attribute__((packed)) usb_hid_descriptor_t;
#define USB_HID_DESCRIPTOR_SIZE(n) (9+(n)*3)

/* Usage pages */
#define HUP_UNDEFINED           0x0000
#define HUP_GENERIC_DESKTOP     0x0001
#define HUP_SIMULATION          0x0002
#define HUP_VR_CONTROLS         0x0003
#define HUP_SPORTS_CONTROLS     0x0004
#define HUP_GAMING_CONTROLS     0x0005
#define HUP_KEYBOARD            0x0007
#define HUP_LEDS                0x0008
#define HUP_BUTTON              0x0009
#define HUP_ORDINALS            0x000a
#define HUP_TELEPHONY           0x000b
#define HUP_CONSUMER            0x000c
#define HUP_DIGITIZERS          0x000d
#define HUP_PHYSICAL_IFACE      0x000e
#define HUP_UNICODE             0x0010
#define HUP_ALPHANUM_DISPLAY    0x0014
#define HUP_MONITOR             0x0080
#define HUP_MONITOR_ENUM_VAL    0x0081
#define HUP_VESA_VC             0x0082
#define HUP_VESA_CMD            0x0083
#define HUP_POWER               0x0084
#define HUP_BATTERY_SYSTEM      0x0085
#define HUP_BARCODE_SCANNER     0x008b
#define HUP_SCALE               0x008c
#define HUP_CAMERA_CONTROL      0x0090
#define HUP_ARCADE              0x0091
#define HUP_MICROSOFT           0xff00

/* Usages, generic desktop */
#define HUG_POINTER             0x0001
#define HUG_MOUSE               0x0002
#define HUG_JOYSTICK            0x0004
#define HUG_GAME_PAD            0x0005
#define HUG_KEYBOARD            0x0006
#define HUG_KEYPAD              0x0007
#define HUG_X                   0x0030
#define HUG_Y                   0x0031
#define HUG_Z                   0x0032
#define HUG_RX                  0x0033
#define HUG_RY                  0x0034
#define HUG_RZ                  0x0035
#define HUG_SLIDER              0x0036
#define HUG_DIAL                0x0037
#define HUG_WHEEL               0x0038
#define HUG_HAT_SWITCH          0x0039
#define HUG_COUNTED_BUFFER      0x003a
#define HUG_BYTE_COUNT          0x003b
#define HUG_MOTION_WAKEUP       0x003c
#define HUG_VX                  0x0040
#define HUG_VY                  0x0041
#define HUG_VZ                  0x0042
#define HUG_VBRX                0x0043
#define HUG_VBRY                0x0044
#define HUG_VBRZ                0x0045
#define HUG_VNO                 0x0046
#define HUG_SYSTEM_CONTROL      0x0080
#define HUG_SYSTEM_POWER_DOWN   0x0081
#define HUG_SYSTEM_SLEEP        0x0082
#define HUG_SYSTEM_WAKEUP       0x0083
#define HUG_SYSTEM_CONTEXT_MENU 0x0084
#define HUG_SYSTEM_MAIN_MENU    0x0085
#define HUG_SYSTEM_APP_MENU     0x0086
#define HUG_SYSTEM_MENU_HELP    0x0087
#define HUG_SYSTEM_MENU_EXIT    0x0088
#define HUG_SYSTEM_MENU_SELECT  0x0089
#define HUG_SYSTEM_MENU_RIGHT   0x008a
#define HUG_SYSTEM_MENU_LEFT    0x008b
#define HUG_SYSTEM_MENU_UP      0x008c
#define HUG_SYSTEM_MENU_DOWN    0x008d

/* Usages Digitizers */
#define HUD_UNDEFINED           0x0000
#define HUD_TIP_PRESSURE        0x0030
#define HUD_BARREL_PRESSURE     0x0031
#define HUD_IN_RANGE            0x0032
#define HUD_TOUCH               0x0033
#define HUD_UNTOUCH             0x0034
#define HUD_TAP                 0x0035
#define HUD_QUALITY             0x0036
#define HUD_DATA_VALID          0x0037
#define HUD_TRANSDUCER_INDEX    0x0038
#define HUD_TABLET_FKEYS        0x0039
#define HUD_PROGRAM_CHANGE_KEYS 0x003a
#define HUD_BATTERY_STRENGTH    0x003b
#define HUD_INVERT              0x003c
#define HUD_X_TILT              0x003d
#define HUD_Y_TILT              0x003e
#define HUD_AZIMUTH             0x003f
#define HUD_ALTITUDE            0x0040
#define HUD_TWIST               0x0041
#define HUD_TIP_SWITCH          0x0042
#define HUD_SEC_TIP_SWITCH      0x0043
#define HUD_BARREL_SWITCH       0x0044
#define HUD_ERASER              0x0045
#define HUD_TABLET_PICK         0x0046

/* Usages LEDs */
#define HUD_LED_NUM_LOCK        0x0001
#define HUD_LED_CAPS_LOCK       0x0002
#define HUD_LED_SCROLL_LOCK     0x0003
#define HUD_LED_COMPOSE         0x0004
#define HUD_LED_KANA            0x0005

#define HID_USAGE2(p, u) (((p) << 16) | u)
#define HID_GET_USAGE(u) ((u) & 0xffff)
#define HID_GET_USAGE_PAGE(u) (((u) >> 16) & 0xffff)

#define UHID_INPUT_REPORT 0x01
#define UHID_OUTPUT_REPORT 0x02
#define UHID_FEATURE_REPORT 0x03

#define HCOLL_PHYSICAL          0
#define HCOLL_APPLICATION       1
#define HCOLL_LOGICAL           2

/* Bits in the input/output/feature items */
#define HIO_CONST       0x001
#define HIO_VARIABLE    0x002
#define HIO_RELATIVE    0x004
#define HIO_WRAP        0x008
#define HIO_NONLINEAR   0x010
#define HIO_NOPREF      0x020
#define HIO_NULLSTATE   0x040
#define HIO_VOLATILE    0x080
#define HIO_BUFBYTES    0x100

BOOL HIDD_USBHID_GetReportDescriptor(OOP_Object *obj, uint16_t length, void *buffer);
BOOL HIDD_USBHID_SetIdle(OOP_Object *obj, uint8_t duration, uint8_t id);
BOOL HIDD_USBHID_SetProtocol(OOP_Object *obj, uint8_t protocol);
BOOL HIDD_USBHID_SetReport(OOP_Object *obj, uint8_t type, uint8_t id, void *report, uint16_t length);
usb_hid_descriptor_t *HIDD_USBHID_GetHidDescriptor(OOP_Object *obj);
void HIDD_USBHID_ParseReport(OOP_Object *obj, uint8_t id, void *report, uint32_t report_length);

#endif /*USB_HID_H*/
