#ifndef _POPLIST_PRIVATE_H_
#define _POPLIST_PRIVATE_H_

struct Poplist_DATA
{
    struct Hook strobj_hook;
    struct Hook objstr_hook;
    Object  	*list;
};

#endif /* _POPLIST_PRIVATE_H_ */
