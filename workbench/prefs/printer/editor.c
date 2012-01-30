/*
    Copyright © 2003-2011, The AROS Development Team. All rights reserved.
    $Id$
*/

#define MUIMASTER_YES_INLINE_STDARG

#include <stdlib.h>
#include <stdio.h>      /* for sprintf */

#define DEBUG 1
#include <aros/debug.h>

#include <zune/customclasses.h>
#include <zune/prefseditor.h>

#include <proto/alib.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>
#include <proto/dos.h>

#include "locale.h"
#include "editor.h"
#include "prefs.h"

/*** String Data ************************************************************/

STATIC CONST_STRPTR const ui_Pages[] = {
    "Driver",
    "Settings",
    NULL,
};

STATIC CONST_STRPTR const ui_DeviceUnits[] = {
    "Unit 0",
    "Unit 1",
    "Unit 2",
    "Unit 3",
    "Unit 4",
    "Unit 5",
    "Unit 6",
    "Unit 7",
    "Unit 8",
    "Unit 9",
    NULL,
};

#define UI_DEVICE_PARALLEL      0
#define UI_DEVICE_SERIAL        1
#define UI_DEVICE_PRINTTOFILE   2
#define UI_DEVICE_USBPRINTER    3

STATIC CONST_STRPTR const ui_Port[] = {
    "Parallel",
    "Serial",
    "Print to file",
    "USB Printer",
    NULL,
};

STATIC CONST_STRPTR const ui_PaperTypes[] = {
    "Fanfold",
    "Single",
    NULL,
};

STATIC CONST_STRPTR const ui_PaperSize[] = {
    "US Letter",
    "US Legal",
    "80 Column",
    "132 Column",
    "Custom",
    "A0",
    "A1",
    "A2",
    "A3",
    "A4",
    "A5",
    "A6",
    "A7",
    "A8",
    NULL,
};

#define UI_UNITS_IN   0       /* inches */
#define UI_UNITS_MM   1       /* millimeters */
#define UI_UNITS_PT   2       /* 1/72 of an inch */

STATIC CONST_STRPTR const ui_Units[] = {
    "in",
    "mm",
    "pt",
    NULL,
};

STATIC CONST_STRPTR const ui_Pitch[] = {
    "10 cpi (Pica)",
    "12 cpi (Elite)",
    "17.1 cpi (Fine)",
    NULL,
};

STATIC CONST_STRPTR const ui_Spacing[] = {
    "6 lpi",
    "8 lpi",
    NULL,
};

STATIC CONST_STRPTR const ui_Quality[] = {
    "Draft (72 dpi)",
    "Letter (240 dpi)",
    NULL,
};

STATIC CONST_STRPTR const ui_Aspect[] = {
    "Landscape",
    "Portrait",
    NULL,
};


STATIC CONST_STRPTR const ui_Shade[] = {
    "Black & White",
    "Greyscape",
    "Color",
    "Greyscape 2",
    NULL,
};


STATIC CONST_STRPTR const ui_Image[] = {
    "Positive",
    "Negative",
    NULL,
};


STATIC CONST_STRPTR const ui_Dimensions[] = {
    "Ignore",
    "Bounded",
    "Absolute",
    "Pixel",
    "Multiply",
    NULL,
};


STATIC CONST_STRPTR const ui_Dithering[] = {
    "Ordered",
    "Halftone",
    "Floyd-Steinberg",
    NULL,
};


/*** Instance Data **********************************************************/

struct PrinterEditor_DATA
{
    STRPTR UnitLabels[10];
    int PrinterDeviceUnit;

    Object *child;

    Object *pd_UnitNum;         /* Cycle */
    Object *pd_UnitName;        /* String */

    Object *pt_DriverList;      /* Dirlist */
    Object *pt_Driver;          /* Text */
    Object *pt_Port;            /* Cycle */
    Object *pt_PaperType;       /* Cycle */
    Object *pt_PaperSize;       /* Cycle */
    Object *pt_PaperLength;     /* String (integer) */
    Object *pt_Pitch;           /* Cycle */
    Object *pt_Spacing;         /* Cycle */
    Object *pt_LeftMargin;      /* String (integer) */
    Object *pt_RightMargin;     /* String (integer) */
    Object *pt_Quality;         /* Cycle */

    Object *pu_UnitNum;         /* String (integer) */
    Object *pu_DeviceName;      /* String */
    Object *pu_DeviceNameCustom;/* CheckMark */

    Object *pg_Aspect;          /* Cycle */
    Object *pg_Shade;           /* Cycle */
    Object *pg_Image;           /* Cycle */
    Object *pg_Threshold;       /* Slider */
    Object *pg_Dimensions;      /* Cycle */
    Object *pg_Dithering;       /* Cycle */
    Object *pg_GraphicFlags_Center;     /* CheckMark */
    Object *pg_GraphicFlags_AntiAlias;  /* CheckMark */
    Object *pg_GraphicFlags_IntScaling; /* CheckMark */
    Object *pg_PrintDensity;    /* Cycle */

    Object *pg_MaxUnits;        /* Cycle */
    Object *pg_PrintMaxWidth;   /* String (integer) */
    Object *pg_PrintMaxHeight;  /* String (integer) */

    Object *pg_OffsetUnits;     /* Label */
    Object *pg_PrintXOffset;    /* String (integer) */
    Object *pg_PrintYOffset;    /* String (integer) */
};

STATIC VOID PrinterPrefs2Gadgets(struct PrinterEditor_DATA *data);
STATIC VOID Gadgets2PrinterPrefs(Class *CLASS, Object *self);

/*** Macros *****************************************************************/
#define SETUP_INST_DATA struct PrinterEditor_DATA *data = INST_DATA(CLASS, self)

/*** Methods ****************************************************************/
Object *PrinterEditor__OM_NEW(Class *CLASS, Object *self, struct opSet *message)
{
    int i;
#if SHOWICON
    Object *icon;
#endif
    D(bug("[PrinterEdit class] PrinterEdit Class New\n"));

    /*
     * we create self first and then create the child,
     * so we have self->data available already
     */
    self = (Object *) DoSuperNewTags
    (
        CLASS, self, NULL,
        MUIA_PrefsEditor_Name, _(MSG_WINTITLE),
        MUIA_PrefsEditor_Path, (IPTR) "SYS/printer.prefs",
        MUIA_PrefsEditor_IconTool, (IPTR) "SYS:Prefs/Printer",

        TAG_DONE
    );

    if (self)
    {
        SETUP_INST_DATA;

        data->PrinterDeviceUnit = 0;
        for (i = 0; i < 10; i++) {
            data->UnitLabels[i] = StrDup(ui_DeviceUnits[i]);
        }

    #if SHOWPIC
        icon = MUI_NewObject("Dtpic.mui", MUIA_Dtpic_Name, "PROGDIR:Printer.info", TAG_DONE);

        if (!icon) icon = HVSpace;
    #endif

        data->child =
        VGroup,
            /* Unit selection */
            Child, (IPTR) HGroup,
#if SHOWPIC
                Child, icon,
#endif
                Child, (IPTR) LLabel1("Printer Unit:"),
                Child, (IPTR) (data->pd_UnitNum = CycleObject, 
                    MUIA_Cycle_Entries, (IPTR)data->UnitLabels,
                End),
                Child, (IPTR) LLabel1("Name:"),
                Child, (IPTR) (data->pd_UnitName = StringObject, 
                    MUIA_String_MaxLen, UNITNAMESIZE,
                End),
            End, 
            /* Pages */
            Child, (IPTR) RegisterGroup(ui_Pages),
                /* Driver and Device page */
                Child, (IPTR) HGroup,
                    Child, (IPTR) VGroup,
                        Child, (IPTR) LLabel("Select Driver:"),
                        Child, (IPTR) (data->pt_DriverList = DirlistObject,
                                MUIA_List_Format, (IPTR) "COL=0",
                                MUIA_List_AdjustWidth, TRUE,
                                MUIA_Dirlist_Directory, (IPTR)"DEVS:Printers",
                                MUIA_Dirlist_FilesOnly, TRUE,
                                MUIA_Dirlist_RejectIcons, TRUE,
                        End),
                    End,
                    Child, (IPTR) VGroup,
                        Child, (IPTR) (data->pt_Driver = TextObject,
                            MUIA_Text_Contents, (IPTR)"generic",
                        End),
                        Child, (IPTR) (data->pt_Port = CycleObject,
                            MUIA_Cycle_Entries, (IPTR) ui_Port,
                        End),
                        Child, (IPTR) HGroup,
                            Child, (IPTR) (data->pu_DeviceNameCustom = CheckMark(FALSE)),
                            Child, (IPTR) Label("Custom device"),
                        End,
                        Child, (IPTR) HGroup,
                            Child, (IPTR) Label("Device:"),
                            Child, (IPTR) (data->pu_DeviceName = StringObject,
                                MUIA_String_Format, MUIV_String_Format_Right,
                                MUIA_String_MaxLen, DEVICENAMESIZE,
                            End),
                        End,
                        Child, (IPTR) HGroup,
                            Child, (IPTR) Label("Unit:"),
                            Child, (IPTR) (data->pu_UnitNum = StringObject,
                                MUIA_String_Format, MUIV_String_Format_Right,
                                MUIA_String_Accept, (IPTR)"0123456879",
                                MUIA_String_Integer, 0,
                            End),
                        End,
                    End,
                End,
                /* Settings page */
                Child, (IPTR) VGroup,
                    Child, (IPTR) HGroup,
                        Child, (IPTR) ColGroup(2),
                            Child, (IPTR) LLabel("Paper Type:"),
                            Child, (IPTR) (data->pt_PaperType = CycleObject,
                                MUIA_Cycle_Entries, (IPTR) ui_PaperTypes,
                            End),
                            Child, (IPTR) LLabel("Paper Size:"),
                            Child, (IPTR) (data->pt_PaperSize = CycleObject,
                                MUIA_Cycle_Entries, (IPTR) ui_PaperSize,
                            End),
                            Child, (IPTR) (data->pg_MaxUnits = CycleObject,
                                MUIA_Cycle_Entries, (IPTR) ui_Units,
                            End),
                            Child, (IPTR) HGroup,
                                Child, (IPTR) (data->pg_PrintMaxWidth = StringObject,
                                    MUIA_Disabled, TRUE,
                                    MUIA_String_Format, MUIV_String_Format_Right,
                                    MUIA_String_Accept, (IPTR)".0123456879",
                                    MUIA_String_Contents, (IPTR)"0.0",
                                End),
                                Child, (IPTR) CLabel("x"),
                                Child, (IPTR) (data->pg_PrintMaxHeight = StringObject,
                                    MUIA_Disabled, TRUE,
                                    MUIA_String_Format, MUIV_String_Format_Right,
                                    MUIA_String_Accept, (IPTR)".0123456879",
                                    MUIA_String_Contents, (IPTR)"0.0",
                                End),
                            End,
                            Child, (IPTR) LLabel("Quality:"),
                            Child, (IPTR) (data->pt_Quality = CycleObject,
                                MUIA_Cycle_Entries, (IPTR) ui_Quality,
                            End),
                            Child, (IPTR) LLabel("Pitch:"),
                            Child, (IPTR) (data->pt_Pitch = CycleObject,
                                MUIA_Cycle_Entries, (IPTR) ui_Pitch,
                            End),
                            Child, (IPTR) LLabel("Spacing:"),
                            Child, (IPTR) (data->pt_Spacing = CycleObject,
                                MUIA_Cycle_Entries, (IPTR) ui_Spacing,
                            End),
                            Child, (IPTR) LLabel("Left Margin:"),
                            Child, (IPTR) (data->pt_LeftMargin = StringObject,
                                MUIA_String_Format, MUIV_String_Format_Right,
                                MUIA_String_Accept, (IPTR)"0123456879",
                                MUIA_String_Integer, 0,
                            End),
                            Child, (IPTR) LLabel("Right Margin:"),
                            Child, (IPTR) (data->pt_LeftMargin = StringObject,
                                MUIA_String_Format, MUIV_String_Format_Right,
                                MUIA_String_Accept, (IPTR)"0123456879",
                                MUIA_String_Integer, 0,
                            End),
                            Child, (IPTR) LLabel("Lines per Page:"),
                            Child, (IPTR) (data->pt_PaperLength = StringObject,
                                MUIA_String_Format, MUIV_String_Format_Right,
                                MUIA_String_Accept, (IPTR)"0123456879",
                                MUIA_String_Integer, 0,
                            End),
                        End,
                        Child, (IPTR) ColGroup(2),
                            Child, (IPTR) LLabel("Aspect:"),
                            Child, (IPTR) (data->pg_Aspect = CycleObject,
                                MUIA_Cycle_Entries, (IPTR) ui_Aspect,
                            End),
                            Child, (IPTR) LLabel("Shade:"),
                            Child, (IPTR) (data->pg_Shade = CycleObject,
                                MUIA_Cycle_Entries, (IPTR) ui_Shade,
                            End),
                            Child, (IPTR) LLabel("Image:"),
                            Child, (IPTR) (data->pg_Image = CycleObject,
                                MUIA_Cycle_Entries, (IPTR) ui_Image,
                            End),
                            Child, (IPTR) LLabel("Threshold:"),
                            Child, (IPTR) (data->pg_Threshold = SliderObject,
                                MUIA_Slider_Horiz, TRUE,
                                MUIA_Numeric_Min, 0,
                                MUIA_Numeric_Max, 255,
                            End),
                            Child, (IPTR) LLabel("Dimensions:"),
                            Child, (IPTR) (data->pg_Dimensions = CycleObject,
                                MUIA_Cycle_Entries, (IPTR) ui_Dimensions,
                            End),
                            Child, (IPTR) LLabel("Dithering:"),
                            Child, (IPTR) (data->pg_Dithering = CycleObject,
                                MUIA_Cycle_Entries, (IPTR) ui_Dithering,
                            End),
                            Child, (IPTR) HGroup,
                                Child, (IPTR) LLabel("Offset ("),
                                Child, (IPTR) (data->pg_OffsetUnits = LLabel("mm")),
                                Child, (IPTR) LLabel("): "),
                            End,
                            Child, (IPTR) HGroup,
                                Child, (IPTR) (data->pg_PrintXOffset = StringObject,
                                    MUIA_String_Format, MUIV_String_Format_Right,
                                    MUIA_String_Accept, (IPTR)".0123456879",
                                    MUIA_String_Contents, (IPTR)"0.0",
                                End),
                                Child, (IPTR) CLabel("x"),
                                Child, (IPTR) (data->pg_PrintYOffset = StringObject,
                                    MUIA_String_Format, MUIV_String_Format_Right,
                                    MUIA_String_Accept, (IPTR)".0123456879",
                                    MUIA_String_Contents, (IPTR)"0.0",
                                End),
                            End,
                            Child, (IPTR) LLabel("Center Image:"),
                            Child, (IPTR) (data->pg_GraphicFlags_Center = CheckMark(FALSE)),
                            Child, (IPTR) LLabel("AntiAlias:"),
                            Child, (IPTR) (data->pg_GraphicFlags_AntiAlias = CheckMark(FALSE)),
                            Child, (IPTR) LLabel("Integer Scaling:"),
                            Child, (IPTR) (data->pg_GraphicFlags_IntScaling = CheckMark(FALSE)),
                        End,
                    End,
                End,
            End,
        End;

        DoMethod(self, OM_ADDMEMBER, (IPTR) data->child);

#define PREF_NOTIFY(x, field)   DoMethod(data->x, MUIM_Notify, field, MUIV_EveryTime, \
                                         (IPTR)self, 3, MUIM_Set, MUIA_PrefsEditor_Changed, TRUE)
#define PREF_SYNC(x, field)     PREF_NOTIFY(x, field); \
                                DoMethod(data->x, MUIM_Notify, field, MUIV_EveryTime, \
                                         (IPTR)self, 1, MUIM_PrinterEditor_SelfCheck)

        PREF_SYNC(pd_UnitNum, MUIA_Cycle_Active);
        PREF_NOTIFY(pd_UnitName, MUIA_String_Contents);

        PREF_SYNC(pt_DriverList, MUIA_List_Active);
        PREF_NOTIFY(pt_Port, MUIA_Cycle_Active);
        PREF_NOTIFY(pt_PaperType, MUIA_Cycle_Active);
        PREF_SYNC(pt_PaperSize, MUIA_Cycle_Active);
        PREF_NOTIFY(pt_PaperLength, MUIA_String_Integer);
        PREF_NOTIFY(pt_Pitch, MUIA_Cycle_Active);
        PREF_NOTIFY(pt_Spacing, MUIA_Cycle_Active);
        PREF_NOTIFY(pt_LeftMargin, MUIA_String_Integer);
        PREF_NOTIFY(pt_RightMargin, MUIA_String_Integer);
        PREF_NOTIFY(pt_Quality, MUIA_Cycle_Active);

        PREF_NOTIFY(pu_UnitNum, MUIA_String_Integer);
        PREF_NOTIFY(pu_DeviceName, MUIA_String_Contents);
        PREF_SYNC(pu_DeviceNameCustom, MUIA_Selected);

        PREF_NOTIFY(pg_Aspect, MUIA_Cycle_Active);
        PREF_NOTIFY(pg_Shade, MUIA_Cycle_Active);
        PREF_NOTIFY(pg_Image, MUIA_Cycle_Active);
        PREF_NOTIFY(pg_Threshold, MUIA_Numeric_Value);
        PREF_NOTIFY(pg_Dimensions, MUIA_Cycle_Active);
        PREF_NOTIFY(pg_Dithering, MUIA_Cycle_Active);
        PREF_NOTIFY(pg_GraphicFlags_Center, MUIA_Selected);
        PREF_NOTIFY(pg_GraphicFlags_AntiAlias, MUIA_Selected);
        PREF_NOTIFY(pg_GraphicFlags_IntScaling, MUIA_Selected);
        PREF_NOTIFY(pg_PrintDensity, MUIA_Cycle_Active);
        PREF_SYNC(pg_MaxUnits, MUIA_Cycle_Active);
        PREF_NOTIFY(pg_PrintMaxWidth, MUIA_String_Integer);
        PREF_NOTIFY(pg_PrintMaxHeight, MUIA_String_Integer);
        PREF_NOTIFY(pg_PrintXOffset, MUIA_String_Integer);
        PREF_NOTIFY(pg_PrintYOffset, MUIA_String_Integer);

        PrinterPrefs2Gadgets(data);
    }

    return self;
}

static inline LONG todeci(LONG units, double val)
{
    switch (units) {
        case UI_UNITS_IN: val *= 10.0; break;
        case UI_UNITS_MM: val /= 2.54; break;
        case UI_UNITS_PT: val /= 7.2 ; break;
    }

    return (LONG)val;
}

static inline double fromdeci(LONG units, LONG value)
{
    double val = value;

    switch (units) {
        case UI_UNITS_IN: val /= 10.0; break;
        case UI_UNITS_MM: val *= 2.54; break;
        case UI_UNITS_PT: val *= 7.2 ; break;
    }

    return val;
}

STATIC void UpdatePrintMax (struct PrinterEditor_DATA *data, int units, BOOL is_set)
{
    struct PrinterTxtPrefs *txt = &printerprefs.pp_Txt;
    struct PrinterGfxPrefs *gfx = &printerprefs.pp_Gfx;
    LONG width, height;
    CONST_STRPTR str;
    char buf[64];

    switch (txt->pt_PaperSize) {
/* PaperSize (in deci-inches) */
    case US_LETTER: width =  85; height = 110; break;   /* 8.5"x11" */
    case US_LEGAL:  width =  85; height = 140; break;   /* 8.5"x14" */
    case N_TRACTOR: width =  95; height = 110; break;   /* 9.5"x11" */
    case W_TRACTOR: width = 149; height = 110; break;   /* 14.86"x11" */
/* European sizes */
    case EURO_A0:   width = 841 * 100 / 254; height = 1189 * 100 / 254; break;  /* A0: 841 x 1189 */
    case EURO_A1:   width = 594 * 100 / 254; height =  841 * 100 / 254; break;  /* A1: 594 x 841  */
    case EURO_A2:   width = 420 * 100 / 254; height =  594 * 100 / 254; break;  /* A2: 420 x 594  */
    case EURO_A3:   width = 297 * 100 / 254; height =  420 * 100 / 254; break;  /* A3: 297 x 420  */
    case EURO_A4:   width = 210 * 100 / 254; height =  297 * 100 / 254; break;  /* A4: 210 x 297  */
    case EURO_A5:   width = 148 * 100 / 254; height =  210 * 100 / 254; break;  /* A5: 148 x 210  */
    case EURO_A6:   width = 105 * 100 / 254; height =  148 * 100 / 254; break;  /* A6: 105 x 148  */
    case EURO_A7:   width =  74 * 100 / 254; height =  105 * 100 / 254; break;  /* A7: 74 x 105   */
    case EURO_A8:   width =  52 * 100 / 254; height =   74 * 100 / 254; break;  /* A8: 52 x 74    */
    case CUSTOM:
        if (is_set) {
            width = gfx->pg_PrintMaxWidth;
            height = gfx->pg_PrintMaxHeight;
        } else {
            str = (CONST_STRPTR)XGET(data->pg_PrintMaxWidth, MUIA_String_Contents);
            width = todeci(units, atof(str));
            str = (CONST_STRPTR)XGET(data->pg_PrintMaxWidth, MUIA_String_Contents);
            height = todeci(units, atof(str));
        }
        break;
    default:
        width = height = 0;
        break;
    }

    if (!is_set) {
        gfx->pg_PrintMaxWidth = width;
        gfx->pg_PrintMaxHeight = height;
    }

    NNSET(data->pg_PrintMaxWidth, MUIA_Disabled, (txt->pt_PaperSize == CUSTOM) ? FALSE : TRUE);
    NNSET(data->pg_PrintMaxHeight, MUIA_Disabled, (txt->pt_PaperSize == CUSTOM) ? FALSE : TRUE);

D(bug("is_set=%d, printerprefs.pp_Txt.pt_PaperSize=%d\n", is_set, printerprefs.pp_Txt.pt_PaperSize));
    if (is_set || printerprefs.pp_Txt.pt_PaperSize != CUSTOM) {
        sprintf(buf, "%.*f", (units == UI_UNITS_IN) ? 1 : 0, fromdeci(units, width));
        NNSET(data->pg_PrintMaxWidth, MUIA_String_Contents, buf);
        sprintf(buf, "%.*f", (units == UI_UNITS_IN) ? 1 : 0, fromdeci(units, height));
        NNSET(data->pg_PrintMaxHeight, MUIA_String_Contents, buf);
    }
}

/*
 * update struct printerprefs with actual data selected in gadgets
 */
STATIC void Gadgets2PrinterPrefs (Class *CLASS, Object *self)
{
    SETUP_INST_DATA;

    struct PrinterTxtPrefs *txt = &printerprefs.pp_Txt;
    struct PrinterGfxPrefs *gfx = &printerprefs.pp_Gfx;
    struct PrinterUnitPrefs *unit = &printerprefs.pp_Unit;
    struct PrinterDeviceUnitPrefs *devunit = &printerprefs.pp_DeviceUnit;

    CONST_STRPTR str;
    int units;
    BOOL custom;

    if (XGET(data->pu_UnitNum, MUIA_Cycle_Active) != data->PrinterDeviceUnit) {
        char buf[64];
        char path[sizeof(buf) + 7]; /* ENVARC: */

        int PrinterDeviceUnit = XGET(data->pu_UnitNum, MUIA_Cycle_Active);

        sprintf(buf, "SYS/printer%d.prefs", PrinterDeviceUnit);
        str = (PrinterDeviceUnit ? buf : "SYS/printer.prefs");
        NNSET(self, MUIA_PrefsEditor_Path, (IPTR) str);
        /*-- Reload preferences --*/
        sprintf(path, "ENV:%s", str);
        if (!DoMethod(self, MUIM_PrefsEditor_Import, path))
        {
            sprintf(path, "ENVARC:%s", str);
            if (!DoMethod(self, MUIM_PrefsEditor_Import, path))
            {
                PrinterPrefs2Gadgets(data);
                Prefs_Default(PrinterDeviceUnit);
                SET(self, MUIA_PrefsEditor_Changed, TRUE);
                DoMethod(self, MUIM_PrinterEditor_SelfCheck);
                return;
            }
        }
        NNSET(data->pu_UnitNum, MUIA_Cycle_Active, PrinterDeviceUnit);
        SET(self, MUIA_PrefsEditor_Changed, TRUE);
        return;
    }

    D(bug("Gadgets2PrinterPrefs\n"));

    /* Internal scale is deci-inches. Yeah.
     */
    units = XGET(data->pg_MaxUnits, MUIA_Cycle_Active);

    switch (XGET(data->pt_Port, MUIA_Cycle_Active)) {
        case UI_DEVICE_SERIAL:
            txt->pt_Port = PP_SERIAL;
            strcpy(unit->pu_DeviceName, "serial");
            break;
        case UI_DEVICE_PARALLEL:
            txt->pt_Port = PP_PARALLEL;
            strcpy(unit->pu_DeviceName, "parallel");
            break;
        case UI_DEVICE_USBPRINTER:
            txt->pt_Port = PP_SERIAL;
            strcpy(unit->pu_DeviceName, "usbprinter");
            break;
        case UI_DEVICE_PRINTTOFILE:
            txt->pt_Port = PP_SERIAL;
            strcpy(unit->pu_DeviceName, "printtofile");
            break;
    }

    custom = XGET(data->pu_DeviceNameCustom, MUIA_Selected) ? TRUE : FALSE;
    if (custom) {
        str = (CONST_STRPTR)XGET(data->pu_DeviceName, MUIA_String_Contents);
        strcpy(unit->pu_DeviceName, str);
        unit->pu_UnitNum = (LONG)XGET(data->pu_UnitNum, MUIA_String_Integer);
    }

    NNSET(data->pt_Port, MUIA_Disabled, custom);

    if (XGET(data->pt_DriverList, MUIA_List_Active) != MUIV_List_Active_Off) {
        str = (CONST_STRPTR)XGET(data->pt_DriverList, MUIA_Dirlist_Path);
        NNSET(data->pt_Driver, MUIA_Text_Contents, FilePart(str));
    }

    strcpy(txt->pt_Driver, (CONST_STRPTR)XGET(data->pt_Driver, MUIA_Text_Contents));

    txt->pt_PaperType = XGET(data->pt_PaperType, MUIA_Cycle_Active);
    txt->pt_PaperSize = XGET(data->pt_PaperSize, MUIA_Cycle_Active) << 4;

    UpdatePrintMax(data, units, FALSE);

    txt->pt_PaperLength = XGET(data->pt_PaperLength, MUIA_String_Integer);
    txt->pt_Pitch = XGET(data->pt_Pitch, MUIA_Cycle_Active);
    txt->pt_Spacing = XGET(data->pt_Spacing, MUIA_Cycle_Active);
    txt->pt_LeftMargin = XGET(data->pt_LeftMargin, MUIA_String_Integer);
    txt->pt_RightMargin = XGET(data->pt_RightMargin, MUIA_String_Integer);
    txt->pt_Quality = XGET(data->pt_Quality, MUIA_Cycle_Active);

    strcpy(devunit->pd_UnitName, (CONST_STRPTR)XGET(data->pd_UnitName, MUIA_String_Contents));

    gfx->pg_Aspect = XGET(data->pg_Aspect, MUIA_Cycle_Active);
    gfx->pg_Shade = XGET(data->pg_Shade, MUIA_Cycle_Active);
    gfx->pg_Image = XGET(data->pg_Image, MUIA_Cycle_Active);
    gfx->pg_Threshold = XGET(data->pg_Threshold, MUIA_Numeric_Value);
    gfx->pg_ColorCorrect = 0; /* TODO! */
    gfx->pg_Dimensions = XGET(data->pg_Dimensions, MUIA_Cycle_Active);
    gfx->pg_GraphicFlags =
        (XGET(data->pg_GraphicFlags_Center, MUIA_Selected) ? PGFF_CENTER_IMAGE : 0) |
        (XGET(data->pg_GraphicFlags_AntiAlias, MUIA_Selected) ? PGFF_INTEGER_SCALING : 0) |
        (XGET(data->pg_GraphicFlags_IntScaling, MUIA_Selected) ? PGFF_ANTI_ALIAS : 0);

    str = (CONST_STRPTR)XGET(data->pg_PrintXOffset, MUIA_String_Contents);
    gfx->pg_PrintXOffset = todeci(units, atof(str));
    str = (CONST_STRPTR)XGET(data->pg_PrintYOffset, MUIA_String_Contents);
    gfx->pg_PrintYOffset = todeci(units, atof(str));

    NNSET(data->pg_OffsetUnits, MUIA_Text_Contents, (IPTR)ui_Units[units]);

    D(bug("Gadgets2PrinterPrefs left\n"));
}

/*
 * update gadgets with values of struct printerprefs
 */
STATIC VOID PrinterPrefs2Gadgets(struct PrinterEditor_DATA *data)
{
    struct PrinterTxtPrefs *txt = &printerprefs.pp_Txt;
    struct PrinterGfxPrefs *gfx = &printerprefs.pp_Gfx;
    struct PrinterUnitPrefs *unit = &printerprefs.pp_Unit;
    struct PrinterDeviceUnitPrefs *devunit = &printerprefs.pp_DeviceUnit;
    int units;
    char buf[64];

    /* Internal scale is deci-inches. Yeah.
     */
    units = XGET(data->pg_MaxUnits, MUIA_Cycle_Active);

    NNSET(data->pd_UnitNum, MUIA_Cycle_Active, devunit->pd_UnitNum);

    NNSET(data->pd_UnitName, MUIA_String_Contents, devunit->pd_UnitName);

    NNSET(data->pt_DriverList, MUIA_List_Active, MUIV_List_Active_Off);
    NNSET(data->pt_Driver, MUIA_String_Contents, txt->pt_Driver);
    NNSET(data->pt_Port, MUIA_Cycle_Active, txt->pt_Port);
    NNSET(data->pt_PaperType, MUIA_Cycle_Active, txt->pt_PaperType);
    NNSET(data->pt_PaperSize, MUIA_Cycle_Active, txt->pt_PaperSize >> 4);
    NNSET(data->pt_PaperLength, MUIA_String_Integer, txt->pt_PaperLength);
    NNSET(data->pt_Pitch, MUIA_Cycle_Active, txt->pt_Pitch);
    NNSET(data->pt_Spacing, MUIA_Cycle_Active, txt->pt_Spacing);
    NNSET(data->pt_LeftMargin, MUIA_String_Integer, txt->pt_LeftMargin);
    NNSET(data->pt_RightMargin, MUIA_String_Integer, txt->pt_RightMargin);
    NNSET(data->pt_Quality, MUIA_Cycle_Active, txt->pt_Quality);

    NNSET(data->pu_UnitNum, MUIA_String_Integer, unit->pu_UnitNum);
    NNSET(data->pu_DeviceName, MUIA_String_Contents, (IPTR)unit->pu_DeviceName);
    NNSET(data->pu_DeviceNameCustom, MUIA_Selected, unit->pu_DeviceName[0] ? TRUE : FALSE);

    if (strcmp(unit->pu_DeviceName, "printofile") == 0) {
        NNSET(data->pt_Port, MUIA_Cycle_Active, UI_DEVICE_PRINTTOFILE);
        NNSET(data->pu_DeviceNameCustom, MUIA_Selected, FALSE);
    }

    if (strcmp(unit->pu_DeviceName, "usbprinter") == 0) {
        NNSET(data->pt_Port, MUIA_Cycle_Active, UI_DEVICE_USBPRINTER);
        NNSET(data->pu_DeviceNameCustom, MUIA_Selected, FALSE);
    }

    NNSET(data->pg_Aspect, MUIA_Cycle_Active, gfx->pg_Aspect);
    NNSET(data->pg_Shade, MUIA_Cycle_Active, gfx->pg_Shade);
    NNSET(data->pg_Image, MUIA_Cycle_Active, gfx->pg_Image);
    NNSET(data->pg_Threshold, MUIA_Numeric_Value, gfx->pg_Threshold);
    NNSET(data->pg_Dimensions, MUIA_Cycle_Active, gfx->pg_Dimensions);
    NNSET(data->pg_Dithering, MUIA_Cycle_Active, gfx->pg_Dithering);
    NNSET(data->pg_GraphicFlags_Center, MUIA_Selected, (gfx->pg_GraphicFlags & PGFF_CENTER_IMAGE) ? TRUE : FALSE);
    NNSET(data->pg_GraphicFlags_AntiAlias, MUIA_Selected, (gfx->pg_GraphicFlags & PGFF_ANTI_ALIAS) ? TRUE : FALSE);
    NNSET(data->pg_GraphicFlags_IntScaling, MUIA_Selected, (gfx->pg_GraphicFlags & PGFF_INTEGER_SCALING) ? TRUE : FALSE);
    NNSET(data->pg_PrintDensity, MUIA_Cycle_Active, gfx->pg_PrintDensity);

    UpdatePrintMax(data, units, TRUE);

    NNSET(data->pg_OffsetUnits, MUIA_Text_Contents, (IPTR)ui_Units[units]);

    sprintf(buf, "%.*f", (units == UI_UNITS_IN) ? 1 : 0, fromdeci(units, gfx->pg_PrintXOffset));
    NNSET(data->pg_PrintXOffset, MUIA_String_Contents, buf);
    sprintf(buf, "%.*f", (units == UI_UNITS_IN) ? 1 : 0, fromdeci(units, gfx->pg_PrintYOffset));
    NNSET(data->pg_PrintYOffset, MUIA_String_Contents, buf);
}

IPTR PrinterEditor__MUIM_PrefsEditor_ImportFH (
    Class *CLASS, Object *self,
    struct MUIP_PrefsEditor_ImportFH *message
)
{
    SETUP_INST_DATA;
    BOOL success = TRUE;

    D(bug("[PrinterEdit class] PrinterEdit Class Import\n"));

    success = Prefs_ImportFH(message->fh);
    if (success) PrinterPrefs2Gadgets(data);

    return success;
}

IPTR PrinterEditor__MUIM_PrefsEditor_ExportFH
(
    Class *CLASS, Object *self,
    struct MUIP_PrefsEditor_ExportFH *message
)
{
    BOOL success = TRUE;

    D(bug("[PrinterEdit class] PrinterEdit Class Export\n"));

    Gadgets2PrinterPrefs(CLASS, self);
    success = Prefs_ExportFH(message->fh);

    return success;
}

IPTR PrinterEditor__MUIM_PrefsEditor_SetDefaults
(
    Class *CLASS, Object *self, Msg message
)
{
    SETUP_INST_DATA;
    BOOL success = TRUE;

    D(bug("[PrinterEdit class] PrinterEdit Class SetDefaults\n"));

    success = Prefs_Default(0);
    if (success) PrinterPrefs2Gadgets(data);

    return success;
}

IPTR PrinterEditor__MUIM_PrinterEditor_SelfCheck(Class *CLASS, Object *self, Msg message)
{
    BOOL success = TRUE;

    Gadgets2PrinterPrefs(CLASS, self);

    return success;
}

/*** Setup ******************************************************************/
ZUNE_CUSTOMCLASS_5
(
    PrinterEditor, NULL, MUIC_PrefsEditor, NULL,
    OM_NEW,                       struct opSet *,
    MUIM_PrefsEditor_ImportFH,    struct MUIP_PrefsEditor_ImportFH *,
    MUIM_PrefsEditor_ExportFH,    struct MUIP_PrefsEditor_ExportFH *,
    MUIM_PrefsEditor_SetDefaults, Msg,
    MUIM_PrinterEditor_SelfCheck, Msg
);

