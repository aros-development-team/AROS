/*
    Copyright © 1995-2002, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef DESKTOPCLASS_H
#define DESKTOPCLASS_H

#define DA_BASE  TAG_USER+7500

#define DA_ActiveWindows DESKTOP_BASE+1
#define DM_DeleteWindow  DESKTOP_BASE+2

struct DesktopClassData
{
	ULONG dummy;
};


#endif
