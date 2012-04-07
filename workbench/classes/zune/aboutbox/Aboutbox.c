/*
    Copyright © 2011, Thore Böckelmann. All rights reserved.
    Copyright © 2012, The AROS Development Team. All rights reserved.
    $Id$
*/


#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>
#include <proto/icon.h>
#include <proto/utility.h>
#include <proto/alib.h>

#include <libraries/mui.h>
#include <mui/Rawimage_mcc.h>
#include <workbench/icon.h>

#include "Aboutbox_mcc.h"
#include "Aboutbox_private.h"

#include <string.h>
#include <stdio.h>

#ifndef ICONDRAWA_Transparency
#define ICONDRAWA_Transparency TAG_IGNORE
#endif

#ifndef MAX
#define MAX(a,b) ((a)>(b)?(a):(b))
#endif

#ifndef DtpicObject
#define DtpicObject         MUIOBJMACRO_START(MUIC_Dtpic)
#endif

struct MUI_CustomClass *icon_mcc;

/* ------------------------------------------------------------------------- */

static BOOL FileExists(CONST_STRPTR filename)
{
    if (filename == NULL)
    {
        return FALSE;
    }

    BOOL retval = FALSE;

    BPTR lock = Lock(filename, ACCESS_READ);
    if (lock)
    {
        struct FileInfoBlock *fib = AllocDosObject(DOS_FIB, NULL);
        if (fib)
        {
            BOOL ex = Examine(lock, fib);
            if (ex)
            {
                if (fib->fib_DirEntryType < 0) // File
                {
                    retval = TRUE;
                }
            }
            FreeDosObject(DOS_FIB, fib);
        }
        UnLock(lock);
    }
    return retval;
}

static void clearExpandStr(struct ExpandString *es)
{
    if(es->string != NULL)
        es->string[0] = '\0';
}

static void appendExpandStr(struct ExpandString *es, const char *append)
{
    // add one for the terminating NUL character
    ULONG length = strlen(append)+1;

    if(es->string == NULL || strlen(es->string)+length > es->size)
    {
        char *newString;
        ULONG newSize;

        newSize = es->size + MAX(length, 128);

        if((newString = AllocVec(newSize, MEMF_ANY)) != NULL)
        {
            if(es->string != NULL)
            {
                strlcpy(newString, es->string, newSize);
                FreeVec(es->string);
            }
      else
      {
        newString[0] = '\0';
      }

            es->string = newString;
            es->size = newSize;
        }
    }

    if(es->string != NULL)
    {
        strlcat(es->string, append, es->size);
    }
}

static void freeExpandString(struct ExpandString *es)
{
    if(es->string != NULL)
    {
        FreeVec(es->string);
        es->string = NULL;
    }
}

static char *BuildAppInfo(struct IClass *cl, Object *obj)
{
    struct Data *data = INST_DATA(cl, obj);
    char *info;

    clearExpandStr(&data->appInfo);

    if((info = (char *)XGET(_app(obj), MUIA_Application_Title)) != NULL)
    {
        appendExpandStr(&data->appInfo, "\033b");
        appendExpandStr(&data->appInfo, info);
        appendExpandStr(&data->appInfo, "\033n\n");
    }
    if((info = (char *)XGET(_app(obj), MUIA_Application_Description)) != NULL)
    {
        appendExpandStr(&data->appInfo, info);
        appendExpandStr(&data->appInfo, "\n");
    }
    if((info = (char *)XGET(_app(obj), MUIA_Application_Version)) != NULL)
    {
        // skip the version cookie
        if(strncmp(info, "$VER: ", 6) == 0)
            info += 6;
        appendExpandStr(&data->appInfo, "\n");
        appendExpandStr(&data->appInfo, info);
        appendExpandStr(&data->appInfo, "\n");
    }
    if((info = (char *)XGET(_app(obj), MUIA_Application_Copyright)) != NULL)
    {
        appendExpandStr(&data->appInfo, "\nCopyright ");
        appendExpandStr(&data->appInfo, info);
        appendExpandStr(&data->appInfo, "\n");
    }

    return data->appInfo.string;
}

static char *ParseCredits(struct IClass *cl, Object *obj)
{
    struct Data *data = INST_DATA(cl, obj);
    const char *credits = data->credits;
    char c[2];
    enum { ps_Text, ps_Placeholder } state;

    state = ps_Text;

    clearExpandStr(&data->parsedCredits);
    appendExpandStr(&data->parsedCredits, "\n");

    // set up a trailing NUL for single characters
    c[1] = '\0';

    while((c[0] = *credits++) != '\0')
    {
        switch(state)
        {
            case ps_Text:
            {
                switch(c[0])
                {
                    case '%':
                        state = ps_Placeholder;
                        break;

                    case '\t':
                        appendExpandStr(&data->parsedCredits, "    ");
                        break;

                    default:
                        appendExpandStr(&data->parsedCredits, c);
                        break;
                }
            }
            break;

            case ps_Placeholder:
            {
                switch(c[0])
                {
                    case '%':
                        appendExpandStr(&data->parsedCredits, c);
                        break;

                    case 'a':
                        appendExpandStr(&data->parsedCredits, "Acknowledgements:");
                        break;

                    case 'd':
                        appendExpandStr(&data->parsedCredits, "Documentation:");
                        break;

                    case 'g':
                        appendExpandStr(&data->parsedCredits, "Graphics:");
                        break;

                    case 'i':
                        appendExpandStr(&data->parsedCredits, "Icons:");
                        break;

                    case 'I':
                        appendExpandStr(&data->parsedCredits, "Icon:");
                        break;

                    case 'l':
                        appendExpandStr(&data->parsedCredits, "Translations:");
                        break;

                    case 'L':
                        appendExpandStr(&data->parsedCredits, "Proofreading:");
                        break;

                    case 'm':
                        appendExpandStr(&data->parsedCredits, "Music:");
                        break;

                    case 'p':
                        appendExpandStr(&data->parsedCredits, "Programming:");
                        break;

                    case 'P':
                        appendExpandStr(&data->parsedCredits, "Additional programming:");
                        break;

                    case 's':
                        appendExpandStr(&data->parsedCredits, "Sound effects:");
                        break;

                    case 't':
                        appendExpandStr(&data->parsedCredits, "Thanks to:");
                        break;

                    case 'T':
                        appendExpandStr(&data->parsedCredits, "Special thanks to:");
                        break;

                    case 'W':
                        appendExpandStr(&data->parsedCredits, "Web support:");
                        break;

                    case '?':
                        appendExpandStr(&data->parsedCredits, "Disclaimer:");
                        break;

                    case '$':
                        appendExpandStr(&data->parsedCredits, "License:");
                        break;

                    default:
                        appendExpandStr(&data->parsedCredits, c);
                        break;
                }

                state = ps_Text;
            }
            break;
        }
    }

    return data->parsedCredits.string;
}

/* ------------------------------------------------------------------------- */

#if !defined(__AROS__) && (defined(__VBCC__) || defined(NO_INLINE_STDARG))
#if defined(_M68000) || defined(__M68000) || defined(__mc68000)

// FIXME: is this good for AROS?

struct DiskObject *GetIconTags(MY_CONST_STRPTR name, ... )
{
    return GetIconTagList(name, (struct TagItem *)(&name+1));
}

BOOL GetIconRectangle(struct RastPort *rp, CONST struct DiskObject *icon, CONST STRPTR label, struct Rectangle *rectangle, ...)
{
    return GetIconRectangleA(rp, icon, label, rectangle, (struct TagItem *)(&rectangle+1));
}

VOID DrawIconState(struct RastPort *rp,CONST struct DiskObject *icon,CONST STRPTR label,LONG leftEdge,LONG topEdge,ULONG state,...)
{
    DrawIconStateA(rp, icon, label, leftEdge, topEdge, state, (struct TagItem *)(&state+1));
}

#endif
#endif

/******************************************************************************/

#define MUIB_Icon                  (TAG_USER | 0x00000300)

/*** Attributes *************************************************************/
#define MUIA_Icon_DiskObject       (MUIB_Icon | 0x00000000) /* I-- struct DiskObject * */
#define MUIA_Icon_File             (MUIB_Icon | 0x00000001) /* I-- CONST_STRPTR        */
#define MUIA_Icon_Screen           (MUIB_Icon | 0x00000002) /* I-- struct Screen *     */

IPTR Icon__OM_NEW(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct DiskObject *icon = NULL;
    STRPTR file = NULL;
    struct Screen *screen = NULL;
    struct TagItem *tag, *tstate = msg->ops_AttrList;

    while ((tag = NextTagItem(&tstate)) != NULL)
    {
        switch(tag->ti_Tag)
        {
            case MUIA_Icon_DiskObject:
                icon = (struct DiskObject *)tag->ti_Data;
                break;

            case MUIA_Icon_File:
                file = (STRPTR)tag->ti_Data;
                break;

            case MUIA_Icon_Screen:
                screen = (struct Screen *)tag->ti_Data;
                break;
        }
    }

    if(icon == NULL && file == NULL)
        goto error; /* Must specify one */
    if(icon != NULL && file != NULL)
        goto error; /* Cannot specify both */

    if(icon == NULL && file != NULL)
    {
        if((icon = GetIconTags(file, ICONGETA_Screen, screen,
                                     TAG_DONE)) == NULL)
            goto error;
    }

    if((obj = (Object *)DoSuperNewTags(cl, obj, NULL,
        MUIA_FillArea, TRUE,
        TAG_MORE, msg->ops_AttrList)) != NULL)
    {
        struct Icon_Data *data = INST_DATA(cl, obj);

        data->icon = icon;
    }
    else
    {
        FreeDiskObject(icon);
        goto error;
    }

    return (IPTR)obj;

error:
    return (IPTR)NULL;
}

IPTR Icon__OM_DISPOSE(struct IClass *cl, Object *obj, Msg msg)
{
    struct Icon_Data *data = INST_DATA(cl, obj);

    if(data->icon != NULL)
        FreeDiskObject(data->icon);

    return DoSuperMethodA(cl,obj,msg);
}

IPTR Icon__MUIM_AskMinMax(struct IClass *cl, Object *obj, struct MUIP_AskMinMax *msg)
{
    struct Icon_Data *data = INST_DATA(cl, obj);
    struct Rectangle rect;
    ULONG ret;

    ret = DoSuperMethodA(cl, obj, (Msg)msg);

    memset(&rect, 0, sizeof(rect));
    if(GetIconRectangle(_rp(obj), data->icon, NULL, &rect, ICONDRAWA_Borderless, TRUE,
                                                           ICONDRAWA_Frameless, TRUE,
                                                           TAG_DONE))
    {
        LONG width = rect.MaxX - rect.MinX + 1;
        LONG height = rect.MaxY - rect.MinY + 1;

        msg->MinMaxInfo->MinWidth += width;
        msg->MinMaxInfo->DefWidth += width;
        msg->MinMaxInfo->MaxWidth += width;

        msg->MinMaxInfo->MinHeight += height;
        msg->MinMaxInfo->DefHeight += height;
        msg->MinMaxInfo->MaxHeight += height;
    }

    return ret;
}

IPTR Icon__MUIM_Draw(struct IClass *cl, Object *obj, struct MUIP_Draw *msg)
{
    struct Icon_Data *data = INST_DATA(cl, obj);
    ULONG ret;

    ret = DoSuperMethodA(cl, obj, (Msg)msg);

    if(msg->flags & MADF_DRAWOBJECT)
    {
        DrawIconState(_rp(obj), data->icon, NULL, _mleft(obj), _mtop(obj), IDS_NORMAL, ICONDRAWA_Borderless, TRUE,
                                                                                       ICONDRAWA_Frameless, TRUE,
                                                                                       ICONDRAWA_EraseBackground, FALSE,
                                                                                       ICONDRAWA_Transparency, 255,
                                                                                       TAG_DONE);
    }

    return ret;
}

/* ------------------------------------------------------------------------- */

IPTR Aboutbox__OM_NEW(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct TagItem *tag, *tstate = msg->ops_AttrList;

    Object *logoGroup;
    Object *appInfoText;
    Object *creditsText;
    Object *okButton;

    if((obj = (Object *)DoSuperNewTags(cl, obj, NULL,
        MUIA_Window_ID       , MAKE_ID('A','B','O','X'),
        MUIA_Window_LeftEdge , MUIV_Window_LeftEdge_Centered,
        MUIA_Window_TopEdge  , MUIV_Window_TopEdge_Centered,
//      MUIA_Window_Width    , MUIV_Window_Width_MinMax(0),
//      MUIA_Window_Height   , MUIV_Window_Height_MinMax(0),
        MUIA_Window_Title    , "Aboutbox",
        MUIA_Window_NoMenus  , TRUE,
#if 0
        // FIXME: following don't exist in AROS
        MUIA_Window_ShowAbout, FALSE,
        MUIA_Window_ShowIconify, FALSE,
        MUIA_Window_ShowJump, FALSE,
        MUIA_Window_ShowPopup, FALSE,
        MUIA_Window_ShowPrefs, FALSE,
        MUIA_Window_ShowSnapshot, FALSE,
#endif
        MUIA_Window_UseRightBorderScroller, TRUE,
        MUIA_Window_CloseGadget, FALSE,
        MUIA_Window_SizeRight, TRUE,
        WindowContents, VGroup,
            Child, HGroup,
                Child, logoGroup = VGroup,
                    Child, VSpace(0),
                    End,
                Child, ScrollgroupObject,
                    MUIA_Weight, 100,
                    MUIA_Scrollgroup_FreeHoriz, FALSE,
                    MUIA_Scrollgroup_UseWinBorder, TRUE,
                    MUIA_Scrollgroup_Contents, VGroupV,
                        VirtualFrame,
                        Child, appInfoText = TextObject, End,
                        Child, RectangleObject, MUIA_Rectangle_HBar, TRUE, End,
                        Child, creditsText = TextObject, End,
                        End,
                    End,
                End,
            Child, HGroup,
                Child, HSpace(0),
                Child, okButton = SimpleButton("_Ok"),
                Child, HSpace(0),
                End,
            End,
        TAG_MORE, msg->ops_AttrList)) != NULL)
    {
        struct Data *data = INST_DATA(cl, obj);

        data->logoGroup = logoGroup;
        data->appInfoText = appInfoText;
        data->creditsText = creditsText;
        data->fallbackMode = MUIV_Aboutbox_LogoFallbackMode_Auto;

        while ((tag = NextTagItem(&tstate)) != NULL)
        {
            switch(tag->ti_Tag)
            {
                case MUIA_Aboutbox_Credits:
                    data->credits = (CONST_STRPTR)tag->ti_Data;
                    break;

                case MUIA_Aboutbox_Build:
                    data->build = (CONST_STRPTR)tag->ti_Data;
                    break;

                case MUIA_Aboutbox_LogoFallbackMode:
                    data->fallbackMode = tag->ti_Data;
                    break;

                case MUIA_Aboutbox_LogoFile:
                    data->logoFile = (CONST_STRPTR)tag->ti_Data;
                    break;

                case MUIA_Aboutbox_LogoData:
                    data->logoData = (CONST_APTR)tag->ti_Data;
                    break;
            }
        }

        DoMethod(obj, MUIM_Notify, MUIA_Window_CloseRequest, TRUE, MUIV_Notify_Application, 6, MUIM_Application_PushMethod, obj, 3, MUIM_Set, MUIA_Window_Open, FALSE);
        DoMethod(okButton, MUIM_Notify, MUIA_Pressed, FALSE, MUIV_Notify_Application, 6, MUIM_Application_PushMethod, obj, 3, MUIM_Set, MUIA_Window_Open, FALSE);
    }

    return (IPTR)obj;
}


IPTR Aboutbox__OM_DISPOSE(struct IClass *cl, Object *obj, Msg msg)
{
    struct Data *data = INST_DATA(cl, obj);

    freeExpandString(&data->appInfo);
    freeExpandString(&data->parsedCredits);

    return DoSuperMethodA(cl,obj,msg);
}


static Object *BuildLogo(struct IClass *cl, Object *obj)
{
    struct Data *data = INST_DATA(cl, obj);
    ULONG fallbackMode = data->fallbackMode;
    Object *logo = NULL;

    while(fallbackMode != MUIV_Aboutbox_LogoFallbackMode_NoLogo && logo == NULL)
    {
        ULONG mode;

        mode = (fallbackMode >> 24) & 0xff;
        fallbackMode <<= 8;

        switch(mode)
        {
            case 'D':
                {
                    // try to get the program name the simple way first
                    if(GetProgramName(data->progName, sizeof(data->progName)) == DOSFALSE)
                    {
                        // no CLI structure, construct the name as "PROGDIR:<task name>"
                        struct Task *me = FindTask(NULL);

                        strlcpy(data->progName, "PROGDIR:", sizeof(data->progName));
                        AddPart(data->progName, me->tc_Node.ln_Name, sizeof(data->progName));
                    }
                    else if(strnicmp(data->progName, "APPDIR:", 7) == 0)
                    {
                        char tempPath[256];

                        // an APPDIR: path, construct the name as "PROGDIR:<file part of app path>"
                        strlcpy(tempPath, "PROGDIR:", sizeof(tempPath));
                        AddPart(tempPath, FilePart(data->progName), sizeof(tempPath));
                        strlcpy(data->progName, tempPath, sizeof(data->progName));
                    }

                    logo = NewObject(icon_mcc->mcc_Class, NULL, MUIA_Icon_File, data->progName,
                                                                MUIA_Icon_Screen, XGET(obj, MUIA_Window_Screen),
                                                                TAG_DONE);
                }
                break;

            case 'E':
                {
                    if(data->logoFile != NULL && FileExists(data->logoFile))
                    {
                        logo = DtpicObject,
                            MUIA_Dtpic_Name, data->logoFile,
                        End;
                    }
                }
                break;

            case 'I':
                {
                    if(data->logoData != NULL)
                    {
                        logo = RawimageObject,
                            MUIA_Rawimage_Data, data->logoData,
                        End;
                    }
                }
                break;

            case '\0':
                fallbackMode = MUIV_Aboutbox_LogoFallbackMode_NoLogo;
                break;
        }
    }

    return logo;
}

IPTR Aboutbox__MUIM_Window_Setup(struct IClass *cl, Object *obj, Msg msg)
{
    ULONG ret;

    if((ret = DoSuperMethodA(cl, obj, (Msg)msg)))
    {
        struct Data *data = INST_DATA(cl, obj);

        if((data->logo = BuildLogo(cl, obj)) != NULL)
            DoMethod(data->logoGroup, MUIM_Group_AddHead, data->logo);

        set(data->appInfoText, MUIA_Text_Contents, BuildAppInfo(cl, obj));
        set(data->creditsText, MUIA_Text_Contents, ParseCredits(cl, obj));

        snprintf(data->title, sizeof(data->title), "About %s", (char *)XGET(_app(obj), MUIA_Application_Title));
        set(obj, MUIA_Window_Title, data->title);
    }

    return ret;
}

IPTR Aboutbox__MUIM_Window_Cleanup(struct IClass *cl, Object *obj, Msg msg)
{
    struct Data *data = INST_DATA(cl, obj);

    if(data->logo != NULL)
    {
        DoMethod(data->logoGroup, OM_REMMEMBER, data->logo);
        MUI_DisposeObject(data->logo);
        data->logo = NULL;
    }

    clearExpandStr(&data->appInfo);
    clearExpandStr(&data->parsedCredits);

    return DoSuperMethodA(cl, obj, (Msg)msg);
}
