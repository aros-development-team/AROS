/*
    Copyright © 2003-2004, The AROS Development Team. All rights reserved.
    This file is part of the AboutWindow class, which is distributed under
    the terms of version 2.1 of the GNU Lesser General Public License.
    
    $Id$
*/

#define MUIMASTER_YES_INLINE_STDARG

#include <utility/tagitem.h>
#include <libraries/mui.h>
#include <dos/dos.h>

#include <proto/muimaster.h>
#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/locale.h>
#include <proto/dos.h>
#include <proto/icon.h>

#include <clib/alib_protos.h>   /* StrDup() */

#include <exec/memory.h>

#include <string.h>

#include <zune/iconimage.h>

#include "aboutwindow.h"
#include "aboutwindow_private.h"

#define CATCOMP_ARRAY
#include "strings.h"

#define DEBUG 1
#include <aros/debug.h>

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

#define IGNORE ((APTR)(1UL))

/*** Utility ****************************************************************/
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

BOOL NamesToList
(
    Object *list, struct TagItem *tags, struct AboutWindow_DATA *data
)
{
    struct TagItem *tstate       = tags,
                   *tag          = NULL;
    BOOL            success      = TRUE;
    IPTR            section      = SID_NONE;
    CONST_STRPTR    sectionName;
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

IPTR AboutWindow__OM_NEW
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
                            *sponsorsList      = NULL;
    
    STRPTR                   title             = NULL,
                             versionNumber     = NULL,
                             versionDate       = NULL,
                             versionExtra      = NULL,
                             description       = NULL,
                             copyright         = NULL;
                             
    CONST_STRPTR             pages[]           = { NULL, NULL, NULL }; 
    UBYTE                    nextPage          = 0;
    
    /* Allocate memory pool ------------------------------------------------*/
    pool = CreatePool(MEMF_ANY, 4096, 4096);
    if (pool == NULL) return NULL;
        
    /* Initialize locale ---------------------------------------------------*/
    catalog = OpenCatalogA
    (
        NULL, "System/Classes/Zune/AboutWindow.catalog", NULL
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
            imageObject = IconImageObject,
                MUIA_IconImage_File, (IPTR) path,
            End;
        }
        
        if (imageObject == NULL)
        {
            imageObject = HVSpace;
        }
    }

    /* Setup pages ---------------------------------------------------------*/
    if (authorsTags != NULL)
    {
        pages[nextPage] = _(MSG_AUTHORS);
        nextPage++;
    }
    
    if (sponsorsTags != NULL)
    {
        pages[nextPage] = _(MSG_SPONSORS);
        nextPage++;
    }
    
    self = (Object *) DoSuperNewTags
    (
        CLASS, self, NULL,
        
        MUIA_Window_Activate, TRUE,
        MUIA_Window_NoMenus,  TRUE,
        MUIA_Window_Height,   MUIV_Window_Height_Visible(25),
        MUIA_Window_Width,    MUIV_Window_Width_Visible(25),
        
        WindowContents, (IPTR) VGroup,
            GroupSpacing(6),
            
            Child, (IPTR) imageGroup = HGroup,
                MUIA_Weight,  0,
                
                Child, (IPTR) HVSpace,
                Child, (IPTR) imageObject,
                Child, (IPTR) HVSpace,
            End,
            Child, (IPTR) versionObject = TextObject,
                MUIA_Text_PreParse, (IPTR) MUIX_C,
                MUIA_Text_Contents, NULL,
            End,
            Child, (IPTR) copyrightObject = TextObject,
                MUIA_Text_PreParse, (IPTR) MUIX_C,
                MUIA_Text_Contents, NULL,
            End,
            Child, (IPTR) descriptionGroup = VGroup,
                Child, (IPTR) VSpace(6),
                Child, (IPTR) descriptionObject = TextObject,
                    MUIA_Text_PreParse, (IPTR) MUIX_I MUIX_C,
                    MUIA_Text_Contents, NULL,
                End,
            End,
            Child, (IPTR) VSpace(6),
            Child, (IPTR) RegisterGroup(pages),
                Child, (IPTR) ListviewObject,
                    MUIA_Listview_List, (IPTR) authorsList = ListObject,
                        ReadListFrame,
                    End,
                End,
                Child, (IPTR) ListviewObject,
                    MUIA_Listview_List, (IPTR) sponsorsList = ListObject,
                        ReadListFrame,
                    End,
                End,
            End, 
        End,
        
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
    
    if (authorsTags != NULL)  NamesToList(authorsList, authorsTags, data);
    if (sponsorsTags != NULL) NamesToList(sponsorsList, sponsorsTags, data);
    
    /* Setup notifications */
    DoMethod
    ( 
        self, MUIM_Notify, MUIA_Window_CloseRequest, TRUE, 
        (IPTR) self, 2, MUIA_Window_Open, FALSE
    );
        
    return (IPTR) self;
    
error:
    if (catalog != NULL) CloseCatalog(catalog);
    
    return NULL;
}

IPTR AboutWindow__MUIM_Window_Setup
(
    Class *CLASS, Object *self, Msg message
)
{
    struct AboutWindow_DATA *data    = INST_DATA(CLASS, self);
    struct Catalog          *catalog = data->awd_Catalog;
    STRPTR                   string  = NULL;
    IPTR                     rc      = NULL;
    
    rc = DoSuperMethodA(CLASS, self, message);
    if (rc == NULL) return rc;

    /*= Setup window title =================================================*/
    {
        STRPTR buffer = NULL;
        ULONG  length = 0;
        
        string = data->awd_Title;
        if (string == NULL)
        {
            GET(self, MUIA_AboutWindow_Title, (IPTR *) &string);
            
            if (string != IGNORE)
            {
                if (string == NULL)
                {
                    GET(_app(self), MUIA_Application_Title, (IPTR *) &string);
                }
                
                if (string != NULL)
                {
                    length = strlen(string) + strlen(_(MSG_ABOUT)) + 2; /* space + newline */
                    buffer = AllocVec(length, MEMF_ANY);
                    
                    if (buffer != NULL)
                    {
                        buffer[0] = '\0';
                        strlcat(buffer, _(MSG_ABOUT), length);
                        strlcat(buffer, " ", length);
                        strlcat(buffer, string, length);
                        
                        set(self, MUIA_Window_Title, buffer);
                        
                        FreeVec(buffer);
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
            GET(_app(self), MUIA_Application_Title, (IPTR *) &title);
        }
        
        if (versionNumber == NULL)
        {
            GET(_app(self), MUIA_Application_Version_Number, (IPTR *) &versionNumber);
        }
        
        if (versionDate == NULL)
        {
            GET(_app(self), MUIA_Application_Version_Date, (IPTR *) &versionDate);
        }
        
        if (versionExtra == NULL)
        {
            GET(_app(self), MUIA_Application_Version_Extra, (IPTR *) &versionExtra);
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
        GET(_app(self), MUIA_Application_Copyright, (IPTR *) &data->awd_Copyright);
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
        GET(_app(self), MUIA_Application_Description, (IPTR *) &data->awd_Description);
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
    
    return (IPTR) rc;
}

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
        data->awd_VersionExtra, data->awd_Copyright, data->awd_VersionExtra
    };
    
    for (i = 0; i < (sizeof(ptrs) / sizeof(APTR)); i++)
    {
        if (ptrs[i] != NULL) FreeVec(ptrs[i]);
    }
    
    if (data->awd_Pool != NULL) DeletePool(data->awd_Pool);
    
    if (data->awd_Catalog != NULL) CloseCatalog(data->awd_Catalog);
    
    return DoSuperMethodA(CLASS, self, message);
}
