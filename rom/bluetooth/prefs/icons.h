#ifndef ICONS_H
#define ICONS_H
#include <exec/types.h>
#define ICON_WIDTH 16
#define ICON_HEIGHT 16
#define ICON_DEPTH 4
#define ICON_NCOLORS 16
#define ICON_TRANSPARENT 0
#define ICON_GENERAL 0
#define ICON_HARDWARE 1
#define ICON_DEVICES 2
#define ICON_CLASSES 3
#define ICON_OPTIONS 4
#define ICON_RADIO 5
#define ICON_DEVICE 6
#define ICON_LED_GREEN 7
#define ICON_LED_GRAY 8
#define ICON_LED_RED 9
#define ICON_LED_ORANGE 10
#define ICON_DEV_COMPUTER 11
#define ICON_DEV_PHONE 12
#define ICON_DEV_KEYBOARD 13
#define ICON_DEV_MOUSE 14
#define ICON_DEV_HEADSET 15
#define ICON_DEV_GENERIC 16
#define ICON_COUNT 17
extern const ULONG icon_colors[48];
extern const UBYTE * const icon_bodies[ICON_COUNT];
#endif
