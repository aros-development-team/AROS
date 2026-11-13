/*
 * otag.h - .otag files interface
 * Copyright © 1995-96 Michael Letowski
 */

#ifndef OTAG_H
#define OTAG_H

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif

#ifndef DISKFONT_DISKFONT_H
#include <diskfont/diskfont.h>
#endif

#ifndef CLASSBASE_H
#include "classbase.h"
#endif

/*
 * Public macros
 */
/* Get pointer to TFontContents array */
#define TFontContents(fch)	((struct TFontContents *)\
    ((IPTR)(fch) + sizeof(struct FontContentsHeader)))

/*
 * Public functions
 */
struct FontContentsHeader *NewFC(BPTR lock, STRPTR name);
VOID DisposeFC(struct FontContentsHeader *fch);

#endif /* OTAG_H */
