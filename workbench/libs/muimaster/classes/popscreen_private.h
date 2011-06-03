#ifndef _POPSCREEN_PRIVATE_H_
#define _POPSCREEN_PRIVATE_H_

#include <utility/hooks.h>
#include <intuition/classusr.h>

struct Popscreen_DATA
{
    struct Hook strobj_hook;
    struct Hook objstr_hook;
    Object  	*list;
};

#endif /* _POPSCREEN_PRIVATE_H_ */
