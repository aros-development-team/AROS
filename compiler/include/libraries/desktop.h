/*
    Copyright � 1995-2002, The AROS Development Team. All rights reserved.
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

#define AICA_BASE TAG_USER+1800
#define AICA_SelectedIcons     AICA_BASE+1

#define DA_BASE  TAG_USER+7500
#define DA_ActiveWindow DA_BASE+1

#define DOC_ICONOP   0x10000000
#define DOC_WINDOWOP 0x20000000
#define DOC_DESKTOPOP 0x40000000

#define DOIF_CHECKED       (1<<1)
#define DOIF_CHECKABLE     (1<<2)
#define DOIF_MUTUALEXCLUDE (1<<3)

struct DesktopOperationItem
{
	ULONG doi_Code;
	ULONG doi_Number;
	UBYTE *doi_Name;
	ULONG doi_MutualExclude;
	ULONG doi_Flags;
	struct DesktopOperationItem *doi_SubItems;
};

// Tags for DoDesktopOperation()

#define DDO_Target  TAG_USER+5002

#endif /* LIBRARIES_DESKTOP_H */

