#ifndef DEVICES_KEYMAP_H
#define DEVICES_KEYMAP_H

/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Keymap definitions
    Lang: english
*/

#ifndef EXEC_LISTS_H
#   include <exec/lists.h>
#endif

#ifndef EXEC_NODES_H
#   include <exec/nodes.h>
#endif

struct KeyMapResource
{
    struct Node kr_Node;
    struct List kr_List;
};

struct KeyMap
{
    CONST UBYTE * km_LoKeyMapTypes;
    CONST IPTR  * km_LoKeyMap;
    CONST UBYTE * km_LoCapsable;
    CONST UBYTE * km_LoRepeatable;
    CONST UBYTE * km_HiKeyMapTypes;
    CONST IPTR  * km_HiKeyMap;
    CONST UBYTE * km_HiCapsable;
    CONST UBYTE * km_HiRepeatable;
};

struct KeyMapNode
{
    struct Node   kn_Node;
    struct KeyMap kn_KeyMap;
};

#define KC_NOQUAL   0
#define KC_VANILLA  7
#define KCB_SHIFT       0
#define KCF_SHIFT   (1<<0)
#define KCB_ALT         1
#define KCF_ALT     (1<<1)
#define KCB_CONTROL     2
#define KCF_CONTROL (1<<2)
#define KCB_DOWNUP      3
#define KCF_DOWNUP  (1<<3)
#define KCB_DEAD        5
#define KCF_DEAD    (1<<5)
#define KCB_STRING      6
#define KCF_STRING  (1<<6)
#define KCB_NOP         7
#define KCF_NOP     (1<<7)

#define DPB_MOD      0
#define DPF_MOD  (1<<0)
#define DPB_DEAD     3
#define DPF_DEAD (1<<3)

#define DP_2DINDEXMASK 0x0f
#define DP_2DFACSHIFT 4

#endif /* DEVICES_KEYMAP_H */
