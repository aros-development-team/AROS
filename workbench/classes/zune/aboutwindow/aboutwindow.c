/*
    Copyright (C) 2003-2026, The AROS Development Team. All rights reserved.
    This file is part of the AboutWindow class, which is distributed under
    the terms of version 2.1 of the GNU Lesser General Public License.
    
*/

/****** AboutWindow.mcc/MUIC_AboutWindow ************
*
*   NAME
*       MUIC_AboutWindow
*
*   SUPERCLASS
*       Window.mui
*
*   DESCRIPTION
*       Build an information window containing a logo, version line, copyright,
*       description, and author/sponsor lists. All local attributes are
*       initialization-only; there are no local setters or getters. The source
*       configuration declares version 1.0.
*       The constructor creates a memory pool, opens AboutWindow.catalog when
*       available, copies supplied descriptive strings, and constructs the
*       Window content. Application defaults are resolved during Window_Setup,
*       after the object belongs to an Application. CloseRequest closes the
*       window through a notification; construction does not open it.
*       Recognized constructor tags are changed to TAG_IGNORE in the caller's
*       list and are not restored. Supply writable tag storage and do not expect
*       those tags to remain reusable unchanged. Repeated tags use their last
*       values; repeated descriptive strings can leak earlier copies.
*       Image becomes a child of the constructed hierarchy. Author/sponsor names
*       are copied into the private pool during construction. Descriptive
*       strings are duplicated with StrDup. The six protected object attributes
*       inside the header's disabled #if 0 block are not implemented or public
*       at this source stand.
*
*   NOTES
*       Omitting a string attribute permits its corresponding Application value.
*       Explicit NULL is converted to an internal omission marker; failed StrDup
*       allocation produces the same marker. No field-specific allocation error
*       is returned. Omission markers are cleared after successful Window_Setup,
*       so omission does not persist reliably across later setup cycles.
*       The generated version line combines bold title, version number,
*       parenthesized date, and bracketed extra text. Strings are displayed as
*       supplied, without date parsing or character-set conversion. If title is
*       absent but another version component exists, its buffer-size calculation
*       omits the terminator allowance and can truncate the final character.
*
*   BUGS
*       Disposal before successful setup can pass an internal omission marker to
*       FreeVec. Explicit NULL strings therefore do not have a safe arbitrary
*       construction/disposal lifecycle. Setup failure does not normalize those
*       markers.
*       Constructor failure after creating the pool does not comprehensively
*       free the pool, copied strings, or intermediate content. Setup removes
*       some absent/failed content from the group without disposing or
*       rebuilding it. Repeated setup can allocate another generated window
*       title without releasing the prior allocation. These paths are not a
*       robust reopen/recovery contract.
*       Both author and sponsor listviews are created even when only one or
*       neither tag list is supplied. Register titles are assembled separately;
*       a sponsors-only configuration does not establish a clean one-page
*       sponsor mapping. NamesToList failures are ignored, and its blank
*       section-separator insertion omits an insertion-position argument.
*       The default IconImage file path inherits IconImage's missing loaded-icon
*       release. A NULL Image requests fallback rather than suppressing the
*       image.
*
*/
/****** AboutWindow.mcc/MUIA_AboutWindow_Image ************
*
*   NAME
*       MUIA_AboutWindow_Image
*
*   FUNCTION
*       Supply the logo object to insert into the window.
*
*   INPUTS
*       Initialize only; Object *, default NULL. No local setter or getter.
*
*   NOTES
*       A non-NULL object is inserted into the image group and follows normal
*       child ownership; do not dispose it separately after successful
*       construction. NULL attempts an IconImage for PROGDIR: plus the current
*       program filename, then uses an empty spacer if that fails. It does not
*       mean no logo. The class performs no size or object-class validation.
*       Construction failure has incomplete intermediate-resource cleanup; do
*       not assume a transactional ownership handoff.
*
*/
/****** AboutWindow.mcc/MUIA_AboutWindow_Title ************
*
*   NAME
*       MUIA_AboutWindow_Title
*
*   FUNCTION
*       Supply the title text.
*
*   INPUTS
*       Initialize only; CONST_STRPTR to a readable NUL-terminated string, or
*       NULL. No local setter or getter.
*
*   NOTES
*       Omission uses MUIA_Application_Title during setup. Explicit non-NULL
*       input is copied during construction, so the caller string is not
*       retained. Explicit NULL requests omission through an internal marker;
*       allocation failure is indistinguishable. The supplied title appears in
*       the version line. Automatic Window_Title generation runs only when the
*       stored Title is NULL, so supplying Title does not generate an
*       About-prefixed window title. Set the inherited Window_Title separately
*       if needed.
*       Read the class lifecycle limitations before using explicit NULL: marker
*       handling is unsafe if disposed before successful setup and does not
*       preserve omission across setup cycles. Avoid duplicate tags, which can
*       leak previous copies.
*
*/
/****** AboutWindow.mcc/MUIA_AboutWindow_Version_Number ************
*
*   NAME
*       MUIA_AboutWindow_Version_Number
*
*   FUNCTION
*       Supply the version number text.
*
*   INPUTS
*       Initialize only; CONST_STRPTR to a readable NUL-terminated string, or
*       NULL. No local setter or getter.
*
*   NOTES
*       Omission uses MUIA_Application_Version_Number during setup. Explicit
*       non-NULL input is copied during construction, so the caller string is
*       not retained. Explicit NULL requests omission through an internal
*       marker; allocation failure is indistinguishable. Displayed after the
*       title. No numeric validation or version parsing occurs.
*       Read the class lifecycle limitations before using explicit NULL: marker
*       handling is unsafe if disposed before successful setup and does not
*       preserve omission across setup cycles. Avoid duplicate tags, which can
*       leak previous copies.
*
*/
/****** AboutWindow.mcc/MUIA_AboutWindow_Version_Date ************
*
*   NAME
*       MUIA_AboutWindow_Version_Date
*
*   FUNCTION
*       Supply the version date text.
*
*   INPUTS
*       Initialize only; CONST_STRPTR to a readable NUL-terminated string, or
*       NULL. No local setter or getter.
*
*   NOTES
*       Omission uses MUIA_Application_Version_Date during setup. Explicit
*       non-NULL input is copied during construction, so the caller string is
*       not retained. Explicit NULL requests omission through an internal
*       marker; allocation failure is indistinguishable. Displayed in
*       parentheses. The header suggests YYYY-MM-DD, but the implementation
*       neither validates nor reformats it.
*       Read the class lifecycle limitations before using explicit NULL: marker
*       handling is unsafe if disposed before successful setup and does not
*       preserve omission across setup cycles. Avoid duplicate tags, which can
*       leak previous copies.
*
*/
/****** AboutWindow.mcc/MUIA_AboutWindow_Version_Extra ************
*
*   NAME
*       MUIA_AboutWindow_Version_Extra
*
*   FUNCTION
*       Supply the version extra text.
*
*   INPUTS
*       Initialize only; CONST_STRPTR to a readable NUL-terminated string, or
*       NULL. No local setter or getter.
*
*   NOTES
*       Omission uses MUIA_Application_Version_Extra during setup. Explicit
*       non-NULL input is copied during construction, so the caller string is
*       not retained. Explicit NULL requests omission through an internal
*       marker; allocation failure is indistinguishable. Displayed in square
*       brackets. No interpretation of build or release labels occurs.
*       Read the class lifecycle limitations before using explicit NULL: marker
*       handling is unsafe if disposed before successful setup and does not
*       preserve omission across setup cycles. Avoid duplicate tags, which can
*       leak previous copies.
*
*/
/****** AboutWindow.mcc/MUIA_AboutWindow_Copyright ************
*
*   NAME
*       MUIA_AboutWindow_Copyright
*
*   FUNCTION
*       Supply the copyright text.
*
*   INPUTS
*       Initialize only; CONST_STRPTR to a readable NUL-terminated string, or
*       NULL. No local setter or getter.
*
*   NOTES
*       Omission uses MUIA_Application_Copyright during setup. Explicit non-NULL
*       input is copied during construction, so the caller string is not
*       retained. Explicit NULL requests omission through an internal marker;
*       allocation failure is indistinguishable. Displayed as its own centered
*       text. The Application fallback is duplicated on setup; no copyright
*       prefix is added here.
*       Read the class lifecycle limitations before using explicit NULL: marker
*       handling is unsafe if disposed before successful setup and does not
*       preserve omission across setup cycles. Avoid duplicate tags, which can
*       leak previous copies.
*
*/
/****** AboutWindow.mcc/MUIA_AboutWindow_Description ************
*
*   NAME
*       MUIA_AboutWindow_Description
*
*   FUNCTION
*       Supply the description text.
*
*   INPUTS
*       Initialize only; CONST_STRPTR to a readable NUL-terminated string, or
*       NULL. No local setter or getter.
*
*   NOTES
*       Omission uses MUIA_Application_Description during setup. Explicit
*       non-NULL input is copied during construction, so the caller string is
*       not retained. Explicit NULL requests omission through an internal
*       marker; allocation failure is indistinguishable. Displayed as centered
*       italic text. There is no enforced 40-character limit. The Application
*       fallback is duplicated on setup.
*       Read the class lifecycle limitations before using explicit NULL: marker
*       handling is unsafe if disposed before successful setup and does not
*       preserve omission across setup cycles. Avoid duplicate tags, which can
*       leak previous copies.
*
*/
/****** AboutWindow.mcc/MUIA_AboutWindow_Authors ************
*
*   NAME
*       MUIA_AboutWindow_Authors
*
*   FUNCTION
*       Supply the authors names and section headings.
*
*   INPUTS
*       Initialize only; struct TagItem * terminated with TAG_DONE. Default
*       NULL. No local setter or getter.
*
*   NOTES
*       Use SECTION_ID with SID_PROGRAMMING or SID_TRANSLATING for localized
*       bold headings; SID_NONE and unknown section IDs produce no heading.
*       NAME_STRING carries a non-NULL readable name string. Names under a
*       recognized heading gain four leading spaces. The SECTION and NAME macros
*       construct these tag pairs; terminate the complete list normally.
*       Tag data is consumed synchronously and names are copied into the private
*       pool. The list pointer is not retained. NULL supplies no names/title,
*       but both internal listviews are still constructed. List population
*       errors are ignored by the constructor; a valid window is not proof of
*       complete contents. See the class notes for register-page mapping and
*       separator limitations.
*
*/
/****** AboutWindow.mcc/MUIA_AboutWindow_Sponsors ************
*
*   NAME
*       MUIA_AboutWindow_Sponsors
*
*   FUNCTION
*       Supply the sponsors names and section headings.
*
*   INPUTS
*       Initialize only; struct TagItem * terminated with TAG_DONE. Default
*       NULL. No local setter or getter.
*
*   NOTES
*       Use SECTION_ID with SID_PROGRAMMING or SID_TRANSLATING for localized
*       bold headings; SID_NONE and unknown section IDs produce no heading.
*       NAME_STRING carries a non-NULL readable name string. Names under a
*       recognized heading gain four leading spaces. The SECTION and NAME macros
*       construct these tag pairs; terminate the complete list normally.
*       Tag data is consumed synchronously and names are copied into the private
*       pool. The list pointer is not retained. NULL supplies no names/title,
*       but both internal listviews are still constructed. List population
*       errors are ignored by the constructor; a valid window is not proof of
*       complete contents. See the class notes for register-page mapping and
*       separator limitations.
*
*/

#define MUIMASTER_YES_INLINE_STDARG

#include <exec/memory.h>
#include <utility/tagitem.h>
#include <dos/dos.h>
#include <libraries/mui.h>
#include <zune/iconimage.h>

#include <proto/exec.h>
#include <proto/muimaster.h>
#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/locale.h>
#include <proto/dos.h>
#include <clib/alib_protos.h>   /* StrDup() */

#include <string.h>

#include "aboutwindow.h"
#include "aboutwindow_private.h"
#include "catalogs/catalog_version.h"

#define CATCOMP_ARRAY
#include "strings.h"

/*** Locale Functions *******************************************************/
/* Return the catalog string for a valid CatCompArray index, using its
 * compiled default, or return that default directly when no catalog is
 * open. The result is borrowed; catalog-backed storage must not outlive the
 * catalog. No index validation or allocation occurs.
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

/*** Macros and Defines *****************************************************/
#define IGNORE ((APTR)(1UL))

/*** Utility ****************************************************************/
/* Map programming and translating section IDs to borrowed localized labels.
 * Return NULL for SID_NONE or any unrecognized value. The catalog may be
 * NULL for compiled defaults; no string ownership is transferred.
 */
CONST_STRPTR Section2Name(struct Catalog *catalog, ULONG section)
{
    switch (section)
    {
        case SID_PROGRAMMING:
            return _(MSG_SECTION_PROGRAMMING);
            
        case SID_TRANSLATING:
            return _(MSG_SECTION_TRANSLATING);
            
        default:
            return NULL;
    }
}

/* Consume name/section tags and allocate formatted strings in the instance
 * pool for list insertion. Return FALSE for NULL tags or allocation failure
 * and TRUE after processing; insertion results are ignored. NAME_STRING
 * must be non-NULL. The blank separator call omits its position argument.
 * Partial list content is retained on failure.
 */
BOOL NamesToList
(
    Object *list, struct TagItem *tags, struct AboutWindow_DATA *data
)
{
    struct TagItem *tag = NULL, *tstate       = tags;
    BOOL            success      = TRUE;
    IPTR            section      = SID_NONE;
    CONST_STRPTR    sectionName  = NULL;
    BOOL            sectionFirst = TRUE;
    STRPTR          name;
    STRPTR          buffer;
    ULONG           length       = 0;
    
    if (tags == NULL) return FALSE;
    
    while ((tag = NextTagItem(&tstate)) != NULL && success == TRUE)
    {
        switch (tag->ti_Tag)
        {
            case SECTION_ID:
                section     = tag->ti_Data;
                sectionName = Section2Name(data->awd_Catalog, section);
                
                if (sectionName != NULL)
                {
                    sectionFirst
                        ? sectionFirst = FALSE
                        : DoMethod(list, MUIM_List_InsertSingle, (IPTR) "");
                    
                    length = strlen(MUIX_B) + strlen(sectionName) + 1;
                    buffer = AllocPooled(data->awd_Pool, length);
                    if (buffer != NULL)
                    {
                        buffer[0] = '\0';
                        strcat(buffer, MUIX_B);
                        strcat(buffer, sectionName);
                        
                        DoMethod
                        (
                            list, MUIM_List_InsertSingle,
                            (IPTR) buffer, MUIV_List_Insert_Bottom
                        );
                    }
                    else
                    {
                        success = FALSE;
                        break;
                    }
                }
                
                break;
                
            case NAME_STRING:
                name   = (STRPTR) tag->ti_Data;
                
                length = strlen(name) + 1;
                if (sectionName != NULL) length += 4;
                
                buffer = AllocPooled(data->awd_Pool, length);
                if (buffer != NULL)
                {
                    buffer[0] = '\0';
                    if (sectionName != NULL) strcat(buffer, "    ");
                    strcat(buffer, name);
                    
                    DoMethod
                    (
                        list, MUIM_List_InsertSingle,
                        (IPTR) buffer, MUIV_List_Insert_Bottom
                    );
                }
                else
                {
                    success = FALSE;
                    break;
                }
                
                break;
        }
    }
    
    return success;
}

/*** Methods ****************************************************************/
/* Create a pool and optional catalog, consume and permanently ignore
 * recognized caller tags, duplicate strings or store omission markers, then
 * build the Window hierarchy. Populate both listviews and install close
 * notification. Return object or NULL; failure cleanup only closes the
 * catalog and is incomplete for other acquired resources.
 */
Object *AboutWindow__OM_NEW
(
    Class *CLASS, Object *self, struct opSet *message
)
{
    struct AboutWindow_DATA *data              = NULL;
    struct TagItem          *tag               = NULL,
                            *tstate            = message->ops_AttrList,
                            *authorsTags       = NULL,
                            *sponsorsTags      = NULL;
    struct Catalog          *catalog           = NULL;
    APTR                     pool;
    Object                  *rootGroup         = NULL,
                            *imageGroup        = NULL,
                            *imageObject       = NULL,
                            *versionObject     = NULL,
                            *copyrightObject   = NULL,
                            *descriptionGroup  = NULL,
                            *descriptionObject = NULL,
                            *authorsList       = NULL,
                            *sponsorsList      = NULL,
                            *registerGroup     = NULL;
    
    STRPTR                   title             = NULL,
                             versionNumber     = NULL,
                             versionDate       = NULL,
                             versionExtra      = NULL,
                             description       = NULL,
                             copyright         = NULL;
                             
    UBYTE                    nextPage          = 0;
    
    /* Allocate memory pool ------------------------------------------------*/
    pool = CreatePool(MEMF_ANY, 4096, 4096);
    if (pool == NULL) return NULL;
        
    /* Initialize locale ---------------------------------------------------*/
    catalog = OpenCatalog
    (
        NULL, "System/Classes/Zune/AboutWindow.catalog",
        OC_Version, CATALOG_VERSION,
        TAG_DONE
    );
        
    /* Parse initial attributes --------------------------------------------*/
    while ((tag = NextTagItem(&tstate)) != NULL)
    {
        switch (tag->ti_Tag)
        {
            case MUIA_AboutWindow_Image:
                imageObject = (Object *) tag->ti_Data;
                break;
            
            case MUIA_AboutWindow_Title:
                title = StrDup((STRPTR) tag->ti_Data);
                if (title == NULL) title = IGNORE;
                break;
                
            case MUIA_AboutWindow_Version_Number:
                versionNumber = StrDup((STRPTR) tag->ti_Data);
                if (versionNumber == NULL) versionNumber = IGNORE;
                break;
                
            case MUIA_AboutWindow_Version_Date:
                versionDate = StrDup((STRPTR) tag->ti_Data);
                if (versionDate == NULL) versionDate = IGNORE;
                break;
                
            case MUIA_AboutWindow_Version_Extra:
                versionExtra = StrDup((STRPTR) tag->ti_Data);
                if (versionExtra == NULL) versionExtra = IGNORE;
                break;
                
            case MUIA_AboutWindow_Copyright:
                copyright = StrDup((STRPTR) tag->ti_Data);
                if (copyright == NULL) copyright = IGNORE;
                break;
                
            case MUIA_AboutWindow_Description:
                description = StrDup((STRPTR) tag->ti_Data);
                if (description == NULL) description = IGNORE;
                break;
            
            case MUIA_AboutWindow_Authors:
                authorsTags = (struct TagItem *) tag->ti_Data;
                break;
                
            case MUIA_AboutWindow_Sponsors:
                sponsorsTags = (struct TagItem *) tag->ti_Data;
                break;
                            
            default:
                continue; /* Don't supress non-processed tags */
        }
        
        tag->ti_Tag = TAG_IGNORE;
    }
    
    /* Setup image ---------------------------------------------------------*/
    if (imageObject == NULL)
    {
        TEXT path[512], program[1024]; path[0] = '\0'; program[0] = '\0';
        
        if (GetProgramName(program, 1024))
        {
            strlcat(path, "PROGDIR:", 512);
            strlcat(path, FilePart(program), 512);
            imageObject = (Object *)IconImageObject,
                MUIA_IconImage_File, (IPTR) path,
            End;
        }
        
        if (imageObject == NULL)
        {
            imageObject = HVSpace;
        }
    }

    self = (Object *) DoSuperNewTags
    (
        CLASS, self, NULL,
        
        MUIA_Window_Activate, TRUE,
        MUIA_Window_NoMenus,  TRUE,
        MUIA_Window_Height,   MUIV_Window_Height_Visible(25),
        MUIA_Window_Width,    MUIV_Window_Width_Visible(25),
        
        WindowContents, (IPTR) (rootGroup = (Object *)VGroup,
            GroupSpacing(6),
            
            Child, (IPTR) (imageGroup = (Object *)HGroup,
                MUIA_Weight,  0,
                
                Child, (IPTR) HVSpace,
                Child, (IPTR) imageObject,
                Child, (IPTR) HVSpace,
            End),
            Child, (IPTR) (versionObject = (Object *)TextObject,
                MUIA_Text_PreParse, (IPTR) MUIX_C,
                MUIA_Text_Contents, (IPTR) NULL,
            End),
            Child, (IPTR) (copyrightObject = (Object *)TextObject,
                MUIA_Text_PreParse, (IPTR) MUIX_C,
                MUIA_Text_Contents, (IPTR) NULL,
            End),
            Child, (IPTR) (descriptionGroup = (Object *)VGroup,
                Child, (IPTR) VSpace(6),
                Child, (IPTR) (descriptionObject = (Object *)TextObject,
                    MUIA_Text_PreParse, (IPTR) MUIX_I MUIX_C,
                    MUIA_Text_Contents, (IPTR) NULL,
                End),
            End),
            Child, (IPTR) VSpace(6),
        End),
        
        TAG_MORE, (IPTR) message->ops_AttrList
    );
    
    if (self == NULL) goto error;
    
    data = INST_DATA(CLASS, self);
    data->awd_Catalog           = catalog;
    data->awd_Pool              = pool;
    data->awd_RootGroup         = rootGroup;
    data->awd_ImageGroup        = imageGroup;
    data->awd_ImageObject       = imageObject;
    data->awd_VersionObject     = versionObject;
    data->awd_CopyrightObject   = copyrightObject;
    data->awd_DescriptionGroup  = descriptionGroup;
    data->awd_DescriptionObject = descriptionObject;
    data->awd_Title             = title;
    data->awd_VersionNumber     = versionNumber;
    data->awd_VersionDate       = versionDate;
    data->awd_VersionExtra      = versionExtra;
    data->awd_Copyright         = copyright;
    data->awd_Description       = description;
    data->awd_PageTitles[0]     =
    data->awd_PageTitles[1]     =
    data->awd_PageTitles[2]     = NULL;

    /* Setup pages ---------------------------------------------------------*/
    if (authorsTags != NULL)
    {
        data->awd_PageTitles[nextPage] = _(MSG_AUTHORS);
        nextPage++;
    }
    
    if (sponsorsTags != NULL)
    {
        data->awd_PageTitles[nextPage] = _(MSG_SPONSORS);
        //nextPage++;
    }

    registerGroup = RegisterGroup(data->awd_PageTitles),
            Child, (IPTR) ListviewObject,
                MUIA_Listview_List, (IPTR) (authorsList = (Object *)ListObject,
                    ReadListFrame,
                End),
            End,
            Child, (IPTR) ListviewObject,
                MUIA_Listview_List, (IPTR) (sponsorsList = (Object *)ListObject,
                    ReadListFrame,
                End),
            End,
        End;

    DoMethod(data->awd_RootGroup, MUIM_Group_AddTail, registerGroup);

    if (authorsTags != NULL)  NamesToList(authorsList, authorsTags, data);
    if (sponsorsTags != NULL) NamesToList(sponsorsList, sponsorsTags, data);
    
    /* Setup notifications */
    DoMethod
    (
        self, MUIM_Notify, MUIA_Window_CloseRequest, TRUE,
        (IPTR) self, 3, MUIM_Set, MUIA_Window_Open, FALSE
    );
        
    return self;
    
error:
    if (catalog != NULL) CloseCatalog(catalog);
    
    return NULL;
}

/* Set up Window, resolve omitted fields from Application and populate text.
 * Generate Window_Title only on the stored-Title-NULL path. Remove
 * absent/failed text members without disposal, then convert omission
 * markers to NULL. Return FALSE only on superclass setup failure; other
 * failures can leave partial content. Repeated setup has no complete
 * rebuild/cleanup contract.
 */
IPTR AboutWindow__MUIM_Window_Setup
(
    Class *CLASS, Object *self, Msg message
)
{
    struct AboutWindow_DATA *data    = INST_DATA(CLASS, self);
    struct Catalog          *catalog = data->awd_Catalog;
    STRPTR                   string  = NULL;
    
    if (!DoSuperMethodA(CLASS, self, message)) return FALSE;

    /*= Setup window title =================================================*/
    {
        ULONG  length = 0;

        string = data->awd_Title;
        if (string == NULL)
        {
            GET(self, MUIA_AboutWindow_Title, &string);
            
            if (string != IGNORE)
            {
                if (string == NULL)
                {
                    GET(_app(self), MUIA_Application_Title, &string);
                }
                
                if (string != NULL)
                {
                    length = strlen(string) + strlen(_(MSG_ABOUT)) + 2; /* space + newline */
                    data->awd_WindowTitle = AllocVec(length, MEMF_ANY);
                    
                    if (data->awd_WindowTitle != NULL)
                    {
                        data->awd_WindowTitle[0] = '\0';
                        strlcat(data->awd_WindowTitle, _(MSG_ABOUT), length);
                        strlcat(data->awd_WindowTitle, " ", length);
                        strlcat(data->awd_WindowTitle, string, length);
                        
                        set(self, MUIA_Window_Title, data->awd_WindowTitle);
                    }
                }
            }
        }
    }
    
    /*= Setup image ========================================================*/
    if (data->awd_ImageObject == NULL)
    {
        DoMethod(data->awd_RootGroup, OM_REMMEMBER, (IPTR) data->awd_ImageGroup);
    }
    
    /*= Setup version ======================================================*/
    /*
        The version string will have the format:
        MUIX_B"<title>"MUIX_N" <version> (<date>) [<extra>]"
    */
    {
        STRPTR title         = data->awd_Title,
               versionNumber = data->awd_VersionNumber,
               versionDate   = data->awd_VersionDate,
               versionExtra  = data->awd_VersionExtra,
               buffer        = NULL;
        ULONG  length        = 0;
        
        /*- Setup default values -------------------------------------------*/
        if (title == NULL)
        {
            GET(_app(self), MUIA_Application_Title, &title);
        }
        
        if (versionNumber == NULL)
        {
            GET(_app(self), MUIA_Application_Version_Number, &versionNumber);
        }
        
        if (versionDate == NULL)
        {
            GET(_app(self), MUIA_Application_Version_Date, &versionDate);
        }
        
        if (versionExtra == NULL)
        {
            GET(_app(self), MUIA_Application_Version_Extra, &versionExtra);
        }
        
        /* Simplify later checks a little */
        if (title         == IGNORE) title         = NULL;
        if (versionNumber == IGNORE) versionNumber = NULL;
        if (versionDate   == IGNORE) versionDate   = NULL;
        if (versionExtra  == IGNORE) versionExtra  = NULL;
        
        /*- Calculate length -----------------------------------------------*/
        if (title != NULL)
        {
            length += strlen(MUIX_B) + strlen(title) + strlen(MUIX_N) + 1;
        }
        
        if (versionNumber != NULL)
        {
            length += 1 /* space */ + strlen(versionNumber);
        }
        
        if (versionDate != NULL)
        {
            length += 1 /* space */ + 1 /* ( */ + strlen(versionDate) + 1 /* ) */;
        }
        
        if (versionExtra != NULL)
        {
            length += 1 /* space */ + 1 /* [ */ + strlen(versionExtra) + 1 /* ] */;
        }
        
        /*- Setup version object -------------------------------------------*/
        if (length > 0)
        {
            /*- Allocate memory --------------------------------------------*/
            buffer = AllocVec(length, MEMF_ANY);
            
            if (buffer != NULL)
            {
                buffer[0] = '\0';
                
                /*- Generate text ------------------------------------------*/
                if (title != NULL)
                {
                    strlcat(buffer, MUIX_B, length);
                    strlcat(buffer, title, length);
                    strlcat(buffer, MUIX_N, length);
                }
                
                if (versionNumber != NULL)
                {
                    strlcat(buffer, " ", length);
                    strlcat(buffer, versionNumber, length);
                }
                
                if (versionDate != NULL)
                {
                    strlcat(buffer, " (", length);
                    strlcat(buffer, versionDate, length);
                    strlcat(buffer, ")", length);
                }
                
                if (versionExtra != NULL)
                {
                    strlcat(buffer, " [", length);
                    strlcat(buffer, versionExtra, length);
                    strlcat(buffer, "]", length);
                }
                
                set(data->awd_VersionObject, MUIA_Text_Contents, buffer);
                FreeVec(buffer); /* don't need it anymore */
            }
            else
            {
                DoMethod
                (
                    data->awd_RootGroup, OM_REMMEMBER, (IPTR) data->awd_VersionObject
                );
            }
        }
    }
        
    /*= Setup copyright ====================================================*/
    if (data->awd_Copyright == NULL)
    {
        STRPTR tmp = NULL;
        GET(_app(self), MUIA_Application_Copyright, &tmp);
        if (tmp != NULL) data->awd_Copyright = StrDup(tmp);
    }
    
    if (data->awd_Copyright != IGNORE && data->awd_Copyright != NULL)
    {
        SET(data->awd_CopyrightObject, MUIA_Text_Contents, data->awd_Copyright);
    }
    else
    {
        DoMethod(data->awd_RootGroup, OM_REMMEMBER, (IPTR) data->awd_CopyrightObject);
    }
    
    /*= Setup description ==================================================*/
    if (data->awd_Description == NULL)
    {
        STRPTR tmp = NULL;
        GET(_app(self), MUIA_Application_Description, &tmp);
        if (tmp != NULL) data->awd_Description = StrDup(tmp);
    }
    
    if (data->awd_Description != IGNORE && data->awd_Description != NULL)
    {
        SET
        (
            data->awd_DescriptionObject,
            MUIA_Text_Contents, data->awd_Description
        );
    }
    else
    {
        DoMethod
        (
            data->awd_RootGroup, OM_REMMEMBER, (IPTR) data->awd_DescriptionGroup
        );
    }
    
    /* We no longer need to know whether to IGNORE or not */
    if (data->awd_Title == IGNORE) data->awd_Title = NULL;
    if (data->awd_VersionNumber == IGNORE) data->awd_VersionNumber = NULL;
    if (data->awd_VersionDate == IGNORE) data->awd_VersionDate = NULL;
    if (data->awd_VersionExtra == IGNORE) data->awd_VersionExtra = NULL;
    if (data->awd_Copyright == IGNORE) data->awd_Copyright = NULL;
    if (data->awd_Description == IGNORE) data->awd_Description = NULL;
    
    return TRUE;
}

/* Free each non-NULL stored string pointer, the pool and catalog before
 * delegating disposal. This assumes setup already cleared omission markers;
 * it does not guard those sentinel values. Detached content is not
 * separately disposed and the routine does not repair constructor/setup
 * leaks.
 */
IPTR AboutWindow__OM_DISPOSE
(
    Class *CLASS, Object *self, Msg message
)
{
    struct AboutWindow_DATA *data   = INST_DATA(CLASS, self);
    UBYTE                    i;
    APTR                     ptrs[] =
    {
        data->awd_Title, data->awd_VersionNumber, data->awd_VersionDate,
        data->awd_VersionExtra, data->awd_Copyright, data->awd_Description,
        data->awd_WindowTitle
    };
    
    for (i = 0; i < (sizeof(ptrs) / sizeof(APTR)); i++)
    {
        if (ptrs[i] != NULL) FreeVec(ptrs[i]);
    }
    
    if (data->awd_Pool != NULL) DeletePool(data->awd_Pool);
    
    if (data->awd_Catalog != NULL) CloseCatalog(data->awd_Catalog);
    
    return DoSuperMethodA(CLASS, self, message);
}
