/*
   Copyright © 1995-2002, The AROS Development Team. All rights reserved.
   $Id$ 
 */

#ifndef PRESENTATION_H
#define PRESENTATION_H

#include <libraries/mui.h>

#    define PA_InTree                 TAG_USER+101
#    define PA_Disused                TAG_USER+102
#    define PA_Observer               TAG_USER+103

struct PresentationClassData
{
    Object         *observer;
};

struct __dummyPresentationData__
{
    struct MUI_NotifyData mnd;
    struct MUI_AreaData mad;
    struct PresentationClassData pcd;
};

#    define presentationData(obj) (&(((struct __dummyPresentationData__ *)(obj))->pcd))

#    define _observer(obj)    (presentationData(obj)->observer)

#endif
