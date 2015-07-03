/*
    Copyright 2015, The AROS Development Team. All rights reserved.
    $Id$
*/

#define DEBUG 1
#include <aros/debug.h>

#include <proto/exec.h>

#include <exec/memory.h>
#include <aros/symbolsets.h>

#include <string.h>

#include "storage_intern.h"

//#include LC_LIBDEFS_FILE

int FindFirstDigit(char *inString)
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

struct Storage_IDFamily *FindIDFamily(char *ID, struct StorageBase_intern *StorageBase)
{
    struct Storage_IDFamily *_IDFamiily  = NULL;
    int firstDigit;

    firstDigit = FindFirstDigit(ID);

    ForeachNode(&StorageBase->libb_NameSpaceFamilies, _IDFamiily)
    {
        if (firstDigit == -1)
        {
            if ((strcmp(_IDFamiily->SIDF_Node.ln_Name, ID)) == 0)
                return _IDFamiily;
        }
        else
        {
            if ((strlen(_IDFamiily->SIDF_Node.ln_Name) == firstDigit) && ((strncmp(_IDFamiily->SIDF_Node.ln_Name, ID, firstDigit)) == 0))
                return _IDFamiily;
        }
    }
    return NULL;
};

/*****************************************************************************

    NAME */

        AROS_LH1(struct Storage_IDNode *, AllocateID,

/*  SYNOPSIS */
	AROS_LHA(char *, ID, A0),

/*  LOCATION */
	APTR *, StorageBase, 10, Storage)

/*  FUNCTION
	Allocate a unit/volume ID.

    INPUTS
    	ID - base ID to allocate. If no unit is specified (e.g. ID = "CD"), a new
    	ID for that family will be allocated (e.g "CD0", "CD1", etc). If an ID unit _is_
    	specified, and it is allready allocated - a new ID will be created (e.g. "CD0_1")

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct Storage_IDFamily *_IDFamiily  = NULL;
    struct Storage_IDNode   *_IDNode = NULL;
    int firstDigit;
    ULONG freeValue = 0;

    D(bug("[StorageRes] %s('%s')\n", __PRETTY_FUNCTION__, ID));

    if ((_IDFamiily = FindIDFamily(ID, (struct StorageBase_intern *)StorageBase)) == NULL)
    {
        if ((_IDFamiily = AllocVec(sizeof(struct Storage_IDFamily), MEMF_CLEAR|MEMF_PUBLIC)) != NULL)
        {
            int baseNameLength = strlen(ID);

            NEWLIST(&_IDFamiily->SIDF_IDs);
            _IDFamiily->SIDF_Node.ln_Name = AllocVec(baseNameLength + 1, MEMF_CLEAR|MEMF_PUBLIC);
            CopyMem(ID, _IDFamiily->SIDF_Node.ln_Name, baseNameLength);
            AddTail(&((struct StorageBase_intern *)StorageBase)->libb_NameSpaceFamilies, &_IDFamiily->SDNF_Node);
            D(bug("[StorageRes] AllocateID: New FamilyNode @ 0x%p for '%s'\n", _IDFamiily, _IDFamiily->SIDF_Node.ln_Name));
        }
        else
        {
            D(bug("[StorageRes] AllocateID: ERROR - No Mem to allocate FamilyNode\n"));
            return NULL;
        }
    }

    D(bug("[StorageRes] AllocateID: using FamilyNode @ 0x%p (family '%s')\n", _IDFamiily, _IDFamiily->SIDF_Node.ln_Name));

    #warning "TODO: We should use locking when manipulating the name list !"

    ForeachNode(&_IDFamiily->SIDF_IDs, _IDNode)
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
    D(bug("[StorageRes] AllocateID: Free Device ID : %d\n", freeValue));

    _IDNode = NULL;

    if ((_IDNode = AllocVec(sizeof(struct Storage_IDNode), MEMF_CLEAR|MEMF_PUBLIC)) != NULL)
    {
        int baseNameLength = strlen(ID);
        int freeValue_len = 1;
        if (freeValue >= 10) freeValue_len ++;
        if (freeValue >= 100) freeValue_len ++;

        _IDNode->SIDN_Node.ln_Name = AllocVec(baseNameLength + freeValue_len + 2, MEMF_CLEAR|MEMF_PUBLIC);

        D(bug("[StorageRes] AllocateID: Name Node Allocated @ 0x%p, Name Storage @ 0x%p [%d bytes]\n", _IDNode, _IDNode->SIDN_Node.ln_Name, (baseNameLength + freeValue_len + 2)));

        CopyMem(_IDFamiily->SIDF_Node.ln_Name, _IDNode->SIDN_Node.ln_Name, baseNameLength);
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

        Enqueue(&_IDFamiily->SIDF_IDs, &_IDNode->SDNN_Node);
        D(bug("[StorageRes] AllocateID: Node '%s' Done\n", _IDNode->SIDN_Node.ln_Name));

        return _IDNode;
    }

    return NULL;

    AROS_LIBFUNC_EXIT
} /* AllocateID */


AROS_LH1(BOOL, FreeID,
	AROS_LHA(struct Storage_IDNode *, _IDNode, A0),
	APTR *, StorageBase, 11, Storage)

/*  FUNCTION
	Free a device/volume ID.

    INPUTS
    	ID Node.

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct Storage_IDFamily *_IDFamiily  = NULL;

    D(bug("[StorageRes]  %s('%s')\n", __PRETTY_FUNCTION__, _IDNode->SIDN_Node.ln_Name));

    return TRUE;

    AROS_LIBFUNC_EXIT
} /* FreeID */
