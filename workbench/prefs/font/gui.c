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

#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>
#include <proto/utility.h>

#include <zune/preferenceswindow.h>

#include <stdio.h>
#include <string.h>

#include "locale.h"
#include "prefs.h"

extern struct FontPrefs *fontPrefs[3]; // prefs.c

#define BUFFERSIZE 512

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
    STRPTR separator    = PathPart(buffer);
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
        nameLength, buffer
    ); 
    fp->fp_TextAttr.ta_Name = fp->fp_Name;
    
    StrToLong(FilePart(buffer), &size);
    fp->fp_TextAttr.ta_YSize = size;

    return TRUE;
}

BOOL Gadgets2FontPrefs(struct FontPrefs **fps, struct FPWindow_DATA *data)
{
    STRPTR str = NULL;
    
    // FIXME: error checking
    get(data->fpwd_IconsString, MUIA_Text_Contents, &str);
    FontString2FontPrefs(fps[FP_WBFONT], str);
    
    get(data->fpwd_SystemString, MUIA_Text_Contents, &str);
    FontString2FontPrefs(fps[FP_SYSFONT], str);
    
    get(data->fpwd_ScreenString, MUIA_Text_Contents, &str);
    FontString2FontPrefs(fps[FP_SCREENFONT], str);
    
    return TRUE;
}


/*** Methods ****************************************************************/
IPTR FPWindow$OM_NEW
(
    struct IClass *CLASS, Object *self, struct opSet *message 
)
{
    struct FPWindow_DATA *data = NULL;
    Object               *iconsString, *screenString, *systemString;
    TEXT                  buffer[BUFFERSIZE];
        
    struct TagItem tags[] =
    {
        { MUIA_Window_Title,     MSG(MSG_WINDOW_TITLE) },
        { MUIA_Window_Activate,  TRUE          },
        { MUIA_Window_Menustrip, NULL          }, /* set later */
        { WindowContents,        NULL          }, /* set later */
        { TAG_DONE,              NULL          }
    };
    
    /*
        WARNING: All FontPrefs structs must be initialized at this point!
    */
    
    tags[2].ti_Data = (IPTR) MenustripObject,
        Child, MenuObject,
            MUIA_Menu_Title, "Preferences",
            
            Child, MenuitemObject,
                MUIA_Menuitem_Title,    "Import...",
                MUIA_Menuitem_Shortcut, "I",
            End,            
            Child, MenuitemObject,
                MUIA_Menuitem_Title,    "Export...",
                MUIA_Menuitem_Shortcut, "E",
            End,
        End,
    End;
    
    tags[3].ti_Data = (IPTR) ColGroup(2),
        Child, Label2(MSG(MSG_ICONS)),
        Child, PopaslObject,
            MUIA_Popasl_Type,              ASL_FontRequest,
            MUIA_Popstring_String,  (IPTR) iconsString = TextObject, 
                TextFrame,
            End,
            MUIA_Popstring_Button,  (IPTR) PopButton(MUII_PopUp),
        End,
        Child, Label2(MSG(MSG_SCREEN)),
        Child, PopaslObject,
            MUIA_Popasl_Type,              ASL_FontRequest,
            MUIA_Popstring_String,  (IPTR) screenString = TextObject, 
                TextFrame,
            End,
            MUIA_Popstring_Button,  (IPTR) PopButton(MUII_PopUp),
        End,
        Child, Label2(MSG(MSG_SYSTEM)),
        Child, PopaslObject,
            MUIA_Popasl_Type,              ASL_FontRequest,
            MUIA_Popstring_String,  (IPTR) systemString = TextObject, 
                TextFrame,
            End,
            MUIA_Popstring_Button,  (IPTR) PopButton(MUII_PopUp),
        End,
    End;
        
    message->ops_AttrList = tags;
          
    self = (Object *) DoSuperMethodA(CLASS, self, (Msg) message);
    if (self == NULL) return FALSE;
    
    FontPrefs2FontString(buffer, BUFFERSIZE, fontPrefs[FP_WBFONT]);
    SetAttrs(iconsString, MUIA_Text_Contents, (IPTR) buffer, TAG_DONE);
    
    FontPrefs2FontString(buffer, BUFFERSIZE, fontPrefs[FP_SYSFONT]);
    SetAttrs(systemString, MUIA_Text_Contents, (IPTR) buffer, TAG_DONE);
    
    FontPrefs2FontString(buffer, BUFFERSIZE, fontPrefs[FP_SCREENFONT]);
    SetAttrs(screenString, MUIA_Text_Contents, (IPTR) buffer, TAG_DONE);
    
    data = INST_DATA(CLASS, self);
    data->fpwd_IconsString  = iconsString;
    data->fpwd_SystemString = systemString;
    data->fpwd_ScreenString = screenString;
    
    return (IPTR) self;
}

IPTR FPWindow$MUIM_PreferencesWindow_Test
(     
    struct IClass *CLASS, Object *self, Msg message 
)
{
    struct FPWindow_DATA *data = INST_DATA(CLASS, self);
    
    return NULL;
}

IPTR FPWindow$MUIM_PreferencesWindow_Revert
(     
    struct IClass *CLASS, Object *self, Msg message 
)
{
    struct FPWindow_DATA *data = INST_DATA(CLASS, self);
    
    return NULL;
}

IPTR FPWindow$MUIM_PreferencesWindow_Save
(     
    struct IClass *CLASS, Object *self, Msg message 
)
{
    struct FPWindow_DATA *data = INST_DATA(CLASS, self);
    
    Gadgets2FontPrefs(fontPrefs, data);
    WritePrefs("ENV:sys/font.prefs", fontPrefs);
    WritePrefs("ENVARC:sys/font.prefs", fontPrefs);
        
    SetAttrs(self, MUIA_Window_Open, FALSE, TAG_DONE);
    DoMethod(_app(self), MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);
    
    return NULL;
}

IPTR FPWindow$MUIM_PreferencesWindow_Use
(     
    struct IClass *CLASS, Object *self, Msg message 
)
{
    struct FPWindow_DATA *data = INST_DATA(CLASS, self);
    
    Gadgets2FontPrefs(fontPrefs, data);
    WritePrefs("ENV:sys/font.prefs", fontPrefs);
    
    SetAttrs(self, MUIA_Window_Open, FALSE, TAG_DONE);
    DoMethod(_app(self), MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);
    
    return NULL;
}

IPTR FPWindow$MUIM_PreferencesWindow_Cancel
(     
    struct IClass *CLASS, Object *self, Msg message 
)
{
    struct FPWindow_DATA *data = INST_DATA(CLASS, self);
    
    SetAttrs(self, MUIA_Window_Open, FALSE, TAG_DONE);
    DoMethod(_app(self), MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);
    
    return NULL;
}

/*** Dispatcher *************************************************************/
BOOPSI_DISPATCHER(IPTR, FPWindow_Dispatcher, CLASS, self, message)
{
    switch (message->MethodID)
    {
        case OM_NEW: 
            return FPWindow$OM_NEW(CLASS, self, (struct opSet *) message);
        
        case MUIM_PreferencesWindow_Test:   
            return FPWindow$MUIM_PreferencesWindow_Test(CLASS, self, message);
        
        case MUIM_PreferencesWindow_Revert:
            return FPWindow$MUIM_PreferencesWindow_Revert(CLASS, self, message);
        
        case MUIM_PreferencesWindow_Save:
            return FPWindow$MUIM_PreferencesWindow_Save(CLASS, self, message);
        
        case MUIM_PreferencesWindow_Use:
            return FPWindow$MUIM_PreferencesWindow_Use(CLASS, self, message);
        
        case MUIM_PreferencesWindow_Cancel:
            return FPWindow$MUIM_PreferencesWindow_Cancel(CLASS, self, message);
        
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
        NULL, MUIC_PreferencesWindow, NULL, 
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
