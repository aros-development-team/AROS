#ifndef _MUI_CLASSES_PANELGROUP_H
#define _MUI_CLASSES_PANELGROUP_H

/*
    Copyright (C) 2025, The AROS Development Team. All rights reserved.

    PanelGroup class public interface
*/

#ifndef LIBRARIES_MUI_H
#include <libraries/mui.h>
#endif

/* PanelGroup class identifier */
#define MUIC_PanelGroup "PanelGroup.mui"

/* PanelGroup attributes */
#define MUIA_PanelGroup_CollapseAll     (TAG_USER | 0x41000001)
#define MUIA_PanelGroup_ExpandAll       (TAG_USER | 0x41000002)
#define MUIA_PanelGroup_AllowMultiple   (TAG_USER | 0x41000003)
#define MUIA_PanelGroup_ExpandedPanel   (TAG_USER | 0x41000005)
#define MUIA_PanelGroup_DragReordering  (TAG_USER | 0x41000006)

/* PanelGroup methods */
#define MUIM_PanelGroup_CollapsePanel   (TAG_USER | 0x41000101)
#define MUIM_PanelGroup_ExpandPanel     (TAG_USER | 0x41000102)
#define MUIM_PanelGroup_TogglePanel     (TAG_USER | 0x41000103)
#define MUIM_PanelGroup_GetPanelState   (TAG_USER | 0x41000104)
#define MUIM_PanelGroup_ScanPanels      (TAG_USER | 0x41000105)

/* Method parameter structures */
struct MUIP_PanelGroup_CollapsePanel {
    STACKED ULONG MethodID;
    STACKED Object *panel;
};

struct MUIP_PanelGroup_ExpandPanel {
    STACKED ULONG MethodID;
    STACKED Object *panel;
};

struct MUIP_PanelGroup_TogglePanel {
    STACKED ULONG MethodID;
    STACKED Object *panel;
};

struct MUIP_PanelGroup_GetPanelState {
    STACKED ULONG MethodID;
    STACKED Object *panel;
};

/* Notification values */
#define MUIV_PanelGroup_Panel_Collapsed 0
#define MUIV_PanelGroup_Panel_Expanded  1

extern const struct __MUIBuiltinClass _MUI_PanelGroup_desc; /* PRIV */

#endif /* _MUI_CLASSES_PANELGROUP_H */
