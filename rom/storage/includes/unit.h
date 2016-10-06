#if !defined(_STORAGE_UNIT_H)
#define _STORAGE_UNIT_H

/*
    Copyright © 1995-2016, The AROS Development Team. All rights reserved.
    $Id$
*/

/*
    Definitions relating to storage units (i.e. drives)
*/

struct StorageUnit
{
    struct Node                 su_Node;        // ln_Name points to the IDNode;
    struct StorageDevice        *su_Device;
    struct List                 su_Volumes;
};

#endif
