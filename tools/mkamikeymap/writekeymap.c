/*
    Copyright © 2020, The AROS Development Team. All rights reserved.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <libgen.h>

#include <dos/doshunks.h>
#include <arpa/inet.h>

#include "mkamikeymap.h"
#include "debug.h"

struct AmiKMap_Node {
    ULONG                       ln_Succ;
    ULONG                       ln_Pred;
    UBYTE                       ln_Type;
    BYTE                        ln_Pri;
    ULONG                       ln_Name;
} __packed;

struct AmiKMap_KeyMap {
    ULONG                       km_LoKeyMapTypes;
    ULONG                       km_LoKeyMap;
    ULONG                       km_LoCapsable;
    ULONG                       km_LoRepeatable;
    ULONG                       km_HiKeyMapTypes;
    ULONG                       km_HiKeyMap;
    ULONG                       km_HiCapsable;
    ULONG                       km_HiRepeatable;
} __packed;

struct AmiKMap_KeyMapNode {
    struct AmiKMap_Node         kn_Node;
    struct AmiKMap_KeyMap       kn_KeyMap;
} __packed;

struct AmiKMap_Hunk {
    ULONG			Hunk;
    ULONG			Length;
    struct AmiKMap_KeyMapNode   kh_KeyMapNode;
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
    FILE *out;
    struct Node *stNode;
    ULONG stdtsize = 0, hunksize, reloccnt = 0, reloctmp;
    BOOL doverbose = cfg->verbose;
    D(doverbose = TRUE;)
    ULONG tmp;
    int r = 0, i;

    char *kmname = basename(cfg->keymap);
    struct AmiKMap_Hunk *hunkRaw;
    ULONG *relocData;
    APTR hunkData;

    if (doverbose)
        fprintf(stdout, "creating keymap '%s' ('%s')\n", cfg->keymap, kmname);

    reloctmp = 0;
    for (i = 0; i < 0x40; i ++)
    {
        if ((cfg->LoKeyMapTypes[i] & (KCF_DEAD|KCF_STRING)) != 0)
        {
            stNode = (struct Node *)cfg->LoKeyMap[i];
            D(fprintf(stdout, "node @ 0x%p\n", stNode);)
            stdtsize += (UBYTE)stNode->ln_Pri;
            reloctmp++;
        }
    }

    reloccnt = reloctmp;
    D(fprintf(stdout, "%u lokey relocations\n", reloccnt);)

    reloctmp = 0;
    for (i = 0; i < 0x38; i ++)
    {
        if ((cfg->HiKeyMapTypes[i] & (KCF_DEAD|KCF_STRING)) != 0)
        {
            stNode = (struct Node *)cfg->HiKeyMap[i];
            D(fprintf(stdout, "node @ 0x%p\n", stNode);)
            stdtsize += (UBYTE)stNode->ln_Pri;
            reloctmp++;
        }
    }

    reloccnt += reloctmp;
    D(fprintf(stdout, "%u hikey relocations\n", reloctmp);)

    if (doverbose)
    {
        fprintf(stdout, "allocating %lu bytes for raw keymap data (%u bytes string data)\n", sizeof(struct AmiKMap_Hunk) + strlen(kmname) + 1 + stdtsize, stdtsize);
        fprintf(stdout, "creating %u relocations\n", reloccnt + 9);
    }

    hunksize = (sizeof(struct AmiKMap_Hunk) + stdtsize + strlen(kmname) + 1 + 0x3) & ~0x3;

    hunkRaw = malloc(hunksize);
    hunkRaw->Hunk = htonl(HUNK_CODE);
    hunkRaw->Length = htonl((hunksize >> 2) - 2);

    memcpy(&hunkRaw->kh_LoKeyMapTypes[0], cfg->LoKeyMapTypes, 0x40);
    memcpy(&hunkRaw->kh_LoCapsable[0], cfg->LoCapsable, 0x8);
    memcpy(&hunkRaw->kh_LoRepeatable[0], cfg->LoRepeatable, 0x8);

    relocData = malloc((reloccnt + 12) << 2);
    relocData[r++] = htonl(HUNK_RELOC32);
    relocData[r++] = htonl(reloccnt + 9);
    relocData[r++] = 0;

    hunkData = (APTR)((IPTR)hunkRaw + sizeof(struct AmiKMap_Hunk));
    strcpy(hunkData, kmname);
    hunkRaw->kh_KeyMapNode.kn_Node.ln_Name = htonl(((IPTR)hunkData - (IPTR)&hunkRaw->kh_KeyMapNode.kn_Node));
    relocData[r++] = htonl(((IPTR)&hunkRaw->kh_KeyMapNode.kn_Node.ln_Name - (IPTR)&hunkRaw->kh_KeyMapNode.kn_Node));
    hunkData = (APTR)((IPTR)hunkData + strlen(kmname) + 1);

    hunkRaw->kh_KeyMapNode.kn_KeyMap.km_LoKeyMapTypes = htonl(((IPTR)&hunkRaw->kh_LoKeyMapTypes[0] - (IPTR)&hunkRaw->kh_KeyMapNode.kn_Node));
    relocData[r++] = htonl(((IPTR)&hunkRaw->kh_KeyMapNode.kn_KeyMap.km_LoKeyMapTypes - (IPTR)&hunkRaw->kh_KeyMapNode.kn_Node));
    hunkRaw->kh_KeyMapNode.kn_KeyMap.km_LoKeyMap = htonl(((IPTR)&hunkRaw->kh_LoKeyMap[0] - (IPTR)&hunkRaw->kh_KeyMapNode.kn_Node));
    relocData[r++] = htonl(((IPTR)&hunkRaw->kh_KeyMapNode.kn_KeyMap.km_LoKeyMap - (IPTR)&hunkRaw->kh_KeyMapNode.kn_Node));
    hunkRaw->kh_KeyMapNode.kn_KeyMap.km_LoCapsable = htonl(((IPTR)&hunkRaw->kh_LoCapsable[0] - (IPTR)&hunkRaw->kh_KeyMapNode.kn_Node));
    relocData[r++] = htonl(((IPTR)&hunkRaw->kh_KeyMapNode.kn_KeyMap.km_LoCapsable - (IPTR)&hunkRaw->kh_KeyMapNode.kn_Node));
    hunkRaw->kh_KeyMapNode.kn_KeyMap.km_LoRepeatable = htonl(((IPTR)&hunkRaw->kh_LoRepeatable[0] - (IPTR)&hunkRaw->kh_KeyMapNode.kn_Node));
    relocData[r++] = htonl(((IPTR)&hunkRaw->kh_KeyMapNode.kn_KeyMap.km_LoRepeatable - (IPTR)&hunkRaw->kh_KeyMapNode.kn_Node));
    hunkRaw->kh_KeyMapNode.kn_KeyMap.km_HiKeyMapTypes = htonl(((IPTR)&hunkRaw->kh_HiKeyMapTypes[0] - (IPTR)&hunkRaw->kh_KeyMapNode.kn_Node));
    relocData[r++] = htonl(((IPTR)&hunkRaw->kh_KeyMapNode.kn_KeyMap.km_HiKeyMapTypes - (IPTR)&hunkRaw->kh_KeyMapNode.kn_Node));
    hunkRaw->kh_KeyMapNode.kn_KeyMap.km_HiKeyMap = htonl(((IPTR)&hunkRaw->kh_HiKeyMap[0] - (IPTR)&hunkRaw->kh_KeyMapNode.kn_Node));
    relocData[r++] = htonl(((IPTR)&hunkRaw->kh_KeyMapNode.kn_KeyMap.km_HiKeyMap - (IPTR)&hunkRaw->kh_KeyMapNode.kn_Node));
    hunkRaw->kh_KeyMapNode.kn_KeyMap.km_HiCapsable = htonl(((IPTR)&hunkRaw->kh_HiCapsable[0] - (IPTR)&hunkRaw->kh_KeyMapNode.kn_Node));
    relocData[r++] = htonl(((IPTR)&hunkRaw->kh_KeyMapNode.kn_KeyMap.km_HiCapsable - (IPTR)&hunkRaw->kh_KeyMapNode.kn_Node));
    hunkRaw->kh_KeyMapNode.kn_KeyMap.km_HiRepeatable = htonl(((IPTR)&hunkRaw->kh_HiRepeatable[0] - (IPTR)&hunkRaw->kh_KeyMapNode.kn_Node));
    relocData[r++] = htonl(((IPTR)&hunkRaw->kh_KeyMapNode.kn_KeyMap.km_HiRepeatable - (IPTR)&hunkRaw->kh_KeyMapNode.kn_Node));

    for (i = 0; i < 0x40; i ++)
    {
        if ((cfg->LoKeyMapTypes[i] & (KCF_DEAD|KCF_STRING)) == 0)
        {
            hunkRaw->kh_LoKeyMap[i] = htonl((ULONG)cfg->LoKeyMap[i]);
        }
        else
        {
            struct Node *knode = (struct Node *)cfg->LoKeyMap[i];
            APTR knodedata = (APTR)((IPTR)knode + sizeof(struct Node) + strlen(knode->ln_Name) + 1);
            hunkRaw->kh_LoKeyMap[i] = htonl(((IPTR)hunkData - (IPTR)&hunkRaw->kh_KeyMapNode.kn_Node));
            memcpy(hunkData, knodedata, knode->ln_Pri);
            relocData[r++] = htonl(((IPTR)&hunkRaw->kh_LoKeyMap[i] - (IPTR)&hunkRaw->kh_KeyMapNode.kn_Node));
            hunkData = (APTR)((IPTR)hunkData + knode->ln_Pri);
        }
    }

    memcpy(&hunkRaw->kh_HiKeyMapTypes[0], cfg->HiKeyMapTypes, 0x38);
    memcpy(&hunkRaw->kh_HiCapsable[0], cfg->HiCapsable, 0x7);
    memcpy(&hunkRaw->kh_HiRepeatable[0], cfg->HiRepeatable, 0x7);

    for (i = 0; i < 0x38; i ++)
    {
        if ((cfg->HiKeyMapTypes[i] & (KCF_DEAD|KCF_STRING)) == 0)
        {
            hunkRaw->kh_HiKeyMap[i] = htonl((ULONG)cfg->HiKeyMap[i]);
        }
        else
        {
            struct Node *knode = (struct Node *)cfg->HiKeyMap[i];
            APTR knodedata = (APTR)((IPTR)knode + sizeof(struct Node) + strlen(knode->ln_Name) + 1);
            hunkRaw->kh_HiKeyMap[i] = htonl(((IPTR)hunkData - (IPTR)&hunkRaw->kh_KeyMapNode.kn_Node));
            memcpy(hunkData, knodedata, knode->ln_Pri);
            relocData[r++] = htonl(((IPTR)&hunkRaw->kh_HiKeyMap[i] - (IPTR)&hunkRaw->kh_KeyMapNode.kn_Node));
            hunkData = (APTR)((IPTR)hunkData + knode->ln_Pri);
        }
    }

    if ((r - 3) != (reloccnt + 9))
    {
        fprintf(stdout, "expected %u relocations - %u performed\n", (reloccnt + 9), (r - 3));
    }

    out = fopen(cfg->keymap, "w");
    if (out==NULL)
    {
        fprintf(stderr, "Could not write %s\n", cfg->keymap);
        exit(20);
    }

    /* write the hunk exe header */
    tmp = htonl(HUNK_HEADER);
    fwrite(&tmp, 4, 1, out);
    tmp = 0;
    fwrite(&tmp, 4, 1, out);
    tmp = htonl(1);
    fwrite(&tmp, 4, 1, out);
    tmp = 0;
    fwrite(&tmp, 4, 1, out);
    tmp = 0;
    fwrite(&tmp, 4, 1, out);
    tmp = htonl((hunksize >> 2) - 2);
    fwrite(&tmp, 4, 1, out);

    if (doverbose)
        fprintf(stdout, "wrote 6x4 (24) bytes hunk exe header\n");

    /* write hunk code */
    fwrite(hunkRaw, hunksize, 1, out);

    if (doverbose)
        fprintf(stdout, "wrote %u bytes hunk code\n", (ntohl(hunkRaw->Length) << 2));

    /* write relocs .. */
    fwrite(relocData, ((reloccnt + 12) << 2), 1, out);
    tmp = 0; // terminate the relocations ...
    fwrite(&tmp, 4, 1, out);

    if (doverbose)
        fprintf(stdout, "wrote %u bytes relocation data\n", ((reloccnt + 12) << 2));

    tmp = htonl(HUNK_END);
    fwrite(&tmp, 4, 1, out);

    fclose(out);
}
