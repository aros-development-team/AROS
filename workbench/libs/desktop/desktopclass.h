/*
   Copyright © 1995-2002, The AROS Development Team. All rights reserved.
   $Id$ 
 */

#ifndef DESKTOPCLASS_H
#    define DESKTOPCLASS_H

#    define DA_BASE  TAG_USER+7500

#    define DA_ActiveWindow  DA_BASE+1
#    define DM_DeleteWindow  DA_BASE+2

struct DesktopClassData
{
    Object         *activeWindow;
};


#endif
