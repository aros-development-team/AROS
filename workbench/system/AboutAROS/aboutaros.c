/*
    Copyright (C) 2003-2026, The AROS Development Team. All rights reserved.
    This file is part of the About program, which is distributed under
    the terms of version 2 of the GNU General Public License.
    
*/

#define MUIMASTER_YES_INLINE_STDARG

#include <aros/isoascii.h>
#include <aros/debug.h>
#include <aros/build.h>
#include <aros/inquire.h>
#include <exec/types.h>
#include <utility/tagitem.h>
#include <libraries/mui.h>

#include <proto/alib.h>
#include <proto/aros.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/muimaster.h>
#include <proto/workbench.h>

#include <zune/aboutwindow.h>
#include <zune/qrcode.h>
#include <zune/customclasses.h>

#include <stdio.h>
#include <string.h>

#include "aboutaros.h"
#include "locale.h"
#include "logotype.h"

#include "authors.h"
#include "sponsors.h"
#include "acknowledgements.h"

#if defined(BUILDCPU)
CONST_STRPTR str_aroscpu = "" BUILDCPU "";
#endif

#define VERSION "$VER: AboutAROS 0.3 ("ADATE") " ISOASCII_COPYRIGHT "AROS Dev Team"

#define WINDOW_BG   ((IPTR) "2:00000000,00000000,00000000")
#define REGISTER_BG ((IPTR) "7:V,00000000,92000000,91000000-00000000,82000000,81000000")
#define LIST_BG     ((IPTR) "2:00000000,82000000,81000000")

#define SETUP_INST_DATA struct AboutAROS_DATA *data = INST_DATA(CLASS, self)

/*** Private methods ********************************************************/
#define MUIM_AboutAROS_ShowLicense (TAG_USER | 0x20000000)

/*** Instance Data **********************************************************/
struct AboutAROS_DATA
{
    Object *aad_Window;
    APTR    aad_Pool;
};

static STRPTR pages[4] = { NULL };

static const char *tpp_Plain = "\0333";
static const char *tpp_Bold = "\0333\033b";

/*** Utility Functions ******************************************************/
/* Return the borrowed localized label for a recognized SID_* value.
 * Return NULL for an unrecognized section; callers must not free the label.
 */
STRPTR Section2Name(ULONG section)
{
    switch (section)
    {
        case SID_COORDINATION:   return _(MSG_SECTION_COORDINATION);
        case SID_EVANGELISM:     return _(MSG_SECTION_EVANGELISM);
        case SID_HIDD:           return _(MSG_SECTION_HIDD);
        case SID_INTUITION:      return _(MSG_SECTION_INTUITION);
        case SID_GRAPHICS:       return _(MSG_SECTION_GRAPHICS);
        case SID_SHELL_COMMANDS: return _(MSG_SECTION_SHELL_COMMANDS);
        case SID_WORKBENCH:      return _(MSG_SECTION_WORKBENCH);
        case SID_TOOLS:          return _(MSG_SECTION_TOOLS);
        case SID_PREFERENCES:    return _(MSG_SECTION_PREFERENCES);
        case SID_BGUI:           return _(MSG_SECTION_BGUI);
        case SID_ZUNE:           return _(MSG_SECTION_ZUNE);
        case SID_KERNEL:         return _(MSG_SECTION_KERNEL);
        case SID_DOS:            return _(MSG_SECTION_DOS);
        case SID_NETWORKING:     return _(MSG_SECTION_NETWORK);
        case SID_LIBC_POSIX:     return _(MSG_SECTION_LIBC_POSIX);
        case SID_DOCUMENTATION:  return _(MSG_SECTION_DOCUMENTATION);
        case SID_TRANSLATION:    return _(MSG_SECTION_TRANSLATION);
        case SID_ARTISTRY:       return _(MSG_SECTION_ARTISTRY);
        case SID_WEBSITE:        return _(MSG_SECTION_WEBSITE);
        case SID_DATATYPES:      return _(MSG_SECTION_DATATYPES);
        case SID_TESTING:        return _(MSG_SECTION_TESTING);
        default:                 return NULL;
    }
}

/* Append section headings and names from a TagItem sequence to a MUI list.
 * The list and data->aad_Pool must be valid; NAME_STRING entries must
 * contain valid NUL-terminated strings. Inserted strings are pool-owned.
 * Return FALSE for NULL tags or a failed pool allocation, TRUE otherwise.
 * List insertion results are not checked; partial insertion is not undone.
 */
BOOL NamesToList
(
    Object *list, struct TagItem *tags, struct AboutAROS_DATA *data
)
{
    struct TagItem       *tstate       = tags;
    struct TagItem       *tag          = NULL;
    BOOL                  success      = TRUE;
    IPTR                  section      = SID_NONE;
    STRPTR                sectionName  = NULL;
    BOOL                  sectionFirst = TRUE;
    STRPTR                name;
    STRPTR                buffer;
    ULONG                 length       = 0;
    
    if (tags == NULL) return FALSE;

    while ((tag = NextTagItem(&tstate)) != NULL && success == TRUE)
    {
        switch (tag->ti_Tag)
        {
            case SECTION_ID:
                section     = tag->ti_Data;
                sectionName = Section2Name(section);
                
                if (sectionName != NULL)
                {
                    sectionFirst
                        ? sectionFirst = FALSE
                        : DoMethod
                          (
                              list, MUIM_List_InsertSingle, (IPTR) "",
                              MUIV_List_Insert_Bottom
                          );

                    length = strlen(MUIX_B) + strlen(sectionName) + 1;
                    buffer = AllocPooled(data->aad_Pool, length);
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
                
                buffer = AllocPooled(data->aad_Pool, length);
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

/* Return the unique bounded AROS website URL already present in the
 * localized About text. Accept only the project host with an optional
 * slash-separated ASCII path; the caller keeps the displayed text unchanged.
 * Invalid, ambiguous and oversized targets simply suppress the optional QR.
 */
#define ABOUTAROS_CATALOG_TEXT_LIMIT 4096UL
static BOOL AboutAROSWebsiteURL(CONST_STRPTR text, char *dst, ULONG capacity,
                                ULONG *urlLength)
{
    static const struct {
        CONST_STRPTR name;
        ULONG length;
    } hosts[] = {
        { "http://www.aros.org", sizeof("http://www.aros.org") - 1 },
        { "https://www.aros.org", sizeof("https://www.aros.org") - 1 },
        { "http://aros.org", sizeof("http://aros.org") - 1 },
        { "https://aros.org", sizeof("https://aros.org") - 1 }
    };
    const char *p;
    ULONG textLength = 0;
    ULONG matches = 0;
    ULONG acceptedLength = 0;

    if (text == NULL || dst == NULL || urlLength == NULL || capacity == 0)
        return FALSE;
    dst[0] = '\0';
    *urlLength = 0;
    while (textLength < ABOUTAROS_CATALOG_TEXT_LIMIT &&
           text[textLength] != '\0')
        textLength++;
    if (textLength == ABOUTAROS_CATALOG_TEXT_LIMIT)
        return FALSE;
    for (p = text; *p != '\0'; p++)
    {
        ULONG i;

        for (i = 0; i < sizeof(hosts) / sizeof(hosts[0]); i++)
        {
            const char *end;
            ULONG hostlen = hosts[i].length;
            ULONG length;

            if (strncmp(p, hosts[i].name, hostlen) != 0)
                continue;
            /* Do not treat a larger host as an AROS hostname. */
            end = p + hostlen;
            if (*end == '/')
            {
                do {
                    end++;
                } while ((*end >= 'a' && *end <= 'z') ||
                         (*end >= 'A' && *end <= 'Z') ||
                         (*end >= '0' && *end <= '9') ||
                         *end == '/' || *end == '-' || *end == '_');
            }
            if (*end != '\0' && *end != ' ' && *end != '\t' &&
                *end != '\r' && *end != '\n' &&
                !(*end == '.' && (end[1] == '\0' || end[1] == ' ')) &&
                !(*end == ',' && (end[1] == '\0' || end[1] == ' ')))
                continue;
            length = (ULONG)(end - p);
            if (length == 0 || length >= capacity || length > 62)
                return FALSE;
            matches++;
            if (matches > 1)
                return FALSE;
            {
                ULONG j;
                for (j = 0; j < length; j++)
                    dst[j] = p[j];
            }
            dst[length] = '\0';
            acceptedLength = length;
            /* Skip the accepted URL to avoid matching inside its path. */
            p = end - 1;
            break;
        }
    }
    if (matches == 1)
    {
        *urlLength = acceptedLength;
        return TRUE;
    }
    return FALSE;
}

/* Localized Text.mui strings: wrap at spaces before drawing.  Unlike
 * Floattext.mui, Text.mui is not a List and cannot create scrollbars.
 * Only 4095 bytes of catalog text are accepted, and no multibyte character
 * is split because only ASCII spaces are changed to newline separators.
 */
static STRPTR AboutAROSWrappedText(APTR pool, ULONG id)
{
    CONST_STRPTR text = _(id);
    ULONG length = 0, i, column = 0;
    ULONG lastSpace = ABOUTAROS_CATALOG_TEXT_LIMIT;
    STRPTR result;

    if (pool == NULL || text == NULL)
        return NULL;
    while (length < ABOUTAROS_CATALOG_TEXT_LIMIT && text[length] != '\0')
        length++;
    if (length == ABOUTAROS_CATALOG_TEXT_LIMIT)
        return NULL;
    result = AllocPooled(pool, length + 4);
    if (result == NULL)
        return NULL;
    result[0] = '\033';
    result[1] = '3';
    for (i = 0; i <= length; i++)
        result[i + 2] = text[i];
    if (id == MSG_LICENSE_1)
    {
        result[length + 2] = ' ';
        result[length + 3] = '\0';
    }
    for (i = 0; i < length; i++)
    {
        if (result[i + 2] == '\n')
        {
            column = 0;
            lastSpace = ABOUTAROS_CATALOG_TEXT_LIMIT;
            continue;
        }
        if (result[i + 2] == ' ')
            lastSpace = i;
        column++;
        if (column >= 72 && lastSpace != ABOUTAROS_CATALOG_TEXT_LIMIT)
        {
            result[lastSpace + 2] = '\n';
            column = i - lastSpace;
            lastSpace = ABOUTAROS_CATALOG_TEXT_LIMIT;
        }
    }
    return result;
}

/* Keep the licence link inline only for short localized sentences.
 * Longer translations retain the original multi-line layout. */
static BOOL AboutAROSLicenseInline(void)
{
    CONST_STRPTR parts[3] = { _(MSG_LICENSE_1), _(MSG_LICENSE_2), _(MSG_LICENSE_3) };
    ULONG i, length = 0;
    for (i = 0; i < 3; i++)
    {
        CONST_STRPTR p = parts[i];
        if (p == NULL) return FALSE;
        while (*p != '\0' && length <= 64)
        {
            length++;
            p++;
        }
        if (*p != '\0') return FALSE;
    }
    return length <= 64;
}

/*** Methods ****************************************************************/
/* Construct the AboutAROS application, window, lists and optional QR view.
 * Forward construction to the MUI superclass. A ready QR object is
 * inserted only when its external class is available; successful group
 * insertion transfers the QR object's lifetime to the group.
 * Return the application object on success or NULL on constructor
 * failure. The instance owns its allocation pool after successful
 * construction. The pre-existing superclass-failure path does not
 * release a pool allocated before DoSuperNewTags.
 */
Object *AboutAROS__OM_NEW(Class *CLASS, Object *self, struct opSet *message)
{
    Object                *window,
                          *licenseButton,
                          *authorsList,
                          *sponsorsList,
                          *acknowledgementsList;

    STRPTR                 str_builddate  = NULL;
    STRPTR                 str_variant    = NULL;
    STRPTR                 str_arosarch   = NULL;
    STRPTR                 str_buildtype;
#if defined(BUILDCPU)
    STRPTR                 str_buildcpu;
#endif
    IPTR                   abiversion;
    STRPTR                 str_abi;
    BOOL                   showLogotype;
    BPTR                   lock;
    APTR                   pool;
    Object                *qrGroup = NULL;
    STRPTR                 copyrightText, licenseFirst, licenseLast, websiteText;
    BOOL                   licenseInline;

    /* Allocate memory pool ------------------------------------------------*/
    pool = CreatePool(MEMF_ANY, 4096, 4096);
    if (pool == NULL) return NULL;
    copyrightText = AboutAROSWrappedText(pool, MSG_COPYRIGHT);
    licenseFirst = AboutAROSWrappedText(pool, MSG_LICENSE_1);
    licenseLast = AboutAROSWrappedText(pool, MSG_LICENSE_3);
    websiteText = AboutAROSWrappedText(pool, MSG_MORE_INFORMATION);
    if (!copyrightText || !licenseFirst || !licenseLast || !websiteText)
    {
        DeletePool(pool);
        return NULL;
    }
    licenseInline = AboutAROSLicenseInline();

    /* Check if the logotype is available ----------------------------------*/
    if ((lock = Lock(LOGOTYPE_IMAGE, ACCESS_READ)) != BNULL)
    {
        showLogotype = TRUE;
        UnLock(lock);
    }
    else
    {
        showLogotype = FALSE;
    }
   
    /* Retrieve the build date and the variant name ------------------------*/
    ArosInquire(AI_ArosBuildDate, (IPTR) &str_builddate,
                AI_ArosVariant, (IPTR) &str_variant,
                AI_ArosArchitecture, (IPTR) &str_arosarch,
                AI_ArosABIMajor, (IPTR) &abiversion,
                TAG_DONE);

    str_buildtype = AllocPooled(pool, strlen(_(MSG_BUILD_TYPE)) + strlen(str_arosarch) + 1);
    sprintf(str_buildtype, _(MSG_BUILD_TYPE), str_arosarch);
    if (abiversion != -1)
    {
        str_abi = AllocPooled(pool, 7 + 1);
        sprintf(str_abi, "ABI_V%d", (int)abiversion);
    }
    else
        str_abi = "ABI_V1";

#if defined(BUILDCPU)
    str_buildcpu = AllocPooled(pool, strlen(_(MSG_BUILD_CPU)) + strlen(str_aroscpu) + 1);
    sprintf(str_buildcpu, _(MSG_BUILD_CPU), str_aroscpu);
#endif

    /* Initialize page labels ----------------------------------------------*/
    pages[0] = _(MSG_PAGE_AUTHORS);
    pages[1] = _(MSG_PAGE_SPONSORS);
    pages[2] = _(MSG_PAGE_ACKNOWLEDGEMENTS);
    
    /* Create application and window objects -------------------------------*/
    self = (Object *) DoSuperNewTags
    (
        CLASS, self, NULL,

        MUIA_Application_Title, __(MSG_TITLE),
        MUIA_Application_Version, (IPTR)VERSION,
        MUIA_Application_Copyright, (IPTR)ISOASCII_COPYRIGHT " 2010-2026, The AROS Development Team",
        MUIA_Application_Description, __(MSG_TITLE),
        MUIA_Application_Base, (IPTR) "ABOUTAROS",
        SubWindow, (IPTR) (window = (Object *)WindowObject,
            MUIA_Window_Title,    __(MSG_TITLE),
            MUIA_Window_Width,    MUIV_Window_Width_MinMax(0),
            MUIA_Window_NoMenus,  TRUE,
            MUIA_Window_Activate, TRUE,
            MUIA_Window_ID, MAKE_ID('A','B','W','N'),
            WindowContents, (IPTR) VGroup,
                InnerSpacing(0, 0),
                GroupSpacing(2),
                MUIA_Background, WINDOW_BG,

                Child, (IPTR) VSpace(8),
                Child, showLogotype
                    ? (IPTR) ImageObject,
                          MUIA_Image_Spec, (IPTR)    "3:"LOGOTYPE_IMAGE,
                      End
                    : (IPTR) TextObject,
                          MUIA_Font,                 MUIV_Font_Fixed,
                          MUIA_Text_PreParse, (IPTR) "\0333\033b\033c",
                          MUIA_Text_Contents, (IPTR) LOGOTYPE_ASCII,
                          MUIA_Weight,               0,
                      End
                    ,
                Child, (IPTR) VSpace(8),
                Child, (IPTR) HGroup,
                    InnerSpacing(0,0),

                    GroupSpacing(6),
                    Child, (IPTR) HVSpace,
                    Child, (IPTR) TextObject,
                        MUIA_Font,                 MUIV_Font_Big,
                        MUIA_Text_PreParse, (IPTR) tpp_Bold,
                        MUIA_Text_Contents, (IPTR) str_buildtype,
                        MUIA_Weight,               0,
                    End,

                    Child, (IPTR) TextObject,
                        MUIA_Font,                 MUIV_Font_Big,
                        MUIA_Text_PreParse, (IPTR) tpp_Bold,
                        MUIA_Text_Contents, (IPTR) str_abi,
                        MUIA_Weight,               0,
                    End,

                    Child, (IPTR) TextObject,
                        MUIA_Font,                 MUIV_Font_Big,
                        MUIA_Text_PreParse, (IPTR) tpp_Bold,
                        MUIA_Text_Contents, (IPTR) str_variant,
                        MUIA_Weight,               0,
                    End,
                    Child, (IPTR) TextObject,
                        MUIA_Font,                 MUIV_Font_Big,
                        MUIA_Text_PreParse, (IPTR) tpp_Bold,
                        MUIA_Text_Contents, (IPTR) str_builddate,
                        MUIA_Weight,               0,
                    End,
                    Child, (IPTR) HVSpace,
                End,
#ifdef REPOTYPE
                Child, (IPTR) HGroup,
                    InnerSpacing(0,0),
                    GroupSpacing(0),

                    Child, (IPTR) TextObject,
                        MUIA_Text_PreParse, (IPTR) tpp_Bold,
                        MUIA_Text_Contents, (IPTR) "" REPOTYPE " ID: " REPOREVISION,
                        MUIA_Weight,               0,
                    End,
#ifdef REPOID
                    Child, (IPTR) TextObject,
                        MUIA_Text_PreParse, (IPTR) tpp_Bold,
                        MUIA_Text_Contents, (IPTR) " (" REPOID ")",
                        MUIA_Weight,               0,
                    End,
#endif
                End,
#endif
                Child, (IPTR) VSpace(4),
                /* Center the information within the original logo footprint. */
                Child, (IPTR) HGroup,
                    InnerSpacing(0,0),
                    GroupSpacing(0),
                    Child, (IPTR) HSpace(0),
                    Child, (IPTR) (qrGroup = (Object *)HGroup,
                        MUIA_HorizWeight, 100,
                        MUIA_FixWidth, 600,
                        MUIA_InnerLeft, 20,
                        MUIA_InnerRight, 12,
                        MUIA_InnerTop, 0,
                        MUIA_InnerBottom, 0,
                        GroupSpacing(16),
                        Child, (IPTR) VGroup,
                            InnerSpacing(0,0),
                            GroupSpacing(2),
                            /* Native Text.mui: no list widgets or scrollbars. */
                            Child, (IPTR) TextObject,
                                NoFrame,
                                MUIA_Background, WINDOW_BG,
                                MUIA_VertWeight, 0,
                                MUIA_Text_Contents, (IPTR)copyrightText,
                            End,
                            /* Short licence sentences use one line; the link remains clickable. */
                            Child, (IPTR) HGroup,
                                MUIA_Group_Horiz, licenseInline,
                                InnerSpacing(0,0),
                                GroupSpacing(0),
                                Child, (IPTR) TextObject,
                                    NoFrame,
                                    MUIA_Background, WINDOW_BG,
                                    MUIA_HorizWeight, 0,
                                    MUIA_Text_Contents, (IPTR)licenseFirst,
                                End,
                                Child, (IPTR) (licenseButton = (Object *)TextObject,
                                    MUIA_InputMode, MUIV_InputMode_RelVerify,
                                    MUIA_Text_PreParse, (IPTR) "\0333\033u",
                                    MUIA_Text_Contents, __(MSG_LICENSE_2),
                                    MUIA_Weight, 0,
                                End),
                                Child, (IPTR) TextObject,
                                    NoFrame,
                                    MUIA_Background, WINDOW_BG,
                                    MUIA_HorizWeight, 0,
                                    MUIA_Text_Contents, (IPTR)licenseLast,
                                End,
                                /* Optical alignment within the existing text column.
                                 * Zero width in the multi-line locale fallback. */
                                Child, (IPTR) HSpace(licenseInline ? 42 : 0),
                            End,
                            Child, (IPTR) VSpace(8),
                            Child, (IPTR) TextObject,
                                NoFrame,
                                MUIA_Background, WINDOW_BG,
                                MUIA_VertWeight, 0,
                                MUIA_Text_Contents, (IPTR)websiteText,
                            End,
#if defined(BUILDCPU)
                            Child, (IPTR) TextObject,
                                MUIA_Text_PreParse, (IPTR) "\0333\033c",
                                MUIA_Text_Contents, (IPTR) str_buildcpu,
                                MUIA_Weight, 0,
                            End,
                            Child, (IPTR) RectangleObject,
                                MUIA_FixHeightTxt, (IPTR) " ",
                                MUIA_Weight, 0,
                            End,
#endif
                        End,
                    End),
                    Child, (IPTR) HSpace(0),
                End,
                Child, (IPTR) VGroup,
                    InnerSpacing(4,4),

                    Child, (IPTR) RegisterGroup(pages),
                        MUIA_Background, REGISTER_BG,

                        Child, (IPTR) ListviewObject,
                            MUIA_Listview_List, (IPTR) (authorsList = (Object *)ListObject,
                                TextFrame,
                                MUIA_Background, LIST_BG,
                            End),
                        End,
                        Child, (IPTR) ListviewObject,
                            MUIA_Listview_List, (IPTR) (sponsorsList = (Object *)ListObject,
                                TextFrame,
                                MUIA_Background, LIST_BG,
                            End),
                        End,
                        Child, (IPTR) ListviewObject,
                            MUIA_Listview_List, (IPTR) (acknowledgementsList = (Object *)ListObject,
                                TextFrame,
                                MUIA_Background, LIST_BG,
                            End),
                        End,
                    End,
                End,
            End,
        End),

        TAG_DONE
    );

    if (self != NULL)
    {
        SETUP_INST_DATA;
        
        data->aad_Window = window;
        data->aad_Pool   = pool;

        /* QRCode.mcc is optional. Reuse the exact localized CTA URL; do not
         * invent a language mapping or a second, different website target.
         */
        {
            char qrURL[128];
            ULONG qrLength = 0;
            if (qrGroup != NULL &&
                AboutAROSWebsiteURL(_(MSG_MORE_INFORMATION), qrURL,
                                    sizeof(qrURL), &qrLength))
            {
                LONG side = qrLength <= 26 ? 99 : (qrLength <= 42 ? 111 : 123);
                IPTR qrStatus = MUIV_QRCode_Status_Invalid;
                Object *qrCode = MUI_NewObject(MUIC_QRCode,
                    MUIA_QRCode_Text, (IPTR)qrURL,
                    MUIA_QRCode_ECC, MUIV_QRCode_ECC_M,
                    MUIA_Background, WINDOW_BG,
                    MUIA_QRCode_ForegroundPenSpec, (IPTR)&(const struct MUI_PenSpec){ "s3" },
                    MUIA_QRCode_Transparent, TRUE,
                    MUIA_FixWidth, side,
                    MUIA_FixHeight, side,
                    TAG_DONE);
                if (qrCode != NULL)
                {
                    if (GetAttr(MUIA_QRCode_Status, qrCode, &qrStatus) &&
                        qrStatus == MUIV_QRCode_Status_Ready)
                    {
                        if (!DoMethod(qrGroup, OM_ADDMEMBER, (IPTR)qrCode))
                            MUI_DisposeObject(qrCode);
                    }
                    else
                        MUI_DisposeObject(qrCode);
                }
            }
        }

        /*-- Initialize lists --------------------------------------------------*/
        NamesToList(authorsList, AUTHORS, data);
        NamesToList(sponsorsList, SPONSORS, data);
    
        DoMethod
        (
            acknowledgementsList, MUIM_List_Insert,
            (IPTR) ACKNOWLEDGEMENTS, ACKNOWLEDGEMENTS_SIZE, MUIV_List_Insert_Top
        );
    
        /*-- Setup notifications -----------------------------------------------*/
        DoMethod
        (
            window, MUIM_Notify, MUIA_Window_CloseRequest, TRUE,
            (IPTR) self, 2, MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit
        );
    
        DoMethod
        (
            licenseButton, MUIM_Notify, MUIA_Pressed, FALSE,
            (IPTR) self, 1, MUIM_AboutAROS_ShowLicense
        );
    }
    
    return self;
}

/* Release the instance pool, then forward OM_DISPOSE to the superclass.
 * Return the superclass result. The inherited Zune hierarchy owns
 * the child objects, including a successfully inserted QR object.
 */
IPTR AboutAROS__OM_DISPOSE(Class *CLASS, Object *self, Msg message)
{
    SETUP_INST_DATA;
    
    if (data->aad_Pool != NULL) DeletePool(data->aad_Pool);

    return DoSuperMethodA(CLASS, self, message);
}

/* Open the About window, execute the superclass application loop, and
 * close the window on return. Return zero, not the superclass result.
 */
IPTR AboutAROS__MUIM_Application_Execute(Class *CLASS, Object *self, Msg message)
{
    SETUP_INST_DATA;
    
    SET(data->aad_Window, MUIA_Window_Open, TRUE);
    DoSuperMethodA(CLASS, self, message);
    SET(data->aad_Window, MUIA_Window_Open, FALSE);

    return 0;
}

/* Attempt to open HELP:LICENSE through workbench.library.
 * Return zero without forwarding the OpenWorkbenchObject result.
 */
IPTR AboutAROS__MUIM_AboutAROS_ShowLicense(Class *CLASS, Object *self, Msg message)
{
    OpenWorkbenchObject("HELP:LICENSE", TAG_DONE);

    return 0;
}

/* ZUNE_CUSTOMCLASS_4 generates AboutAROS_Dispatcher, AboutAROS_Initialize
 * and AboutAROS_Deinitialize, plus the AboutAROS_CLASS pointer. The
 * dispatcher routes the four methods above and forwards other methods.
 * ADD2INIT/ADD2EXIT register class creation and deletion. These are
 * macro-owned internal entrypoints, not public AboutAROS interfaces.
 */
/*** Setup ******************************************************************/
ZUNE_CUSTOMCLASS_4
(
    AboutAROS, NULL, MUIC_Application, NULL,
    OM_NEW,                     struct opSet *,
    OM_DISPOSE,                 Msg,
    MUIM_Application_Execute,   Msg,
    MUIM_AboutAROS_ShowLicense, Msg
);
