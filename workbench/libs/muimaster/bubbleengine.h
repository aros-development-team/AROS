/* 
    Copyright © 1999, David Le Corfec.
    Copyright © 2002, The AROS Development Team.
    All rights reserved.

    $Id$
*/

#ifndef _ZUNE_BUBBLEENGINE_H__
#define _ZUNE_BUBBLEENGINE_H__


typedef struct ZBubble
{
    struct Window *win;
    ZText   	  *ztext;
    STRPTR  	   text;
    UBYTE   	   flags;
#ifdef __AROS__
    struct Region *shape;
#endif    
} ZBubble;

#define BUBBLEF_CREATESHORTHELP_CALLED 1

APTR zune_bubble_create(Object *obj, LONG x, LONG y, char *text, ULONG flags);
void zune_bubble_delete(Object *obj, APTR bubble);



#endif
