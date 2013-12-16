/*
    Copyright � 2002-2011, The AROS Development Team. All rights reserved.
    $Id$
*/

#define MUIMASTER_YES_INLINE_STDARG

#include <proto/alib.h>
#include <proto/intuition.h>
#include <intuition/classusr.h>
#include <libraries/gadtools.h>
#include <proto/dos.h>
#include <proto/utility.h>
#include <proto/muimaster.h>
#include <string.h>

#ifdef HAVE_COOLIMAGES
#include <libraries/coolimages.h>
#include <proto/coolimages.h>
#endif

#include "muimaster_intern.h"
#include "mui.h"

/* #define MYDEBUG 1 */
#include "debug.h"

extern struct Library *MUIMasterBase;

STATIC int get_control_char(const char *label)
{
    /* find the control char */
    int control_char = 0;

    if (label)
    {
        const unsigned char *p = (const unsigned char *)label;
        unsigned char c;

        while ((c = *p++))
        {
            if (c == '_')
            {
                control_char = ToLower(*p);
                break;
            }
        }

    }
    return control_char;
}

STATIC Object *CreateMenuString(struct NewMenu * newmenu, ULONG flags,
    struct Library * MUIMasterBase)
{
    int i = 0;
    UWORD nmflags;

    Object *menustrip = (Object *) MenuitemObject, End;
    Object *menu = NULL;
    Object *menuitem = NULL;

    if (!menustrip)
        return NULL;

    for (; newmenu[i].nm_Type != NM_END; i++)
    {
        nmflags = newmenu[i].nm_Flags;

        if (newmenu[i].nm_Type == NM_TITLE)
        {
            menu = (Object *)MenuitemObject, 
                MUIA_Menuitem_Title,   (IPTR)newmenu[i].nm_Label, 
                MUIA_UserData,         (IPTR)newmenu[i].nm_UserData,
                MUIA_Menuitem_Enabled, (nmflags & NM_MENUDISABLED) == 0,
                End;
            if (menu)
                DoMethod(menustrip, MUIM_Family_AddTail, (IPTR) menu);
            menuitem = NULL;
        }
        else
        {
            char *label = (char *)newmenu[i].nm_Label;
            char *commkey;

            if ((flags & MUIO_MenustripNM_CommandKeyCheck)
                && label != (char *)NM_BARLABEL && label[1] == '\0')
            {
                label += 2;
                commkey = label;
            }
            else
                commkey = (char *)newmenu[i].nm_CommKey;

            if (newmenu[i].nm_Type == NM_ITEM || (menuitem != NULL
                    && newmenu[i].nm_Type == NM_SUB))
            {
                Object *item = (Object *)(MenuitemObject,
                    MUIA_Menuitem_Title,    (IPTR)label,
                    MUIA_Menuitem_Shortcut, (IPTR)commkey,
                    MUIA_UserData,          (IPTR)newmenu[i].nm_UserData,
                    MUIA_Menuitem_Exclude,  (IPTR)newmenu[i].nm_MutualExclude,
                    MUIA_Menuitem_Checkit,  (nmflags & CHECKIT) != 0,
                    MUIA_Menuitem_Checked,  (nmflags & CHECKED) != 0,
                    MUIA_Menuitem_Toggle,   (nmflags & MENUTOGGLE) != 0,
                    MUIA_Menuitem_CommandString,
                        (nmflags & NM_COMMANDSTRING) != 0,
                    MUIA_Menuitem_Enabled,  (nmflags & NM_ITEMDISABLED) == 0,
                End);

                if (item != NULL)
                {
                    if (newmenu[i].nm_Type == NM_ITEM)
                    {
                        DoMethod(menu, MUIM_Family_AddTail, (IPTR) item);
                        menuitem = item;
                    }
                    else
                        DoMethod(menuitem, MUIM_Family_AddTail,
                            (IPTR) item);
                }
            }
        }
    }
    return menustrip;
}


Object *INTERNAL_ImageButton(CONST_STRPTR label, CONST_STRPTR imagePath,
    struct TextFont * textFont)
{
#   define BUFFERSIZE 512

    BPTR lock = Lock(imagePath, ACCESS_READ);

    D(bug("[ImageButton] Font 0x%p\n", textFont));
    if (lock != BNULL)
    {

        TEXT imageSpec[BUFFERSIZE];
        TEXT controlChar = get_control_char(label);

        imageSpec[0] = '\0';
        strlcat(imageSpec, "3:", BUFFERSIZE);
        strlcat(imageSpec, imagePath, BUFFERSIZE);

        UnLock(lock);

        return (Object *) HGroup,
            ButtonFrame,
            MUIA_Background, MUII_ButtonBack,
            MUIA_CycleChain, 1,
            MUIA_InputMode, MUIV_InputMode_RelVerify,
            MUIA_Group_Spacing, 0,
            //MUIA_Group_SameHeight, TRUE,
            controlChar ? MUIA_ControlChar : TAG_IGNORE, (IPTR) controlChar,
            
            Child, (IPTR)HVSpace,
            Child, (IPTR)ImageObject,
                MUIA_Image_Spec, (IPTR) imageSpec,
                MUIA_Image_FreeVert,    FALSE,
                MUIA_Image_FreeHoriz,   FALSE,
                MUIA_Image_FontMatch,   TRUE,
                MUIA_Font,       (IPTR) textFont,
                MUIA_Weight,            0,
            End,
            Child, (IPTR)HSpace(4),
            Child, (IPTR)TextObject,
                MUIA_Font,           (IPTR) MUIV_Font_Button,
                MUIA_Text_HiCharIdx, (IPTR) '_',
                MUIA_Text_Contents,  (IPTR) label,
                MUIA_Text_PreParse,  (IPTR) "\33c",
                MUIA_Weight,                0,
            End,
            Child, (IPTR)HVSpace,
        End;    

    }
    else
    {
        return SimpleButton(label);
    }

#   undef BUFFERSIZE
}

/*****************************************************************************

    NAME */
        AROS_LH2(Object *, MUI_MakeObjectA,

/*  SYNOPSIS */
        AROS_LHA(LONG, type, D0),
        AROS_LHA(IPTR *, params, A0),

/*  LOCATION */
        struct Library *, MUIMasterBase, 20, MUIMaster)

/*  FUNCTION
        Create an object from the builtin object collection.
        See file libraries/mui.h for a list of available objects
        and their parameters. Note that this is not a taglist.

    INPUTS
        type   - kind of builtin object
        params - additional parameters for object

    RESULT
        Pointer to object

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        MUI_CreateCustomClass(), MUI_DeleteCustomClass()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    switch (type)
    {
    case MUIO_Label:           /* STRPTR label, ULONG flags */
        {
            struct TagItem tags[5];
            int ntags = 0;

            if (params[1] & MUIO_Label_SingleFrame)
            {
                tags[ntags].ti_Tag = MUIA_Frame;
                tags[ntags].ti_Data = (IPTR) MUIV_Frame_ImageButton;
                ntags++;
            }
            else if (params[1] & MUIO_Label_DoubleFrame)
            {
                tags[ntags].ti_Tag = MUIA_Frame;
                tags[ntags].ti_Data = (IPTR) MUIV_Frame_String;
                ntags++;
            }

            if (params[1] & MUIO_Label_LeftAligned)
            {
                tags[ntags].ti_Tag = MUIA_Text_PreParse;
                tags[ntags].ti_Data = (IPTR) "\33l";
                ntags++;
            }
            else if (params[1] & MUIO_Label_Centered)
            {
                tags[ntags].ti_Tag = MUIA_Text_PreParse;
                tags[ntags].ti_Data = (IPTR) "\33c";
                ntags++;
            }
            else
            {
                tags[ntags].ti_Tag = MUIA_Text_PreParse;
                tags[ntags].ti_Data = (IPTR) "\33r";
                ntags++;
            }

            if (params[1] & MUIO_Label_FreeVert)
            {
                tags[ntags].ti_Tag = MUIA_Text_SetVMax;
                tags[ntags].ti_Data = (IPTR) FALSE;
                ntags++;
            }

            tags[ntags].ti_Tag = TAG_DONE;
            tags[ntags].ti_Data = 0;

            return MUI_NewObject(MUIC_Text,
                MUIA_FramePhantomHoriz, TRUE,
                MUIA_Weight, 0,
                //MUIA_Text_SetMax, TRUE,
                (params[1] & 0xff) ? MUIA_Text_HiChar : TAG_IGNORE,
                    params[1] & 0xff,
                (params[1] & 0xff) ? TAG_IGNORE : MUIA_Text_HiCharIdx, '_',
                MUIA_Text_Contents, params[0],
                TAG_MORE, tags,
                TAG_DONE);
            break;
        }

    case MUIO_CoolButton: /* STRPTR label, APTR CoolImage, ULONG flags */
#ifdef HAVE_COOLIMAGES
        if (CoolImagesBase)
        {
            struct CoolImage *img;
            char *label = (char*)params[0];
            int control_char = 0;
            ULONG flags = params[2];

            if (flags & MUIO_CoolButton_CoolImageID)
            {
                img = (struct CoolImage *)COOL_ObtainImageA(params[1], NULL);
            }
            else
            {
                img = (struct CoolImage *)params[1];
            }
            
            if (img)
            {
                control_char = get_control_char(label);
                
                return (Object *)HGroup,
                    MUIA_VertWeight, 0,
                    ButtonFrame,
                    MUIA_Background, MUII_ButtonBack,
                    MUIA_InputMode, MUIV_InputMode_RelVerify,
                    MUIA_CycleChain, 1,
                    control_char ? MUIA_ControlChar : TAG_IGNORE, control_char,
                    MUIA_Group_HorizSpacing, 0,
                    MUIA_Group_SameHeight, TRUE,
                    Child, (IPTR)HSpace(2),
                    Child, (IPTR)VGroup,
                        MUIA_Group_VertSpacing, 0,
                        Child, (IPTR)RectangleObject, End,
                        Child, (IPTR)ChunkyImageObject,
                            MUIA_FixWidth,              img->width,
                            MUIA_FixHeight,             img->height,
                            MUIA_ChunkyImage_Pixels,    (IPTR)img->data,
                            MUIA_ChunkyImage_NumColors, img->numcolors,
                            MUIA_ChunkyImage_Palette,   (IPTR)img->pal,
                            MUIA_Bitmap_Width,          img->width,
                            MUIA_Bitmap_Height,         img->height,
                            MUIA_Bitmap_UseFriend,      TRUE,
                            MUIA_Bitmap_Transparent,    0,
                        End,
                        Child, (IPTR)RectangleObject, End,
                    End,
                    Child, (IPTR)HSpace(2),
                    Child, (IPTR)RectangleObject, End,
                    Child, (IPTR)VGroup,
                        MUIA_Group_VertSpacing, 0,
                        Child, (IPTR)RectangleObject, End,
                        Child, (IPTR)TextObject,
                            MUIA_Font, (IPTR)MUIV_Font_Button,
                            MUIA_Text_HiCharIdx, '_',
                            MUIA_Text_Contents, (IPTR)label,
                            MUIA_Text_SetMax, TRUE,
                        End,
                        Child, (IPTR)RectangleObject, End,
                    End,
                    Child, (IPTR)RectangleObject, End,
                    Child, (IPTR)HSpace(2),
                End;
                    
            } /* if (img) */
            
        } /* if (CoolImagesBase) */
#endif
        /* fall through */

    case MUIO_Button: /* STRPTR label */
        {
            int control_char = get_control_char((const char *)params[0]);

            return MUI_NewObject(MUIC_Text,
                ButtonFrame,
                MUIA_Font, MUIV_Font_Button,
                MUIA_Text_HiCharIdx, '_',
                MUIA_Text_Contents, params[0],
                MUIA_Text_PreParse, "\33c",
                MUIA_InputMode    , MUIV_InputMode_RelVerify,
                MUIA_Background   , MUII_ButtonBack,
                control_char ? MUIA_ControlChar : TAG_IGNORE, control_char,
                MUIA_CycleChain,    1,
                TAG_DONE);
        }
    case MUIO_ImageButton: 
        return INTERNAL_ImageButton
        ( 
            (CONST_STRPTR) params[0],
            (CONST_STRPTR) params[1],
            ((struct MUIMasterBase_intern *)MUIMasterBase)->topaz8font
        );

    case MUIO_Checkmark: /* STRPTR label */
        {
            int control_char = get_control_char((const char *)params[0]);
            
            return MUI_NewObject(MUIC_Image,
                ImageButtonFrame,
                MUIA_Background, MUII_ButtonBack,
                MUIA_Weight,0,
                MUIA_Image_Spec, MUII_CheckMark,
                MUIA_InputMode, MUIV_InputMode_Toggle,
                MUIA_Image_FreeVert, TRUE,
                MUIA_ShowSelState, FALSE,
                control_char ? MUIA_ControlChar : TAG_IGNORE, control_char,
                TAG_DONE);
            break;
        }

    case MUIO_Cycle: /* STRPTR label, STRPTR *entries */
        {
            int control_char = get_control_char((const char *)params[0]);

            return MUI_NewObject(MUIC_Cycle,
                ButtonFrame,
                MUIA_Font, MUIV_Font_Button,
                MUIA_Cycle_Entries, params[1],
                control_char ? MUIA_ControlChar : TAG_IGNORE, control_char,
                MUIA_CycleChain, 1,
                TAG_DONE);
            break;
        }
    case MUIO_Radio: /* STRPTR label, STRPTR *entries */
        {
            int control_char = get_control_char((const char *)params[0]);

            return MUI_NewObject(MUIC_Radio,
                MUIA_Radio_Entries, params[1],
                control_char ? MUIA_ControlChar : TAG_IGNORE, control_char,
                TAG_DONE);
            break;
        }


    case MUIO_Slider: /* STRPTR label, LONG min, LONG max */
        {
            int control_char = get_control_char((const char *)params[0]);

            return (Object *)SliderObject,
                MUIA_Numeric_Min, params[1],
                MUIA_Numeric_Max, params[2],
                control_char ? MUIA_ControlChar : TAG_IGNORE, control_char,
                End;
            break;
        }
    case MUIO_String: /* STRPTR label, LONG maxlen */
        {
            int control_char = get_control_char((const char *)params[0]);

            return MUI_NewObject(MUIC_String,
                StringFrame,
                    MUIA_String_MaxLen,params[1],
                control_char ? MUIA_ControlChar : TAG_IGNORE, control_char,
                    TAG_DONE);
            break;
        }
    case MUIO_PopButton: /* STRPTR imagespec */
        return MUI_NewObject(MUIC_Image,
            ImageButtonFrame,
            MUIA_Background, MUII_ButtonBack,
            MUIA_Image_Spec, params[0],
            MUIA_InputMode, MUIV_InputMode_RelVerify,
            MUIA_Image_FreeVert, TRUE,
            MUIA_Image_FreeHoriz, FALSE,
            TAG_DONE);
        break;

    case MUIO_HSpace: /* LONG space   */
        return MUI_NewObject(MUIC_Rectangle,
            MUIA_VertWeight, 0,
            MUIA_FixWidth, params[0],
            TAG_DONE);
        break;

    case MUIO_VSpace: /* LONG space   */
        return MUI_NewObject(MUIC_Rectangle,
            MUIA_HorizWeight, 0,
            MUIA_FixHeight, params[0],
            TAG_DONE);
        break;

    case MUIO_HBar: /* LONG space   */
        return MUI_NewObject(MUIC_Rectangle,
            MUIA_Rectangle_HBar, TRUE,
            MUIA_FixHeight, params[0],
            TAG_DONE);
        break;

    case MUIO_VBar: /* LONG space   */
        return MUI_NewObject(MUIC_Rectangle,
            MUIA_Rectangle_VBar, TRUE,
            MUIA_FixWidth, params[0],
            TAG_DONE);
        break;

    case MUIO_MenustripNM: /* struct NewMenu *nm, ULONG flags */
        return CreateMenuString((struct NewMenu *)params[0], params[1],
            MUIMasterBase);

    case MUIO_Menuitem: /* STRPTR label, STRPTR shortcut, ULONG flags, ULONG data  */
        return MUI_NewObject( MUIC_Menuitem,
            MUIA_Menuitem_Title, params[0],
            MUIA_Menuitem_Shortcut, params[1],
            MUIA_Menuitem_Checkit, !!(params[2] & CHECKIT),
            MUIA_Menuitem_Checked, !!(params[2] & CHECKED),
            MUIA_Menuitem_Toggle,  !!(params[2] & MENUTOGGLE),
            MUIA_Menuitem_Enabled, !(params[2] & NM_ITEMDISABLED),
            /* flags NYI */
            MUIA_UserData, params[3],
            TAG_DONE);

    case MUIO_BarTitle: /* STRPTR label */
        return MUI_NewObject(MUIC_Rectangle,
            MUIA_Rectangle_HBar, TRUE,
            MUIA_Rectangle_BarTitle, params[0],
            MUIA_FixHeight, 1,
            TAG_DONE);
        break;

    case MUIO_NumericButton: /* STRPTR label, LONG min, LONG max, STRPTR format */
        {
            int control_char = get_control_char((const char *)params[0]);

            return MUI_NewObject(MUIC_Numericbutton,
                MUIA_Numeric_Min, params[1],
                MUIA_Numeric_Max, params[2],
                (params[3] ? MUIA_Numeric_Format : TAG_IGNORE), params[3],
                (control_char ? MUIA_ControlChar: TAG_IGNORE), control_char,
                TAG_DONE);
        }
        break;
    }

    return NULL;

    AROS_LIBFUNC_EXIT
}                               /* MUIA_MakeObjectA */
