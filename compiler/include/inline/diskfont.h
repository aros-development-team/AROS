#ifndef _INLINE_DISKFONT_H
#define _INLINE_DISKFONT_H

#ifndef __INLINE_MACROS_H
#include <inline/macros.h>
#endif

#ifndef DISKFONT_BASE_NAME
#define DISKFONT_BASE_NAME DiskfontBase
#endif

#define AvailFonts(buffer, bufBytes, flags) \
	LP3(0x24, LONG, AvailFonts, STRPTR, buffer, a0, long, bufBytes, d0, long, flags, d1, \
	, DISKFONT_BASE_NAME)

#define DisposeFontContents(fontContentsHeader) \
	LP1NR(0x30, DisposeFontContents, struct FontContentsHeader *, fontContentsHeader, a1, \
	, DISKFONT_BASE_NAME)

#define NewFontContents(fontsLock, fontName) \
	LP2(0x2a, struct FontContentsHeader *, NewFontContents, BPTR, fontsLock, a0, STRPTR, fontName, a1, \
	, DISKFONT_BASE_NAME)

#define NewScaledDiskFont(sourceFont, destTextAttr) \
	LP2(0x36, struct DiskFont *, NewScaledDiskFont, struct TextFont *, sourceFont, a0, struct TextAttr *, destTextAttr, a1, \
	, DISKFONT_BASE_NAME)

#define OpenDiskFont(textAttr) \
	LP1(0x1e, struct TextFont *, OpenDiskFont, struct TextAttr *, textAttr, a0, \
	, DISKFONT_BASE_NAME)

#endif /* _INLINE_DISKFONT_H */
