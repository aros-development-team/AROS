#ifndef HIDD_ACPIPOWERMETER_H
#define HIDD_ACPIPOWERMETER_H

#include <interface/HW_ACPIPowerMeter.h>

#define CLID_HW_ACPIPowerMeter "hw.acpi.powermeter"

#define IS_HWACPIPOWERMETER_ATTR(attr, idx) IS_IF_ATTR(attr, idx, HWACPIPowerMeterAB, num_HW_ACPIPowerMeter_Attrs)

#endif /* !HIDD_ACPIPOWERMETER_H */
