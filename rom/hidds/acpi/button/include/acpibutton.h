#ifndef HIDD_ACPIBUTTON_H
#define HIDD_ACPIBUTTON_H

/* ACPI Button interface */
#include <interface/HW_ACPIButton.h>

#define CLID_HW_ACPIButton "hw.acpi.button"

#define IS_HWACPIBUTTON_ATTR(attr, idx) IS_IF_ATTR(attr, idx, HWACPIButtonAB, num_HW_ACPIButton_Attrs)

/* Button types */
enum {
   vHW_ACPIButton_Power,
   vHW_ACPIButton_PowerF,
   vHW_ACPIButton_Sleep,
   vHW_ACPIButton_SleepF,
   vHW_ACPIButton_Lid
};


#endif /* !HIDD_ACPIBUTTON_H */
