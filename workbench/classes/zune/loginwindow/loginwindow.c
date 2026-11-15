/*
    Copyright (C) 2003-2026, The AROS Development Team. All rights reserved.
    
*/

/****** LoginWindow.mcc/MUIC_LoginWindow ************
*
*   NAME
*       MUIC_LoginWindow
*
*   SUPERCLASS
*       Window.mui
*
*   DESCRIPTION
*       Collect a username, password, and login-method string in a window. The
*       configured class version is 1.3. Its 14 attributes are declared
*       protected in loginwindow.h. This class supplies a user interface
*       only: the application must validate the values and perform
*       authentication or system actions.
*       Ordinary mode provides Login and Cancel buttons; system mode provides
*       Shutdown, Reboot, and Login. Button release posts an Application return
*       ID. Acknowledging the password field also posts LWA_RV_OK, independently
*       of the Login button state. Ordinary-mode CloseRequest posts
*       LWA_RV_CANCEL; system mode installs no CloseRequest notification. The
*       class does not invoke its declared OK or Cancel methods.
*       LWA_RV_OK is 1, LWA_RV_SHUTDOWN is 2, and LWA_RV_REBOOT is 3.
*       LWA_RV_CANCEL equals MUIV_Application_ReturnID_Quit. LWRV_OK and
*       LWRV_CANCEL are compatibility aliases. Handle these IDs in the
*       application's input loop; they are requests, not evidence that
*       credentials were accepted or the requested action occurred.
*
*   NOTES
*       Construction defaults to an active window without menus or a close
*       gadget. It builds its own WindowContents. Do not supply replacement
*       WindowContents: remaining caller tags are forwarded to the superclass
*       and can conflict with the internal hierarchy. Recognized constructor
*       tags are permanently changed to TAG_IGNORE; supply writable tag storage.
*       The first username-status and method-status tags determine their
*       controls. Most other repeated settings replace earlier values; Method
*       tags accumulate.
*       Labels use the security catalog with compiled fallbacks. The default
*       prompt includes the global Kickstart variable, or the AROS version, and
*       the HostName before its first dot, or a question mark. Default images
*       use IMAGES: when that assign exists, otherwise SYS:System/Images/.
*       Individual missing images become spacers; an existing assign does not
*       cause a second search in the fallback path. Buttons use
*       THEME:Images/Gadgets images.
*       Use within a live MUI Application. Get returns borrowed pointers
*       to control contents or stored strings; ownership is not transferred.
*       Copy values before further editing or object disposal. Password masking affects presentation; this class provides no
*       password authentication, encryption, or explicit memory wiping.
*
*   BUGS
*       Method entries are sent through MUIA_Poplist_Array after the Poplist has
*       already been created. At this AROS source revision, Poplist reads that
*       attribute only in OM_NEW, so this does not populate its list. The sole
*       local-login choice can still prefill the method string. Treat the
*       returned method as text to validate, not as a verified selection.
*       Several declared attributes have no effective implementation; see their
*       individual entries. Construction and allocation failures are not fully
*       cleaned up. Method and prompt copies are not explicitly freed by this
*       class, and repeated string tags can leak earlier copies. The stored
*       method-array address is not durable storage and is not used by the
*       getter. Do not rely on failure-atomic construction.
*
*/
/****** LoginWindow.mcc/MUIA_LoginWindow_Title ************
*
*   NAME
*       MUIA_LoginWindow_Title
*
*   FUNCTION
*       Set the window title during construction.
*
*   INPUTS
*       Initialize; CONST_STRPTR. Default: the localized Security Login Request
*       title.
*
*   NOTES
*       Supply a non-NULL readable NUL-terminated string. The constructor
*       duplicates it and stores the final copy for disposal. NULL or copy
*       failure is not handled safely; no reliable fallback is provided for an
*       explicit Title tag. There is no local setter or getter for this
*       attribute after construction. Use the Window interface for subsequent
*       window-title operations.
*
*/
/****** LoginWindow.mcc/MUIA_LoginWindow_Prompt ************
*
*   NAME
*       MUIA_LoginWindow_Prompt
*
*   FUNCTION
*       Set the text above the input controls.
*
*   INPUTS
*       Initialize; CONST_STRPTR. Default: the generated version/hostname
*       prompt.
*
*   NOTES
*       A non-NULL string is duplicated and passed to a Text object. NULL
*       supplies no replacement. There is no local setter or getter. Supply one
*       prompt tag; repeated tags can abandon previously created objects and
*       copies. The class does not explicitly release its duplicated prompt
*       string.
*
*/
/****** LoginWindow.mcc/MUIA_LoginWindow_SystemMode ************
*
*   NAME
*       MUIA_LoginWindow_SystemMode
*
*   FUNCTION
*       Choose the button row and return-ID notifications.
*
*   INPUTS
*       Initialize; BOOL. Default FALSE; any nonzero value selects system mode.
*
*   NOTES
*       TRUE replaces Cancel with Shutdown and Reboot. Those buttons post
*       LWA_RV_SHUTDOWN and LWA_RV_REBOOT to the Application; the application
*       decides what to do. Ordinary mode posts LWA_RV_CANCEL for Cancel and
*       CloseRequest. No local setter or getter exists after construction. This
*       flag does not confer privileges or execute a system action.
*
*/
/****** LoginWindow.mcc/MUIA_LoginWindow_LoginLogo ************
*
*   NAME
*       MUIA_LoginWindow_LoginLogo
*
*   FUNCTION
*       Specify the nominal custom logo slot.
*
*   INPUTS
*       Initialize; Object pointer. Default NULL.
*
*   NOTES
*       The current implementation does not install a supplied non-NULL object:
*       it replaces the slot with a spacer. That supplied object is not adopted
*       or disposed by this class. NULL uses the default login.logo image when
*       its file can be locked, otherwise a spacer. No local setter or getter is
*       implemented.
*
*/
/****** LoginWindow.mcc/MUIA_LoginWindow_LogoFrame ************
*
*   NAME
*       MUIA_LoginWindow_LogoFrame
*
*   FUNCTION
*       Declare the logo-frame setting.
*
*   INPUTS
*       Initialize; IPTR frame value. Default zero.
*
*   NOTES
*       The constructor consumes the tag and stores its value only in an unused
*       local variable. It applies no frame. No local setter or getter is
*       implemented; do not infer visible behavior from this declaration.
*
*/
/****** LoginWindow.mcc/MUIA_LoginWindow_DetailsFrame ************
*
*   NAME
*       MUIA_LoginWindow_DetailsFrame
*
*   FUNCTION
*       Declare the details-frame setting.
*
*   INPUTS
*       Initialize; IPTR frame value. Default zero.
*
*   NOTES
*       The constructor consumes the tag and stores its value only in an unused
*       local variable. It applies no frame. No local setter or getter is
*       implemented.
*
*/
/****** LoginWindow.mcc/MUIA_LoginWindow_UserName_Status ************
*
*   NAME
*       MUIA_LoginWindow_UserName_Status
*
*   FUNCTION
*       Choose the username control.
*
*   INPUTS
*       Initialize; ULONG. Default LWA_UNT_Input.
*
*   NOTES
*       LWA_UNT_Input (0) creates an editable String; LWA_UNT_Read (1) creates
*       Text; LWA_UNT_Disabled (2) creates a disabled String; LWA_UNT_None (3)
*       creates a spacer. The first supplied status wins. Use one of these
*       constants; other values take an inconsistent fallback path and are not a
*       supported mode. Initial focus is the username only for Input, otherwise
*       the password. No local setter or getter exists.
*
*/
/****** LoginWindow.mcc/MUIA_LoginWindow_UserName ************
*
*   NAME
*       MUIA_LoginWindow_UserName
*
*   FUNCTION
*       Supply an initial username and read its current value.
*
*   INPUTS
*       Initialize, set, get; CONST_STRPTR. Initially no stored username.
*
*   NOTES
*       Construction duplicates the string and fills Input, Disabled, or Read
*       controls. If the explicit string is NULL or cannot be copied, it
*       attempts to copy <USERNAME>. None mode retains only the stored copy.
*       After construction, Set changes only the stored copy, not the displayed
*       input or text. It frees the old copy before duplicating unless the
*       pointer is exactly the stored pointer. NULL clears the stored value;
*       copy failure loses it. Get returns live Text contents in Read mode, the
*       stored copy in None mode, and live String contents otherwise. Thus a
*       post-construction set is reflected by this getter only in None mode.
*       Returned pointers are borrowed; do not modify/free them or retain them
*       across relevant changes or disposal.
*
*/
/****** LoginWindow.mcc/MUIA_LoginWindow_UserPass ************
*
*   NAME
*       MUIA_LoginWindow_UserPass
*
*   FUNCTION
*       Read the current password input.
*
*   INPUTS
*       Get; CONST_STRPTR. The password input starts empty.
*
*   NOTES
*       The getter returns borrowed String contents. The control uses
*       MUIA_String_Secret TRUE. There is no local initializer or setter for
*       this attribute; passing a UserPass tag does not initialize the password.
*       Copy the returned value if it must survive editing or disposal. No
*       explicit secret-memory clearing is implemented by this class.
*
*/
/****** LoginWindow.mcc/MUIA_LoginWindow_UserPass_Disabled ************
*
*   NAME
*       MUIA_LoginWindow_UserPass_Disabled
*
*   FUNCTION
*       Declare password-input disabling.
*
*   INPUTS
*       Declared BOOL attribute; no local initialize, set, or get
*       implementation.
*
*   NOTES
*       This attribute is not handled by the constructor, setter, or getter. Do
*       not rely on it to disable the password input. Unknown tags and queries
*       are delegated to Window; this class defines no resulting value.
*
*/
/****** LoginWindow.mcc/MUIA_LoginWindow_Method_Status ************
*
*   NAME
*       MUIA_LoginWindow_Method_Status
*
*   FUNCTION
*       Choose the method control.
*
*   INPUTS
*       Initialize; ULONG. Default LWA_METH_Input.
*
*   NOTES
*       LWA_METH_Input (0) creates a Poplist with an editable String;
*       LWA_METH_Disabled (1) disables the Poplist; LWA_METH_None (2) creates a
*       spacer and leaves no method String. The first supplied status wins. Use
*       the declared constants. In None mode the Method getter returns NULL. No
*       local setter or getter exists for the status attribute.
*
*/
/****** LoginWindow.mcc/MUIA_LoginWindow_Method ************
*
*   NAME
*       MUIA_LoginWindow_Method
*
*   FUNCTION
*       Supply nominal method choices and read the current method text.
*
*   INPUTS
*       Initialize, get; CONST_STRPTR. Repeat the constructor tag to supply
*       method names.
*
*   NOTES
*       Constructor strings are duplicated. Keep the number of caller-supplied
*       methods within 18 when the local entry is enabled (the default), or 19
*       when omitted; the implementation uses a fixed-capacity array and does
*       not enforce its capacity. Supply non-NULL readable strings.
*       The list-population call occurs too late for the pinned AROS Poplist
*       implementation, so these entries do not populate its list. If local
*       login is the sole entry, its text is prefilled. Otherwise no initial
*       selection is established. Get returns live borrowed method String
*       contents, or NULL for Method_Status None. There is no local setter after
*       construction. The string can be edited and must be validated by the
*       caller.
*
*/
/****** LoginWindow.mcc/MUIA_LoginWindow_LocalLogin_Disabled ************
*
*   NAME
*       MUIA_LoginWindow_LocalLogin_Disabled
*
*   FUNCTION
*       Control inclusion of the built-in Local Logon entry.
*
*   INPUTS
*       Initialize; BOOL. Default TRUE in the local implementation.
*
*   NOTES
*       Despite the attribute name, nonzero includes the entry and zero omits
*       it. The label is the literal Local Logon, not a catalog string. If it is
*       the only entry and a method String exists, that String is prefilled. No
*       local setter or getter is implemented. This affects UI preparation only;
*       it neither enables nor disables an authentication backend.
*
*/
/****** LoginWindow.mcc/MUIA_LoginWindow_Cancel_Disabled ************
*
*   NAME
*       MUIA_LoginWindow_Cancel_Disabled
*
*   FUNCTION
*       Control the Cancel button disabled state.
*
*   INPUTS
*       Initialize, set, get; BOOL. Ordinary mode uses the button default unless
*       supplied.
*
*   NOTES
*       Set forwards MUIA_Disabled to the Cancel button when it exists. Get
*       returns that button state, or TRUE in system mode where no Cancel button
*       exists. It does not disable ordinary-mode CloseRequest notifications. It
*       has no effect on password acknowledgement, Login, Shutdown, or Reboot.
*
*/
/****** LoginWindow.mcc/MUIM_LoginWindow_OK ************
*
*   NAME
*       MUIM_LoginWindow_OK
*
*   SYNOPSIS
*       IPTR result = DoMethod(obj, MUIM_LoginWindow_OK);
*
*   FUNCTION
*       Declare an abstract OK action.
*
*   INPUTS
*       obj - A subclass that implements this method. No payload structure is
*       declared.
*
*   RESULT
*       No base-class result is defined; LoginWindow has no handler for this
*       method.
*
*   NOTES
*       The built-in UI sends Application return IDs directly and never invokes
*       this abstract method. Overriding it alone does not intercept the
*       LoginWindow buttons or password acknowledgement. A subclass must arrange
*       its own invocation if it uses this declaration.
*
*/
/****** LoginWindow.mcc/MUIM_LoginWindow_Cancel ************
*
*   NAME
*       MUIM_LoginWindow_Cancel
*
*   SYNOPSIS
*       IPTR result = DoMethod(obj, MUIM_LoginWindow_Cancel);
*
*   FUNCTION
*       Declare an abstract Cancel action.
*
*   INPUTS
*       obj - A subclass that implements this method. No payload structure is
*       declared.
*
*   RESULT
*       No base-class result is defined; LoginWindow has no handler for this
*       method.
*
*   NOTES
*       The built-in UI sends Application return IDs directly and never invokes
*       this abstract method. Overriding it alone does not intercept the
*       LoginWindow buttons or password acknowledgement. A subclass must arrange
*       its own invocation if it uses this declaration.
*
*/

#define MUIMASTER_YES_INLINE_STDARG

#include <utility/tagitem.h>
#include <libraries/mui.h>
#include <libraries/security.h>
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
/* Return a borrowed localized string for a valid CatCompArray index, or the
 * compiled fallback when the catalog is absent. No index validation or
 * allocation occurs. Catalog-backed strings must not outlive the catalog.
 */
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

/* Lock the DOS assign list for reading, test whether IMAGES exists, unlock,
 * and return one of two static path strings. This checks assign presence,
 * not individual image availability. No returned storage is caller-owned.
 */
static CONST_STRPTR LoginWindow_ImageBase(void)
{
    struct DosList *dl = LockDosList(LDF_ASSIGNS | LDF_READ);
    BOOL assigned = (FindDosEntry(dl, "IMAGES", LDF_ASSIGNS) != NULL);

    UnLockDosList(LDF_ASSIGNS | LDF_READ);

    return assigned ? IMAGES_ASSIGN : IMAGES_SYSPATH;
}

/* Builds a MUI external-image spec ("3:<path>") for one of the images */
/* Build the external-image specification 3: followed by base and image.
 * Require readable NUL-terminated input strings and a writable destination
 * large enough for all bytes and the final terminator. The fixed
 * constructor buffers are sized for the known image paths. No capacity
 * check or allocation occurs.
 */
static void LoginWindow_ImageSpec(char *spec, CONST_STRPTR base, CONST_STRPTR image)
{
    strcpy(spec, "3:");
    strcat(spec, base);
    strcat(spec, image);
}


/*** Methods ****************************************************************/
/* Create a pool/catalog and all login controls; consume recognized tags by
 * changing their IDs to TAG_IGNORE in writable caller storage. Configure
 * mode-specific Application return IDs and password acknowledgement, not
 * abstract OK/Cancel methods. Return the Window or NULL. No authentication
 * or system action occurs. Local capabilities, unsupported attributes,
 * string ownership and incomplete failure cleanup are described in the
 * class AutoDocs. The nominal method array is passed to Poplist after
 * construction and its stack address is not durable instance storage.
 */
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
    BPTR                    lock                = BNULL;
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
    catalog = OpenCatalog(NULL, SECURITYCATALOGNAME,
        OC_Version, SECURITYCATALOGVERSION, TAG_DONE);

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
        if ((lock = Lock(logoSpec + 2, ACCESS_READ)) != BNULL)
        {
            img_logo = ImageObject,
                    MUIA_Image_Spec, (IPTR)logoSpec,
                End;

            UnLock(lock);
        }
        else img_logo = HVSpace;
    }
    else img_logo = HVSpace;

    lock = BNULL;

    if ((lock = Lock(userSpec + 2, ACCESS_READ)) != BNULL)
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

/* Free the stored title, username, password and method-string fields when
 * non-NULL, delete the pool, close the catalog, then delegate Window
 * disposal. This does not explicitly free duplicated prompt/method-entry
 * strings; the method-list freeing loop is disabled. No password wiping is
 * performed here. Return superclass result.
 */
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

/* For UserName, replace only the stored copy unless its pointer equals the
 * supplied pointer; free before StrDup, allowing NULL or allocation failure
 * to clear it. Do not update username controls. Forward Cancel_Disabled to
 * the Cancel button when present. Delegate original tags to Window and
 * return its result. No other LoginWindow setter is implemented.
 */
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

/* Return borrowed live method/password String contents, username
 * Text/String contents according to mode or stored username in None mode,
 * and Cancel disabled state (TRUE when absent). Require writable
 * opg_Storage. Return TRUE for recognized attributes; delegate others to
 * Window. Returned strings may change with editing and must not be retained
 * past disposal.
 */
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

    /* Borrowed contents; editing can change or invalidate the returned data. */
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
