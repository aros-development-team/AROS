/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/

/*********************************************************************************************/

#include "global.h"
#include "version.h"

#include <string.h>

#include <aros/debug.h>

/*********************************************************************************************/

#include <zune/prefswindow.h>

#include "menupopup3d_image.c"
#include "menupopupclassic_image.c"
#include "menupulldown3d_image.c"
#include "menupulldownclassic_image.c"

#if MENUPOPUP3D_PACKED
static UBYTE menupopup3d_imagedata[MENUPOPUP3D_WIDTH * MENUPOPUP3D_HEIGHT];
#else
#define menupopup3d_imagedata menupopup3d_data
#endif

#if MENUPOPUPCLASSIC_PACKED
static UBYTE menupopupclassic_imagedata[MENUPOPUPCLASSIC_WIDTH * MENUPOPUPCLASSIC_HEIGHT];
#else
#define menupopupclassic_imagedata menupopupclassic_data
#endif

#if MENUPULLDOWN3D_PACKED
static UBYTE menupulldown3d_imagedata[MENUPULLDOWN3D_WIDTH * MENUPULLDOWN3D_HEIGHT];
#else
#define menupulldown3d_imagedata menupulldown3d_data
#endif

#if MENUPULLDOWNCLASSIC_PACKED
static UBYTE menupulldownclassic_imagedata[MENUPULLDOWNCLASSIC_WIDTH * MENUPULLDOWNCLASSIC_HEIGHT];
#else
#define menupulldownclassic_imagedata menupulldownclassic_data
#endif

/*********************************************************************************************/

#define IPWindowObject BOOPSIOBJMACRO_START(IPWindow_CLASS->mcc_Class)

#define MUIM_IPWindow_Open    	(TAG_USER | 0x20000000)
#define MUIM_IPWindow_SaveAs  	(TAG_USER | 0x20000001)
#define MUIM_IPWindow_Default 	(TAG_USER | 0x20000002)
#define MUIM_IPWindow_LastSaved (TAG_USER | 0x20000003)

/*********************************************************************************************/

struct IPWindow_DATA
{
    Object  *menutypeobj;
    Object  *menulookobj;
    Object  *offscreenobj;
    Object  *defpubscrobj;
    BOOL    tested;
};

/*********************************************************************************************/

static struct MUI_CustomClass *IPWindow_CLASS;
static struct Hook  	       previewhook;
static STRPTR 	    	       menutype_labels[3], menulook_labels[3];
static WORD 	    	       imagetransparray[4];

/*********************************************************************************************/

#if MENUPOPUP3D_PACKED || MENUPOPUPCLASSIC_PACKED ||MENUPULLDOWN3D_PACKED || MENUPULLDOWNCLASSIC_PACKED

static UBYTE *unpack_byterun1(UBYTE *source, UBYTE *dest, LONG unpackedsize)
{
    UBYTE r;
    BYTE c;
    
    for(;;)
    {
	c = (BYTE)(*source++);
	if (c >= 0)
	{
    	    while(c-- >= 0)
	    {
		*dest++ = *source++;
		if (--unpackedsize <= 0) return source;
	    }
	}
	else if (c != -128)
	{
    	    c = -c;
	    r = *source++;

	    while(c-- >= 0)
	    {
		*dest++ = r;
		if (--unpackedsize <= 0) return source;
	    }
	}
    }
    
}

#endif

/*********************************************************************************************/

BOOL Gadgets2Prefs
(
    struct IControlPrefs *prefs, struct IPWindow_DATA *data
)
{
    IPTR active;
    
    get(data->menutypeobj, MUIA_Cycle_Active, &active);
    if (active == 0)
    {
    	prefs->ic_Flags &= ~ICF_POPUPMENUS;
    }
    else
    {
    	prefs->ic_Flags |= ICF_POPUPMENUS;
    }
    
    get(data->menulookobj, MUIA_Cycle_Active, &active);
    if (active == 0)
    {
    	prefs->ic_Flags &= ~ICF_3DMENUS;
    }
    else
    {
    	prefs->ic_Flags |= ICF_3DMENUS;
    }

    get(data->offscreenobj, MUIA_Selected, &active);
    if (active == 0)
    {
    	prefs->ic_Flags &= ~ICF_OFFSCREENLAYERS;
    }
    else
    {
    	prefs->ic_Flags |= ICF_OFFSCREENLAYERS;
    }

    get(data->defpubscrobj, MUIA_Selected, &active);
    if (active == 0)
    {
    	prefs->ic_Flags &= ~ICF_DEFPUBSCREEN;
    }
    else
    {
    	prefs->ic_Flags |= ICF_DEFPUBSCREEN;
    }

    prefs->ic_Flags |= ICF_AVOIDWINBORDERERASE;

    return TRUE;
}

/*********************************************************************************************/

BOOL Prefs2Gadgets
(
    struct IPWindow_DATA *data, struct IControlPrefs *prefs
)
{
    set(data->menutypeobj, MUIA_Cycle_Active, (prefs->ic_Flags & ICF_POPUPMENUS) ? 1 : 0);
    set(data->menulookobj, MUIA_Cycle_Active, (prefs->ic_Flags & ICF_3DMENUS) ? 1 : 0);
    set(data->offscreenobj, MUIA_Selected, (prefs->ic_Flags & ICF_OFFSCREENLAYERS) ? 1 : 0);
    set(data->defpubscrobj, MUIA_Selected, (prefs->ic_Flags & ICF_DEFPUBSCREEN) ? 1 : 0);
    return TRUE;
}

/*********************************************************************************************/

IPTR IPWindow__OM_NEW
(
    Class *CLASS, Object *self, struct opSet *message 
)
{
    struct IPWindow_DATA *data = NULL;
    Object *menu, *previewpage, *menutypeobj, *menulookobj;
    Object *offscreenobj;
    Object *defpubscrobj;

    extern struct NewMenu nm[];
    
    menu = MUI_MakeObject(MUIO_MenustripNM, nm, 0);
    
    /*
        WARNING: All Prefs structs must be initialized at this point!
    */
    
    menutype_labels[0] = MSG(MSG_MENUS_TYPE_PULLDOWN);
    menutype_labels[1] = MSG(MSG_MENUS_TYPE_POPUP);

    menulook_labels[0] = MSG(MSG_MENUS_LOOK_CLASSIC);
    menulook_labels[1] = MSG(MSG_MENUS_LOOK_3D);
    
    self = (Object *) DoSuperNewTags
    (
        CLASS, self, NULL,
        
        MUIA_Window_Title,    (IPTR)MSG(MSG_WINTITLE),
        MUIA_Window_Activate, TRUE,
        
        menu ? MUIA_Window_Menustrip : TAG_IGNORE, (IPTR)menu,
 
        WindowContents, (IPTR)HGroup,
	    Child, (IPTR)VGroup,
	    	MUIA_Weight, 0,
    		GroupFrameT(MSG(MSG_MENUS_GROUP)),
    		Child, ColGroup(2),
		    Child, Label1(MSG(MSG_MENUS_TYPE)),
		    Child, menutypeobj = MUI_MakeObject(MUIO_Cycle, NULL, menutype_labels),
		    Child, Label1(MSG(MSG_MENUS_LOOK)),
		    Child, menulookobj = MUI_MakeObject(MUIO_Cycle, NULL, menulook_labels),
		    End,
		Child, VSpace(1),
		Child, previewpage = PageGroup,
		    //ImageButtonFrame,
		    MUIA_Background, (IPTR)"2:6c6c6c6c,6a6a6a6a,b5b5b5b5",
		    Child, ChunkyImageObject,
	    		MUIA_ChunkyImage_Pixels, (IPTR)menupulldownclassic_imagedata,
			MUIA_ChunkyImage_Palette, (IPTR)menupulldownclassic_pal,
			MUIA_ChunkyImage_NumColors, MENUPULLDOWNCLASSIC_COLORS,
			MUIA_Bitmap_Width, MENUPULLDOWNCLASSIC_WIDTH,
			MUIA_Bitmap_Height, MENUPULLDOWNCLASSIC_HEIGHT,
			MUIA_FixWidth, MENUPULLDOWNCLASSIC_WIDTH,
			MUIA_FixHeight, MENUPULLDOWNCLASSIC_HEIGHT,
			MUIA_Bitmap_UseFriend, TRUE,
			MUIA_Bitmap_Transparent, imagetransparray[0],
	    		End,
		    Child, ChunkyImageObject,
	    		MUIA_ChunkyImage_Pixels, (IPTR)menupulldown3d_imagedata,
			MUIA_ChunkyImage_Palette, (IPTR)menupulldown3d_pal,
			MUIA_ChunkyImage_NumColors, MENUPULLDOWN3D_COLORS,
			MUIA_Bitmap_Width, MENUPULLDOWN3D_WIDTH,
			MUIA_Bitmap_Height, MENUPULLDOWN3D_HEIGHT,
			MUIA_FixWidth, MENUPULLDOWN3D_WIDTH,
			MUIA_FixHeight, MENUPULLDOWN3D_HEIGHT,
			MUIA_Bitmap_UseFriend, TRUE,
			MUIA_Bitmap_Transparent, imagetransparray[1],
	    		End,
		    Child, ChunkyImageObject,
	    		MUIA_ChunkyImage_Pixels, (IPTR)menupopupclassic_imagedata,
			MUIA_ChunkyImage_Palette, (IPTR)menupopupclassic_pal,
			MUIA_ChunkyImage_NumColors, MENUPOPUPCLASSIC_COLORS,
			MUIA_Bitmap_Width, MENUPOPUPCLASSIC_WIDTH,
			MUIA_Bitmap_Height, MENUPOPUPCLASSIC_HEIGHT,
			MUIA_FixWidth, MENUPOPUPCLASSIC_WIDTH,
			MUIA_FixHeight, MENUPOPUPCLASSIC_HEIGHT,
			MUIA_Bitmap_UseFriend, TRUE,
			MUIA_Bitmap_Transparent, imagetransparray[2],
	    		End,
		    Child, ChunkyImageObject,
	    		MUIA_ChunkyImage_Pixels, (IPTR)menupopup3d_imagedata,
			MUIA_ChunkyImage_Palette, (IPTR)menupopup3d_pal,
			MUIA_ChunkyImage_NumColors, MENUPOPUP3D_COLORS,
			MUIA_Bitmap_Width, MENUPOPUP3D_WIDTH,
			MUIA_Bitmap_Height, MENUPOPUP3D_HEIGHT,
			MUIA_FixWidth, MENUPOPUP3D_WIDTH,
			MUIA_FixHeight, MENUPOPUP3D_HEIGHT,
			MUIA_Bitmap_UseFriend, TRUE,
			MUIA_Bitmap_Transparent, imagetransparray[3],
	    		End,
    	    	    End,
		End,
	    Child, VGroup,
    		Child, VGroup,
    		    GroupFrameT("Windows"),
	            Child, VSpace(0),
    		    Child, ColGroup(4),
	                Child, HSpace(0),
    			Child, Label1("Offscreen move:"),
    			Child, offscreenobj = MUI_MakeObject(MUIO_Checkmark,
    							     NULL),
	                Child, HSpace(0),
    		        End,
	            Child, VSpace(0),
	            End,
    		Child, VGroup,
    		    GroupFrameT("Screens"),
	            Child, VSpace(0),
    		    Child, ColGroup(4),
	                Child, HSpace(0),
    			Child, Label1("Frontmost set as DefaultPubScreen:"),
    			Child, defpubscrobj = MUI_MakeObject(MUIO_Checkmark,
    						    NULL),
	                Child, HSpace(0),
    			End,
	            Child, VSpace(0),
    		    End,
	        End,
	    End,
       TAG_DONE
    );
    
    if (self == NULL) goto error;
    
    data = INST_DATA(CLASS, self);
    data->menutypeobj = menutypeobj;
    data->menulookobj = menulookobj;
    data->offscreenobj = offscreenobj;
    set(data->offscreenobj, MUIA_ShortHelp,
	(IPTR)"Allow windows to be moved out of\nthe visible area");
    data->defpubscrobj = defpubscrobj;
    set(data->defpubscrobj, MUIA_ShortHelp,
	(IPTR)"Make the frontmost public screen\nthe default public screen");

    DoMethod(menutypeobj, MUIM_Notify, MUIA_Cycle_Active, MUIV_EveryTime,
    	     (IPTR)previewpage, 3, MUIM_CallHook, (IPTR)&previewhook, (IPTR)data);
    DoMethod(menulookobj, MUIM_Notify, MUIA_Cycle_Active, MUIV_EveryTime,
    	     (IPTR)previewpage, 3, MUIM_CallHook, (IPTR)&previewhook, (IPTR)data);
	         
    Prefs2Gadgets(data, &icontrolprefs);

    return (IPTR) self;
    
error:
    
    return 0;
}

/*********************************************************************************************/

IPTR IPWindow__MUIM_PrefsWindow_Test
(     
    Class *CLASS, Object *self, Msg message 
)
{
    struct IPWindow_DATA *data = INST_DATA(CLASS, self);
    
    Gadgets2Prefs(&icontrolprefs, data);
    SavePrefs(CONFIGNAME_ENV);
    
    data->tested = TRUE;
    
    return 0;
}

/*********************************************************************************************/

IPTR IPWindow__MUIM_PrefsWindow_Revert
(     
    Class *CLASS, Object *self, Msg message 
)
{
    struct IPWindow_DATA *data = INST_DATA(CLASS, self);
    
    RestorePrefs();
    Prefs2Gadgets(data, &icontrolprefs);
    
    return 0;
}

/*********************************************************************************************/

IPTR IPWindow__MUIM_PrefsWindow_Save
(     
    Class *CLASS, Object *self, Msg message 
)
{
    struct IPWindow_DATA *data = INST_DATA(CLASS, self);
    
    Gadgets2Prefs(&icontrolprefs, data);
    SavePrefs(CONFIGNAME_ENVARC);
    SavePrefs(CONFIGNAME_ENV);
    
    SetAttrs(self, MUIA_Window_Open, FALSE, TAG_DONE);
    DoMethod(_app(self), MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);
    
    return 0;
}

/*********************************************************************************************/

IPTR IPWindow__MUIM_PrefsWindow_Use
(     
    Class *CLASS, Object *self, Msg message 
)
{
    struct IPWindow_DATA *data = INST_DATA(CLASS, self);
    
    Gadgets2Prefs(&icontrolprefs, data);
    SavePrefs(CONFIGNAME_ENV);
    
    SetAttrs(self, MUIA_Window_Open, FALSE, TAG_DONE);
    DoMethod(_app(self), MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);
    
    return 0;
}

/*********************************************************************************************/

IPTR IPWindow__MUIM_PrefsWindow_Cancel
(     
    Class *CLASS, Object *self, Msg message 
)
{
    struct IPWindow_DATA *data = INST_DATA(CLASS, self);

    if (data->tested)
    {
    	RestorePrefs();
	SavePrefs(CONFIGNAME_ENV);
    }
    
    SetAttrs(self, MUIA_Window_Open, FALSE, TAG_DONE);
    DoMethod(_app(self), MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);
    
    return 0;
}

/*********************************************************************************************/

IPTR IPWindow__MUIM_IPWindow_Open
(     
    Class *CLASS, Object *self, Msg message 
)
{
    struct IPWindow_DATA *data = INST_DATA(CLASS, self);
    STRPTR filename;
    
    if ((filename = GetFile(MSG(MSG_ASL_OPEN_TITLE), "SYS:Prefs/Presets", FALSE)))
    {
    	if (LoadPrefs(filename))
	{
	    Prefs2Gadgets(data, &icontrolprefs);
	}
    }
    
    return 0;
}

/*********************************************************************************************/

IPTR IPWindow__MUIM_IPWindow_SaveAs
(     
    Class *CLASS, Object *self, Msg message 
)
{
    STRPTR filename;
    
    if ((filename = GetFile(MSG(MSG_ASL_SAVE_TITLE), "SYS:Prefs/Presets", TRUE)))
    {
    	SavePrefs(filename);
    }
        
    return 0;
}

/*********************************************************************************************/

IPTR IPWindow__MUIM_IPWindow_Default
(     
    Class *CLASS, Object *self, Msg message 
)
{
    struct IPWindow_DATA *data = INST_DATA(CLASS, self);
        
    DefaultPrefs();
    Prefs2Gadgets(data, &icontrolprefs);
    
    return 0;
}

/*********************************************************************************************/

IPTR IPWindow__MUIM_IPWindow_LastSaved
(     
    Class *CLASS, Object *self, Msg message 
)
{
    struct IPWindow_DATA *data = INST_DATA(CLASS, self);
        
    if (LoadPrefs(CONFIGNAME_ENVARC))
    {
    	Prefs2Gadgets(data, &icontrolprefs);
    }
    
    return 0;
}

/*********************************************************************************************/

BOOPSI_DISPATCHER(IPTR, IPWindow_Dispatcher, CLASS, self, message)
{
    switch (message->MethodID)
    {
        case OM_NEW: 
            return IPWindow__OM_NEW(CLASS, self, (struct opSet *) message);
        
        case MUIM_PrefsWindow_Test:   
            return IPWindow__MUIM_PrefsWindow_Test(CLASS, self, message);
        
        case MUIM_PrefsWindow_Revert:
            return IPWindow__MUIM_PrefsWindow_Revert(CLASS, self, message);
        
        case MUIM_PrefsWindow_Save:
            return IPWindow__MUIM_PrefsWindow_Save(CLASS, self, message);
        
        case MUIM_PrefsWindow_Use:
            return IPWindow__MUIM_PrefsWindow_Use(CLASS, self, message);
        
        case MUIM_PrefsWindow_Cancel:
            return IPWindow__MUIM_PrefsWindow_Cancel(CLASS, self, message);
        
   	case MUIM_IPWindow_Open:
	    return IPWindow__MUIM_IPWindow_Open(CLASS, self, message);

    	case MUIM_IPWindow_SaveAs:
	    return IPWindow__MUIM_IPWindow_SaveAs(CLASS, self, message);

    	case MUIM_IPWindow_Default:
	    return IPWindow__MUIM_IPWindow_Default(CLASS, self, message);

    	case MUIM_IPWindow_LastSaved:
	    return IPWindow__MUIM_IPWindow_LastSaved(CLASS, self, message);

        default:     
            return DoSuperMethodA(CLASS, self, message);
    }
    
    return 0;
}
BOOPSI_DISPATCHER_END

/*********************************************************************************************/

static BOOL IPWindow_Initialize()
{
    IPWindow_CLASS = MUI_CreateCustomClass
    (
        NULL, MUIC_PrefsWindow, NULL, 
        sizeof(struct IPWindow_DATA), IPWindow_Dispatcher
    );

    if (IPWindow_CLASS != NULL)
        return TRUE;
    else
        return FALSE;
}
/*********************************************************************************************/

static void IPWindow_Deinitialize()
{
    if (IPWindow_CLASS) MUI_DeleteCustomClass(IPWindow_CLASS);
}

/*********************************************************************************************/

static void InitImagePal(ULONG *pal, WORD numcols, WORD index)
{
    WORD i;
    
    imagetransparray[index] = -1;
    for(i = 0; i < numcols; i++)
    {
    	if ((pal[i] & 0xFCFCFC) == 0xFC00FC)
	{
	    imagetransparray[index] = i;
	}

	((UBYTE *)pal)[i * 3 + 0] = pal[i] >> 16;
	((UBYTE *)pal)[i * 3 + 1] = pal[i] >> 8;
	((UBYTE *)pal)[i * 3 + 2] = pal[i];
	
    }
}

/*********************************************************************************************/

static void InitImages(void)
{
    #if MENUPOPUP3D_PACKED
    unpack_byterun1(menupopup3d_data, menupopup3d_imagedata, sizeof(menupopup3d_imagedata));
    #endif

    #if MENUPOPUPCLASSIC_PACKED
    unpack_byterun1(menupopupclassic_data, menupopupclassic_imagedata, sizeof(menupopupclassic_imagedata));
    #endif

    #if MENUPULLDOWN3D_PACKED
    unpack_byterun1(menupulldown3d_data, menupulldown3d_imagedata, sizeof(menupulldown3d_imagedata));
    #endif

    #if MENUPULLDOWNCLASSIC_PACKED
    unpack_byterun1(menupulldownclassic_data, menupulldownclassic_imagedata, sizeof(menupulldownclassic_imagedata));
    #endif
    
    InitImagePal(menupulldownclassic_pal, MENUPULLDOWNCLASSIC_COLORS, 0);
    InitImagePal(menupulldown3d_pal, MENUPULLDOWN3D_COLORS, 1);
    InitImagePal(menupopupclassic_pal, MENUPOPUPCLASSIC_COLORS, 2);
    InitImagePal(menupopup3d_pal, MENUPOPUP3D_COLORS, 3);
    
}

/*********************************************************************************************/

static void PreviewFunc(struct Hook *hook, Object *previewpage, struct IPWindow_DATA **data)
{
    IPTR type, look;
    
    get((*data)->menutypeobj, MUIA_Cycle_Active, &type);
    get((*data)->menulookobj, MUIA_Cycle_Active, &look);
    
    nnset(previewpage, MUIA_Group_ActivePage, type * 2 + look);   
}

/*********************************************************************************************/

void MakeGUI(void)
{
    if (!IPWindow_Initialize()) Cleanup("Error creating custom preferenceswindow class!");
    
    InitImages();
    
    previewhook.h_Entry = HookEntry;
    previewhook.h_SubEntry = (HOOKFUNC)PreviewFunc;
    
    app = ApplicationObject,
	MUIA_Application_Title, (IPTR)"IControl",
	MUIA_Application_Version, (IPTR)VERSIONSTR,
	MUIA_Application_Copyright, (IPTR)"Copyright © 1995-2003, The AROS Development Team",
	MUIA_Application_Author, (IPTR)"The AROS Development Team",
	MUIA_Application_Description, (IPTR)MSG(MSG_WINTITLE),
	MUIA_Application_Base, (IPTR)"Icontrol",
  	SubWindow, (IPTR) (wnd = NewObject(IPWindow_CLASS->mcc_Class, NULL, TAG_DONE)),
	End;

    if (!app) Cleanup(MSG(MSG_CANT_CREATE_APP));

    DoMethod(wnd, MUIM_Notify, MUIA_Window_CloseRequest, TRUE, (IPTR) app, 2, MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);

    DoMethod(wnd, MUIM_Notify, MUIA_Window_MenuAction, MSG_MEN_PROJECT_OPEN, (IPTR) wnd, 1, MUIM_IPWindow_Open);    
    DoMethod(wnd, MUIM_Notify, MUIA_Window_MenuAction, MSG_MEN_PROJECT_SAVEAS, (IPTR) wnd, 1, MUIM_IPWindow_SaveAs);    
    DoMethod(wnd, MUIM_Notify, MUIA_Window_MenuAction, MSG_MEN_PROJECT_QUIT, (IPTR) app, 2, MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);
    DoMethod(wnd, MUIM_Notify, MUIA_Window_MenuAction, MSG_MEN_EDIT_DEFAULT, (IPTR) wnd, 1, MUIM_IPWindow_Default);    
    DoMethod(wnd, MUIM_Notify, MUIA_Window_MenuAction, MSG_MEN_EDIT_LASTSAVED, (IPTR) wnd, 1, MUIM_IPWindow_LastSaved);    
    DoMethod(wnd, MUIM_Notify, MUIA_Window_MenuAction, MSG_MEN_EDIT_RESTORE, (IPTR) wnd, 1, MUIM_PrefsWindow_Revert);

}

/*********************************************************************************************/

void KillGUI(void)
{
    DisposeObject(app);
    IPWindow_Deinitialize();
}
