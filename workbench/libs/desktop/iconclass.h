/*
   Copyright © 1995-2002, The AROS Development Team. All rights reserved.
   $Id$
 */

#ifndef ICONCLASS_H
#    define ICONCLASS_H

#include "abstracticon.h"

#    define IA_BASE   TAG_USER+3200

#    define IA_DiskObject IA_BASE+1
#    define IA_Label      IA_BASE+2
#    define IA_Selected   IA_BASE+3
#    define IA_Executed   IA_BASE+4
#    define IA_Directory  IA_BASE+5
#    define IA_ViewMode     IA_BASE+14
#    define IA_Size         IA_BASE+15
#    define IA_LastModified IA_BASE+16
#    define IA_Type         IA_BASE+17
#    define IA_Desktop      IA_BASE+18

#    define IAVM_LARGEICON  1
#    define IAVM_SMALLICON  2
#    define IAVM_DETAIL     3

#    define WR_SELECTED     1

struct IconClassData
{
    struct DiskObject *diskObject;
    UBYTE          *label,
                   *directory;
    Object         *imagePart;
    Object         *labelPart;
    BOOL            selected;
    ULONG           lastClickSecs,
                    lastClickMicros;
    UBYTE           viewMode;
    struct MUI_EventHandlerNode ehn;
    Object         *sizePart,
                   *typePart,
                   *lastModifiedPart;
    ULONG           size;
    struct DateStamp lastChanged;
    LONG            type;
    Object         *desktop;
    BYTE            whyRedraw;
};

struct __dummyIconData__
{
    struct MUI_NotifyData mnd;
    struct MUI_AreaData mad;
    struct PresentationClassData pcd;
    struct AbstractIconClassData aic;
    struct IconClassData icd;
};

#    define iconData(obj) (&(((struct __dummyIconData__ *)(obj))->icd))

#    define _selected(obj)    (iconData(obj)->selected)
#    define _diskobject(obj)  (iconData(obj)->diskObject)


#endif
