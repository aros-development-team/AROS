#ifndef _INLINE_COLORWHEEL_H
#define _INLINE_COLORWHEEL_H

#ifndef __INLINE_MACROS_H
#include <inline/macros.h>
#endif

#ifndef COLORWHEEL_BASE_NAME
#define COLORWHEEL_BASE_NAME ColorWheelBase
#endif

#define ConvertHSBToRGB(hsb, rgb) \
	LP2NR(0x1e, ConvertHSBToRGB, struct ColorWheelHSB *, hsb, a0, struct ColorWheelRGB *, rgb, a1, \
	, COLORWHEEL_BASE_NAME)

#define ConvertRGBToHSB(rgb, hsb) \
	LP2NR(0x24, ConvertRGBToHSB, struct ColorWheelRGB *, rgb, a0, struct ColorWheelHSB *, hsb, a1, \
	, COLORWHEEL_BASE_NAME)

#endif /* _INLINE_COLORWHEEL_H */
