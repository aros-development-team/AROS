/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Add an icon to Workbench's list of AppIcons.
    Lang: English
*/

#include <exec/types.h>
#include <exec/ports.h>
#include <utility/tagitem.h>
#include <dos/dos.h>
#include <graphics/gfx.h>

#include <proto/utility.h>

#include "workbench_intern.h"
#include <workbench/workbench.h>


/*****************************************************************************

    NAME */
    #include <proto/workbench.h>

        AROS_LH7(struct AppIcon *, AddAppIconA,
/*  SYNOPSIS */
        AROS_LHA(ULONG,               id,       D0),
        AROS_LHA(IPTR,                userdata, D1),
        AROS_LHA(const char *,        text,     A0),
        AROS_LHA(struct MsgPort *,    msgport,  A1),
        AROS_LHA(BPTR,                lock,     A2),
        AROS_LHA(struct DiskObject *, diskobj,  A3),
        AROS_LHA(struct TagItem *,    taglist,  A4),

/*  LOCATION */
        struct WorkbenchBase *, WorkbenchBase, 10, Workbench)

/*  FUNCTION

    Add an icon to the workbench's list of AppIcons. If a workbench is
    running, the icon will appear on the workbench screen given that the
    call is successful.
        When a user interacts with the AppIcon, an AppMessage of type
    MTYPE_APPICON is sent to the message port specified. The different
    supported actions are:

    1. User double-clicking on the icon. am_NumArgs is zero and am_ArgList is
       NULL.
    2. Dropping one or more icons on the AppIcon. am_Numargs is the number of
       icons dropped plus one; am_ArgList is an array of pointers to WBArg
       structures of the icons dropped.
    3. Dropping the AppIcon on another icon -- NOT SUPPORTED.
    4. Invoking an "Icons" menu item when the AppIcon is selected. am_Class
       will be set to a value in AMCLASSICON_Open ... AMCLASSICON_EmptyTrash.

    INPUTS

    id        --  AppIcon identification number; only for your use (ignored by
                  workbench.library)
    userdata  --  user specific data (ignored by workbench.library)
    text      --  name of the icon
    lock      --  currently unused (must be set to NULL)
    msgport   --  message port to which notification messages will be sent
    diskobj   --  pointer to a DiskObject structure filled in as described
                  below:
		      do_Magic    --  0
		      do_Version  --  0
		      do_Gadget   --  a gadget structure filled in as follows:
		             NextGadget    --  NULL
			     LeftEdge      --  0
			     TopEdge       --  0
			     Width         --  width of icon hit box
			     Height        --  height of icon hit box
			     Flags         --  0 or GADGHIMAGE
			     Activation    --  0
			     GadgetType    --  0
			     GadgetRender  --  pointer to an Image structure
			                       filled in as follows:
				   LeftEdge    --  0
				   TopEdge     --  0
				   Width       --  width of image (must be <=
				                   width of icon hit box)
				   Height      --  height of image (must be <=
				                   height of icon hit box)
				   Depth       --  number of bit planes of
				                   image
				   ImageData   --  pointer to word aligned
				                   image data
				   PlanePick   --  plane mask
				                   ((1 << depth) - 1)
				   PlaneOnOff  --  0
				   NextImage   --  NULL
			     SelectRender   --  NULL
			     GadgetText     --  NULL
			     MutualExclude  --  NULL
			     SpecialInfo    --  NULL
			     GadgetID       --  NULL
			     UserData       --  NULL
		      do_Type         --  0
		      do_DefaultTool  --  NULL
		      do_ToolTypes    --  NULL
		      do_CurrentX     --  NO_ICON_POSITION (recommended)
		      do_CurrentY     --  NO_ICON_POSITION (recommended)
		      do_DrawerData   --  NULL
		      do_ToolWindow   --  NULL
		      do_StackSize    --  0

    taglist  --  tags (see below)

    TAGS
    
    WBAPPICONA_SupportsOpen (BOOL)
    Set to TRUE if the AppIcon should respond to the "Open" menu.
    [default = TRUE]

    WBAPPICONA_SupportsCopy (BOOL)
    Set to TRUE if the AppIcon should respond to the "Copy" menu.
    [default = FALSE]

    WBAPPICONA_SupportsRename (BOOL)
    Set to TRUE if the AppIcon should respond to the "Rename" menu.
    [default = FALSE]
  
    WBAPPICONA_SupportsInformation (BOOL)
    Set to TRUE if the AppIcon should respond to the "Information" menu.
    [default = FALSE]
  
    WBAPPICONA_SupportsSnapshot (BOOL)
    Set to TRUE if the AppIcon should respond to the "Snapshot" menu.
    [default = FALSE]
  
    WBAPPICONA_SupportsUnSnapshot (BOOL)
    Set to TRUE if the AppIcon should respond to the "UnSnapshot" menu.
    [default = FALSE]

    WBAPPICONA_SupportsLeaveOut (BOOL)
    Set to TRUE if the AppIcon should respond to the "Leave Out" menu.
    [default = FALSE]

    WBAPPICONA_SupportsPutAway (BOOL)
    Set to TRUE if the AppIcon should respond to the "Put Away" menu.
    [default = FALSE]

    WBAPPICONA_SupportsDelete (BOOL)
    Set to TRUE if the AppIcon should respond to the "Delete" menu.
    [default = FALSE]

    WBAPPICONA_SupportsFormatDisk (BOOL)
    Set to TRUE if the AppIcon should respond to the "Format Disk" menu.
    [default = FALSE]

    WBAPPICONA_SupportsEmptyTrash (BOOL)
    Set to TRUE if the AppIcon should respond to the "Empty Trash" menu.
    [default = FALSE]

    WBAPPICONA_PropagatePosition (BOOL)
    Set to TRUE if the AppIcon's position should be updated in the DiskObject
    passed to this function when the AppIcon is moved. If this is set to TRUE,
    workbench.library will assume that the structure is not freed as long as
    the AppIcon is alive.
    [default = FALSE]

    WBAPPICONA_RenderHook (struct Hook *)
    Pointer to a hook that will be invoked when the AppIcon is rendered.
    Using this hook and WorkbenchControlA() dynamic or animated AppIcons may
    be created. The hook will be called with the following parameters:

          result = hookFunc(hook, reserved, arm);

	  where the 'hookFunc' has the prototype

	  LONG hookFunc(struct Hook *hook, APTR reserved,
	                struct AppIconRenderMsg *arm);

    If the hook function returns TRUE, the regular image of the AppIcon will
    be drawn; if it returns FALSE, nothing will be drawn. This allows you to
    do all the icon rendering except for when dragging the icon on the screen.

    WBAPPICONA_NotifySelectState (BOOL)
    When TRUE, you will be notificed whenever the AppIcon becomes selected or
    unselected; the am_Class will be set to AMCLASSICON_Selected or
    AMCLASSICON_Unselected.

    RESULT

    A pointer to an AppIcon structure -- which should be used with
    RemoveAppIcon() when you want to remove the icon -- or NULL if it was
    not possible to add the AppIcon.

    NOTES

    Contrary to AmigaOS, AppIcons may be added when there is no workbench
    application running.

    EXAMPLE

    BUGS

    SEE ALSO

    RemoveAppIcon(), WorkbenchControlA(), icon.library/DrawIconStateA()

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    const struct TagItem *tagState = taglist;
    const struct TagItem *tag;
    struct AppIcon *appIcon;

    if (diskobj == NULL || msgport == NULL ||
	diskobj->do_Gadget.GadgetRender == NULL)
    {
	return NULL;
    }

    appIcon = AllocVec(sizeof(struct AppIcon), MEMF_CLEAR | MEMF_ANY);
    
    if (appIcon == NULL)
    {     
        return NULL;
    }

    appIcon->ai_ID         = id;
    appIcon->ai_UserData   = userdata;
    appIcon->ai_Text       = text;
    appIcon->ai_MsgPort    = msgport;
    appIcon->ai_Flags      = WBAPPICONF_SupportsOpen;

    while ((tag = NextTagItem(&tagState)))
    {
        switch (tag->ti_Tag)
	{
	case WBAPPICONA_SupportsOpen:
	    if (tag->ti_Data)
	    {
		appIcon->ai_Flags |= WBAPPICONF_SupportsOpen;
	    }
	    break;

	case WBAPPICONA_SupportsCopy:
	    if (tag->ti_Data)
	    {
		appIcon->ai_Flags |= WBAPPICONF_SupportsCopy;
	    }
	    break;

	case WBAPPICONA_SupportsRename:
	    if (tag->ti_Data)
	    {
		appIcon->ai_Flags |= WBAPPICONF_SupportsRename;
	    }
	    break;

	case WBAPPICONA_SupportsInformation:
	    if (tag->ti_Data)
	    {
		appIcon->ai_Flags |= WBAPPICONF_SupportsInformation;
	    }
	    break;

	case WBAPPICONA_SupportsSnapshot:
	    if (tag->ti_Data)
	    {
		appIcon->ai_Flags |= WBAPPICONF_SupportsSnapshot;
	    }
	    break;

	case WBAPPICONA_SupportsUnSnapshot:
	    if (tag->ti_Data)
	    {
		appIcon->ai_Flags |= WBAPPICONF_SupportsUnSnapshot;
	    }
	    break;
	    
	case WBAPPICONA_SupportsLeaveOut:
	    if (tag->ti_Data)
	    {
		appIcon->ai_Flags |= WBAPPICONF_SupportsLeaveOut;
	    }
	    break;
	    
	case WBAPPICONA_SupportsPutAway:
	    if (tag->ti_Data)
	    {
		appIcon->ai_Flags |= WBAPPICONF_SupportsPutAway;
	    }
	    break;

	case WBAPPICONA_SupportsDelete:
	    if (tag->ti_Data)
	    {
		appIcon->ai_Flags |= WBAPPICONF_SupportsDelete;
	    }
	    break;
	    
	case WBAPPICONA_SupportsFormatDisk:
	    if (tag->ti_Data)
	    {
		appIcon->ai_Flags |= WBAPPICONF_SupportsFormatDisk;
	    }
	    break;
	    
	case WBAPPICONA_SupportsEmptyTrash:
	    if (tag->ti_Data)
	    {
		appIcon->ai_Flags |= WBAPPICONF_SupportsEmptyTrash;
	    }
	    break;
	    
	case WBAPPICONA_PropagatePosition:
	    if (tag->ti_Data)
	    {
		appIcon->ai_Flags |= WBAPPICONF_PropagatePosition;
	    }
	    break;

	case WBAPPICONA_RenderHook:
	    if (appIcon->ai_RenderHook != NULL)
	    {
		appIcon->ai_RenderHook = (struct Hook *)tag->ti_Data;
	    }
	    break;
	    
	case WBAPPICONA_NotifySelectState:
	    if (tag->ti_Data)
	    {
		appIcon->ai_Flags |= WBAPPICONF_NotifySelectState;
	    }
	    break;
        }
    }

    if (appIcon->ai_Flags & WBAPPICONF_PropagatePosition)
    {
	appIcon->ai_DiskObject = DupDiskObject(diskobj, TAG_DONE);

	if (appIcon->ai_DiskObject == NULL)
	{
	    FreeVec(appIcon);
	    
	    return NULL;
	}
    }
    else
    {
	appIcon->ai_DiskObject = diskobj;
    }

    LockWorkbench();
    AddTail(&WorkbenchBase->wb_AppIcons, (struct Node *)appIcon);
    UnlockWorkbench();

    /*
      NotifyWorkbench(WBNOTIFY_Create, WBNOTIFY_AppIcon, WorkbenchBase);
     */

    return appIcon;

    AROS_LIBFUNC_EXIT
} /* AddAppIconA() */
