/*
    Copyright © 1995-2002, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef LIBRARIES_DESKTOP_H
#define LIBRARIES_DESKTOP_H

/*** Kinds for use with CreateDesktopObjectA() ****************/

#define CDO_DirectoryWindow               1
#define CDO_IconContainer                 2
#define CDO_DiskIcon                      3
#define CDO_DrawerIcon                    4
#define CDO_ToolIcon                      5
#define CDO_ProjectIcon                   6
#define CDO_TrashcanIcon                  7
#define CDO_Desktop                       8
#define CDO_DesktopWindow                 9

/*** Tags for use with CreateDesktopObjectA() *****************/

#define ICOA_Directory            TAG_USER+1

#define ICA_BASE TAG_USER+1000

#define ICA_VertScroller  ICA_BASE+1
#define ICA_HorizScroller ICA_BASE+2
#define ICA_ScrollToHoriz ICA_BASE+3
#define ICA_ScrollToVert  ICA_BASE+4
#define ICA_SelectedIcons ICA_BASE+6

#define DA_BASE  TAG_USER+7500
#define DA_ActiveWindows DA_BASE+1

#define DOC_ICONOP   0x10000000
#define DOC_WINDOWOP 0x20000000
#define DOC_DESKTOPOP 0x40000000

struct DesktopOperationItem
{
	ULONG doi_Code;
	UBYTE *doi_Name;
};

// Tags for DoDesktopOperation()

#define DDO_Target  TAG_USER+5002

#endif /* LIBRARIES_DESKTOP_H */

