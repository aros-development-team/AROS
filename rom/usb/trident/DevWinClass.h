/*****************************************************************************
** This is the DevWin custom class, a sub class of Window.mui.
******************************************************************************/
#ifndef DEVWINCLASS_H
#define DEVWINCLASS_H

#include "ActionClass.h"

struct DevWinData
{
    struct Hook InterfaceDisplayHook;
    struct DevListEntry *dlnode;
    //struct ActionData   *adata;
    struct Node *pd;
    Object      *contents;
    //Object      *infowindow;
    Object      *classpopup;
    Object      *cwnameobj;
    Object      *changenameobj;
    Object      *resetnameobj;
    Object      *langlvobj;
    Object      *cfglvobj;
    Object      *iflvobj;
    Object      *clsscanobj;
    Object      *unbindobj;
    Object      *cfgobj;
    Object      *dontpopupobj;
    Object      *noclassbindobj;
    Object      *overridepowerobj;
    struct List  iflist;
};

struct IfListEntry
{
    struct Node  node;
    Object      *infowindow;
    Object      *classpopup;
    struct Node *pif;
    char         buf[128];
};


#define TAGBASE_DevWin (TAG_USER | 342<<16)
#define MUIA_DevWin_DevEntry        (TAGBASE_DevWin | 0x0001)
#define MUIM_DevWin_Dev_Bind        (TAGBASE_DevWin | 0x0021)
#define MUIM_DevWin_If_Activate     (TAGBASE_DevWin | 0x0028)
#define MUIM_DevWin_If_Unbind       (TAGBASE_DevWin | 0x0029)
#define MUIM_DevWin_If_Config       (TAGBASE_DevWin | 0x002a)
#define MUIM_DevWin_If_FBind        (TAGBASE_DevWin | 0x002b)
#define MUIM_DevWin_SetCustomName   (TAGBASE_DevWin | 0x0030)
#define MUIM_DevWin_ResetCustomName (TAGBASE_DevWin | 0x0031)
#define MUIM_DevWin_PopupInhibitChg (TAGBASE_DevWin | 0x0040)
#define MUIM_DevWin_NoClassBindChg  (TAGBASE_DevWin | 0x0041)
#define MUIM_DevWin_PowerInfoChg    (TAGBASE_DevWin | 0x0042)

struct IfListEntry * AllocIfEntry(struct DevWinData *data, struct Node *pif, BOOL intend);
void FreeIfEntry(struct DevWinData *data, struct IfListEntry *iflnode);

AROS_UFP3(LONG, InterfaceListDisplayHook,
          AROS_UFPA(struct Hook *, hook, A0),
          AROS_UFPA(char **, strarr, A2),
          AROS_UFPA(struct IfListEntry *, iflnode, A1));

AROS_UFP3(IPTR, DevWinDispatcher,
          AROS_UFPA(struct IClass *, cl, A0),
          AROS_UFPA(Object *, obj, A2),
          AROS_UFPA(Msg, msg, A1));

#endif
