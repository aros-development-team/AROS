#ifndef _POPIMAGE_PRIVATE_H_
#define _POPIMAGE_PRIVATE_H_

#include <exec/types.h>
#include <intuition/classusr.h>

/*** Instance data **********************************************************/
struct Popimage_DATA
{
    Object *wnd;
    Object *imageadjust;
    ULONG adjust_type;
    CONST_STRPTR wintitle;
};

#endif /* _POPIMAGE_PRIVATE_H_ */
