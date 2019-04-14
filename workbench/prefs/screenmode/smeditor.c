/*
    Copyright © 2003-2019, The AROS Development Team. All rights reserved.
    $Id$
*/

#define MUIMASTER_YES_INLINE_STDARG
#define DEBUG 0

#include <libraries/mui.h>

#include <proto/alib.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/muimaster.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/utility.h>
#include <proto/iffparse.h>

#include <zune/customclasses.h>
#include <zune/prefseditor.h>
#include <zune/systemprefswindow.h>

#include <prefs/screenmode.h>
#include <prefs/prefhdr.h>

#include <stdio.h>

#include "locale.h"

#include "prefs.h"
#include "smeditor.h"
#include "smselector.h"
#include "smproperties.h"
#include "smattributes.h"


struct SMEditor_DATA
{
    Object *selector, *properties, *attributes;
};

#define SMEditorObject BOOPSIOBJMACRO_START(SMEditor_CLASS->mcc_Class)
#define SETUP_INST_DATA struct SMEditor_DATA *data = INST_DATA(CLASS, self)

static BOOL Gadgets2ScreenmodePrefs
(
    struct SMEditor_DATA *data
)
{
    UWORD width, height;

    if (XGET(data->selector, MUIA_List_Active) == MUIV_List_Active_Off)
    {
        // No active list entry? Reset to defaults
        Prefs_Default();
    }
    else
    {
        screenmodeprefs.smp_DisplayID = XGET(data->properties, MUIA_ScreenModeProperties_DisplayID);

        width = XGET(data->properties, MUIA_ScreenModeProperties_Width);
        if (width == XGET(data->properties, MUIA_ScreenModeProperties_DefWidth))
            width = ~0;
        screenmodeprefs.smp_Width = width;

        height = XGET(data->properties, MUIA_ScreenModeProperties_Height);
        if (height == XGET(data->properties, MUIA_ScreenModeProperties_DefHeight))
            height = ~0;
        screenmodeprefs.smp_Height = height;

        screenmodeprefs.smp_Depth     = XGET(data->properties, MUIA_ScreenModeProperties_Depth);
        
        if (XGET(data->properties, MUIA_ScreenModeProperties_Autoscroll))
            screenmodeprefs.smp_Control = SMF_AUTOSCROLL;
        else
            screenmodeprefs.smp_Control = 0;
    }

    D(bug("[smeditor] Gadgets2Prefs:\n"));
    D(bug("[smeditor] DisplayID 0x%08lX\n", screenmodeprefs.smp_DisplayID));
    D(bug("[smeditor] Size %ldx%ld\n", screenmodeprefs.smp_Width, screenmodeprefs.smp_Height));
    D(bug("[smeditor] Depth %ld\n", screenmodeprefs.smp_Depth));
    D(bug("[smeditor] Control 0x%08lX\n", screenmodeprefs.smp_Control));

    return TRUE;
}

static BOOL ScreenmodePrefs2Gadgets
(
    struct SMEditor_DATA *data
)
{
    D(bug("[smeditor] Prefs2Gadgets:\n"));
    D(bug("[smeditor] DisplayID 0x%08lX\n", screenmodeprefs.smp_DisplayID));
    D(bug("[smeditor] Size %ldx%ld\n", screenmodeprefs.smp_Width, screenmodeprefs.smp_Height));
    D(bug("[smeditor] Depth %ld\n", screenmodeprefs.smp_Depth));
    D(bug("[smeditor] Control 0x%08lX\n", screenmodeprefs.smp_Control));

    NNSET(data->selector, MUIA_ScreenModeSelector_Active, screenmodeprefs.smp_DisplayID);
    SetAttrs
    (
        data->properties,
        MUIA_NoNotify,                        TRUE,
        MUIA_ScreenModeProperties_DisplayID,  screenmodeprefs.smp_DisplayID,
        MUIA_ScreenModeProperties_Width,      screenmodeprefs.smp_Width,
        MUIA_ScreenModeProperties_Height,     screenmodeprefs.smp_Height,
        MUIA_ScreenModeProperties_Depth,      screenmodeprefs.smp_Depth,
        MUIA_ScreenModeProperties_Autoscroll, screenmodeprefs.smp_Control & SMF_AUTOSCROLL,
        TAG_DONE
    );
    
    SetAttrs
    (
        data->attributes,
        MUIA_ScreenModeAttributes_DisplayID, screenmodeprefs.smp_DisplayID,
        TAG_DONE
    );

    return TRUE;
}

static Object *SMEditor__OM_NEW(Class *CLASS, Object *self, struct opSet *message)
{
    Object *selector, *properties, *attributes;
    self = (Object *) DoSuperNewTags
    (
        CLASS, self, NULL,
        
        MUIA_PrefsEditor_Name, __(MSG_NAME),
        MUIA_PrefsEditor_Path, (IPTR)"SYS/screenmode.prefs",
        MUIA_PrefsEditor_IconTool, (IPTR)"SYS:Prefs/Screenmode",

        Child, (IPTR)HGroup,
        
            Child, (IPTR)VGroup,
                MUIA_Weight, 70,
                Child, (IPTR) CLabel(_(MSG_DISPLAY_MODE)),
                Child, (IPTR)(selector   = (Object *)ScreenModeSelectorObject, End),
                Child, (IPTR)(properties = (Object *)ScreenModePropertiesObject, GroupFrame, End),
            End,

            Child, (IPTR)VGroup,
                MUIA_Weight, 30,
                Child, (IPTR) CLabel(_(MSG_MODE_ATTRIBUTES)),
                Child, (IPTR)(attributes = (Object *)ScreenModeAttributesObject, GroupFrame, End),
            End,

        End,
        
        TAG_DONE
    );
    
    if (self)
    {
        SETUP_INST_DATA;
        
        data->selector   = selector;
        data->properties = properties;
        data->attributes = attributes;
             
        /*-- Setup notifications -------------------------------------------*/
        DoMethod
        (
            selector, MUIM_Notify, MUIA_ScreenModeSelector_Active, MUIV_EveryTime,
            (IPTR)properties, 3,
            MUIM_Set, MUIA_ScreenModeProperties_DisplayID, MUIV_TriggerValue
        );
        
        DoMethod
        (
            selector, MUIM_Notify, MUIA_ScreenModeSelector_Active, MUIV_EveryTime,
            (IPTR)attributes, 3,
            MUIM_Set, MUIA_ScreenModeAttributes_DisplayID, MUIV_TriggerValue
        );
                
        DoMethod
        (
            properties, MUIM_Notify, MUIA_ScreenModeProperties_DisplayID, MUIV_EveryTime,
            (IPTR)self, 3,
            MUIM_Set, MUIA_PrefsEditor_Changed, TRUE
        );
    
        DoMethod
        (
            properties, MUIM_Notify, MUIA_ScreenModeProperties_Width, MUIV_EveryTime,
            (IPTR)self, 3,
            MUIM_Set, MUIA_PrefsEditor_Changed, TRUE
        );
        
        DoMethod
        (
            properties, MUIM_Notify, MUIA_ScreenModeProperties_Height, MUIV_EveryTime,
            (IPTR)self, 3,
            MUIM_Set, MUIA_PrefsEditor_Changed, TRUE
        );
        
        DoMethod
        (
            properties, MUIM_Notify, MUIA_ScreenModeProperties_Depth, MUIV_EveryTime,
            (IPTR)self, 3,
            MUIM_Set, MUIA_PrefsEditor_Changed, TRUE
        );
        
        DoMethod
        (
            properties, MUIM_Notify, MUIA_ScreenModeProperties_Autoscroll, MUIV_EveryTime,
            (IPTR)self, 3,
            MUIM_Set, MUIA_PrefsEditor_Changed, TRUE
        );
    }
    
    return self;
}

static IPTR SMEditor__MUIM_PrefsEditor_ImportFH
(
    Class *CLASS, Object *self, 
    struct MUIP_PrefsEditor_ImportFH *message
)
{
    SETUP_INST_DATA;
    BOOL success = TRUE;

    success = Prefs_ImportFH(message->fh);

    if (success)
    {
        ScreenmodePrefs2Gadgets(data);
    }

    return success;
}

static IPTR SMEditor__MUIM_PrefsEditor_ExportFH
(
    Class *CLASS, Object *self,
    struct MUIP_PrefsEditor_ExportFH *message
)
{
    SETUP_INST_DATA;
    BOOL success = TRUE;

    Gadgets2ScreenmodePrefs(data);
    success = Prefs_ExportFH(message->fh);

    return success;
}

static IPTR SMEditor__MUIM_PrefsEditor_Export
(
    Class *CLASS, Object *self,
    struct MUIP_PrefsEditor_ExportFH *message
)
{
#if (0)
    Object *window;
#endif
    IPTR retval;

#if (0)
    window = _win(self);

    SET(window, MUIA_Window_Open, FALSE);
#endif
    retval = DoSuperMethodA(CLASS, self, (Msg)message);
#if (0)
    if (!retval)
    {
        SET(window, MUIA_Window_Open, TRUE);
    }
#endif
    return retval;
}

static IPTR SMEditor__MUIM_PrefsEditor_Test
(
    Class *CLASS, Object *self,
    struct MUIP_PrefsEditor_ExportFH *message
)
{
    SETUP_INST_DATA;
    struct MsgPort *testPort;
    struct Screen *testScreen;
    ULONG modeid;
    UWORD width, height;
    UBYTE depth;

    testPort = CreateMsgPort();
    if (testPort)
    {
        struct timerequest *timer; 
        bug("[SMPrefs] testPort @ 0x%p\n", testPort);
        timer = (struct timerequest *)CreateIORequest(testPort, sizeof(struct timerequest)); 
        if (timer)
        {
            bug("[SMPrefs] timer @ 0x%p\n", timer);
            if (!OpenDevice ("timer.device", UNIT_VBLANK, (struct IORequest *)timer,0))
            {
                bug("[SMPrefs] timer.device opened\n");

                timer->tr_node.io_Command = TR_ADDREQUEST;
                timer->tr_time.tv_secs    = 6;
                timer->tr_time.tv_micro   = 0;

                modeid = XGET(data->properties, MUIA_ScreenModeProperties_DisplayID);
                width = XGET(data->properties, MUIA_ScreenModeProperties_Width);
                height = XGET(data->properties, MUIA_ScreenModeProperties_Height);
                depth = XGET(data->properties, MUIA_ScreenModeProperties_Height);

                testScreen = OpenScreenTags
                (
                    NULL,
            /*        SA_PubName, name,*/
                    SA_Title, "ScreenMode Test",
                    SA_Width, width,
                    SA_Height, height,
                    SA_Depth, depth,
                    SA_DisplayID, modeid,
                    TAG_DONE
                );

                if (testScreen)
                {
                    struct RastPort *rp;
                    int col, colwidth, colheight, rowheight;
                    LONG colPen;
                    UBYTE r, g, b;

                    colwidth = width / 7;
                    colheight = (height / 100) * 70;
                    rowheight = (height - colheight) / 3;

                    bug("[SMPrefs] testScreen @ 0x%p\n", testScreen);

                    ULONG rgbPens[] = {
                       0x9F9F9F, // Gray
                       0xFFFF00, // Yellow
                       0x00FFFF, // Turqouise
                       0x00FF00, // Green
                       0xFF00FF, // Purple
                       0xFF0000, // Red
                       0x0000FF, // Blue
                    };

                    rp = CreateRastPort();
                    rp->BitMap = testScreen->RastPort.BitMap;

                    bug("[SMPrefs] rp @ 0x%p\n", rp);
                    bug("[SMPrefs] bm @ 0x%p\n", rp->BitMap);

                    SetRGB32(&testScreen->ViewPort, 0, 0, 0, 0);

                    for (col = 0; col < 7; col ++)
                    {
                        r = (rgbPens[col] & 0xFF0000) >> 16;
                        g = (rgbPens[col] & 0x00FF00) >> 8;
                        b = rgbPens[col] & 0x0000FF;

                        SetRGB32(&testScreen->ViewPort,
                            (col + 1),
                            r + (r<<8) + (r<<16) + (r<<24),
                            g + (g<<8) + (g<<16) + (g<<24),
                            b + (b<<8) + (b<<16) + (b<<24));

                        colPen = ObtainBestPen (testScreen->ViewPort.ColorMap,
                            r + (r<<8) + (r<<16) + (r<<24),
                            g + (g<<8) + (g<<16) + (g<<24),
                            b + (b<<8) + (b<<16) + (b<<24),
                            OBP_FailIfBad, FALSE,
                            OBP_Precision, PRECISION_GUI,
                            TAG_END);

                        SetAPen(rp, colPen);
                        RectFill(rp, col * colwidth, 0, ((col + 1) * colwidth) - 1, colheight);
                    }
                    for (col = 0; col < 7; col ++)
                    {
                        if (col % 2 == 0)
                        {
                            r = (rgbPens[6 - col] & 0xFF0000) >> 16;
                            g = (rgbPens[6 - col] & 0x00FF00) >> 8;
                            b = rgbPens[6 - col] & 0x0000FF;
                        }
                        else
                        {
                            r = 0;
                            g = 0;
                            b = 0;
                        }
                        colPen = ObtainBestPen (testScreen->ViewPort.ColorMap,
                            r + (r<<8) + (r<<16) + (r<<24),
                            g + (g<<8) + (g<<16) + (g<<24),
                            b + (b<<8) + (b<<16) + (b<<24),
                            OBP_FailIfBad, FALSE,
                            OBP_Precision, PRECISION_GUI,
                            TAG_END);

                        SetAPen(rp, colPen);
                        RectFill(rp, col * colwidth, colheight, ((col + 1) * colwidth) - 1, colheight + rowheight);
                    }
                    /* Draw greyscale */
                    colwidth = width / 12;
                    for (col = 0; col < 12; col ++)
                    {
                        r = col * (0xFF/12);
                        g = col * (0xFF/12);
                        b = col * (0xFF/12);

                        if (col != 0)
                        {
                            SetRGB32(&testScreen->ViewPort,
                                (col + 9),
                                r + (r<<8) + (r<<16) + (r<<24),
                                g + (g<<8) + (g<<16) + (g<<24),
                                b + (b<<8) + (b<<16) + (b<<24));
                        }
                        colPen = ObtainBestPen (testScreen->ViewPort.ColorMap,
                            r + (r<<8) + (r<<16) + (r<<24),
                            g + (g<<8) + (g<<16) + (g<<24),
                            b + (b<<8) + (b<<16) + (b<<24),
                            OBP_FailIfBad, FALSE,
                            OBP_Precision, PRECISION_GUI,
                            TAG_END);

                        SetAPen(rp, colPen);
                        RectFill(rp, col * colwidth, colheight + rowheight, ((col + 1) * colwidth) - 1, colheight + (rowheight << 1));
                    }
                    DoIO ((struct IORequest *)timer);

                    CloseScreen(testScreen);
                }
                CloseDevice((struct IORequest *)timer);
            }
            DeleteIORequest((struct IORequest *)timer);
        }
        DeleteMsgPort(testPort); 
    }

    return TRUE;
}

static IPTR SMEditor__MUIM_PrefsEditor_SetDefaults
(
    Class *CLASS, Object *self,
    Msg message
)
{
    SETUP_INST_DATA;
    BOOL success = TRUE;

    success = Prefs_Default();
    if (success)
    {
        ScreenmodePrefs2Gadgets(data);
        SET(data->selector, MUIA_List_Active, MUIV_List_Active_Off);
    }

    return success;
}

ZUNE_CUSTOMCLASS_6
(
    SMEditor, NULL, MUIC_PrefsEditor, NULL,
    OM_NEW,                    struct opSet *,
    MUIM_PrefsEditor_ImportFH, struct MUIP_PrefsEditor_ImportFH *,
    MUIM_PrefsEditor_Export, struct MUIP_PrefsEditor_ExportFH *,
    MUIM_PrefsEditor_ExportFH, struct MUIP_PrefsEditor_ExportFH *,
    MUIM_PrefsEditor_Test, struct MUIP_PrefsEditor_ExportFH *,
    MUIM_PrefsEditor_SetDefaults,     Msg
);
