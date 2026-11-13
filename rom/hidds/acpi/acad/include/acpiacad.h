#ifndef HIDD_ACPIACAD_H
#define HIDD_ACPIACAD_H

/* ACPI ACAd interface */
#include <interface/HW_ACPIACAd.h>

#define CLID_HW_ACPIACAd "hw.acpi.acad"

#define IS_HWACPIACAD_ATTR(attr, idx) IS_IF_ATTR(attr, idx, HWACPIACAdAB, num_HW_ACPIACAd_Attrs)

#endif /* !HIDD_ACPIACAD_H */
