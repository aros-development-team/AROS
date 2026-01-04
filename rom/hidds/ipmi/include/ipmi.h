#ifndef HIDD_IPMI_H
#define HIDD_IPMI_H

#define IID_Hidd_IPMI    "hidd.ipmi"
#define CLID_Hidd_IPMI   IID_Hidd_IPMI

#define CLID_HW_IPMI "hw.ipmi"

#include <interface/Hidd_IPMI.h>

enum {
    vHidd_IPMI_Interface_Unknown,
    vHidd_IPMI_Interface_KCS,
    vHidd_IPMI_Interface_SMIC,
    vHidd_IPMI_Interface_BT,
    vHidd_IPMI_Interface_SSIF
};

enum {
    vHidd_IPMI_AddressSpace_Memory,
    vHidd_IPMI_AddressSpace_IO
};

enum {
    vHidd_IPMI_RegSpacing_1,
    vHidd_IPMI_RegSpacing_4,
    vHidd_IPMI_RegSpacing_16
};

#endif /* !HIDD_IPMI_H */
