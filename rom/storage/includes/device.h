#if !defined(_STORAGE_DEVICE_H)
#define _STORAGE_DEVICE_H

/*
    Definitions relating to storage devices (i.e controllers)
    */

struct StorageDevice
{
    struct Node                 sd_Node;
    struct List                 sd_Units;
};

#endif
