#ifndef OOP_ROOT_H
#define OOP_ROOT_H
/*
    Copyright 1995-1997 AROS - The Amiga Replacement OS
    $Id$

    Desc: Include file for meta class
    Lang: english
*/


extern ULONG __OOPI_Root;


/* Root class defs */

#define GUID_Root "Root"
#define ROOTCLASS "rootclass"

#define RootBase (__OOPI_Root)

#define MIDX_Root_New		0
#define MIDX_Root_Dispose	1

#define M_Root_New	(RootBase + MIDX_Root_New)
#define M_Root_Dispose	(RootBase + MIDX_Root_Dispose)

/* Message structs */
struct P_Root_New
{
    ULONG MethodID;
    struct TagItem *AttrList;
};

#endif /* OOP_ROOT_H */
