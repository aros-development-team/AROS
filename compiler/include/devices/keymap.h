#ifndef DEVICES_KEYMAP_H
#define DEVICES_KEYMAP_H

/*
    Copyright © 1995-2020, The AROS Development Team. All rights reserved.
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

#if (__WORDSIZE == 64)
struct KeyMapPacked
{
    CONST ULONG km_LoKeyMapTypes;
    CONST ULONG km_LoKeyMap;
    CONST ULONG km_LoCapsable;
    CONST ULONG km_LoRepeatable;
    CONST ULONG km_HiKeyMapTypes;
    CONST ULONG km_HiKeyMap;
    CONST ULONG km_HiCapsable;
    CONST ULONG km_HiRepeatable;
} __packed;
#endif

struct KeyMapNode
{
    struct Node   kn_Node;
    struct KeyMap kn_KeyMap;
};

#if (__WORDSIZE == 64)
struct KeyMapNodePacked
{
    struct
    {
        ULONG   ln_Succ;
        ULONG   ln_Pred;
        UBYTE   ln_Type;
        BYTE    ln_Pri;
        ULONG   ln_Name;
    } kn_Node;
    struct KeyMapPacked kn_KeyMap;
} __packed;
#endif

#define KCB_SHIFT       0
#define KCF_SHIFT       (1 << KCB_SHIFT)
#define KCB_ALT         1
#define KCF_ALT         (1 << KCB_ALT)
#define KCB_CONTROL     2
#define KCF_CONTROL     (1 << KCB_CONTROL)
#define KCB_DOWNUP      3
#define KCF_DOWNUP      (1 << KCB_DOWNUP)
#define KCB_DEAD        5
#define KCF_DEAD        (1 << KCB_DEAD)
#define KCB_STRING      6
#define KCF_STRING      (1 << KCB_STRING)
#define KCB_NOP         7
#define KCF_NOP         (1 << KCB_NOP)

#define KC_NOQUAL       0
#define KC_VANILLA      (KCF_CONTROL | KCF_ALT | KCF_SHIFT)

#define DPB_MOD         0
#define DPF_MOD         (1 << DPB_MOD)
#define DPB_DEAD        3
#define DPF_DEAD        (1 << DPB_DEAD)

#define DP_2DINDEXMASK  0x0f
#define DP_2DFACSHIFT   4

#endif /* DEVICES_KEYMAP_H */
