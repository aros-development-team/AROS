/*
    Copyright © 1995-1997 Stefan Stuntz.
    Copyright © 2009-2010, The AROS Development Team.
    All rights reserved.

    $Id$
*/

#include <proto/intuition.h>
#include <proto/muimaster.h>
#include <proto/utility.h>
#include <proto/graphics.h>

#include "displayidinfo_class.h"

#define USE_PSI_COLORS_BODY
#include "psi_colors.bh"

#define USE_PSI_SIZES_BODY
#define USE_PSI_SIZES_COLORS
#include "psi_sizes.bh"

#define USE_PSI_FREQS_BODY
#include "psi_freqs.bh"

/****************************************************************************************/

struct DispIDinfo_Data
{
    Object *TX_Visible[2];
    Object *TX_Minimum[2];
    Object *TX_Maximum[2];
    Object *TX_BitsPerGun;
    Object *TX_NumColors;
    Object *TX_ScanRate;
    Object *TX_ScanLine;
};

/****************************************************************************************/

Object *MakeMoni(LONG w, LONG h, LONG d, const UBYTE *body, const ULONG *colors)
{
    Object *obj = BodychunkObject,
        MUIA_FixWidth             , w,
        MUIA_FixHeight            , h,
        MUIA_Bitmap_Width         , w,
        MUIA_Bitmap_Height        , h,
        MUIA_Bodychunk_Depth      , d,
        MUIA_Bodychunk_Body       , (UBYTE *)body,
        MUIA_Bodychunk_Compression, PSI_COLORS_COMPRESSION,
        MUIA_Bodychunk_Masking    , PSI_COLORS_MASKING,
        MUIA_Bitmap_SourceColors  , (ULONG *)colors,
        MUIA_Bitmap_Transparent   , 0,
    End;

    return obj;
}

/****************************************************************************************/

Object *MakeSize(void)
{
    Object *obj = TextObject,
        MUIA_Text_Contents, "0",
        MUIA_Text_PreParse, "\33r",
        MUIA_FixWidthTxt, "00000",
    End;

    return obj;
}

/****************************************************************************************/

IPTR DispIDinfo_New(struct IClass *cl, Object *obj, struct opSet *msg)
{
    Object *TX_Visible[2];
    Object *TX_Minimum[2];
    Object *TX_Maximum[2];
    Object *TX_BitsPerGun;
    Object *TX_NumColors;
    Object *TX_ScanRate;
    Object *TX_ScanLine;
    Object *g1,*g2,*g3;

    obj = (Object *)DoSuperNewTags(cl, obj, NULL,
        MUIA_Group_Columns, 2,
        Child, g1 = MakeMoni(PSI_SIZES_WIDTH,PSI_SIZES_HEIGHT,PSI_SIZES_DEPTH,psi_sizes_body,psi_sizes_colors),
        Child, ColGroup(4), MUIA_Group_VertSpacing, 0, MUIA_Group_HorizSpacing, 4,
            Child, MakeLabel(MSG_LABEL_VISIBLE),
            Child, TX_Visible[0]=MakeSize(),
            Child, Label("x"),
            Child, TX_Visible[1]=MakeSize(),
            Child, MakeLabel(MSG_LABEL_MINIMUM),
            Child, TX_Minimum[0]=MakeSize(),
            Child, Label("x"),
            Child, TX_Minimum[1]=MakeSize(),
            Child, MakeLabel(MSG_LABEL_MAXIMUM),
            Child, TX_Maximum[0]=MakeSize(),
            Child, Label("x"),
            Child, TX_Maximum[1]=MakeSize(),
        End,
        Child, g2 = MakeMoni(PSI_COLORS_WIDTH,PSI_COLORS_HEIGHT,PSI_COLORS_DEPTH,psi_colors_body,psi_sizes_colors),
        Child, ColGroup(2), MUIA_Group_VertSpacing, 0, MUIA_Group_HorizSpacing, 4,
            Child, MakeLabel(MSG_LABEL_BITSPERGUN),
            Child, TX_BitsPerGun = TextObject, End,
            Child, MakeLabel(MSG_LABEL_MAXIMUM),
            Child, TX_NumColors  = TextObject, End,
        End,
        Child, g3 = MakeMoni(PSI_FREQS_WIDTH,PSI_FREQS_HEIGHT,PSI_FREQS_DEPTH,psi_freqs_body,psi_sizes_colors),
        Child, ColGroup(2), MUIA_Group_VertSpacing, 0, MUIA_Group_HorizSpacing, 4,
            Child, MakeLabel(MSG_LABEL_SCANRATE),
            Child, TX_ScanRate = TextObject, End,
            Child, MakeLabel(MSG_LABEL_SCANLINE),
            Child, TX_ScanLine = TextObject, End,
        End,
    TAG_MORE, msg->ops_AttrList);

    if (obj)
    {
        struct DispIDinfo_Data *data = INST_DATA(cl, obj);

/*
        set(g1,MUIA_VertDisappear,3);
        set(g2,MUIA_VertDisappear,2);
        set(g3,MUIA_VertDisappear,1);
*/

        data->TX_Visible[0] = TX_Visible[0];
        data->TX_Visible[1] = TX_Visible[1];
        data->TX_Minimum[0] = TX_Minimum[0];
        data->TX_Minimum[1] = TX_Minimum[1];
        data->TX_Maximum[0] = TX_Maximum[0];
        data->TX_Maximum[1] = TX_Maximum[1];
        data->TX_BitsPerGun = TX_BitsPerGun;
        data->TX_NumColors  = TX_NumColors;
        data->TX_ScanRate   = TX_ScanRate;
        data->TX_ScanLine   = TX_ScanLine;

        return (IPTR)obj;
    }

    return 0;
}

/****************************************************************************************/

IPTR DispIDinfo_Set(struct IClass *cl, Object *obj, struct opSet *msg)
{
    #define offset(a,b) (((IPTR)b)-(((IPTR)a)))
    struct DispIDinfo_Data *data = INST_DATA(cl, obj);
    struct TagItem *tag;

    if ((tag = FindTagItem(MUIA_DispIDinfo_ID,msg->ops_AttrList)))
    {
        struct DisplayInfo   dis;
        struct DimensionInfo dim;
        struct MonitorInfo   mon;
        int dislen;
        int dimlen;
        int monlen;

        set(data->TX_Visible[0], MUIA_String_Contents, "");
        set(data->TX_Visible[1], MUIA_String_Contents, "");
        set(data->TX_Minimum[0], MUIA_String_Contents, "");
        set(data->TX_Minimum[1], MUIA_String_Contents, "");
        set(data->TX_Maximum[0], MUIA_String_Contents, "");
        set(data->TX_Maximum[1], MUIA_String_Contents, "");
        set(data->TX_BitsPerGun, MUIA_String_Contents, "");
        set(data->TX_NumColors , MUIA_String_Contents, "");
        set(data->TX_ScanRate  , MUIA_String_Contents, "");
        set(data->TX_ScanLine  , MUIA_String_Contents, "");

        dislen = GetDisplayInfoData(0, (char *)&dis, sizeof(struct DisplayInfo  ), DTAG_DISP,tag->ti_Data);
        dimlen = GetDisplayInfoData(0, (char *)&dim, sizeof(struct DimensionInfo), DTAG_DIMS,tag->ti_Data);
        monlen = GetDisplayInfoData(0, (char *)&mon, sizeof(struct MonitorInfo  ), DTAG_MNTR,tag->ti_Data);

        if (dimlen > offset(&dim, &dim.MaxOScan))
        {
            settxt(data->TX_Visible[0], RectangleWidth (dim.MaxOScan));
            settxt(data->TX_Visible[1], RectangleHeight(dim.MaxOScan));
            settxt(data->TX_Minimum[0], dim.MinRasterWidth );
            settxt(data->TX_Minimum[1], dim.MinRasterHeight);
            settxt(data->TX_Maximum[0], dim.MaxRasterWidth );
            settxt(data->TX_Maximum[1], dim.MaxRasterHeight);
            settxt(data->TX_NumColors, 1 << dim.MaxDepth);
        }

        if (dislen > offset(&dis, &dis.BlueBits))
        {
            DoMethod
            (
                data->TX_BitsPerGun, MUIM_SetAsString, MUIA_Text_Contents, "%ld x %ld x %ld",
                dis.RedBits, dis.GreenBits, dis.BlueBits
            );
        }

        if (monlen > offset(&mon, &mon.TotalColorClocks))
        {
            /* These calculations were taken from ScreenManager by Bernhard "ZZA" Moellemann. Thanks! */

            if (mon.TotalRows)
            {
                ULONG vfreqint=1000000000L / ((ULONG)mon.TotalColorClocks * 280 * mon.TotalRows / 1000) + 5;
                DoMethod
                (
                    data->TX_ScanRate, MUIM_SetAsString, MUIA_Text_Contents, "%ld.%02ld Hz",
                    vfreqint / 1000, (vfreqint - (vfreqint / 1000) * 1000) / 10
                );
            }
            if (mon.TotalColorClocks)
            {
                ULONG hfreq = 1000000000L / ((ULONG)mon.TotalColorClocks * 280) + 5;
                ULONG hfreqint = hfreq / 1000;
                DoMethod
                (
                    data->TX_ScanLine, MUIM_SetAsString, MUIA_Text_Contents, "%ld.%02ld kHz",
                    hfreqint, (hfreq-hfreqint * 1000) / 10
                );
            }
        }
    }

    return DoSuperMethodA(cl, obj, (Msg)msg);
}

/****************************************************************************************/

BOOPSI_DISPATCHER(IPTR, DispIDinfo_Dispatcher, cl, obj, msg)
{
    switch (msg->MethodID)
    {
        case OM_NEW: return DispIDinfo_New(cl, obj, (APTR)msg);
        case OM_SET: return DispIDinfo_Set(cl, obj, (APTR)msg);
    }
    return DoSuperMethodA(cl, obj, msg);
}
BOOPSI_DISPATCHER_END

/****************************************************************************************/

VOID DispIDinfo_Init(VOID)
{
    CL_DispIDinfo = MUI_CreateCustomClass
    (
        NULL, MUIC_Group, NULL, sizeof(struct DispIDinfo_Data), DispIDinfo_Dispatcher
    );
}

/****************************************************************************************/

VOID DispIDinfo_Exit(VOID)
{
    if (CL_DispIDinfo) MUI_DeleteCustomClass(CL_DispIDinfo);
}
