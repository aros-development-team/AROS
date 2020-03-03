#ifndef MKKEYMAP_H
#define MKKEYMAP_H

/*
    Copyright © 2020, The AROS Development Team. All rights reserved.

    Desc: global include for mkkeymap. Defines global variables and
          the function prototypes.
*/

#ifndef EXEC_TYPES_H
#   include <exec/types.h>
#endif

#include <devices/keymap.h>

#include "config.h"

struct Reloc_Node {
    u_int32_t                   Next;
    u_int32_t                   Offset;
};

struct raw_KeyMap
{
    u_int32_t                   km_LoKeyMapTypes;
    u_int32_t                   km_LoKeyMap;
    u_int32_t                   km_LoCapsable;
    u_int32_t                   km_LoRepeatable;
    u_int32_t                   km_HiKeyMapTypes;
    u_int32_t                   km_HiKeyMap;
    u_int32_t                   km_HiCapsable;
    u_int32_t                   km_HiRepeatable;
};

struct raw_KeyMapNode
{
    struct
    {
        u_int32_t               ln_Succ;
        u_int32_t               ln_Pred;
        u_int8_t                ln_Type;
        int8_t                  ln_Pri;
        u_int32_t               ln_Name;
    } kn_Node;
    struct raw_KeyMap           kn_KeyMap;
};

struct raw_HalfKeyMap {
    u_int32_t                   KeyMapTypes;
    u_int32_t                   KeyMap;
    u_int32_t                   Capsable;
    u_int32_t                   Repeatable;
};

struct KeyMap_Hunk {
    u_int32_t                   Hunk;
    u_int32_t                   Length;
    struct raw_KeyMapNode       kh_KeyMapNode;
    u_int8_t                    kh_LoKeyMapTypes[0x40];
    u_int32_t                   kh_LoKeyMap[0x40];
    u_int8_t                    kh_LoCapsable[0x08];
    u_int8_t                    kh_LoRepeatable[0x08];
    u_int8_t                    kh_HiKeyMapTypes[0x38];
    u_int32_t                   kh_HiKeyMap[0x38];
    u_int8_t                    kh_HiCapsable[0x07];
    u_int8_t                    kh_HiRepeatable[0x07];
};

extern BOOL parseKeyDescriptor(struct config *);
extern BOOL writeKeyMap(struct config *cfg);

#endif
