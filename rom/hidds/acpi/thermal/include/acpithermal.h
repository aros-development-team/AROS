#ifndef HIDD_ACPITHERMAL_H
#define HIDD_ACPITHERMAL_H

#include <interface/HW_ACPIThermal.h>

#define CLID_HW_ACPIThermal "hw.acpi.thermal"

#define IS_HWACPITHERMAL_ATTR(attr, idx) IS_IF_ATTR(attr, idx, HWACPIThermalAB, num_HW_ACPIThermal_Attrs)

#endif /* !HIDD_ACPITHERMAL_H */
