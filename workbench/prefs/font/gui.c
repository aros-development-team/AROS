/*
    Copyright © 2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#define MUIMASTER_YES_INLINE_STDARG

#include <exec/types.h>
#include <utility/tagitem.h>
#include <libraries/mui.h>
#include <libraries/asl.h>
#include <libraries/gadtools.h>
#include <prefs/font.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/muimaster.h>

#include <zune/prefswindow.h>

#include <stdio.h>
#include <string.h>

#include "locale.h"
#include "prefs.h"
#include "asl.h"

#define BUFFERSIZE 512

/*** Private methods ********************************************************/
#define MUIM_FPWindow_Import (TAG_USER | 0x20000000)
#define MUIM_FPWindow_Export (TAG_USER | 0x20000001)

/*** Instance data **********************************************************/
struct FPWindow_DATA
{
    Object *fpwd_IconsString, 
           *fpwd_ScreenString, 
           *fpwd_SystemString;
};

/*** Utility functions ******************************************************/

void FontPrefs2FontString
(
    STRPTR buffer, ULONG buffersize, struct FontPrefs *fp
)
{
    snprintf
    (
        buffer, BUFFERSIZE, "%.*s/%d", 
        strlen(fp->fp_TextAttr.ta_Name) - strlen(".font"), 
        fp->fp_TextAttr.ta_Name, fp->fp_TextAttr.ta_YSize
    );
}

BOOL FontString2FontPrefs(struct FontPrefs *fp, CONST_STRPTR buffer)
{
    STRPTR separator    = PathPart((STRPTR)buffer);
    ULONG  nameLength   = separator - buffer;
    ULONG  suffixLength = strlen(".font");
    ULONG  size;
    
    if (nameLength + suffixLength >= FONTNAMESIZE)
    {
        /* Not enough space for the font name */
        return FALSE;
    }
    
    snprintf
    (
        fp->fp_Name, nameLength + suffixLength + 1, "%.*s.font", 
        (int) nameLength, buffer
    ); 
    fp->fp_TextAttr.ta_Name = fp->fp_Name;
    
    StrToLong(FilePart((STRPTR)buffer), &size);
    fp->fp_TextAttr.ta_YSize = size;

    return TRUE;
}

BOOL Gadgets2FontPrefs
(
    struct FontPrefs *fp[FP_COUNT], struct FPWindow_DATA *data
)
{
    STRPTR str = NULL;
    
    // FIXME: error checking
    get(data->fpwd_IconsString, MUIA_Text_Contents, &str);
    FontString2FontPrefs(fp[FP_WBFONT], str);
    
    get(data->fpwd_SystemString, MUIA_Text_Contents, &str);
    FontString2FontPrefs(fp[FP_SYSFONT], str);
    
    get(data->fpwd_ScreenString, MUIA_Text_Contents, &str);
    FontString2FontPrefs(fp[FP_SCREENFONT], str);
    
    return TRUE;
}

BOOL FontPrefs2Gadgets
(
    struct FPWindow_DATA *data, struct FontPrefs *fp[FP_COUNT]
)
{
    TEXT buffer[FONTNAMESIZE + 8];
     
    // FIXME: error checking
    FontPrefs2FontString(buffer, FONTNAMESIZE + 8, fp[FP_WBFONT]);
    set(data->fpwd_IconsString, MUIA_Text_Contents, (IPTR)buffer);
    
    FontPrefs2FontString(buffer, FONTNAMESIZE + 8, fp[FP_SYSFONT]);
    set(data->fpwd_SystemString, MUIA_Text_Contents, (IPTR)buffer);
    
    FontPrefs2FontString(buffer, FONTNAMESIZE + 8, fp[FP_SCREENFONT]);
    set(data->fpwd_ScreenString, MUIA_Text_Contents, (IPTR)buffer);
    
    return TRUE;
}

Object *makeMenuitem(CONST_STRPTR text)
{
    CONST_STRPTR title    = NULL, 
                 shortcut = NULL;
    
    if (text[1] == '\0')
    {
        title    = text + 2;
        shortcut = text;
    }
    else
    {
        title    = text;
        shortcut = NULL;
    }
    
    return MenuitemObject,
        MUIA_Menuitem_Title,      (IPTR) title,
        shortcut != NULL       ?
        MUIA_Menuitem_Shortcut :
        TAG_IGNORE,               (IPTR) shortcut,
    End;
}

/*** Methods ****************************************************************/
IPTR FPWindow__OM_NEW
(
    Class *CLASS, Object *self, struct opSet *message 
)
{
    struct FPWindow_DATA *data = NULL;
    Object               *iconsString, *screenString, *systemString;
    Object               *importMI, *exportMI; /* menu items */
    
    /*
        WARNING: All FontPrefs structs must be initialized at this point!
    */
    
    self = (Object *) DoSuperNewTags
    (
        CLASS, self, NULL,
        
        MUIA_Window_Title,    __(MSG_WINDOW_TITLE),
        MUIA_Window_Activate, TRUE,
        
        MUIA_Window_Menustrip, (IPTR) MenustripObject,
            Child, (IPTR) MenuObject,
                MUIA_Menu_Title, __(MSG_MENU_PREFERENCES),
                
                Child, (IPTR) importMI = makeMenuitem(_(MSG_MENU_PREFERENCES_IMPORT)),
                Child, (IPTR) exportMI = makeMenuitem(_(MSG_MENU_PREFERENCES_EXPORT)),
            End,
        End, 
        
        WindowContents, (IPTR) ColGroup(2),
            Child, (IPTR) Label2(_(MSG_ICONS)),
            Child, (IPTR) PopaslObject,
                MUIA_Popasl_Type,              ASL_FontRequest,
		ASLFO_MaxHeight,               100,
                MUIA_Popstring_String,  (IPTR) iconsString = TextObject, 
                    TextFrame,
		    MUIA_Background, MUII_TextBack,
                End,
                MUIA_Popstring_Button,  (IPTR) PopButton(MUII_PopUp),
            End,
            Child, (IPTR) Label2(_(MSG_SCREEN)),
            Child, (IPTR) PopaslObject,
                MUIA_Popasl_Type,              ASL_FontRequest,
		ASLFO_MaxHeight,               100,
                MUIA_Popstring_String,  (IPTR) screenString = TextObject, 
                    TextFrame,
		    MUIA_Background, MUII_TextBack,
                End,
                MUIA_Popstring_Button,  (IPTR) PopButton(MUII_PopUp),
            End,
            Child, (IPTR) Label2(_(MSG_SYSTEM)),
            Child, (IPTR) PopaslObject,
		MUIA_Popasl_Type,              ASL_FontRequest,
                ASLFO_FixedWidthOnly,          TRUE,
		ASLFO_MaxHeight,               100,
                MUIA_Popstring_String,  (IPTR) systemString = TextObject, 
                    TextFrame,
		    MUIA_Background, MUII_TextBack,
                End,
                MUIA_Popstring_Button,  (IPTR) PopButton(MUII_PopUp),
            End,
        End,
        
        TAG_DONE
    );
    
    if (self == NULL) goto error;
    
    data = INST_DATA(CLASS, self);
    data->fpwd_IconsString  = iconsString;
    data->fpwd_SystemString = systemString;
    data->fpwd_ScreenString = screenString;
    
    FontPrefs2Gadgets(data, fp_Current);

    DoMethod
    (
        importMI, MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime,
        (IPTR) self, 1, MUIM_FPWindow_Import
    );
    
    DoMethod
    (
        exportMI, MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime,
        (IPTR) self, 1, MUIM_FPWindow_Export
    );
    
    return (IPTR) self;
    
error:
    
    return NULL;
}

IPTR FPWindow__MUIM_PrefsWindow_Test
(     
    Class *CLASS, Object *self, Msg message 
)
{
    struct FPWindow_DATA *data = INST_DATA(CLASS, self);
    
    Gadgets2FontPrefs(fp_Current, data);
    FP_Test(); /* FIXME: check error? */
    
    return NULL;
}

IPTR FPWindow__MUIM_PrefsWindow_Revert
(     
    Class *CLASS, Object *self, Msg message 
)
{
    struct FPWindow_DATA *data = INST_DATA(CLASS, self);
    
    FP_Revert(); /* FIXME: check error? */
    FontPrefs2Gadgets(data, fp_Current);
    
    return NULL;
}

IPTR FPWindow__MUIM_PrefsWindow_Save
(     
    Class *CLASS, Object *self, Msg message 
)
{
    struct FPWindow_DATA *data = INST_DATA(CLASS, self);
    
    Gadgets2FontPrefs(fp_Current, data);
    FP_Save(); /* FIXME: check error? */
    
    SetAttrs(self, MUIA_Window_Open, FALSE, TAG_DONE);
    DoMethod(_app(self), MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);
    
    return NULL;
}

IPTR FPWindow__MUIM_PrefsWindow_Use
(     
    Class *CLASS, Object *self, Msg message 
)
{
    struct FPWindow_DATA *data = INST_DATA(CLASS, self);
    
    Gadgets2FontPrefs(fp_Current, data);
    FP_Use(); /* FIXME: check error? */
    
    SetAttrs(self, MUIA_Window_Open, FALSE, TAG_DONE);
    DoMethod(_app(self), MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);
    
    return NULL;
}

IPTR FPWindow__MUIM_PrefsWindow_Cancel
(     
    Class *CLASS, Object *self, Msg message 
)
{
    FP_Cancel(); /* FIXME: check error? */
    
    SetAttrs(self, MUIA_Window_Open, FALSE, TAG_DONE);
    DoMethod(_app(self), MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);
    
    return NULL;
}

IPTR FPWindow__MUIM_FPWindow_Import
(     
    Class *CLASS, Object *self, Msg message 
)
{
    struct FPWindow_DATA *data = INST_DATA(CLASS, self);
    
    STRPTR filename = ASL_SelectFile(ASL_MODE_IMPORT);
    if (filename != NULL)
    {
        FP_LoadFrom(filename); /* FIXME: check error? */
        FontPrefs2Gadgets(data, fp_Current);
        FreeVec(filename);
    }
    
    return NULL;
}

IPTR FPWindow__MUIM_FPWindow_Export
(     
    Class *CLASS, Object *self, Msg message 
)
{
    struct FPWindow_DATA *data = INST_DATA(CLASS, self);
    
    STRPTR filename = ASL_SelectFile(ASL_MODE_EXPORT);
    if (filename != NULL)
    {
        Gadgets2FontPrefs(fp_Current, data);
        FP_SaveTo(filename); /* FIXME: check error? */
        FreeVec(filename);
    }
    
    return NULL;
}

/*** Dispatcher *************************************************************/
BOOPSI_DISPATCHER(IPTR, FPWindow_Dispatcher, CLASS, self, message)
{
    switch (message->MethodID)
    {
        case OM_NEW: 
            return FPWindow__OM_NEW(CLASS, self, (struct opSet *) message);
        
        case MUIM_PrefsWindow_Test:   
            return FPWindow__MUIM_PrefsWindow_Test(CLASS, self, message);
        
        case MUIM_PrefsWindow_Revert:
            return FPWindow__MUIM_PrefsWindow_Revert(CLASS, self, message);
        
        case MUIM_PrefsWindow_Save:
            return FPWindow__MUIM_PrefsWindow_Save(CLASS, self, message);
        
        case MUIM_PrefsWindow_Use:
            return FPWindow__MUIM_PrefsWindow_Use(CLASS, self, message);
        
        case MUIM_PrefsWindow_Cancel:
            return FPWindow__MUIM_PrefsWindow_Cancel(CLASS, self, message);
        
        case MUIM_FPWindow_Import:
            return FPWindow__MUIM_FPWindow_Import(CLASS, self, message);
        
        case MUIM_FPWindow_Export:
            return FPWindow__MUIM_FPWindow_Export(CLASS, self, message);
        
        default:     
            return DoSuperMethodA(CLASS, self, message);
    }
    
    return NULL;
}

/*** Setup ******************************************************************/
struct MUI_CustomClass *FPWindow_CLASS;

BOOL FPWindow_Initialize()
{
    FPWindow_CLASS = MUI_CreateCustomClass
    (
        NULL, MUIC_PrefsWindow, NULL, 
        sizeof(struct FPWindow_DATA), FPWindow_Dispatcher
    );

    if (FPWindow_CLASS != NULL)
        return TRUE;
    else
        return FALSE;
}

void FPWindow_Deinitialize()
{
    MUI_DeleteCustomClass(FPWindow_CLASS);
}
