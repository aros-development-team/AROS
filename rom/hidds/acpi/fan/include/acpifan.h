#ifndef HIDD_ACPIFAN_H
#define HIDD_ACPIFAN_H

#include <interface/HW_ACPIFan.h>

#define CLID_HW_ACPIFan "hw.acpi.acpifan"

#define IS_HWACPIFAN_ATTR(attr, idx) IS_IF_ATTR(attr, idx, HWACPIFanAB, num_HW_ACPIFan_Attrs)

#endif /* !HIDD_ACPIFAN_H */
