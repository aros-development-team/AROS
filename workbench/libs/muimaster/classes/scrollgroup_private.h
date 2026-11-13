#ifndef _SCROLLGROUP_PRIVATE_H_
#define _SCROLLGROUP_PRIVATE_H_

#include <exec/types.h>
#include <intuition/classusr.h>
#include <utility/hooks.h>

/*** Instance data **********************************************************/
struct Scrollgroup_DATA
{
    Object *contents;
    Object *vert, *horiz, *button;
    struct Hook hook;
    struct Hook *layout_hook;
    BOOL usewinborder;
};

#endif /* _SCROLLGROUP_PRIVATE_H_ */
