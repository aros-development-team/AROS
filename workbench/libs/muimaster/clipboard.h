/* 
    Copyright © 1999, David Le Corfec.
    Copyright © 2002, The AROS Development Team.
    All rights reserved.

    $Id$
*/

#ifndef _ZUNE_CLIPBOARD_H__
#define _ZUNE_CLIPBOARD_H__

void   clipboard_write_text(STRPTR text, LONG textlen);
STRPTR clipboard_read_text(void);
void   clipboard_free_text(STRPTR text);

#endif
