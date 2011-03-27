/*
    Copyright © 1995-1997 Stefan Stuntz.
    Copyright © 2009-2011, The AROS Development Team.
    All rights reserved.

    $Id$
*/

#define MUI_OBSOLETE

#include <libraries/muiscreen.h>

#include <proto/muiscreen.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>
#include <proto/asl.h>
#include <proto/intuition.h>
#include <proto/graphics.h>

#include <string.h>
#include <stdlib.h>

#include "displayidlist_class.h"
#include "displayidinfo_class.h"
#include "editpanel_class.h"
#include "syspenfield_class.h"

/****************************************************************************************/

#define ForChilds(group) \
{\
    APTR child,cstate;\
    struct MinList *list;\
    get(group,MUIA_Group_ChildList,&list);\
    cstate=list->mlh_Head;\
    while (child=NextObject(&cstate))

#define NextChilds }

struct NewMenu PaletteMenu[] =
{
    { NM_TITLE, (STRPTR)MSG_PALPRES_TITLE, 0 , 0,0,(APTR)0   },
    { NM_ITEM , (STRPTR)MSG_PALPRES_CLONEWB  , 0,0,0,(APTR)10},
    { NM_ITEM , (STRPTR)NM_BARLABEL          , 0,0,0,(APTR)0 },
    { NM_ITEM , (STRPTR)MSG_PALPRES_MAGICWB  , 0,0,0,(APTR)9 },
    { NM_ITEM , (STRPTR)NM_BARLABEL          , 0,0,0,(APTR)0 },
    { NM_ITEM , (STRPTR)MSG_PALPRES_STANDARD , 0,0,0,(APTR)0 },
    { NM_ITEM , (STRPTR)MSG_PALPRES_TINT     , 0,0,0,(APTR)1 },
    { NM_ITEM , (STRPTR)MSG_PALPRES_PHARAO   , 0,0,0,(APTR)2 },
    { NM_ITEM , (STRPTR)MSG_PALPRES_SUNSET   , 0,0,0,(APTR)3 },
    { NM_ITEM , (STRPTR)MSG_PALPRES_OCEAN    , 0,0,0,(APTR)4 },
    { NM_ITEM , (STRPTR)MSG_PALPRES_STEEL    , 0,0,0,(APTR)5 },
    { NM_ITEM , (STRPTR)MSG_PALPRES_CHOCOLATE, 0,0,0,(APTR)6 },
    { NM_ITEM , (STRPTR)MSG_PALPRES_PEWTER   , 0,0,0,(APTR)7 },
    { NM_ITEM , (STRPTR)MSG_PALPRES_WINE     , 0,0,0,(APTR)8 },

    { NM_END,NULL,0,0,0,(APTR)0 },
};

char *CYA_EditPages[] =
{
    (char *)MSG_EDITPAGE_ATTRIBUTES,
    (char *)MSG_EDITPAGE_DISPLAY,
    (char *)MSG_EDITPAGE_COLORS,
    NULL
};

char *CYA_Overscan[] =
{
    (char *)MSG_OVERSCAN_TEXT,
    (char *)MSG_OVERSCAN_GRAPHICS,
    (char *)MSG_OVERSCAN_EXTREME,
    (char *)MSG_OVERSCAN_MAXIMUM,
    NULL
};

/****************************************************************************************/

struct EditPanel_Data
{
    Object *TX_Info;
    /*
    Object *CM_Adjustable;
    */

    Object *GR_EditPages;

    Object *ST_Name;
    Object *ST_Title;
    Object *ST_Font;
    Object *ST_Background;
    Object *CM_AutoScroll;
    Object *CM_NoDrag;
    Object *CM_Exclusive;
    Object *CM_Interleaved;
    Object *CM_Behind;
    Object *CM_SysDefault;
    Object *CM_AutoClose;
    Object *CM_CloseGadget;

    Object *GR_Size;
    Object *LV_Modes;
    Object *LI_Modes;
    Object *CY_Overscan;
    Object *ST_Width;
    Object *ST_Height;
    Object *SL_Depth;
    Object *TX_ModeInfo;

    Object *palette[PSD_NUMCOLS];
    Object *syspens[PSD_NUMSYSPENS];
    /*Object *muipens[PSD_NUMMUIPENS];*/
    Object *ColorMenu;

    LONG update;
};

/****************************************************************************************/

IPTR EditPanel_SetScreen(struct IClass *cl, Object *obj, struct MUIP_EditPanel_SetScreen *msg)
{
    struct EditPanel_Data *data = INST_DATA(cl, obj);
    struct MUI_PubScreenDesc *desc = msg->desc;
    int i;

    set(data->ST_Name      , MUIA_String_Contents, desc->Name      );
    set(data->ST_Title     , MUIA_String_Contents, desc->Title     );
    set(data->ST_Font      , MUIA_String_Contents, desc->Font      );
    set(data->ST_Background, MUIA_String_Contents, desc->Background);

    /*
    set(data->CM_Adjustable ,MUIA_Selected       ,!desc->Foreign   );
    */
    set(data->CM_AutoScroll , MUIA_Selected, desc->AutoScroll );
    set(data->CM_NoDrag     , MUIA_Selected, desc->NoDrag     );
    set(data->CM_Exclusive  , MUIA_Selected, desc->Exclusive  );
    set(data->CM_Interleaved, MUIA_Selected, desc->Interleaved);
    set(data->CM_SysDefault , MUIA_Selected, desc->SysDefault );
    set(data->CM_Behind     , MUIA_Selected, desc->Behind     );
    set(data->CM_AutoClose  , MUIA_Selected, desc->AutoClose  );
    set(data->CM_CloseGadget, MUIA_Selected, desc->CloseGadget);

    set(data->LI_Modes, MUIA_DispIDlist_CurrentID, desc->DisplayID);

    setstr(data->ST_Width, desc->DisplayWidth);
    setstr(data->ST_Height, desc->DisplayHeight);
    set(data->SL_Depth, MUIA_Slider_Level, desc->DisplayDepth);
    set(data->CY_Overscan, MUIA_Cycle_Active, desc->OverscanType);

    for (i = 0; i < PSD_NUMCOLS; i++)
    {
        set(data->palette[i], MUIA_Pendisplay_RGBcolor, &desc->Palette[i]);
    }

    /*
    for (i=0;i<PSD_NUMMUIPENS;i++)
    {
        set(data->muipens[i],MUIA_Pendisplay_Spec,&desc->MUIPens[i]);
    }
    */

    for (i = 0; i < PSD_NUMSYSPENS; i++)
    {
        if (data->syspens[i])
        {
            BYTE p = desc->SystemPens[i];
            p = BETWEEN(0, p, 3) ? p : BETWEEN(-4, p, -1) ? 8 + p : 0;
            set(data->syspens[i], MUIA_Pendisplay_Reference, data->palette[p]);
        }
    }

    return 0;
}

/****************************************************************************************/

IPTR EditPanel_GetScreen(struct IClass *cl, Object *obj, struct MUIP_EditPanel_GetScreen *msg)
{
    struct EditPanel_Data *data = INST_DATA(cl, obj);
    struct MUI_PubScreenDesc *desc = msg->desc;
    int i;

    strcpy(desc->Name      , getstr(data->ST_Name      ));
    strcpy(desc->Title     , getstr(data->ST_Title     ));
    strcpy(desc->Font      , getstr(data->ST_Font      ));
    strcpy(desc->Background, getstr(data->ST_Background));

    /*
    desc->Foreign     = !getbool(data->CM_Adjustable );
    */
    desc->AutoScroll  = getbool(data->CM_AutoScroll );
    desc->NoDrag      = getbool(data->CM_NoDrag     );
    desc->Exclusive   = getbool(data->CM_Exclusive  );
    desc->Interleaved = getbool(data->CM_Interleaved);
    desc->SysDefault  = getbool(data->CM_SysDefault );
    desc->Behind      = getbool(data->CM_Behind     );
    desc->AutoClose   = getbool(data->CM_AutoClose  );
    desc->CloseGadget = getbool(data->CM_CloseGadget);

    desc->DisplayID     = xget(data->LI_Modes, MUIA_DispIDlist_CurrentID);
    desc->DisplayWidth  = atol(getstr(data->ST_Width ));
    desc->DisplayHeight = atol(getstr(data->ST_Height));

    desc->DisplayDepth  = xget(data->SL_Depth, MUIA_Slider_Level);
    desc->OverscanType  = xget(data->CY_Overscan, MUIA_Cycle_Active);

    for (i = 0; i < PSD_NUMCOLS; i++)
    {
        desc->Palette[i] = *((struct MUI_RGBcolor *)xget(data->palette[i], MUIA_Pendisplay_RGBcolor));
    }

    /*
    for (i=0;i<PSD_NUMMUIPENS;i++)
    {
        desc->MUIPens[i] = *((struct MUI_PenSpec *)xget(data->muipens[i],MUIA_Pendisplay_Spec));
    }
    */

    for (i = 0; i < PSD_NUMSYSPENS; i++)
    {
        if (data->syspens[i])
        {
            BYTE p = muiUserData(xget(data->syspens[i], MUIA_Pendisplay_Reference));
            desc->SystemPens[i] = BETWEEN(1, p, 4) ? p - 1 : p - 9;
        }
    }

    return 0;
}

/****************************************************************************************/

IPTR EditPanel_ContextMenuChoice(struct IClass *cl, Object *obj, struct MUIP_ContextMenuChoice *msg)
{
    struct PalettePreset
    {
        struct MUI_RGBcolor col[8];
    };

    static const struct PalettePreset PalettePreset[10] =
    {
        /* def */
        {
            {
            { 0xAAAAAAAA,0xAAAAAAAA,0xAAAAAAAA },
            { 0x00000000,0x00000000,0x00000000 },
            { 0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF },
            { 0x66666666,0x88888888,0xBBBBBBBB },
            { 0xEEEEEEEE,0x44444444,0x44444444 },
            { 0x55555555,0xDDDDDDDD,0x55555555 },
            { 0x00000000,0x44444444,0xDDDDDDDD },
            { 0xEEEEEEEE,0x99999999,0x00000000 },
            }
        },
        /* tint */
        {
            {
            { 0xCCCCCCCC,0xCCCCCCCC,0xBBBBBBBB },
            { 0x00000000,0x00000000,0x33333333 },
            { 0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF },
            { 0x99999999,0xAAAAAAAA,0xBBBBBBBB },
            { 0xEEEEEEEE,0x44444444,0x44444444 },
            { 0x55555555,0xDDDDDDDD,0x55555555 },
            { 0xE0E0E0E0,0xF0F0F0F0,0x88888888 },
            { 0x00000000,0x44444444,0xDDDDDDDD },
            }
        },
        /* pharao */
        {
            {
            { 0x55555555,0xBBBBBBBB,0xAAAAAAAA },
            { 0x00000000,0x00000000,0x22222222 },
            { 0xEEEEEEEE,0xEEEEEEEE,0xFFFFFFFF },
            { 0x55555555,0x77777777,0xAAAAAAAA },
            { 0xF6F6F6F6,0xF6F6F6F6,0x00000000 },
            { 0x62626262,0x51515151,0xF0F0F0F0 },
            { 0x00000000,0xF0F0F0F0,0x00000000 },
            { 0xF0F0F0F0,0x30303030,0x10101010 },
            }
        },
        /* sunset */
        {
            {
            { 0xAAAAAAAA,0x99999999,0x88888888 },
            { 0x33333333,0x22222222,0x11111111 },
            { 0xFFFFFFFF,0xEEEEEEEE,0xEEEEEEEE },
            { 0xFFFFFFFF,0xDDDDDDDD,0xBBBBBBBB },
            { 0xEEEEEEEE,0x44444444,0x44444444 },
            { 0x55555555,0xDDDDDDDD,0x55555555 },
            { 0xCFCFCFCF,0xDBDBDBDB,0xFFFFFFFF },
            { 0x00000000,0x44444444,0xDDDDDDDD },
            }
        },
        /* ocean */
        {
            {
            { 0x88888888,0xAAAAAAAA,0xCCCCCCCC },
            { 0x00000000,0x00000000,0x22222222 },
            { 0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF },
            { 0xFFFFFFFF,0xCCCCCCCC,0x99999999 },
            { 0x00000000,0x00000000,0xF0F0F0F0 },
            { 0xF9F9F9F9,0x21212121,0x21212121 },
            { 0x52525252,0xF2F2F2F2,0x76767676 },
            { 0xDFDFDFDF,0xA5A5A5A5,0x26262626 },
            }
        },
        /* steel */
        {
            {
            { 0x99999999,0xBBBBBBBB,0xDDDDDDDD },
            { 0x00000000,0x00000000,0x22222222 },
            { 0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF },
            { 0x66666666,0x88888888,0xBBBBBBBB },
            { 0xB2B2B2B2,0xDEDEDEDE,0xFFFFFFFF },
            { 0xFFFFFFFF,0xA1A1A1A1,0x1C1C1C1C },
            { 0xF0F0F0F0,0x44444444,0x87878787 },
            { 0xBFBFBFBF,0xFFFFFFFF,0x90909090 },
            }
        },
        /* chocolate */
        {
            {
            { 0xBBBBBBBB,0xAAAAAAAA,0x99999999 },
            { 0x00000000,0x00000000,0x22222222 },
            { 0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF },
            { 0x66666666,0x88888888,0xBBBBBBBB },
            { 0xEEEEEEEE,0x44444444,0x44444444 },
            { 0x55555555,0xDDDDDDDD,0x55555555 },
            { 0x00000000,0x44444444,0xDDDDDDDD },
            { 0xEEEEEEEE,0x99999999,0x00000000 },
            }
        },
        /* pewter */
        {
            {
            { 0x88888888,0xAAAAAAAA,0xCCCCCCCC },
            { 0x00000000,0x00000000,0x22222222 },
            { 0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF },
            { 0xEEEEEEEE,0x99999999,0x77777777 },
            { 0xD9D9D9D9,0xFFFFFFFF,0x09090909 },
            { 0xF0F0F0F0,0x2D2D2D2D,0x31313131 },
            { 0x38383838,0xF2F2F2F2,0x38383838 },
            { 0x44444444,0x49494949,0xF0F0F0F0 },
            }
        },
        /* wine */
        {
            {
            { 0xCCCCCCCC,0x99999999,0x99999999 },
            { 0x00000000,0x00000000,0x22222222 },
            { 0xFFFFFFFF,0xEEEEEEEE,0xEEEEEEEE },
            { 0xBBBBBBBB,0x66666666,0x77777777 },
            { 0xE0E0E0E0,0xF0F0F0F0,0x88888888 },
            { 0x79797979,0x46464646,0xE8E8E8E8 },
            { 0x60606060,0xC7C7C7C7,0x52525252 },
            { 0x89898989,0xEAEAEAEA,0xC8C8C8C8 },
            }
        },
        /* magicwb */
        {
            {
            { 0x95959595,0x95959595,0x95959595 },
            { 0x00000000,0x00000000,0x00000000 },
            { 0xffffffff,0xffffffff,0xffffffff },
            { 0x3b3b3b3b,0x67676767,0xa2a2a2a2 },
            { 0x7b7b7b7b,0x7b7b7b7b,0x7b7b7b7b },
            { 0xafafafaf,0xafafafaf,0xafafafaf },
            { 0xaaaaaaaa,0x90909090,0x7c7c7c7c },
            { 0xffffffff,0xa9a9a9a9,0x97979797 },
            }
        },
    };
    /*{   0, 1, 1, 2, 1, 3, 1, 0, 2, 1, 2, 1 }*/

    LONG i = muiUserData(msg->item);
    struct MUI_PubScreenDesc *desc = NULL;
    struct MUI_RGBcolor *new = NULL;
    struct EditPanel_Data *data = INST_DATA(cl,obj);

    if (i >= 0 && i < 10)
    {
        new = (struct MUI_RGBcolor *)PalettePreset[i].col;
    }
    else if (i == 10)
    {
        if ((desc = MUIS_AllocPubScreenDesc(NULL)))
        {
            new = desc->Palette;
        }
    }

    if (new)
    {
        for (i = 0; i < PSD_NUMCOLS; i++)
        {
            set(data->palette[i], MUIA_Pendisplay_RGBcolor, &new[i]);
        }
    }

    if (desc)
        MUIS_FreePubScreenDesc(desc);

    return 0;
}

/****************************************************************************************/

IPTR EditPanel_New(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct EditPanel_Data tmp = {0};
    Object *l1,*l2;

    obj = (Object *)DoSuperNewTags(cl, obj, NULL,
        MUIA_Group_Horiz, FALSE,

        MUIA_ContextMenu, tmp.ColorMenu = MUI_MakeObject(MUIO_MenustripNM, PaletteMenu, MUIO_MenustripNM_CommandKeyCheck),

        /*
        Child, HGroup, GroupSpacing(1),
            MUIA_Weight, 0,
            */
            Child, tmp.TX_Info = TextObject,
                TextFrame, MUIA_Background, MUII_TextBack, MUIA_Text_PreParse, "\33c",
            End,
            /*
            Child, tmp.CM_Adjustable = MUI_MakeObject(MUIO_Checkmark,NULL),
            End,
            */

        Child, tmp.GR_EditPages = RegisterGroup(CYA_EditPages),
            MUIA_CycleChain, 1,

            Child, ColGroup(2),
                Child, MakeLabel2(MSG_LABEL_PUBLICNAME),
                Child, tmp.ST_Name = MUI_NewObject(MUIC_Popscreen,
                    MUIA_Popstring_String, MakeString(PSD_MAXLEN_NAME, MSG_LABEL_PUBLICNAME),
                    MUIA_Popstring_Button, PopButton(MUII_PopUp),
                TAG_DONE),
                Child, MakeLabel2(MSG_LABEL_SCREENTITLE),
                Child, tmp.ST_Title = MakeString(PSD_MAXLEN_TITLE,MSG_LABEL_SCREENTITLE),
                Child, MakeLabel2(MSG_LABEL_DEFAULTFONT),
                Child, tmp.ST_Font = PopaslObject,
                    MUIA_Popstring_String, MakeString(PSD_MAXLEN_FONT,MSG_LABEL_DEFAULTFONT),
                    MUIA_Popstring_Button, PopButton(MUII_PopUp),
                    MUIA_Popasl_Type, ASL_FontRequest,
                End,
                Child, MakeLabel2(MSG_LABEL_BACKGROUND ),
                Child, tmp.ST_Background = PopaslObject,
                    MUIA_Popstring_String, MakeString(PSD_MAXLEN_BACKGROUND,MSG_LABEL_BACKGROUND),
                    MUIA_Popstring_Button, PopButton(MUII_PopUp),
                    MUIA_Popasl_Type, ASL_FileRequest,
                End,
                Child, VSpace(2), Child, VSpace(2),
                Child, MakeFreeLabel(MSG_LABEL_PUBLICFLAGS),
                Child, HGroup,
                    Child, ColGroup(2),
                        Child, tmp.CM_AutoScroll  = MakeCheck(MSG_LABEL_AUTOSCROLL   ), Child, MakeLLabel1(MSG_LABEL_AUTOSCROLL   ),
                        Child, tmp.CM_NoDrag      = MakeCheck(MSG_LABEL_NODRAG       ), Child, MakeLLabel1(MSG_LABEL_NODRAG       ),
                        Child, tmp.CM_Exclusive   = MakeCheck(MSG_LABEL_EXCLUSIVE    ), Child, MakeLLabel1(MSG_LABEL_EXCLUSIVE    ),
                    End,
                    Child, HSpace(0),
                    Child, ColGroup(2),
                        Child, tmp.CM_AutoClose   = MakeCheck(MSG_LABEL_AUTOCLOSE    ), Child, MakeLLabel1(MSG_LABEL_AUTOCLOSE    ),
                        Child, tmp.CM_Interleaved = MakeCheck(MSG_LABEL_INTERLEAVED  ), Child, MakeLLabel1(MSG_LABEL_INTERLEAVED  ),
                        Child, tmp.CM_Behind      = MakeCheck(MSG_LABEL_OPENBEHIND   ), Child, MakeLLabel1(MSG_LABEL_OPENBEHIND   ),
                    End,
                    Child, HSpace(0),
                    Child, ColGroup(2),
                        Child, tmp.CM_SysDefault  = MakeCheck(MSG_LABEL_SYSTEMDEFAULT), Child, MakeLLabel1(MSG_LABEL_SYSTEMDEFAULT),
                        Child, tmp.CM_CloseGadget = MakeCheck(MSG_LABEL_CLOSEGADGET  ), Child, MakeLLabel1(MSG_LABEL_CLOSEGADGET  ),
                        Child, VSpace(0), Child, VSpace(0),
                    End,
                End,
            End,

            Child, HGroup,
                Child, tmp.LV_Modes = ListviewObject,
                    MUIA_CycleChain, 1,
                    MUIA_Listview_List, tmp.LI_Modes = NewObject(CL_DispIDlist->mcc_Class, NULL,
                        InputListFrame,
                        MUIA_List_AdjustWidth, TRUE,
                    TAG_DONE),
                End,
                Child, VGroup,
                    Child, ScrollgroupObject,
                        MUIA_Scrollgroup_FreeHoriz, FALSE,
                        MUIA_Scrollgroup_Contents, VirtgroupObject,
                            TextFrame,
                            GroupSpacing(0),
                            MUIA_Group_Horiz, TRUE,
                            Child, tmp.TX_ModeInfo = NewObject(CL_DispIDinfo->mcc_Class,NULL,TAG_DONE),
                            Child, HSpace(0),
                        End,
                    End,
                    Child, RectangleObject, MUIA_VertWeight, 1, End,
                    Child, tmp.GR_Size = ColGroup(2),
                        Child, MakeLabel1(MSG_LABEL_OVERSCAN), Child, tmp.CY_Overscan = MakeCycle(CYA_Overscan,MSG_LABEL_OVERSCAN),
                        Child, MakeLabel2(MSG_LABEL_SIZE),
                        Child, HGroup,
                            Child, tmp.ST_Width = MakeString(8,MSG_LABEL_SIZE),
                            Child, MakeLabel2(MSG_LABEL_CROSS),
                            Child, tmp.ST_Height = MakeString(8,MSG_LABEL_CROSS),
                        End,
                        Child, MakeLabel2(MSG_LABEL_DEPTH), Child, tmp.SL_Depth = MakeSlider(1, 24, MSG_LABEL_DEPTH),
                    End,
                End,
            End,

            Child, VGroup,
                Child, ColGroup(3),
                    GroupFrameT(GetStr(MSG_PALETTE_TITLE)),
                    MUIA_Group_VertSpacing, 1,
                    Child, l1 = MakeCLabel(MSG_PALETTE_FIRST),
                    Child, HSpace(4),
                    Child, l2 = MakeCLabel(MSG_PALETTE_LAST),
                    Child, HGroup,
                        Child, tmp.palette[0] = MakePalette(),
                        Child, tmp.palette[1] = MakePalette(),
                        Child, tmp.palette[2] = MakePalette(),
                        Child, tmp.palette[3] = MakePalette(),
                    End,
                    Child, HSpace(4),
                    Child, HGroup,
                        Child, tmp.palette[4] = MakePalette(),
                        Child, tmp.palette[5] = MakePalette(),
                        Child, tmp.palette[6] = MakePalette(),
                        Child, tmp.palette[7] = MakePalette(),
                    End,
                End,
                Child, HGroup,
                    Child, ColGroup(3),
                        GroupFrameT(GetStr(MSG_SYSPENS_TITLE)),
                        Child, MakeSysPen(MSG_SYSPEN_TEXT     ,&tmp.syspens[TEXTPEN         ]),
                        Child, MakeSysPen(MSG_SYSPEN_SHINE    ,&tmp.syspens[SHINEPEN        ]),
                        Child, MakeSysPen(MSG_SYSPEN_SHADOW   ,&tmp.syspens[SHADOWPEN       ]),
                        Child, MakeSysPen(MSG_SYSPEN_FILL     ,&tmp.syspens[FILLPEN         ]),
                        Child, MakeSysPen(MSG_SYSPEN_FILLTEXT ,&tmp.syspens[FILLTEXTPEN     ]),
                        Child, MakeSysPen(MSG_SYSPEN_HIGHLIGHT,&tmp.syspens[HIGHLIGHTTEXTPEN]),
                        Child, MakeSysPen(MSG_SYSPEN_BARDETAIL,&tmp.syspens[BARDETAILPEN    ]),
                        Child, MakeSysPen(MSG_SYSPEN_BARBLOCK ,&tmp.syspens[BARBLOCKPEN     ]),
                        Child, MakeSysPen(MSG_SYSPEN_BARTRIM  ,&tmp.syspens[BARTRIMPEN      ]),
                    End,
                    /*
                    Child, VGroup,
                        GroupFrameT(GetStr(MSG_MUIPENS_TITLE)),
                        Child, ColGroup(3),
                            Child, MakeMUIPen(MSG_MUIPEN_SHINE     ,&tmp.muipens[MPEN_SHINE     ]),
                            Child, MakeMUIPen(MSG_MUIPEN_HALFSHINE ,&tmp.muipens[MPEN_HALFSHINE ]),
                            Child, MakeMUIPen(MSG_MUIPEN_BACKGROUND,&tmp.muipens[MPEN_BACKGROUND]),
                            Child, MakeMUIPen(MSG_MUIPEN_HALFSHADOW,&tmp.muipens[MPEN_HALFSHADOW]),
                            Child, MakeMUIPen(MSG_MUIPEN_SHADOW    ,&tmp.muipens[MPEN_SHADOW    ]),
                            Child, MakeMUIPen(MSG_MUIPEN_TEXT      ,&tmp.muipens[MPEN_TEXT      ]),
                            Child, MakeMUIPen(MSG_MUIPEN_FILL      ,&tmp.muipens[MPEN_FILL      ]),
                            Child, MakeMUIPen(MSG_MUIPEN_MARK      ,&tmp.muipens[MPEN_MARK      ]),
                            End,
                        End,
                    */
                End,
            End,

        End,
        TAG_MORE, msg->ops_AttrList);

    if (obj)
    {
        struct EditPanel_Data *data = INST_DATA(cl, obj);
        int i;

        *data = tmp;

        DoMethod(tmp.LI_Modes   , MUIM_Notify,MUIA_DispIDlist_CurrentID, MUIV_EveryTime, obj, 2,MUIM_EditPanel_Update, 3);
        DoMethod(tmp.CY_Overscan, MUIM_Notify,MUIA_Cycle_Active        , MUIV_EveryTime, obj, 2,MUIM_EditPanel_Update, 2);
        DoMethod(tmp.ST_Width   , MUIM_Notify,MUIA_String_Acknowledge  , MUIV_EveryTime, obj, 2,MUIM_EditPanel_Update, 1);
        DoMethod(tmp.ST_Height  , MUIM_Notify,MUIA_String_Acknowledge  , MUIV_EveryTime, obj, 2,MUIM_EditPanel_Update, 1);
        DoMethod(tmp.SL_Depth   , MUIM_Notify,MUIA_Slider_Level        , MUIV_EveryTime, obj, 2,MUIM_EditPanel_Update, 1);

        /*
        set(tmp.CM_Adjustable,MUIA_Selected,TRUE);
        DoMethod(tmp.CM_Adjustable,MUIM_Notify,MUIA_Selected            ,MUIV_EveryTime,obj,1,MUIM_EditPanel_ToggleForeign);
        */

        set(tmp.TX_Info       , MUIA_ShortHelp, GetStr(MSG_HELP_INFO         ));
        /*
        set(tmp.CM_Adjustable ,MUIA_ShortHelp,GetStr(MSG_HELP_ADJUSTABLE   ));
        */
        set(tmp.ST_Name       , MUIA_ShortHelp, GetStr(MSG_HELP_NAME         ));
        set(tmp.ST_Title      , MUIA_ShortHelp, GetStr(MSG_HELP_TITLE        ));
        set(tmp.ST_Font       , MUIA_ShortHelp, GetStr(MSG_HELP_FONT         ));
        set(tmp.ST_Background , MUIA_ShortHelp, GetStr(MSG_HELP_BACKGROUND   ));
        set(tmp.CM_AutoScroll , MUIA_ShortHelp, GetStr(MSG_HELP_AUTOSCROLL   ));
        set(tmp.CM_NoDrag     , MUIA_ShortHelp, GetStr(MSG_HELP_NODRAG       ));
        set(tmp.CM_Exclusive  , MUIA_ShortHelp, GetStr(MSG_HELP_EXCLUSIVE    ));
        set(tmp.CM_Interleaved, MUIA_ShortHelp, GetStr(MSG_HELP_INTERLEAVED  ));
        set(tmp.CM_Behind     , MUIA_ShortHelp, GetStr(MSG_HELP_BEHIND       ));
        set(tmp.CM_AutoClose  , MUIA_ShortHelp, GetStr(MSG_HELP_AUTOCLOSE    ));
        set(tmp.CM_CloseGadget, MUIA_ShortHelp, GetStr(MSG_HELP_CLOSEGADGET  ));
        set(tmp.CM_SysDefault , MUIA_ShortHelp, GetStr(MSG_HELP_SYSTEMDEFAULT));
        set(tmp.LV_Modes      , MUIA_ShortHelp, GetStr(MSG_HELP_MODELIST     ));
        set(tmp.CY_Overscan   , MUIA_ShortHelp, GetStr(MSG_HELP_OVERSCAN     ));
        set(tmp.ST_Width      , MUIA_ShortHelp, GetStr(MSG_HELP_WIDTH        ));
        set(tmp.ST_Height     , MUIA_ShortHelp, GetStr(MSG_HELP_HEIGHT       ));
        set(tmp.SL_Depth      , MUIA_ShortHelp, GetStr(MSG_HELP_DEPTH        ));
        set(tmp.TX_ModeInfo   , MUIA_ShortHelp, GetStr(MSG_HELP_MODEINFO     ));

        for (i = 0; i < 8; i++)
            set(data->palette[i], MUIA_UserData, i + 1);

        set(l1, MUIA_Font,MUIV_Font_Tiny);
        set(l2, MUIA_Font,MUIV_Font_Tiny);

        /*
        DoMethod(obj,MUIM_EditPanel_ToggleForeign);
        */

        /*
        if (IntuitionBase->lib_Version<39)
            set(tmp.ST_Background,MUIA_Disabled,TRUE);
        */

        return (IPTR)obj;
    }

    return 0;
}

/****************************************************************************************/

IPTR EditPanel_Dispose(struct IClass *cl,Object *obj,Msg msg)
{
    struct EditPanel_Data *data = INST_DATA(cl, obj);

    if (data->ColorMenu)
        MUI_DisposeObject(data->ColorMenu);

    return DoSuperMethodA(cl, obj, msg);
}


/*
ULONG EditPanel_ToggleForeign(struct IClass *cl,Object *obj,Msg msg)
{
    struct EditPanel_Data *data = INST_DATA(cl,obj);
    BOOL disable = !getbool(data->CM_Adjustable);

    if (disable)
        set(data->GR_EditPages,MUIA_Group_ActivePage,2);

    DoMethod(obj,MUIM_EditPanel_Update,1);

    DoMethod(obj,MUIM_MultiSet,MUIA_Disabled,disable,
        data->ST_Title,
        data->ST_Font,
        data->ST_Background,
        data->CM_AutoScroll,
        data->CM_NoDrag,
        data->CM_Exclusive,
        data->CM_Interleaved,
        data->CM_Behind,
        data->CM_SysDefault,
        data->CM_AutoClose,
        data->CM_CloseGadget,
        data->LV_Modes,
        data->CY_Overscan,
        data->ST_Width,
        data->ST_Height,
        data->SL_Depth,
        data->TX_ModeInfo,
        NULL);

    return(0);
}
*/

/****************************************************************************************/

IPTR EditPanel_Update(struct IClass *cl, Object *obj, struct MUIP_EditPanel_Update *msg)
{
    struct EditPanel_Data *data = INST_DATA(cl, obj);
    struct NameInfo *ni;
    struct DimensionInfo DimensionInfo;

    if (data->update)
        return 0;
    data->update = TRUE;

    /*
    if (getbool(data->CM_Adjustable))
    */
    {
        DoMethod(data->LI_Modes, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, &ni);

        if (ni && GetDisplayInfoData(NULL, (char *)&DimensionInfo, sizeof(DimensionInfo), DTAG_DIMS, ni->Header.DisplayID))
        {
            /*set(data->GR_SysPots,MUIA_Coloradjust_ModeID,ni->Header.DisplayID); !!!*/

            if (msg->level >= 1)
            {
                set(data->GR_Size, MUIA_Disabled, FALSE);
                set(data->SL_Depth, MUIA_Slider_Max, DimensionInfo.MaxDepth);
            }

            if (msg->level >= 3)
                set(data->TX_ModeInfo, MUIA_DispIDinfo_ID, ni->Header.DisplayID);

            if (msg->level >= 2)
            {
                int w = RectangleWidth(DimensionInfo.TxtOScan);
                int h = RectangleHeight(DimensionInfo.TxtOScan);

                switch (xget(data->CY_Overscan, MUIA_Cycle_Active))
                {
                    case 1:
                        w = RectangleWidth(DimensionInfo.StdOScan);
                        h = RectangleHeight(DimensionInfo.StdOScan);
                        break;

                    case 2:
                        w = RectangleWidth(DimensionInfo.MaxOScan);
                        h = RectangleHeight(DimensionInfo.MaxOScan);
                        break;

                    case 3:
                        w = RectangleWidth(DimensionInfo.VideoOScan);
                        h = RectangleHeight(DimensionInfo.VideoOScan);
                        break;
                }

                setstr(data->ST_Width , w);
                setstr(data->ST_Height, h);
            }

            if (msg->level >= 1)
                DoMethod
                (
                    data->TX_Info, MUIM_SetAsString, MUIA_Text_Contents, "%s (%ld x %ld x %ld)",
                    ni->Name, atol(getstr(data->ST_Width)), atol(getstr(data->ST_Height)),
                    xget(data->SL_Depth, MUIA_Slider_Level)
                );
        }
        else
        {
            /* set(data->GR_SysPots,MUIA_Coloradjust_ModeID,INVALID_ID); !!!*/
            set(data->TX_ModeInfo, MUIA_DispIDinfo_ID, INVALID_ID);
            set(data->TX_Info, MUIA_Text_Contents, GetStr(MSG_TEXT_UNKNOWNMODE));
            set(data->GR_Size, MUIA_Disabled, TRUE);
        }
    }
    /*
    else
    {
        set(data->TX_Info,MUIA_Text_Contents,GetStr(MSG_TEXT_FOREIGNSCREEN));
    }
    */

    data->update = FALSE;
    return 0;
}

/****************************************************************************************/

BOOPSI_DISPATCHER(IPTR, EditPanel_Dispatcher, cl, obj, msg)
{
    switch (msg->MethodID)
    {
        case OM_NEW                      : return EditPanel_New              (cl,obj,(APTR)msg);
        case OM_DISPOSE                  : return EditPanel_Dispose          (cl,obj,(APTR)msg);

        case MUIM_ContextMenuChoice      : return EditPanel_ContextMenuChoice(cl,obj,(APTR)msg);

        case MUIM_EditPanel_SetScreen    : return EditPanel_SetScreen        (cl,obj,(APTR)msg);
        case MUIM_EditPanel_GetScreen    : return EditPanel_GetScreen        (cl,obj,(APTR)msg);
        case MUIM_EditPanel_Update       : return EditPanel_Update           (cl,obj,(APTR)msg);
        /*
        case MUIM_EditPanel_ToggleForeign: return EditPanel_ToggleForeign    (cl,obj,(APTR)msg);
        */
    }

    return DoSuperMethodA(cl,obj,msg);
}
BOOPSI_DISPATCHER_END

/****************************************************************************************/

VOID EditPanel_Init(VOID)
{
    CL_EditPanel = MUI_CreateCustomClass
    (
        NULL, MUIC_Group, NULL, sizeof(struct EditPanel_Data), EditPanel_Dispatcher
    );
}

/****************************************************************************************/

VOID EditPanel_Exit(VOID)
{
    if (CL_EditPanel)
        MUI_DeleteCustomClass(CL_EditPanel);
}
