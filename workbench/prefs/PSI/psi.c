/*
    Copyright © 1995-1997 Stefan Stuntz.
    Copyright © 2009-2011, The AROS Development Team.
    All rights reserved.

    $Id$
*/

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/gadtools.h>
#include <proto/locale.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>
#include <proto/muiscreen.h>

#include <string.h>
#include <stdlib.h>

#include "psi.h"
#include "displayidinfo_class.h"
#include "displayidlist_class.h"
#include "editpanel_class.h"
#include "editwindow_class.h"
#include "mainwindow_class.h"
#include "screenlist_class.h"
#include "screenpanel_class.h"
#include "syspenfield_class.h"

/****************************************************************************/
/* Global Vars                                                              */
/****************************************************************************/

const char VersionString[] = "$VER: PSI 20.1 (10.06.2010)";
struct Catalog *Catalog;

/****************************************************************************/
/* Locale Stuff                                                             */
/****************************************************************************/

#define CATALOG_NAME     "System/Prefs/PSI.catalog"
#define CATALOG_VERSION  19

/****************************************************************************************/

CONST_STRPTR GetStr(int num)
{
    struct CatCompArrayType *cca = (struct CatCompArrayType *)CatCompArray;
    while (cca->cca_ID != num) cca++;
    if (LocaleBase)
        return GetCatalogStr(Catalog,num,cca->cca_Str);

    return (char *)cca->cca_Str;
}

/****************************************************************************************/

VOID LocalizeStringArray(CONST_STRPTR *array)
{
    CONST_STRPTR *x;
    for (x = array; *x; x++)
        *x = GetStr((int)(IPTR)*x);
}

/****************************************************************************************/

VOID LocalizeNewMenu(struct NewMenu *nm)
{
    for ( ; nm->nm_Type != NM_END; nm++)
        if (nm->nm_Label != NM_BARLABEL)
            nm->nm_Label = GetStr((int)(IPTR)nm->nm_Label);
}

/****************************************************************************************/

VOID InitLocale(VOID)
{
    Catalog = OpenCatalog(NULL, CATALOG_NAME, OC_Version, CATALOG_VERSION, TAG_DONE);

    LocalizeNewMenu(MainMenu);
    LocalizeNewMenu(PaletteMenu);
    /*LocalizeNewMenu(EditMenu);*/
    LocalizeStringArray(CYA_Overscan);
    LocalizeStringArray(CYA_EditPages);
}

/****************************************************************************************/

VOID ExitLocale(VOID)
{
    if (Catalog)
    {
        CloseCatalog(Catalog);
        Catalog = NULL;
    }
}


/****************************************************************************/
/* Misc Help Functions                                                      */
/****************************************************************************/

IPTR xget(Object *obj, IPTR attribute)
{
    IPTR x = 0;
    get(obj, attribute, &x);
    return x;
}

/****************************************************************************************/

char *getstr(Object *obj)
{
    return (char *)xget(obj,MUIA_String_Contents);
}

/****************************************************************************************/

BOOL getbool(Object *obj)
{
    return (BOOL)xget(obj, MUIA_Selected);
}

/****************************************************************************************/

VOID setstr(APTR str, LONG num)
{
    DoMethod(str, MUIM_SetAsString, MUIA_String_Contents, "%ld", num);
}

/****************************************************************************************/

VOID settxt(APTR str, LONG num)
{
    DoMethod(str, MUIM_SetAsString, MUIA_Text_Contents, "%ld", num);
}

/****************************************************************************************/

Object *MakeCheck(int num)
{
    Object *obj = MUI_MakeObject(MUIO_Checkmark, GetStr(num));
    if (obj)
        set(obj, MUIA_CycleChain, 1);

    return obj;
}

/****************************************************************************************/

Object *MakeButton(int num)
{
    Object *obj = MUI_MakeObject(MUIO_Button, GetStr(num));
    if (obj)
        set(obj, MUIA_CycleChain, 1);

    return obj;
}

/****************************************************************************************/

/*
Object *MakeBackground(int num)
{
    Object *obj;

    obj = MUI_NewObject(MUIC_Popimage,
        MUIA_Imageadjust_Type, MUIV_Imageadjust_Type_Background,
        MUIA_Window_Title, "Adjust Screen Background",
        MUIA_CycleChain, 1,
        TAG_DONE);

    return(obj);
}
*/

/****************************************************************************************/

Object *MakeString(int maxlen,int num)
{
    Object *obj = MUI_MakeObject(MUIO_String, GetStr(num), maxlen);

    if (obj)
    {
        SetAttrs
        (
            obj,
            MUIA_CycleChain, 1,
            MUIA_String_AdvanceOnCR, TRUE,
            TAG_DONE
        );
    }

    return obj;
}

/****************************************************************************************/

Object *MakeCycle(CONST_STRPTR *array, int num)
{
    Object *obj = MUI_MakeObject(MUIO_Cycle, GetStr(num), array);
    if (obj)
        set(obj, MUIA_CycleChain, 1);

    return obj;
}

/****************************************************************************************/

Object *MakeSlider(int min, int max, int num)
{
    Object *obj = MUI_MakeObject(MUIO_Slider, GetStr(num), min,max);
    if (obj)
        set(obj, MUIA_CycleChain, 1);

    return obj;
}

/****************************************************************************************/

Object *MakeCLabel(int num)
{
    return MUI_MakeObject(MUIO_Label, GetStr(num), MUIO_Label_Centered);
}

/****************************************************************************************/

Object *MakeLabel(int num)
{
    return MUI_MakeObject(MUIO_Label, GetStr(num),0);
}

/****************************************************************************************/

Object *MakeLabel1(int num)
{
    return MUI_MakeObject(MUIO_Label, GetStr(num), MUIO_Label_SingleFrame);
}

/****************************************************************************************/

Object *MakeLabel2(int num)
{
    return MUI_MakeObject(MUIO_Label, GetStr(num), MUIO_Label_DoubleFrame);
}

Object *MakeLLabel(int num)
{
    return MUI_MakeObject(MUIO_Label, GetStr(num), MUIO_Label_LeftAligned);
}

/****************************************************************************************/

Object *MakeLLabel1(int num)
{
    return MUI_MakeObject(MUIO_Label, GetStr(num), MUIO_Label_LeftAligned | MUIO_Label_SingleFrame);
}

Object *MakeFreeLabel(int num)
{
    return MUI_MakeObject(MUIO_Label, GetStr(num), MUIO_Label_FreeVert);
}

/****************************************************************************************/

Object *MakeFreeLLabel(int num)
{
    return MUI_MakeObject(MUIO_Label, GetStr(num), MUIO_Label_FreeVert | MUIO_Label_LeftAligned);
}

/****************************************************************************************/

BOOL TestPubScreen(char *name)
{
    struct List *pslist;
    struct PubScreenNode *psn,*succ;
    BOOL res = FALSE;
    pslist = LockPubScreenList();
    ForEntries(pslist,psn,succ)
    {
        if (!stricmp(psn->psn_Node.ln_Name,name))
        {
            res = TRUE;
            break;
        }
    }
    UnlockPubScreenList();

    return res;
}


/****************************************************************************/
/* Init/Exit Functions                                                      */
/****************************************************************************/

VOID ExitClasses(VOID)
{
    DispIDinfo_Exit();
    DispIDlist_Exit();
    EditPanel_Exit();
    EditWindow_Exit();
    MainWindow_Exit();
    ScreenList_Exit();
    ScreenPanel_Exit();
    SysPenField_Exit();
}

/****************************************************************************************/

BOOL InitClasses(VOID)
{
    DispIDinfo_Init();
    DispIDlist_Init();
    EditPanel_Init();
    EditWindow_Init();
    MainWindow_Init();
    ScreenList_Init();
    ScreenPanel_Init();
    SysPenField_Init();

    if
    (
        CL_SysPenField && CL_EditWindow && CL_EditPanel && CL_DispIDlist
        && CL_DispIDinfo && CL_ScreenList && CL_ScreenPanel && CL_MainWindow
    )
        return TRUE;

    ExitClasses();

    return FALSE;
}

/****************************************************************************************/

const char CLITemplate[] = "NAME,OPEN/S,CLOSE/S";

const char CLIHelp[] = "\
\n\
Usage: PSI <name> OPEN/CLOSE\n\
<name>: name of (preconfigured) public screen\n\
 OPEN : open this public screen\n\
 CLOSE: close this public screen\n\
";

/****************************************************************************************/

LONG HandleArgs(Object *mainwin)
{
    struct MUI_PubScreenDesc *desc;
    struct RDArgs *rda,*rdas;
    LONG msg = 0;
    struct CLIArgs
    {
        char *Name;
        IPTR Open;
        IPTR Close;
    } argarray = { 0,0,0 };

    if ((rdas = AllocDosObject(DOS_RDARGS, NULL)))
    {
        rdas->RDA_ExtHelp = (char *)CLIHelp;

        if ((rda = ReadArgs((char *)CLITemplate, (IPTR *)&argarray,rdas)))
        {
            if (argarray.Name)
            {
                DoMethod(mainwin, MUIM_ScreenList_Find, argarray.Name, &desc);

                if (argarray.Open)
                {
                    if (!desc)
                        msg = MSG_CLI_SCREENNOTFOUND;
                    else if (MUIS_OpenPubScreen(desc))
                        msg = MSG_CLI_SCREENOPENED;
                    else
                        msg = MSG_CLI_SCREENOPENFAILED;
                }
                else if (argarray.Close)
                {
                    if (!desc)
                        msg = MSG_CLI_SCREENNOTFOUND;
                    else if (MUIS_ClosePubScreen(desc->Name))
                        msg = MSG_CLI_SCREENCLOSED;
                    else
                        msg = MSG_CLI_SCREENCLOSEFAILED;
                }
            }
            else
            {
                if (argarray.Open || argarray.Close)
                    msg = MSG_CLI_SYNTAXERROR;
            }
            FreeArgs(rda);
        }
        FreeDosObject(DOS_RDARGS,rdas);
    }
    else
        msg = MSG_CLI_OUTOFMEMORY;

    return msg;
}


/****************************************************************************/
/* Main Program                                                             */
/****************************************************************************/

int main(int argc, char *argv[])
{
    struct MUIS_InfoClient sic;
    ULONG sigs=0;
    Object *app;
    Object *win;
    int res;
    int msg;

    InitLocale();

    if (InitClasses())
    {
        app = ApplicationObject,
            MUIA_Application_Title      , "PSI",
            MUIA_Application_Version    , VersionString,
            MUIA_Application_Copyright  , "©1995-97, Stefan Stuntz",
            MUIA_Application_Author     , "Stefan Stuntz",
            MUIA_Application_Description, "Public Screen Inspector",
            MUIA_Application_Base       , "PSI",
            MUIA_Application_Window     , win = NewObject(CL_MainWindow->mcc_Class, NULL, TAG_DONE),
        End;

        if (app)
        {
            if (argc == 0)
                msg = 0;
            else
                msg = HandleArgs(win);

            if (!msg)
            {
                set(win, MUIA_Window_Open, TRUE);

                /* special magic to keep track about public screen open/close status */
                sic.task   = FindTask(NULL);
                sic.sigbit = SIGBREAKF_CTRL_E;
                MUIS_AddInfoClient(&sic);

                while (DoMethod(app, MUIM_Application_NewInput, &sigs) != MUIV_Application_ReturnID_Quit)
                {
                    if (sigs)
                    {
                        sigs = Wait(sigs | SIGBREAKF_CTRL_C | SIGBREAKF_CTRL_E | SIGBREAKF_CTRL_F);

                        /* quit when receiving break from console */
                        if (sigs & SIGBREAKF_CTRL_C)
                            break;

                        /* update listview whenever a screen was opened/closed */
                        if (sigs & SIGBREAKF_CTRL_E)
                            DoMethod(win, MUIM_ScreenPanel_Update);

                        /* deiconify & activate on ctrl-f just like the other prefs programs */
                        if (sigs & SIGBREAKF_CTRL_F)
                        {
                            set(app, MUIA_Application_Iconified, FALSE);
                            set(win, MUIA_Window_Open, TRUE);
                        }
                    }
                }

                MUIS_RemInfoClient(&sic);

                DoMethod(win, MUIM_ScreenPanel_CloseWindows);
                set(win, MUIA_Window_Open, FALSE);
            }
            MUI_DisposeObject(app);
        }
        else msg = MSG_CLI_NOAPPLICATION;
        ExitClasses();
    }
    else msg = MSG_CLI_OUTOFMEMORY;

    if (msg)
    {
        CONST_STRPTR str = GetStr(msg);
        char *c = strchr(str,'(');
        Write(Output(),str,strlen(str));
        res = c ? atol(c+1) : RETURN_OK;
    }
    else
        res = RETURN_OK;

    ExitLocale();
    /*return(res);*/
    exit(res);
}
