/* 
    Copyright © 1999, David Le Corfec.
    Copyright © 2002, The AROS Development Team.
    All rights reserved.

    $Id$
*/

#ifndef __ZUNE_FONT_H__
#define __ZUNE_FONT_H__

#ifdef __AROS__
#include <graphics/text.h>
typedef struct TextFont GdkFont;
extern const struct TextAttr defaultFont;
#endif

GdkFont *zune_font_get (LONG preset);
void zune_font_replace (GdkFont **font, STRPTR fontname);

#endif
