/*
    Copyright © 2012, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <proto/muimaster.h>
#include <proto/intuition.h>
#include <proto/keymap.h>
#include <proto/utility.h>
#include <proto/alib.h>

//#define DEBUG 1
#include <aros/debug.h>

#include <libraries/mui.h>
#include <devices/rawkeycodes.h>
#include <zune/customclasses.h>

#include <stdio.h>

#include "keyboardgroup_class.h"
#include "locale.h"

#define KEY(n) Child, keybtn[n]

#define KBUFSIZE (12)

// On classic keyboard in top left corner of num pad
#define RAWKEY_KP_LBRACKET     0x5A
#define RAWKEY_KP_RBRACKET     0x5B

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

// copied from rom/keymap/defaultkeymap.c
static const BYTE keymapstr_table[8][8] =
{
    {0, 0, 0, 0, 0, 0, 0, 0},   /* KCF_NOQUAL                   == 0 */
    {0, 1, 0, 1, 0, 1, 0, 1},   /* KCF_SHIFT                    == 1 */
    {0, 0, 1, 1, 0, 0, 1, 1},   /* KCF_ALT                      == 2 */
    {0, 1, 2, 3, 0, 1, 2, 3},   /* KCF_SHIFT|KCF_ALT            == 3 */
    {0, 0, 0, 0, 1, 1, 1, 1},   /* KCF_CONTROL                  == 4 */
    {0, 1, 0, 1, 2, 3, 2, 3},   /* KCF_SHIFT|KCF_CONTROL        == 5 */
    {0, 0, 1, 1, 2, 2, 3, 3},   /* KCF_ALT|KCF_CONTROL          == 6 */
    {0, 1, 2, 3, 4, 5, 6, 7}    /* KCF_SHIFT|KCF_ALT|KCF_CONTROL == KC__VANILLA == 7 */
};

// TODO: is this order always the same?
static const BYTE deadkey_table[] = " ´`^~\"°";


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


static void parse_normal_key(struct Key *key, UBYTE type, IPTR value)
{
    switch (type)
    {
        case KC_NOQUAL:
            (*key).alone[0]     = value & 0xff;
            break;
        case KCF_SHIFT:
            (*key).alone[0]     = value & 0xff;
            (*key).shift[0]     = (value >> 8) & 0xff;
            break;
        case KCF_ALT:
            (*key).alone[0]     = value & 0xff;
            (*key).alt[0]       = (value >> 8) & 0xff;
            break;
        case KCF_CONTROL:
            (*key).alone[0]     = value & 0xff;
            (*key).ctrl[0]      = (value >> 8) & 0xff;
            break;
        case KCF_ALT + KCF_SHIFT:
            (*key).alone[0]     = value & 0xff;
            (*key).shift[0]     = (value >> 8) & 0xff;
            (*key).alt[0]       = (value >> 16) & 0xff;
            (*key).shift_alt[0] = (value >> 24) & 0xff;
            break;
        case KCF_CONTROL + KCF_ALT:
            (*key).alone[0]     = value & 0xff;
            (*key).alt[0]       = (value >> 8) & 0xff;
            (*key).ctrl[0]      = (value >> 16) & 0xff;
            (*key).ctrl_alt[0]  = (value >> 24) & 0xff;
            break;
        case KCF_CONTROL + KCF_SHIFT:
            (*key).alone[0]     = value & 0xff;
            (*key).shift[0]     = (value >> 8) & 0xff;
            (*key).ctrl[0]      = (value >> 16) & 0xff;
            (*key).ctrl_shift[0]= (value >> 24) & 0xff;
            break;
        case KC_VANILLA:
            (*key).alone[0]     = value & 0xff;
            (*key).shift[0]     = (value >> 8) & 0xff;
            (*key).alt[0]       = (value >> 16) & 0xff;
            (*key).shift_alt[0] = (value >> 24) & 0xff;
            (*key).ctrl[0]      = '^';
            (*key).ctrl[1]      = value & 0xff;
            break;
    }
}


static UBYTE set_string_key(UBYTE type, IPTR value, BYTE qual)
{
    D(bug("[KeyShow] set_string_key type %u value %u\n", type, value));

    BYTE idx;

    D(bug("[KeyShow] getting idx at [%d][%d]\n", type, qual));
    idx = keymapstr_table[type][qual];

    if (idx != -1)
    {
        UBYTE *str_descrs = (UBYTE *)value;
        UBYTE len, offset;

        /* Since each string descriptor uses two bytes we multiply by 2 */
        idx *= 2;

        /* Get string info from string descriptor table */
        len    = str_descrs[idx];
        offset = str_descrs[idx + 1];

        D(bug("[KeyShow] len=%d, offset=%d\n", len, offset));

        /* Return char only if len is 1 */
        if (len == 1 && str_descrs[offset] > 31)
        {
            D(bug("[KeyShow] retval %d", str_descrs[offset]));
            return str_descrs[offset];
        }
    } /* if (idx != -1) */

    return 0;
}


static void parse_string_key(struct Key *key, UBYTE type, IPTR value)
{
    D(bug("[KeyShow] parse_string_key type %u value %u\n", type, value));

    switch (type)
    {
        case KC_NOQUAL:
            (*key).alone[0]     = set_string_key(type, value, KC_NOQUAL);
            break;
        case KCF_SHIFT:
            (*key).alone[0]     = set_string_key(type, value, KC_NOQUAL);
            (*key).shift[0]     = set_string_key(type, value, KCF_SHIFT);
            break;
        case KCF_ALT:
            (*key).alone[0]     = set_string_key(type, value, KC_NOQUAL);
            (*key).alt[0]       = set_string_key(type, value, KCF_ALT);
            break;
        case KCF_CONTROL:
            (*key).alone[0]     = set_string_key(type, value, KC_NOQUAL);
            (*key).ctrl[0]      = set_string_key(type, value, KCF_CONTROL);
            break;
        case KCF_ALT + KCF_SHIFT:
            (*key).alone[0]     = set_string_key(type, value, KC_NOQUAL);
            (*key).shift[0]     = set_string_key(type, value, KCF_SHIFT);
            (*key).alt[0]       = set_string_key(type, value, KCF_ALT);
            (*key).shift_alt[0] = set_string_key(type, value, KCF_ALT + KCF_SHIFT);
            break;
        case KCF_CONTROL + KCF_ALT:
            (*key).alone[0]     = set_string_key(type, value, KC_NOQUAL);
            (*key).alt[0]       = set_string_key(type, value, KCF_ALT);
            (*key).ctrl[0]      = set_string_key(type, value, KCF_CONTROL);
            (*key).ctrl_alt[0]  = set_string_key(type, value, KCF_CONTROL + KCF_ALT);
            break;
        case KCF_CONTROL + KCF_SHIFT:
            (*key).alone[0]     = set_string_key(type, value, KC_NOQUAL);
            (*key).shift[0]     = set_string_key(type, value, KCF_SHIFT);
            (*key).ctrl[0]      = set_string_key(type, value, KCF_CONTROL);
            (*key).ctrl_shift[0]= set_string_key(type, value, KCF_CONTROL + KCF_SHIFT);
            break;
        case KC_VANILLA:
            (*key).alone[0]     = set_string_key(type, value, KC_NOQUAL);
            (*key).shift[0]     = set_string_key(type, value, KCF_SHIFT);
            (*key).alt[0]       = set_string_key(type, value, KCF_ALT);
            (*key).shift_alt[0] = set_string_key(type, value, KCF_ALT + KCF_SHIFT);
            (*key).ctrl[0]      = set_string_key(type, value, KCF_CONTROL);
            (*key).ctrl_alt[0]  = set_string_key(type, value, KCF_CONTROL + KCF_ALT);
            (*key).ctrl_shift[0]= set_string_key(type, value, KCF_CONTROL + KCF_SHIFT);
            break;
    }
}


static void set_dead_key(STRPTR buffer, UBYTE type, IPTR value, BYTE qual)
{
    D(bug("[KeyShow] set_dead_key type %u value %u\n", type, value));

    BYTE idx;
    UBYTE result;

    /* Use keymap_str table to get idx to right key descriptor */
    idx = keymapstr_table[type & KC_VANILLA][qual];
    if (idx != -1)
    {
        UBYTE *dead_descr = (UBYTE *)value;

        switch (dead_descr[idx * 2])
        {
            case DPF_DEAD:
                // dead key
                D(bug("[KeyShow] set_dead_key DPF_DEAD\n"));
                UBYTE deadidx = dead_descr[idx * 2 + 1];
                if (deadidx < 8)
                {
                    result = deadkey_table[deadidx];
                    if (result > 31)
                    {
                        // render char. with pen number 5
                        snprintf(buffer, KBUFSIZE - 1, "\0335%c", result);
                    }
                }
                break;

            case DPF_MOD:
                // deadable key
                D(bug("[KeyShow] set_dead_key DPF_MOD\n"));
                result = dead_descr[dead_descr[idx * 2 + 1]];
                if (result > 31)
                {
                    // render char. in bold
                    snprintf(buffer, KBUFSIZE - 1, "\033b%c", result);
                }
                break;

            case 0:
                D(bug("[KeyShow] set_dead_key DPF 0\n"));
                result = dead_descr[idx * 2 + 1];
                if (result > 31)
                {
                    buffer[0] = result;
                }
                break;

        }
    }
}


static void parse_dead_key(struct Key *key, UBYTE type, IPTR value)
{
    D(bug("[KeyShow] parse_dead_key key %p type %u value %u\n", key, type, value));

    switch (type)
    {
        case KC_NOQUAL:
            set_dead_key((*key).alone, type, value, KC_NOQUAL);
            break;
        case KCF_SHIFT:
            set_dead_key((*key).alone, type, value, KC_NOQUAL);
            set_dead_key((*key).shift, type, value, KCF_SHIFT);
            break;
        case KCF_ALT:
            set_dead_key((*key).alone, type, value, KC_NOQUAL);
            set_dead_key((*key).alt, type, value, KCF_ALT);
            break;
        case KCF_CONTROL:
            set_dead_key((*key).alone, type, value, KC_NOQUAL);
            set_dead_key((*key).ctrl, type, value, KCF_CONTROL);
            break;
        case KCF_ALT + KCF_SHIFT:
            set_dead_key((*key).alone, type, value, KC_NOQUAL);
            set_dead_key((*key).shift, type, value, KCF_SHIFT);
            set_dead_key((*key).alt, type, value, KCF_ALT);
            set_dead_key((*key).shift_alt, type, value, KCF_ALT + KCF_SHIFT);
            break;
        case KCF_CONTROL + KCF_ALT:
            set_dead_key((*key).alone, type, value, KC_NOQUAL);
            set_dead_key((*key).alt, type, value, KCF_ALT);
            set_dead_key((*key).ctrl, type, value, KCF_CONTROL);
            set_dead_key((*key).ctrl_alt, type, value, KCF_CONTROL + KCF_ALT);
            break;
        case KCF_CONTROL + KCF_SHIFT:
            set_dead_key((*key).alone, type, value, KC_NOQUAL);
            set_dead_key((*key).shift, type, value, KCF_SHIFT);
            set_dead_key((*key).ctrl, type, value, KCF_CONTROL);
            set_dead_key((*key).ctrl_shift, type, value, KCF_CONTROL + KCF_SHIFT);
            break;
        case KC_VANILLA:
            set_dead_key((*key).alone, type, value, KC_NOQUAL);
            set_dead_key((*key).shift, type, value, KCF_SHIFT);
            set_dead_key((*key).alt, type, value, KCF_ALT);
            set_dead_key((*key).shift_alt, type, value, KCF_ALT + KCF_SHIFT);
            set_dead_key((*key).ctrl, type, value, KCF_CONTROL);
            set_dead_key((*key).ctrl_alt, type, value, KCF_CONTROL + KCF_ALT);
            set_dead_key((*key).ctrl_shift, type, value, KCF_CONTROL + KCF_SHIFT);
            break;
    }
}


static struct Key *read_keymap(void)
{
    struct Key *key = AllocVec(sizeof(struct Key) * 128, MEMF_CLEAR);
    if (key)
    {
        struct KeyMap *km = AskKeyMapDefault();
        LONG i;
        IPTR value;
        UBYTE type;
        for (i = 0; i < 128; i++)
        {
            if (i < 64)
            {
                type = km->km_LoKeyMapTypes[i];
                value = km->km_LoKeyMap[i];
            }
            else
            {
                type = km->km_HiKeyMapTypes[i-64];
                value = km->km_HiKeyMap[i-64];
            }

            if (type & KCF_STRING)
            {
                parse_string_key(&key[i], type & KC_VANILLA, value);
            }
            else if (type & KCF_DEAD)
            {
                parse_dead_key(&key[i], type & KC_VANILLA, value);
            }
            else
            {
                parse_normal_key(&key[i], type, value);
            }
        }
        // Qualifier keys
        set_immutable_key(key, RAWKEY_LSHIFT, _(MSG_KEY_SHIFT));
        set_immutable_key(key, RAWKEY_RSHIFT, _(MSG_KEY_SHIFT));
        set_immutable_key(key, RAWKEY_LCONTROL, _(MSG_KEY_CTRL));
        set_immutable_key(key, RAWKEY_LALT, _(MSG_KEY_ALT));
        set_immutable_key(key, RAWKEY_RALT, _(MSG_KEY_ALT));
        
        // Special keys
        set_immutable_key(key, RAWKEY_UP, "\033I[6:11]");
        set_immutable_key(key, RAWKEY_DOWN, "\033I[6:12]");
        set_immutable_key(key, RAWKEY_RIGHT, "\033I[6:14]");
        set_immutable_key(key, RAWKEY_LEFT, "\033I[6:13]");


        set_immutable_key(key, RAWKEY_CAPSLOCK, _(MSG_KEY_LOCK));
        set_immutable_key(key, RAWKEY_BACKSPACE,  _(MSG_KEY_BACKSP));
        set_immutable_key(key, RAWKEY_TAB,  _(MSG_KEY_TAB));
        set_immutable_key(key, RAWKEY_RETURN,  _(MSG_KEY_RETURN));
        set_immutable_key(key, RAWKEY_ESCAPE, _(MSG_KEY_ESC));

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

        set_immutable_key(key, RAWKEY_PRTSCREEN, _(MSG_KEY_PRTSCREEN));
        set_immutable_key(key, RAWKEY_SCRLOCK, _(MSG_KEY_SCRLOCK));
        set_immutable_key(key, RAWKEY_PAUSE, _(MSG_KEY_PAUSE));

        set_immutable_key(key, RAWKEY_KP_ENTER, _(MSG_KEY_NUM_ENTER));
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
    ULONG kbtype = MUIV_KeyboardGroup_Type_Amiga;

    struct TagItem  *tstate = message->ops_AttrList;
    struct TagItem  *tag    = NULL;

    while ((tag = NextTagItem(&tstate)) != NULL)
    {
        switch (tag->ti_Tag)
        {
            case MUIA_KeyboardGroup_Type:
                kbtype = tag->ti_Data;
                break;
        }
    }

    for (i = 0; i < 128; i++)
    {
        if ((i < 96 || i > 101 || i == RAWKEY_CAPSLOCK) && i != 127)
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

    if (kbtype == MUIV_KeyboardGroup_Type_PC105)
    {
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
                Child, HGroup,
                    GroupFrame,
                    //MUIA_Group_SameSize, TRUE,
                    KEY(RAWKEY_PRTSCREEN), KEY(RAWKEY_SCRLOCK), KEY(RAWKEY_PAUSE),
                End,
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
                    // rawkey codes refer to a matrix position and can
                    // differ from their real meaning
                    KEY(RAWKEY_KP_LBRACKET), KEY(RAWKEY_KP_RBRACKET), KEY(RAWKEY_KP_DIVIDE), KEY(RAWKEY_KP_MULTIPLY),
                    KEY(RAWKEY_KP_7), KEY(RAWKEY_KP_8), KEY(RAWKEY_KP_9), KEY(RAWKEY_KP_PLUS),
                    KEY(RAWKEY_KP_4), KEY(RAWKEY_KP_5), KEY(RAWKEY_KP_6), Child, HVSpace,
                    KEY(RAWKEY_KP_1), KEY(RAWKEY_KP_2), KEY(RAWKEY_KP_3), KEY(RAWKEY_KP_ENTER),
                    KEY(RAWKEY_KP_0), Child, HVSpace, KEY(RAWKEY_KP_DECIMAL), Child, HVSpace,
                End,
            End,
            TAG_MORE, (IPTR)message->ops_AttrList
        );
    }
    else if (kbtype == MUIV_KeyboardGroup_Type_PC104)
    {
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
                        KEY(0), KEY(1), KEY(2), KEY(3), KEY(4), KEY(5), KEY(6), KEY(7), KEY(8), KEY(9), KEY(10), KEY(11), KEY(12),
                    End,
                    KEY(RAWKEY_BACKSPACE),
                End,
                Child, HGroup,
                    KEY(RAWKEY_TAB),
                    Child, HGroup,
                        MUIA_Group_SameSize, TRUE,
                        KEY(16), KEY(17), KEY(18), KEY(19), KEY(20), KEY(21), KEY(22), KEY(23), KEY(24), KEY(25), KEY(26), KEY(27),
                    End,
                    KEY(13), // FIXME: is this correct?
                End,
                Child, HGroup,
                    KEY(RAWKEY_CAPSLOCK),
                    Child, HGroup,
                        MUIA_Group_SameSize, TRUE,
                        KEY(32), KEY(33), KEY(34), KEY(35), KEY(36), KEY(37), KEY(38), KEY(39), KEY(40), KEY(41), KEY(42),
                    End,
                    KEY(RAWKEY_RETURN),
                End,
                Child, HGroup,
                    KEY(RAWKEY_LSHIFT),
                    Child, HGroup,
                        MUIA_Group_SameSize, TRUE,
                        KEY(49), KEY(50), KEY(51), KEY(52), KEY(53), KEY(54), KEY(55), KEY(56), KEY(57), KEY(58),
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
                Child, HGroup,
                    GroupFrame,
                    //MUIA_Group_SameSize, TRUE,
                    KEY(RAWKEY_PRTSCREEN), KEY(RAWKEY_SCRLOCK), KEY(RAWKEY_PAUSE),
                End,
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
                    // rawkey codes refer to a matrix position and can
                    // differ from their real meaning
                    KEY(RAWKEY_KP_LBRACKET), KEY(RAWKEY_KP_RBRACKET), KEY(RAWKEY_KP_DIVIDE), KEY(RAWKEY_KP_MULTIPLY),
                    KEY(RAWKEY_KP_7), KEY(RAWKEY_KP_8), KEY(RAWKEY_KP_9), KEY(RAWKEY_KP_PLUS),
                    KEY(RAWKEY_KP_4), KEY(RAWKEY_KP_5), KEY(RAWKEY_KP_6), Child, HVSpace,
                    KEY(RAWKEY_KP_1), KEY(RAWKEY_KP_2), KEY(RAWKEY_KP_3), KEY(RAWKEY_KP_ENTER),
                    KEY(RAWKEY_KP_0), Child, HVSpace, KEY(RAWKEY_KP_DECIMAL), Child, HVSpace,
                End,
            End,
            TAG_MORE, (IPTR)message->ops_AttrList
        );
    }
    else
    {
        self = (Object *) DoSuperNewTags
        (
            CLASS, self, NULL,
            MUIA_Group_Horiz, TRUE,
            Child, VGroup,
                GroupFrame,
                Child, HGroup,
                    //MUIA_Group_SameSize, TRUE,
                    KEY(RAWKEY_ESCAPE), KEY(RAWKEY_F1), KEY(RAWKEY_F2), KEY(RAWKEY_F3), KEY(RAWKEY_F4), KEY(RAWKEY_F5),
                    KEY(RAWKEY_F6), KEY(RAWKEY_F7), KEY(RAWKEY_F8), KEY(RAWKEY_F9), KEY(RAWKEY_F10),
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
                    KEY(RAWKEY_LCONTROL),
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
                        KEY(RAWKEY_LALT), KEY(RAWKEY_LAMIGA),
                    End,
                    KEY(RAWKEY_SPACE),
                    Child, HGroup,
                        MUIA_Group_SameSize, TRUE,
                        KEY(RAWKEY_RAMIGA), KEY(RAWKEY_RALT),
                    End,
                End,
            End,
            Child, VGroup,
                Child, HGroup,
                    GroupFrame,
                    //MUIA_Group_SameSize, TRUE,
                    KEY(RAWKEY_DELETE), KEY(RAWKEY_HELP),
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
                    KEY(RAWKEY_KP_LBRACKET), KEY(RAWKEY_KP_RBRACKET), KEY(RAWKEY_KP_DIVIDE), KEY(RAWKEY_KP_MULTIPLY),
                    KEY(RAWKEY_KP_7), KEY(RAWKEY_KP_8), KEY(RAWKEY_KP_9), KEY(RAWKEY_KP_MINUS),
                    KEY(RAWKEY_KP_4), KEY(RAWKEY_KP_5), KEY(RAWKEY_KP_6), KEY(RAWKEY_KP_PLUS),
                    KEY(RAWKEY_KP_1), KEY(RAWKEY_KP_2), KEY(RAWKEY_KP_3), KEY(RAWKEY_KP_ENTER),
                    KEY(RAWKEY_KP_0), Child, HVSpace, KEY(RAWKEY_KP_DECIMAL), Child, HVSpace,
                End,
            End,
            TAG_MORE, (IPTR)message->ops_AttrList
        );
    }

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
                D(bug("[KeyShow/HandleInput] Rawkey %d\n", msg->imsg->Code));
                if (msg->imsg->Code > 95 && msg->imsg->Code < 102 && msg->imsg->Code != RAWKEY_CAPSLOCK)      // Qualifier key
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
