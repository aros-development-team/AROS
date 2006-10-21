/*
    Copyright © 2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#define MUIMASTER_YES_INLINE_STDARG

#define DEBUG 0
#include <aros/debug.h>

#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>
#include <proto/workbench.h>
#include <proto/dos.h>
#include <proto/alib.h>
#include <proto/icon.h>

#include <dos/dos.h>
#include <dos/datetime.h>
#include <exec/memory.h>
#include <libraries/asl.h>
#include <libraries/commodities.h>
#include <libraries/mui.h>
#include <workbench/startup.h>
#include <workbench/workbench.h>
#include <workbench/icon.h>
#include <devices/inputevent.h>

#include <MUI/TextEditor_mcc.h>
#include <zune/iconimage.h>

#include "locale.h"

#include <stdio.h>

#include <string.h>

#define RETURNID_SLEEP      1
#define RETURNID_WAKE       2
#define RETURNID_ABOUT      3
#define RETURNID_NEWKEY     4
#define RETURNID_DELKEY     5
#define RETURNID_LVACK      6
#define RETURNID_STRINGACK  7
#define RETURNID_SAVE       8
#define RETURNID_TOOLACK    9
#define RETURNID_STACKACK   10
#define RETURNID_COMMENTACK 11

//#define VERSION "$VER: Info 0.1 ("ADATE") © AROS Dev Team"

#define  MAX_PATH_LEN  1024
#define  MAX_TOOLTYPE_LINE 256
#define BUFFERSIZE 1024

#define USE_TEXTEDITOR 1

static Object *window, *commentspace, *filename_string, *stackspace, *savebutton;
#if USE_TEXTEDITOR
static Object *editor, *slider;
#else
static Object *list, *editor, *liststr;
#endif

BOOL file_altered = FALSE;
BOOL icon_altered = FALSE;

UBYTE **BuildToolTypes(UBYTE **src_ttypes)
{
#if USE_TEXTEDITOR
    APTR     pool = CreatePool(MEMF_CLEAR, 200, 200);
    STRPTR   contents, sp, *dst_ttypes;
    ULONG    lines = 1;

    if (!pool) return NULL;
    contents = (STRPTR)DoMethod(editor, MUIM_TextEditor_ExportText);

    if (!contents || !contents[0])
    {
    	DeletePool(pool);
	return NULL;
    }
    
    sp = contents;
    
    while((sp = strchr(sp, '\n')))
    {
    	sp++;
	lines++;
    }
    
    dst_ttypes = AllocPooled(pool, (lines + 3) * sizeof(STRPTR));
    if (!dst_ttypes)
    {
    	FreeVec(contents);
	DeletePool(pool);
	return NULL;
    }
    
    *dst_ttypes++ = (STRPTR)pool;
    *dst_ttypes++ = (STRPTR)contents;
    
    for(sp = contents, lines = 0; sp; lines++)
    {
	    dst_ttypes[lines] = sp;
    	sp = strchr(sp, '\n');
	if (sp)
	{
	    *sp++ = '\0';
	}
	else
	{
	    dst_ttypes[lines] = 0;
	}
    }
    dst_ttypes[lines] = 0;
     
    return dst_ttypes;   
#else
    APTR     pool = CreatePool(MEMF_CLEAR, 200, 200);
    Object  *listobj = list;
    WORD     list_index = 0, num_ttypes = 0, alloc_ttypes = 10;
    UBYTE **dst_ttypes;
    
    if (!pool) return NULL;
    
    dst_ttypes = AllocPooled(pool, (alloc_ttypes + 2) * sizeof(UBYTE *));
    if (!dst_ttypes)
    {
        DeletePool(pool);
        return NULL;
    }

    get(listobj, MUIA_List_Entries, &list_index);
    while(num_ttypes<list_index)
    {
        STRPTR text = NULL;
        DoMethod(listobj, MUIM_List_GetEntry, num_ttypes, (IPTR)&text);
        if (num_ttypes >= alloc_ttypes)
        {
            UBYTE **new_dst_ttypes = AllocPooled(pool, (alloc_ttypes + 10 + 2) * sizeof(UBYTE *));
            if (!new_dst_ttypes)
            {
                DeletePool(pool);
                return NULL;
            }
            CopyMem(dst_ttypes + 1, new_dst_ttypes + 1, alloc_ttypes * sizeof(UBYTE *));
            dst_ttypes = new_dst_ttypes;
            alloc_ttypes += 10;
        }
        dst_ttypes[num_ttypes + 1] = AllocPooled(pool, strlen(text) + 1);
        if (!dst_ttypes[num_ttypes + 1])
        {
            DeletePool(pool);
            return NULL;
        }
        CopyMem(text, dst_ttypes[num_ttypes + 1], strlen(text) + 1);
        D(bug("[WBInfo] tooltype #%ld %s\n", num_ttypes + 1, text));
        num_ttypes++;
    }
    dst_ttypes[0] = (APTR)pool;
    return dst_ttypes + 1;
#endif    
}

void FreeToolTypes(UBYTE **ttypes)
{
#if USE_TEXTEDITOR
    if (ttypes)
    {
        DeletePool((APTR)ttypes[-2]);
	FreeVec((APTR)ttypes[-1]);
    }
#else
    if (ttypes)
    {
        DeletePool((APTR)ttypes[-1]);
    }
#endif
}

void SaveIcon(struct DiskObject *icon, STRPTR name, BPTR cd)
{
    STRPTR tool = NULL, stack = NULL;
    BPTR restored_cd;
    LONG ls;
    UBYTE **ttypes = NULL, **old_ttypes = NULL;

    restored_cd = CurrentDir(cd);

    switch(icon->do_Type)
    {
        case 4:
            get(filename_string, MUIA_String_Contents, &tool);
            icon->do_DefaultTool = tool;
            break;
        case 3:
            get(stackspace, MUIA_String_Contents, &stack);
            stcd_l(stack, &ls);
            icon->do_StackSize = ls;
            break;
    }

    old_ttypes = (UBYTE **)icon->do_ToolTypes;
    if ((ttypes = BuildToolTypes(old_ttypes)))
    {
        icon->do_ToolTypes = ttypes;
    }
    PutIconTags(name, icon, TAG_DONE);
    if (ttypes)
    {
        icon->do_ToolTypes = old_ttypes;
        FreeToolTypes(ttypes);
    }
    CurrentDir(restored_cd);
    icon_altered = FALSE;
}
void SaveFile(struct AnchorPath * ap, BPTR cd)
{
    STRPTR text=NULL;
    BPTR restored_cd;
    restored_cd = CurrentDir(cd);
    get(commentspace, MUIA_String_Contents, &text);
    if (text)
        SetComment(ap->ap_Buf, text);
    CurrentDir(restored_cd);
    file_altered = FALSE;
}

#if !USE_TEXTEDITOR
void ListToString(void)
{
    STRPTR text = NULL;
    DoMethod(list, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, (IPTR) &text);
    if (text)
    {
        set(liststr, MUIA_String_Contents, text);
        nnset(liststr, MUIA_Disabled, FALSE);
        set(window, MUIA_Window_ActiveObject, (IPTR)liststr);
    }
}

void StringToKey(void)
{
    STRPTR  text = NULL;
    STRPTR  input = NULL;

    icon_altered = TRUE;

    DoMethod(list, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, (IPTR) &text);
    if (text)
    {
        get(liststr, MUIA_String_Contents, &input);
            DoMethod(list, MUIM_List_InsertSingle, input, MUIV_List_Insert_Active);
            set(list, MUIA_List_Active, MUIV_List_Active_Down);
            DoMethod(list, MUIM_List_Remove, MUIV_List_Remove_Active);
            nnset(liststr, MUIA_String_Contents, (IPTR) "");
            nnset(liststr, MUIA_Disabled, TRUE);
            set(window, MUIA_Window_ActiveObject, (IPTR)savebutton);
    }
}

void NewKey(void)
{
    nnset(liststr, MUIA_String_Contents, "");
    nnset(liststr, MUIA_Disabled, FALSE);
    DoMethod(list, MUIM_List_InsertSingle, "", MUIV_List_Insert_Bottom);
    nnset(list, MUIA_List_Active, MUIV_List_Active_Bottom);
    set(window, MUIA_Window_ActiveObject, (IPTR) liststr);
}

void DelKey(void)
{
    STRPTR  text = NULL;
    DoMethod(list, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, (IPTR)&text);

    if (text) 
    {
        DoMethod(list, MUIM_List_Remove, MUIV_List_Remove_Active);
        DoMethod(list, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, (IPTR)&text);
        if (!text) 
        {
            nnset(liststr, MUIA_String_Contents, (IPTR) "");
            nnset(liststr, MUIA_Disabled, TRUE);
            ListToString();
        } else { 
            ListToString();
        }
    }
}
#endif

int main(int argc, char **argv)
{
    Object *application, *group, *toolspace=NULL, *toollabel=NULL;
    Object *stacklabel=NULL, *drawerspace=NULL;
    Object *sizespace=NULL, *typespace=NULL, *quit=NULL, *aboutmenu=NULL;
    Object *datespace=NULL, *versionspace=NULL;
    Object *cancelbutton=NULL;
    Object *newkey=NULL, *delkey=NULL;
    Object *readobject=NULL, *writeobject=NULL, *executeobject=NULL, *deleteobject=NULL;
    Object *scriptobject=NULL, *pureobject=NULL, *archiveobject=NULL;
    struct WBStartup *startup;
    struct DiskObject *icon=NULL;
    struct AnchorPath *ap=NULL;
    struct DateStamp *ds=NULL;
    struct DateTime dt;
    IPTR dte;
    STRPTR name=NULL, type=NULL;
    BPTR cd, lock, icon_cd;
    LONG  returnid = 0;
    ULONG protection;
    char stack[16];
    char deftool[MAX_PATH_LEN] ;
    char comment[MAXCOMMENTLENGTH];
    char size[10];
    char date[LEN_DATSTRING];
    char time[LEN_DATSTRING];
    char  dow[LEN_DATSTRING];
    char datetime[2*LEN_DATSTRING];
    UBYTE flags[8], lname[MAXFILENAMELENGTH];

    char *pages[] = {_(MSG_INFORMATION),_(MSG_PROTECTION),_(MSG_TOOLTYPES),NULL};
    char * typeNames[] =
    {
         NULL,      /* not used */
        _(MSG_DISK),    /* 1 */
        _(MSG_DRAWER),  /* 2 */
        _(MSG_TOOL),    /* 3 */
        _(MSG_PROJECT), /* 4 */
        _(MSG_GARBAGE), /* 5 */
        _(MSG_DEVICE),  /* 6 */
        _(MSG_KICK),    /* 7 */
        _(MSG_APPICON)  /* 8 */
    };

    if (argc != 0)
    {
        /* start from wanderer only */
        PrintFault(ERROR_FILE_NOT_OBJECT, argv[0]);
        return RETURN_FAIL;
    }

    startup = (struct WBStartup *) argv;

    if (startup->sm_NumArgs < 2)
    {
        /* need atleast 1 arg */
        PrintFault(ERROR_REQUIRED_ARG_MISSING, argv[0]);
        D(bug("[WBInfo] required arg missing\n"));
        return RETURN_FAIL;
    }

    lock = startup->sm_ArgList[1].wa_Lock;
    NameFromLock(lock, lname, MAXFILENAMELENGTH);
    D(bug("[WBInfo] name from lock: %s\n",lname));
    name = startup->sm_ArgList[1].wa_Name;
    cd = CurrentDir(lock);
    if (name == NULL)
    {
        /* directory not found*/
        PrintFault(ERROR_DIR_NOT_FOUND, argv[0]);
        D(bug("[WBInfo] dir not found\n"));
        return RETURN_FAIL;
    };

    ap = AllocVec(sizeof(struct AnchorPath) + MAX_PATH_LEN, MEMF_CLEAR);
    if (ap == NULL)
    {
        PrintFault(ERROR_NO_FREE_STORE, argv[0]);
        D(bug("[WBInfo] no free store\n"));
        return RETURN_FAIL;
    }

    ap->ap_Strlen = MAX_PATH_LEN;

    if (0 != MatchFirst(name, ap))
    {
        D(bug("[WBInfo] pass to diskinfo\n"));
        OpenWorkbenchObject(
            "WANDERER:Tools/DiskInfo",
            WBOPENA_ArgLock, (IPTR) startup->sm_ArgList[1].wa_Lock,
            WBOPENA_ArgName, (IPTR) startup->sm_ArgList[1].wa_Name,
            TAG_DONE);
        FreeVec(ap);
        return RETURN_OK;
    };

    ap->ap_BreakBits = SIGBREAKF_CTRL_C;

    if (!(ap->ap_Flags & APF_DIDDIR))
    {
        D(bug("[WBInfo] scan file\n"));
        /* fill comment */
        sprintf(comment,"%s",ap->ap_Info.fib_Comment);

        /* fill date and time */
        ds = &ap->ap_Info.fib_Date;
        DateStamp((struct DateStamp *)&dt);
        CopyMem(ds, &dt.dat_Stamp, sizeof(struct DateStamp));
        dt.dat_Format = FORMAT_DOS;
        dt.dat_Flags = 0;
        dt.dat_StrDay = dow;
        dt.dat_StrDate = date;
        dt.dat_StrTime = time;
        dte = DateToStr(&dt);
        sprintf(datetime, "%s %s", time, date);

        /* fill size */
        sprintf(size, "%ld", ap->ap_Info.fib_Size);

        /* fill protection */
        protection = ap->ap_Info.fib_Protection;

            /* Convert the protection bits to a boolean */
        flags[0] = protection & FIBF_SCRIPT  ? 1 : 0; /* s */
        flags[1] = protection & FIBF_PURE    ? 1 : 0; /* p */
        flags[2] = protection & FIBF_ARCHIVE ? 1 : 0; /* a */

            /* The following flags are high-active! */
        flags[3] = protection & FIBF_READ    ? 0 : 1; /* r */
        flags[4] = protection & FIBF_WRITE   ? 0 : 1; /* w */
        flags[5] = protection & FIBF_EXECUTE ? 0 : 1; /* e */
        flags[6] = protection & FIBF_DELETE  ? 0 : 1; /* d */
        flags[7] = 0x00;                              /* h */
    };

    /* read icon */

    icon = GetIconTags(name,
            ICONGETA_FailIfUnavailable, FALSE,
            ICONGETA_IsDefaultIcon, FALSE,
            TAG_DONE);

    if (icon != NULL)
    {
        D(bug("[WBInfo] file has icon\n"));
        type = (char *) typeNames[icon->do_Type];
        D(bug("[WBInfo] icon type is: %s\n", type));
        sprintf(stack, "%ld", icon->do_StackSize);
        if (icon->do_DefaultTool) sprintf(deftool, "%s", icon->do_DefaultTool);
    } else {
        FreeVec(ap);
        PrintFault(ERROR_OBJECT_WRONG_TYPE, argv[0]);
        return RETURN_FAIL;
    }

    application = ApplicationObject,
        MUIA_Application_Title,  __(MSG_TITLE),
        MUIA_Application_Version, (IPTR) "$VER: Info 0.1 ("ADATE") © AROS Dev Team",
        MUIA_Application_Description,  __(MSG_DESCRIPTION),
        MUIA_Application_Base, (IPTR) "INFO",
        MUIA_Application_Menustrip, (IPTR) MenuitemObject,
            MUIA_Family_Child, (IPTR) MenuitemObject,
                MUIA_Menuitem_Title,  __(MSG_PROJECT),
                MUIA_Family_Child, (IPTR) MenuitemObject, MUIA_Menuitem_Title,
                    __(MSG_INFORMATION), MUIA_Menuitem_Shortcut, (IPTR) "I", End,
                MUIA_Family_Child, (IPTR) MenuitemObject, MUIA_Menuitem_Title,
                    __(MSG_PROTECTION),MUIA_Menuitem_Shortcut, (IPTR) "P", End,
                MUIA_Family_Child, (IPTR) MenuitemObject, MUIA_Menuitem_Title,
                    __(MSG_TOOLTYPES),MUIA_Menuitem_Shortcut, (IPTR) "T", End,
                MUIA_Family_Child, (IPTR) MenuitemObject, MUIA_Menuitem_Title, ~0, End,
                MUIA_Family_Child, (IPTR) (aboutmenu = MenuitemObject, MUIA_Menuitem_Title,
                    __(MSG_ABOUT), (IPTR) MUIA_Menuitem_Shortcut, (IPTR) "?", End),
                MUIA_Family_Child, (IPTR) MenuitemObject, MUIA_Menuitem_Title, ~0, End,
                MUIA_Family_Child, (IPTR) (quit = MenuitemObject, MUIA_Menuitem_Title,
                    __(MSG_QUIT), MUIA_Menuitem_Shortcut, (IPTR) "Q", End),
                End,
            End,
        SubWindow, (IPTR) (window = WindowObject,
            MUIA_Window_Title, (IPTR) __(MSG_ICON),
            MUIA_Window_Activate, TRUE,
            WindowContents, (IPTR) VGroup,
                Child, (IPTR) HGroup,
                    Child, (IPTR) VGroup,
                        Child, (IPTR) IconImageObject,
                            MUIA_InputMode, MUIV_InputMode_Toggle,
                            MUIA_IconImage_File, (IPTR) name,
                        End,
                    End,
                    Child, (IPTR) VGroup,
                        Child, (IPTR) TextObject,
                            TextFrame,
                            MUIA_Background, MUII_TextBack,
                            MUIA_Text_PreParse, (IPTR) "\33l",
                            MUIA_Text_Contents, (IPTR) FilePart(name),
                        End,
                    End,
                End,
                Child, (IPTR) RegisterGroup(pages),
                    MUIA_CycleChain, 1,
                    Child, (IPTR) HGroup, /* hgroup information pannel */
                        Child, (IPTR) VGroup,
                            Child, (IPTR) HGroup,
                                Child, (IPTR) ColGroup(2),
                                    Child, (IPTR) Label2(__(MSG_SIZE)),
                                    Child, (IPTR) (sizespace = TextObject,
                                        TextFrame,
                                        MUIA_Background, MUII_TextBack,
                                        MUIA_Text_PreParse, (IPTR) "\33r",
                                        MUIA_Text_Contents, (IPTR) size,
                                    End),
                                    Child, (IPTR) Label2(__(MSG_DATE)),
                                    Child, (IPTR) (datespace = TextObject,
                                        TextFrame,
                                        MUIA_Background, MUII_TextBack,
                                        MUIA_Text_PreParse, (IPTR) "\33r",
                                        MUIA_Text_Contents, (IPTR) datetime,
                                    End),
                                    Child, (IPTR) Label2(__(MSG_TYPE)),
                                    Child, (IPTR) (typespace = TextObject,
                                        TextFrame,
                                        MUIA_Background, MUII_TextBack,
                                        MUIA_Text_PreParse, (IPTR) "\33r",
                                        MUIA_Text_Contents, (IPTR) type,
                                    End),
                                    Child, (IPTR) Label2(__(MSG_VERSION)),
                                    Child, (IPTR) (versionspace = TextObject,
                                        TextFrame,
                                        MUIA_Background, MUII_TextBack,
                                        MUIA_Text_PreParse, (IPTR) "\33r",
                                        MUIA_Text_Contents, (IPTR) NULL,
                                    End),
                                End,
                            End,
                            Child, (IPTR) HVSpace,
                            Child, (IPTR) (group = HGroup,
                            End),
                            Child, (IPTR) HGroup,
                                Child, (IPTR) Label2(__(MSG_COMMENT)),
                                Child, (IPTR) (commentspace = StringObject,
                                    StringFrame,
                                    MUIA_CycleChain, 1,
                                End),
                            End,
                        End,
                    End, /* end hgroup information pannel */
                    Child, (IPTR) HGroup, /* hgroup protections pannel */
                        Child, (IPTR) VGroup,
                            Child, (IPTR) ColGroup(2),
                                Child, (IPTR) HVSpace,
                                Child, (IPTR) ColGroup(2),
                                    Child, (IPTR) Label2(_(MSG_READ)),
                                    Child, (IPTR) (readobject = MUI_MakeObject(MUIO_Checkmark,NULL)),
                                    Child, (IPTR) Label2(_(MSG_WRITE)),
                                    Child, (IPTR) (writeobject = MUI_MakeObject(MUIO_Checkmark,NULL)),
                                    Child, (IPTR) Label2(_(MSG_EXECUTE)),
                                    Child, (IPTR) (executeobject = MUI_MakeObject(MUIO_Checkmark,NULL)),
                                    Child, (IPTR) Label2(_(MSG_DELETE)),
                                    Child, (IPTR) (deleteobject = MUI_MakeObject(MUIO_Checkmark,NULL)),
                                    Child, (IPTR) Label2(_(MSG_SCRIPT)),
                                    Child, (IPTR) (scriptobject = MUI_MakeObject(MUIO_Checkmark,NULL)),
                                    Child, (IPTR) Label2(_(MSG_PURE)),
                                    Child, (IPTR) (pureobject = MUI_MakeObject(MUIO_Checkmark,NULL)),
                                    Child, (IPTR) Label2(_(MSG_ARCHIVED)),
                                    Child, (IPTR) (archiveobject = MUI_MakeObject(MUIO_Checkmark,NULL)),
                                End,
                            End,
                        End,
                    End, /* end hgroup protections pannel */
                    Child, (IPTR) HGroup, /* hgroup tooltypes pannel */
                        Child, (IPTR) VGroup,
                            Child, (IPTR) HGroup,
                                Child, (IPTR) VGroup,
                                GroupSpacing(0),
				#if !USE_TEXTEDITOR
                                    Child, (IPTR) ListviewObject,
                                        MUIA_Listview_List, (IPTR) (list = ListObject,
                                        InputListFrame,
                                            MUIA_List_ConstructHook, MUIV_List_ConstructHook_String,
                                            MUIA_List_DestructHook, MUIV_List_DestructHook_String,
                                            MUIA_List_AutoVisible, TRUE,
                                        End),
                                    End,
                                    Child, (IPTR) (liststr = StringObject,
                                        MUIA_Disabled, TRUE,
                                        StringFrame,
                                    End),
				#else
				    Child, (IPTR) HGroup,
				        GroupSpacing(0),
				    	Child, (IPTR) (editor = MUI_NewObject(MUIC_TextEditor,
				    	TAG_DONE)),
					Child, (IPTR) (slider = ScrollbarObject,
					End),
				    End,
				#endif
                                End,
                            End,
			#if !USE_TEXTEDITOR
                            Child, (IPTR) HGroup,
                                MUIA_Group_SameSize, TRUE,
                                Child, (IPTR) (newkey = SimpleButton(__(MSG_NEW_KEY))),
                                Child, (IPTR) (delkey = SimpleButton(__(MSG_DELETE_KEY))),
                            End,
			#endif
                        End,
                    End, /* end hgroup tooltypes pannel */
                End,
                Child, (IPTR) HGroup,
                    MUIA_Group_SameSize, TRUE,
                    Child, (IPTR) (savebutton = 
                     ImageButton(_(MSG_SAVE),"THEME:Images/Gadgets/Prefs/Save")),
                    Child, (IPTR) (cancelbutton = 
                     ImageButton(_(MSG_CANCEL),"THEME:Images/Gadgets/Prefs/Cancel")),
                End,
            End,
        End),
    End;

    icon_cd = CurrentDir(cd);

    if (application != NULL)
    {
        ULONG signals = 0;

    #if !USE_TEXTEDITOR
        set(liststr, MUIA_String_AttachedList, (IPTR)list);
    #endif
    
        DoMethod(quit, MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime,
            (IPTR) application, 2, MUIM_Application_ReturnID,
            MUIV_Application_ReturnID_Quit);
        DoMethod(cancelbutton, MUIM_Notify, MUIA_Pressed, FALSE,
            (IPTR) application, 2,
            MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);
        DoMethod(savebutton, MUIM_Notify, MUIA_Pressed, FALSE,
            (IPTR) application, 2,
            MUIM_Application_ReturnID, RETURNID_SAVE);
        DoMethod(aboutmenu, MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime,
            (IPTR) application, 2,
            MUIM_Application_ReturnID, RETURNID_ABOUT);
        DoMethod(application, MUIM_Notify, MUIA_Application_Sleep, FALSE,
            (IPTR) application, 2,
            MUIM_Application_ReturnID, RETURNID_WAKE);
        DoMethod(application, MUIM_Notify, MUIA_Application_Sleep, TRUE,
            (IPTR) application, 2,
            MUIM_Application_ReturnID, RETURNID_SLEEP);
        DoMethod(window, MUIM_Notify, MUIA_Window_CloseRequest, TRUE,
            (IPTR) application, 2, MUIM_Application_ReturnID,
            MUIV_Application_ReturnID_Quit );
    #if !USE_TEXTEDITOR
        DoMethod(newkey, MUIM_Notify, MUIA_Pressed, FALSE,
            (IPTR) application, 2, MUIM_Application_ReturnID, RETURNID_NEWKEY);
        DoMethod(delkey, MUIM_Notify, MUIA_Pressed, FALSE,
            (IPTR) application, 2, MUIM_Application_ReturnID, RETURNID_DELKEY);
        DoMethod(list, MUIM_Notify, MUIA_List_Active, MUIV_EveryTime,
            (IPTR) application, 2, MUIM_Application_ReturnID, RETURNID_LVACK);
        DoMethod(liststr, MUIM_Notify, MUIA_String_Acknowledge, MUIV_EveryTime,
            (IPTR) application, 2, MUIM_Application_ReturnID, RETURNID_STRINGACK);
    #endif
        DoMethod(commentspace, MUIM_Notify, MUIA_String_Acknowledge, MUIV_EveryTime,
            (IPTR) application, 2, MUIM_Application_ReturnID, RETURNID_COMMENTACK);

    #if USE_TEXTEDITOR
    	set(editor, MUIA_TextEditor_Slider, slider);

        if (icon->do_ToolTypes)
        {
            char *tt = NULL, *contents;
            int   i  = 0;
	    int   first = 1;
	    ULONG len = 0;
	    
            while ((tt = icon->do_ToolTypes[i]) != NULL)
            {
	    	len = len + strlen(icon->do_ToolTypes[i]) + 1;
		i++;
	    }
	    
	    contents = AllocVec(len + 1, MEMF_ANY);
	    if (contents)
	    {
	    	contents[0] = 0;
		i = 0;

        	while ((tt = icon->do_ToolTypes[i]) != NULL)
        	{
		    strcat(contents, icon->do_ToolTypes[i]);
		    strcat(contents, "\n");
		    i++;
		}

    	    	set(editor, MUIA_TextEditor_Contents, contents);
		FreeVec(contents);
            }
        }
    #else
    	
        if (icon->do_ToolTypes)
        {
            char *tt = NULL;
            int   i  = 0;
            while ((tt = icon->do_ToolTypes[i]) != NULL)
            {
                DoMethod(list, MUIM_List_InsertSingle, tt, MUIV_List_Insert_Bottom);
                i++;
            }
        }
    #endif
    
        switch(icon->do_Type)
        {
            case 4:
                toolspace = MUI_NewObject(MUIC_Popasl, ASLFR_DoSaveMode, TRUE,
                    MUIA_Popstring_String, 
                filename_string = MUI_MakeObject(MUIO_String, NULL, MAX_PATH_LEN),
                    MUIA_Popstring_Button, PopButton(MUII_PopFile), TAG_DONE);
                toollabel = MUI_MakeObject(MUIO_Label, _(MSG_DEFTOOL), NULL);
                if ((toolspace != NULL)&&(toollabel != NULL)&&(filename_string != NULL))
                {
                    DoMethod(group, MUIM_Group_InitChange);
                    DoMethod(group, OM_ADDMEMBER, toollabel);
                    DoMethod(group, OM_ADDMEMBER, toolspace);
                    DoMethod(group, MUIM_Group_ExitChange);

                    set(filename_string, MUIA_String_Contents, deftool);
                    set(toolspace, MUIA_CycleChain, 1);
                    DoMethod(filename_string, MUIM_Notify,
                        MUIA_String_Acknowledge, MUIV_EveryTime,
                        (IPTR) application, 2, MUIM_Application_ReturnID, RETURNID_TOOLACK);
                }
                break;
            case 3:
                stackspace = MUI_MakeObject(MUIO_String, NULL, 16);
                stacklabel = MUI_MakeObject(MUIO_Label, _(MSG_STACK), NULL);
                if ((stackspace != NULL)&&(stacklabel !=NULL))
                {
                    DoMethod(group, MUIM_Group_InitChange);
                    DoMethod(group, OM_ADDMEMBER, stacklabel);
                    DoMethod(group, OM_ADDMEMBER, stackspace);
                    DoMethod(group, MUIM_Group_ExitChange);
                    SetAttrs(stackspace, MUIA_String_Contents, stack,
                        MUIA_CycleChain, 1,
                        MUIA_String_Accept, (IPTR)"0123456789",TAG_DONE);
                    DoMethod(stackspace, MUIM_Notify,
                        MUIA_String_Acknowledge, MUIV_EveryTime,
                        (IPTR) application, 2, MUIM_Application_ReturnID, RETURNID_STACKACK);
                }
                break;
            case 2:
                drawerspace = MUI_NewObject(MUIC_Rectangle, TAG_DONE);
                if ((drawerspace != NULL))
                {
                    DoMethod(group, MUIM_Group_InitChange);
                    DoMethod(group, OM_ADDMEMBER, drawerspace);
                    DoMethod(group, MUIM_Group_ExitChange);
                }
                break;
        }

        if (comment != NULL)
            set(commentspace, MUIA_String_Contents, comment);

        set(readobject, MUIA_Selected, flags[3]);
        set(writeobject, MUIA_Selected, flags[4]);
        set(executeobject, MUIA_Selected, flags[5]);
        set(deleteobject, MUIA_Selected, flags[6]);

        set(scriptobject, MUIA_Selected, flags[0]);
        set(pureobject, MUIA_Selected, flags[1]);
        set(archiveobject, MUIA_Selected, flags[2]);

        SetAttrs(window, MUIA_Window_Open, TRUE, TAG_DONE);

        while (returnid != MUIV_Application_ReturnID_Quit)
        { 
#ifdef DEBUG
            if (returnid)
            {
                D(bug("[WBInfo] broker command received: %ld\n", returnid));
#endif
                switch(returnid)
                {
                    case RETURNID_SLEEP:
                        set(window, MUIA_Window_Open, FALSE);
                        set(window, MUIA_Window_Activate, TRUE);
                        break;
                    case RETURNID_WAKE:
                        set(window, MUIA_Window_Open, TRUE);
                        break;
                    case RETURNID_ABOUT:
                        if (MUI_RequestA(application,NULL,0,
                            _(MSG_ABOUT), _(MSG_OK), _(MSG_COPYRIGHT), NULL))
                        break;
		#if !USE_TEXTEDITOR
                    case RETURNID_NEWKEY:
                        NewKey();
                        break;
                    case RETURNID_DELKEY:
                        DelKey();
                        icon_altered = TRUE;
                        break;
                    case RETURNID_LVACK:
                        ListToString();
                        break;  
                    case RETURNID_STRINGACK:
                        StringToKey();
                        icon_altered = TRUE;
                        break;
		#endif
                    case RETURNID_SAVE:
		    #if !USE_TEXTEDITOR
                        if (icon_altered)
		    #endif
                            SaveIcon(icon, name, lock);
                        if (file_altered)
                            SaveFile(ap, lock);
                        DoMethod(application, MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);
                        break;
                    case RETURNID_STACKACK:
                        set(window, MUIA_Window_ActiveObject, (IPTR)savebutton);
                        icon_altered = TRUE;
                        break;
                    case RETURNID_TOOLACK:
                        set(window, MUIA_Window_ActiveObject, (IPTR)savebutton);
                        icon_altered = TRUE;
                        break;
                    case RETURNID_COMMENTACK:
                        set(window, MUIA_Window_ActiveObject, (IPTR)savebutton);
                        file_altered = TRUE;
                        break;
                }
#ifdef DEBUG
            }
#endif
            if (signals)
            {
                signals = Wait(signals | SIGBREAKF_CTRL_C);
                if(signals & SIGBREAKF_CTRL_C) break;
            }

        returnid = ((LONG) DoMethod(application, MUIM_Application_NewInput, (IPTR) &signals));
        }
        SetAttrs(window, MUIA_Window_Open, FALSE, TAG_DONE);
        MUI_DisposeObject(application);
    } else {
        PrintFault(ERROR_INVALID_RESIDENT_LIBRARY, argv[0]);
        D(bug("[WBInfo: Couldn't create app\n"));
    }
    FreeDiskObject(icon);
    FreeVec(ap);
    return RETURN_OK;
}
