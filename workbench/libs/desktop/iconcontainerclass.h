/*
   Copyright © 1995-2002, The AROS Development Team. All rights reserved.
   $Id$ 
 */

#ifndef ICONCONTAINERCLASS_H
#define ICONCONTAINERCLASS_H

#include "abstracticoncontainer.h"

#    define ICA_BASE TAG_USER+1000

#    define ICA_VertScroller   ICA_BASE+1
#    define ICA_HorizScroller  ICA_BASE+2
#    define ICA_ScrollToHoriz  ICA_BASE+3
#    define ICA_ScrollToVert   ICA_BASE+4
#    define ICA_Open           ICA_BASE+7
                                        /* (-SG) */
                                        /* (ISG) */
#    define ICA_DeleteMe       ICA_BASE+9
#    define ICM_GetColumn      ICA_BASE+12
#    define ICA_ViewMode       ICA_BASE+15

#    define ICAVM_LARGE  1
#    define ICAVM_SMALL  2
#    define ICAVM_DETAIL 3

struct opGetColumn
{
    ULONG           methodID;
    Tag             colType;
};

struct DetailColumn
{
    Tag             dc_Content;
    ULONG           dc_X,
                    dc_Width;
};

struct IconContainerClassData
{
// this is true if the user hasn't moved any icons about
// enables us to use a more optimized icon layouter if
// we know where everything is.. otherwise it will search
// for gaps
    BOOL            perfectLayout;

// only valid when perfectLayout is TRUE.. the current
// width & height of the
    ULONG           thisColumnWidth,
                    thisColumnHeight;

    Object         *vertProp,
                   *horizProp;
    LONG            xView,
                    yView;
    LONG            lastXView,
                    lastYView;

// one of these is set to true after a scroll
    BOOL            horizScroll,
                    vertScroll;

// visible size of the iconcontainer - same as _mwidth()/_mheight()
    ULONG           visibleWidth,
                    visibleHeight;
// total size of the iconcontainer
    ULONG           virtualWidth,
                    virtualHeight;

    LONG            heightAdjusted,
                    widthAdjusted;
    BOOL            iconSelected;
    BOOL            justSelected;
    BOOL            open;
    struct MUI_EventHandlerNode ehn;
    BYTE            viewMode;
    struct DetailColumn *columns;
    ULONG           numColumns;
    Object         *last;
};

#    define ICONSPACINGX 10
#    define ICONSPACINGY 10

#endif
