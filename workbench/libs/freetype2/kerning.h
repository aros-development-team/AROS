#ifndef _FT_AROS_KERNING_H
#define _FT_AROS_KERNING_H

#include "ftglyphengine.h"

#include <exec/lists.h>

void FreeWidthList(FT_GlyphEngine *, struct MinList *);
struct MinList *GetWidthList(FT_GlyphEngine *);
int get_kerning_dir(FT_GlyphEngine *);

#endif /*_FT_AROS_KERNING_H*/
