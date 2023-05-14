/*
    Copyright (C) 2023, The AROS Development Team. All rights reserved.
*/

#define USE_INLINE_STDARG
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#define __NOLIBBASE__
#include <proto/muimaster.h>
#include <proto/dos.h>
#include <proto/log.h>
#include <proto/utility.h>

#include <resources/log.h>
#include <workbench/startup.h>
#include <libraries/mui.h>

#include <stdlib.h>
#include <string.h>

#include "logview.h"

/* defines */

struct WBStartup *_WBenchMsg;

extern struct IntuitionBase *IntuitionBase;

extern struct DosLibrary *DOSBase;
extern struct Library *UtilityBase;
struct Library *MUIMasterBase; // no extern to prevent auto-opening
extern struct Library *IconBase;

extern struct List     logList;
extern ULONG           logmask;
extern char            *logfilt;

APTR LogResBase;
struct Hook LogDisplayHook;

Object *appobj = NULL;
Object *mainwinobj = NULL;
static Object *logdetwinobj = NULL;
static Object *o_detlistobj;
static Object *o_tasktxt;
static Object *o_idtxt;
static Object *o_cattxt;
static Object *o_dttxt;
static Object *o_comptxt;
static Object *filtwinobj = NULL;

static Object *mi_about;
static Object *mi_aboutmui;
static Object *mi_help;
static Object *mi_iconify;
static Object *mi_quit;
static Object *mi_savelog;
static Object *mi_clearlog;

static Object *o_filterbut;
static Object *o_savelogbut;
static Object *o_clearlogbut;
static Object *o_critbut;
static Object *o_errbut;
static Object *o_warnbut;
static Object *o_infbut;
static Object *o_verbbut;
static Object *o_debbut;
static Object *o_taskstr;
static Object *o_idstr;
static Object *o_applybut;

Object *o_listobj;

#define LEN_SOURCESTRING            64
#define LEN_SUBJSTRING              256

static STRPTR loglvlstrings[] =
{
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL
};

static struct Hook clrloghook;
static struct Hook logshowhook;
static struct Hook applyflthook;

Object *GUITextObject(char *labelstr, char *helpstr)
{
    Object  *obj;

    obj = TextObject,
                ButtonFrame,
                MUIA_ShortHelp,     helpstr,
                MUIA_Background,    MUII_ButtonBack,
                MUIA_CycleChain,    1,
                MUIA_InputMode,     MUIV_InputMode_RelVerify,
                MUIA_Text_Contents, labelstr,
                End;

    return(obj);
}

Object *GUICheckMObject(char *labelstr, char *helpstr, Object **objPtr)
{
    Object  *cxobj, *obj;

    obj = HGroup,
                Child, (IPTR)MUI_MakeObject(MUIO_Label, labelstr, 0),
                Child, (IPTR)(cxobj = MUI_MakeObject(MUIO_Checkmark, NULL)),
                End;

    if (objPtr)
        *objPtr = cxobj;

    if (obj)
        SetAttrs(cxobj,
            MUIA_CycleChain, 1,
            MUIA_ShortHelp, (IPTR)helpstr,
            TAG_DONE);

    return(obj);
}

Object *GUITextStrObject(char *labelstr, char *helpstr, Object **objPtr, ULONG align)
{
    Object  *strobj, *obj;

    obj = HGroup,
                Child, (IPTR)MUI_MakeObject(MUIO_Label, labelstr, 0),
                Child, (IPTR)(strobj = StringObject,
                    StringFrame,
                    MUIA_Background,    MUII_TextBack,
                    MUIA_CycleChain,    1,
                    MUIA_ShortHelp,     (IPTR)helpstr,
                    MUIA_String_Accept, (IPTR)"",
                    MUIA_String_Format, align,
                    End),
                End;

    if (objPtr)
        *objPtr = strobj;

    return(obj);
}

Object *GUIStringObject(char *labelstr, char *helpstr, Object **objPtr)
{
    Object  *strobj, *obj;

    obj = HGroup,
                Child, (IPTR)MUI_MakeObject(MUIO_Label, labelstr, 0),
                Child, (IPTR)(strobj = StringObject,
                    StringFrame,
                    MUIA_CycleChain,            1,
                    MUIA_ShortHelp,             (IPTR)helpstr,
                    MUIA_String_AdvanceOnCR,    TRUE,
                    End),
                End;

    if (objPtr)
        *objPtr = strobj;

    return(obj);
}

Object *GUIImgButtonObject(char *labelstr, char *imgpath, char *helpstr)
{
    Object  *obj;

    obj = ImageButton(labelstr, imgpath);

    if (obj)
        SetAttrs(obj,
            MUIA_CycleChain,    1,
            MUIA_ShortHelp,     (IPTR)helpstr,
            TAG_DONE);

    return(obj);
}

AROS_UFH3(LONG, LogListDisplayHook,
                   AROS_UFHA(struct Hook *, hook, A0),
                   AROS_UFHA(char **, strarr, A2),
                   AROS_UFHA(struct LogListEntry *, llenode, A1))
{
    AROS_USERFUNC_INIT

    struct DateStamp    *ds;
    struct DateTime     dt;
    static char         eventdetails[LEN_SUBJSTRING];
    static char         strtime[LEN_DATSTRING];
    static char         source[LEN_SOURCESTRING];
    static char         strid[10];
    ULONG               flags;

    if(llenode)
    {
        char *evtStr = NULL, *subjEnd;
        char *compStr = NULL;
        char *subStr = NULL;
        char *tagStr = NULL;
        ULONG levtID = 0;

        struct TagItem entryAttribs[] = {
            { LOGMA_Flags,          (IPTR)&flags        },
            { LOGMA_DateStamp,      (IPTR)&ds           },
            { LOGMA_EventID,        (IPTR)&levtID       },
            { LOGMA_Component,      (IPTR)&compStr      },
            { LOGMA_SubComponent,   (IPTR)&subStr       },
            { LOGMA_LogTag,         (IPTR)&tagStr       },
            { LOGMA_Entry,          (IPTR)&evtStr       },
            { TAG_DONE, 0 }
        };

        ds = NULL;
        logGetEntryAttrs(llenode->entry, entryAttribs);
        if ((subStr) && (strlen(subStr) > 0))
            logRawDoFmt(source, LEN_SOURCESTRING, "%s:%s", compStr, subStr);
        else
            logRawDoFmt(source, LEN_SOURCESTRING, "%s", compStr);
        strarr[1] = source;
        logRawDoFmt(strid, 10, "%08x", levtID);
        strarr[3] = strid;

        if(ds)
        {
            dt.dat_Stamp.ds_Days = ds->ds_Days;
            dt.dat_Stamp.ds_Minute = ds->ds_Minute;
            dt.dat_Stamp.ds_Tick = ds->ds_Tick;
            dt.dat_Format = FORMAT_DEF;
            dt.dat_Flags = 0;
            dt.dat_StrDay = NULL;
            dt.dat_StrDate = NULL;
            dt.dat_StrTime = strtime;
            DateToStr(&dt);
            strarr[0] = strtime;
        } else {
            strarr[0] = "";
        }

        switch ((flags & LOGM_Flag_TypeMask) >> LOGMS_Flag_Type)
        {
        case LOGF_Type_Information:
            strarr[2] = "Information";
            break;
        case LOGF_Type_Verbose:
            strarr[2] = "Verbose";
            break;
        case LOGF_Type_Warn:
            strarr[2] = "Warning";
            break;
        case LOGF_Type_Error:
            strarr[2] = "Error";
            break;
        case LOGF_Type_Crit:
            strarr[2] = "Critical";
            break;
        case LOGF_Type_Debug:
            strarr[2] = "Debug";
            break;
        default:
            strarr[2] = "Ok";
            break;
        }
        subjEnd = strchr(evtStr, '\n');
        if (subjEnd)
            levtID = (IPTR)subjEnd - (IPTR)evtStr;
        else
            levtID = strlen(evtStr) + 1;
        if (levtID > LEN_SUBJSTRING)
            levtID = LEN_SUBJSTRING;
        if (tagStr)
        {
            levtID += strlen(tagStr) + 2;
            logRawDoFmt(eventdetails, levtID, "%s: %s", tagStr, evtStr);
        }
        else
        {
            logRawDoFmt(eventdetails, levtID, "%s", evtStr);
        }
        strarr[4] = eventdetails;
    }
    else
    {
        strarr[0] = "Date/Time";
        strarr[1] = "Source";
        strarr[2] = "Type";
        strarr[3] = "ID";
        strarr[4] = "Details";
    }
    return(0);
    AROS_USERFUNC_EXIT
}

AROS_UFH3
(
    void, logClearHookFunc,
    AROS_UFHA(struct Hook *, hook, A0),
    AROS_UFHA(Object *, obj, A2),
    AROS_UFHA(IPTR *, param, A1)
)
{
    AROS_USERFUNC_INIT

    LONG action;
    action = MUI_RequestA(appobj, NULL, 0, "Clear Log ...", "Save & Clear|Clear|Cancel", "Save log before clearing?", NULL);
    switch (action) {
    case 1:
        break;
    case 2:
        break;
    default:
        return;
    }

    AROS_USERFUNC_EXIT
}

AROS_UFH3
(
    void, logShowHookFunc,
    AROS_UFHA(struct Hook *, hook, A0),
    AROS_UFHA(Object *, obj, A2),
    AROS_UFHA(IPTR *, param, A1)
)
{
    AROS_USERFUNC_INIT

    struct LogListEntry *llenode = NULL;
    IPTR active = XGET(obj, MUIA_List_Active);

    DoMethod(obj, MUIM_List_GetEntry, active, (IPTR)&llenode);
    if (llenode)
    {
        struct DateStamp    *ds;
        char                *compStr = NULL;
        char                *subStr = NULL;
        char                *tagStr = NULL;
        char                *entryStr = NULL;
        char                *originStr = NULL;
        ULONG               flags = 0, levtID = 0;
        struct TagItem entryAttribs[] = {
            { LOGMA_Flags,          (IPTR)&flags        },
            { LOGMA_DateStamp,      (IPTR)&ds           },
            { LOGMA_Origin,         (IPTR)&originStr    },
            { LOGMA_EventID,        (IPTR)&levtID       },
            { LOGMA_Component,      (IPTR)&compStr      },
            { LOGMA_SubComponent,   (IPTR)&subStr       },
            { LOGMA_LogTag,         (IPTR)&tagStr       },
            { LOGMA_Entry,          (IPTR)&entryStr     },
            { TAG_DONE, 0 }
        };
        struct DateTime     dt;
        static char         strdt[(LEN_DATSTRING + 1) << 1];
        static char         strdate[LEN_DATSTRING];
        static char         strtime[LEN_DATSTRING];
        static char         source[LEN_SOURCESTRING];

        logGetEntryAttrs(llenode->entry, entryAttribs);

        SET(logdetwinobj, MUIA_Window_Title, "Event Details...");

        if(ds)
        {
            dt.dat_Stamp.ds_Days = ds->ds_Days;
            dt.dat_Stamp.ds_Minute = ds->ds_Minute;
            dt.dat_Stamp.ds_Tick = ds->ds_Tick;
            dt.dat_Format = FORMAT_DEF;
            dt.dat_Flags = DTF_SUBST;
            dt.dat_StrDay = NULL;
            dt.dat_StrDate = strdate;
            dt.dat_StrTime = strtime;
            DateToStr(&dt);
            logRawDoFmt(strdt, (LEN_DATSTRING + 1) << 1, "%s %s", strdate, strtime);
            SET(o_dttxt, MUIA_String_Contents, strdt);
        } else {
            SET(o_dttxt, MUIA_String_Contents, "");
        }
        SET(o_tasktxt, MUIA_String_Contents, originStr);
        SET(o_idtxt, MUIA_String_Integer, levtID);
        if ((subStr) && (strlen(subStr) > 0))
            logRawDoFmt(source, LEN_SOURCESTRING, "%s : %s", compStr, subStr);
        else
            logRawDoFmt(source, LEN_SOURCESTRING, "%s", compStr);
        SET(o_comptxt, MUIA_String_Contents, source);

        SET(o_detlistobj, MUIA_List_Quiet, TRUE);
        DoMethod(o_detlistobj, MUIM_List_Clear);
        switch ((flags & LOGM_Flag_TypeMask) >> LOGMS_Flag_Type)
        {
        case LOGF_Type_Information:
            SET(o_cattxt, MUIA_String_Contents, "Information");
            break;
        case LOGF_Type_Verbose:
            SET(o_cattxt, MUIA_String_Contents, "Verbose");
            break;
        case LOGF_Type_Warn:
            SET(o_cattxt, MUIA_String_Contents, "Warning");
            break;
        case LOGF_Type_Error:
            SET(o_cattxt, MUIA_String_Contents, "Error");
            break;
        case LOGF_Type_Crit:
            SET(o_cattxt, MUIA_String_Contents, "Critical");
            break;
        case LOGF_Type_Debug:
            SET(o_cattxt, MUIA_String_Contents, "Debug");
            DoMethod(o_detlistobj, MUIM_List_InsertSingle, (IPTR)tagStr, MUIV_List_Insert_Bottom);
            break;
        default:
            SET(o_cattxt, MUIA_String_Contents, "Ok");
            break;
        }

        DoMethod(o_detlistobj, MUIM_List_InsertSingle, (IPTR)entryStr, MUIV_List_Insert_Bottom);
        SET(o_detlistobj, MUIA_List_Quiet, FALSE);

        if ((BOOL)XGET(logdetwinobj, MUIA_Window_Open) == FALSE)
        {
            SET(logdetwinobj, MUIA_Window_Open, TRUE);
        }
    }
    AROS_USERFUNC_EXIT
}

AROS_UFH3
(
    void, logFilterFunc,
    AROS_UFHA(struct Hook *, hook, A0),
    AROS_UFHA(Object *, obj, A2),
    AROS_UFHA(IPTR *, param, A1)
)
{
    AROS_USERFUNC_INIT

    IPTR    gadval = 0;
    ULONG   flagmask = 0;
    BOOL    update = FALSE;

    GET(o_critbut, MUIA_Selected, &gadval);
    if (gadval == TRUE)
        flagmask |= LOGF_Flag_Type_Crit;
    GET(o_errbut, MUIA_Selected, &gadval);
    if (gadval == TRUE)
        flagmask |= LOGF_Flag_Type_Error;
    GET(o_warnbut, MUIA_Selected, &gadval);
    if (gadval == TRUE)
        flagmask |= LOGF_Flag_Type_Warn;
    GET(o_infbut, MUIA_Selected, &gadval);
    if (gadval == TRUE)
        flagmask |= LOGF_Flag_Type_Information;
    GET(o_verbbut, MUIA_Selected, &gadval);
    if (gadval == TRUE)
        flagmask |= LOGF_Flag_Type_Verbose;
    GET(o_debbut, MUIA_Selected, &gadval);
    if (gadval == TRUE)
        flagmask |= LOGF_Flag_Type_Debug;

    if (logmask != flagmask)
    {
        logmask = flagmask;
        update = TRUE;
    }
    
    GET(o_taskstr, MUIA_String_Contents, &gadval);
    if (gadval && (strlen((STRPTR)gadval) > 0))
    {
        int pbLen = ((strlen((STRPTR)gadval) + 1) << 1);
        char *patternBuf = AllocVec(pbLen, MEMF_ANY);
        if (patternBuf)
        {
            ParsePatternNoCase((STRPTR)gadval, patternBuf, pbLen);
            if (logfilt)
                FreeVec(logfilt);
            logfilt = patternBuf;
        }
        update = TRUE;
    }
    else if (logfilt) {
        FreeVec(logfilt);
        logfilt = NULL;
        update = TRUE;
    }
    GET(o_idstr, MUIA_String_Integer, &gadval);
    if (gadval != 0)
    {
    }
    if (update)
        CreateLogEntryList();

    AROS_USERFUNC_EXIT
}

int main(int argc, char *argv[])
{
    if(!(MUIMasterBase = OpenLibrary(MUIMASTER_NAME, MUIMASTER_VMIN)))
    {
        // we can't use MUI_RequestA here
        return 1;
    }

    if(!(LogResBase = OpenResource("log.resource")))
    {
        MUI_RequestA(NULL, NULL, 0, "Error", "OK", "Can't open log.resource", NULL);
        return 1;
    }

    {
        struct Task *thistask;
        IPTR stackfree;
        thistask = FindTask(NULL);
        stackfree = ((IPTR) thistask->tc_SPUpper) - ((IPTR) thistask->tc_SPLower);
        if(stackfree < 16000)
        {
            MUI_RequestA(NULL, NULL, 0, "Error", "OK", "Too few stack", NULL);
            return 1;
        }
    }

    NewList(&logList);

    LogDisplayHook.h_Data = 0;
    LogDisplayHook.h_Entry = (APTR) LogListDisplayHook;

    loglvlstrings[0] = "All";
    loglvlstrings[1] = "Critical";
    loglvlstrings[2] = "Errors";
    loglvlstrings[3] = "Warnings";
    loglvlstrings[4] = "Information";
    loglvlstrings[5] = "Verbose";
    loglvlstrings[6] = "Debug";

    appobj = ApplicationObject,
        MUIA_Application_Title      , "LogView",
        MUIA_Application_Version    , "",
        MUIA_Application_Copyright  , (IPTR) "©2023 The AROS Dev Team",
        MUIA_Application_Author     , (IPTR) "The AROS Dev Team",
        MUIA_Application_Description, "AROS System Log Viewer",
        MUIA_Application_Base       , "LOGVIEW",
        MUIA_Application_SingleTask , TRUE,
        MUIA_Application_Menustrip  , MenustripObject,
            Child, MenuObjectT("Project"),
                Child, mi_about = MenuitemObject,
                    MUIA_Menuitem_Title, "About",
                    MUIA_Menuitem_Shortcut, "?",
                    End,
                Child, mi_aboutmui = MenuitemObject,
                    MUIA_Menuitem_Title, "About MUI",
                    End,
                Child, mi_help = MenuitemObject,
                    MUIA_Menuitem_Title, "Help",
                    End,
                Child, MenuitemObject,
                    MUIA_Menuitem_Title, NM_BARLABEL,
                    End,
                Child, mi_iconify = MenuitemObject,
                    MUIA_Menuitem_Title, "",
                    MUIA_Menuitem_Shortcut, "",
                    End,
                Child, MenuitemObject,
                    MUIA_Menuitem_Title, NM_BARLABEL,
                    End,
                Child, mi_quit = MenuitemObject,
                    MUIA_Menuitem_Title, "Quit",
                    MUIA_Menuitem_Shortcut, "Q",
                    End,
                End,
            Child, MenuObjectT("Logs"),
                Child, mi_savelog = MenuitemObject,
                    MUIA_Menuitem_Title, "Save Log",
                    End,
                Child, mi_clearlog = MenuitemObject,
                    MUIA_Menuitem_Title, "Clear Log",
                    End,
                Child, MenuitemObject,
                    MUIA_Menuitem_Title, NM_BARLABEL,
                    End,
                End,
            End,

        SubWindow, mainwinobj = WindowObject,
            MUIA_Window_ID   , MAKE_ID('L','O','G','V'),
            MUIA_Window_Title, (IPTR)"Log Viewer",
            MUIA_Window_Width, MUIV_Window_Width_Visible(90),

            WindowContents, VGroup,
                Child, VGroup, GroupFrameT("AROS System Log"),
                    MUIA_VertWeight, 20,
                    Child, HGroup,
                        Child, o_filterbut = GUITextObject("Filter ...", "Select filtering options"),
                        Child, (IPTR) RectangleObject,
                            MUIA_Weight, 50,
                            End,
                        Child, HSpace(0),
                        Child, HGroup,
                            MUIA_Group_SameWidth, TRUE,
                            Child, o_savelogbut = GUITextObject("Save", "Save the log to a file"),
                            Child, o_clearlogbut = GUITextObject("Clear Log", "Clear events from the log"),
                            End,
                        End,
                    Child, o_listobj = ListviewObject,
                        MUIA_CycleChain, 1,
                        MUIA_Listview_Input, TRUE,
                        MUIA_Listview_MultiSelect, MUIV_Listview_MultiSelect_None,
                        MUIA_Listview_List, ListObject,
                            MUIA_ShortHelp, "Displays events that have been recorded in the system log",
                            ReadListFrame,
                            MUIA_List_Title, TRUE,
                            MUIA_List_Format, "BAR,BAR,BAR,BAR,",
                            MUIA_List_DisplayHook, &LogDisplayHook,
                            End,
                        End,
                    End,
                End,
            End,


        SubWindow, logdetwinobj = WindowObject,
            MUIA_Window_ID   , MAKE_ID('L','D','T','L'),

            WindowContents, VGroup,
                Child, (IPTR)ColGroup(2), GroupFrameT("General"),
                    MUIA_VertWeight, 20,
                    Child, (IPTR)GUITextStrObject("Task", "Running task when the event was logged", &o_tasktxt, MUIV_String_Format_Left),
                    Child, (IPTR)GUITextStrObject("ID", "Event ID", &o_idtxt, MUIV_String_Format_Right),
                    Child, (IPTR)GUITextStrObject("Category", "Category of event", &o_cattxt, MUIV_String_Format_Center),
                    Child, (IPTR) RectangleObject,
                        MUIA_Weight, 50,
                        End,
                    Child, (IPTR)GUITextStrObject("Date/Time", "Date/Time the event was logged", &o_dttxt, MUIV_String_Format_Right),
                    Child, (IPTR)GUITextStrObject("Component/Sub", "Component/Subcomponent the event was logged by", &o_comptxt, MUIV_String_Format_Left),
                    End,
                Child, o_detlistobj = ListviewObject,
                    MUIA_CycleChain, 1,
                    MUIA_Listview_Input, TRUE,
                    MUIA_Listview_MultiSelect, MUIV_Listview_MultiSelect_None,
                    MUIA_Listview_List, ListObject,
                        MUIA_ShortHelp, "Logged event details",
                        ReadListFrame,
                        End,
                    End,

                End,
            End,

        SubWindow, filtwinobj = WindowObject,
            MUIA_Window_ID   , MAKE_ID('L','F','L','T'),
            MUIA_Window_Title, (IPTR)"Filter options ...",

            WindowContents, VGroup,
                Child, VGroup, GroupFrameT("Event categories"),
                    Child, HGroup,
                        Child, (IPTR)GUICheckMObject("Critical", "Show critical error events", &o_critbut),
                        Child, (IPTR)GUICheckMObject("Errors", "Show error events", &o_errbut),
                        Child, (IPTR)GUICheckMObject("Warnings", "Show warnings", &o_warnbut),
                        End,
                    Child, HGroup,
                        Child, HSpace(0),
                        Child, (IPTR)GUICheckMObject("Information", "Show general information", &o_infbut),
                        Child, (IPTR)GUICheckMObject("Verbose", "Show verbose information", &o_verbbut),
                        Child, HSpace(0),
                        End,
                    Child, (IPTR) RectangleObject,
                        MUIA_Rectangle_HBar, TRUE,
                        MUIA_FixHeight,      2,
                        End,
                    Child, HGroup,
                        Child, (IPTR)GUICheckMObject("Debug", "Show debug", &o_debbut),
                        Child, HSpace(0),
                        End,
                    End,
                Child, HGroup, GroupFrameT("Advanced"),
                    Child, VGroup,
                        Child, (IPTR)GUIStringObject("Task Name", "Specify the task that created the entry (use patterns to match)", &o_taskstr),
                        Child, (IPTR)GUIStringObject("Event ID", "Only list events matching the specified ID", &o_idstr),
                        End,
                    End,
                Child, (IPTR) RectangleObject,
                    MUIA_Rectangle_HBar, TRUE,
                    MUIA_FixHeight,      2,
                    End,
                Child, HGroup,
                    Child, HSpace(0),
                    Child, (IPTR) RectangleObject,
                        MUIA_Weight, 50,
                        End,
                    Child, (IPTR) RectangleObject,
                        MUIA_Weight, 50,
                        End,
                    Child, (IPTR)(o_applybut = GUIImgButtonObject("Apply", "THEME:Images/Gadgets/Use", "Apply filtering options")),
                    End,
                End,
            End,

        End;

    if(!appobj)
    {
        return 1;
//        fail("Failed to create Application. Already running?\n");
    }

    // Menu Notifications
    DoMethod(mi_iconify, MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime,
             MUIV_Notify_Application, 3, MUIM_Set, MUIA_Application_Iconified, TRUE);
    DoMethod(mi_quit, MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime,
             MUIV_Notify_Application, 2, MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);

    // Main Window notifications
    clrloghook.h_Entry = (APTR)logClearHookFunc;
    DoMethod(o_clearlogbut, MUIM_Notify, MUIA_Pressed, FALSE,
             MUIV_Notify_Self, 2, MUIM_CallHook, &clrloghook);
    DoMethod(mi_clearlog, MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime,
             MUIV_Notify_Self, 2, MUIM_CallHook, &clrloghook);

    DoMethod(o_filterbut, MUIM_Notify, MUIA_Pressed, FALSE,
             filtwinobj, 3, MUIM_Set, MUIA_Window_Open, TRUE);

    DoMethod(mainwinobj, MUIM_Notify, MUIA_Window_CloseRequest, TRUE,
             MUIV_Notify_Application, 2, MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);
    DoMethod(appobj, MUIM_Notify, MUIA_Application_DoubleStart, MUIV_EveryTime,
             MUIV_Notify_Application, 3, MUIM_Set, MUIA_Application_Iconified, FALSE);

    logshowhook.h_Entry = (APTR)logShowHookFunc;
    DoMethod(o_listobj, MUIM_Notify, MUIA_Listview_DoubleClick, TRUE,
             MUIV_Notify_Self, 2, MUIM_CallHook, &logshowhook);

    // Detail View Window notifications
    DoMethod(logdetwinobj, MUIM_Notify, MUIA_Window_CloseRequest, TRUE,
             MUIV_Notify_Self, 3, MUIM_Set, MUIA_Window_Open, FALSE);

    // Filter Window notifications
    SET(o_critbut, MUIA_Selected, TRUE);
    SET(o_errbut, MUIA_Selected, TRUE);
    SET(o_warnbut, MUIA_Selected, TRUE);
    SET(o_infbut, MUIA_Selected, TRUE);
    SET(o_verbbut, MUIA_Selected, TRUE);
    SET(o_idstr, MUIA_String_Accept, "0123456879abcdef");
    DoMethod(filtwinobj, MUIM_Notify, MUIA_Window_CloseRequest, TRUE,
             MUIV_Notify_Self, 3, MUIM_Set, MUIA_Window_Open, FALSE);

    applyflthook.h_Entry = (APTR)logFilterFunc;
    DoMethod(o_applybut, MUIM_Notify, MUIA_Pressed, FALSE,
             MUIV_Notify_Self, 2, MUIM_CallHook, &applyflthook);
    // App Notifications

    // Main Loop
    {
        ULONG sigs = 0;
        ULONG sigmask;
        LONG isopen = 0;
        LONG iconify = 0;

        DoMethod(appobj, MUIM_Application_Load, MUIV_Application_Load_ENVARC);

        CreateLogEntryList();

        GET(appobj, MUIA_Application_Iconified, &iconify);
        SET(mainwinobj, MUIA_Window_Open, TRUE);
        GET(mainwinobj, MUIA_Window_Open, &isopen);
        if(!(isopen || iconify))
        {
            return 1;
        }

        sigmask = 0;
        while(DoMethod(appobj, MUIM_Application_NewInput, &sigs) != MUIV_Application_ReturnID_Quit)
        {
            if(sigs)
            {
                sigs = Wait(sigs | sigmask | SIGBREAKF_CTRL_C);
                if(sigs & SIGBREAKF_CTRL_C)
                    break;
            }
        }
        SET(mainwinobj, MUIA_Window_Open, FALSE);
    }
    DoMethod(appobj, MUIM_Application_Save, MUIV_Application_Save_ENVARC);
    MUI_DisposeObject(appobj);

    return(0);
}
