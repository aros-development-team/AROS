/*
   Copyright © 1995-2002, The AROS Development Team. All rights reserved.
   $Id$ 
 */

#ifndef OBSERVER_H
#    define OBSERVER_H

#    define OA_Base                   TAG_USER+4200

#    define OA_InTree                 OA_Base+1
#    define OA_Presentation           OA_Base+2
#    define OA_Parent                 OA_Base+3
#    define OA_Disused                OA_Base+4

#    define OM_FreeList_Add           OA_Base+5
#    define OM_Delete                 OA_Base+6

struct FreeNode
{
    struct MinNode  f_Node;
    APTR            f_mem;
};

struct ObserverClassData
{
    Object         *presentation;
    Object         *parent;
    struct MinList  freeList;
    BOOL            inTree;
};

struct __dummyObserverData__
{
    struct MUI_NotifyData mnd;
    struct ObserverClassData ocd;
};

struct ObsFreeListAddMsg
{
    Msg             methodID;
    APTR            free;
};

struct ObsDeleteMsg
{
    Msg             methodID;
    Object         *obj;
};

#    define observerData(obj) (&(((struct __dummyObserverData__ *)(obj))->ocd))

#    define _presentation(obj)    (observerData(obj)->presentation)
#    define _o_parent(obj)    (observerData(obj)->parent)


#endif
