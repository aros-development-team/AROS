#ifndef _FT_AROS_GLYPH_H
#define _FT_AROS_GLYPH_H

#include "ftglyphengine.h"

void set_transform(FT_GlyphEngine *);
int char_to_glyph(FT_GlyphEngine *, int);
int UnicodeToGlyphIndex(FT_GlyphEngine *);
int SetInstance(FT_GlyphEngine *);
void RenderGlyph(FT_GlyphEngine *, int);
void switch_family(FT_GlyphEngine *);

#endif /*_FT_AROS_GLYPH_H*/
