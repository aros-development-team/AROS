/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Firewall Preferences -- PrefsEditor subclass implementation.
    Main window shows filter/NAT rule lists with add/remove buttons.
    Editing is done in separate sub-windows opened on Add or double-click.
*/

#define MUIMASTER_YES_INLINE_STDARG

#include <exec/types.h>
#include <exec/memory.h>
#include <libraries/mui.h>
#include <utility/hooks.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>
#include <proto/utility.h>
#include <clib/alib_protos.h>

#include <zune/customclasses.h>
#include <zune/prefseditor.h>

#include <stdio.h>
#include <string.h>

#include "fweditor.h"
#include "prefsdata.h"
#include "locale.h"

/* Cycle labels -- populated at runtime from locale catalog */
static CONST_STRPTR actionLabels[FW_ACTION_NUM + 1];
static CONST_STRPTR dirLabels[FW_DIR_NUM + 1];
static CONST_STRPTR protoLabels[FW_PROTO_NUM + 1];
static CONST_STRPTR natTypeLabels[FW_NAT_NUM + 1];
static CONST_STRPTR pages[3];

/*** Instance data **********************************************************/

struct FWEditor_DATA
{
    /* Main window lists */
    Object *filterList;
    Object *natList;

    /* Filter rule editing sub-window and its gadgets */
    Object *filterEditWin;
    Object *fActionCycle;
    Object *fDirCycle;
    Object *fProtoCycle;
    Object *fIfStr;
    Object *fSrcStr;
    Object *fSrcPortStr;
    Object *fDstStr;
    Object *fDstPortStr;
    Object *fQuickChk;
    Object *fKeepStateChk;
    Object *fLogChk;
    BOOL    filterIsEdit;   /* TRUE = editing existing entry */

    /* NAT rule editing sub-window and its gadgets */
    Object *natEditWin;
    Object *nTypeCycle;
    Object *nProtoCycle;
    Object *nIfStr;
    Object *nSrcStr;
    Object *nSrcPortStr;
    Object *nNatStr;
    Object *nNatPortStr;
    BOOL    natIsEdit;      /* TRUE = editing existing entry */
};

/* ===== Filter list hooks ================================================ */

AROS_UFH3S(APTR, FilterConstructFunc,
    AROS_UFHA(struct Hook *, hook, A0),
    AROS_UFHA(APTR, pool, A2),
    AROS_UFHA(struct FilterRule *, entry, A1))
{
    AROS_USERFUNC_INIT
    struct FilterRule *n = AllocPooled(pool, sizeof(struct FilterRule));
    if (n) CopyMem(entry, n, sizeof(struct FilterRule));
    return (APTR)n;
    AROS_USERFUNC_EXIT
}

AROS_UFH3S(void, FilterDestructFunc,
    AROS_UFHA(struct Hook *, hook, A0),
    AROS_UFHA(APTR, pool, A2),
    AROS_UFHA(struct FilterRule *, entry, A1))
{
    AROS_USERFUNC_INIT
    FreePooled(pool, entry, sizeof(struct FilterRule));
    AROS_USERFUNC_EXIT
}

AROS_UFH3S(LONG, FilterDisplayFunc,
    AROS_UFHA(struct Hook *, hook, A0),
    AROS_UFHA(char **, array, A2),
    AROS_UFHA(struct FilterRule *, entry, A1))
{
    AROS_USERFUNC_INIT

    static char col0[12], col1[8], col2[8], col3[48], col4[48], col5[24];

    if (entry)
    {
        strlcpy(col0, actionLabels[entry->action], sizeof(col0));
        strlcpy(col1, dirLabels[entry->direction], sizeof(col1));
        strlcpy(col2, protoLabels[entry->protocol], sizeof(col2));

        if (entry->src_addr[0])
        {
            if (entry->src_port[0])
                snprintf(col3, sizeof(col3), "%s:%s", entry->src_addr, entry->src_port);
            else
                strlcpy(col3, entry->src_addr, sizeof(col3));
        }
        else
        {
            strlcpy(col3, entry->src_port[0] ? entry->src_port : (char *)_(MSG_ANY), sizeof(col3));
        }

        if (entry->dst_addr[0])
        {
            if (entry->dst_port[0])
                snprintf(col4, sizeof(col4), "%s:%s", entry->dst_addr, entry->dst_port);
            else
                strlcpy(col4, entry->dst_addr, sizeof(col4));
        }
        else
        {
            strlcpy(col4, entry->dst_port[0] ? entry->dst_port : (char *)_(MSG_ANY), sizeof(col4));
        }

        col5[0] = '\0';
        if (entry->quick)      strlcat(col5, "Q", sizeof(col5));
        if (entry->keep_state) strlcat(col5, "S", sizeof(col5));
        if (entry->log)        strlcat(col5, "L", sizeof(col5));

        array[0] = col0;
        array[1] = col1;
        array[2] = col2;
        array[3] = col3;
        array[4] = col4;
        array[5] = col5;
    }
    else
    {
        array[0] = (char *)_(MSG_ACTION);
        array[1] = (char *)_(MSG_DIRECTION);
        array[2] = (char *)_(MSG_PROTOCOL);
        array[3] = (char *)_(MSG_SOURCE);
        array[4] = (char *)_(MSG_DESTINATION);
        array[5] = (char *)_(MSG_FLAGS);
    }

    return 0;

    AROS_USERFUNC_EXIT
}

/* ===== NAT list hooks =================================================== */

AROS_UFH3S(APTR, NATConstructFunc,
    AROS_UFHA(struct Hook *, hook, A0),
    AROS_UFHA(APTR, pool, A2),
    AROS_UFHA(struct NATRule *, entry, A1))
{
    AROS_USERFUNC_INIT
    struct NATRule *n = AllocPooled(pool, sizeof(struct NATRule));
    if (n) CopyMem(entry, n, sizeof(struct NATRule));
    return (APTR)n;
    AROS_USERFUNC_EXIT
}

AROS_UFH3S(void, NATDestructFunc,
    AROS_UFHA(struct Hook *, hook, A0),
    AROS_UFHA(APTR, pool, A2),
    AROS_UFHA(struct NATRule *, entry, A1))
{
    AROS_USERFUNC_INIT
    FreePooled(pool, entry, sizeof(struct NATRule));
    AROS_USERFUNC_EXIT
}

AROS_UFH3S(LONG, NATDisplayFunc,
    AROS_UFHA(struct Hook *, hook, A0),
    AROS_UFHA(char **, array, A2),
    AROS_UFHA(struct NATRule *, entry, A1))
{
    AROS_USERFUNC_INIT

    static char col0[16], col1[16], col2[48], col3[48], col4[12];

    if (entry)
    {
        strlcpy(col0, natTypeLabels[entry->type], sizeof(col0));
        strlcpy(col1, entry->interface, sizeof(col1));

        if (entry->src_port[0])
            snprintf(col2, sizeof(col2), "%s:%s", entry->src_addr, entry->src_port);
        else
            strlcpy(col2, entry->src_addr[0] ? entry->src_addr : (char *)_(MSG_ANY), sizeof(col2));

        if (entry->nat_port[0])
            snprintf(col3, sizeof(col3), "%s:%s", entry->nat_addr, entry->nat_port);
        else
            strlcpy(col3, entry->nat_addr[0] ? entry->nat_addr : (char *)_(MSG_ANY), sizeof(col3));

        strlcpy(col4, protoLabels[entry->protocol], sizeof(col4));

        array[0] = col0;
        array[1] = col1;
        array[2] = col2;
        array[3] = col3;
        array[4] = col4;
    }
    else
    {
        array[0] = (char *)_(MSG_NAT_TYPE);
        array[1] = (char *)_(MSG_INTERFACE);
        array[2] = (char *)_(MSG_SOURCE);
        array[3] = (char *)_(MSG_TARGET);
        array[4] = (char *)_(MSG_PROTOCOL);
    }

    return 0;

    AROS_USERFUNC_EXIT
}

/* ===== Hook structs ===================================================== */

static struct Hook filterConstructHook;
static struct Hook filterDestructHook;
static struct Hook filterDisplayHook;
static struct Hook natConstructHook;
static struct Hook natDestructHook;
static struct Hook natDisplayHook;

static void
InitHooks(void)
{
    filterConstructHook.h_Entry = (HOOKFUNC)FilterConstructFunc;
    filterDestructHook.h_Entry  = (HOOKFUNC)FilterDestructFunc;
    filterDisplayHook.h_Entry   = (HOOKFUNC)FilterDisplayFunc;
    natConstructHook.h_Entry    = (HOOKFUNC)NATConstructFunc;
    natDestructHook.h_Entry     = (HOOKFUNC)NATDestructFunc;
    natDisplayHook.h_Entry      = (HOOKFUNC)NATDisplayFunc;
}

/* ===== Helpers: gadgets <-> rule structs ================================ */

static void
GadgetsToFilterRule(struct FWEditor_DATA *data, struct FilterRule *rule)
{
    IPTR val;

    memset(rule, 0, sizeof(*rule));

    GET(data->fActionCycle, MUIA_Cycle_Active, &val);
    rule->action = (LONG)val;

    GET(data->fDirCycle, MUIA_Cycle_Active, &val);
    rule->direction = (LONG)val;

    GET(data->fProtoCycle, MUIA_Cycle_Active, &val);
    rule->protocol = (LONG)val;

    GET(data->fQuickChk, MUIA_Selected, &val);
    rule->quick = (BOOL)val;

    GET(data->fKeepStateChk, MUIA_Selected, &val);
    rule->keep_state = (BOOL)val;

    GET(data->fLogChk, MUIA_Selected, &val);
    rule->log = (BOOL)val;

    {
        STRPTR s = NULL;
        GET(data->fIfStr, MUIA_String_Contents, &s);
        if (s) strlcpy(rule->interface, s, FW_MAX_IFNAME);

        GET(data->fSrcStr, MUIA_String_Contents, &s);
        if (s) strlcpy(rule->src_addr, s, FW_MAX_ADDR_LEN);

        GET(data->fSrcPortStr, MUIA_String_Contents, &s);
        if (s) strlcpy(rule->src_port, s, FW_MAX_PORT_LEN);

        GET(data->fDstStr, MUIA_String_Contents, &s);
        if (s) strlcpy(rule->dst_addr, s, FW_MAX_ADDR_LEN);

        GET(data->fDstPortStr, MUIA_String_Contents, &s);
        if (s) strlcpy(rule->dst_port, s, FW_MAX_PORT_LEN);
    }
}

static void
FilterRuleToGadgets(struct FWEditor_DATA *data, struct FilterRule *rule)
{
    NNSET(data->fActionCycle,   MUIA_Cycle_Active,     (IPTR)rule->action);
    NNSET(data->fDirCycle,      MUIA_Cycle_Active,     (IPTR)rule->direction);
    NNSET(data->fProtoCycle,    MUIA_Cycle_Active,     (IPTR)rule->protocol);
    NNSET(data->fQuickChk,      MUIA_Selected,         (IPTR)rule->quick);
    NNSET(data->fKeepStateChk,  MUIA_Selected,         (IPTR)rule->keep_state);
    NNSET(data->fLogChk,        MUIA_Selected,         (IPTR)rule->log);
    NNSET(data->fIfStr,         MUIA_String_Contents,  (IPTR)rule->interface);
    NNSET(data->fSrcStr,        MUIA_String_Contents,  (IPTR)rule->src_addr);
    NNSET(data->fSrcPortStr,    MUIA_String_Contents,  (IPTR)rule->src_port);
    NNSET(data->fDstStr,        MUIA_String_Contents,  (IPTR)rule->dst_addr);
    NNSET(data->fDstPortStr,    MUIA_String_Contents,  (IPTR)rule->dst_port);
}

static void
ClearFilterGadgets(struct FWEditor_DATA *data)
{
    NNSET(data->fActionCycle,   MUIA_Cycle_Active,     0);
    NNSET(data->fDirCycle,      MUIA_Cycle_Active,     0);
    NNSET(data->fProtoCycle,    MUIA_Cycle_Active,     0);
    NNSET(data->fQuickChk,      MUIA_Selected,         FALSE);
    NNSET(data->fKeepStateChk,  MUIA_Selected,         FALSE);
    NNSET(data->fLogChk,        MUIA_Selected,         FALSE);
    NNSET(data->fIfStr,         MUIA_String_Contents,  (IPTR)"");
    NNSET(data->fSrcStr,        MUIA_String_Contents,  (IPTR)"");
    NNSET(data->fSrcPortStr,    MUIA_String_Contents,  (IPTR)"");
    NNSET(data->fDstStr,        MUIA_String_Contents,  (IPTR)"");
    NNSET(data->fDstPortStr,    MUIA_String_Contents,  (IPTR)"");
}

static void
GadgetsToNATRule(struct FWEditor_DATA *data, struct NATRule *rule)
{
    IPTR val;

    memset(rule, 0, sizeof(*rule));

    GET(data->nTypeCycle, MUIA_Cycle_Active, &val);
    rule->type = (LONG)val;

    GET(data->nProtoCycle, MUIA_Cycle_Active, &val);
    rule->protocol = (LONG)val;

    {
        STRPTR s = NULL;
        GET(data->nIfStr, MUIA_String_Contents, &s);
        if (s) strlcpy(rule->interface, s, FW_MAX_IFNAME);

        GET(data->nSrcStr, MUIA_String_Contents, &s);
        if (s) strlcpy(rule->src_addr, s, FW_MAX_ADDR_LEN);

        GET(data->nSrcPortStr, MUIA_String_Contents, &s);
        if (s) strlcpy(rule->src_port, s, FW_MAX_PORT_LEN);

        GET(data->nNatStr, MUIA_String_Contents, &s);
        if (s) strlcpy(rule->nat_addr, s, FW_MAX_ADDR_LEN);

        GET(data->nNatPortStr, MUIA_String_Contents, &s);
        if (s) strlcpy(rule->nat_port, s, FW_MAX_PORT_LEN);
    }
}

static void
NATRuleToGadgets(struct FWEditor_DATA *data, struct NATRule *rule)
{
    NNSET(data->nTypeCycle,   MUIA_Cycle_Active,     (IPTR)rule->type);
    NNSET(data->nProtoCycle,  MUIA_Cycle_Active,     (IPTR)rule->protocol);
    NNSET(data->nIfStr,       MUIA_String_Contents,  (IPTR)rule->interface);
    NNSET(data->nSrcStr,      MUIA_String_Contents,  (IPTR)rule->src_addr);
    NNSET(data->nSrcPortStr,  MUIA_String_Contents,  (IPTR)rule->src_port);
    NNSET(data->nNatStr,      MUIA_String_Contents,  (IPTR)rule->nat_addr);
    NNSET(data->nNatPortStr,  MUIA_String_Contents,  (IPTR)rule->nat_port);
}

static void
ClearNATGadgets(struct FWEditor_DATA *data)
{
    NNSET(data->nTypeCycle,   MUIA_Cycle_Active,     0);
    NNSET(data->nProtoCycle,  MUIA_Cycle_Active,     0);
    NNSET(data->nIfStr,       MUIA_String_Contents,  (IPTR)"");
    NNSET(data->nSrcStr,      MUIA_String_Contents,  (IPTR)"");
    NNSET(data->nSrcPortStr,  MUIA_String_Contents,  (IPTR)"");
    NNSET(data->nNatStr,      MUIA_String_Contents,  (IPTR)"");
    NNSET(data->nNatPortStr,  MUIA_String_Contents,  (IPTR)"");
}

/* ===== Sync MUI lists back to global arrays ============================= */

static void
SyncRulesFromLists(struct FWEditor_DATA *data)
{
    LONG count = 0;
    int i;

    GET(data->filterList, MUIA_List_Entries, &count);
    numFilterRules = (count > FW_MAX_FILTER_RULES) ? FW_MAX_FILTER_RULES : (int)count;
    for (i = 0; i < numFilterRules; i++)
    {
        struct FilterRule *entry = NULL;
        DoMethod(data->filterList, MUIM_List_GetEntry, i, (IPTR)&entry);
        if (entry)
            filterRules[i] = *entry;
    }

    GET(data->natList, MUIA_List_Entries, &count);
    numNatRules = (count > FW_MAX_NAT_RULES) ? FW_MAX_NAT_RULES : (int)count;
    for (i = 0; i < numNatRules; i++)
    {
        struct NATRule *entry = NULL;
        DoMethod(data->natList, MUIM_List_GetEntry, i, (IPTR)&entry);
        if (entry)
            natRules[i] = *entry;
    }
}

/* ===== Populate MUI lists from global arrays ============================ */

static void
PopulateListsFromRules(struct FWEditor_DATA *data)
{
    int i;

    DoMethod(data->filterList, MUIM_List_Clear);
    for (i = 0; i < numFilterRules; i++)
        DoMethod(data->filterList, MUIM_List_InsertSingle,
            (IPTR)&filterRules[i], MUIV_List_Insert_Bottom);

    DoMethod(data->natList, MUIM_List_Clear);
    for (i = 0; i < numNatRules; i++)
        DoMethod(data->natList, MUIM_List_InsertSingle,
            (IPTR)&natRules[i], MUIV_List_Insert_Bottom);
}

/* ==================== PREFSEDITOR METHODS ================================ */

Object *FWEditor__OM_NEW(Class *CLASS, Object *self, struct opSet *message)
{
    static const STRPTR themeImages[] =
    {
        "THEME:Images/Gadgets/small/Add",
        "THEME:Images/Gadgets/small/Delete",
        "THEME:Images/Gadgets/small/Up",
        "THEME:Images/Gadgets/small/Down",
        NULL
    };
    struct FWEditor_DATA *data;
    Object *filterList, *natList;
    Object *filterEditWin, *natEditWin;

    /* Filter editing sub-window gadgets */
    Object *fActionCycle, *fDirCycle, *fProtoCycle;
    Object *fIfStr, *fSrcStr, *fSrcPortStr, *fDstStr, *fDstPortStr;
    Object *fQuickChk, *fKeepStateChk, *fLogChk;
    Object *fUseBtn, *fCancelBtn;

    /* NAT editing sub-window gadgets */
    Object *nTypeCycle, *nProtoCycle;
    Object *nIfStr, *nSrcStr, *nSrcPortStr, *nNatStr, *nNatPortStr;
    Object *nUseBtn, *nCancelBtn;

    /* Main window buttons */
    Object *fAddBtn, *fRemBtn, *fUpBtn, *fDownBtn;
    Object *nAddBtn, *nRemBtn;

    /* Probe for themed button images */
    BOOL hasThemeImages = FALSE;
    {
        BPTR lk;
        hasThemeImages = TRUE;

        int i;
        for (i = 0; themeImages[i] != NULL; i++)
        {
            lk = Lock(themeImages[i], ACCESS_READ);
            if (lk != BNULL)
                UnLock(lk);
            else
            {
                hasThemeImages = FALSE;
                break;
            }
        }
    }

    InitHooks();

    /* Populate localized cycle and tab labels */
    actionLabels[0] = _(MSG_PASS);
    actionLabels[1] = _(MSG_BLOCK);
    actionLabels[2] = _(MSG_COUNT);
    actionLabels[3] = NULL;

    dirLabels[0] = _(MSG_IN);
    dirLabels[1] = _(MSG_OUT);
    dirLabels[2] = NULL;

    protoLabels[0] = _(MSG_ANY);
    protoLabels[1] = _(MSG_TCP);
    protoLabels[2] = _(MSG_UDP);
    protoLabels[3] = _(MSG_ICMP);
    protoLabels[4] = _(MSG_TCPUDP);
    protoLabels[5] = NULL;

    natTypeLabels[0] = _(MSG_NAT_MAP);
    natTypeLabels[1] = _(MSG_NAT_RDR);
    natTypeLabels[2] = _(MSG_NAT_BIMAP);
    natTypeLabels[3] = NULL;

    pages[0] = _(MSG_TAB_FILTER);
    pages[1] = _(MSG_TAB_NAT);
    pages[2] = NULL;

    /* --- Create filter rule editing sub-window --- */
    filterEditWin = WindowObject,
        MUIA_Window_Title, __(MSG_EDIT_FILTER),
        MUIA_Window_ID, MAKE_ID('F','E','d','t'),
        MUIA_Window_CloseGadget, TRUE,
        WindowContents, (IPTR)VGroup,
            Child, (IPTR)ColGroup(4),
                Child, (IPTR)Label2(__(MSG_ACTION)),
                Child, (IPTR)(fActionCycle = CycleObject,
                    MUIA_Cycle_Entries, (IPTR)actionLabels,
                End),
                Child, (IPTR)Label2(__(MSG_DIRECTION)),
                Child, (IPTR)(fDirCycle = CycleObject,
                    MUIA_Cycle_Entries, (IPTR)dirLabels,
                End),

                Child, (IPTR)Label2(__(MSG_PROTOCOL)),
                Child, (IPTR)(fProtoCycle = CycleObject,
                    MUIA_Cycle_Entries, (IPTR)protoLabels,
                End),
                Child, (IPTR)Label2(__(MSG_INTERFACE)),
                Child, (IPTR)(fIfStr = StringObject,
                    StringFrame,
                    MUIA_String_MaxLen, FW_MAX_IFNAME,
                End),

                Child, (IPTR)Label2(__(MSG_SOURCE)),
                Child, (IPTR)(fSrcStr = StringObject,
                    StringFrame,
                    MUIA_String_MaxLen, FW_MAX_ADDR_LEN,
                End),
                Child, (IPTR)Label2(__(MSG_SRC_PORT)),
                Child, (IPTR)(fSrcPortStr = StringObject,
                    StringFrame,
                    MUIA_String_MaxLen, FW_MAX_PORT_LEN,
                End),

                Child, (IPTR)Label2(__(MSG_DESTINATION)),
                Child, (IPTR)(fDstStr = StringObject,
                    StringFrame,
                    MUIA_String_MaxLen, FW_MAX_ADDR_LEN,
                End),
                Child, (IPTR)Label2(__(MSG_DST_PORT)),
                Child, (IPTR)(fDstPortStr = StringObject,
                    StringFrame,
                    MUIA_String_MaxLen, FW_MAX_PORT_LEN,
                End),
            End,

            Child, (IPTR)HGroup,
                Child, (IPTR)HSpace(0),
                Child, (IPTR)(fQuickChk = MUI_MakeObject(MUIO_Checkmark, __(MSG_QUICK))),
                Child, (IPTR)Label2(__(MSG_QUICK)),
                Child, (IPTR)(fKeepStateChk = MUI_MakeObject(MUIO_Checkmark, __(MSG_KEEPSTATE))),
                Child, (IPTR)Label2(__(MSG_KEEPSTATE)),
                Child, (IPTR)(fLogChk = MUI_MakeObject(MUIO_Checkmark, __(MSG_LOG))),
                Child, (IPTR)Label2(__(MSG_LOG)),
            End,

            Child, (IPTR) RectangleObject,
                MUIA_Rectangle_HBar, TRUE,
                MUIA_FixHeight,      2,
            End,

            Child, (IPTR)HGroup,
                Child, (IPTR)(fUseBtn = ImageButton(_(MSG_USE),
                                "THEME:Images/Gadgets/Use")),
                Child, (IPTR)HSpace(0),
                Child, (IPTR)(fCancelBtn = ImageButton(_(MSG_CANCEL),
                                "THEME:Images/Gadgets/Cancel")),
            End,
        End,
    End;

    /* --- Create NAT rule editing sub-window --- */
    natEditWin = WindowObject,
        MUIA_Window_Title, __(MSG_EDIT_NAT),
        MUIA_Window_ID, MAKE_ID('N','E','d','t'),
        MUIA_Window_CloseGadget, TRUE,
        WindowContents, (IPTR)VGroup,
            Child, (IPTR)ColGroup(4),
                Child, (IPTR)Label2(__(MSG_NAT_TYPE)),
                Child, (IPTR)(nTypeCycle = CycleObject,
                    MUIA_Cycle_Entries, (IPTR)natTypeLabels,
                End),
                Child, (IPTR)Label2(__(MSG_PROTOCOL)),
                Child, (IPTR)(nProtoCycle = CycleObject,
                    MUIA_Cycle_Entries, (IPTR)protoLabels,
                End),

                Child, (IPTR)Label2(__(MSG_INTERFACE)),
                Child, (IPTR)(nIfStr = StringObject,
                    StringFrame,
                    MUIA_String_MaxLen, FW_MAX_IFNAME,
                End),
                Child, (IPTR)HSpace(0),
                Child, (IPTR)HSpace(0),

                Child, (IPTR)Label2(__(MSG_SOURCE)),
                Child, (IPTR)(nSrcStr = StringObject,
                    StringFrame,
                    MUIA_String_MaxLen, FW_MAX_ADDR_LEN,
                End),
                Child, (IPTR)Label2(__(MSG_PORT)),
                Child, (IPTR)(nSrcPortStr = StringObject,
                    StringFrame,
                    MUIA_String_MaxLen, FW_MAX_PORT_LEN,
                End),

                Child, (IPTR)Label2(__(MSG_TARGET)),
                Child, (IPTR)(nNatStr = StringObject,
                    StringFrame,
                    MUIA_String_MaxLen, FW_MAX_ADDR_LEN,
                End),
                Child, (IPTR)Label2(__(MSG_TGT_PORT)),
                Child, (IPTR)(nNatPortStr = StringObject,
                    StringFrame,
                    MUIA_String_MaxLen, FW_MAX_PORT_LEN,
                End),
            End,

            Child, (IPTR) RectangleObject,
                MUIA_Rectangle_HBar, TRUE,
                MUIA_FixHeight,      2,
            End,

            Child, (IPTR)HGroup,
                Child, (IPTR)(nUseBtn = ImageButton(_(MSG_USE),
                                "THEME:Images/Gadgets/Use")),
                Child, (IPTR)HSpace(0),
                Child, (IPTR)(nCancelBtn = ImageButton(_(MSG_CANCEL),
                                "THEME:Images/Gadgets/Cancel")),
            End,
        End,
    End;

    /* --- Create main PrefsEditor window --- */
    self = (Object *)DoSuperNewTags(CLASS, self, NULL,
        MUIA_PrefsEditor_Name,     __(MSG_NAME),
        MUIA_PrefsEditor_Path,     (IPTR)"AROSTCP/ipf.rules",
        MUIA_PrefsEditor_IconTool, (IPTR)"SYS:Prefs/Firewall",

        Child, (IPTR)RegisterGroup(pages),
            /* ===== Filter Rules page -- list + buttons ===== */
            Child, (IPTR)VGroup,
                Child, (IPTR)ListviewObject,
                    MUIA_Listview_List, (IPTR)(filterList = ListObject,
                        InputListFrame,
                        MUIA_List_Title, TRUE,
                        MUIA_List_Format, (IPTR)"BAR,BAR,BAR,BAR,BAR,",
                        MUIA_List_ConstructHook, (IPTR)&filterConstructHook,
                        MUIA_List_DestructHook, (IPTR)&filterDestructHook,
                        MUIA_List_DisplayHook, (IPTR)&filterDisplayHook,
                    End),
                End,

                Child, (IPTR)HGroup,
                    MUIA_Group_SameSize, hasThemeImages ? FALSE : TRUE,
                    Child, (IPTR)HSpace(0),
                    Child, (IPTR)(fAddBtn = hasThemeImages
                        ? ImageButton(NULL, themeImages[0])
                        : SimpleButton(__(MSG_ADD))),
                    Child, (IPTR)(fRemBtn = hasThemeImages
                        ? ImageButton(NULL, themeImages[1])
                        : SimpleButton(__(MSG_REMOVE))),
                    Child, (IPTR) RectangleObject,
                        MUIA_Rectangle_VBar, TRUE,
                        MUIA_FixWidth,     1,
                    End,
                    Child, (IPTR)(fUpBtn = hasThemeImages
                        ? ImageButton(NULL, themeImages[2])
                        : SimpleButton(__(MSG_MOVEUP))),
                    Child, (IPTR)(fDownBtn = hasThemeImages
                        ? ImageButton(NULL, themeImages[3])
                        : SimpleButton(__(MSG_MOVEDOWN))),
                End,
            End,

            /* ===== NAT Rules page -- list + buttons ===== */
            Child, (IPTR)VGroup,
                Child, (IPTR)ListviewObject,
                    MUIA_Listview_List, (IPTR)(natList = ListObject,
                        InputListFrame,
                        MUIA_List_Title, TRUE,
                        MUIA_List_Format, (IPTR)"BAR,BAR,BAR,BAR,",
                        MUIA_List_ConstructHook, (IPTR)&natConstructHook,
                        MUIA_List_DestructHook, (IPTR)&natDestructHook,
                        MUIA_List_DisplayHook, (IPTR)&natDisplayHook,
                    End),
                End,

                Child, (IPTR)HGroup,
                    MUIA_Group_SameSize, hasThemeImages ? FALSE : TRUE,
                    Child, (IPTR)HSpace(0),
                    Child, (IPTR)(nAddBtn = hasThemeImages
                        ? ImageButton(NULL, themeImages[0])
                        : SimpleButton(__(MSG_ADD))),
                    Child, (IPTR)(nRemBtn = hasThemeImages
                        ? ImageButton(NULL, themeImages[1])
                        : SimpleButton(__(MSG_REMOVE))),
                End,
            End,
        End, /* RegisterGroup */

        TAG_DONE);

    if (self == NULL)
    {
        if (filterEditWin) MUI_DisposeObject(filterEditWin);
        if (natEditWin)    MUI_DisposeObject(natEditWin);
        return NULL;
    }

    if (hasThemeImages) {
        SET(fAddBtn, MUIA_ShortHelp, _(MSG_ADD));
        SET(fRemBtn, MUIA_ShortHelp, _(MSG_REMOVE));
        SET(fUpBtn, MUIA_ShortHelp, _(MSG_MOVEUP));
        SET(fDownBtn, MUIA_ShortHelp, _(MSG_MOVEDOWN));

        SET(nAddBtn, MUIA_ShortHelp, _(MSG_ADD));
        SET(nRemBtn, MUIA_ShortHelp, _(MSG_REMOVE));
    }

    data = INST_DATA(CLASS, self);
    data->filterList    = filterList;
    data->natList       = natList;
    data->filterEditWin = filterEditWin;
    data->natEditWin    = natEditWin;
    data->filterIsEdit  = FALSE;
    data->natIsEdit     = FALSE;

    /* Store editing gadgets */
    data->fActionCycle  = fActionCycle;
    data->fDirCycle     = fDirCycle;
    data->fProtoCycle   = fProtoCycle;
    data->fIfStr        = fIfStr;
    data->fSrcStr       = fSrcStr;
    data->fSrcPortStr   = fSrcPortStr;
    data->fDstStr       = fDstStr;
    data->fDstPortStr   = fDstPortStr;
    data->fQuickChk     = fQuickChk;
    data->fKeepStateChk = fKeepStateChk;
    data->fLogChk       = fLogChk;

    data->nTypeCycle    = nTypeCycle;
    data->nProtoCycle   = nProtoCycle;
    data->nIfStr        = nIfStr;
    data->nSrcStr       = nSrcStr;
    data->nSrcPortStr   = nSrcPortStr;
    data->nNatStr       = nNatStr;
    data->nNatPortStr   = nNatPortStr;

    /* Double-click on filter list opens editor for that entry */
    DoMethod(filterList, MUIM_Notify, MUIA_Listview_DoubleClick, TRUE,
        (IPTR)self, 1, MUIM_FWEditor_EditFilter);

    /* Double-click on NAT list opens editor for that entry */
    DoMethod(natList, MUIM_Notify, MUIA_Listview_DoubleClick, TRUE,
        (IPTR)self, 1, MUIM_FWEditor_EditNAT);

    /* Main window filter buttons */
    DoMethod(fAddBtn, MUIM_Notify, MUIA_Pressed, FALSE,
        (IPTR)self, 1, MUIM_FWEditor_AddFilter);
    DoMethod(fRemBtn, MUIM_Notify, MUIA_Pressed, FALSE,
        (IPTR)self, 1, MUIM_FWEditor_RemFilter);
    DoMethod(fUpBtn, MUIM_Notify, MUIA_Pressed, FALSE,
        (IPTR)self, 1, MUIM_FWEditor_UpFilter);
    DoMethod(fDownBtn, MUIM_Notify, MUIA_Pressed, FALSE,
        (IPTR)self, 1, MUIM_FWEditor_DownFilter);

    /* Main window NAT buttons */
    DoMethod(nAddBtn, MUIM_Notify, MUIA_Pressed, FALSE,
        (IPTR)self, 1, MUIM_FWEditor_AddNAT);
    DoMethod(nRemBtn, MUIM_Notify, MUIA_Pressed, FALSE,
        (IPTR)self, 1, MUIM_FWEditor_RemNAT);

    /* Filter editing sub-window buttons */
    DoMethod(fUseBtn, MUIM_Notify, MUIA_Pressed, FALSE,
        (IPTR)self, 1, MUIM_FWEditor_UseFilterEdit);
    DoMethod(fCancelBtn, MUIM_Notify, MUIA_Pressed, FALSE,
        (IPTR)self, 1, MUIM_FWEditor_CancelFilterEdit);
    /* Close gadget = cancel */
    DoMethod(filterEditWin, MUIM_Notify, MUIA_Window_CloseRequest, TRUE,
        (IPTR)self, 1, MUIM_FWEditor_CancelFilterEdit);

    /* NAT editing sub-window buttons */
    DoMethod(nUseBtn, MUIM_Notify, MUIA_Pressed, FALSE,
        (IPTR)self, 1, MUIM_FWEditor_UseNATEdit);
    DoMethod(nCancelBtn, MUIM_Notify, MUIA_Pressed, FALSE,
        (IPTR)self, 1, MUIM_FWEditor_CancelNATEdit);
    DoMethod(natEditWin, MUIM_Notify, MUIA_Window_CloseRequest, TRUE,
        (IPTR)self, 1, MUIM_FWEditor_CancelNATEdit);

    /* Populate lists from already-loaded global data */
    PopulateListsFromRules(data);

    return self;
}

/* ---- Setup/Cleanup: add/remove sub-windows to/from application --------- */

IPTR FWEditor__MUIM_Setup(Class *CLASS, Object *self, Msg message)
{
    struct FWEditor_DATA *data = INST_DATA(CLASS, self);

    if (!DoSuperMethodA(CLASS, self, message))
        return FALSE;

    /* Add editing sub-windows to the application */
    DoMethod(_app(self), OM_ADDMEMBER, (IPTR)data->filterEditWin);
    DoMethod(_app(self), OM_ADDMEMBER, (IPTR)data->natEditWin);

    return TRUE;
}

IPTR FWEditor__MUIM_Cleanup(Class *CLASS, Object *self, Msg message)
{
    struct FWEditor_DATA *data = INST_DATA(CLASS, self);

    /* Close and remove editing sub-windows */
    SET(data->filterEditWin, MUIA_Window_Open, FALSE);
    SET(data->natEditWin, MUIA_Window_Open, FALSE);
    DoMethod(_app(self), OM_REMMEMBER, (IPTR)data->filterEditWin);
    DoMethod(_app(self), OM_REMMEMBER, (IPTR)data->natEditWin);

    return DoSuperMethodA(CLASS, self, message);
}

/* ---- ImportFH: called by PrefsEditor to read rules from file handle ----- */

IPTR FWEditor__MUIM_PrefsEditor_ImportFH(
    Class *CLASS, Object *self,
    struct MUIP_PrefsEditor_ImportFH *message)
{
    struct FWEditor_DATA *data = INST_DATA(CLASS, self);
    BPTR fh = message->fh;
    char line[512];

    numFilterRules = 0;
    if (fh != BNULL)
    {
        while (FGets(fh, line, sizeof(line) - 1) &&
               numFilterRules < FW_MAX_FILTER_RULES)
        {
            struct FilterRule rule;
            if (ParseFilterLine(line, &rule))
                filterRules[numFilterRules++] = rule;
        }
    }

    ReadNATConf(PREFS_PATH_ENV);
    if (numNatRules == 0)
        ReadNATConf(PREFS_PATH_ENVARC);

    PopulateListsFromRules(data);
    return TRUE;
}

/* ---- ExportFH: called by PrefsEditor to write rules to file handle ----- */

IPTR FWEditor__MUIM_PrefsEditor_ExportFH(
    Class *CLASS, Object *self,
    struct MUIP_PrefsEditor_ExportFH *message)
{
    struct FWEditor_DATA *data = INST_DATA(CLASS, self);
    BPTR fh = message->fh;
    char line[512];
    int i;

    SyncRulesFromLists(data);

    if (fh != BNULL)
    {
        FPuts(fh, "# AROS Firewall Preferences -- filter rules for ipf(1)\n");
        FPuts(fh, "# Generated file -- edit with Firewall preferences application\n");

        for (i = 0; i < numFilterRules; i++)
        {
            FormatFilterRule(&filterRules[i], line, sizeof(line));
            FPuts(fh, line);
            FPuts(fh, "\n");
        }
    }

    return TRUE;
}

/* ---- Save: write both config files to ENVARC + ENV, apply rules -------- */

IPTR FWEditor__MUIM_PrefsEditor_Save(
    Class *CLASS, Object *self, Msg message)
{
    struct FWEditor_DATA *data = INST_DATA(CLASS, self);

    SyncRulesFromLists(data);

    if (SaveFirewallPrefs())
    {
        SET(self, MUIA_PrefsEditor_Changed, FALSE);
        SET(self, MUIA_PrefsEditor_Testing, FALSE);
        return TRUE;
    }

    return FALSE;
}

/* ---- Use: write both config files to ENV, apply rules ------------------ */

IPTR FWEditor__MUIM_PrefsEditor_Use(
    Class *CLASS, Object *self, Msg message)
{
    struct FWEditor_DATA *data = INST_DATA(CLASS, self);

    SyncRulesFromLists(data);

    if (UseFirewallPrefs())
    {
        SET(self, MUIA_PrefsEditor_Changed, FALSE);
        SET(self, MUIA_PrefsEditor_Testing, FALSE);
        return TRUE;
    }

    return FALSE;
}

/* ==================== FILTER RULE METHODS ================================ */

/* Add: open editing window with empty fields for a new rule */
IPTR FWEditor__MUIM_FWEditor_AddFilter(
    Class *CLASS, Object *self, Msg message)
{
    struct FWEditor_DATA *data = INST_DATA(CLASS, self);

    data->filterIsEdit = FALSE;
    ClearFilterGadgets(data);
    SET(data->filterEditWin, MUIA_Window_Open, TRUE);

    return 0;
}

/* EditFilter: open editing window populated with the selected entry */
IPTR FWEditor__MUIM_FWEditor_EditFilter(
    Class *CLASS, Object *self, Msg message)
{
    struct FWEditor_DATA *data = INST_DATA(CLASS, self);
    struct FilterRule *entry = NULL;
    IPTR active = MUIV_List_Active_Off;

    GET(data->filterList, MUIA_List_Active, &active);
    if (active == (IPTR)MUIV_List_Active_Off)
        return 0;

    DoMethod(data->filterList, MUIM_List_GetEntry, active, (IPTR)&entry);
    if (entry == NULL)
        return 0;

    data->filterIsEdit = TRUE;
    FilterRuleToGadgets(data, entry);
    SET(data->filterEditWin, MUIA_Window_Open, TRUE);

    return 0;
}

/* UseFilterEdit: apply the editing window's values */
IPTR FWEditor__MUIM_FWEditor_UseFilterEdit(
    Class *CLASS, Object *self, Msg message)
{
    struct FWEditor_DATA *data = INST_DATA(CLASS, self);
    struct FilterRule rule;

    GadgetsToFilterRule(data, &rule);

    if (data->filterIsEdit)
    {
        /* Update existing entry in-place */
        struct FilterRule *entry = NULL;
        IPTR active = MUIV_List_Active_Off;

        GET(data->filterList, MUIA_List_Active, &active);
        if (active != (IPTR)MUIV_List_Active_Off)
        {
            DoMethod(data->filterList, MUIM_List_GetEntry, active, (IPTR)&entry);
            if (entry)
            {
                CopyMem(&rule, entry, sizeof(struct FilterRule));
                DoMethod(data->filterList, MUIM_List_Redraw, active);
            }
        }
    }
    else
    {
        /* Insert new entry at bottom */
        DoMethod(data->filterList, MUIM_List_InsertSingle,
            (IPTR)&rule, MUIV_List_Insert_Bottom);
    }

    SET(data->filterEditWin, MUIA_Window_Open, FALSE);
    SET(self, MUIA_PrefsEditor_Changed, TRUE);

    return 0;
}

/* CancelFilterEdit: close editing window without changes */
IPTR FWEditor__MUIM_FWEditor_CancelFilterEdit(
    Class *CLASS, Object *self, Msg message)
{
    struct FWEditor_DATA *data = INST_DATA(CLASS, self);

    SET(data->filterEditWin, MUIA_Window_Open, FALSE);

    return 0;
}

IPTR FWEditor__MUIM_FWEditor_RemFilter(
    Class *CLASS, Object *self, Msg message)
{
    struct FWEditor_DATA *data = INST_DATA(CLASS, self);

    DoMethod(data->filterList, MUIM_List_Remove, MUIV_List_Remove_Active);
    SET(self, MUIA_PrefsEditor_Changed, TRUE);

    return 0;
}

IPTR FWEditor__MUIM_FWEditor_UpFilter(
    Class *CLASS, Object *self, Msg message)
{
    struct FWEditor_DATA *data = INST_DATA(CLASS, self);
    IPTR active = MUIV_List_Active_Off;

    GET(data->filterList, MUIA_List_Active, &active);
    if (active != (IPTR)MUIV_List_Active_Off && active > 0)
    {
        DoMethod(data->filterList, MUIM_List_Exchange, active, active - 1);
        SET(data->filterList, MUIA_List_Active, active - 1);
        SET(self, MUIA_PrefsEditor_Changed, TRUE);
    }

    return 0;
}

IPTR FWEditor__MUIM_FWEditor_DownFilter(
    Class *CLASS, Object *self, Msg message)
{
    struct FWEditor_DATA *data = INST_DATA(CLASS, self);
    IPTR active = MUIV_List_Active_Off;
    LONG count = 0;

    GET(data->filterList, MUIA_List_Active, &active);
    GET(data->filterList, MUIA_List_Entries, &count);
    if (active != (IPTR)MUIV_List_Active_Off && (LONG)active < count - 1)
    {
        DoMethod(data->filterList, MUIM_List_Exchange, active, active + 1);
        SET(data->filterList, MUIA_List_Active, active + 1);
        SET(self, MUIA_PrefsEditor_Changed, TRUE);
    }

    return 0;
}

/* ==================== NAT RULE METHODS ================================== */

/* Add: open NAT editing window with empty fields */
IPTR FWEditor__MUIM_FWEditor_AddNAT(
    Class *CLASS, Object *self, Msg message)
{
    struct FWEditor_DATA *data = INST_DATA(CLASS, self);

    data->natIsEdit = FALSE;
    ClearNATGadgets(data);
    SET(data->natEditWin, MUIA_Window_Open, TRUE);

    return 0;
}

/* EditNAT: open NAT editing window populated with selected entry */
IPTR FWEditor__MUIM_FWEditor_EditNAT(
    Class *CLASS, Object *self, Msg message)
{
    struct FWEditor_DATA *data = INST_DATA(CLASS, self);
    struct NATRule *entry = NULL;
    IPTR active = MUIV_List_Active_Off;

    GET(data->natList, MUIA_List_Active, &active);
    if (active == (IPTR)MUIV_List_Active_Off)
        return 0;

    DoMethod(data->natList, MUIM_List_GetEntry, active, (IPTR)&entry);
    if (entry == NULL)
        return 0;

    data->natIsEdit = TRUE;
    NATRuleToGadgets(data, entry);
    SET(data->natEditWin, MUIA_Window_Open, TRUE);

    return 0;
}

/* UseNATEdit: apply the NAT editing window's values */
IPTR FWEditor__MUIM_FWEditor_UseNATEdit(
    Class *CLASS, Object *self, Msg message)
{
    struct FWEditor_DATA *data = INST_DATA(CLASS, self);
    struct NATRule rule;

    GadgetsToNATRule(data, &rule);

    if (data->natIsEdit)
    {
        struct NATRule *entry = NULL;
        IPTR active = MUIV_List_Active_Off;

        GET(data->natList, MUIA_List_Active, &active);
        if (active != (IPTR)MUIV_List_Active_Off)
        {
            DoMethod(data->natList, MUIM_List_GetEntry, active, (IPTR)&entry);
            if (entry)
            {
                CopyMem(&rule, entry, sizeof(struct NATRule));
                DoMethod(data->natList, MUIM_List_Redraw, active);
            }
        }
    }
    else
    {
        DoMethod(data->natList, MUIM_List_InsertSingle,
            (IPTR)&rule, MUIV_List_Insert_Bottom);
    }

    SET(data->natEditWin, MUIA_Window_Open, FALSE);
    SET(self, MUIA_PrefsEditor_Changed, TRUE);

    return 0;
}

/* CancelNATEdit: close NAT editing window without changes */
IPTR FWEditor__MUIM_FWEditor_CancelNATEdit(
    Class *CLASS, Object *self, Msg message)
{
    struct FWEditor_DATA *data = INST_DATA(CLASS, self);

    SET(data->natEditWin, MUIA_Window_Open, FALSE);

    return 0;
}

IPTR FWEditor__MUIM_FWEditor_RemNAT(
    Class *CLASS, Object *self, Msg message)
{
    struct FWEditor_DATA *data = INST_DATA(CLASS, self);

    DoMethod(data->natList, MUIM_List_Remove, MUIV_List_Remove_Active);
    SET(self, MUIA_PrefsEditor_Changed, TRUE);

    return 0;
}

/* ==================== CLASS REGISTRATION ================================ */

ZUNE_CUSTOMCLASS_19
(
    FWEditor, NULL, MUIC_PrefsEditor, NULL,
    OM_NEW,                             struct opSet *,
    MUIM_Setup,                         Msg,
    MUIM_Cleanup,                       Msg,
    MUIM_PrefsEditor_ImportFH,          struct MUIP_PrefsEditor_ImportFH *,
    MUIM_PrefsEditor_ExportFH,          struct MUIP_PrefsEditor_ExportFH *,
    MUIM_PrefsEditor_Save,              Msg,
    MUIM_PrefsEditor_Use,               Msg,
    MUIM_FWEditor_AddFilter,            Msg,
    MUIM_FWEditor_EditFilter,           Msg,
    MUIM_FWEditor_UseFilterEdit,        Msg,
    MUIM_FWEditor_CancelFilterEdit,     Msg,
    MUIM_FWEditor_RemFilter,            Msg,
    MUIM_FWEditor_UpFilter,             Msg,
    MUIM_FWEditor_DownFilter,           Msg,
    MUIM_FWEditor_AddNAT,               Msg,
    MUIM_FWEditor_EditNAT,              Msg,
    MUIM_FWEditor_UseNATEdit,           Msg,
    MUIM_FWEditor_CancelNATEdit,        Msg,
    MUIM_FWEditor_RemNAT,               Msg
);
