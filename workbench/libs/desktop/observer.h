/*
    Copyright © 1995-2002, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef OBSERVER_H
#define OBSERVER_H

#define OA_InTree                 TAG_USER+101
#define OA_Presentation           TAG_USER+102

#define OM_FreeList_Add           TAG_USER+6501

struct FreeNode
{
	struct MinNode f_Node;
	APTR f_mem;
};

struct ObserverClassData
{
	Object *presentation;
	struct MinList freeList;
};

struct __dummyObserverData__
{
    struct MUI_NotifyData mnd;
    struct ObserverClassData ocd;
};

struct ObsFreeListAddMsg
{
	Msg methodID;
	APTR free;
};

#define observerData(obj) (&(((struct __dummyObserverData__ *)(obj))->ocd))

#define _presentation(obj)    (observerData(obj)->presentation)


#endif
