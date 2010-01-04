/*
    Copyright © 2003-2004, The AROS Development Team. All rights reserved.
    $Id$
*/

#define MUIMASTER_YES_INLINE_STDARG

#include <libraries/asl.h>
#include <libraries/mui.h>
#include <zune/customclasses.h>
#include <zune/prefseditor.h>

#include <proto/intuition.h>
#include <proto/dos.h>

#include <string.h>
#include <stdio.h>

#include "misc.h"
#include "locale.h"
#include "fpeditor.h"
#include "prefs.h"

/*** Instance Data **********************************************************/
struct FPEditor_DATA
{
    struct FontPrefs  fped_FontPrefs[FP_COUNT];
    Object           *fped_IconsString,
                     *fped_ScreenString,
                     *fped_SystemString;
};

/*** Macros *****************************************************************/
#define SETUP_INST_DATA struct FPEditor_DATA *data = INST_DATA(CLASS, self)
#define FP(i) (&(data->fped_FontPrefs[(i)]))

/*** Utility Functions ******************************************************/
static VOID FontPrefs2FontString
(
    STRPTR buffer, ULONG buffersize, struct FontPrefs *fp
)
{
    snprintf
    (
        buffer, buffersize, "%.*s/%d",
        strlen(fp->fp_TextAttr.ta_Name) - 5 /* strlen(".font") */,
        fp->fp_TextAttr.ta_Name, fp->fp_TextAttr.ta_YSize
    );
}

static BOOL FontString2FontPrefs(struct FontPrefs *fp, CONST_STRPTR buffer)
{
    STRPTR separator    = PathPart((STRPTR) buffer);
    ULONG  nameLength   = separator - buffer;
    ULONG  suffixLength = 5; /* strlen(".font") */
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

    StrToLong(FilePart((STRPTR) buffer), &size);
    fp->fp_TextAttr.ta_YSize = size;

    return TRUE;
}

static BOOL Gadgets2FontPrefs
(
    struct FPEditor_DATA *data
)
{
    STRPTR str = NULL;

    // FIXME: error checking
    GET(data->fped_IconsString, MUIA_String_Contents, &str);
    FontString2FontPrefs(FP(FP_WBFONT), str);
    FP(FP_WBFONT)->fp_Type = FP_WBFONT;

    GET(data->fped_SystemString, MUIA_String_Contents, &str);
    FontString2FontPrefs(FP(FP_SYSFONT), str);
    FP(FP_SYSFONT)->fp_Type = FP_SYSFONT;

    GET(data->fped_ScreenString, MUIA_String_Contents, &str);
    FontString2FontPrefs(FP(FP_SCREENFONT), str);
    FP(FP_SCREENFONT)->fp_Type = FP_SCREENFONT;

    return TRUE;
}

static BOOL FontPrefs2Gadgets
(
    struct FPEditor_DATA *data
)
{
    TEXT buffer[FONTNAMESIZE + 8];

    // FIXME: error checking
    FontPrefs2FontString(buffer, FONTNAMESIZE + 8, FP(FP_WBFONT));
    NNSET(data->fped_IconsString, MUIA_String_Contents, (IPTR) buffer);

    FontPrefs2FontString(buffer, FONTNAMESIZE + 8, FP(FP_SYSFONT));
    NNSET(data->fped_SystemString, MUIA_String_Contents, (IPTR) buffer);

    FontPrefs2FontString(buffer, FONTNAMESIZE + 8, FP(FP_SCREENFONT));
    NNSET(data->fped_ScreenString, MUIA_String_Contents, (IPTR) buffer);

    return TRUE;
}

/*** Methods ****************************************************************/
static Object *FPEditor__OM_NEW(Class *CLASS, Object *self, struct opSet *message)
{
    Object *iconsString, *screenString, *systemString;

    self = (Object *) DoSuperNewTags
    (
        CLASS, self, NULL,

        MUIA_PrefsEditor_Name,        __(MSG_NAME),
        MUIA_PrefsEditor_Path, (IPTR) "SYS/font.prefs",
        MUIA_PrefsEditor_IconTool, (IPTR) "SYS:Prefs/Font",

        Child, (IPTR) ColGroup(2),
            Child, (IPTR) Label2(_(MSG_ICONS)),
            Child, (IPTR) PopaslObject,
                MUIA_Popasl_Type,              ASL_FontRequest,
                ASLFO_MaxHeight,               100,
                MUIA_Popstring_String,  (IPTR) (iconsString = (Object *)StringObject,
                    TextFrame,
                    MUIA_Background, MUII_TextBack,
                End),
                MUIA_Popstring_Button,  (IPTR) PopButton(MUII_PopUp),
            End,
            Child, (IPTR) Label2(_(MSG_SCREEN)),
            Child, (IPTR) PopaslObject,
                MUIA_Popasl_Type,              ASL_FontRequest,
                ASLFO_MaxHeight,               100,
                MUIA_Popstring_String,  (IPTR) (screenString = (Object *)StringObject,
                    TextFrame,
                    MUIA_Background, MUII_TextBack,
                End),
                MUIA_Popstring_Button,  (IPTR) PopButton(MUII_PopUp),
            End,
            Child, (IPTR) Label2(_(MSG_SYSTEM)),
            Child, (IPTR) PopaslObject,
                MUIA_Popasl_Type,              ASL_FontRequest,
                ASLFO_FixedWidthOnly,          TRUE,
                ASLFO_MaxHeight,               100,
                MUIA_Popstring_String,  (IPTR) (systemString = (Object *)StringObject,
                    TextFrame,
                    MUIA_Background, MUII_TextBack,
                End),
                MUIA_Popstring_Button,  (IPTR) PopButton(MUII_PopUp),
            End,
        End,

        TAG_DONE
    );

    if (self != NULL)
    {
        SETUP_INST_DATA;
        data->fped_IconsString  = iconsString;
        data->fped_ScreenString = screenString;
        data->fped_SystemString = systemString;

        /*-- Setup notifications -------------------------------------------*/
        DoMethod
        (
            iconsString, MUIM_Notify, MUIA_String_Acknowledge, MUIV_EveryTime,
            (IPTR) self, 3, MUIM_Set, MUIA_PrefsEditor_Changed, TRUE
        );
        DoMethod
        (
            screenString, MUIM_Notify, MUIA_String_Acknowledge, MUIV_EveryTime,
            (IPTR) self, 3, MUIM_Set, MUIA_PrefsEditor_Changed, TRUE
        );
        DoMethod
        (
            systemString, MUIM_Notify, MUIA_String_Acknowledge, MUIV_EveryTime,
            (IPTR) self, 3, MUIM_Set, MUIA_PrefsEditor_Changed, TRUE
        );
    }

    return self;
}

static IPTR FPEditor__MUIM_PrefsEditor_ImportFH
(
    Class *CLASS, Object *self,
    struct MUIP_PrefsEditor_ImportFH *message
)
{
    SETUP_INST_DATA;
    BOOL success = TRUE;

    success = Prefs_ImportFH(message->fh, data->fped_FontPrefs);
    if (success) FontPrefs2Gadgets(data);

    return success;
}

static IPTR FPEditor__MUIM_PrefsEditor_ExportFH
(
    Class *CLASS, Object *self,
    struct MUIP_PrefsEditor_ExportFH *message
)
{
    SETUP_INST_DATA;
    BOOL success = TRUE;

    Gadgets2FontPrefs(data);
    success = Prefs_ExportFH(message->fh, data->fped_FontPrefs);

    return success;
}

static IPTR FPEditor__MUIM_PrefsEditor_SetDefaults
(
    Class *CLASS, Object *self,
    Msg message
)
{
    SETUP_INST_DATA;
    BOOL success = TRUE;

    success = Prefs_Default(data->fped_FontPrefs);
    if (success) FontPrefs2Gadgets(data);

    return success;
}

/*** Setup ******************************************************************/
ZUNE_CUSTOMCLASS_4
(
    FPEditor, NULL, MUIC_PrefsEditor, NULL,
    OM_NEW,                    struct opSet *,
    MUIM_PrefsEditor_ImportFH, struct MUIP_PrefsEditor_ImportFH *,
    MUIM_PrefsEditor_ExportFH, struct MUIP_PrefsEditor_ExportFH *,
    MUIM_PrefsEditor_SetDefaults, Msg
);
