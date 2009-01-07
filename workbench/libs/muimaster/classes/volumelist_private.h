#ifndef _VOLUMELIST_PRIVATE_H_
#define _VOLUMELIST_PRIVATE_H_

struct Volumelist_DATA
{
    struct Hook construct_hook;
    struct Hook destruct_hook;
    struct Hook display_hook;
};

struct Volumelist_Entry
{
    char name[100];
    char full[20];
    char free[20];
    char used[20];
    int  type;
};

#endif /* _VOLUMELIST_PRIVATE_H_ */
