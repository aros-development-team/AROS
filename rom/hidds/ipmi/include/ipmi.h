#ifndef HIDD_IPMI_H
#define HIDD_IPMI_H

/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: IPMI (Intelligent Platform Management Interface) hidd definitions.

    hidd.ipmi objects represent one system interface to a Baseboard
    Management Controller (BMC). The attributes describe where the
    interface lives (they follow SMBIOS type 38 / ACPI SPMI); the
    SendCommand method issues an IPMI request to the BMC over it.

    HIDD_IPMI_SendCommand(obj, netfn, command, request, requestLen,
                          response, &responseLen)

        netfn       IPMI network function (0..0x3F), LUN 0 is used.
        command     IPMI command code.
        request     Request data bytes following the command byte (may
                    be NULL when requestLen is 0).
        response    Buffer for the response: the completion code followed
                    by the response data. May be NULL.
        responseLen On entry the size of the response buffer; on return
                    the length of the complete response, which may exceed
                    the buffer size if it was truncated.

        Returns TRUE if the BMC returned a response (whose first byte is
        the completion code, 0 = success), FALSE if the transport failed
        or the interface type is not supported.
*/

#define IID_Hidd_IPMI    "hidd.ipmi"
#define CLID_Hidd_IPMI   IID_Hidd_IPMI

#define CLID_HW_IPMI "hw.ipmi"

#include <interface/Hidd_IPMI.h>

/* aoHidd_IPMI_InterfaceType, values follow SMBIOS type 38 */
enum {
    vHidd_IPMI_Interface_Unknown,
    vHidd_IPMI_Interface_KCS,           /* Keyboard Controller Style */
    vHidd_IPMI_Interface_SMIC,          /* Server Management Interface Chip */
    vHidd_IPMI_Interface_BT,            /* Block Transfer */
    vHidd_IPMI_Interface_SSIF           /* SMBus System Interface */
};

/* aoHidd_IPMI_AddressSpace */
enum {
    vHidd_IPMI_AddressSpace_Memory,
    vHidd_IPMI_AddressSpace_IO
};

/* aoHidd_IPMI_RegisterSpacing */
enum {
    vHidd_IPMI_RegSpacing_1,
    vHidd_IPMI_RegSpacing_4,
    vHidd_IPMI_RegSpacing_16
};

/* Maximum request data length accepted by SendCommand */
#define IPMI_MAX_REQUEST_DATA   250

/* Network functions (request values; responses use netfn + 1) */
#define IPMI_NETFN_CHASSIS      0x00
#define IPMI_NETFN_BRIDGE       0x02
#define IPMI_NETFN_SENSOR_EVENT 0x04
#define IPMI_NETFN_APP          0x06
#define IPMI_NETFN_FIRMWARE     0x08
#define IPMI_NETFN_STORAGE      0x0A
#define IPMI_NETFN_TRANSPORT    0x0C

/* Application commands */
#define IPMI_CMD_GET_DEVICE_ID          0x01
#define IPMI_CMD_COLD_RESET             0x02
#define IPMI_CMD_WARM_RESET             0x03
#define IPMI_CMD_GET_SELF_TEST_RESULTS  0x04
#define IPMI_CMD_RESET_WATCHDOG_TIMER   0x22
#define IPMI_CMD_SET_WATCHDOG_TIMER     0x24
#define IPMI_CMD_GET_WATCHDOG_TIMER     0x25
#define IPMI_CMD_SET_SYSTEM_INFO_PARAMS 0x58
#define IPMI_CMD_GET_SYSTEM_INFO_PARAMS 0x59

/* Chassis commands */
#define IPMI_CMD_GET_CHASSIS_STATUS     0x01
#define IPMI_CMD_CHASSIS_CONTROL        0x02

/* Sensor/Event commands */
#define IPMI_CMD_GET_SENSOR_READING     0x2D

/* Storage commands */
#define IPMI_CMD_GET_SDR_REPOSITORY_INFO 0x20
#define IPMI_CMD_RESERVE_SDR_REPOSITORY 0x22
#define IPMI_CMD_GET_SDR                0x23
#define IPMI_CMD_GET_SEL_INFO           0x40
#define IPMI_CMD_RESERVE_SEL            0x42
#define IPMI_CMD_GET_SEL_ENTRY          0x43
#define IPMI_CMD_CLEAR_SEL              0x47

/* Completion codes */
#define IPMI_CC_OK                      0x00
#define IPMI_CC_NODE_BUSY               0xC0
#define IPMI_CC_INVALID_COMMAND         0xC1
#define IPMI_CC_TIMEOUT                 0xC3
#define IPMI_CC_OUT_OF_SPACE            0xC4
#define IPMI_CC_INVALID_RESERVATION     0xC5
#define IPMI_CC_REQ_DATA_TRUNCATED      0xC6
#define IPMI_CC_REQ_DATA_LEN_INVALID    0xC7
#define IPMI_CC_PARAM_OUT_OF_RANGE      0xC9
#define IPMI_CC_NOT_PRESENT             0xCB
#define IPMI_CC_INVALID_DATA_FIELD      0xCC
#define IPMI_CC_UNSPECIFIED             0xFF

#endif /* !HIDD_IPMI_H */
