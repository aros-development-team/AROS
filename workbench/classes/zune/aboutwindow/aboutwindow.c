/*
    Copyright © 2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#define MUIMASTER_YES_INLINE_STDARG

#include <utility/tagitem.h>
#include <libraries/mui.h>
#include <dos/dos.h>

#include <proto/intuition.h>
#include <proto/muimaster.h>
#include <proto/utility.h>
#include <proto/locale.h>
#include <proto/dos.h>
#include <proto/icon.h>

#include <string.h>

#include <zune/iconimage.h>

#include "aboutwindow.h"

#define CATCOMP_ARRAY
#include "strings.h"

#define DEBUG 1
#include <aros/debug.h>

/*** Locale functions *******************************************************/

STRPTR __MSG(struct Catalog *catalog, ULONG id)
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

#define MSG(id) __MSG(catalog,id)



/*** Instance data **********************************************************/

struct AboutWindow_DATA
{
    struct Catalog *awd_Catalog;
    Object         *awd_Root,
                   *awd_ImageGroup,
                   *awd_Image,
                   *awd_Version,
                   *awd_Copyright,
                   *awd_DescriptionGroup,
                   *awd_Description;
};

/*** Utility ****************************************************************/
STRPTR Section2Name(struct Catalog *catalog, ULONG section)
{
    switch (section)
    {
        case SID_PROGRAMMING:
            return MSG(MSG_SECTION_PROGRAMMING);
            
        case SID_TRANSLATING:
            return MSG(MSG_SECTION_TRANSLATING);
            
        default:
            return NULL;
    }
}

STRPTR Names2Text(struct Catalog *catalog, struct TagItem *tags)
{
    struct TagItem *tag          = NULL, 
                   *tstate       = tags;
    CONST_STRPTR    indent       = "    ";
    ULONG           indentLength = strlen(indent);
    STRPTR          result       = NULL;
    ULONG           length       = 0;
    BOOL            sectionFirst = TRUE;
    ULONG           section      = NULL;
    STRPTR          sectionName  = NULL;
    ULONG           sectionCount = 0;
    
    /* Check for sane input parameters -------------------------------------*/
    if (tags == NULL) return NULL;
    
    /* Calculate the length of text ----------------------------------------*/
    while ((tag = NextTagItem(&tstate)) != NULL)
    {
        switch (tag->ti_Tag)
        {
            case SECTION_ID:
                section     = (ULONG) tag->ti_Data;
                sectionName = Section2Name(catalog, section);
                
                if (sectionName != NULL)
                {
                    sectionCount++;
                
                    length += strlen(MUIX_B) + strlen(sectionName) 
                            + strlen(MUIX_N) + 1; /* newline */
                }
                break;
                
            case NAME:
                length += strlen((STRPTR) tag->ti_Data) + 1 /* newline */;
                
                if (sectionName != NULL)
                {
                    /*  Indent name unless in SID_NONE or unknown section */
                    length += indentLength;
                }
                
                break;
        }
    }
    if (sectionCount > 1)
    {
        length += sectionCount - 1; /* Newline between sections */
    }
    length += 1; /* Terminating '\0' */
    
    /* Allocate space for result -------------------------------------------*/
    result = AllocVec(length, MEMF_ANY);
    if (result == NULL) return NULL;
    result[0] = '\0';
    
    /* Generate text -------------------------------------------------------*/
    tstate = tags; 
    sectionFirst = TRUE;
    while ((tag = NextTagItem(&tstate)) != NULL)
    {
        switch (tag->ti_Tag)
        {
            case SECTION_ID:
                section     = (ULONG) tag->ti_Data;
                sectionName = Section2Name(catalog, section);
                
                if (sectionName != NULL)
                {
                    sectionFirst ? sectionFirst = FALSE 
                                 : strlcat(result, "\n", length);
                    
                    strlcat(result, MUIX_B, length);
                    strlcat(result, sectionName, length);
                    strlcat(result, MUIX_N, length);
                    strlcat(result, "\n", length);
                }
                break;
                
            case NAME:
                if (sectionName != NULL)
                {
                    /*  Indent name unless in SID_NONE or unknown section */
                    strlcat(result, indent, length);
                }
                strlcat(result, (STRPTR) tag->ti_Data, length);
                strlcat(result, "\n", length);
                break;
        }
    }
    
    /* Remove trailing newline */
    if (result[length - 2] == '\n') result[length - 2] = '\0';
    
    return result;
}


/*** Methods ****************************************************************/

IPTR AboutWindow$OM_NEW
(
    struct IClass *CLASS, Object *self, struct opSet *message 
)
{
    struct AboutWindow_DATA *data             = NULL; 
    struct TagItem          *tag              = NULL, 
                            *tstate           = message->ops_AttrList;    
    struct Catalog          *catalog          = NULL;
    Object                  *root             = NULL,
                            *imageGroup       = NULL,
                            *image            = NULL,
                            *version          = NULL,
                            *copyright        = NULL,
                            *descriptionGroup = NULL,
                            *description      = NULL;
    
    STRPTR                   authors          = NULL, 
                             sponsors         = NULL;
    
    STRPTR                   pages[]          = { NULL, NULL, NULL }; 
    UBYTE                    nextPage         = 0;
    
    struct TagItem *tags = TAGLIST
    (
        MUIA_Window_Activate,        TRUE,
        MUIA_Window_NoMenus,         TRUE,
        MUIA_Window_Height,          MUIV_Window_Height_Visible(25),
        MUIA_Window_Width,           MUIV_Window_Width_Visible(25),
        WindowContents,              NULL,
        TAG_MORE,             (IPTR) message->ops_AttrList
    );
    
    /* Initialize locale */
    catalog = OpenCatalogA
    (
        NULL, "System/Classes/Zune/AboutWindow.catalog", NULL
    );
    
    /* Parse initial attributes */
    while ((tag = NextTagItem(&tstate)) != NULL)
    {
        switch (tag->ti_Tag)
        {
            case MUIA_AboutWindow_Image:
                image = (Object *) tag->ti_Data;
                break;
            
            case MUIA_AboutWindow_Authors:
                authors = Names2Text(catalog, (struct TagItem *) tag->ti_Data);
                break; 
                
            case MUIA_AboutWindow_Sponsors:
                sponsors = Names2Text(catalog, (struct TagItem *) tag->ti_Data);
                break;
                            
            default:
                continue; /* Don't supress non-processed tags */
        }
        
        tag->ti_Tag = TAG_IGNORE;
    }
    
    /* Setup image */
    if (image == NULL)
    {
        TEXT path[1024], program[1024]; path[0] = '\0'; program[0] = '\0';
        
        strlcat(path, "PROGDIR:", 1024);
        if (GetProgramName(program, 1024))
        {
            strlcat(path, program, 1024);
            image = IconImageObject,
                MUIA_IconImage_File, path,
            End;
        }
    }

    /* Setup pages */
    if (authors != NULL)
    {
        pages[nextPage] = MSG(MSG_AUTHORS);
        nextPage++;
    }
    
    if (sponsors != NULL)
    {
        pages[nextPage] = MSG(MSG_SPONSORS);
        nextPage++;
    }
    
    
    tags[4].ti_Data = (IPTR) root = VGroup,
        GroupSpacing(6),
        
        Child, imageGroup = HGroup,
            Child, HVSpace,
            Child, image,
            Child, HVSpace,
        End,
        Child, version = TextObject,
            MUIA_Text_PreParse, MUIX_C,
            MUIA_Text_Contents, NULL,
        End,
        Child, copyright = TextObject,
            MUIA_Text_PreParse, MUIX_C,
            MUIA_Text_Contents, NULL,
        End,
        Child, descriptionGroup = VGroup,
            Child, VSpace(6),
            Child,  description = TextObject,
                MUIA_Text_PreParse, MUIX_I MUIX_C,
                MUIA_Text_Contents, NULL,
            End,
        End,
        Child, VSpace(6),
        Child, RegisterGroup(pages),
            Child, VGroup,
                Child, ScrollgroupObject,
                    MUIA_Scrollgroup_Contents, VGroupV,
                        TextFrame,
                        
                        Child, TextObject,
                            MUIA_Text_PreParse, MUIX_L,
                            MUIA_Text_Contents, authors,
                        End,
                        Child, HVSpace,
                    End,
                End,
            End,
            Child, VGroup,
                Child, ScrollgroupObject,
                    MUIA_Scrollgroup_Contents, VGroupV,
                        TextFrame,
                        
                        Child, TextObject,
                            MUIA_Text_PreParse, MUIX_L,
                            MUIA_Text_Contents, sponsors,
                        End,
                        Child, HVSpace,
                    End,
                End,
            End,
        End, 
    End;
    
    if (tags[3].ti_Data == NULL) goto error;
    message->ops_AttrList = tags;
          
    self = (Object *) DoSuperMethodA(CLASS, self, (Msg) message);
    if (self == NULL) goto error;
    
    data = INST_DATA(CLASS, self);
    data->awd_Catalog          = catalog;
    data->awd_Root             = root;
    data->awd_ImageGroup       = imageGroup;
    data->awd_Image            = image;
    data->awd_Version          = version;
    data->awd_Copyright        = copyright;
    data->awd_DescriptionGroup = descriptionGroup;
    data->awd_Description      = description;
    
    if (authors != NULL) FreeVec(authors);
    if (sponsors != NULL) FreeVec(sponsors);
    
    /* Setup notifications */
    DoMethod
    ( 
        self, MUIM_Notify, MUIA_Window_CloseRequest, TRUE, 
        (IPTR) self, 2, MUIA_Window_Open, FALSE
    );
        
    return self;
    
error:
    if (catalog != NULL) CloseCatalog(catalog);
    
    if (authors != NULL) FreeVec(authors);
    if (sponsors != NULL) FreeVec(sponsors);
    
    return NULL;
}

IPTR AboutWindow$MUIM_Window_Setup
(
    struct IClass *CLASS, Object *self, Msg message
)
{
    struct AboutWindow_DATA *data    = INST_DATA(CLASS, self);
    struct Catalog          *catalog = data->awd_Catalog;
    STRPTR                   string  = NULL;
    IPTR                     rc      = NULL;
    ULONG                    length;
    STRPTR                   buffer  = NULL;
    
    rc = DoSuperMethodA(CLASS, self, message);
    if (rc == NULL) return rc;

    /* Setup window title */
    get(self, MUIA_Window_Title, &string);
    if (string == NULL)
    {
        get(_app(self), MUIA_Application_Title, &string);
        length = strlen(string) + strlen(MSG(MSG_ABOUT)) + 2; /* space + newline */
        buffer = AllocVec(length, MEMF_ANY);
        
        if (buffer != NULL)
        {
            buffer[0] = '\0';
            strlcat(buffer, MSG(MSG_ABOUT), length);
            strlcat(buffer, " ", length);
            strlcat(buffer, string, length);
            
            set(self, MUIA_Window_Title, buffer);
            
            FreeVec(buffer);
        }
        
    }
    
    /* Setup image */
    if (data->awd_Image == NULL)
    {
        DoMethod(data->awd_Root, OM_REMMEMBER, data->awd_ImageGroup, TAG_DONE);
    }
    
    /* Setup version */
    get(_app(self), MUIA_Application_Version, &string);
    if (string != NULL)
    {
        set(data->awd_Version, MUIA_Text_Contents, string);
    }
    else
    {
        DoMethod(data->awd_Root, OM_REMMEMBER, data->awd_Version, TAG_DONE);
    }
    
    /* Setup copyright */
    get(_app(self), MUIA_Application_Copyright, &string);
    if (string != NULL)
    {
        set(data->awd_Copyright, MUIA_Text_Contents, string);
    }
    else
    {
        DoMethod(data->awd_Root, OM_REMMEMBER, data->awd_Copyright, TAG_DONE);
    }
    
    /* Setup description */
    get(_app(self), MUIA_Application_Description, &string);
    if (string != NULL)
    {
        set(data->awd_Description, MUIA_Text_Contents, string);
    }
    else
    {
        DoMethod
        (
            data->awd_Root, OM_REMMEMBER, data->awd_DescriptionGroup, TAG_DONE
        );
    }
    
    return rc;
}

IPTR AboutWindow$OM_DISPOSE
(
    struct IClass *CLASS, Object *self, Msg message 
)
{
    struct AboutWindow_DATA *data = INST_DATA(CLASS, self);

    if (data->awd_Catalog != NULL) CloseCatalog(data->awd_Catalog);
    
    return DoSuperMethodA(CLASS, self, message);
}
