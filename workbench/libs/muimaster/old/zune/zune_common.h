/* 
    Copyright © 1999, David Le Corfec.
    Copyright © 2002, The AROS Development Team.
    All rights reserved.

    $Id$
*/

#ifndef __ZUNE_COMMON_H__
#define __ZUNE_COMMON_H__

#ifndef INTUITION_CLASSES_H
#include <intuition/classes.h>
#endif

#define _between(a,x,b) ((x)>=(a) && (x)<=(b))
#define _isinobject(x,y) (_between(_mleft(obj),(x),_mright (obj)) \
                          && _between(_mtop(obj) ,(y),_mbottom(obj)))

/* add mask in flags if tag is true, else sub mask */
#define _handle_bool_tag(flags, tag, mask) \
((tag) ? ((flags) |= (mask)) : ((flags) &= ~(mask)))


/* some useful shortcuts for lists */
#define InitList(list) \
  ({(list)->lh_Head = (struct Node *)&(list)->lh_Tail; \
   (list)->lh_Tail = 0; \
   (list)->lh_TailPred = (struct Node *)&(list)->lh_Head;})

#define InitMinList(list) \
  ({(list)->mlh_Head = (struct MinNode *)&(list)->mlh_Tail; \
   (list)->mlh_Tail = 0; \
   (list)->mlh_TailPred = (struct MinNode *)&(list)->mlh_Head;})


#endif /* __ZUNE_COMMON_H__ */
