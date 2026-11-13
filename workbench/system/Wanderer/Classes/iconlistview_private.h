#ifndef _ICONLISTVIEW_PRIVATE_H_
#define _ICONLISTVIEW_PRIVATE_H_

#include <intuition/classusr.h>
#include <utility/hooks.h>

/*** Instance data **********************************************************/
struct IconListview_DATA
{
    Object *iconlist;
    Object *vert, *horiz, *button;
    struct Hook hook;
    struct Hook *layout_hook;
};

#endif /* _ICONLISTVIEW_PRIVATE_H_ */
