/*
    Copyright © 2012, The AROS Development Team. All rights reserved.
    $Id$
*/

//#include <proto/dos.h>
#include <proto/muimaster.h>
#include <proto/intuition.h>
#include <proto/keymap.h>
//#include <proto/asl.h>
//#include <proto/utility.h>
#include <proto/alib.h>

#define DEBUG 1
#include <aros/debug.h>

#include <libraries/mui.h>
#include <devices/rawkeycodes.h>
#include <zune/customclasses.h>

#include "keyboardgroup_class.h"
#include "locale.h"

#define KEY(n) Child, keybtn[n]

#define KBUFSIZE (12)

struct Key
{
    TEXT alone[KBUFSIZE];
    TEXT shift[KBUFSIZE];
    TEXT alt[KBUFSIZE];
    TEXT ctrl[KBUFSIZE];
    TEXT shift_alt[KBUFSIZE];
    TEXT ctrl_alt[KBUFSIZE];
    TEXT ctrl_shift[KBUFSIZE];
    BOOL immutable;
};

struct KeyboardGroup_DATA
{
    struct Key *key;
    Object *keybutton[128]; // 64-127 are high keys
    struct Hook change_qualifier_hook;
};


static void set_immutable_key(struct Key *key, ULONG idx, CONST_STRPTR content)
{
    strlcpy(key[idx].alone, content, KBUFSIZE);
    key[idx].immutable = TRUE;
}


static struct Key *read_keymap(void)
{
    struct Key *key = AllocVec(sizeof(struct Key) * 128, MEMF_CLEAR);
    if (key)
    {
        struct KeyMap *km = AskKeyMapDefault();
        LONG i;
        for (i = 0; i < 64; i++)
        {
            ULONG value = km->km_LoKeyMap[i];
            switch (km->km_LoKeyMapTypes[i])
            {
                case KC_NOQUAL:
                    key[i].alone[0]     = value & 0xff;
                    break;
                case KCF_SHIFT:
                    key[i].alone[0]     = value & 0xff;
                    key[i].shift[0]     = (value >> 8) & 0xff;
                    break;
                case KCF_ALT:
                    key[i].alone[0]     = value & 0xff;
                    key[i].alt[0]       = (value >> 8) & 0xff;
                    break;
                case KCF_CONTROL:
                    key[i].alone[0]     = value & 0xff;
                    key[i].ctrl[0]      = (value >> 8) & 0xff;
                    break;
                case KCF_ALT + KCF_SHIFT:
                    key[i].alone[0]     = value & 0xff;
                    key[i].shift[0]     = (value >> 8) & 0xff;
                    key[i].alt[0]       = (value >> 16) & 0xff;
                    key[i].shift_alt[0] = (value >> 24) & 0xff;
                    break;
                case KCF_CONTROL + KCF_ALT:
                    key[i].alone[0]     = value & 0xff;
                    key[i].alt[0]       = (value >> 8) & 0xff;
                    key[i].ctrl[0]      = (value >> 16) & 0xff;
                    key[i].ctrl_alt[0]  = (value >> 24) & 0xff;
                    break;
                case KCF_CONTROL + KCF_SHIFT:
                    key[i].alone[0]     = value & 0xff;
                    key[i].shift[0]     = (value >> 8) & 0xff;
                    key[i].ctrl[0]      = (value >> 16) & 0xff;
                    key[i].ctrl_shift[0]= (value >> 24) & 0xff;
                    break;
                case KC_VANILLA:
                    key[i].alone[0]     = value & 0xff;
                    key[i].shift[0]     = (value >> 8) & 0xff;
                    key[i].alt[0]       = (value >> 16) & 0xff;
                    key[i].shift_alt[0] = (value >> 24) & 0xff;
                    key[i].ctrl[0]      = '^';
                    key[i].ctrl[1]      = value & 0xff;
                    break;
            }
        }
        // Qualifier keys
        set_immutable_key(key, RAWKEY_LSHIFT, _(MSG_KEY_SHIFT));
        set_immutable_key(key, RAWKEY_RSHIFT, _(MSG_KEY_SHIFT));
        set_immutable_key(key, RAWKEY_CAPSLOCK, _(MSG_KEY_LOCK));
        set_immutable_key(key, RAWKEY_LCONTROL, _(MSG_KEY_CTRL));
        set_immutable_key(key, RAWKEY_LALT, _(MSG_KEY_ALT));
        set_immutable_key(key, RAWKEY_RALT, _(MSG_KEY_ALT));
        
        // Special keys
        set_immutable_key(key, RAWKEY_UP, "\033I[6:11]");
        set_immutable_key(key, RAWKEY_DOWN, "\033I[6:12]");
        set_immutable_key(key, RAWKEY_RIGHT, "\033I[6:14]");
        set_immutable_key(key, RAWKEY_LEFT, "\033I[6:13]");

        set_immutable_key(key, RAWKEY_BACKSPACE, "^H");
        set_immutable_key(key, RAWKEY_TAB, "^I");
        set_immutable_key(key, RAWKEY_RETURN, "^M");
        set_immutable_key(key, RAWKEY_ESCAPE, "^[");
        set_immutable_key(key, RAWKEY_HELP, _(MSG_KEY_HELP));
        set_immutable_key(key, RAWKEY_F1, "F1");
        set_immutable_key(key, RAWKEY_F2, "F2");
        set_immutable_key(key, RAWKEY_F3, "F3");
        set_immutable_key(key, RAWKEY_F4, "F4");
        set_immutable_key(key, RAWKEY_F5, "F5");
        set_immutable_key(key, RAWKEY_F6, "F6");
        set_immutable_key(key, RAWKEY_F7, "F7");
        set_immutable_key(key, RAWKEY_F8, "F8");
        set_immutable_key(key, RAWKEY_F9, "F9");
        set_immutable_key(key, RAWKEY_F10, "F10");
        set_immutable_key(key, RAWKEY_F11, "F11");
        set_immutable_key(key, RAWKEY_F12, "F12");
        set_immutable_key(key, RAWKEY_LAMIGA, _(MSG_KEY_A));
        set_immutable_key(key, RAWKEY_RAMIGA, _(MSG_KEY_A));

        set_immutable_key(key, RAWKEY_INSERT, _(MSG_KEY_INSERT));
        set_immutable_key(key, RAWKEY_HOME, _(MSG_KEY_HOME));
        set_immutable_key(key, RAWKEY_PAGEUP, _(MSG_KEY_PAGEUP));
        set_immutable_key(key, RAWKEY_DELETE, _(MSG_KEY_DELETE));
        set_immutable_key(key, RAWKEY_END, _(MSG_KEY_END));
        set_immutable_key(key, RAWKEY_PAGEDOWN, _(MSG_KEY_PAGEDOWN));

        set_immutable_key(key, RAWKEY_NUMLOCK, _(MSG_KEY_NUM));
        set_immutable_key(key, RAWKEY_KP_DIVIDE, "/");
        set_immutable_key(key, RAWKEY_KP_MULTIPLY, "*");
        set_immutable_key(key, RAWKEY_KP_MINUS, "-");
        set_immutable_key(key, RAWKEY_KP_PLUS, "+");
        set_immutable_key(key, RAWKEY_KP_ENTER, "^M");

        set_immutable_key(key, 127, _(MSG_KEY_CTRL));   // Pseudo right Ctrl
    }
    return key;
}

AROS_UFH3S(void, change_qualifier_func,
    AROS_UFHA(struct Hook *, h, A0),
    AROS_UFHA(Object *, obj, A2),
    AROS_UFHA(ULONG *, msg, A1))
{
    AROS_USERFUNC_INIT

    struct KeyboardGroup_DATA *data = h->h_Data;

    ULONG i;
    ULONG selectedkey = msg[0];
    BOOL keydown = msg[1];

    BOOL shift = XGET(data->keybutton[96], MUIA_Selected) | XGET(data->keybutton[97], MUIA_Selected);
    BOOL alt = XGET(data->keybutton[100], MUIA_Selected) | XGET(data->keybutton[101], MUIA_Selected);
    BOOL ctrl = XGET(data->keybutton[99], MUIA_Selected) | XGET(data->keybutton[127], MUIA_Selected);

    D(bug("[keyshow/change_qualifier_func] old: shift %d alt %d ctrl %d key %d trigger %d\n", shift, alt, ctrl, selectedkey, keydown));

    if (keydown)
    {
        if ((selectedkey == RAWKEY_LSHIFT) || (selectedkey == RAWKEY_RSHIFT))
            shift = TRUE;
        else if ((selectedkey == RAWKEY_LALT) || (selectedkey == RAWKEY_RALT))
            alt = TRUE;
        else if ((selectedkey == RAWKEY_LCONTROL) || (selectedkey == 127))
            ctrl = TRUE;
    }
    else
    {
        if ((selectedkey == RAWKEY_LSHIFT) || (selectedkey == RAWKEY_RSHIFT))
            shift = FALSE;
        else if ((selectedkey == RAWKEY_LALT) || (selectedkey == RAWKEY_RALT))
            alt = FALSE;
        else if ((selectedkey == RAWKEY_LCONTROL) || (selectedkey == 127))
            ctrl = FALSE;
    }

    NNSET(data->keybutton[RAWKEY_LSHIFT], MUIA_Selected, shift);
    NNSET(data->keybutton[RAWKEY_RSHIFT], MUIA_Selected, shift);
    NNSET(data->keybutton[RAWKEY_LALT], MUIA_Selected, alt);
    NNSET(data->keybutton[RAWKEY_RALT], MUIA_Selected, alt);
    NNSET(data->keybutton[RAWKEY_LCONTROL], MUIA_Selected, ctrl);
    NNSET(data->keybutton[127], MUIA_Selected, ctrl);

    D(bug("[keyshow/change_qualifier_func] new: shift %d alt %d ctrl %d key %d trigger %d\n", shift, alt, ctrl, selectedkey, keydown));

    if (shift && !alt && !ctrl)
    {
        for (i = 0; i < 128; i++)
        {
            if (!data->key[i].immutable)
                SET(data->keybutton[i], MUIA_Text_Contents, data->key[i].shift);
        }
    }
    else if (!shift && alt && !ctrl)
    {
        for (i = 0; i < 128; i++)
        {
            if (!data->key[i].immutable)
                SET(data->keybutton[i], MUIA_Text_Contents, data->key[i].alt);
        }
    }
    else if (!shift && !alt && ctrl)
    {
        for (i = 0; i < 128; i++)
        {
            if (!data->key[i].immutable)
                SET(data->keybutton[i], MUIA_Text_Contents, data->key[i].ctrl);
        }
    }
    else if (shift && alt && !ctrl)
    {
        for (i = 0; i < 128; i++)
        {
            if (!data->key[i].immutable)
                SET(data->keybutton[i], MUIA_Text_Contents, data->key[i].shift_alt);
        }
    }
    else if (!shift && alt && ctrl)
    {
        for (i = 0; i < 128; i++)
        {
            if (!data->key[i].immutable)
                SET(data->keybutton[i], MUIA_Text_Contents, data->key[i].ctrl_alt);
        }
    }
    else if (shift && !alt && ctrl)
    {
        for (i = 0; i < 128; i++)
        {
            if (!data->key[i].immutable)
                SET(data->keybutton[i], MUIA_Text_Contents, data->key[i].ctrl_shift);
        }
    }
    else
    {
        for (i = 0; i < 128; i++)
        {
            if (!data->key[i].immutable)
                SET(data->keybutton[i], MUIA_Text_Contents, data->key[i].alone);
        }
    }

    AROS_USERFUNC_EXIT
}


Object *KeyboardGroup__OM_NEW(Class *CLASS, Object *self, struct opSet *message)
{
    LONG i;
    Object *keybtn[128];
    struct Key *key = read_keymap();

    //struct TagItem  *tstate = message->ops_AttrList;
    //struct TagItem  *tag    = NULL;

#if 0
    while ((tag = NextTagItem(&tstate)) != NULL)
    {
        switch (tag->ti_Tag)
        {
            case MUIA_UnarcWindow_Archive:
                archive = (STRPTR)tag->ti_Data;
                break;

            case MUIA_UnarcWindow_Destination:
                destination = (STRPTR)tag->ti_Data;
                break;
        }
    }
#endif

    for (i = 0; i < 128; i++)
    {
        if ((i < 96 || i > 101) && i != 127 && i != 98)
        {
            keybtn[i] = MUI_NewObject(MUIC_Text,
                ButtonFrame,
                MUIA_Font, MUIV_Font_Tiny,
                MUIA_Text_Contents, key[i].alone,
                MUIA_Text_PreParse, "\33c",
                TAG_DONE);
        }
        else
        {
            // Qualifier keys
            keybtn[i] = MUI_NewObject(MUIC_Text,
            ButtonFrame,
            MUIA_Font, MUIV_Font_Tiny,
            MUIA_Text_Contents, key[i].alone,
            MUIA_Text_PreParse, "\33c",
            MUIA_InputMode    , MUIV_InputMode_Toggle,
            MUIA_Background   , MUII_ButtonBack,
            TAG_DONE);
        }
    }

    self = (Object *) DoSuperNewTags
    (
        CLASS, self, NULL,
        MUIA_Group_Horiz, TRUE,
        Child, VGroup,
            GroupFrame,
            Child, HGroup,
                //MUIA_Group_SameSize, TRUE,
                KEY(RAWKEY_ESCAPE), KEY(RAWKEY_F1), KEY(RAWKEY_F2), KEY(RAWKEY_F3), KEY(RAWKEY_F4), KEY(RAWKEY_F5),
                KEY(RAWKEY_F6), KEY(RAWKEY_F7), KEY(RAWKEY_F8), KEY(RAWKEY_F9), KEY(RAWKEY_F10), KEY(RAWKEY_F11), KEY(RAWKEY_F12),
            End,
            Child, VSpace(5),
            Child, HGroup,
                Child, HGroup,
                    MUIA_Group_SameSize, TRUE,
                    KEY(0), KEY(1), KEY(2), KEY(3), KEY(4), KEY(5), KEY(6), KEY(7), KEY(8), KEY(9), KEY(10), KEY(11), KEY(12), KEY(13),
                End,
                KEY(RAWKEY_BACKSPACE),
            End,
            Child, HGroup,
                KEY(RAWKEY_TAB),
                Child, HGroup,
                    MUIA_Group_SameSize, TRUE,
                    KEY(16), KEY(17), KEY(18), KEY(19), KEY(20), KEY(21), KEY(22), KEY(23), KEY(24), KEY(25), KEY(26), KEY(27),
                End,
                KEY(RAWKEY_RETURN),
            End,
            Child, HGroup,
                KEY(RAWKEY_CAPSLOCK),
                Child, HGroup,
                    MUIA_Group_SameSize, TRUE,
                    KEY(32), KEY(33), KEY(34), KEY(35), KEY(36), KEY(37), KEY(38), KEY(39), KEY(40), KEY(41), KEY(42), KEY(43),
                End,
                Child, HVSpace,
            End,
            Child, HGroup,
                KEY(RAWKEY_LSHIFT),
                Child, HGroup,
                    MUIA_Group_SameSize, TRUE,
                    KEY(48), KEY(49), KEY(50), KEY(51), KEY(52), KEY(53), KEY(54), KEY(55), KEY(56), KEY(57), KEY(58),
                End,
                KEY(RAWKEY_RSHIFT),
            End,
            Child, HGroup,
                Child, HGroup,
                    MUIA_Group_SameSize, TRUE,
                    KEY(RAWKEY_LCONTROL), KEY(RAWKEY_LAMIGA), KEY(RAWKEY_LALT),
                End,
                KEY(RAWKEY_SPACE),
                Child, HGroup,
                    MUIA_Group_SameSize, TRUE,
                    KEY(RAWKEY_RALT), KEY(RAWKEY_RAMIGA), KEY(RAWKEY_HELP), KEY(127),
                End,
            End,
        End,
        Child, VGroup,
            Child, ColGroup(3),
                GroupFrame,
                //MUIA_Group_SameSize, TRUE,
                KEY(RAWKEY_INSERT), KEY(RAWKEY_HOME), KEY(RAWKEY_PAGEUP),
                KEY(RAWKEY_DELETE), KEY(RAWKEY_END), KEY(RAWKEY_PAGEDOWN),
            End,
            Child, HVSpace,
            Child, ColGroup(3),
                GroupFrame,
                MUIA_Group_SameSize, TRUE,
                Child, HVSpace,
                KEY(RAWKEY_UP),
                Child, HVSpace,
                KEY(RAWKEY_LEFT), KEY(RAWKEY_DOWN), KEY(RAWKEY_RIGHT),
            End,
        End,
        Child, VGroup,
            Child, HVSpace,
            Child, ColGroup(4),
                GroupFrame,
                MUIA_Group_SameSize, TRUE,
                KEY(RAWKEY_NUMLOCK), KEY(RAWKEY_KP_DIVIDE), KEY(RAWKEY_KP_MULTIPLY), KEY(RAWKEY_KP_MINUS),
                KEY(RAWKEY_KP_7), KEY(RAWKEY_KP_8), KEY(RAWKEY_KP_9), KEY(RAWKEY_KP_PLUS),
                KEY(RAWKEY_KP_4), KEY(RAWKEY_KP_5), KEY(RAWKEY_KP_6), Child, HVSpace,
                KEY(RAWKEY_KP_1), KEY(RAWKEY_KP_2), KEY(RAWKEY_KP_3), KEY(RAWKEY_KP_ENTER),
                KEY(RAWKEY_KP_0), Child, HVSpace, KEY(RAWKEY_KP_DECIMAL), Child, HVSpace,
            End,
        End,
        TAG_MORE, (IPTR)message->ops_AttrList
    );

    if (self)
    {
        struct KeyboardGroup_DATA *data = INST_DATA(CLASS, self);

        data->key = key;
        for (i = 0; i < 128; i++)
        {
            data->keybutton[i] = keybtn[i];
        }

        data->change_qualifier_hook.h_Entry = (HOOKFUNC)change_qualifier_func;
        data->change_qualifier_hook.h_Data = data;

        // add notifications to the qualifier keys
        for (i = 96; i < 104; i++)
        {
            DoMethod
            (
                data->keybutton[i], MUIM_Notify, MUIA_Selected, MUIV_EveryTime,
                self, 4, MUIM_CallHook, &data->change_qualifier_hook, i, MUIV_TriggerValue
            );
        }
        DoMethod
        (
            data->keybutton[127], MUIM_Notify, MUIA_Selected, MUIV_EveryTime,
            self, 4, MUIM_CallHook, &data->change_qualifier_hook, 127, MUIV_TriggerValue
        );
    }
    return self;
}


IPTR KeyboardGroup__OM_DISPOSE(Class *CLASS, Object *self, Msg message)
{
    struct KeyboardGroup_DATA *data = INST_DATA(CLASS, self);

    FreeVec(data->key);

    return DoSuperMethodA(CLASS, self, message);
}


IPTR KeyboardGroup__MUIM_Setup(Class *CLASS, Object *obj, struct MUIP_HandleInput *msg)
{
    if (!DoSuperMethodA(CLASS, obj, msg))
        return FALSE;

    MUI_RequestIDCMP(obj, IDCMP_RAWKEY);

    return TRUE;
}


IPTR KeyboardGroup__MUIM_Cleanup(Class *CLASS, Object *obj, struct MUIP_HandleInput *msg)
{
    MUI_RejectIDCMP(obj,IDCMP_MOUSEBUTTONS|IDCMP_RAWKEY);
    return DoSuperMethodA(CLASS, obj, msg);
}


IPTR KeyboardGroup__MUIM_HandleInput(Class *CLASS, Object *obj, struct MUIP_HandleInput *msg)
{
    struct KeyboardGroup_DATA *data = INST_DATA(CLASS, obj);

    if (msg->imsg)
    {
        switch (msg->imsg->Class)
        {
            case IDCMP_RAWKEY:
            {
                D(bug("Rawkey %d\n", msg->imsg->Code));
                if (msg->imsg->Code > 95 && msg->imsg->Code < 102)      // Qualifier key
                {
                    Object *btn = data->keybutton[msg->imsg->Code];
                    if (btn)
                    {
                        SET(btn, MUIA_Selected, XGET(btn, MUIA_Selected) ? FALSE : TRUE);
                        //MUI_Redraw(obj, MADF_DRAWUPDATE);
                    }
                }
            }
            break;
        }
    }

    return 0;
}

ZUNE_CUSTOMCLASS_5
(
    KeyboardGroup, NULL, MUIC_Group, NULL,
    OM_NEW,             struct opSet *,
    OM_DISPOSE,         Msg,
    MUIM_Setup,         struct MUIP_HandleInput *,
    MUIM_Cleanup,       struct MUIP_HandleInput *,
    MUIM_HandleInput,   struct MUIP_HandleInput *
);
