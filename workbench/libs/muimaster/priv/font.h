#ifndef __ZUNE_FONT_H__
#define __ZUNE_FONT_H__

#ifdef _AROS
#include <graphics/text.h>
typedef struct TextFont GdkFont;
extern const struct TextAttr defaultFont;
#endif

GdkFont *zune_font_get (LONG preset);
void zune_font_replace (GdkFont **font, STRPTR fontname);

#endif
