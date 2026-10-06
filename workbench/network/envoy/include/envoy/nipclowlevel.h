#ifndef ENVOY_NIPCLOWLEVEL_H
#define ENVOY_NIPCLOWLEVEL_H
/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: nipc.library - low-level configuration tags for NIPCControlA()
          and the protocol-author functions (public Envoy developer kit
          definitions). On AROS the IP layer belongs to the TCP/IP stack,
          so the IP, ARP and fragment settings are accepted but have no
          effect; the RDP settings do.
*/

#include <utility/tagitem.h>

#define NIPCTAG_Dummy                   (TAG_USER)

/* IP header manipulation for SendIPPacketA() */
#define NIPCTAG_IPDontFragment          (NIPCTAG_Dummy + 0x1000)
#define NIPCTAG_IPMoreFragments         (NIPCTAG_Dummy + 0x1001)
#define NIPCTAG_IPOptions               (NIPCTAG_Dummy + 0x1002)
#define NIPCTAG_IPOptionsLength         (NIPCTAG_Dummy + 0x1003)
#define NIPCTAG_IPPacketID              (NIPCTAG_Dummy + 0x1004)
#define NIPCTAG_IPTimeToLive            (NIPCTAG_Dummy + 0x1005)
#define NIPCTAG_IPTypeOfService         (NIPCTAG_Dummy + 0x1006)
#define NIPCTAG_IPIdentification        (NIPCTAG_Dummy + 0x1007)
#define NIPCTAG_IPFragmentOffset        (NIPCTAG_Dummy + 0x1008)

/* Protocol / port registration */
#define NIPCTAG_ProtocolName            (NIPCTAG_Dummy + 0x3000)
#define NIPCTAG_ProtocolPriority        (NIPCTAG_Dummy + 0x3001)
#define NIPCTAG_ProtocolPortNumber      (NIPCTAG_Dummy + 0x3002)
#define NIPCTAG_ProtocolUserDataSize    (NIPCTAG_Dummy + 0x3003)
#define NIPCTAG_ProtocolInputICMP       (NIPCTAG_Dummy + 0x3004)
#define NIPCTAG_ProtocolHeartbeat       (NIPCTAG_Dummy + 0x3005)

/* Global configuration: Set... takes a value, Get... a pointer to a ULONG */
#define NIPCTAG_SetDefaultTTL           (NIPCTAG_Dummy + 0x4000)
#define NIPCTAG_SetDefaultTOS           (NIPCTAG_Dummy + 0x4001)
#define NIPCTAG_SetFragmentTO           (NIPCTAG_Dummy + 0x4002)
#define NIPCTAG_SetARPResolveTO         (NIPCTAG_Dummy + 0x4010)
#define NIPCTAG_SetARPResolveRetries    (NIPCTAG_Dummy + 0x4011)
#define NIPCTAG_SetARPEntryTO           (NIPCTAG_Dummy + 0x4012)
#define NIPCTAG_SetRDPInactivityCheck   (NIPCTAG_Dummy + 0x4020)
#define NIPCTAG_SetRDPInactivityLimit   (NIPCTAG_Dummy + 0x4021)
#define NIPCTAG_SetRDPTransmitRetries   (NIPCTAG_Dummy + 0x4022)
#define NIPCTAG_SetRDPTransmitMinTO     (NIPCTAG_Dummy + 0x4023)
#define NIPCTAG_SetRDPTransmitMaxTO     (NIPCTAG_Dummy + 0x4024)
#define NIPCTAG_SetRDPInitialRoundTripTO (NIPCTAG_Dummy + 0x4025)
#define NIPCTAG_SetRDPConnectTO         (NIPCTAG_Dummy + 0x4026)
#define NIPCTAG_SetRDPConnectRetries    (NIPCTAG_Dummy + 0x4027)

#define NIPCTAG_GetDefaultTTL           (NIPCTAG_Dummy + 0x4100)
#define NIPCTAG_GetDefaultTOS           (NIPCTAG_Dummy + 0x4101)
#define NIPCTAG_GetFragmentTO           (NIPCTAG_Dummy + 0x4102)
#define NIPCTAG_GetARPResolveTO         (NIPCTAG_Dummy + 0x4110)
#define NIPCTAG_GetARPResolveRetries    (NIPCTAG_Dummy + 0x4111)
#define NIPCTAG_GetARPEntryTO           (NIPCTAG_Dummy + 0x4112)
#define NIPCTAG_GetRDPInactivityCheck   (NIPCTAG_Dummy + 0x4120)
#define NIPCTAG_GetRDPInactivityLimit   (NIPCTAG_Dummy + 0x4121)
#define NIPCTAG_GetRDPTransmitRetries   (NIPCTAG_Dummy + 0x4122)
#define NIPCTAG_GetRDPTransmitMinTO     (NIPCTAG_Dummy + 0x4123)
#define NIPCTAG_GetRDPTransmitMaxTO     (NIPCTAG_Dummy + 0x4124)
#define NIPCTAG_GetRDPInitialRoundTripTO (NIPCTAG_Dummy + 0x4125)
#define NIPCTAG_GetRDPConnectTO         (NIPCTAG_Dummy + 0x4126)
#define NIPCTAG_GetRDPConnectRetries    (NIPCTAG_Dummy + 0x4127)

/* (ULONG *) in: an IP address; out: the MTU of the route to it, 0 if none */
#define NIPCTAG_GetMTUforIP             (NIPCTAG_Dummy + 0x4200)

/* AS225-style control channel (not supported on AROS: the stack owns the interfaces) */
#define NIPCTAG_ControlInterfaceName    (NIPCTAG_Dummy + 0x4300)
#define NIPCTAG_ControlCommand          (NIPCTAG_Dummy + 0x4301)
#define NIPCTAG_ControlData             (NIPCTAG_Dummy + 0x4302)
#define NIPCTAG_ControlResult           (NIPCTAG_Dummy + 0x4303)
#define NIPCTAG_ControlResult2          (NIPCTAG_Dummy + 0x4304)

#endif /* ENVOY_NIPCLOWLEVEL_H */
