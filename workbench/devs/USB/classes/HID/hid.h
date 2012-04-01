#ifndef HID_H_
#define HID_H_

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

#include <exec/libraries.h>
#include <exec/execbase.h>
#include <exec/nodes.h>
#include <exec/lists.h>
#include <exec/semaphores.h>
#include <exec/ports.h>
#include <exec/interrupts.h>
#include <dos/bptr.h>
#include <dos/dosextens.h>

#include <aros/libcall.h>
#include <aros/asmcall.h>

#include <oop/oop.h>

#include <aros/arossupportbase.h>
#include <exec/execbase.h>

#include LC_LIBDEFS_FILE

#undef HiddPCIDeviceAttrBase
#undef HiddUSBDeviceAttrBase
#undef HiddUSBHubAttrBase
#undef HiddUSBDrvAttrBase
#undef HiddUSBAttrBase
#undef HiddUSBHIDAttrBase
#undef HiddAttrBase

#define HiddPCIDeviceAttrBase (SD(cl)->HiddPCIDeviceAB)
#define HiddUSBDeviceAttrBase (SD(cl)->HiddUSBDeviceAB)
#define HiddUSBHubAttrBase (SD(cl)->HiddUSBHubAB)
#define HiddUSBDrvAttrBase (SD(cl)->HiddUSBDrvAB)
#define HiddAttrBase (SD(cl)->HiddAB)
#define HiddUSBAttrBase (SD(cl)->HiddUSBAB)
#define HiddUSBHIDAttrBase (SD(cl)->HiddUSBHIDAB)

struct hid_location {
    uint32_t size;
    uint32_t count;
    uint32_t pos;
};

struct hid_range {
    int32_t minimum;
    int32_t maximum;
};

struct hid_staticdata
{
    struct SignalSemaphore  Lock;
    void                    *MemPool;
    OOP_Class               *hidClass;
    OOP_Class               *mouseClass;
    OOP_Class               *kbdClass;

    OOP_AttrBase            HiddPCIDeviceAB;
    OOP_AttrBase            HiddUSBDeviceAB;
    OOP_AttrBase            HiddUSBHubAB;
    OOP_AttrBase            HiddUSBDrvAB;
    OOP_AttrBase            HiddUSBAB;
    OOP_AttrBase            HiddUSBHIDAB;
    OOP_AttrBase            HiddAB;
};

struct hidbase
{
    struct Library          LibNode;
    struct hid_staticdata   sd;
};

#define MAX_BTN 16

typedef struct HidData {
    struct hid_staticdata       *sd;
    OOP_Object                  *o;

    usb_config_descriptor_t     *cdesc;
    usb_device_descriptor_t     ddesc;
    usb_hid_descriptor_t        *hd;
    char                        *report;
    uint16_t                    reportLength;
    uint16_t                    nreport;

    struct Interrupt            interrupt;
    APTR                        intr_pipe;
    char                        *buffer;
    uint16_t                    buflen;
} HidData;

typedef struct {
    int32_t     dx, dy, dz;
    uint16_t    btn;
} report_t;

typedef struct MouseData {
    struct hid_staticdata       *sd;
    OOP_Object                  *o;

    usb_hid_descriptor_t        *hd;
    char                        *report;
    uint16_t                    reportLength;
    uint16_t                    nreport;
    int                         mouse_report;

    struct Task                 *mouse_task;

    uint8_t                     rel_x, rel_y, rel_z;
    uint8_t                     buttonstate;

    int32_t                     last_x, last_y, max_x, max_y;
    uint8_t                     tablet;

    struct hid_range            range_x;
    struct hid_range            range_y;
    struct hid_location         loc_x;
    struct hid_location         loc_y;
    struct hid_location         loc_wheel;
    struct hid_location         loc_btn[MAX_BTN];
    uint8_t                     loc_btncnt;

#define RING_SIZE       8
    report_t report_ring[RING_SIZE];
    uint8_t head,tail;
} MouseData;

struct key_mod {
    struct hid_location loc;
    uint8_t             key;
};

typedef struct KbdData {
    struct hid_staticdata       *sd;
    OOP_Object                  *o;

    usb_hid_descriptor_t        *hd;
    char                        *report;
    uint16_t                    reportLength;

    struct Task                 *kbd_task;

    uint16_t                    prev_key;
    uint16_t                    prev_prev_key;
    uint16_t                    prev_qual;
    uint16_t                    prev_prev_qual;

    uint8_t                     *prev_code;
    uint8_t                     *code;

    struct hid_location         loc_scrollock;
    struct hid_location         loc_numlock;
    struct hid_location         loc_capslock;

    struct key_mod              loc_mod[8];
    struct hid_location         loc_keycode;
    uint8_t                     loc_modcnt;
    uint8_t                     loc_keycnt;
#define LED_CAPSLOCK    1
#define LED_NUMLOCK     2
#define LED_SCROLLOCK   4
    uint8_t                     leds;
} KbdData;

enum hid_kind {
    hid_input,
    hid_output,
    hid_feature,
    hid_collection,
    hid_endcollection,
    hid_none
};

struct hid_item {
    /* Global */
    int32_t _usage_page;
    int32_t logical_minimum;
    int32_t logical_maximum;
    int32_t physical_minimum;
    int32_t physical_maximum;
    int32_t unit_exponent;
    int32_t unit;
    int32_t report_ID;
    /* Local */
    int32_t usage;
    int32_t usage_minimum;
    int32_t usage_maximum;
    int32_t designator_index;
    int32_t designator_minimum;
    int32_t designator_maximum;
    int32_t string_index;
    int32_t string_minimum;
    int32_t string_maximum;
    int32_t set_delimiter;
    /* Misc */
    int32_t collection;
    int collevel;
    enum hid_kind kind;
    uint32_t flags;
    /* Location */
    struct hid_location loc;
    /* */
    struct hid_item *next;
};

struct hid_data *hid_start_parse(void *d, int len, enum hid_kind kind);
void hid_end_parse(struct hid_data *s);
int hid_maxrepid(void *s, int len);
int hid_get_item(struct hid_data *s, struct hid_item *h);
int hid_report_size(void *buf, int len, enum hid_kind k, uint8_t id);
int hid_locate(void *desc, int size, uint32_t usage, uint8_t id,
               enum hid_kind kind, struct hid_location *loc, uint32_t *flags, struct hid_range *range);
uint32_t hid_get_data(unsigned char *buf, struct hid_location *loc);
int hid_is_collection(void *desc, int size, uint8_t id, uint32_t usage);


#define METHOD(base, id, name) \
    base ## __ ## id ## __ ## name (OOP_Class *cl, OOP_Object *o, struct p ## id ## _ ## name *msg)

#define BASE(lib)((struct hidbase*)(lib))
#define SD(cl) (&BASE(cl->UserData)->sd)

#endif /*HID_H_*/
