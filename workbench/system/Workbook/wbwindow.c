#define DEBUG 0
#include <aros/debug.h>

#include <string.h>
#include <limits.h>
#include <intuition/icclass.h>

#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/gadtools.h>
#include <proto/workbench.h>
#include <proto/graphics.h>
#include <proto/layers.h>
#include <proto/icon.h>

#include <intuition/classusr.h>
#include <libraries/gadtools.h>

#include "workbook_intern.h"
#include "classes.h"

static inline WORD max(WORD a, WORD b)
{
    return (a > b) ? a : b;
}

struct wbWindow {
    STRPTR         Path;
    struct Window *Window;
    struct Menu   *Menu;
    Object        *ScrollH;
    Object        *ScrollV;
    Object        *Area;      /* Virual area of icons */
    Object        *Set;       /* Set of icons */

    ULONG          Flags;
};

#define WBWF_USERPORT   (1 << 0)    /* Window has a custom port */

static const struct NewMenu WBWindow_menu[] =  {
	{ NM_TITLE, "Workbook",   0, 0, 0, 0, },
	{  NM_ITEM, "Quit...",   "Q", 0, 0, 0, },
	{   NM_END, NULL,          0, 0, 0, 0, },
};

static const struct TagItem icon2window[] = {
    { GA_ID, WBWA_ActiveIconID },
    { TAG_END }
};

static IPTR wbIgnoreInfo_Hook(struct Hook *hook, struct ExAllData *ead, LONG *type)
{
    int i;

    i = strlen(ead->ed_Name);
    if (i >= 5 && strcmp(&ead->ed_Name[i-5],".info")==0)
    	return FALSE;

    return TRUE;
}

static void wbAddFiles(Class *cl, Object *obj, CONST_STRPTR path)
{
    struct WorkbookBase *wb = (APTR)cl->cl_UserData;
    struct wbWindow *my = INST_DATA(cl, obj);
    struct ExAllControl *eac;
    struct ExAllData *ead;
    BPTR lock;
    const ULONG eadSize = sizeof(struct ExAllData) + 1024;

    lock = Lock(path, SHARED_LOCK);
    if (lock != BNULL) {
    	STRPTR text,cp;
    	ULONG size = strlen(path) + NAME_MAX + 1;
    	text = AllocVec(size, MEMF_ANY);
    	cp = stpcpy(text, path);

    	ead = AllocVec(eadSize, MEMF_CLEAR);
    	if (ead != NULL) {
    	    eac = AllocDosObject(DOS_EXALLCONTROL, NULL);
    	    if (eac != NULL) {
    	    	struct Hook hook;
    	    	BOOL more = TRUE;

    	    	hook.h_Entry = HookEntry;
    	    	hook.h_SubEntry = wbIgnoreInfo_Hook;

    	    	eac->eac_MatchFunc = &hook;
    		while (more) {
    		    struct ExAllData *tmp = ead;
    		    int i;

    		    more = ExAll(lock, ead, eadSize, ED_NAME, eac);
    		    for (i = 0; i < eac->eac_Entries; i++, tmp=tmp->ed_Next) {
    		    	*cp = 0;
    		    	AddPart(text, tmp->ed_Name, size);
    			Object *iobj = NewObject(WBIcon, NULL,
    					   WBIA_File, text,
    					   ICA_TARGET, obj,
    					   ICA_MAP, icon2window,
    					   TAG_END);
    			if (iobj != NULL)
    			    DoMethod(my->Set, OM_ADDMEMBER, iobj);
    		    }
    		}
    		FreeDosObject(DOS_EXALLCONTROL, eac);
    	    }
    	    FreeVec(ead);
    	}
    	FreeVec(text);
    	UnLock(lock);
    } else {
    	D(bug("Can't lock %s\n", path));
    }
}

static void wbAddVolumeIcons(Class *cl, Object *obj)
{
    struct WorkbookBase *wb = (APTR)cl->cl_UserData;
    struct wbWindow *my = INST_DATA(cl, obj);
    struct DosList *dl;
    char text[NAME_MAX];

    /* Add all the DOS disks */
    dl = LockDosList(LDF_VOLUMES | LDF_READ);

    if (dl != BNULL) {
    	struct DosList *tdl;

    	tdl = dl;
    	while ((tdl = NextDosEntry(tdl, LDF_VOLUMES))) {
    	    Object *iobj;
    	   
    	    CopyMem(AROS_BSTR_ADDR(tdl->dol_Name), text, AROS_BSTR_strlen(tdl->dol_Name));
    	    CopyMem(":",&text[AROS_BSTR_strlen(tdl->dol_Name)],2);
    	    
    	    iobj = NewObject(WBIcon, NULL,
    	    	    WBIA_File, text,
    	    	    TAG_END);
    	    D(bug("Volume: %s => %p\n", text, iobj));
    	    if (iobj)
        	DoMethod(my->Set, OM_ADDMEMBER, iobj);
        }
        UnLockDosList(LDF_VOLUMES | LDF_READ);
    }
}

static void wbAddAppIcons(Class *cl, Object *obj)
{
    struct WorkbookBase *wb = (APTR)cl->cl_UserData;
    struct wbWindow *my = INST_DATA(cl, obj);
    struct DiskObject *icon;
    char text[NAME_MAX];


    /* Add all the AppIcons */
    icon = NULL;
    while ((icon = GetNextAppIcon(icon, &text[0]))) {
        Object *iobj = NewObject(WBIcon, NULL,
        	              WBIA_Icon, icon,
        	              TAG_END);
        if (iobj != NULL) {
        	DoMethod(my->Set, OM_ADDMEMBER, iobj);
        }
    }
}

static void wbRedimension(Class *cl, Object *obj)
{
    struct WorkbookBase *wb = (APTR)cl->cl_UserData;
    struct wbWindow *my = INST_DATA(cl, obj);
    struct Window *win = my->Window;
    struct IBox    real;     /* pos & size of the inner window area */

    real.Left = win->BorderLeft;
    real.Top  = win->BorderTop;
    real.Width = win->Width - (win->BorderLeft + win->BorderRight);
    real.Height= win->Height- (win->BorderTop  + win->BorderBottom + 6);

    D(bug("%s: (%d,%d) %dx%d\n", __func__,
    		real.Left, real.Top, real.Width, real.Height));

    SetAttrs(my->Area, GA_Top, real.Top,
    	               GA_Left,  real.Left,
    	               GA_Width, real.Width,
    	               GA_Height, real.Height,
    	               TAG_END);

    SetAttrs(my->ScrollH, PGA_Visible, real.Width,
    	                  GA_Left, real.Left,
    	                  GA_RelBottom, -my->Window->BorderBottom - 5,
    	                  GA_Width, real.Width,
    	                  GA_Height, my->Window->BorderBottom,
    	                  TAG_END);

    SetAttrs(my->ScrollV, PGA_Visible, real.Height,
    	                  GA_RelRight, -my->Window->BorderRight + 5,
    	                  GA_Top, real.Top,
    	                  GA_Width, my->Window->BorderRight,
    	                  GA_Height, real.Height - 10,
    	                  TAG_END);

    {
    	IPTR tot = 0, vis = 0;
    	GetAttr(PGA_Visible, my->ScrollV, &vis);
    	GetAttr(PGA_Total, my->ScrollV, &tot);
    	D(bug("%s: VScroll Total=%d Visible=%d\n", __func__, tot, vis));
    }
}

const struct TagItem scrollv2window[] = {
	{ PGA_Top, WBVA_VirtTop },
	{ TAG_END, 0 },
};
const struct TagItem scrollh2window[] = {
	{ PGA_Top, WBVA_VirtLeft },
	{ TAG_END, 0 },
};
const struct TagItem set2window[] = {
	{ GA_Width, WBVA_VirtWidth },
	{ GA_Height, WBVA_VirtHeight },
	{ TAG_END, 0 },
};


static IPTR WBWindowNew(Class *cl, Object *obj, struct opSet *ops)
{
    struct WorkbookBase *wb = (APTR)cl->cl_UserData;
    struct wbWindow *my;
    struct MsgPort *userport;
    CONST_STRPTR path;
    ULONG idcmp;
    IPTR rc = 0;
    APTR vis;

    rc = DoSuperMethodA(cl, obj, (Msg)ops);
    if (rc == 0)
    	return rc;

    obj = (Object *)rc;
    my = INST_DATA(cl, obj);

    path = (CONST_STRPTR)GetTagData(WBWA_Path, (IPTR)NULL, ops->ops_AttrList);
    if (path == NULL) {
    	my->Path = NULL;
    } else {
	my->Path = AllocVec(strlen(path)+1, MEMF_ANY);
	if (my->Path == NULL) {
	    DoSuperMethod(cl, obj, OM_DISPOSE, 0);
	    return 0;
	}
	strcpy(my->Path, path);
    }
D(bug("%s: Path='%s'\n", __func__, my->Path));
    /* Create icon set */
    my->Set = NewObject(WBSet, NULL,
    		ICA_TARGET, (IPTR)obj,
    		ICA_MAP, (IPTR)set2window,
    		TAG_END);

    if (my->Path == NULL) {
    	idcmp = IDCMP_CLOSEWINDOW;
    	my->Window = OpenWindowTags(NULL,
    			WA_IDCMP, 0,
    			WA_Backdrop,    TRUE,
    			WA_Borderless,  TRUE,
    			WA_Title,       "Workbook",
    			WA_ScreenTitle, "Workbook 1.0",
    			WA_NoCareRefresh, TRUE,
    			TAG_MORE, ops->ops_AttrList );
    } else {
    	struct DiskObject *icon;
    	struct NewWindow *nwin = NULL;

    	icon = GetDiskObjectNew(my->Path);
    	if (icon == NULL) {
    	    DisposeObject(my->Set);
    	    FreeVec(my->Path);
    	    DoSuperMethod(cl, obj, OM_DISPOSE);
    	    return 0;
    	}

    	if (icon->do_DrawerData) {
    	    nwin = &icon->do_DrawerData->dd_NewWindow;
    	    D(bug("%s: NewWindow %p\n", __func__, nwin));
    	}

    	idcmp = IDCMP_NEWSIZE | IDCMP_CLOSEWINDOW;
    	my->Window = OpenWindowTags(nwin,
    			WA_IDCMP, 0,
    			WA_MinWidth, 100,
    			WA_MinHeight, 100,
    			WA_MaxWidth, ~0,
    			WA_MaxHeight, ~0,
    			WA_Backdrop, FALSE,
    			WA_Title,    my->Path,
    			WA_NoCareRefresh, TRUE,
    			WA_SizeGadget, TRUE,
    			WA_DragBar, TRUE,
    			WA_DepthGadget, TRUE,
    			WA_CloseGadget, TRUE,
    			WA_Activate, TRUE,
    			WA_NewLookMenus, TRUE,
    			WA_AutoAdjust, TRUE,
    			WA_PubScreen, NULL,
    			TAG_MORE, ops->ops_AttrList );

    	FreeDiskObject(icon);
    }

    if (!my->Window) {
    	FreeVec(my->Path);
    	DisposeObject(my->Set);
    	DoSuperMethod(cl, obj, OM_DISPOSE, 0);
    	return 0;
    }

    /* If we want a shared port, do it. */
    userport = (struct MsgPort *)GetTagData(WBWA_UserPort, (IPTR)NULL, ops->ops_AttrList);
    if (userport) {
    	my->Flags |= WBWF_USERPORT;
    	my->Window->UserPort = userport;
    }
    ModifyIDCMP(my->Window, idcmp);

    /* The gadgets' layout will be performed during wbRedimension
     */
    AddGadget(my->Window, (struct Gadget *)(my->Area = NewObject(WBVirtual, NULL,
    		WBVA_Gadget, (IPTR)my->Set,
    		TAG_END)), 0);

    /* Add the verical scrollbar */
    AddGadget(my->Window, (struct Gadget *)(my->ScrollV = NewObject(NULL, "propgclass",
    		GA_RightBorder, TRUE,

    		ICA_TARGET, (IPTR)obj,
    		ICA_MAP, (IPTR)scrollv2window,
    		PGA_Borderless, TRUE,
    		PGA_Freedom, FREEVERT,
    		PGA_NewLook, TRUE,
    		PGA_Total, 1,
    		PGA_Visible, 1,
    		PGA_Top, 0,
    		TAG_END)), 0);
    
    /* Add the horizontal scrollbar */
    AddGadget(my->Window, (struct Gadget *)(my->ScrollH = NewObject(NULL, "propgclass",
    		ICA_TARGET, (IPTR)obj,
    		ICA_MAP, (IPTR)scrollh2window,
    		PGA_Freedom, FREEHORIZ,
    		PGA_NewLook, TRUE,
    		PGA_Total, 1,
    		PGA_Visible, 1,
    		PGA_Top, 0,
    		TAG_END)), 0);

    if (my->Path == NULL) {
    	wbAddVolumeIcons(cl, obj);
    	wbAddAppIcons(cl, obj);
    } else
    	wbAddFiles(cl, obj, my->Path);

    wbRedimension(cl, obj);

    my->Menu = CreateMenusA(WBWindow_menu, NULL);
    if (my->Menu == NULL) {
    	CloseWindow(my->Window);
    	FreeVec(my->Path);
    	DoSuperMethod(cl, obj, OM_DISPOSE, 0);
    	return 0;
    }

    vis = GetVisualInfo(my->Window->WScreen, TAG_END);
    LayoutMenus(my->Menu, vis, TAG_END);
    FreeVisualInfo(vis);

    SetMenuStrip(my->Window, my->Menu);

    RefreshGadgets(my->Window->FirstGadget, my->Window, NULL);

    return rc;
}

static IPTR WBWindowDispose(Class *cl, Object *obj, Msg msg)
{
    struct WorkbookBase *wb = (APTR)cl->cl_UserData;
    struct wbWindow *my = INST_DATA(cl, obj);

    ClearMenuStrip(my->Window);
    FreeMenus(my->Menu);

    /* If we have a custom user port, be paranoid.
     * See the Autodocs for CloseWindow().
     */
    if (my->Flags & WBWF_USERPORT) {
    	struct IntuiMessage *msg;
    	struct Node *succ;

    	Forbid();
    	msg = (APTR)my->Window->UserPort->mp_MsgList.lh_Head;
    	while ((succ = msg->ExecMessage.mn_Node.ln_Succ )) {
    	    if (msg->IDCMPWindow == my->Window) {
    	    	Remove((APTR)msg);
    	    	ReplyMsg((struct Message *)msg);
    	    }

    	    msg = (struct IntuiMessage *) succ;
    	}

    	my->Window->UserPort = NULL;
    	ModifyIDCMP(my->Window, 0);

    	Permit();
    }

    /* As a side effect, this will close all the
     * gadgets attached to it.
     */
    CloseWindow(my->Window);

    /* .. except for my->Set */
    DisposeObject(my->Set);

    return DoSuperMethodA(cl, obj, msg);
}

// OM_GET
static IPTR WBWindowGet(Class *cl, Object *obj, struct opGet *opg)
{
    struct wbWindow *my = INST_DATA(cl, obj);
    IPTR rc = TRUE;

    switch (opg->opg_AttrID) {
    case WBWA_Path:
    	*(opg->opg_Storage) = (IPTR)my->Path;
    	break;
    case WBWA_Window:
    	*(opg->opg_Storage) = (IPTR)my->Window;
    	break;
    default:
    	rc = DoSuperMethodA(cl, obj, (Msg)opg);
    	break;
    }

    return rc;
}

// OM_UPDATE
static IPTR WBWindowUpdate(Class *cl, Object *obj, struct opUpdate *opu)
{
    struct WorkbookBase *wb = (APTR)cl->cl_UserData;
    struct wbWindow *my = INST_DATA(cl, obj);
    const struct TagItem *tstate;
    struct TagItem *tag;
    IPTR rc;

    rc = DoSuperMethodA(cl, obj, (Msg)opu);

    /* Also send these to the Area */
    rc |= DoMethodA(my->Area, (Msg)opu);

    /* Update scrollbars if needed */
    tstate = opu->opu_AttrList;
    while ((tag = NextTagItem(&tstate))) {
    	switch (tag->ti_Tag) {
    	case WBVA_VirtLeft:
    	    rc = TRUE;
    	    break;
    	case WBVA_VirtTop:
    	    rc = TRUE;
    	    break;
    	case WBVA_VirtWidth:
    	    SetAttrs(my->ScrollH, PGA_Total, tag->ti_Data, TAG_END);
    	    rc = TRUE;
    	    break;
    	case WBVA_VirtHeight:
    	    SetAttrs(my->ScrollV, PGA_Total, tag->ti_Data, TAG_END);
    	    rc = TRUE;
    	    break;
    	}
    }

    if (rc && !(opu->opu_Flags & OPUF_INTERIM))
    	RefreshGadgets(my->Window->FirstGadget, my->Window, NULL);

    return rc;
}


// WBWM_NEWSIZE
static IPTR WBWindowNewSize(Class *cl, Object *obj, Msg msg)
{
    struct WorkbookBase *wb = (APTR)cl->cl_UserData;
    struct wbWindow *my = INST_DATA(cl, obj);

    wbRedimension(cl, obj);
    RefreshWindowFrame(my->Window);
    RefreshGadgets(my->Window->FirstGadget, my->Window, NULL);

    return 0;
}

static IPTR dispatcher(Class *cl, Object *obj, Msg msg)
{
    IPTR rc = 0;

    switch (msg->MethodID) {
    case OM_NEW:       rc = WBWindowNew(cl, obj, (APTR)msg); break;
    case OM_DISPOSE:   rc = WBWindowDispose(cl, obj, (APTR)msg); break;
    case OM_GET:       rc = WBWindowGet(cl, obj, (APTR)msg); break;
    case OM_UPDATE:    rc = WBWindowUpdate(cl, obj, (APTR)msg); break;
    case WBWM_NEWSIZE: rc = WBWindowNewSize(cl, obj, (APTR)msg); break;
    default:           rc = DoSuperMethodA(cl, obj, msg); break;
    }

    return rc;
}

Class *WBWindow_MakeClass(struct WorkbookBase *wb)
{
    Class *cl;

    cl = MakeClass( NULL, "rootclass", NULL,
                    sizeof(struct wbWindow),
                    0);
    if (cl != NULL) {
    	cl->cl_Dispatcher.h_Entry = HookEntry;
    	cl->cl_Dispatcher.h_SubEntry = dispatcher;
    	cl->cl_Dispatcher.h_Data = NULL;
    	cl->cl_UserData = (IPTR)wb;
    }

    return cl;
}
