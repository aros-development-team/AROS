/*
    Copyright (C) 2003-2026, The AROS Development Team. All rights reserved.
    
*/

#define MUIMASTER_YES_INLINE_STDARG

#include <utility/tagitem.h>
#include <libraries/mui.h>
#include <dos/dos.h>
#include <zune/iconimage.h>
#include <exec/memory.h>
#include <intuition/gadgetclass.h>
#include <intuition/icclass.h>

#include <aros/inquire.h>
#include <proto/aros.h>

#include <proto/exec.h>
#include <proto/muimaster.h>
#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/locale.h>
#include <proto/dos.h>
#include <proto/alib.h>
#include <proto/graphics.h>


#include <string.h>


#include "loginwindow.h"
#include "loginwindow_private.h"

#define CATCOMP_ARRAY
#include "strings.h"

#ifndef DEBUG
#define DEBUG 0
#endif
#include <aros/debug.h>

/*
 * IMAGES: is made by the Startup-Sequence, which has not run yet when the
 * login window is shown at boot; fall back to where the images really are.
 */
#define IMAGES_ASSIGN  "IMAGES:"
#define IMAGES_SYSPATH "SYS:System/Images/"
#define LOGOTYPE_IMAGE "Logos/login.logo"
#define USERTYPE_IMAGE "Gadgets/System/sys_user"
#define IMAGESPEC_MAX  (2 + sizeof(IMAGES_SYSPATH) + sizeof(USERTYPE_IMAGE))

#define ENV    ((IPTR) "ENV:")

/*** Macros and Defines *****************************************************/
#define IGNORE ((APTR)(1UL))

/*** Locale functions *******************************************************/
CONST_STRPTR MSG(struct Catalog *catalog, ULONG id)
{
    if (catalog != NULL)
    {
        return GetCatalogStr(catalog, id, CatCompArray[id].cca_Str);
    }
    else
    {
        return CatCompArray[id].cca_Str;
    }
}

#define _(id) MSG(catalog,id)

static CONST_STRPTR LoginWindow_ImageBase(void)
{
    struct DosList *dl = LockDosList(LDF_ASSIGNS | LDF_READ);
    BOOL assigned = (FindDosEntry(dl, "IMAGES", LDF_ASSIGNS) != NULL);

    UnLockDosList(LDF_ASSIGNS | LDF_READ);

    return assigned ? IMAGES_ASSIGN : IMAGES_SYSPATH;
}

/* Builds a MUI external-image spec ("3:<path>") for one of the images */
static void LoginWindow_ImageSpec(char *spec, CONST_STRPTR base, CONST_STRPTR image)
{
    strcpy(spec, "3:");
    strcat(spec, base);
    strcat(spec, image);
}


/*** Methods ****************************************************************/
Object *LoginWindow__OM_NEW
(
    Class *CLASS, Object *self, struct opSet *message
)
{
    int                     authm_count         = 0,
                            local_login         = 0,
                            nametype            = 0,
                            methodtype          = 0;
    APTR                    pool                = NULL;
    BPTR                    lock                = NULL;
    CONST_STRPTR            imageBase           = LoginWindow_ImageBase();
    char                    logoSpec[IMAGESPEC_MAX],
                            userSpec[IMAGESPEC_MAX];
    IPTR                    logoFrame           = 0,
                            detailsFrame        = 0;
    BOOL                    localLogins         = TRUE,
                            nametypeset         = FALSE,
                            methodtypeset       = FALSE;
    STRPTR                  localLoginStr       = NULL,
                            title               = NULL,
                            logonString         = NULL,
                            user                = NULL,
                            pass                = NULL;
    char                    *authmethodList[20] = { NULL };
    struct LoginWindow_DATA *data               = NULL;
    struct TagItem          *tstate             = message->ops_AttrList,
                            *tag                = NULL;
    struct Catalog          *catalog            = NULL;
    Object                  *methodString       = NULL;
    Object                  *imageGroup         = NULL,
                            *img_logo           = NULL,
                            *img_user           = NULL,
                            *contents           = NULL,
                            *nameUser           = NULL,
                            *passUser           = NULL,
                            *logonMethod        = NULL,
                            *okButton           = NULL,
                            *cancelButton       = NULL,
                            *shutdownButton     = NULL,
                            *rebootButton       = NULL,
                            *buttonGroup        = NULL;
    BOOL                    systemMode          = FALSE;

    /* Allocate memory pool ------------------------------------------------*/
    pool = CreatePool(MEMF_ANY, 4096, 4096);
    if (pool == NULL) return NULL;

    /* Initialize locale ---------------------------------------------------*/
    catalog = OpenCatalogA(NULL, "System/security.catalog", NULL);

    tag = FindTagItem(WindowContents, message->ops_AttrList);

    localLoginStr = StrDup("Local Logon\0");
    title = StrDup(_(MSG_LOGINREQ_GUI));

    D(bug("LOGINWINDOW checking tags..\n"));

    while ((tag = NextTagItem(&tstate)) != NULL)
    {
        switch (tag->ti_Tag)
        {
        case MUIA_LoginWindow_SystemMode:
            systemMode = tag->ti_Data ? TRUE : FALSE;
            break;

        case MUIA_LoginWindow_Prompt:
            if (tag->ti_Data)
                contents = TextObject, MUIA_Text_Contents, (IPTR) StrDup((STRPTR) tag->ti_Data), End;
            break;

        case MUIA_LoginWindow_Title:
            //if ((title)&&(title!=IGNORE)) FreeVec(title);
            title = StrDup((STRPTR) tag->ti_Data);
            if (title == NULL) title = IGNORE;
            break;

        case MUIA_LoginWindow_UserName:
            //if (user) FreeVec(user);
            user = StrDup((STRPTR) tag->ti_Data);
            if (user == NULL) user = StrDup("<USERNAME>");
            break;

        case MUIA_LoginWindow_LoginLogo:
            img_logo = (Object *) tag->ti_Data;
            break;

        case MUIA_LoginWindow_Method:
            authmethodList[authm_count] = StrDup((STRPTR) tag->ti_Data);
            authm_count++;
            break;

        case MUIA_LoginWindow_UserName_Status:
            if (nametypeset) break;
            nametype = tag->ti_Data;
            nametypeset = TRUE;
            break;

        case MUIA_LoginWindow_Method_Status:
            if (methodtypeset) break;
            methodtype = tag->ti_Data;
            methodtypeset = TRUE;
            break;

        case MUIA_LoginWindow_LocalLogin_Disabled:
            localLogins = (BOOL) tag->ti_Data;
            break;

        case MUIA_LoginWindow_LogoFrame:
            logoFrame = (IPTR) tag->ti_Data;
            break;

        case MUIA_LoginWindow_DetailsFrame:
            detailsFrame = (IPTR) tag->ti_Data;
            break;

        case TAG_IGNORE:
            contents    = (Object*) tag->ti_Data;
            break;

        default:
            continue; /* Don't supress non-processed tags */
        }

        tag->ti_Tag = TAG_IGNORE;
    }

    switch(nametype)
    {
    
    case LWA_UNT_Read:
        nameUser =  TextObject,
            TextFrame,
            MUIA_Text_Contents, (IPTR) user,
        End;
        break;
    case LWA_UNT_Disabled:
        nameUser =  StringObject,
            StringFrame,
            MUIA_Disabled, TRUE,
            MUIA_String_AdvanceOnCR,TRUE,
            MUIA_CycleChain, TRUE,
        End;
        break;
    case LWA_UNT_None:
        nameUser = HVSpace;
        break;
    default:
        nameUser =  StringObject,
                    StringFrame,
                    MUIA_String_AdvanceOnCR,TRUE,
                    MUIA_CycleChain,        TRUE,
                End;
        break;
    }

    switch(methodtype)
    {
    case LWA_METH_Disabled:
        logonMethod = PoplistObject,
            MUIA_Disabled, TRUE,
            MUIA_Popstring_String, (IPTR) (methodString = StringObject, StringFrame, End),
            MUIA_Popstring_Button, (IPTR) PopButton(MUII_PopUp),
        End;
        break;
    case LWA_METH_None:
        logonMethod = HVSpace;
        break;
    default:
        logonMethod = PoplistObject,
                    MUIA_Popstring_String, (IPTR) (methodString = StringObject, StringFrame, End),
                    MUIA_Popstring_Button, (IPTR) PopButton(MUII_PopUp),
                End;
        break;
    }

    /* the password input */
    passUser = StringObject,
        StringFrame,
        MUIA_String_Contents,   (IPTR) pass,
        MUIA_String_Secret,     TRUE,
        MUIA_String_AdvanceOnCR,TRUE,
        MUIA_CycleChain,        TRUE,
    End;

    D(bug("LOGINWINDOW checking LoginStr..\n"));

    if (localLoginStr)
    {
        if (localLogins)
        {
            authmethodList[authm_count] = localLoginStr;
            authm_count++;
            local_login = TRUE;
        }
    }

    authmethodList[authm_count] = NULL;

    D(bug("LOGINWINDOW checking contents..\n"));

    if (!contents)
    {
        int                     i;
        char                    tmpversion[8]   ="\0",
                                tmphostname[32] ="\0",
                                strbuff[1024]   = "\0";
        STRPTR                  version         = NULL,
                                hostname        = NULL;

        if (GetVar("Kickstart", &tmpversion[0], 8, GVF_GLOBAL_ONLY) == -1)
        {
            ArosInquire( AI_ArosVersion, (IPTR)&i, TAG_DONE);
            __sprintf(&tmpversion[0],"%d\0",i);
        }
        version = StrDup( &tmpversion[0] );

        if (GetVar("HostName", &tmphostname[0], 32, GVF_GLOBAL_ONLY) == -1) hostname = StrDup( "?" );
        else
        {
            for (i = 0; tmphostname[i] && (tmphostname[i] != '.'); i++);
            {
                if (tmphostname[i] == '.') tmphostname[i] = '\0';
            }
            hostname = StrDup( &tmphostname[0] );

        }

        __sprintf(&strbuff[0],_(MSG_LOGINPROMPT_GUI),version,hostname);

        if (hostname) FreeVec(hostname);
        if (version) FreeVec(version);

        contents = TextObject,
            MUIA_Text_Contents, (IPTR)StrDup(strbuff),
        End;
    }

    D(bug("LOGINWINDOW checking Logo..\n"));

    /* Setup image ---------------------------------------------------------*/
    LoginWindow_ImageSpec(logoSpec, imageBase, LOGOTYPE_IMAGE);
    LoginWindow_ImageSpec(userSpec, imageBase, USERTYPE_IMAGE);

    if (!img_logo)
    {
        if ((lock = Lock(logoSpec + 2, ACCESS_READ)) != NULL)
        {
            img_logo = ImageObject,
                    MUIA_Image_Spec, (IPTR)logoSpec,
                End;

            UnLock(lock);
        }
        else img_logo = HVSpace;
    }
    else img_logo = HVSpace;

    lock = NULL;

    if ((lock = Lock(userSpec + 2, ACCESS_READ)) != NULL)
    {
        img_user = ImageObject,
                MUIA_Image_Spec, (IPTR)userSpec,
            End;

        UnLock(lock);
    }
    else img_user = HVSpace;

    /* The button row: "<space> [Login] [Cancel]", or in system mode (the
     * boot login, nothing to cancel) "[Shutdown] [Reboot] <space> [Login]" */
/* theme images when present (ImageButton falls back to a text button) */
#define LWButton(text, image) ImageButton((text), "THEME:Images/Gadgets/" image)
    if (systemMode)
    {
        buttonGroup = HGroup,
            Child, (IPTR) HGroup,
                MUIA_Group_SameWidth, TRUE,
                MUIA_Weight, 0,
                Child, (IPTR) (shutdownButton = LWButton(_(MSG_SHUTDOWN), "Cancel")),
                Child, (IPTR) (rebootButton   = LWButton(_(MSG_REBOOT), "Revert")),
            End,
            Child, (IPTR) HVSpace,
            Child, (IPTR) HGroup,
                MUIA_Weight, 0,
                Child, (IPTR) (okButton = LWButton(_(MSG_LOGIN), "OK")),
            End,
        End;
    }
    else
    {
        buttonGroup = HGroup,
            Child, (IPTR) HVSpace,
            Child, (IPTR) HGroup,
                MUIA_Group_SameWidth, TRUE,
                MUIA_Weight, 0,
                Child, (IPTR) (okButton     = LWButton(_(MSG_LOGIN), "OK")),
                Child, (IPTR) (cancelButton = LWButton(_(MSG_CANCEL), "Cancel")),
            End,
        End;
    }

    D(bug("LOGINWINDOW Creating window..\n"));

    self = (Object *) DoSuperNewTags
    (
        CLASS, self, NULL,
        MUIA_Window_Title,      (IPTR) title,
        MUIA_Window_Activate,   TRUE,
        MUIA_Window_NoMenus,    TRUE,
        MUIA_Window_CloseGadget, FALSE,

        WindowContents, (IPTR) VGroup,
            Child, (IPTR) HGroup,
                Child, (IPTR) (imageGroup = VGroup,
                    MUIA_Weight,  0,
                    Child, (IPTR) HVSpace,
                    Child, (IPTR) img_logo,
                    Child, (IPTR) HVSpace,
                End),
                Child, (IPTR) VGroup,
                    Child, (IPTR) RectangleObject,
                        MUIA_Weight, 50,
                    End,
                
                    Child, (IPTR) contents,
                    
                    Child, (IPTR) RectangleObject,
                        MUIA_Weight, 50,
                    End,

                    Child, (IPTR) ColGroup(2),
                        MUIA_Group_SameWidth, FALSE,
                        Child, (IPTR) img_user,
                        Child, (IPTR) nameUser,
                        Child, (IPTR) HVSpace,
                        Child, (IPTR) passUser,
                    End,

                    Child, (IPTR) logonMethod,

                    Child, (IPTR) buttonGroup,
                End,
            End,
        End,

        TAG_MORE, (IPTR) message->ops_AttrList
    );

    if (self != NULL)
    {
        D(bug("LOGINWINDOW Window created..\n"));

        data = INST_DATA(CLASS, self);
        data->lwd_Catalog       = catalog;
        data->lwd_Pool          = pool;
        data->lwd_Title         = title;
        data->lwd_OKButton      = okButton;
        data->lwd_CancelButton  = cancelButton;
        data->lwd_ShutdownButton = shutdownButton;
        data->lwd_RebootButton  = rebootButton;
        data->lwd_SystemMode    = systemMode;
        data->lwd_UNInput       = nameUser;
        data->lwd_UPInput       = passUser;

        data->lwd_UserName      = user;
        data->lwd_UserPass      = pass;

        data->lwd_LogonLogo     = img_logo;
        data->lwd_LogonHeader   = contents;
        data->lwd_Method        = logonMethod;
        data->lwd_MethodString  = methodString;
        data->lwd_NameType      = nametype;
        data->lwd_DoMethod      = logonString;

        data->lwd_MethodList    = authmethodList;

        /*-- Handle initial attribute values -------------------------------*/

        if (methodtype != LWA_METH_None && methodString != NULL)
        {
            D(bug("LOGINWINDOW Setting LoginMethods..\n"));
            set(logonMethod, MUIA_Poplist_Array, (IPTR) authmethodList);
            /* preselect the local login when it is the only choice */
            if (local_login && authm_count == 1)
                set(methodString, MUIA_String_Contents, (IPTR) localLoginStr);
        }

        if (!authm_count) set(data->lwd_OKButton, MUIA_Disabled, TRUE);

        if (user)
        {
            switch (nametype)
            {
            case LWA_UNT_Input:
            case LWA_UNT_Disabled:
                D(bug("LOGINWINDOW updateing name string..\n"));
                set(data->lwd_UNInput, MUIA_String_Contents, user);
                break;
            case LWA_UNT_Read:
                D(bug("LOGINWINDOW updateing name text..\n"));
                set(data->lwd_UNInput, MUIA_Text_Contents, user);
                break;
            default:
                break;
            }
        }

        SetAttrsA(self, message->ops_AttrList);

        /*-- Set up the cycle group ----------------------------------------*/

        if (nametype==LWA_UNT_Input) set(nameUser, MUIA_CycleChain, 1);
        set(passUser, MUIA_CycleChain, 1);
        if (methodtype!=LWA_METH_None) set(logonMethod, MUIA_CycleChain, 1);
        /* start in the first editable field */
        set(self, MUIA_Window_ActiveObject, (IPTR) ((nametype == LWA_UNT_Input) ? nameUser : passUser));
        set(okButton, MUIA_CycleChain, 1);
        if (cancelButton) set(cancelButton, MUIA_CycleChain, 1);

        /*-- Setup notifications -------------------------------------------*/

        if (!systemMode)
        {
            DoMethod
            (
                self, MUIM_Notify, MUIA_Window_CloseRequest, TRUE,
                MUIV_Notify_Application, 2, MUIM_Application_ReturnID, LWA_RV_CANCEL
            );
            DoMethod
            (
                cancelButton, MUIM_Notify, MUIA_Pressed, FALSE,
                MUIV_Notify_Application, 2, MUIM_Application_ReturnID, LWA_RV_CANCEL
            );
        }
        else
        {
            DoMethod
            (
                shutdownButton, MUIM_Notify, MUIA_Pressed, FALSE,
                MUIV_Notify_Application, 2, MUIM_Application_ReturnID, LWA_RV_SHUTDOWN
            );
            DoMethod
            (
                rebootButton, MUIM_Notify, MUIA_Pressed, FALSE,
                MUIV_Notify_Application, 2, MUIM_Application_ReturnID, LWA_RV_REBOOT
            );
        }
        DoMethod
        (
            okButton, MUIM_Notify, MUIA_Pressed, FALSE,
            MUIV_Notify_Application, 2, MUIM_Application_ReturnID, LWA_RV_OK
        );
        /* Return in the password field is the same as OK */
        DoMethod
        (
            passUser, MUIM_Notify, MUIA_String_Acknowledge, MUIV_EveryTime,
            MUIV_Notify_Application, 2, MUIM_Application_ReturnID, LWA_RV_OK
        );
    }
    else
    {
        D(bug("LOGINWINDOW failed to create window..\n"));
        if (catalog != NULL) CloseCatalog(catalog);
    }

    return self;
}

IPTR LoginWindow__OM_DISPOSE
(
    Class *CLASS, Object *self, Msg message
)
{
    struct LoginWindow_DATA *data = INST_DATA(CLASS, self);
    char                    *authmethodList[20];
    UBYTE                    i;
    APTR                     ptrs[] =
    {
        data->lwd_Title,
        data->lwd_UserName,
        data->lwd_UserPass,
        data->lwd_DoMethod
    };

//   *authmethodList = data->lwd_MethodList;
//   i = 0;
//
//   while( authmethodList[i++] )FreeVec(authmethodList[i-1]);

    for (i = 0; i < (sizeof(ptrs) / sizeof(APTR)); i++)
    {
        if (ptrs[i] != NULL) FreeVec(ptrs[i]);
    }

    if (data->lwd_Pool != NULL) DeletePool(data->lwd_Pool);

    if (data->lwd_Catalog != NULL) CloseCatalog(data->lwd_Catalog);

    return DoSuperMethodA(CLASS, self, message);
}

IPTR LoginWindow__OM_SET
(
    Class *CLASS, Object *self, struct opSet *message
)
{
    struct LoginWindow_DATA *data   = INST_DATA(CLASS, self);
    struct TagItem                *tstate = message->ops_AttrList,
                                  *tag;

    while ((tag = NextTagItem(&tstate)) != NULL)
    {
        switch (tag->ti_Tag)
        {
        case MUIA_LoginWindow_UserName:
            if ((IPTR)data->lwd_UserName != (IPTR)tag->ti_Data)
            {
                if (data->lwd_UserName != NULL) FreeVec(data->lwd_UserName);
                data->lwd_UserName = StrDup((STRPTR) tag->ti_Data);
            }
            break;

        case MUIA_LoginWindow_Cancel_Disabled:
            if (data->lwd_CancelButton)
                set(data->lwd_CancelButton, MUIA_Disabled, tag->ti_Data);
            break;
        }
    }
    
    return DoSuperMethodA(CLASS, self, (Msg) message);
}

IPTR LoginWindow__OM_GET
(
    Class *CLASS, Object *self, struct opGet *message
)
{
    struct LoginWindow_DATA *data  = INST_DATA(CLASS, self);
    IPTR                          *store = message->opg_Storage;
    IPTR                           rv    = TRUE;

    switch (message->opg_AttrID)
    {

    /* the live gadget contents (valid until the object is disposed) */
    case MUIA_LoginWindow_Method:
        *store = data->lwd_MethodString ? XGET(data->lwd_MethodString, MUIA_String_Contents) : (IPTR) NULL;
        break;

    case MUIA_LoginWindow_UserName:
        if (data->lwd_NameType == LWA_UNT_Read)
            *store = XGET(data->lwd_UNInput, MUIA_Text_Contents);
        else if (data->lwd_NameType == LWA_UNT_None)
            *store = (IPTR) data->lwd_UserName;
        else
            *store = XGET(data->lwd_UNInput, MUIA_String_Contents);
        break;

    case MUIA_LoginWindow_UserPass:
        *store = XGET(data->lwd_UPInput, MUIA_String_Contents);
        break;

    case MUIA_LoginWindow_Cancel_Disabled:
        *store = data->lwd_CancelButton ? XGET(data->lwd_CancelButton, MUIA_Disabled) : TRUE;
        break;
        
    default:
        rv = DoSuperMethodA(CLASS, self, (Msg) message);
    }
    
    return rv;
}
