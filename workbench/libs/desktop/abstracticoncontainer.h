/*
   Copyright © 1995-2002, The AROS Development Team. All rights reserved.
   $Id$
 */

#ifndef ABSTRACTICONCONTAINER_H
#define ABSTRACTICONCONTAINER_H

#include "presentation.h"

#define AICA_BASE TAG_USER+1800

#define AICA_SelectedIcons     AICA_BASE+1 // (--G)
#define AICA_MemberList        AICA_BASE+2 // (--G)
#define AICA_Desktop               AICA_BASE+3 // (ISG)
#define AICA_ApplyMethodsToMembers AICA_BASE+4 // (ISG)

// Unselect all icons.
#define AICM_UnselectAll      AICA_BASE+4 // args: none

// an icon selection state has changed, call this method to
// update the selected list.
#define AICM_UpdateSelectList AICA_BASE+5 // args: Object *target, ULONG selectState

struct MemberNode
{
    struct MinNode  m_Node;
    Object         *m_Object;
};

struct opUpdateSelectList
{
    ULONG           methodID;
    Object         *target;
    ULONG           selectState;
};

struct AbstractIconContainerData
{
    // list of MemberNodes
    struct MinList memberList;
    ULONG  memberCount;
    // list of MemberNodes
    struct MinList selectedList;
    // the container is on this desktop
    Object *desktop;
    BOOL applyMethodsToMembers;
};

struct __dummyAbstractIconContainerData__
{
    struct MUI_NotifyData mnd;
    struct MUI_AreaData mad;
    struct PresentationClassData ocd;
    struct AbstractIconContainerData aicd;
};

#define abstracticonContainerData(obj) (&(((struct __dummyAbstractIconContainerData__*)(obj))->aicd))

#define _memberList(obj) (abstracticonContainerData(obj)->memberList)
#define _memberCount(obj) (abstracticonContainerData(obj)->memberCount)

#endif

