#ifndef OOP_ROOT_H
#define OOP_ROOT_H
/*
    Copyright 1995-1997 AROS - The Amiga Replacement OS
    $Id$

    Desc: Include file for meta class
    Lang: english
*/

#ifndef OOP_OOP_H
#   include <oop/oop.h>
#endif


/* Root class defs */

#define IID_Root "Root"
#define CLID_Root "rootclass"


enum
{
    MO_Root_New = 0,
    MO_Root_Dispose,
    MO_Root_Set,
    MO_Root_Get,
    
    NUM_M_Root
};
    


/* Message structs */
struct P_Root_New
{
    MethodID MID;
    struct TagItem *AttrList;
};

struct P_Root_Set
{
    MethodID MID;
    struct TagItem *AttrList;
};

struct P_Root_Get
{
    MethodID MID;
    ULONG AttrID;
    IPTR *Storage;
};


#endif /* OOP_ROOT_H */
