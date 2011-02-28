/* 
    Copyright © 2003-2011, The AROS Development Team.
    All rights reserved.

    $Id$
*/

#ifndef _ZUNE_FONT_H
#define _ZUNE_FONT_H

/* Returns a given text font, if necessary it opens the font.
 * Must be called after Area's MUIM_Setup.
 */
struct TextFont *zune_font_get(Object *obj, IPTR preset);

#endif /* _ZUNE_FONT_H */
