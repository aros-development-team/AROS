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

#define KBUFSIZE (8)

struct Key
{
    TEXT alone[KBUFSIZE];
    TEXT shift[KBUFSIZE];
    TEXT alt[KBUFSIZE];
    TEXT ctrl[KBUFSIZE];
    TEXT shift_alt[KBUFSIZE];
    TEXT ctrl_alt[KBUFSIZE];
    TEXT ctrl_shift[KBUFSIZE];
};

struct KeyboardGroup_DATA
{
    struct Key *key;
    Object *keybutton[128]; // 64-127 are high keys
    struct Hook change_qualifier_hook;
};

static struct Key *read_keymap(void)
{
    struct Key *key = AllocVec(sizeof(struct Key) * 128, MEMF_CLEAR);
    if (key)
    {
        struct KeyMap *km = AskKeyMapDefault();
        LONG i;
        for (i = 0; i < 128; i++)
        {
            UBYTE type;
            if (i < 64)
            {
                type = km->km_LoKeyMapTypes[i];
            }
            else
            {
                type = km->km_HiKeyMapTypes[i - 64];
            }

            switch (type)
            {
                case KC_NOQUAL:
                    key[i].alone[0]     = km->km_LoKeyMap[i] & 0xff;
                    break;
                case KCF_SHIFT:
                    key[i].alone[0]     = km->km_LoKeyMap[i] & 0xff;
                    key[i].shift[0]     = (km->km_LoKeyMap[i] >> 8) & 0xff;
                    break;
                case KCF_ALT:
                    key[i].alone[0]     = km->km_LoKeyMap[i] & 0xff;
                    key[i].alt[0]       = (km->km_LoKeyMap[i] >> 8) & 0xff;
                    break;
                case KCF_CONTROL:
                    key[i].alone[0]     = km->km_LoKeyMap[i] & 0xff;
                    key[i].ctrl[0]      = (km->km_LoKeyMap[i] >> 8) & 0xff;
                    break;
                case KCF_ALT + KCF_SHIFT:
                    key[i].alone[0]     = km->km_LoKeyMap[i] & 0xff;
                    key[i].shift[0]     = (km->km_LoKeyMap[i] >> 8) & 0xff;
                    key[i].alt[0]       = (km->km_LoKeyMap[i] >> 16) & 0xff;
                    key[i].shift_alt[0] = (km->km_LoKeyMap[i] >> 24) & 0xff;
                    break;
                case KCF_CONTROL + KCF_ALT:
                    key[i].alone[0]     = km->km_LoKeyMap[i] & 0xff;
                    key[i].alt[0]       = (km->km_LoKeyMap[i] >> 8) & 0xff;
                    key[i].ctrl[0]      = (km->km_LoKeyMap[i] >> 16) & 0xff;
                    key[i].ctrl_alt[0]  = (km->km_LoKeyMap[i] >> 24) & 0xff;
                    break;
                case KCF_CONTROL + KCF_SHIFT:
                    key[i].alone[0]     = km->km_LoKeyMap[i] & 0xff;
                    key[i].shift[0]     = (km->km_LoKeyMap[i] >> 8) & 0xff;
                    key[i].ctrl[0]      = (km->km_LoKeyMap[i] >> 16) & 0xff;
                    key[i].ctrl_shift[0]= (km->km_LoKeyMap[i] >> 24) & 0xff;
                    break;
                case KC_VANILLA:
                    key[i].alone[0]     = km->km_LoKeyMap[i] & 0xff;
                    key[i].shift[0]     = (km->km_LoKeyMap[i] >> 8) & 0xff;
                    key[i].alt[0]       = (km->km_LoKeyMap[i] >> 16) & 0xff;
                    key[i].shift_alt[0] = (km->km_LoKeyMap[i] >> 24) & 0xff;
                    key[i].ctrl[0]      = '^';
                    key[i].ctrl[1]      = km->km_LoKeyMap[i] & 0xff;
                    break;
            }
        }
        // Qualifier keys
        strlcpy(key[96].alone, "LShift", KBUFSIZE);
        strlcpy(key[97].alone, "RShift", KBUFSIZE);
        strlcpy(key[98].alone, "Lock", KBUFSIZE);
        strlcpy(key[99].alone, "Ctrl", KBUFSIZE);
        strlcpy(key[100].alone, "LAlt", KBUFSIZE);
        strlcpy(key[101].alone, "RAlt", KBUFSIZE);
        
        // Special keys
        strlcpy(key[76].alone, "^", KBUFSIZE);          // cursor
        strlcpy(key[77].alone, "v", KBUFSIZE);
        strlcpy(key[78].alone, ">", KBUFSIZE);
        strlcpy(key[79].alone, "<", KBUFSIZE);

        strlcpy(key[65].alone, "<-", KBUFSIZE);         // backspace
        strlcpy(key[66].alone, "->|", KBUFSIZE);        // tab
        strlcpy(key[95].alone, "Help", KBUFSIZE);
        strlcpy(key[80].alone, "F1", KBUFSIZE);
        strlcpy(key[81].alone, "F2", KBUFSIZE);
        strlcpy(key[82].alone, "F3", KBUFSIZE);
        strlcpy(key[83].alone, "F4", KBUFSIZE);
        strlcpy(key[84].alone, "F5", KBUFSIZE);
        strlcpy(key[85].alone, "F6", KBUFSIZE);
        strlcpy(key[86].alone, "F7", KBUFSIZE);
        strlcpy(key[87].alone, "F8", KBUFSIZE);
        strlcpy(key[88].alone, "F9", KBUFSIZE);
        strlcpy(key[89].alone, "F10", KBUFSIZE);
        strlcpy(key[75].alone, "F11", KBUFSIZE);
        strlcpy(key[111].alone, "F12", KBUFSIZE);
        strlcpy(key[102].alone, "LAmiga", KBUFSIZE);
        strlcpy(key[103].alone, "RAmiga", KBUFSIZE);

        strlcpy(key[71].alone, "Insert", KBUFSIZE);
        strlcpy(key[112].alone, "Pos 1", KBUFSIZE);
        strlcpy(key[72].alone, "^", KBUFSIZE);
        strlcpy(key[70].alone, "Del", KBUFSIZE);
        strlcpy(key[113].alone, "End", KBUFSIZE);
        strlcpy(key[73].alone, "v", KBUFSIZE);

        strlcpy(key[90].alone, "Num", KBUFSIZE);
        strlcpy(key[91].alone, "/", KBUFSIZE);
        strlcpy(key[92].alone, "*", KBUFSIZE);
        strlcpy(key[93].alone, "-", KBUFSIZE);
        strlcpy(key[94].alone, "+", KBUFSIZE);
        strlcpy(key[67].alone, "<|", KBUFSIZE);
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
    BOOL ctrl = XGET(data->keybutton[99], MUIA_Pressed);

    bug("[keyshow/change_qualifier_func] shift %d alt %d ctrl %d\n", shift, alt, ctrl);

    if (shift && !alt && !ctrl)
    {
        for (i = 0; i < 128; i++)
        {
            SET(data->keybutton[i], MUIA_Text_Contents, data->key[i].shift);
        }
    }
    else if (!shift && alt && !ctrl)
    {
        for (i = 0; i < 128; i++)
        {
            SET(data->keybutton[i], MUIA_Text_Contents, data->key[i].alt);
        }
    }
    else if (!shift && !alt && ctrl)
    {
        for (i = 0; i < 128; i++)
        {
            SET(data->keybutton[i], MUIA_Text_Contents, data->key[i].ctrl);
        }
    }
    else if (shift && alt && !ctrl)
    {
        for (i = 0; i < 128; i++)
        {
            SET(data->keybutton[i], MUIA_Text_Contents, data->key[i].shift_alt);
        }
    }
    else if (!shift && alt && ctrl)
    {
        for (i = 0; i < 128; i++)
        {
            SET(data->keybutton[i], MUIA_Text_Contents, data->key[i].ctrl_alt);
        }
    }
    else if (shift && !alt && ctrl)
    {
        for (i = 0; i < 128; i++)
        {
            SET(data->keybutton[i], MUIA_Text_Contents, data->key[i].ctrl_shift);
        }
    }
    else
    {
        for (i = 0; i < 128; i++)
        {
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
        if (i < 96 || i > 101)
        {
            keybtn[i] = MUI_NewObject(MUIC_Text,
                ButtonFrame,
                MUIA_Font, MUIV_Font_Button,
                MUIA_Text_Contents, key[i].alone,
                MUIA_Text_PreParse, "\33c",
                //MUIA_FixWidthTxt, "W",
                //MUIA_Background   , MUII_ButtonBack,
                TAG_DONE);
        }
        else
        {
            keybtn[i] = MUI_NewObject(MUIC_Text,
            ButtonFrame,
            MUIA_Font, MUIV_Font_Button,
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
                    KEY(101), KEY(103), KEY(95),  // FIXME: right Ctrl key
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
    }
    return self;
}


IPTR KeyboardGroup__OM_DISPOSE(Class *CLASS, Object *self, Msg message)
{
    struct KeyboardGroup_DATA *data = INST_DATA(CLASS, self);

    FreeVec(data->key);

    return DoSuperMethodA(CLASS, self, message);
}


ZUNE_CUSTOMCLASS_2
(
    KeyboardGroup, NULL, MUIC_Group, NULL,
    OM_NEW,             struct opSet *,
    OM_DISPOSE,         Msg
);
