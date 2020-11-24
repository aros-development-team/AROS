/*
    Copyright (C) 2015-2020, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/debug.h>

#include <proto/utility.h>

#include <oop/oop.h>
#include <utility/tagitem.h>

#include "storage_intern.h"

OOP_Object *StorageHW__Root__New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    D(bug ("[Storage] %s()\n", __func__);)
    if (!CSD(cl)->instance)
    {
        struct TagItem new_tags[] =
        {
            {aHW_ClassName, (IPTR)"Storage Controllers"},
            {TAG_DONE     , 0          }
        };
        struct pRoot_New new_msg =
        {
            .mID      = msg->mID,
            .attrList = new_tags
        };

        CSD(cl)->instance =  (OOP_Object *)OOP_DoSuperMethod(cl, o, &new_msg.mID);
    }

    D(bug ("[Storage] %s: Instance @ 0x%p\n", __func__, CSD(cl)->instance);)
    return CSD(cl)->instance;
}

VOID StorageHW__Root__Dispose(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    D(bug ("[Storage] %s(0x%p)\n", __func__, o);)
    /* We are singletone. Cannot dispose. */
}

static int FindFirstDigit(char *inString)
{
    int inStr_len = strlen(inString);
    int i = 0;

    for (i = 0; i < inStr_len; i++)
    {
        if ((inString[i] >= 0x30) && (inString[i] <= 0x39))
            return i;
    }  
    return -1;
};

static struct Storage_IDStem *FindIDStemNode(char *ID, struct List *stemList)
{
    struct Storage_IDStem *_IDStem  = NULL;
    int firstDigit;

    firstDigit = FindFirstDigit(ID);

    ForeachNode(stemList, _IDStem)
    {
        if (firstDigit == -1)
        {
            if ((strcmp(_IDStem->SIDS_Node.ln_Name, ID)) == 0)
                return _IDStem;
        }
        else
        {
            if ((strlen(_IDStem->SIDS_Node.ln_Name) == firstDigit) && ((strncmp(_IDStem->SIDS_Node.ln_Name, ID, firstDigit)) == 0))
                return _IDStem;
        }
    }
    return NULL;
};


APTR StorageHW__Hidd_Storage__AllocateID(OOP_Class *cl, OOP_Object *o, struct pHidd_Storage_AllocateID *msg)
{
    struct Library *UtilityBase = CSD(cl)->cs_UtilityBase;
    char *ID_Stem = (char *)GetTagData(tHidd_Storage_IDStem, 0, msg->IDTags);
    struct Storage_IDStem *_IDStem  = NULL;
    struct Storage_IDNode   *_IDNode = NULL;
    int firstDigit;
    ULONG freeValue = 0;

    D(bug ("[Storage] %s(0x%p)\n", __func__, o);)
    if (ID_Stem)
    {
        if ((_IDStem = FindIDStemNode(ID_Stem, &CSD(cl)->cs_IDs)) == NULL)
        {
            if ((_IDStem = AllocVec(sizeof(struct Storage_IDStem), MEMF_CLEAR|MEMF_PUBLIC)) != NULL)
            {
                int baseNameLength = strlen(ID_Stem);

                NEWLIST(&_IDStem->SIDS_IDs);
                _IDStem->SIDS_Node.ln_Name = AllocVec(baseNameLength + 1, MEMF_CLEAR|MEMF_PUBLIC);
                CopyMem(ID_Stem, _IDStem->SIDS_Node.ln_Name, baseNameLength);
                AddTail(&CSD(cl)->cs_IDs, &_IDStem->SIDS_Node);
                D(bug("[Storage] %s: New StemNode @ 0x%p for '%s'\n", __func__, _IDStem, _IDStem->SIDS_Node.ln_Name));
            }
            else
            {
                D(bug("[Storage] %s: ERROR - No Mem to allocate StemNode\n", __func__));
                return NULL;
            }
        }

        D(bug("[Storage] %s: using StemNode @ 0x%p (stem '%s')\n", __func__, _IDStem, _IDStem->SIDS_Node.ln_Name));

        /* TODO: We should use locking when manipulating the name list ! */

        ForeachNode(&_IDStem->SIDS_IDs, _IDNode)
        {
            ULONG nodeValue = 0;
            int i;

            firstDigit = FindFirstDigit(_IDNode->SIDN_Node.ln_Name);

            for (i = 0; i < (strlen(_IDNode->SIDN_Node.ln_Name) - firstDigit); i++)
            {
                nodeValue = nodeValue * 10;
                nodeValue = nodeValue + (_IDNode->SIDN_Node.ln_Name[firstDigit + i] - 0x30);
            }
            //find the index of the value in the name
            //convert it to a real value
            if (nodeValue >= freeValue)
            {
                freeValue = nodeValue +1;
            }      
        }
        D(bug("[Storage] %s: Free ID# %d\n", __func__, freeValue));

        _IDNode = NULL;

        if ((_IDNode = AllocVec(sizeof(struct Storage_IDNode), MEMF_CLEAR|MEMF_PUBLIC)) != NULL)
        {
            int baseNameLength = strlen(ID_Stem);
            int freeValue_len = 1;
            if (freeValue >= 10) freeValue_len ++;
            if (freeValue >= 100) freeValue_len ++;

            _IDNode->SIDN_Node.ln_Name = AllocVec(baseNameLength + freeValue_len + 2, MEMF_CLEAR|MEMF_PUBLIC);

            D(bug("[Storage] %s: Name Node Allocated @ 0x%p, Name Storage @ 0x%p (%d bytes)\n", __func__, _IDNode, _IDNode->SIDN_Node.ln_Name, (baseNameLength + freeValue_len + 2)));

            CopyMem(_IDStem->SIDS_Node.ln_Name, _IDNode->SIDN_Node.ln_Name, baseNameLength);
            int startnum = freeValue;
            int count = 0;
            if (freeValue >= 100)
            {
                _IDNode->SIDN_Node.ln_Name[baseNameLength + count] = (char)(0x30 + (freeValue / 100));
                freeValue = freeValue - ((freeValue / 100) * 100);
                count ++;
            } 
            if (freeValue >= 10)
            {
                _IDNode->SIDN_Node.ln_Name[baseNameLength + count] = (char)(0x30 + (freeValue / 10));
                freeValue = freeValue - ((freeValue / 10) * 10);
                count ++;
            }
            _IDNode->SIDN_Node.ln_Name[baseNameLength + count] = (char)(0x30 + freeValue);
            freeValue = startnum;
            count ++;
            _IDNode->SIDN_Node.ln_Name[baseNameLength + count] = 0x00;

            Enqueue(&_IDStem->SIDS_IDs, &_IDNode->SIDN_Node);
            D(bug("[Storage] %s: Node '%s' Done\n", __func__, _IDNode->SIDN_Node.ln_Name));

            return _IDNode;
        }
    }
    return NULL;
}

VOID StorageHW__Hidd_Storage__ReleaseID(OOP_Class *cl, OOP_Object *o, struct pHidd_Storage_ReleaseID *msg)
{
    D(bug ("[Storage] %s(0x%p)\n", __func__, o);)
}
