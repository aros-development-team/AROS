#ifndef _POPFRAME_PRIVATE_H_
#define _POPFRAME_PRIVATE_H_

#include <exec/types.h>
#include <intuition/classusr.h>

/*** Instance data **********************************************************/
struct Popframe_DATA
{
    Object *wnd;
    Object *frameadjust;
    CONST_STRPTR wintitle;
};

#endif /* _POPFRAME_PRIVATE_H_ */
