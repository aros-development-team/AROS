#ifndef _POPPEN_PRIVATE_H_
#define _POPPEN_PRIVATE_H_

#include <exec/types.h>
#include <intuition/classusr.h>

/*** Instance data **********************************************************/
struct Poppen_DATA
{
    Object *wnd;
    Object *penadjust;
    CONST_STRPTR wintitle;
};

#endif /* _POPPEN_PRIVATE_H_ */
