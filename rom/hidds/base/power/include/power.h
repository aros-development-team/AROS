#ifndef HIDD_POWER_H
#define HIDD_POWER_H

#define IID_Hidd_Power    "hidd.power"
#define CLID_Hidd_Power   IID_Hidd_Power                

#include <interface/Hidd_Power.h>

/* Power Object Types */
enum {
   vHW_PowerType_Unknown,
   vHW_PowerType_Battery,
   vHW_PowerType_AC
};

/* Power Object States */
enum {
   vHW_PowerState_NotPresent,
   vHW_PowerState_Discharging,
   vHW_PowerState_Charging
};

/* Battery flags */
enum {
   vHW_PowerFlag_Unknown,
   vHW_PowerFlag_Low,
   vHW_PowerFlag_High,
   vHW_PowerFlag_Critical
};

/* Power Object Units */
enum {
   vHW_PowerUnit_mA,
   vHW_PowerUnit_mW
};

#endif /* !HIDD_POWER_H */
