/*
    Copyright © 1995-2002, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef ICONCONTAINERCLASS_H
#define ICONCONTAINERCLASS_H

#define ICA_BASE TAG_USER+1000

#define ICA_VertScroller   ICA_BASE+1
#define ICA_HorizScroller  ICA_BASE+2
#define ICA_ScrollToHoriz  ICA_BASE+3
#define ICA_ScrollToVert   ICA_BASE+4
#define ICA_JustSelected   ICA_BASE+5
#define ICA_SelectedIcons  ICA_BASE+6
#define ICA_Open           ICA_BASE+7 /* (-SG) */
#define ICA_Desktop        ICA_BASE+8 /* (ISG) */
#define ICA_DeleteMe       ICA_BASE+9
#define ICM_UnselectAll      ICA_BASE+10
#define ICM_UpdateSelectList ICA_BASE+11
#define ICM_GetColumn      ICA_BASE+12
#define ICA_ViewMode       ICA_BASE+15

#define ICAVM_LARGE  1
#define ICAVM_SMALL  2
#define ICAVM_DETAIL 3

struct MemberNode
{
    struct MinNode m_Node;
    Object *m_Object;
};

struct opGetColumn
{
    ULONG methodID;
    Tag colType;
};

struct opUpdateSelectList
{
    ULONG methodID;
    Object *target;
    ULONG selectState;
};

struct DetailColumn
{
    Tag dc_Content;
    ULONG dc_X, dc_Width;
};

struct IconContainerClassData
{
    // icons are ordered in the order they were added to the
    // container... the layouter will lay icons in columns
    // and will start a new column when there is no room
    // left (for icon view)
    struct MinList memberList;
    ULONG memberCount;

    // list of selected icons
    struct MinList selectedList;

    // this is true if the user hasn't moved any icons about
    // enables us to use a more optimized icon layouter if
    // we know where everything is.. otherwise it will search
    // for gaps
    BOOL perfectLayout;

    // only valid when perfectLayout is TRUE.. the current
    // width & height of the
    ULONG thisColumnWidth, thisColumnHeight;

    Object *vertProp, *horizProp;
    LONG xView, yView;
    LONG lastXView, lastYView;

    // one of these is set to true after a scroll
    BOOL horizScroll, vertScroll;

    // visible size of the iconcontainer - same as _mwidth()/_mheight()
    ULONG visibleWidth, visibleHeight;
    // total size of the iconcontainer
    ULONG virtualWidth, virtualHeight;

    LONG heightAdjusted, widthAdjusted;
    BOOL iconSelected;
    BOOL justSelected;
    BOOL open;
    struct MUI_EventHandlerNode ehn;
    BYTE viewMode;
    struct DetailColumn *columns;
    ULONG numColumns;
    Object *desktop;
    Object *last;
};

#define ICONSPACINGX 10
#define ICONSPACINGY 10

#endif

