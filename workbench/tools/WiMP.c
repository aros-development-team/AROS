/*
    Copyright © 1995-2006, The AROS Development Team. All rights reserved.
    $Id$

    WiMP -- Window manipulation program.
 */

/******************************************************************************

    NAME

        WiMP

    SYNOPSIS


    LOCATION

        Workbench:Tools

    FUNCTION

        Manipulates screens and windows

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/

#define MUIMASTER_YES_INLINE_STDARG

#include <libraries/mui.h>
#include <libraries/gadtools.h>
#include <proto/muimaster.h>
#include <proto/intuition.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/utility.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>


static Object *app, *wnd, *list_gad;

static Object *close_gad, *front_gad, *back_gad, *origin_gad, *activate_gad, *zip_gad, *hide_gad, *show_gad;
static Object *update_gad, *rescue_gad, *showall_gad, *rethink_gad, *about_gad;

static Object *info_wnd, *page_gad;

static Object *info_scr_addr_gad, *info_scr_leftedge_gad, *info_scr_topedge_gad, *info_scr_width_gad, *info_scr_height_gad,
	      *info_scr_flags_gad, *info_scr_title_gad, *info_scr_deftitle_gad, *info_scr_firstwindow_gad;


static Object *info_win_addr_gad, *info_win_nextwin_gad, *info_win_leftedge_gad, *info_win_topedge_gad, *info_win_width_gad,
    *info_win_height_gad, *info_win_minwidth_gad, *info_win_minheight_gad, *info_win_maxwidth_gad, *info_win_maxheight_gad,
    *info_win_flags_gad, *info_win_idcmp_gad, *info_win_title_gad, *info_win_req_gad, *info_win_screen_gad,
    *info_win_borderleft_gad, *info_win_bordertop_gad, *info_win_borderright_gad, *info_win_borderbottom_gad,
    *info_win_parentwin_gad, *info_win_firstchild_gad, *info_win_parent_gad, *info_win_descendant_gad;

static ULONG lock;


static struct Hook close_hook, front_hook, back_hook, origin_hook, activate_hook, zip_hook, hide_hook, show_hook;
static struct Hook update_hook, rescue_hook, showall_hook, rethink_hook, about_hook;
static struct Hook display_hook, construct_hook, destruct_hook;
static struct Hook openinfo_hook, updateinfo_hook;


static const STRPTR ABOUT_TXT		= "WiMP - The Window Manipulation Program\n\n"
					  "Copyright © 2000-2006 by The AROS Development Team";
static const STRPTR TITLE_TXT		= "WiMP - The Window Manipulation Program";
static const STRPTR INFOTITLE_TXT	= "WiMP - InfoWindow";
static const STRPTR CLOSESCREEN_TXT	= "Do you really want to Close the selected Screen?";
static const STRPTR CLOSEWINDOW_TXT	= "Do you really want to Close the selected Window?";
static const STRPTR YESNO_TXT		= "Yes.|No!";
static const STRPTR CONTINUE_TXT	= "Continue";


enum {
  None_type,
  Window_type,
  Screen_type,
  Max_type
};


struct ListEntry
{
    LONG type;
    APTR aptr;
    TEXT address[20];
    TEXT size[12];
    TEXT pos[12];
    TEXT status[4];
    TEXT title[40];
};

static const char version[] = "$VER: WiMP 0.11 (02.05.2006) © AROS Dev Team";

/*********************************************************************************************/

static void Cleanup(CONST_STRPTR txt);
static LONG get_selected(struct Screen **scr, struct Window **win);
static void MakeGUI(void);

/*********************************************************************************************/

enum {
    MN_ABOUT=1,
    MN_QUIT,
    MN_UPDATE,
    MN_KILL,
    MN_FRONT,
    MN_BACK,
    MN_ORIGIN,
    MN_ACTIVATE,
    MN_ZIP,
    MN_HIDE,
    MN_SHOW,
    MN_INFO,
    MN_RESCUE,
    MN_SHOWALL,
    MN_RETHINK,
};

static struct NewMenu nm[] =
{
  {NM_TITLE, "Project"},
    {NM_ITEM, "About...", NULL, 0, 0, (APTR)MN_ABOUT},
    {NM_ITEM, NM_BARLABEL},
    {NM_ITEM, "Quit", "Q", 0, 0, (APTR)MN_QUIT},
  {NM_TITLE, "Window List"},
    {NM_ITEM, "Update List", "U", 0, 0, (APTR)MN_UPDATE},
    {NM_ITEM, NM_BARLABEL},
    {NM_ITEM, "Kill", NULL, 0, 0, (APTR)MN_KILL},
    {NM_ITEM, "To Front", NULL, 0, 0, (APTR)MN_FRONT},
    {NM_ITEM, "To Back", NULL, 0, 0, (APTR)MN_BACK},
    {NM_ITEM, "To Origin", NULL, 0, 0, (APTR)MN_ORIGIN},
    {NM_ITEM, "Activate", NULL, 0, 0, (APTR)MN_ACTIVATE},
    {NM_ITEM, "Zip", NULL, 0, 0, (APTR)MN_ZIP},
    {NM_ITEM, "Hide", NULL, 0, 0, (APTR)MN_HIDE},
    {NM_ITEM, "Show", NULL, 0, 0, (APTR)MN_SHOW},
    {NM_ITEM, NM_BARLABEL},
    {NM_ITEM, "Info", NULL, 0, 0, (APTR)MN_INFO},
  {NM_TITLE, "Generic"},
    {NM_ITEM, "Rescue All", NULL, 0, 0, (APTR)MN_RESCUE},
    {NM_ITEM, "Show All", NULL, 0, 0, (APTR)MN_SHOWALL},
    {NM_ITEM, "RethinkDisplay", NULL, 0, 0, (APTR)MN_RETHINK},
  {NM_END}
};

/*********************************************************************************************/

static LONG get_selected(struct Screen **scr, struct Window **win)
{
    *scr = NULL;
    *win = NULL;

    struct ListEntry *le;
    DoMethod(list_gad, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, (IPTR)&le);

    if (le)
    {
	lock = LockIBase ( 0 );
	*scr = IntuitionBase->FirstScreen;
	UnlockIBase ( lock );
	/* Traverse through all Screens */
	while ( *scr )
	{
	    if ((le->type == Screen_type) && (le->aptr == *scr))
	    {
		return Screen_type;
	    }

	    /* Traverse through all Windows of current Screen */
	    *win = (*scr)->FirstWindow;
	    while ( *win )
	    {
		if ((le->type == Window_type) && (le->aptr == *win))
		{
		    return Window_type;
		}

		*win = (*win)->NextWindow;
	    }
	    *scr = (*scr)->NextScreen;
	}
    }
    return None_type;
}

/*********************************************************************************************/

AROS_UFH3(void, display_func,
    AROS_UFHA(struct Hook *     , h,     A0),
    AROS_UFHA(char **           , array, A2),
    AROS_UFHA(struct ListEntry *, msg,   A1))
{
    AROS_USERFUNC_INIT;

    if (msg)
    {
	if (msg->type == Window_type)
	{
	    array[0] = "  \033bWindow";
	}
	else
	{
	    array[0] = "\033b\033uScreen";
	}
	array[1] = msg->address;
	array[2] = msg->size;
	array[3] = msg->pos;
	array[4] = msg->status;
	array[5] = msg->title;
    }
    else
    {
	array[0] = "Type";
	array[1] = "Address";
	array[2] = "Size";
	array[3] = "Position";
	array[4] = "Status";
	array[5] = "Title";
    }

    AROS_USERFUNC_EXIT
}

/*********************************************************************************************/

AROS_UFH3(APTR, construct_func,
    AROS_UFHA(struct Hook *     , h,    A0),
    AROS_UFHA(APTR              , pool, A2),
    AROS_UFHA(struct ListEntry *, msg,  A1))
{
    AROS_USERFUNC_INIT

    struct ListEntry *new = AllocPooled(pool, sizeof(*msg));
    if (new)
	*new = *msg;

    return new;

    AROS_USERFUNC_EXIT
}

/*********************************************************************************************/

AROS_UFH3(void, destruct_func,
    AROS_UFHA(struct Hook *     , h,    A0),
    AROS_UFHA(APTR              , pool, A2),
    AROS_UFHA(struct ListEntry *, msg,  A1))
{
    AROS_USERFUNC_INIT

    FreePooled(pool, msg, sizeof(*msg));

    AROS_USERFUNC_EXIT
}

/*********************************************************************************************/

AROS_UFH3(void, update_func,
    AROS_UFHA(struct Hook *, h,      A0),
    AROS_UFHA(Object *     , object, A2),
    AROS_UFHA(APTR         , msg,    A1))
{
    AROS_USERFUNC_INIT;

    struct Screen *scr;
    struct Window *win;
    struct ListEntry entry;

    set(list_gad, MUIA_List_Quiet, TRUE);
    DoMethod(list_gad, MUIM_List_Clear);

    /* Get Intuition's first Screen */
    lock = LockIBase ( 0 );
    scr = IntuitionBase->FirstScreen;
    UnlockIBase ( lock );

    /* Traverse through all Screens */
    while ( scr )
    {
	entry.type = Screen_type;
	entry.aptr = scr;
	sprintf(entry.address, "%p", scr);
	sprintf(entry.size, "%d x %d", scr->Width, scr->Height);
	sprintf(entry.pos, "%d x %d", scr->LeftEdge, scr->TopEdge);
	entry.status[0] = '\0';
	snprintf(entry.title, sizeof(entry.title) - 1, "%s", scr->Title);
	DoMethod(list_gad, MUIM_List_InsertSingle, &entry, MUIV_List_Insert_Bottom);

	/* Traverse through all Windows of current Screen */
	win = scr->FirstWindow;
	while ( win )
	{
	    entry.type = Window_type;
	    entry.aptr = win;
	    sprintf(entry.address, "%p", win);
	    sprintf(entry.size, "%d x %d", win->Width, win->Height);
	    sprintf(entry.pos, "%d x %d", win->LeftEdge, win->TopEdge);
	    sprintf(entry.status, "%c%c%c", 
		    (IsWindowVisible(win) ? ' ' : 'H'),
		    (IS_CHILD(win)        ? 'C' : ' '),
		    (HAS_CHILDREN(win)    ? 'P' : ' '));
	    snprintf(entry.title, sizeof(entry.title) - 1, "%s", win->Title);
	    DoMethod(list_gad, MUIM_List_InsertSingle, &entry, MUIV_List_Insert_Bottom);

	    win = win->NextWindow;
	}
	scr = scr->NextScreen;
    }
    set(list_gad, MUIA_List_Quiet, FALSE);

    AROS_USERFUNC_EXIT
}

/*********************************************************************************************/

AROS_UFH3(void, close_func,
    AROS_UFHA(struct Hook *, h,      A0),
    AROS_UFHA(Object *     , object, A2),
    AROS_UFHA(APTR         , msg,    A1))
{
    AROS_USERFUNC_INIT;

    struct Screen *scr;
    struct Window *win;
    switch (get_selected(&scr, &win))
    {
	case Screen_type :
	    if (MUI_Request(app, wnd, 0, TITLE_TXT,  YESNO_TXT, CLOSESCREEN_TXT))
	    {
		CloseScreen(scr);
	    }
	    break;
	case Window_type :
	    if (MUI_Request(app, wnd, 0, TITLE_TXT,  YESNO_TXT, CLOSEWINDOW_TXT))
	    {
		CloseWindow(win);
	    }
	    break;
    }

    CallHookPkt(&update_hook, 0, 0);

    AROS_USERFUNC_EXIT
}

/*********************************************************************************************/

AROS_UFH3(void, front_func,
    AROS_UFHA(struct Hook *, h,      A0),
    AROS_UFHA(Object *     , object, A2),
    AROS_UFHA(APTR         , msg,    A1))
{
    AROS_USERFUNC_INIT;

    struct Screen *scr;
    struct Window *win;
    switch (get_selected(&scr, &win))
    {
	case Screen_type:
	    ScreenToFront(scr);
	    break;
	case Window_type:
	    WindowToFront(win);
	    break;
    }

    CallHookPkt(&update_hook, 0, 0);

    AROS_USERFUNC_EXIT
}

/*********************************************************************************************/

AROS_UFH3(void, back_func,
    AROS_UFHA(struct Hook *, h,      A0),
    AROS_UFHA(Object *     , object, A2),
    AROS_UFHA(APTR         , msg,    A1))
{
    AROS_USERFUNC_INIT;

    struct Screen *scr;
    struct Window *win;
    switch (get_selected(&scr, &win))
    {
	case Screen_type:
	    ScreenToBack(scr);
	    break;
	case Window_type:
	    WindowToBack(win);
	    break;
    }

    CallHookPkt(&update_hook, 0, 0);

    AROS_USERFUNC_EXIT
}

/*********************************************************************************************/

AROS_UFH3(void, origin_func,
    AROS_UFHA(struct Hook *, h,      A0),
    AROS_UFHA(Object *     , object, A2),
    AROS_UFHA(APTR         , msg,    A1))
{
    AROS_USERFUNC_INIT;

    struct Screen *scr;
    struct Window *win;
    switch (get_selected(&scr, &win))
    {
	case Screen_type :
	    MoveScreen ( scr, -scr->LeftEdge, -scr->TopEdge );
	    break;
	case Window_type :
	    MoveWindow ( win, -win->RelLeftEdge, -win->RelTopEdge );
	    break;
    }

    Delay(5);
    CallHookPkt(&update_hook, 0, 0);

    AROS_USERFUNC_EXIT
}

/*********************************************************************************************/

AROS_UFH3(void, activate_func,
    AROS_UFHA(struct Hook *, h,      A0),
    AROS_UFHA(Object *     , object, A2),
    AROS_UFHA(APTR         , msg,    A1))
{
    AROS_USERFUNC_INIT;

    struct Screen *scr;
    struct Window *win;
    if (get_selected(&scr, &win) == Window_type )
    {
	ActivateWindow ( win );
    }

    CallHookPkt(&update_hook, 0, 0);

    AROS_USERFUNC_EXIT
}

/*********************************************************************************************/

AROS_UFH3(void, hide_func,
    AROS_UFHA(struct Hook *, h,      A0),
    AROS_UFHA(Object *     , object, A2),
    AROS_UFHA(APTR         , msg,    A1))
{
    AROS_USERFUNC_INIT;

    struct Screen *scr;
    struct Window *win;
    if (get_selected(&scr, &win) == Window_type)
    {
	if ((struct Window*)XGET(wnd, MUIA_Window_Window) != win) // You can't hide WiMP
	{
	    HideWindow ( win );
	}
    }

    Delay(5);
    CallHookPkt(&update_hook, 0, 0);

    AROS_USERFUNC_EXIT
}

/*********************************************************************************************/

AROS_UFH3(void, show_func,
    AROS_UFHA(struct Hook *, h,      A0),
    AROS_UFHA(Object *     , object, A2),
    AROS_UFHA(APTR         , msg,    A1))
{
    AROS_USERFUNC_INIT;

    struct Screen *scr;
    struct Window *win;
    if (get_selected(&scr, &win) == Window_type)
    {
	ShowWindow ( win );
    }

    Delay(5);
    CallHookPkt(&update_hook, 0, 0);

    AROS_USERFUNC_EXIT
}

/*********************************************************************************************/

AROS_UFH3(void, zip_func,
    AROS_UFHA(struct Hook *, h,      A0),
    AROS_UFHA(Object *     , object, A2),
    AROS_UFHA(APTR         , msg,    A1))
{
    AROS_USERFUNC_INIT;

    struct Screen *scr;
    struct Window *win;
    if (get_selected(&scr, &win) == Window_type)
    {
	ZipWindow ( win );
    }

    CallHookPkt(&update_hook, 0, 0);

    AROS_USERFUNC_EXIT
}

/*********************************************************************************************/

AROS_UFH3(void, showall_func,
    AROS_UFHA(struct Hook *, h,      A0),
    AROS_UFHA(Object *     , object, A2),
    AROS_UFHA(APTR         , msg,    A1))
{
    AROS_USERFUNC_INIT;

    struct Screen *scr;
    struct Window *win;

    /* Get Intuition's first Screen */
    lock = LockIBase ( 0 );
    scr = IntuitionBase->FirstScreen;
    UnlockIBase ( lock );

    /* Traverse through all Screens */
    while ( scr )
    {
	win = scr->FirstWindow;
	/* Traverse through all Windows of current Screen */
	while ( win )
	{
	    /* Show Window if hidden */
	    if ( IsWindowVisible ( win ) != TRUE )
	    {
		ShowWindow ( win );
	    }
	    win = win->NextWindow;
	}
	scr = scr->NextScreen;
    }

    Delay(5);
    CallHookPkt(&update_hook, 0, 0);

    AROS_USERFUNC_EXIT
}

/*********************************************************************************************/

AROS_UFH3(void, rescue_func,
    AROS_UFHA(struct Hook *, h,      A0),
    AROS_UFHA(Object *     , object, A2),
    AROS_UFHA(APTR         , msg,    A1))
{
    AROS_USERFUNC_INIT;

    struct Screen *scr;
    struct Window *win;
    WORD width, height;

    /* Get Intuition's first Screen */
    lock = LockIBase ( 0 );
    scr = IntuitionBase->FirstScreen;
    UnlockIBase ( lock );

    /* Traverse through all Screens */
    while ( scr )
    {
	win = scr->FirstWindow;
	/* Traverse through all Windows of current Screen */
	while ( win )
	{
	    /* Move Window onto the Screen if outside */
	    if ( win->parent == NULL )
	    {
		width = scr->Width;
		height = scr->Height;
	    }
	    else
	    {
		width = win->parent->Width;
		height = win->parent->Height;
	    }
	    /* TODO:	calculate reasonable values:
	       eg. this way only the Close Gadget my be visible :-( */
	    if ( win->RelLeftEdge < 0
		    || win->RelTopEdge  <= -(win->BorderTop)
		    || win->RelLeftEdge > width
		    || win->RelTopEdge  >= (height - win->BorderTop) )
	    {
		MoveWindow ( win, - win->RelLeftEdge, - win->RelTopEdge );
	    }
	    win = win->NextWindow;
	}
	scr = scr->NextScreen;
    }

    Delay(5);
    CallHookPkt(&update_hook, 0, 0);

    AROS_USERFUNC_EXIT
}

/*********************************************************************************************/

AROS_UFH3(void, rethink_func,
    AROS_UFHA(struct Hook *, h,      A0),
    AROS_UFHA(Object *     , object, A2),
    AROS_UFHA(APTR         , msg,    A1))
{
    AROS_USERFUNC_INIT

    RethinkDisplay();
    CallHookPkt(&update_hook, 0, 0);

    AROS_USERFUNC_EXIT
}

/*********************************************************************************************/

AROS_UFH3(void, about_func,
    AROS_UFHA(struct Hook *, h,      A0),
    AROS_UFHA(Object *     , object, A2),
    AROS_UFHA(APTR         , msg,    A1))
{
    AROS_USERFUNC_INIT

    MUI_Request(app, wnd, 0, TITLE_TXT, CONTINUE_TXT, ABOUT_TXT);

    AROS_USERFUNC_EXIT
}

/*********************************************************************************************/

AROS_UFH3(void, update_info_func,
    AROS_UFHA(struct Hook *, h,      A0),
    AROS_UFHA(Object *     , object, A2),
    AROS_UFHA(APTR         , msg,    A1))
{
    AROS_USERFUNC_INIT;

    static TEXT buffer[50];
    struct ListEntry *le;
    struct Window *win;
    struct Screen *scr;

    DoMethod(list_gad, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, (IPTR)&le);
    if (le)
    {
	set(close_gad, MUIA_Disabled, FALSE);
	set(front_gad, MUIA_Disabled, FALSE);
	set(back_gad, MUIA_Disabled, FALSE);
	set(origin_gad, MUIA_Disabled, FALSE);
	set(activate_gad, MUIA_Disabled, FALSE);
	set(zip_gad, MUIA_Disabled, FALSE);
	set(hide_gad, MUIA_Disabled, FALSE);
	set(show_gad, MUIA_Disabled, FALSE);
    }
    else
    {
	set(close_gad, MUIA_Disabled, TRUE);
	set(front_gad, MUIA_Disabled, TRUE);
	set(back_gad, MUIA_Disabled, TRUE);
	set(origin_gad, MUIA_Disabled, TRUE);
	set(activate_gad, MUIA_Disabled, TRUE);
	set(zip_gad, MUIA_Disabled, TRUE);
	set(hide_gad, MUIA_Disabled, TRUE);
	set(show_gad, MUIA_Disabled, TRUE);
    }

    if (XGET(wnd,  MUIA_Window_Open) && le)
    {
	switch (get_selected(&scr, &win))
	{
	    case Screen_type:

		scr = le->aptr;
		set(page_gad, MUIA_Group_ActivePage, 0);

		sprintf(buffer, "%p", scr);
		set(info_scr_addr_gad, MUIA_Text_Contents, buffer);

		sprintf(buffer, "%d", scr->LeftEdge);
		set(info_scr_leftedge_gad, MUIA_Text_Contents, buffer);

		sprintf(buffer, "%d", scr->TopEdge);
		set(info_scr_topedge_gad, MUIA_Text_Contents, buffer);

		sprintf(buffer, "%d", scr->Width);
		set(info_scr_width_gad, MUIA_Text_Contents, buffer);

		sprintf(buffer, "%d", scr->Height);
		set(info_scr_height_gad, MUIA_Text_Contents, buffer);

		sprintf(buffer, "0x%08x", scr->Flags);
		set(info_scr_flags_gad, MUIA_Text_Contents, buffer);

		set(info_scr_title_gad, MUIA_Text_Contents, scr->Title);
		set(info_scr_deftitle_gad, MUIA_Text_Contents, scr->DefaultTitle);

		sprintf(buffer, "%p", scr->FirstWindow);
		set(info_scr_firstwindow_gad, MUIA_Text_Contents, buffer);

		break;

	    case Window_type:
		win = le->aptr;
		set(page_gad, MUIA_Group_ActivePage, 1);

		sprintf(buffer, "%p", win);
		set(info_win_addr_gad, MUIA_Text_Contents, buffer);

		sprintf(buffer, "%p", win->NextWindow);
		set(info_win_nextwin_gad, MUIA_Text_Contents, buffer);

		sprintf(buffer, "%d", win->LeftEdge);
		set(info_win_leftedge_gad, MUIA_Text_Contents, buffer);

		sprintf(buffer, "%d", win->TopEdge);
		set(info_win_topedge_gad, MUIA_Text_Contents, buffer);

		sprintf(buffer, "%d", win->Height);
		set(info_win_height_gad, MUIA_Text_Contents, buffer);

		sprintf(buffer, "%d", win->Width);
		set(info_win_width_gad, MUIA_Text_Contents, buffer);

		sprintf(buffer, "%d", win->MinWidth);
		set(info_win_minwidth_gad, MUIA_Text_Contents, buffer);

		sprintf(buffer, "%d", win->MinHeight);
		set(info_win_minheight_gad, MUIA_Text_Contents, buffer);

		sprintf(buffer, "%d", win->MaxWidth);
		set(info_win_maxwidth_gad, MUIA_Text_Contents, buffer);

		sprintf(buffer, "%d", win->MaxHeight);
		set(info_win_maxheight_gad, MUIA_Text_Contents, buffer);

		sprintf(buffer, "0x%08lx", win->Flags);
		set(info_win_flags_gad, MUIA_Text_Contents, buffer);

		sprintf(buffer, "0x%08lx", win->IDCMPFlags);
		set(info_win_idcmp_gad, MUIA_Text_Contents, buffer);

		set(info_win_title_gad, MUIA_Text_Contents, win->Title);

		sprintf(buffer, "%d", win->ReqCount);
		set(info_win_req_gad, MUIA_Text_Contents, buffer);

		sprintf(buffer, "%p", win->WScreen);
		set(info_win_screen_gad, MUIA_Text_Contents, buffer);

		sprintf(buffer, "%d", win->BorderLeft);
		set(info_win_borderleft_gad, MUIA_Text_Contents, buffer);

		sprintf(buffer, "%d", win->BorderTop);
		set(info_win_bordertop_gad, MUIA_Text_Contents, buffer);

		sprintf(buffer, "%d", win->BorderRight);
		set(info_win_borderright_gad, MUIA_Text_Contents, buffer);

		sprintf(buffer, "%d", win->BorderBottom);
		set(info_win_borderbottom_gad, MUIA_Text_Contents, buffer);

		sprintf(buffer, "%p", win->parent);
		set(info_win_parentwin_gad, MUIA_Text_Contents, buffer);

		sprintf(buffer, "%p", win->firstchild);
		set(info_win_firstchild_gad, MUIA_Text_Contents, buffer);

		sprintf(buffer, "%p", win->Parent);
		set(info_win_parent_gad, MUIA_Text_Contents, buffer);

		sprintf(buffer, "%p", win->Descendant);
		set(info_win_descendant_gad, MUIA_Text_Contents, buffer);

		break;
	    default:
		// selected screen/window doesn't exist anymore
		CallHookPkt(&update_hook, 0, 0);
		break;
	}
    }
    else
    {
	set(info_scr_addr_gad, MUIA_Text_Contents, NULL);
	set(info_scr_leftedge_gad, MUIA_Text_Contents, NULL);
	set(info_scr_topedge_gad, MUIA_Text_Contents, NULL);
	set(info_scr_width_gad, MUIA_Text_Contents, NULL);
	set(info_scr_height_gad, MUIA_Text_Contents, NULL);
	set(info_scr_flags_gad, MUIA_Text_Contents, NULL);
	set(info_scr_title_gad, MUIA_Text_Contents, NULL);
	set(info_scr_deftitle_gad, MUIA_Text_Contents, NULL);
	set(info_scr_firstwindow_gad, MUIA_Text_Contents, NULL);
	set(info_win_addr_gad, MUIA_Text_Contents, NULL);
	set(info_win_nextwin_gad, MUIA_Text_Contents, NULL);
	set(info_win_leftedge_gad, MUIA_Text_Contents, NULL);
	set(info_win_topedge_gad, MUIA_Text_Contents, NULL);
	set(info_win_height_gad, MUIA_Text_Contents, NULL);
	set(info_win_width_gad, MUIA_Text_Contents, NULL);
	set(info_win_minwidth_gad, MUIA_Text_Contents, NULL);
	set(info_win_minheight_gad, MUIA_Text_Contents, NULL);
	set(info_win_maxwidth_gad, MUIA_Text_Contents, NULL);
	set(info_win_maxheight_gad, MUIA_Text_Contents, NULL);
	set(info_win_flags_gad, MUIA_Text_Contents, NULL);
	set(info_win_idcmp_gad, MUIA_Text_Contents, NULL);
	set(info_win_title_gad, MUIA_Text_Contents, NULL);
	set(info_win_req_gad, MUIA_Text_Contents, NULL);
	set(info_win_screen_gad, MUIA_Text_Contents, NULL);
	set(info_win_borderleft_gad, MUIA_Text_Contents, NULL);
	set(info_win_bordertop_gad, MUIA_Text_Contents, NULL);
	set(info_win_borderright_gad, MUIA_Text_Contents, NULL);
	set(info_win_borderbottom_gad, MUIA_Text_Contents, NULL);
	set(info_win_parentwin_gad, MUIA_Text_Contents, NULL);
	set(info_win_firstchild_gad, MUIA_Text_Contents, NULL);
	set(info_win_parent_gad, MUIA_Text_Contents, NULL);
	set(info_win_descendant_gad, MUIA_Text_Contents, NULL);
    }

    AROS_USERFUNC_EXIT
}

/*********************************************************************************************/

AROS_UFH3(void, openinfo_func,
    AROS_UFHA(struct Hook *, h,      A0),
    AROS_UFHA(Object *     , object, A2),
    AROS_UFHA(APTR         , msg,    A1))
{
    AROS_USERFUNC_INIT

    set(info_wnd, MUIA_Window_Open, TRUE);
    CallHookPkt(&updateinfo_hook, 0, 0);
    AROS_USERFUNC_EXIT
}

/*********************************************************************************************/

static void MakeGUI(void)
{
    Object *menu = MUI_MakeObject(MUIO_MenustripNM, &nm, 0);

    display_hook.h_Entry = (HOOKFUNC)display_func;
    construct_hook.h_Entry = (HOOKFUNC)construct_func;
    destruct_hook.h_Entry = (HOOKFUNC)destruct_func;
    
    close_hook.h_Entry = (HOOKFUNC)close_func;
    front_hook.h_Entry = (HOOKFUNC)front_func;
    back_hook.h_Entry = (HOOKFUNC)back_func;
    origin_hook.h_Entry = (HOOKFUNC)origin_func;
    activate_hook.h_Entry = (HOOKFUNC)activate_func;
    zip_hook.h_Entry = (HOOKFUNC)zip_func;
    hide_hook.h_Entry = (HOOKFUNC)hide_func;
    show_hook.h_Entry = (HOOKFUNC)show_func;

    update_hook.h_Entry = (HOOKFUNC)update_func;
    rescue_hook.h_Entry = (HOOKFUNC)rescue_func;
    showall_hook.h_Entry = (HOOKFUNC)showall_func;
    rethink_hook.h_Entry = (HOOKFUNC)rethink_func;
    about_hook.h_Entry = (HOOKFUNC)about_func;

    openinfo_hook.h_Entry = (HOOKFUNC)openinfo_func;
    updateinfo_hook.h_Entry = (HOOKFUNC)update_info_func;
    
    app = ApplicationObject,
	MUIA_Application_Title, (IPTR)"WiMP",
	MUIA_Application_Version, (IPTR)version,
	MUIA_Application_Copyright, (IPTR)"Copyright  © 1995-2006, The AROS Development Team",
	MUIA_Application_Author, (IPTR)"The AROS Development Team",
	MUIA_Application_Description, (IPTR)"Window Manipulator",
	MUIA_Application_Base, (IPTR)"WIMP",
	MUIA_Application_SingleTask, TRUE,
	MUIA_Application_Menustrip, (IPTR)menu,
	SubWindow, (IPTR)(wnd = WindowObject,
	    MUIA_Window_Title, (IPTR)TITLE_TXT,
	    MUIA_Window_ID, MAKE_ID('W', 'I', 'M', 'P'),
	    WindowContents, (IPTR)(VGroup,
		Child, (IPTR)(VGroup,
		    GroupFrameT("Screen/Window List"),
		    Child, (IPTR)(ListviewObject,
			MUIA_Listview_List, (IPTR)(list_gad = ListObject,
			    InputListFrame,
			    MUIA_List_Format, (IPTR)"BAR,BAR,P=\033c BAR,P=\033c BAR,BAR,BAR",
			    MUIA_List_ConstructHook, (IPTR)&construct_hook,
			    MUIA_List_DestructHook, (IPTR)&destruct_hook,
			    MUIA_List_DisplayHook, (IPTR)&display_hook,
			    MUIA_List_Title, TRUE,
			    MUIA_CycleChain, 1,
			End),
		    End),
		    Child, (IPTR)(HGroup,
			Child, (IPTR)(close_gad = SimpleButton("\033iKill")),
			Child, (IPTR)(front_gad = SimpleButton("To _Front")),
			Child, (IPTR)(back_gad = SimpleButton("To _Back")),
			Child, (IPTR)(origin_gad = SimpleButton("Move to _Origin")),
			Child, (IPTR)(activate_gad = SimpleButton("_Activate")),
			Child, (IPTR)(zip_gad = SimpleButton("_Zip")),
			Child, (IPTR)(hide_gad = SimpleButton("_Hide")),
			Child, (IPTR)(show_gad = SimpleButton("_Show")),
		    End),
		    Child, (IPTR)(HGroup,
			Child, (IPTR)(update_gad = SimpleButton("_Update List")),
			Child, (IPTR)(rescue_gad = SimpleButton("_Rescue all Windows")),
			Child, (IPTR)(showall_gad = SimpleButton("Show all Windows")),
			Child, (IPTR)(rethink_gad = SimpleButton("Rethink Display")),
			Child, (IPTR)(about_gad = SimpleButton("About")),
		    End),
		End),
	    End),
	End), // wnd
	SubWindow, (IPTR)(info_wnd = WindowObject,
	    MUIA_Window_Title, (IPTR)INFOTITLE_TXT,
	    MUIA_Window_ID, MAKE_ID('W', 'I', 'N', 'F'),
	    WindowContents, (IPTR)(page_gad = PageGroup,
		Child, (IPTR)(HGroup,
		    Child, (IPTR)(ColGroup(2),
		    GroupFrameT("Screen"),

			Child, (IPTR)Label("Address"),
			Child, (IPTR)(info_scr_addr_gad = TextObject,
			    TextFrame,
			    MUIA_Text_Contents, (IPTR)"WWWWWWWWWWWWWWWWWWWWWWWWWWWWWW",
			    MUIA_Text_SetMin, TRUE,
			End),
			Child, (IPTR)Label("LeftEdge"),
			Child, (IPTR)(info_scr_leftedge_gad = TextObject, TextFrame, End),
			Child, (IPTR)Label("TopEdge"),
			Child, (IPTR)(info_scr_topedge_gad = TextObject, TextFrame, End),
			Child, (IPTR)Label("Width"),
			Child, (IPTR)(info_scr_width_gad = TextObject, TextFrame, End),
			Child, (IPTR)Label("Height"),
			Child, (IPTR)(info_scr_height_gad = TextObject, TextFrame, End),
			Child, (IPTR)Label("Flags"),
			Child, (IPTR)(info_scr_flags_gad = TextObject, TextFrame, End),
			Child, (IPTR)Label("Title"),
			Child, (IPTR)(info_scr_title_gad = TextObject, TextFrame, End),
			Child, (IPTR)Label("DefaultTitle"),
			Child, (IPTR)(info_scr_deftitle_gad = TextObject, TextFrame, End),
			Child, (IPTR)Label("FirstWindow"),
			Child, (IPTR)(info_scr_firstwindow_gad = TextObject, TextFrame, End),
		    End),
		End),
		Child, (IPTR)(HGroup,
		    Child, (IPTR)(ColGroup(2),
			GroupFrameT("Window"),
			Child, (IPTR)Label("Address"),
			Child, (IPTR)(info_win_addr_gad = TextObject, TextFrame, End),
			Child, (IPTR)Label("NextWindow"),
			Child, (IPTR)(info_win_nextwin_gad = TextObject, TextFrame, End),
			Child, (IPTR)Label("LeftEdge"),
			Child, (IPTR)(info_win_leftedge_gad = TextObject, TextFrame, End),
			Child, (IPTR)Label("TopEdge"),
			Child, (IPTR)(info_win_topedge_gad = TextObject, TextFrame, End),
			Child, (IPTR)Label("Width"),
			Child, (IPTR)(info_win_width_gad = TextObject, TextFrame, End),
			Child, (IPTR)Label("Height"),
			Child, (IPTR)(info_win_height_gad = TextObject, TextFrame, End),
			Child, (IPTR)Label("MinWidth"),
			Child, (IPTR)(info_win_minwidth_gad = TextObject, TextFrame, End),
			Child, (IPTR)Label("MinHeight"),
			Child, (IPTR)(info_win_minheight_gad = TextObject, TextFrame, End),
			Child, (IPTR)Label("MaxWidth"),
			Child, (IPTR)(info_win_maxwidth_gad = TextObject, TextFrame, End),
			Child, (IPTR)Label("MaxHeight"),
			Child, (IPTR)(info_win_maxheight_gad = TextObject, TextFrame, End),
			Child, (IPTR)Label("Flags"),
			Child, (IPTR)(info_win_flags_gad = TextObject, TextFrame, End),
			Child, (IPTR)Label("IDCMPFlags"),
			Child, (IPTR)(info_win_idcmp_gad = TextObject, TextFrame, End),
			Child, (IPTR)Label("Title"),
			Child, (IPTR)(info_win_title_gad = TextObject, TextFrame, End),
			Child, (IPTR)Label("ReqCount"),
			Child, (IPTR)(info_win_req_gad = TextObject, TextFrame, End),
			Child, (IPTR)Label("WScreen"),
			Child, (IPTR)(info_win_screen_gad = TextObject, TextFrame, End),
			Child, (IPTR)Label("BorderLeft"),
			Child, (IPTR)(info_win_borderleft_gad = TextObject, TextFrame, End),
			Child, (IPTR)Label("BorderTop"),
			Child, (IPTR)(info_win_bordertop_gad = TextObject, TextFrame, End),
			Child, (IPTR)Label("BorderRight"),
			Child, (IPTR)(info_win_borderright_gad = TextObject, TextFrame, End),
			Child, (IPTR)Label("BoderBottom"),
			Child, (IPTR)(info_win_borderbottom_gad = TextObject, TextFrame, End),
			Child, (IPTR)Label("Parent Window"),
			Child, (IPTR)(info_win_parentwin_gad = TextObject, TextFrame, End),
			Child, (IPTR)Label("First Child"),
			Child, (IPTR)(info_win_firstchild_gad = TextObject, TextFrame, End),
			Child, (IPTR)Label("Parent"),
			Child, (IPTR)(info_win_parent_gad = TextObject, TextFrame, End),
			Child, (IPTR)Label("Descendant"),
			Child, (IPTR)(info_win_descendant_gad = TextObject, TextFrame, End),
		    End),
		End),
	    End),
	End), // infownd
    End; // app
    
    if (! app)
	Cleanup(NULL); // Probably double start

    DoMethod(wnd, MUIM_Notify, MUIA_Window_CloseRequest, TRUE,
	app, 2, MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);

    DoMethod(info_wnd, MUIM_Notify, MUIA_Window_CloseRequest, TRUE,
	info_wnd, 3, MUIM_Set, MUIA_Window_Open, FALSE);

    DoMethod(list_gad, MUIM_Notify, MUIA_List_Active, MUIV_EveryTime,
	    (IPTR)app, 2, MUIM_CallHook, (IPTR)&updateinfo_hook);

    DoMethod(list_gad, MUIM_Notify, MUIA_Listview_DoubleClick, TRUE,
	    (IPTR)app, 2, MUIM_CallHook, (IPTR)&openinfo_hook);


    // menu bar
    DoMethod(app, MUIM_Notify, MUIA_Application_MenuAction, MN_QUIT,
	(IPTR)app, 2, MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);

    DoMethod(app, MUIM_Notify, MUIA_Application_MenuAction, MN_ABOUT,
	(IPTR)app, 2, MUIM_CallHook, (IPTR)&about_hook);

    DoMethod(app, MUIM_Notify, MUIA_Application_MenuAction, MN_UPDATE,
	(IPTR)app, 2, MUIM_CallHook, (IPTR)&update_hook);

    DoMethod(app, MUIM_Notify, MUIA_Application_MenuAction, MN_KILL,
	(IPTR)app, 2, MUIM_CallHook, (IPTR)&close_hook);

    DoMethod(app, MUIM_Notify, MUIA_Application_MenuAction, MN_FRONT,
	(IPTR)app, 2, MUIM_CallHook, (IPTR)&front_hook);

    DoMethod(app, MUIM_Notify, MUIA_Application_MenuAction, MN_BACK,
	(IPTR)app, 2, MUIM_CallHook, (IPTR)&back_hook);

    DoMethod(app, MUIM_Notify, MUIA_Application_MenuAction, MN_ORIGIN,
	(IPTR)app, 2, MUIM_CallHook, (IPTR)&origin_hook);

    DoMethod(app, MUIM_Notify, MUIA_Application_MenuAction, MN_ACTIVATE,
	(IPTR)app, 2, MUIM_CallHook, (IPTR)&activate_hook);

    DoMethod(app, MUIM_Notify, MUIA_Application_MenuAction, MN_ZIP,
	(IPTR)app, 2, MUIM_CallHook, (IPTR)&zip_hook);

    DoMethod(app, MUIM_Notify, MUIA_Application_MenuAction, MN_HIDE,
	(IPTR)app, 2, MUIM_CallHook, (IPTR)&hide_hook);

    DoMethod(app, MUIM_Notify, MUIA_Application_MenuAction, MN_SHOW,
	(IPTR)app, 2, MUIM_CallHook, (IPTR)&show_hook);

    DoMethod(app, MUIM_Notify, MUIA_Application_MenuAction, MN_INFO,
	(IPTR)app, 2, MUIM_CallHook, (IPTR)&openinfo_hook);

    DoMethod(app, MUIM_Notify, MUIA_Application_MenuAction, MN_RESCUE,
	(IPTR)app, 2, MUIM_CallHook, (IPTR)&rescue_hook);

    DoMethod(app, MUIM_Notify, MUIA_Application_MenuAction, MN_SHOWALL,
	(IPTR)app, 2, MUIM_CallHook, (IPTR)&showall_hook);

    DoMethod(app, MUIM_Notify, MUIA_Application_MenuAction, MN_RETHINK,
	(IPTR)app, 2, MUIM_CallHook, (IPTR)&rethink_hook);


    // buttons first row
    DoMethod(close_gad, MUIM_Notify, MUIA_Pressed, FALSE,
	(IPTR)app, 2, MUIM_CallHook, (IPTR)&close_hook);

    DoMethod(front_gad, MUIM_Notify, MUIA_Pressed, FALSE,
	(IPTR)app, 2, MUIM_CallHook, (IPTR)&front_hook);

    DoMethod(back_gad, MUIM_Notify, MUIA_Pressed, FALSE,
	(IPTR)app, 2, MUIM_CallHook, (IPTR)&back_hook);

    DoMethod(origin_gad, MUIM_Notify, MUIA_Pressed, FALSE,
	(IPTR)app, 2, MUIM_CallHook, (IPTR)&origin_hook);

    DoMethod(activate_gad, MUIM_Notify, MUIA_Pressed, FALSE,
	(IPTR)app, 2, MUIM_CallHook, (IPTR)&activate_hook);

    DoMethod(zip_gad, MUIM_Notify, MUIA_Pressed, FALSE,
	(IPTR)app, 2, MUIM_CallHook, (IPTR)&zip_hook);

    DoMethod(hide_gad, MUIM_Notify, MUIA_Pressed, FALSE,
	(IPTR)app, 2, MUIM_CallHook, (IPTR)&hide_hook);

    DoMethod(show_gad, MUIM_Notify, MUIA_Pressed, FALSE,
	(IPTR)app, 2, MUIM_CallHook, (IPTR)&show_hook);


    // buttons second row
    DoMethod(update_gad, MUIM_Notify, MUIA_Pressed, FALSE,
	(IPTR)app, 2, MUIM_CallHook, (IPTR)&update_hook);

    DoMethod(rescue_gad, MUIM_Notify, MUIA_Pressed, FALSE,
	(IPTR)app, 2, MUIM_CallHook, (IPTR)&rescue_hook);

    DoMethod(showall_gad, MUIM_Notify, MUIA_Pressed, FALSE,
	(IPTR)app, 2, MUIM_CallHook, (IPTR)&showall_hook);

    DoMethod(rethink_gad, MUIM_Notify, MUIA_Pressed, FALSE,
	(IPTR)app, 2, MUIM_CallHook, (IPTR)&rethink_hook);

    DoMethod(about_gad, MUIM_Notify, MUIA_Pressed, FALSE,
	(IPTR)app, 2, MUIM_CallHook, (IPTR)&about_hook);

    set(wnd, MUIA_Window_Open, TRUE);
    CallHookPkt(&update_hook, 0, 0);
    CallHookPkt(&updateinfo_hook, 0, 0);
    DoMethod(app, MUIM_Application_Execute);
}

/*********************************************************************************************/

static void Cleanup(CONST_STRPTR txt)
{
    MUI_DisposeObject(app);
    if (txt)
    {
	MUI_Request(app, wnd, 0, TITLE_TXT,  "OK", txt);
	exit(RETURN_ERROR);
    }
    exit(RETURN_OK);
}

/*********************************************************************************************/

int main(int argc, char **argv)
{
    MakeGUI();
    Cleanup(NULL);
    return RETURN_OK;
}
