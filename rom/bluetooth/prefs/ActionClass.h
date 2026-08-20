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

struct BtActionData
{
    Object *navlst;         /* left navigation (IconListClass) */
    Object *pagegrp;        /* page group, driven by navlst */

    Object *hwlist;
    Object *hwinfoobj;

    Object *devlist;
    Object *bt_adddev, *bt_register, *bt_unregister, *bt_pair;
    Object *bt_connect, *bt_disconnect, *bt_info, *bt_forget;
    Object *scanwin;        /* the "Add Device" discovery window */
    Object *devwin;         /* per-device information/services window */

    Object *clslist;
    Object *bt_clsscan;

    /* options page */
    Object *opt_discoverable, *opt_connectable, *opt_autoconnect;
    Object *opt_popuppairing, *opt_loginfo, *opt_logwarn, *opt_logerr, *opt_logfail;

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
    struct MinList hwentries, deventries, clsentries, errentries;

    /* live events */
    struct MsgPort         *eventport;
    APTR                    eventhandler;
    struct MUI_InputHandlerNode ihnode;
    BOOL                    ihadded;

    /* display hooks */
    struct Hook navhook, hwhook, devhook, clshook, errhook;
};

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

struct MUIP_BtA_Reply { STACKED ULONG MethodID; STACKED IPTR yes; };
#define MUIM_BtA_PairReply    (TAGBASE_BtA | 0x16)

AROS_UFP3(IPTR, ActionDispatcher,
          AROS_UFPA(struct IClass *, cl, A0),
          AROS_UFPA(Object *, obj, A2),
          AROS_UFPA(Msg, msg, A1));

#endif /* ACTIONCLASS_H */
