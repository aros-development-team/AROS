/*
    Copyright © 2003-2004, The AROS Development Team. All rights reserved.
    This file is part of the LoginWindow class, which is distributed under
    the terms of version 2.1 of the GNU Lesser General Public License.
    
    $Id$
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

#define DEBUG 1
#include <aros/debug.h>

#define LOGOTYPE_IMAGE "IMAGES:login.png"
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
    APTR                    pool;
    BPTR                    lock;
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
                            *authmethodTags     = NULL,
                            *tag                = NULL;    
    struct Catalog          *catalog            = NULL;
    Object                  *imageGroup         = NULL,
                            *logo               = NULL,
                            *contents           = NULL,
                            *nameUser           = NULL,
                            *passUser           = NULL,
                            *logonMethod        = NULL,
                            *logonmethStr       = NULL,
                            *okButton           = NULL,
                            *cancelButton       = NULL,
                            *logoFrame          = NULL,
                            *detailsFrame       = NULL;

    /* Allocate memory pool ------------------------------------------------*/
    pool = CreatePool(MEMF_ANY, 4096, 4096);
    if (pool == NULL) return NULL;

    /* Initialize locale ---------------------------------------------------*/
    catalog = OpenCatalogA(NULL, "System/security.catalog", NULL);
    
    tag = FindTagItem(WindowContents, message->ops_AttrList);
    
    localLoginStr = StrDup("Local Logon\0");
    title = StrDup(_(MSG_LOGINREQ_GUI));

    kprintf("LOGINWINDOW checking tags..\n");

    while ((tag = NextTagItem(&tstate)) != NULL)
    {
        switch (tag->ti_Tag)
        {
        case MUIA_LoginWindow_Title:
            if ((title)&&(title!=IGNORE)) FreeVec(title);
            title = StrDup((STRPTR) tag->ti_Data);
            if (title == NULL) title = IGNORE;
            break;

        case MUIA_LoginWindow_UserName:
            if (user) FreeVec(user);
            user = StrDup((STRPTR) tag->ti_Data);
            if (user == NULL) user = StrDup("<USERNAME>");
            break;

        case MUIA_LoginWindow_LoginLogo:
            logo = (Object *) tag->ti_Data;
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
            logoFrame = (Object *) tag->ti_Data;
            break;

        case MUIA_LoginWindow_DetailsFrame:
            detailsFrame = (Object *) tag->ti_Data;
            break;

        case TAG_IGNORE:
            contents    = (Object *) tag->ti_Data;
            break;

        default:
            continue; // Don't supress non-processed tags 
        }

        tag->ti_Tag = TAG_IGNORE;
    }

    switch(nametype)
    {
    
    case LWA_UNT_Read:
        nameUser =  TextObject,
	    MUIA_Text_Editable, FALSE,
	    MUIA_Text_Multiline, FALSE,
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
		    MUIA_CycleChain,	    TRUE,
	        End;
        break;
    }

    switch(methodtype)
    {
    case LWA_METH_Disabled:
        logonMethod = PoplistObject,
            MUIA_Disabled, TRUE,
	    MUIA_Popstring_String, logonmethStr = StringObject, StringFrame, End,
	    MUIA_Popstring_Button,  PopButton(MUII_PopUp),
        End;
        break;
    case LWA_METH_None:
        methodtype = 1;
        logonMethod = HVSpace;
        break;
    default:
        logonMethod = PoplistObject,
		    MUIA_Popstring_String,  logonmethStr = StringObject, StringFrame, End,
		    MUIA_Popstring_Button,  PopButton(MUII_PopUp),
                End;
        break;
    }

    kprintf("LOGINWINDOW checking LoginStr..\n");

    if (localLoginStr)
    {
        if (localLogins)
        {
            authmethodList[authm_count] = localLoginStr;
            authm_count++;
            local_login = TRUE;
        }
    }
    
    authmethodList[authm_count] = NULL; /* make sure we end on NULL */

    kprintf("LOGINWINDOW checking contents..\n");

    if (!contents) contents = HVSpace;

    if (!contents)
    {
        int                     i;
	char                    tmpversion[8]   ="\0",
	                        tmphostname[32] ="\0",
                                strbuff[1024]   = "\0";
        STRPTR                  version         = NULL,
                                hostname        = NULL;

	if (GetVar("Kickstart", &tmpversion, 8, GVF_GLOBAL_ONLY) == -1) 
        {
            ArosInquire( AI_ArosVersion, (ULONG)&i, TAG_DONE);
            __sprintf(&tmpversion,"%d\0",i);
        }
        version = StrDup( &tmpversion );

        if (GetVar("HostName", &tmphostname, 32, GVF_GLOBAL_ONLY) == -1) hostname = StrDup( "?" );
	else 
        {
	    for (i = 0; tmphostname[i] && (tmphostname[i] != '.'); i++);
            {
                if (tmphostname[i] == '.') tmphostname[i] = '\0';
            }
            hostname = StrDup( &tmphostname );            

        }
        
        __sprintf(&strbuff,_(MSG_LOGINPROMPT_GUI),version,hostname);

        //if (hostname) FreeVec(hostname);
        //if (version) FreeVec(version);

        contents = TextObject,
            MUIA_Text_Contents, (IPTR)StrDup(strbuff),
        End;
    }

    kprintf("LOGINWINDOW checking Logo..\n");

    /* Setup image ---------------------------------------------------------*/
    if (!logo)
    {
        if ((lock = Lock(LOGOTYPE_IMAGE, ACCESS_READ)) != NULL)
        {
            logo = ImageObject,
                    MUIA_Image_Spec, (IPTR)"3:"LOGOTYPE_IMAGE,
                End;

            UnLock(lock);
        }
        else logo = HVSpace;
    }

    kprintf("LOGINWINDOW Creating window..\n");

    self = (Object *) DoSuperNewTags
    (
        CLASS, self, NULL,
    	MUIA_Window_Title,	(IPTR)title,
        MUIA_Window_Activate,   TRUE,
        MUIA_Window_NoMenus,    TRUE,
        MUIA_Window_CloseGadget, FALSE,

        WindowContents, (IPTR) VGroup,
            Child, (IPTR) HGroup,
                Child, (IPTR) imageGroup = VGroup,
                    logoFrame
                    ? logoFrame
                    : TAG_IGNORE
                    ,
                    MUIA_Weight,  0,
                    Child, (IPTR) HVSpace,
                    Child, (IPTR) logo,
                    Child, (IPTR) HVSpace,
                End,
                Child, (IPTR) VGroup,
                    detailsFrame
                    ? detailsFrame
                    : TAG_IGNORE
                    ,
                    Child, (IPTR) RectangleObject,
                        MUIA_Weight, 50,
                    End,
                
                    Child, (IPTR) contents,
                    
                    Child, (IPTR) RectangleObject,
                        MUIA_Weight, 50,
                    End,
                    
                    Child, (IPTR) nameUser,

                    Child, (IPTR) passUser      = StringObject,
			StringFrame,
		        MUIA_String_Contents,	(IPTR)pass,
                        MUIA_String_Secret,     TRUE,
			MUIA_String_AdvanceOnCR,TRUE,
			MUIA_CycleChain,	TRUE,
		    End,

    	    	    Child, (IPTR)logonMethod,

                    Child, (IPTR) HGroup,
                        MUIA_Group_SameWidth, TRUE,
                        MUIA_Weight,             0,
                        Child, (IPTR) okButton   = ImageButton(_(MSG_OK), "THEME:Images/Gadgets/Preferences/Test.png"),
                        Child, (IPTR) cancelButton = ImageButton(_(MSG_RESUME), "THEME:Images/Gadgets/Preferences/Revert.png"),
                    End,
                End, 
            End,  
        End, 

        TAG_MORE, (IPTR) message->ops_AttrList
    );

    if (self != NULL)
    {
        kprintf("LOGINWINDOW Window created..\n");
        
        data = INST_DATA(CLASS, self);
        data->lwd_Catalog       = catalog;
        data->lwd_Pool          = pool;
        data->lwd_Title         = title;
        data->lwd_OKButton      = okButton;
        data->lwd_CancelButton  = cancelButton;
        data->lwd_UNInput       = nameUser;
        data->lwd_UPInput       = passUser;

        data->lwd_UserName      = user;
        data->lwd_UserPass      = pass;

        data->lwd_LogonLogo     = logo;
        data->lwd_LogonHeader   = contents;
        data->lwd_Method        = logonMethod;
        data->lwd_DoMethod      = logonString;

        data->lwd_MethodList    = authmethodList;

        /*-- Handle initial attribute values -------------------------------*/

        if (methodtype!=LWA_METH_None)
        {
            kprintf("LOGINWINDOW Setting LoginMethods..\n");
            //select the local login text
            if (logonString != NULL) FreeVec(logonString);
            logonString = StrDup((STRPTR) localLoginStr);
            //and then prevent changes to it
            SET(data->lwd_DoMethod, MUIA_Poplist_Array, (IPTR)authmethodList);
            if ((local_login)&&(authm_count == 1)) SET(data->lwd_DoMethod, MUIA_String_Contents, logonString);
        }

        if (!authm_count) SET(data->lwd_OKButton, MUIA_Disabled, TRUE);

        if (user)
        {
            switch (nametype)
            {
            case LWA_UNT_Input:
            case LWA_UNT_Disabled:
                kprintf("LOGINWINDOW updateing name string..\n");
                SET(data->lwd_UNInput, MUIA_String_Contents, (IPTR)user);
                break;
            case LWA_UNT_Read:
                kprintf("LOGINWINDOW updateing name text..\n");
                SET(data->lwd_UNInput, MUIA_Text_Contents, (IPTR)(user));
                break;
            default:
                break;
            }
        }

        //SetAttrsA(self, message->ops_AttrList);

        /*-- Set up the cycle group ----------------------------------------*/

        if (nametype==LWA_UNT_Input) SET(nameUser, MUIA_CycleChain, 1);
        SET(passUser, MUIA_CycleChain, 1);
        //if (methodtype!=LWA_METH_None) SET(logonMethod, MUIA_CycleChain, 1);
        SET(okButton, MUIA_CycleChain, 1);
        SET(cancelButton, MUIA_CycleChain, 1);

        /*-- Setup notifications -------------------------------------------*/

        DoMethod
        ( 
            self, MUIM_Notify, MUIA_Window_CloseRequest, TRUE, 
            (IPTR) cancelButton, 2, MUIA_Pressed, FALSE 
        );
        
        DoMethod
        ( 
            okButton, MUIM_Notify, MUIA_Pressed, FALSE, 
            (IPTR) self, 2, MUIM_Application_ReturnID, LWA_RV_OK
        );
        DoMethod
        ( 
            cancelButton, MUIM_Notify, MUIA_Pressed, FALSE, 
            (IPTR) self, 2, MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit
        );
    }
    else
    {
        kprintf("LOGINWINDOW failed to create window..\n");
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

    *authmethodList = data->lwd_MethodList;
    i = 0;

    while( authmethodList[i++] )FreeVec(authmethodList[i-1]);

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
            if (data->lwd_UserName != tag->ti_Data)
            {
                if (data->lwd_UserName != NULL) FreeVec(data->lwd_UserName);
                data->lwd_UserName = StrDup((CONST_STRPTR) tag->ti_Data);
            }
            break;

        case MUIA_LoginWindow_Cancel_Disabled:
            SET(data->lwd_CancelButton, MUIA_Disabled, tag->ti_Data);
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
    
    case MUIA_LoginWindow_Method:
        *store = (IPTR) data->lwd_DoMethod;
        break;

    case MUIA_LoginWindow_UserName:
        *store = (IPTR) data->lwd_UserName;
        break;   
    
    case MUIA_LoginWindow_UserPass:
        *store = (IPTR) data->lwd_UserPass;
        break;

    case MUIA_LoginWindow_Cancel_Disabled:
        *store = XGET(data->lwd_CancelButton, MUIA_Disabled);
        break;
        
    default:
        rv = DoSuperMethodA(CLASS, self, (Msg) message);
    }
    
    return rv;
}
