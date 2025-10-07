/*
    Copyright (C) 2025, The AROS Development Team.
    All rights reserved.

    PanelGroup and Panel classes demonstration
*/

#include "clib/alib_protos.h"
#include "libraries/mui.h"
#include "muizunesupport.h"
#include "utility/hooks.h"

/* Objects */
Object *app;
Object *WD_Main;
Object *PG_Sidebar, *PG_ThreadPanel;
Object *P_Channels, *P_DirectMessages, *P_Apps, *P_ThreadDetails;
Object *BT_Workspace, *BT_Search, *BT_Reorder, *BT_Profile;
Object *LV_Messages, *ST_MessageInput;
Object *GR_MainContent, *GR_TopBar, *GR_MessageArea;

/* Channel and DM buttons for the sidebar */
Object *BT_General, *BT_Random, *BT_Development, *BT_Design;
Object *BT_JohnDoe, *BT_JaneSmith, *BT_TeamLead;
Object *BT_Slack, *BT_GitHub, *BT_Jira;

/* Thread panel content */
Object *TX_ThreadTitle, *LV_ThreadReplies, *ST_ThreadReply;

BOOL drag_reordering_enabled = FALSE;
BOOL collapse_all = FALSE;

IPTR collapse_all_hook(struct Hook *hook, APTR obj, APTR msg) {
  collapse_all = !collapse_all;
  if (collapse_all) {
    set(PG_Sidebar, MUIA_PanelGroup_CollapseAll, TRUE);
    set(BT_Profile, MUIA_Text_Contents, "Expand all");
  } else {
    set(PG_Sidebar, MUIA_PanelGroup_ExpandAll, TRUE);
    set(BT_Profile, MUIA_Text_Contents, "Collapse all");
  }
  return 0;
}

struct Hook collapse_all_toggle_hook = { {NULL, NULL},
  (HOOKFUNC)HookEntry,
  (HOOKFUNC)collapse_all_hook, NULL};

/* Hook to toggle drag reordering */
IPTR reorder_hook(struct Hook *hook, APTR obj, APTR msg)
{
    drag_reordering_enabled = !drag_reordering_enabled;

    set(PG_Sidebar, MUIA_PanelGroup_DragReordering, drag_reordering_enabled);

    /* Update button text to show current state */
    if (drag_reordering_enabled) {
        set(BT_Reorder, MUIA_Text_Contents, "Disable reordering");
    } else {
        set(BT_Reorder, MUIA_Text_Contents, "Enable reordering");
    }
    return 0;
}

struct Hook reorder_toggle_hook = { {NULL, NULL},
  (HOOKFUNC)HookEntry,
  (HOOKFUNC)reorder_hook, NULL};

/****************************************************************
 Allocate resources for gui
*****************************************************************/
BOOL init_gui(void)
{
    // clang-format off
    app = ApplicationObject,
        MUIA_Application_Title,      (IPTR) "Chat-like Interface",
        MUIA_Application_Version,    (IPTR) "$VER: PanelGroup 1.0 (01.01.25)",
        MUIA_Application_Copyright,  (IPTR) "© 2025, The AROS Development Team",
        MUIA_Application_Author,     (IPTR) "The AROS Development Team",
        MUIA_Application_Description,(IPTR) "Panel and PanelGroup demonstration",
        MUIA_Application_Base,       (IPTR) "PanelGroup",

        SubWindow, WD_Main = WindowObject,
            MUIA_Window_Title,  (IPTR) "Chat-like Interface - Panel & PanelGroup Demo",
            MUIA_Window_ID,     MAKE_ID('M', 'A', 'I', 'N'),
            MUIA_Window_Width,  1000,
            MUIA_Window_Height, 700,

            WindowContents, VGroup,
                /* Top Bar - Workspace info and search */
                Child, GR_TopBar = HGroup,
                    MUIA_Group_Spacing, 4,
                    MUIA_Background,    MUII_GroupBack,
                    MUIA_Frame,         MUIV_Frame_Button,

                    Child, BT_Workspace = SimpleButton("Toggle apps"),
                    Child, BT_Search = SimpleButton("Toggle threads"),
                    Child, HSpace(0),
                    Child, BT_Reorder = SimpleButton("Toggle enable reordering"),
                    Child, BT_Profile = SimpleButton("Collapse all"),
                End,

                /* Main Layout - Three column layout */
                Child, HGroup,
                    MUIA_Group_Spacing, 10,

                    /* Left Sidebar - Collapsible panels for channels, DMs, apps */
                    Child, VGroup,
                        MUIA_Group_Spacing, 2,
                        MUIA_HorizWeight,   25,

                        Child, PG_Sidebar = VPanelGroup,
                            MUIA_PanelGroup_AllowMultiple, TRUE,
                            MUIA_Group_Spacing, 10,
                            MUIA_PanelGroup_DragReordering, FALSE,

                            /* Channels Panel */
                            Child, P_Channels = Panel,
                                MUIA_Background,              MUII_GroupBack,
                                MUIA_Panel_Title,             (IPTR) "Channels",
                                MUIA_Panel_TitlePosition,     MUIV_Panel_Title_Top,
                                MUIA_Panel_TitleTextPosition, MUIV_Panel_Title_Text_Left,
                                MUIA_Panel_Collapsible,       TRUE,
                                MUIA_Panel_Collapsed,         FALSE,
                                MUIA_Panel_DrawSeparator,     TRUE,
                                MUIA_Panel_Padding,           4,
                                MUIA_Frame,                   MUIV_Frame_Button,

                                Child, VGroup,
                                    MUIA_Group_Spacing, 2,
                                    Child, BT_General = SimpleText("# general"),
                                    Child, BT_Random = SimpleText("# random"),
                                    Child, BT_Development = SimpleText("# development"),
                                    Child, BT_Design = SimpleText("# design"),
                                    Child, SimpleText("+ Add channels"),
                                End,
                            End,

                            /* Direct Messages Panel */
                            Child, P_DirectMessages = Panel,
                                MUIA_Background,              MUII_GroupBack,
                                MUIA_Panel_Title,             (IPTR) "Direct Messages",
                                MUIA_Panel_TitlePosition,     MUIV_Panel_Title_Top,
                                MUIA_Panel_TitleTextPosition, MUIV_Panel_Title_Text_Left,
                                MUIA_Panel_Collapsible,       TRUE,
                                MUIA_Panel_Collapsed,         FALSE,
                                MUIA_Panel_DrawSeparator,     FALSE,
                                MUIA_Panel_Padding,           4,

                                Child, VGroup,
                                    MUIA_Group_Spacing, 2,
                                    Child, BT_JohnDoe = SimpleText("- John Doe"),
                                    Child, BT_JaneSmith = SimpleText("- Jane Smith"),
                                    Child, BT_TeamLead = SimpleText("- Team Lead"),
                                    Child, SimpleText("+ Add people"),
                                End,
                            End,

                            /* Apps Panel */
                            Child, P_Apps = Panel,
                                MUIA_Background,              MUII_GroupBack,
                                MUIA_Panel_Title,             (IPTR) "Apps",
                                MUIA_Panel_TitlePosition,     MUIV_Panel_Title_Top,
                                MUIA_Panel_TitleTextPosition, MUIV_Panel_Title_Text_Right,
                                MUIA_Panel_Collapsible,       TRUE,
                                MUIA_Panel_Collapsed,         FALSE, /* Start collapsed */
                                MUIA_Panel_DrawSeparator,     FALSE,
                                MUIA_Panel_DrawStateIndicator, TRUE,
                                MUIA_Panel_Padding,           4,

                                Child, VGroup,
                                    MUIA_Group_Spacing, 2,
                                    Child, BT_Slack = SimpleText("Slack"),
                                    Child, BT_GitHub = SimpleText("GitHub"),
                                    Child, BT_Jira = SimpleText("Jira"),
                                    Child, SimpleText("+ Add apps"),
                                End,
                            End,
                        End, /* PG_Sidebar */
                    End, /* Left Sidebar VGroup */

                    /* Main Content Area */
                    Child, VGroup,
                        MUIA_HorizWeight,   50,
                        MUIA_Group_Spacing, 4,
                        MUIA_Background,    MUII_PageBack,

                        /* Channel Header */
                        Child, HGroup,
                            MUIA_Background, MUII_ButtonBack,
                            MUIA_Frame,      MUIV_Frame_Button,
                            Child, SimpleText("# general"),
                            Child, HSpace(0),
                            Child, SimpleText("42 members"),
                        End,

                        /* Message Area */
                        Child, GR_MessageArea = VGroup,
                            MUIA_Frame,      MUIV_Frame_ReadList,
                            MUIA_Background, MUII_ReadListBack,

                            Child, LV_Messages = ListviewObject,
                                MUIA_Listview_List, ListObject,
                                    InputListFrame,
                                    MUIA_List_ConstructHook, MUIV_List_ConstructHook_String,
                                    MUIA_List_DestructHook,  MUIV_List_DestructHook_String,
                                End,
                            End,
                        End,

                        /* Message Input */
                        Child, HGroup,
                            MUIA_Group_Spacing, 4,
                            Child, ST_MessageInput = StringObject,
                                StringFrame,
                                MUIA_String_Contents, (IPTR) "Type a message...",
                                MUIA_CycleChain,     TRUE,
                            End,
                            Child, SimpleButton("Send"),
                        End,
                    End, /* Main Content VGroup */

                    /* Right Sidebar - Thread Panel (collapsible) */
                    Child, VGroup,
                        MUIA_HorizWeight,   25,
                        MUIA_Group_Spacing, 2,

                        Child, PG_ThreadPanel = VPanelGroup,
                            MUIA_PanelGroup_AllowMultiple, FALSE, /* Only one panel expanded */

                            Child, P_ThreadDetails = Panel,
                                MUIA_Panel_Title,             (IPTR) "THREAD",
                                MUIA_Panel_TitleTextPosition, MUIV_Panel_Title_Text_Centered,
                                MUIA_Panel_Collapsible,       TRUE,
                                MUIA_Panel_Collapsed,         FALSE,
                                MUIA_Panel_DrawSeparator,     FALSE,
                                MUIA_Panel_Padding,           4,
                                MUIA_Panel_TitlePosition,     MUIV_Panel_Title_Left,
                                MUIA_Panel_TitleVertical,     TRUE,
                                MUIA_Background,              MUII_GroupBack,

                                Child, VGroup,
                                    Child, TX_ThreadTitle = SimpleText("Selected Message Thread"),

                                    Child, LV_ThreadReplies = ListviewObject,
                                        MUIA_Listview_List, ListObject,
                                            InputListFrame,
                                            MUIA_List_ConstructHook, MUIV_List_ConstructHook_String,
                                            MUIA_List_DestructHook,  MUIV_List_DestructHook_String,
                                        End,
                                    End,

                                    Child, ST_ThreadReply = StringObject,
                                        StringFrame,
                                        MUIA_String_Contents, (IPTR) "Reply to thread...",
                                        MUIA_CycleChain,     TRUE,
                                    End,
                                End,
                            End,
                        End, /* PG_ThreadPanel */
                        Child, HVSpace,
                    End, /* Right Sidebar VGroup */
                End, /* Main HGroup */
            End, /* Main VGroup */
        End, /* WindowObject */
    End; /* ApplicationObject */
    // clang-format on

    if (app) {
        /* Add some sample messages to the main message list */
        DoMethod(LV_Messages, MUIM_List_InsertSingle, "John Doe: Welcome to the team! ", MUIV_List_Insert_Bottom);
        DoMethod(LV_Messages,
                 MUIM_List_InsertSingle,
                 "Jane Smith: Thanks! Excited to be here",
                 MUIV_List_Insert_Bottom);
        DoMethod(LV_Messages,
                 MUIM_List_InsertSingle,
                 "Team Lead: We have a standup at 9 AM tomorrow",
                 MUIV_List_Insert_Bottom);
        DoMethod(LV_Messages, MUIM_List_InsertSingle, "John Doe: Perfect, I'll be there", MUIV_List_Insert_Bottom);
        DoMethod(LV_Messages,
                 MUIM_List_InsertSingle,
                 "Design Team: New mockups are ready for review",
                 MUIV_List_Insert_Bottom);
        DoMethod(LV_Messages,
                 MUIM_List_InsertSingle,
                 "Developer: I'll check them out after the meeting",
                 MUIV_List_Insert_Bottom);
        DoMethod(LV_Messages, MUIM_List_InsertSingle, "Manager: Great work everyone! ", MUIV_List_Insert_Bottom);

        /* Add some sample thread replies */
        DoMethod(LV_ThreadReplies, MUIM_List_InsertSingle, "Original: Welcome to the team!", MUIV_List_Insert_Bottom);
        DoMethod(LV_ThreadReplies, MUIM_List_InsertSingle, "Reply 1: Great to be here!", MUIV_List_Insert_Bottom);
        DoMethod(LV_ThreadReplies,
                 MUIM_List_InsertSingle,
                 "Reply 2: Looking forward to working together",
                 MUIV_List_Insert_Bottom);
        DoMethod(LV_ThreadReplies,
                 MUIM_List_InsertSingle,
                 "Reply 3: Any good resources for new team members?",
                 MUIV_List_Insert_Bottom);

        /* Quit application if the windowclosegadget or the esc key is pressed. */
        DoMethod(WD_Main,
                 MUIM_Notify,
                 MUIA_Window_CloseRequest,
                 TRUE,
                 app,
                 2,
                 MUIM_Application_ReturnID,
                 MUIV_Application_ReturnID_Quit);

        /* Toggle Apps panel when workspace button is clicked */
        DoMethod(BT_Workspace, MUIM_Notify, MUIA_Pressed, FALSE, PG_Sidebar, 3, MUIM_PanelGroup_TogglePanel, P_Apps);

        /* Replace your current BT_Reorder notify with this: */
        DoMethod(BT_Reorder, MUIM_Notify, MUIA_Pressed, FALSE, MUIV_Notify_Application, 2, MUIM_CallHook, &reorder_toggle_hook);

        /* Collapse all sidebar panels when profile button is clicked */
        DoMethod(BT_Profile, MUIM_Notify, MUIA_Pressed, FALSE, PG_Sidebar, 2, MUIM_CallHook, &collapse_all_toggle_hook);

        /* Toggle thread panel when search button is clicked */
        DoMethod(BT_Search,
                 MUIM_Notify,
                 MUIA_Pressed,
                 FALSE,
                 PG_ThreadPanel,
                 3,
                 MUIM_PanelGroup_TogglePanel,
                 P_ThreadDetails);

        /* Add message when send button or enter is pressed in message input */
        DoMethod(ST_MessageInput,
                 MUIM_Notify,
                 MUIA_String_Acknowledge,
                 MUIV_EveryTime,
                 LV_Messages,
                 3,
                 MUIM_List_InsertSingle,
                 "You: New message from input!",
                 MUIV_List_Insert_Bottom);

        /* Clear message input after sending */
        DoMethod(ST_MessageInput,
                 MUIM_Notify,
                 MUIA_String_Acknowledge,
                 MUIV_EveryTime,
                 ST_MessageInput,
                 3,
                 MUIM_Set,
                 MUIA_String_Contents,
                 "");

        return (TRUE);
    }
    return (FALSE);
}


/****************************************************************
 Deallocates all gui resources
*****************************************************************/
void deinit_gui(void)
{
    if (app)
        MUI_DisposeObject(app);
}


/****************************************************************
 The message loop
*****************************************************************/
void loop(void)
{
    ULONG sigs = 0;

    while ((LONG) DoMethod(app, MUIM_Application_NewInput, &sigs) != MUIV_Application_ReturnID_Quit) {
        if (sigs) {
            sigs = Wait(sigs | SIGBREAKF_CTRL_C | SIGBREAKF_CTRL_D);
            if (sigs & SIGBREAKF_CTRL_C) {
                break;
            }
            if (sigs & SIGBREAKF_CTRL_D) {
                break;
            }
        }
    }
}


/****************************************************************
 The main entry point
*****************************************************************/
int main(int argc, char *argv[])
{
    if (open_libs()) {
        if (init_gui()) {
            set(WD_Main, MUIA_Window_Open, TRUE);
            if (xget(WD_Main, MUIA_Window_Open)) {
                loop();
            }
            set(WD_Main, MUIA_Window_Open, FALSE);
            deinit_gui();
        }
        close_libs();
    }
    return RETURN_OK;
}
