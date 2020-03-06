/*
    Copyright © 2020, The AROS Development Team. All rights reserved.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <dos/doshunks.h>

#include "mkkeymap.h"
#include "debug.h"

struct KeyMap_Hunk {
	ULONG			Hunk;
	ULONG			Length;
        struct
        {
            struct
            {
                ULONG   ln_Succ;
                ULONG   ln_Pred;
                UBYTE   ln_Type;
                BYTE    ln_Pri;
                ULONG   ln_Name;
            } kn_Node;
            struct
            {
                CONST ULONG km_LoKeyMapTypes;
                CONST ULONG km_LoKeyMap;
                CONST ULONG km_LoCapsable;
                CONST ULONG km_LoRepeatable;
                CONST ULONG km_HiKeyMapTypes;
                CONST ULONG km_HiKeyMap;
                CONST ULONG km_HiCapsable;
                CONST ULONG km_HiRepeatable;
            } kn_KeyMap;
        } kh_KeyMapNode;
	UBYTE			kh_LoKeyMapTypes[0x40];
	ULONG			kh_LoKeyMap[0x40];
	UBYTE			kh_LoCapsable[0x08];
	UBYTE			kh_LoRepeatable[0x08];
	UBYTE			kh_HiKeyMapTypes[0x38];
	ULONG			kh_HiKeyMap[0x38];
	UBYTE			kh_HiCapsable[0x07];
	UBYTE			kh_HiRepeatable[0x07];
} __packed;

BOOL writeKeyMap(struct config *cfg)
{
    struct Node *stNode;
    ULONG stdtsize = 0, reloccnt = 0, i;
    BOOL doverbose = cfg->verbose;
    D(doverbose = TRUE;)

    struct KeyMap_Hunk *hunkData;

    if (doverbose)
        fprintf(stdout, "creating keymap '%s'\n", cfg->keymap);

    for (i = 0; i < 0x40; i ++)
    {
        if ((cfg->LoKeyMapTypes[i] & (KCF_DEAD|KCF_STRING)) != 0)
        {
            stNode = (struct Node *)cfg->LoKeyMap[i];
            fprintf(stdout, "node @ 0x%p\n", stNode);
            stdtsize += (UBYTE)stNode->ln_Pri;
            reloccnt++;
        }
    }

    for (i = 0; i < 0x38; i ++)
    {
        if ((cfg->HiKeyMapTypes[i] & (KCF_DEAD|KCF_STRING)) != 0)
        {
            stNode = (struct Node *)cfg->HiKeyMap[i];
            fprintf(stdout, "node @ 0x%p\n", stNode);
            stdtsize += (UBYTE)stNode->ln_Pri;
            reloccnt++;
        }
    }

    fprintf(stdout, "allocating %u bytes for raw keymap data (%u bytes string data)\n", sizeof(struct KeyMap_Hunk) + strlen(cfg->keymap) + 1 + stdtsize, stdtsize);
    fprintf(stdout, "creating %u relocations\n", reloccnt + 1);

    hunkData = malloc(sizeof(struct KeyMap_Hunk) + strlen(cfg->keymap) + 1 + stdtsize);

    memcpy(&hunkData->kh_LoKeyMapTypes[0], cfg->LoKeyMapTypes, 0x40);
    memcpy(&hunkData->kh_LoCapsable[0], cfg->LoCapsable, 0x8);
    memcpy(&hunkData->kh_LoRepeatable[0], cfg->LoRepeatable, 0x8);

    for (i = 0; i < 0x40; i ++)
    {
        if ((cfg->LoKeyMapTypes[i] & (KCF_DEAD|KCF_STRING)) == 0)
        {
            hunkData->kh_LoKeyMap[i] = (ULONG)cfg->LoKeyMap[i];
        }
    }

    memcpy(&hunkData->kh_HiKeyMapTypes[0], cfg->HiKeyMapTypes, 0x38);
    memcpy(&hunkData->kh_HiCapsable[0], cfg->HiCapsable, 0x7);
    memcpy(&hunkData->kh_HiRepeatable[0], cfg->HiRepeatable, 0x7);

    for (i = 0; i < 0x38; i ++)
    {
        if ((cfg->HiKeyMapTypes[i] & (KCF_DEAD|KCF_STRING)) == 0)
        {
            hunkData->kh_HiKeyMap[i] = (ULONG)cfg->HiKeyMap[i];
        }
    }
}
