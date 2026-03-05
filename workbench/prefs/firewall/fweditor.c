/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Firewall Preferences -- PrefsEditor subclass implementation.
    Provides a tabbed filter / NAT rule editor that generates
    ipf.conf and ipnat.conf for AROSTCP's packet filter.
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

/* Cycle labels */
static const char *actionLabels[]   = { "Pass", "Block", "Count", NULL };
static const char *dirLabels[]      = { "In", "Out", NULL };
static const char *protoLabels[]    = { "Any", "TCP", "UDP", "ICMP", "TCP/UDP", NULL };
static const char *natTypeLabels[]  = { "Map (SNAT)", "Redirect (DNAT)", "Bidirectional", NULL };

/*** Instance data **********************************************************/

struct FWEditor_DATA
{
    /* Filter rules page */
    Object *filterList;
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

    /* NAT rules page */
    Object *natList;
    Object *nTypeCycle;
    Object *nProtoCycle;
    Object *nIfStr;
    Object *nSrcStr;
    Object *nSrcPortStr;
    Object *nNatStr;
    Object *nNatPortStr;
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
        static const char *anames[] = { "Pass", "Block", "Count" };
        static const char *dnames[] = { "In", "Out" };
        static const char *pnames[] = { "Any", "TCP", "UDP", "ICMP", "TCP/UDP" };

        strlcpy(col0, anames[entry->action], sizeof(col0));
        strlcpy(col1, dnames[entry->direction], sizeof(col1));
        strlcpy(col2, pnames[entry->protocol], sizeof(col2));

        if (entry->src_addr[0])
        {
            if (entry->src_port[0])
                snprintf(col3, sizeof(col3), "%s:%s", entry->src_addr, entry->src_port);
            else
                strlcpy(col3, entry->src_addr, sizeof(col3));
        }
        else
        {
            strlcpy(col3, entry->src_port[0] ? entry->src_port : "any", sizeof(col3));
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
            strlcpy(col4, entry->dst_port[0] ? entry->dst_port : "any", sizeof(col4));
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
        array[0] = "\033bAction";
        array[1] = "\033bDir";
        array[2] = "\033bProto";
        array[3] = "\033bSource";
        array[4] = "\033bDest";
        array[5] = "\033bFlags";
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
        static const char *tnames[] = { "Map", "Rdr", "Bimap" };
        static const char *pnames[] = { "Any", "TCP", "UDP", "ICMP", "TCP/UDP" };

        strlcpy(col0, tnames[entry->type], sizeof(col0));
        strlcpy(col1, entry->interface, sizeof(col1));

        if (entry->src_port[0])
            snprintf(col2, sizeof(col2), "%s:%s", entry->src_addr, entry->src_port);
        else
            strlcpy(col2, entry->src_addr[0] ? entry->src_addr : "any", sizeof(col2));

        if (entry->nat_port[0])
            snprintf(col3, sizeof(col3), "%s:%s", entry->nat_addr, entry->nat_port);
        else
            strlcpy(col3, entry->nat_addr[0] ? entry->nat_addr : "any", sizeof(col3));

        strlcpy(col4, pnames[entry->protocol], sizeof(col4));

        array[0] = col0;
        array[1] = col1;
        array[2] = col2;
        array[3] = col3;
        array[4] = col4;
    }
    else
    {
        array[0] = "\033bType";
        array[1] = "\033bIface";
        array[2] = "\033bSource";
        array[3] = "\033bTarget";
        array[4] = "\033bProto";
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
    struct FWEditor_DATA *data;
    Object *filterList, *natList;
    Object *fActionCycle, *fDirCycle, *fProtoCycle;
    Object *fIfStr, *fSrcStr, *fSrcPortStr, *fDstStr, *fDstPortStr;
    Object *fQuickChk, *fKeepStateChk, *fLogChk;
    Object *nTypeCycle, *nProtoCycle;
    Object *nIfStr, *nSrcStr, *nSrcPortStr, *nNatStr, *nNatPortStr;
    Object *fAddBtn, *fRemBtn, *fUpBtn, *fDownBtn, *fUpdateBtn;
    Object *nAddBtn, *nRemBtn, *nUpdateBtn;

    static const char *pages[] = { "Filter Rules", "NAT Rules", NULL };

    InitHooks();

    self = (Object *)DoSuperNewTags(CLASS, self, NULL,
        MUIA_PrefsEditor_Name,     __(MSG_NAME),
        MUIA_PrefsEditor_Path,     (IPTR)"AROSTCP/ipf.conf",
        MUIA_PrefsEditor_IconTool, (IPTR)"SYS:Prefs/Firewall",

        Child, (IPTR)RegisterGroup(pages),
            /* ===== Filter Rules page ===== */
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

                Child, (IPTR)ColGroup(4),
                    Child, (IPTR)Label2("Action:"),
                    Child, (IPTR)(fActionCycle = CycleObject,
                        MUIA_Cycle_Entries, (IPTR)actionLabels,
                    End),
                    Child, (IPTR)Label2("Direction:"),
                    Child, (IPTR)(fDirCycle = CycleObject,
                        MUIA_Cycle_Entries, (IPTR)dirLabels,
                    End),

                    Child, (IPTR)Label2("Protocol:"),
                    Child, (IPTR)(fProtoCycle = CycleObject,
                        MUIA_Cycle_Entries, (IPTR)protoLabels,
                    End),
                    Child, (IPTR)Label2("Interface:"),
                    Child, (IPTR)(fIfStr = StringObject,
                        StringFrame,
                        MUIA_String_MaxLen, FW_MAX_IFNAME,
                    End),

                    Child, (IPTR)Label2("Source:"),
                    Child, (IPTR)(fSrcStr = StringObject,
                        StringFrame,
                        MUIA_String_MaxLen, FW_MAX_ADDR_LEN,
                    End),
                    Child, (IPTR)Label2("Src Port:"),
                    Child, (IPTR)(fSrcPortStr = StringObject,
                        StringFrame,
                        MUIA_String_MaxLen, FW_MAX_PORT_LEN,
                    End),

                    Child, (IPTR)Label2("Destination:"),
                    Child, (IPTR)(fDstStr = StringObject,
                        StringFrame,
                        MUIA_String_MaxLen, FW_MAX_ADDR_LEN,
                    End),
                    Child, (IPTR)Label2("Dst Port:"),
                    Child, (IPTR)(fDstPortStr = StringObject,
                        StringFrame,
                        MUIA_String_MaxLen, FW_MAX_PORT_LEN,
                    End),
                End,

                Child, (IPTR)HGroup,
                    Child, (IPTR)(fQuickChk = MUI_MakeObject(MUIO_Checkmark, (IPTR)"Quick")),
                    Child, (IPTR)Label2("Quick"),
                    Child, (IPTR)(fKeepStateChk = MUI_MakeObject(MUIO_Checkmark, (IPTR)"Keep State")),
                    Child, (IPTR)Label2("Keep State"),
                    Child, (IPTR)(fLogChk = MUI_MakeObject(MUIO_Checkmark, (IPTR)"Log")),
                    Child, (IPTR)Label2("Log"),
                    Child, (IPTR)HSpace(0),
                End,

                Child, (IPTR)HGroup,
                    Child, (IPTR)(fAddBtn = SimpleButton("Add")),
                    Child, (IPTR)(fUpdateBtn = SimpleButton("Update")),
                    Child, (IPTR)(fRemBtn = SimpleButton("Remove")),
                    Child, (IPTR)(fUpBtn = SimpleButton("Move Up")),
                    Child, (IPTR)(fDownBtn = SimpleButton("Move Down")),
                End,
            End,

            /* ===== NAT Rules page ===== */
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

                Child, (IPTR)ColGroup(4),
                    Child, (IPTR)Label2("Type:"),
                    Child, (IPTR)(nTypeCycle = CycleObject,
                        MUIA_Cycle_Entries, (IPTR)natTypeLabels,
                    End),
                    Child, (IPTR)Label2("Protocol:"),
                    Child, (IPTR)(nProtoCycle = CycleObject,
                        MUIA_Cycle_Entries, (IPTR)protoLabels,
                    End),

                    Child, (IPTR)Label2("Interface:"),
                    Child, (IPTR)(nIfStr = StringObject,
                        StringFrame,
                        MUIA_String_MaxLen, FW_MAX_IFNAME,
                    End),
                    Child, (IPTR)Label2("Source:"),
                    Child, (IPTR)(nSrcStr = StringObject,
                        StringFrame,
                        MUIA_String_MaxLen, FW_MAX_ADDR_LEN,
                    End),

                    Child, (IPTR)Label2("Port:"),
                    Child, (IPTR)(nSrcPortStr = StringObject,
                        StringFrame,
                        MUIA_String_MaxLen, FW_MAX_PORT_LEN,
                    End),
                    Child, (IPTR)Label2("Target:"),
                    Child, (IPTR)(nNatStr = StringObject,
                        StringFrame,
                        MUIA_String_MaxLen, FW_MAX_ADDR_LEN,
                    End),

                    Child, (IPTR)Label2("Tgt Port:"),
                    Child, (IPTR)(nNatPortStr = StringObject,
                        StringFrame,
                        MUIA_String_MaxLen, FW_MAX_PORT_LEN,
                    End),
                    Child, (IPTR)HSpace(0),
                End,

                Child, (IPTR)HGroup,
                    Child, (IPTR)(nAddBtn = SimpleButton("Add")),
                    Child, (IPTR)(nUpdateBtn = SimpleButton("Update")),
                    Child, (IPTR)(nRemBtn = SimpleButton("Remove")),
                    Child, (IPTR)HSpace(0),
                End,
            End,
        End, /* RegisterGroup */

        TAG_DONE);

    if (self == NULL)
        return NULL;

    data = INST_DATA(CLASS, self);
    data->filterList    = filterList;
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

    data->natList       = natList;
    data->nTypeCycle    = nTypeCycle;
    data->nProtoCycle   = nProtoCycle;
    data->nIfStr        = nIfStr;
    data->nSrcStr       = nSrcStr;
    data->nSrcPortStr   = nSrcPortStr;
    data->nNatStr       = nNatStr;
    data->nNatPortStr   = nNatPortStr;

    /* List selection populates editing gadgets */
    DoMethod(filterList, MUIM_Notify, MUIA_List_Active, MUIV_EveryTime,
        (IPTR)self, 1, MUIM_FWEditor_Refresh);
    DoMethod(natList, MUIM_Notify, MUIA_List_Active, MUIV_EveryTime,
        (IPTR)self, 1, MUIM_FWEditor_Refresh);

    /* Filter buttons */
    DoMethod(fAddBtn, MUIM_Notify, MUIA_Pressed, FALSE,
        (IPTR)self, 1, MUIM_FWEditor_AddFilter);
    DoMethod(fUpdateBtn, MUIM_Notify, MUIA_Pressed, FALSE,
        (IPTR)self, 1, MUIM_FWEditor_UpdateFilter);
    DoMethod(fRemBtn, MUIM_Notify, MUIA_Pressed, FALSE,
        (IPTR)self, 1, MUIM_FWEditor_RemFilter);
    DoMethod(fUpBtn, MUIM_Notify, MUIA_Pressed, FALSE,
        (IPTR)self, 1, MUIM_FWEditor_UpFilter);
    DoMethod(fDownBtn, MUIM_Notify, MUIA_Pressed, FALSE,
        (IPTR)self, 1, MUIM_FWEditor_DownFilter);

    /* NAT buttons */
    DoMethod(nAddBtn, MUIM_Notify, MUIA_Pressed, FALSE,
        (IPTR)self, 1, MUIM_FWEditor_AddNAT);
    DoMethod(nUpdateBtn, MUIM_Notify, MUIA_Pressed, FALSE,
        (IPTR)self, 1, MUIM_FWEditor_UpdateNAT);
    DoMethod(nRemBtn, MUIM_Notify, MUIA_Pressed, FALSE,
        (IPTR)self, 1, MUIM_FWEditor_RemNAT);

    /* Populate lists from already-loaded global data */
    PopulateListsFromRules(data);

    return self;
}

/* ---- ImportFH: called by PrefsEditor to refresh from data -------------- */

IPTR FWEditor__MUIM_PrefsEditor_ImportFH(
    Class *CLASS, Object *self,
    struct MUIP_PrefsEditor_ImportFH *message)
{
    struct FWEditor_DATA *data = INST_DATA(CLASS, self);

    PopulateListsFromRules(data);
    return TRUE;
}

/* ---- ExportFH: called by PrefsEditor to prepare data for writing ------- */

IPTR FWEditor__MUIM_PrefsEditor_ExportFH(
    Class *CLASS, Object *self,
    struct MUIP_PrefsEditor_ExportFH *message)
{
    struct FWEditor_DATA *data = INST_DATA(CLASS, self);

    SyncRulesFromLists(data);
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

/* ==================== CUSTOM METHODS ==================================== */

IPTR FWEditor__MUIM_FWEditor_AddFilter(
    Class *CLASS, Object *self, Msg message)
{
    struct FWEditor_DATA *data = INST_DATA(CLASS, self);
    struct FilterRule rule;

    GadgetsToFilterRule(data, &rule);
    DoMethod(data->filterList, MUIM_List_InsertSingle,
        (IPTR)&rule, MUIV_List_Insert_Bottom);
    SET(self, MUIA_PrefsEditor_Changed, TRUE);

    return 0;
}

IPTR FWEditor__MUIM_FWEditor_UpdateFilter(
    Class *CLASS, Object *self, Msg message)
{
    struct FWEditor_DATA *data = INST_DATA(CLASS, self);
    struct FilterRule rule;
    struct FilterRule *entry = NULL;
    IPTR active = MUIV_List_Active_Off;

    GET(data->filterList, MUIA_List_Active, &active);
    if (active == (IPTR)MUIV_List_Active_Off)
        return 0;

    DoMethod(data->filterList, MUIM_List_GetEntry, active, (IPTR)&entry);
    if (entry == NULL)
        return 0;

    GadgetsToFilterRule(data, &rule);
    CopyMem(&rule, entry, sizeof(struct FilterRule));
    DoMethod(data->filterList, MUIM_List_Redraw, active);
    SET(self, MUIA_PrefsEditor_Changed, TRUE);

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

IPTR FWEditor__MUIM_FWEditor_AddNAT(
    Class *CLASS, Object *self, Msg message)
{
    struct FWEditor_DATA *data = INST_DATA(CLASS, self);
    struct NATRule rule;

    GadgetsToNATRule(data, &rule);
    DoMethod(data->natList, MUIM_List_InsertSingle,
        (IPTR)&rule, MUIV_List_Insert_Bottom);
    SET(self, MUIA_PrefsEditor_Changed, TRUE);

    return 0;
}

IPTR FWEditor__MUIM_FWEditor_UpdateNAT(
    Class *CLASS, Object *self, Msg message)
{
    struct FWEditor_DATA *data = INST_DATA(CLASS, self);
    struct NATRule rule;
    struct NATRule *entry = NULL;
    IPTR active = MUIV_List_Active_Off;

    GET(data->natList, MUIA_List_Active, &active);
    if (active == (IPTR)MUIV_List_Active_Off)
        return 0;

    DoMethod(data->natList, MUIM_List_GetEntry, active, (IPTR)&entry);
    if (entry == NULL)
        return 0;

    GadgetsToNATRule(data, &rule);
    CopyMem(&rule, entry, sizeof(struct NATRule));
    DoMethod(data->natList, MUIM_List_Redraw, active);
    SET(self, MUIA_PrefsEditor_Changed, TRUE);

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

IPTR FWEditor__MUIM_FWEditor_Refresh(
    Class *CLASS, Object *self, Msg message)
{
    struct FWEditor_DATA *data = INST_DATA(CLASS, self);
    struct FilterRule *fentry = NULL;
    struct NATRule *nentry = NULL;
    IPTR active;

    GET(data->filterList, MUIA_List_Active, &active);
    if (active != (IPTR)MUIV_List_Active_Off)
    {
        DoMethod(data->filterList, MUIM_List_GetEntry, active, (IPTR)&fentry);
        if (fentry)
            FilterRuleToGadgets(data, fentry);
    }

    GET(data->natList, MUIA_List_Active, &active);
    if (active != (IPTR)MUIV_List_Active_Off)
    {
        DoMethod(data->natList, MUIM_List_GetEntry, active, (IPTR)&nentry);
        if (nentry)
            NATRuleToGadgets(data, nentry);
    }

    return 0;
}

/* ==================== CLASS REGISTRATION ================================ */

ZUNE_CUSTOMCLASS_14
(
    FWEditor, NULL, MUIC_PrefsEditor, NULL,
    OM_NEW,                             struct opSet *,
    MUIM_PrefsEditor_ImportFH,          struct MUIP_PrefsEditor_ImportFH *,
    MUIM_PrefsEditor_ExportFH,          struct MUIP_PrefsEditor_ExportFH *,
    MUIM_PrefsEditor_Save,              Msg,
    MUIM_PrefsEditor_Use,               Msg,
    MUIM_FWEditor_AddFilter,            Msg,
    MUIM_FWEditor_RemFilter,            Msg,
    MUIM_FWEditor_UpFilter,             Msg,
    MUIM_FWEditor_DownFilter,           Msg,
    MUIM_FWEditor_UpdateFilter,         Msg,
    MUIM_FWEditor_AddNAT,               Msg,
    MUIM_FWEditor_RemNAT,               Msg,
    MUIM_FWEditor_UpdateNAT,            Msg,
    MUIM_FWEditor_Refresh,              Msg
);
