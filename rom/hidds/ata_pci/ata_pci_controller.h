#ifndef ATA_PCI_CONTROLLER_H
#define ATA_PCI_CONTROLLER_H

#include <exec/types.h>
#include <oop/oop.h>
/*
 * A single PCI device is shared between two buses.
 * The driver is designed as unloadable, so our bus objects can be
 * destroyed. We need to release the device only when both objects
 * are disposed, so we maintain this structure with reference
 * counter.
 * It raises a question if our PCI subsystem needs to support this.
 * However, we'll wait until more use cases pop up.
 */
struct PCIDeviceRef
{
    OOP_Object                  *ref_Device;
    ULONG                       ref_Count;
};


struct PCIATACtrllrData
{
    struct PCIDeviceRef         ctrllrDevice;
};

void DeviceFree(struct PCIDeviceRef *ref, struct atapciBase *base);
void DeviceUnref(struct PCIDeviceRef *ref, struct atapciBase *base);

#endif
