#ifndef _POPSCREEN_PRIVATE_H_
#define _POPSCREEN_PRIVATE_H_

struct Popscreen_DATA
{
    struct Hook strobj_hook;
    struct Hook objstr_hook;
    Object  	*list;
};

#endif /* _POPSCREEN_PRIVATE_H_ */
