#ifndef HIDD_ACPIBATTERY_H
#define HIDD_ACPIBATTERY_H

/* ACPI Battery interface */
#include <interface/HW_ACPIBattery.h>

#define CLID_HW_ACPIBattery "hw.acpi.battery"

#define IS_HWACPIBATTERY_ATTR(attr, idx) IS_IF_ATTR(attr, idx, HWACPIBatteryAB, num_HW_ACPIBattery_Attrs)

#endif /* !HIDD_ACPIBATTERY_H */
