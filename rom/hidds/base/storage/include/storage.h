#ifndef HIDD_STORAGE_H
#define HIDD_STORAGE_H

#define CLID_Hidd_Storage                       "hw.storage"
#define IID_Hidd_Storage                        CLID_Hidd_Storage

#include <interface/Hidd_Bus.h>
#include <interface/Hidd_Storage.h>
#include <interface/Hidd_StorageController.h>
#include <interface/Hidd_StorageBus.h>
#include <interface/Hidd_StorageUnit.h>

#define CLID_Hidd_StorageController             IID_Hidd_StorageController
#define CLID_Hidd_StorageBus                    IID_Hidd_StorageBus
#define CLID_Hidd_StorageUnit                   IID_Hidd_StorageUnit

/* Unit types */
#define vHidd_StorageUnit_Type_Unknown          0x00
#define vHidd_StorageUnit_Type_RaidArray        0x01
#define vHidd_StorageUnit_Type_FixedDisk        0x02
#define vHidd_StorageUnit_Type_SolidStateDisk   0x03
#define vHidd_StorageUnit_Type_OpticalDisc      0x04
#define vHidd_StorageUnit_Type_MagneticTape     0x05

#endif
