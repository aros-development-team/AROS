#ifndef USB_HID_H
#define USB_HID_H

#include "usb.h"

#define INTERFACE_CLASS_HID 3
#define INTERFACE_SUB_CLASS_BOOT 1
/* only if INTERFACE_SUB_CLASS_BOOT! */
#define INTERFACE_PROTOCOL_KEYBOARD 1
#define INTERFACE_PROTOCOL_MOUSE 2

struct DescriptorDefinition {
	UBYTE bDescriptorType;
	UWORD wDescriptorLength; /* total size of report descriptor */
} __attribute__ ((packed));

struct DescriptorHID {
	struct DescriptorHeader header;
	UWORD bcdHID;
	UBYTE bCountryCode;      /* see below */
	UBYTE bNumDescriptors;   /* there is at least a Report descriptor */
	struct DescriptorDefinition ad[1]; /* definition of at least one descriptor */
} __attribute__ ((packed));

/* Country Codes */
#define CC_NOT_SUPPORTED      0
#define CC_ARABIC             1
#define CC_BELGIAN            2
#define CC_CANADIAN_BILINGUAL 3
#define CC_CANADIAN_FRENCH    4
#define CC_CZECH_REPUBLIC     5
#define CC_DANISH             6
#define CC_FINNISH            7
#define CC_FRENCH             8
#define CC_GERMAN             9
#define CC_GREEK             10
#define CC_HEBREW            11
#define CC_HUNGARY           12
#define CC_INTERNATIONAL     13 /* ISO */
#define CC_ITALIAN           14
#define CC_JAPAN             15 /* Katakana */
#define CC_KOREAN            16
#define CC_LATIN_AMERICAN    17
#define CC_NETHERLANDS       18
#define CC_DUTCH             18
#define CC_NORWEGIAN         19
#define CC_PERSIAN           20 /* Farsi */
#define CC_POLAND            21
#define CC_PORTUGUESE        22
#define CC_RUSSIA            23
#define CC_SLOVAKIA          24
#define CC_SPANISH           25
#define CC_SWEDISH           26
#define CC_SWISS_FRENCH      27
#define CC_SWISS_GERMAN      28
#define CC_SWITZERLAND       29
#define CC_TAIWAN            30
#define CC_TURKISH_Q         31
#define CC_UK                32
#define CC_US                33
#define CC_YUGOSLAVIA        34
#define CC_TURKISH_F         35

/* report descriptor item */
struct ShortItem {
	UBYTE bSize:2;
	UBYTE bType:2; /* see below */
	UBYTE bTag:4;
} __attribute__ ((packed)); /* 0,1,2, or 4 data bytes follow */

/* item types */
#define ITEM_TYPE_MAIN   (0)
#define ITEM_TYPE_GLOBAL (1)
#define ITEM_TYPE_LOCAL  (2)

struct LongItem {
	struct ShortItem si; /* size=2, type=3, tag=0xF */
	UBYTE bDataSize;
	UBYTE bLongItemTag;
} __attribute__ ((packed)); /* data bytes follow */

/* MAIN ITEMS */

#endif
