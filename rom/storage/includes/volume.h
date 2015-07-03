#if !defined(_STORAGE_VOLUME_H)
#define _STORAGE_VOLUME_H

struct StorageVolume
{
    struct Node                 sv_Node;        // ln_Name points to the IDNode;
    struct StorageUnit          *sv_Unit;
};

#endif
