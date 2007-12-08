/*
**  OpenURL - MUI preferences for openurl.library
**
**  Written by Troels Walsted Hansen <troels@thule.no>
**  Placed in the public domain.
**
**  Developed by:
**  - Alfonso Ranieri <alforan@tin.it>
**  - Stefan Kost <ensonic@sonicpulse.de>
**
**  Ported to OS4 by Alexandre Balaban <alexandre@balaban.name>
**
**  About window
*/


#include "OpenURL.h"

#define CATCOMP_NUMBERS
#include "loc.h"

#include "OpenURL_rev.h"
#include "libraries/openurl.h"

/***********************************************************************/
/*
** These makes the About window look nicer
*/

#ifndef MUIA_Window_ShowPopup
#define MUIA_Window_ShowPopup 0x8042324e
#endif

#ifndef MUIA_Window_ShowSnapshot
#define MUIA_Window_ShowSnapshot 0x80423c55
#endif

#ifndef MUIA_Window_ShowPrefs
#define MUIA_Window_ShowPrefs 0x8042e262
#endif

#ifndef MUIA_Window_ShowIconify
#define MUIA_Window_ShowIconify 0x8042bc26
#endif

#ifndef MUIA_Window_ShowAbout
#define MUIA_Window_ShowAbout 0x80429c1e
#endif

#ifndef MUIA_Window_ShowJump
#define MUIA_Window_ShowJump 0x80422f40
#endif

#ifndef MUIA_Window_Frontdrop
#define MUIA_Window_Frontdrop 0x80426411
#endif

/***********************************************************************/
/*
** Some local MUI macro
*/

#define ovfixspace RectangleObject, MUIA_FixHeightTxt, " ", End
#define ohfixspace RectangleObject, MUIA_FixWidthTxt, " ", End
#define otextitem  TextObject, MUIA_Text_Contents, "-", MUIA_Text_SetMax, TRUE, End
#define hbar       RectangleObject, MUIA_Weight, 0, MUIA_Rectangle_HBar, TRUE, End

/***********************************************************************/
/*
** If Urltext.mcc is present use it,
** otherwise falls back to a text object
*/

static Object *
ourltext(STRPTR url,STRPTR text)
{
    Object *o = NULL;

#ifndef __AROS__
    o = UrltextObject,
        MUIA_Urltext_Text,           text,
        MUIA_Urltext_Url,            url,
        MUIA_Urltext_SetMax,         FALSE,
        MUIA_Urltext_NoOpenURLPrefs, TRUE,
    End;
#endif

    if (!o) o = TextObject, MUIA_Text_SetMax, FALSE, MUIA_Text_Contents, text ? text : url, End;

    return o;
}

/***********************************************************************/
/*
** Create a miny tiny About MUI button
*/

static Object *
othirdMUI(Object **mui)
{
    return HGroup,
        MUIA_Group_HorizSpacing, 0,
        Child, ohfixspace,
        Child, otextitem,
        Child, ohfixspace,
        Child, *mui = TextObject,
            ButtonFrame,
            MUIA_Background,     MUII_ButtonBack,
            MUIA_InputMode,      MUIV_InputMode_RelVerify,
            MUIA_Font,           MUIV_Font_Button,
            MUIA_ControlChar,    'm',
            MUIA_CycleChain,     TRUE,
            MUIA_Text_Contents,  "_MUI",
            MUIA_Text_PreParse,  MUIX_C,
            MUIA_Text_HiCharIdx, '_',
            MUIA_Text_SetMax,    TRUE,
        End,
        Child, ollabel(MSG_About_OfCourse),
        Child, HSpace(0),
    End;
}

/***********************************************************************/
/*
** Third object
*/

static Object *
othird(ULONG stuff,STRPTR author,STRPTR url)
{
    Object *g, *o[8];

    if (g = HGroup,
            MUIA_Group_HorizSpacing, 0,
            Child, o[0] = ohfixspace,
            Child, o[1] = otextitem,
            Child, o[2] = ohfixspace,
            Child, o[3] = ollabel(stuff),
            Child, o[4] = ohfixspace,
            Child, o[5] = HSpace(0),
        End)
    {
        Object *u;

        if (author && *author)
            if (url && *url) u = ourltext(url,author);
            else u = LLabel((ULONG)author);
        else u = NULL;

        if (u)
        {
            DoMethod(g,OM_ADDMEMBER,(ULONG)u);
            DoMethod(g,MUIM_Group_Sort,(ULONG)o[0],(ULONG)o[1],(ULONG)o[2],
                                       (ULONG)o[3],(ULONG)o[4],(ULONG)u,(ULONG)o[5],NULL);
        }
    }

    return g;
}

/***********************************************************************/
/*
** Here we go
*/

static ULONG
mNew(struct IClass *cl,Object *obj,struct opSet *msg)
{
    Object         *g, *o[8], *mui, *ok;
    struct TagItem *attrs = msg->ops_AttrList;
    STRPTR          lver;

    if (!URL_GetAttr(URL_GetAttr_VerString,(ULONG*)(&lver))) lver = "";

    if (obj = (Object *)DoSuperNew(cl,obj,
            MUIA_HelpNode,              "WRID",
            MUIA_Window_Title,          getString(MSG_About_WinTitle),
        	MUIA_Window_ScreenTitle, 	getString(MSG_App_ScreenTitle),
	        MUIA_Window_ShowIconify,    FALSE,
	        MUIA_Window_ShowPopup,      FALSE,
	        MUIA_Window_ShowSnapshot,   FALSE,
	        MUIA_Window_ShowPrefs,      FALSE,
	        MUIA_Window_SizeGadget,     FALSE,
	        MUIA_Window_CloseGadget,    FALSE,
            MUIA_Window_AllowTopMenus,  FALSE,
            MUIA_Window_ShowJump,       FALSE,

            WindowContents, VGroup,

                Child, TextObject,
                    MUIA_Font, MUIV_Font_Big,
                    MUIA_Text_Contents, PRG,
                    MUIA_Text_PreParse, MUIX_B MUIX_C,
                End,

                Child, TextObject,
                    MUIA_Text_Contents, getString(MSG_About_Descr),
                    MUIA_Text_PreParse, MUIX_C,
                End,

                Child, ovfixspace,
                //Child, VSpace(0),

                Child, MUI_MakeObject(MUIO_BarTitle,(ULONG)getString(MSG_About_Info)),

                Child, HGroup,
                    Child, HSpace(0),
                    Child, ColGroup(2),

                        /* Authors */
                        Child, VGroup,
                            Child, HGroup, Child, HSpace(0), Child, olabel(MSG_About_Authors), End,
                            Child, VSpace(0),
                        End,
                        Child, VGroup,
                            Child, HGroup, Child, ourltext("mailto:troels@thule.no","Troels Walsted Hansen"), Child, HSpace(0), End,
                            Child, HGroup, Child, ourltext("mailto:alforan@tin.it","Alfonso Ranieri"), Child, HSpace(0), End,
                            Child, HGroup, Child, ourltext("mailto:ensonic@sonicpulse.de","Stefan Kost"), Child, HSpace(0), End,
                            Child, HGroup, Child, ourltext("mailto:alexandrec@balaban.name","OS4 Port by Alexandre Balaban"), Child, HSpace(0), End,
                        End,

                        /* Support */
                        Child, olabel(MSG_About_Support),
                        Child, ourltext("https://sourceforge.net/projects/openurllib",NULL),

                        /* Version */
                        Child, VGroup,
                            Child, HGroup, Child, HSpace(0), Child, olabel(MSG_About_Version), End,
                            Child, VSpace(0),
                        End,
                        Child, VGroup,
                            Child, HGroup,
                                Child, LLabel((ULONG)PRGNAME),
                                Child, HSpace(0),
                            End,
                            Child, HGroup,
                                Child, LLabel((ULONG)lver),
                                Child, HSpace(0),
                            End,
                        End,
                    End,
                    Child, HSpace(0),
                End,

                Child, ovfixspace,
                //Child, VSpace(0),

                Child, MUI_MakeObject(MUIO_BarTitle,(ULONG)getString(MSG_About_ThirdParts)),

                Child, g = VGroup,
                    Child, o[0] = othirdMUI(&mui),
                End,

                Child, ovfixspace,
                //Child, VSpace(0),
                Child, hbar,

                Child, HGroup,
                    Child, wspace(200),
                    Child, ok = obutton(MSG_About_OK,0),
                    Child, wspace(200),
                End,
            End,

            TAG_MORE,attrs))
    {
        Object *win, *space;
        STRPTR  tn;

        /*
        ** If there are the translator stuff
        */
        if ((tn = getString(MSG_About_Translation)) && *tn)
        {
            Object *to;

            if (to = othird(MSG_About_Translation,getString(MSG_About_Translator),getString(MSG_About_TranslatorURL)))
            {
                DoMethod(g,OM_ADDMEMBER,(ULONG)to);
                DoMethod(g,MUIM_Group_Sort,(ULONG)to,(ULONG)o[0],NULL);
            }
        }

        if (space = ovfixspace) DoMethod(g,OM_ADDMEMBER,(ULONG)space);

    	set(obj,MUIA_Window_ActiveObject,ok);

        win = (Object *)GetTagData(MUIA_Window_RefWindow,(ULONG)NULL,attrs);
        DoMethod(mui,MUIM_Notify,MUIA_Pressed,FALSE,MUIV_Notify_Application,2,MUIM_Application_AboutMUI,(ULONG)win);

        DoMethod(ok,MUIM_Notify,MUIA_Pressed,FALSE,(ULONG)obj,3,MUIM_Set,MUIA_Window_CloseRequest,TRUE);
    }

    return (ULONG)obj;
}

/***********************************************************************/

M_DISP(dispatcher)
{
    M_DISPSTART

    switch(msg->MethodID)
    {
        case OM_NEW: return mNew(cl,obj,(APTR)msg);
        default:     return DoSuperMethodA(cl,obj,msg);
    }
}

M_DISPEND(dispatcher)

/***********************************************************************/

ULONG
initAboutClass(void)
{
    return (ULONG)(g_aboutClass = MUI_CreateCustomClass(NULL,MUIC_Window,NULL,0,DISP(dispatcher)));
}

/***********************************************************************/

void
disposeAboutClass(void)
{
    if (g_aboutClass) MUI_DeleteCustomClass(g_aboutClass);
}

/***********************************************************************/

