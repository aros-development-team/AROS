
#include <proto/intuition.h>
#include <proto/muimaster.h>
#include <proto/utility.h>

#include <libraries/mui.h>

#include "muimiamipanel_intern.h"
#include "muimiamipanel_locale.h"
#include "muimiamipanel_misc.h"

/***********************************************************************/

#ifndef MUIA_Window_MenuGadget
#define MUIA_Window_MenuGadget       0x8042324E
#endif

#ifndef MUIA_Window_SnapshotGadget
#define MUIA_Window_SnapshotGadget   0x80423C55
#endif

#ifndef MUIA_Window_ConfigGadget
#define MUIA_Window_ConfigGadget     0x8042E262
#endif

#ifndef MUIA_Window_IconifyGadget
#define MUIA_Window_IconifyGadget    0x8042BC26
#endif

/***********************************************************************/

#define MUIButton\
    TextObject,\
        ButtonFrame,\
        MUIA_Background,     MUII_ButtonBack,\
        MUIA_InputMode,      MUIV_InputMode_RelVerify,\
        MUIA_Font,           MUIV_Font_Button,\
        MUIA_ControlChar,    'm',\
        MUIA_CycleChain,     TRUE,\
        MUIA_Text_Contents,  "_MUI",\
        MUIA_Text_PreParse,  MUIX_C,\
        MUIA_Text_HiCharIdx, '_',\
        MUIA_Text_SetMax,    TRUE

#define ThirdPart(stuff, author, url)\
Child, ohfixspace(),\
Child, otextitem(),\
Child, ohfixspace(),\
Child, HGroup,\
    MUIA_Group_HorizSpacing,0,\
    Child, olabel(stuff, MiamiPanelBaseIntern),\
    Child, ohfixspace(),\
    Child, ourlText(url, author, MiamiPanelBaseIntern),\
    Child, HSpace(0),\
End

#define ThirdMUI \
Child, ohfixspace(),\
Child, otextitem(),\
Child, ohfixspace(),\
Child, HGroup,\
    MUIA_Group_HorizSpacing,0,\
    Child, mui = MUIButton, End,\
    Child, olabel(MSG_About_OfCourse, MiamiPanelBaseIntern),\
    Child, HSpace(0),\
End

/***********************************************************************/

static struct MiamiPanelBase_intern *MiamiPanelBaseIntern;

IPTR MUIPC_About__OM_NEW
(
    Class *CLASS, Object *self, struct opSet *message
)
{
    Object         *g, *mui;
    struct TagItem *attrs = message->ops_AttrList;

    if (self = (Object *)DoSuperNewTags
		(
			CLASS, self, NULL,

			MUIA_HelpNode,              "WRID",
            MUIA_Window_ID,             MAKE_ID('A','B','O','T'),
            MUIA_Window_Title,          __(MSG_About_WinTitle),
            MUIA_Window_IconifyGadget,  FALSE,
            MUIA_Window_MenuGadget,     FALSE,
            MUIA_Window_SnapshotGadget, FALSE,
            MUIA_Window_ConfigGadget,   FALSE,
            MUIA_Window_SizeGadget,     FALSE,

            WindowContents, VGroup,
                MUIA_Background, MUII_TextBack,

                Child, TextObject,
                    MUIA_Text_Contents, MUIX_C MUIX_B "MUI.MiamiPanel",
                End,

                Child, TextObject,
                    MUIA_Text_Contents, __(MSG_Copyright),
                    MUIA_Text_PreParse, MUIX_C MUIX_B,
                End,

                Child, ovfixspace(),

                Child, obartitle(MSG_About_Information, MiamiPanelBaseIntern),

                Child, HGroup,
                    Child, HSpace(0),
                    Child, ColGroup(2),
                        /*Child, olabel(MSG_About_Version),
                        Child, LLabel((ULONG)lib_vers),*/
                        Child, olabel(MSG_About_Author, MiamiPanelBaseIntern),
                        Child, ourlText("mailto:"DEF_EMail, DEF_Author, MiamiPanelBaseIntern),
                        Child, olabel(MSG_About_Support, MiamiPanelBaseIntern),
                        Child, ourlText(DEF_HomePage, NULL, MiamiPanelBaseIntern),
                    End,
                    Child, HSpace(0),
                End,

                Child, ovfixspace(),

                Child, obartitle(MSG_About_ThirdParts, MiamiPanelBaseIntern),

                Child, g = VGroup,
                    MUIA_Group_HorizSpacing, 0,
                    MUIA_Group_Columns,      4,
                        ThirdPart(MSG_About_Busy, "Klaus Melchior"," mailto:kmel@eifel.oche.de"),
                        ThirdPart(MSG_About_Lamp, "Maik Schreiber", "mailto:BLiZZeR@dame.de"),
                        ThirdMUI,
                End,
        End,
        TAG_MORE,attrs))
    {
        Object *app, *space;
        UBYTE  *tn;

        if ((tn = __(MSG_About_Translation)) && *tn)
        {
            Object *sp1, *tti, *sp2 = NULL, *tg; // gcc

            if ((sp1 = ohfixspace()) &&
                (tti = otextitem()) &&
                (sp2 = ohfixspace()) &&
                (tg= HGroup, Child, Label((ULONG)tn), Child, HSpace(0),End))
            {
                DoMethod(g, OM_ADDMEMBER, (ULONG)sp1);
                DoMethod(g, OM_ADDMEMBER, (ULONG)tti);
                DoMethod(g, OM_ADDMEMBER, (ULONG)sp2);
                DoMethod(g, OM_ADDMEMBER, (ULONG)tg);
            }
            else
                if (sp1)
                {
                    if (tti)
                    {
                        if (sp2) MUI_DisposeObject(sp2);
                        MUI_DisposeObject(tti);
                    }
                    MUI_DisposeObject(sp1);
                }
        }

        if (space = ovfixspace()) DoMethod(g, OM_ADDMEMBER, (ULONG)space);

        if (app = (Object *)GetTagData(MPA_Application, NULL, attrs))
            DoMethod(mui, MUIM_Notify, MUIA_Pressed, FALSE, (ULONG)app, 2, MUIM_Application_AboutMUI, GetTagData(MUIA_Window_RefWindow, NULL, attrs));
    }

    return (ULONG)self;
}

/***********************************************************************/

BOOPSI_DISPATCHER(IPTR, MUIPC_About_Dispatcher, CLASS, self, message)
{
    switch(message->MethodID)
    {
        case OM_NEW: return MUIPC_About__OM_NEW(CLASS, self, (struct opSet *)message);
        default:     return DoSuperMethodA(CLASS, self, message);
    }
	return 0;
}
BOOPSI_DISPATCHER_END

/***********************************************************************/

ULONG
MUIPC_About_ClassInit(struct MiamiPanelBase_intern *MiamiPanelBase)
{
	MiamiPanelBaseIntern = MiamiPanelBase;
    return (ULONG)(MiamiPanelBaseIntern->mpb_aboutClass = MUI_CreateCustomClass(NULL, MUIC_Window, NULL, 0, MUIPC_About_Dispatcher));
}

/***********************************************************************/
