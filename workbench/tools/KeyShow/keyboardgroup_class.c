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
        set_immutable_key(key, 96, _(MSG_KEY_SHIFT));   // Left Shift
        set_immutable_key(key, 97, _(MSG_KEY_SHIFT));   // Right Shift
        set_immutable_key(key, 98, _(MSG_KEY_LOCK));    // Caps Lock
        set_immutable_key(key, 99, _(MSG_KEY_CTRL));    // (Left) Ctrl
        set_immutable_key(key, 100, _(MSG_KEY_ALT));    // Left Alt
        set_immutable_key(key, 101, _(MSG_KEY_ALT));    // Right Alt
        
        // Special keys
        set_immutable_key(key, 76, "\033I[6:11]");      // Cursor
        set_immutable_key(key, 77, "\033I[6:12]");
        set_immutable_key(key, 78, "\033I[6:14]");
        set_immutable_key(key, 79, "\033I[6:13]");

        set_immutable_key(key, 65, "^H");               // Backspace
        set_immutable_key(key, 66, "^I");               // Tab
        set_immutable_key(key, 68, "^M");               // Enter
        set_immutable_key(key, 69, "^[");               // Esc
        set_immutable_key(key, 95, _(MSG_KEY_HELP));
        set_immutable_key(key, 80, "F1");
        set_immutable_key(key, 81, "F2");
        set_immutable_key(key, 82, "F3");
        set_immutable_key(key, 83, "F4");
        set_immutable_key(key, 84, "F5");
        set_immutable_key(key, 85, "F6");
        set_immutable_key(key, 86, "F7");
        set_immutable_key(key, 87, "F8");
        set_immutable_key(key, 88, "F9");
        set_immutable_key(key, 89, "F10");
        set_immutable_key(key, 75, "F11");
        set_immutable_key(key, 111, "F12");
        set_immutable_key(key, 102, _(MSG_KEY_A));      // Left A
        set_immutable_key(key, 103, _(MSG_KEY_A));      // Right A

        set_immutable_key(key, 71, _(MSG_KEY_INSERT));
        set_immutable_key(key, 112, _(MSG_KEY_HOME));
        set_immutable_key(key, 72, _(MSG_KEY_PAGEUP));
        set_immutable_key(key, 70, _(MSG_KEY_DELETE));
        set_immutable_key(key, 113, _(MSG_KEY_END));
        set_immutable_key(key, 73, _(MSG_KEY_PAGEDOWN));

        set_immutable_key(key, 90, _(MSG_KEY_NUM));
        set_immutable_key(key, 91, "/");
        set_immutable_key(key, 92, "*");
        set_immutable_key(key, 93, "-");
        set_immutable_key(key, 94, "+");
        set_immutable_key(key, 67, "^M");               // Numpad Enter

        set_immutable_key(key, 127, _(MSG_KEY_CTRL));   // Pseudo right Ctrl
    }
    return key;
}

AROS_UFH3S(void, change_qualifier_func,
    AROS_UFHA(struct Hook *, h, A0),
    AROS_UFHA(Object *, obj, A2),
    AROS_UFHA(APTR, msg, A1))
{
    AROS_USERFUNC_INIT

    struct KeyboardGroup_DATA *data = h->h_Data;

    ULONG i;
    BOOL shift = XGET(data->keybutton[96], MUIA_Pressed) | XGET(data->keybutton[97], MUIA_Pressed);
    BOOL alt = XGET(data->keybutton[100], MUIA_Pressed) | XGET(data->keybutton[101], MUIA_Pressed);
    BOOL ctrl = XGET(data->keybutton[99], MUIA_Pressed) | XGET(data->keybutton[127], MUIA_Pressed);

    D(bug("[keyshow/change_qualifier_func] shift %d alt %d ctrl %d\n", shift, alt, ctrl));

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
                MUIA_Group_SameSize, TRUE,
                KEY(69), KEY(80), KEY(81), KEY(82), KEY(83), KEY(84), KEY(85), KEY(86), KEY(87), KEY(88), KEY(89), KEY(75), KEY(111),
            End,
            Child, VSpace(5),
            Child, HGroup,
                Child, HGroup,
                    MUIA_Group_SameSize, TRUE,
                    KEY(0), KEY(1), KEY(2), KEY(3), KEY(4), KEY(5), KEY(6), KEY(7), KEY(8), KEY(9), KEY(10), KEY(11), KEY(12), KEY(13),
                End,
                KEY(65),
            End,
            Child, HGroup,
                KEY(66),
                Child, HGroup,
                    MUIA_Group_SameSize, TRUE,
                    KEY(16), KEY(17), KEY(18), KEY(19), KEY(20), KEY(21), KEY(22), KEY(23), KEY(24), KEY(25), KEY(26), KEY(27),
                End,
                KEY(68),
            End,
            Child, HGroup,
                KEY(98),
                Child, HGroup,
                    MUIA_Group_SameSize, TRUE,
                    KEY(32), KEY(33), KEY(34), KEY(35), KEY(36), KEY(37), KEY(38), KEY(39), KEY(40), KEY(41), KEY(42), KEY(43),
                End,
                Child, HVSpace,
            End,
            Child, HGroup,
                KEY(96),
                Child, HGroup,
                    MUIA_Group_SameSize, TRUE,
                    KEY(48), KEY(49), KEY(50), KEY(51), KEY(52), KEY(53), KEY(54), KEY(55), KEY(56), KEY(57), KEY(58),
                End,
                KEY(97),
            End,
            Child, HGroup,
                Child, HGroup,
                    MUIA_Group_SameSize, TRUE,
                    KEY(99), KEY(102), KEY(100),
                End,
                KEY(64),
                Child, HGroup,
                    MUIA_Group_SameSize, TRUE,
                    KEY(101), KEY(103), KEY(95), KEY(127),
                End,
            End,
        End,
        Child, VGroup,
            Child, ColGroup(3),
                GroupFrame,
                MUIA_Group_SameSize, TRUE,
                KEY(71), KEY(112), KEY(72),
                KEY(70), KEY(113), KEY(73),
            End,
            Child, HVSpace,
            Child, ColGroup(3),
                GroupFrame,
                MUIA_Group_SameSize, TRUE,
                Child, HVSpace,
                KEY(76),
                Child, HVSpace,
                KEY(79), KEY(77), KEY(78),
            End,
        End,
        Child, VGroup,
            Child, HVSpace,
                Child, ColGroup(4),
                GroupFrame,
                MUIA_Group_SameSize, TRUE,
                KEY(90), KEY(91), KEY(92), KEY(93),
                KEY(61), KEY(62), KEY(63), KEY(94),
                KEY(45), KEY(46), KEY(47), Child, HVSpace,
                KEY(29), KEY(30), KEY(31), KEY(67),
                KEY(15), Child, HVSpace, KEY(60), Child, HVSpace,
            End,
        End,
        TAG_DONE
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
                data->keybutton[i], MUIM_Notify, MUIA_Pressed, MUIV_EveryTime,
                self, 2, MUIM_CallHook, &data->change_qualifier_hook
            );
        }
        DoMethod
        (
            data->keybutton[127], MUIM_Notify, MUIA_Pressed, MUIV_EveryTime,
            self, 2, MUIM_CallHook, &data->change_qualifier_hook
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
                        SET(btn, MUIA_Selected, XGET(btn, MUIA_Selected) ? FALSE : TRUE); // FIXME: this doesn't trigger the callback hook
                        MUI_Redraw(obj, MADF_DRAWUPDATE);
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
