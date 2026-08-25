/*
** ActionClass - the main content class of the Bluetooth prefs, a subclass of
** Group.mui. Mirrors Trident's ActionClass: it builds the whole panel (left
** navigation, per-page groups, always-on message log and the bottom button
** bar) and implements every action as a method. All state lives in its
** instance data, so the input loop in main() only needs to watch for "quit".
*/

#ifndef ACTIONCLASS_H
#define ACTIONCLASS_H

#include <exec/types.h>
#include <exec/lists.h>
#include <exec/ports.h>
#include <intuition/classusr.h>
#include <utility/hooks.h>
#include <libraries/mui.h>

/* one line in each of the panels' lists */
struct HWEntry  { struct MinNode node; APTR bth; ULONG icon; char name[64]; char addr[24]; char state[16]; char prod[64]; char info[64]; };
struct DevEntry { struct MinNode node; APTR bd;  ULONG icon; ULONG statusicon; char addr[24]; char name[48]; char type[10]; char flags[48]; LONG rssi; };
struct SvcEntry { struct MinNode node; APTR bsv; char name[48]; char type[8]; char uuid[12]; char proto[40]; char binding[40]; };
struct ClsEntry { struct MinNode node; APTR bc;  ULONG icon; char name[48]; char use[8];   char path[80]; };
struct ErrEntry { struct MinNode node; char level[8]; char origin[24]; char msg[160]; };
/* one config form (or forced binding chunk) on the Config page, Trident style */
struct CfgEntry { struct MinNode node; ULONG formid; ULONG parentid; ULONG size; char type[20]; char desc[96]; char owner[40]; char devid[32]; };

struct BtActionData
{
    Object *navlst;         /* left navigation (IconListClass) */
    Object *pagegrp;        /* page group, driven by navlst */

    Object *hwlist;
    Object *hwinfoobj;
    Object *hwdevobj, *hwunitobj;   /* manual "add radio": HCI device name + unit */
    Object *bt_hwadd, *bt_hwremove;

    Object *devlist;
    Object *bt_adddev, *bt_register, *bt_unregister, *bt_pair;
    Object *bt_connect, *bt_disconnect, *bt_info, *bt_forget;
    Object *bt_devsettings;  /* the bound class's per-device settings window (Trident "Settings") */
    Object *scanwin;        /* the "Add Device" discovery window */
    Object *devwin;         /* per-device information/services window */

    Object *clslist;
    Object *bt_clsscan, *bt_clscfg;   /* class defaults settings window (Trident "Configure") */

    /* options page (the stack's global config, BGA_STACKCFG) */
    Object *opt_discoverable, *opt_connectable, *opt_autoconnect;
    Object *opt_popuppairing, *opt_loginfo, *opt_logwarn, *opt_logerr, *opt_logfail;
    Object *opt_localname, *opt_disctime, *opt_popupnew, *opt_popupgone;
    Object *opt_popupdelay, *opt_popupactivate, *opt_popuptofront, *opt_taskpri;
    BOOL    optloading;     /* filling the gadgets: ignore their notifications */

    /* config page: every form in the stack's config, Trident's "Config" panel */
    Object *cfglist;
    Object *bt_cfgexport, *bt_cfgimport, *bt_cfgremove;

    /* message log + bottom bar */
    Object *errlist, *errlvl;
    Object *bt_flush, *bt_save, *bt_savelog;
    Object *bt_allon, *bt_alloff, *bt_restart, *bt_use;
    Object *statustxt;

    /* pairing request popup */
    Object *pairwin, *pairtext, *pairyes, *pairno;
    APTR    pairdev;
    ULONG   pairtype;

    /* list backing stores */
    struct MinList hwentries, deventries, clsentries, errentries, cfgentries;

    /* live events */
    struct MsgPort         *eventport;
    APTR                    eventhandler;
    struct MUI_InputHandlerNode ihnode;
    BOOL                    ihadded;

    /* display hooks */
    struct Hook navhook, hwhook, devhook, clshook, errhook, cfghook;
};

/* incremental list update shared by the Devices page and the Add Device window (ActionClass.c) */
void MergeDevList(Object *list, struct MinList *entries, struct MinList *fresh);

#define TAGBASE_BtA (TAG_USER | 0x1a00)

#define MUIM_BtA_Scan         (TAGBASE_BtA | 0x01)
#define MUIM_BtA_Stop         (TAGBASE_BtA | 0x02)
#define MUIM_BtA_Register     (TAGBASE_BtA | 0x03)
#define MUIM_BtA_Unregister   (TAGBASE_BtA | 0x04)
#define MUIM_BtA_Pair         (TAGBASE_BtA | 0x05)
#define MUIM_BtA_Connect      (TAGBASE_BtA | 0x06)
#define MUIM_BtA_Disconnect   (TAGBASE_BtA | 0x07)
#define MUIM_BtA_Services     (TAGBASE_BtA | 0x08)
#define MUIM_BtA_Forget       (TAGBASE_BtA | 0x09)
#define MUIM_BtA_DevActive    (TAGBASE_BtA | 0x0a)
#define MUIM_BtA_HwActive     (TAGBASE_BtA | 0x0b)
#define MUIM_BtA_HwInfo       (TAGBASE_BtA | 0x0c)
#define MUIM_BtA_ClsScan      (TAGBASE_BtA | 0x0d)
#define MUIM_BtA_FlushLog     (TAGBASE_BtA | 0x0e)
#define MUIM_BtA_Save         (TAGBASE_BtA | 0x0f)
#define MUIM_BtA_Use          (TAGBASE_BtA | 0x10)
#define MUIM_BtA_UseQuit      (TAGBASE_BtA | 0x11)
#define MUIM_BtA_Restart      (TAGBASE_BtA | 0x12)
#define MUIM_BtA_AllOnline    (TAGBASE_BtA | 0x13)
#define MUIM_BtA_AllOffline   (TAGBASE_BtA | 0x14)
#define MUIM_BtA_HandleEvents (TAGBASE_BtA | 0x15)
#define MUIM_BtA_AddDevice    (TAGBASE_BtA | 0x17)
#define MUIM_BtA_DevInfo      (TAGBASE_BtA | 0x18)
#define MUIM_BtA_SaveLog      (TAGBASE_BtA | 0x19)
#define MUIM_BtA_HwAdd        (TAGBASE_BtA | 0x1a)
#define MUIM_BtA_HwRemove     (TAGBASE_BtA | 0x1b)
#define MUIM_BtA_OptChanged   (TAGBASE_BtA | 0x1c)   /* an Options page gadget changed */
#define MUIM_BtA_CfgActive    (TAGBASE_BtA | 0x1d)   /* Config page selection changed */
#define MUIM_BtA_CfgExport    (TAGBASE_BtA | 0x1e)
#define MUIM_BtA_CfgImport    (TAGBASE_BtA | 0x1f)
#define MUIM_BtA_CfgRemove    (TAGBASE_BtA | 0x20)
#define MUIM_BtA_ClsConfigure (TAGBASE_BtA | 0x21)   /* open the class's default settings window */
#define MUIM_BtA_DevSettings  (TAGBASE_BtA | 0x22)   /* open the binding settings window(s) of a device */
#define MUIM_BtA_ClsActive    (TAGBASE_BtA | 0x23)

struct MUIP_BtA_Reply { STACKED ULONG MethodID; STACKED IPTR yes; };
#define MUIM_BtA_PairReply    (TAGBASE_BtA | 0x16)

AROS_UFP3(IPTR, ActionDispatcher,
          AROS_UFPA(struct IClass *, cl, A0),
          AROS_UFPA(Object *, obj, A2),
          AROS_UFPA(Msg, msg, A1));

#endif /* ACTIONCLASS_H */
