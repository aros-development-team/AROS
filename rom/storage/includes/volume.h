#if !defined(_STORAGE_VOLUME_H)
#define _STORAGE_VOLUME_H

/*
    Copyright © 1995-2016, The AROS Development Team. All rights reserved.
    $Id$
*/

struct StorageVolume
{
    struct Node                 sv_Node;        // ln_Name points to the IDNode;
    struct StorageUnit          *sv_Unit;
};

#endif
