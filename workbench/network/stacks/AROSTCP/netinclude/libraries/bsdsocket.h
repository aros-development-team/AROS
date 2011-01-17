#ifndef _LIBRARIES_BSDSOCKET_H
#define _LIBRARIES_BSDSOCKET_H

/*
 * $Id$
 *
 * :ts=8
 *
 * Copyright © 2006 by Pavel Fedin
 *
 * Taken from 'Roadshow' -- Amiga TCP/IP stack
 * Copyright © 2001-2004 by Olaf Barthel.
 * All Rights Reserved.
 *
 * Amiga specific TCP/IP 'C' header files;
 * Freely Distributable
 */

/****************************************************************************/

#ifndef EXEC_PORTS_H
#include <exec/ports.h>
#endif /* EXEC_PORTS_H */

#ifndef UTILITY_TAGITEM_H
#include <utility/tagitem.h>
#endif /* UTILITY_TAGITEM_H */

#ifndef DOS_H
#include <dos/dos.h>
#endif /* DOS_H */

#include <bsdsocket/socketbasetags.h>

/****************************************************************************/

#ifndef _SYS_SOCKET_H_
#include <sys/socket.h>
#endif /* _SYS_SOCKET_H_ */

#ifndef _NETINET_IN_H_
#include <netinet/in.h>
#endif /* _NETINET_IN_H_ */

#ifndef _NET_IF_H_
#include <net/if.h>
#endif /* _NET_IF_H_ */

#ifndef _SYS_MBUF_H_
#include <sys/mbuf.h>
#endif /* _SYS_MBUF_H_ */

/****************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/****************************************************************************/

#ifdef __GNUC__
 #ifdef __PPC__
  #pragma pack(2)
 #endif
#elif defined(__VBCC__)
 #pragma amiga-align
#endif

/****************************************************************************/

/*
 * Additional parameters for SocketBaseTagList().
 * Standard bsdsocket.library v4 tags are placed in amitcp/socketbasetags.h
 */

/* Number of Berkeley packet filter channels available. */
#define SBTC_NUM_PACKET_FILTER_CHANNELS 40

/* Whether or not the routing API is supported. */
#define SBTC_HAVE_ROUTING_API 41

/* Enable/Disable UDP checksums? */
#define SBTC_UDP_CHECKSUM 42

/* Enable/Disable IP packet forwarding? */
#define SBTC_IP_FORWARDING 43

/* Get/set the default IP packet TTL value. */
#define SBTC_IP_DEFAULT_TTL 44

/* Respond to ICMP mask requests? */
#define SBTC_ICMP_MASK_REPLY 45

/* Enable/Disable sending of redirection messages? */
#define SBTC_ICMP_SEND_REDIRECTS 46

/* Whether or not the interface API is supported. */
#define SBTC_HAVE_INTERFACE_API 47

/* How ICMP echo messages should be processed. */
#define SBTC_ICMP_PROCESS_ECHO 48

/* How ICMP time stamp messages should be processed. */
#define SBTC_ICMP_PROCESS_TSTAMP 49

/* Whether or not the monitoring API is supported. */
#define SBTC_HAVE_MONITORING_API 50

/* Whether or not library bases can be shared by different
   callers. */
#define SBTC_CAN_SHARE_LIBRARY_BASES 51

/* Set the name of the log output file or get a pointer
   to the current file name. */
#define SBTC_LOG_FILE_NAME 52

/* Whether or not the status API is supported. */
#define SBTC_HAVE_STATUS_API 53

/* Whether or not the DNS API is supported. */
#define SBTC_HAVE_DNS_API 54

/* Set or get a pointer to the currently installed log hook. */
#define SBTC_LOG_HOOK 55

/* Get the system status, with regard to whether the TCP/IP stack
   has access to network interfaces and name resolution servers. */
#define SBTC_SYSTEM_STATUS 56

/* Get or set the signal mask used to send a notification when
   the address of an interface has changed. */
#define SBTC_SIG_ADDRESS_CHANGE_MASK 57

/* If the IP filter API is supported, which version of that API
   would it be? */
#define SBTC_IPF_API_VERSION 58

/* Whether or not the local database access API is supported. */
#define SBTC_HAVE_LOCAL_DATABASE_API 59

/* Whether or not the address conversion API is supported. */
#define SBTC_HAVE_ADDRESS_CONVERSION_API 60

/* Whether or not the kernel memory API is supported. */
#define SBTC_HAVE_KERNEL_MEMORY_API 61

/* Get or set the IP filter hook which is invoked whenever a
   datagram has arrived or is about to be sent. */
#define SBTC_IP_FILTER_HOOK 62

/* Whether or not the server API is supported. */
#define SBTC_HAVE_SERVER_API 63

/* Query the number of bytes received so far. */
#define SBTC_GET_BYTES_RECEIVED 64

/* Query the number of bytes sent so far. */
#define SBTC_GET_BYTES_SENT 65

/****************************************************************************/

/*
 * Flags returned by a SocketBaseTagList() 'SBTC_SYSTEM_STATUS' query.
 */

#define SBSYSSTAT_Interfaces		(1L<<0)	/* Interfaces are available,
						   configured and
						   operational. */
#define SBSYSSTAT_PTP_Interfaces	(1L<<1)	/* Point-to-point interfaces
						   are available, configured
						   and operational. */
#define SBSYSSTAT_BCast_Interfaces	(1L<<2)	/* Broadcast interfaces are
						   available, configured and
						   operational. */
#define SBSYSSTAT_Resolver		(1L<<3)	/* Domain name servers are
						   known and available */
#define SBSYSSTAT_Routes		(1L<<4)	/* Routing information is
						   configured */
#define SBSYSSTAT_DefaultRoute		(1L<<5)	/* A default route is
						   configured */

/****************************************************************************/

/*
 * Data structures for use with the log hook.
 */

struct LogHookMessage
{
	LONG	lhm_Size;	/* Size of this data structure in bytes. */
	LONG	lhm_Priority;	/* Log entry priority (LOG_EMERG..LOG_DEBUG) */
	struct DateStamp
		lhm_Date;	/* Time and date when this log entry was
				   added. */
	STRPTR	lhm_Tag;	/* Name of the facility which added this entry;
				   this can be NULL. */
	ULONG	lhm_ID;		/* ID of the facility which added this entry;
				   this can be zero. */
	STRPTR	lhm_Message;	/* NUL-terminated string which contains the
				   log message to be displayed. */
};

/****************************************************************************/

/*
 * Options for configuring how ICMP echo and time stamp
 * requests should be processed.
 */
#define IR_Process	0	/* Process the request and respond to it */
#define IR_Ignore	1	/* Ignore the request, but feed it into the
				   raw IP packet processing framework */
#define IR_Drop		2	/* Ignore the request and treat it as if
				   it had a checksum error */

/****************************************************************************/

/*
 * For use with ReleaseSocket() and ReleaseCopyOfSocket().
 */

#define UNIQUE_ID (-1)

/****************************************************************************/

/*
 * Tags for use with the routing API.
 */

#define RTA_BASE (TAG_USER+1600)

#define RTA_Destination		(RTA_BASE+1)	/* Route destination
						   address */
#define RTA_Gateway		(RTA_BASE+2)	/* Gateway address */
#define RTA_DefaultGateway	(RTA_BASE+3)	/* Default gateway address */
#define RTA_DestinationHost	(RTA_BASE+4)	/* Route destination
						   address; destination is
						   assumed to be a host and
						   not a network */
#define RTA_DestinationNet	(RTA_BASE+5)	/* Route destination
						   address; destination is
						   assumed to be a network
						   and not a host */

/****************************************************************************/

/*
 * Tags for use with the interface management API,
 * specifically the AddInterfaceTagList() call.
 */
#define IFA_BASE (TAG_USER+1700)

#define IFA_IPType		(IFA_BASE+1)	/* IP packet type */
#define IFA_ARPType		(IFA_BASE+2)	/* ARP packet type */
#define IFA_NumReadRequests	(IFA_BASE+3)	/* Number of read requests
						   to queue */
#define IFA_NumWriteRequests	(IFA_BASE+4)	/* Number of write requests
						   to queue */
#define IFA_NumARPRequests	(IFA_BASE+5)	/* Number of ARP requests
						   to queue */
#define IFA_PacketFilterMode	(IFA_BASE+6)	/* Operating mode of the
						   Berkeley packet filter */
#define IFA_PointToPoint	(IFA_BASE+7)	/* Whether or not this
						   interface is of the
						   point-to-point type */
#define IFA_Reserved0		(IFA_BASE+8)	/* This tag is reserved for
						   future use */
#define IFA_Multicast		(IFA_BASE+9)	/* Whether or not this
						   interface is allowed to
						   send multicast packets */
#define IFA_DownGoesOffline	(IFA_BASE+10)	/* Whether or not this
						   interface will go offline
						   when it is taken down */
#define IFA_ReportOffline	(IFA_BASE+11)	/* Whether or not this
						   interface will cause a
						   notification message to
						   be logged when it goes
						   offline */
#define IFA_RequiresInitDelay	(IFA_BASE+12)	/* Whether or not this
						   interface requires a short
						   delay to precede the
						   device initialization */
#define IFA_CopyMode		(IFA_BASE+13)	/* Special data copy options
						   for this interface */
#define IFA_HardwareAddress	(IFA_BASE+14)	/* Set the interface hardware
						   address to a certain
						   value. */
#define IFA_SetDebugMode	(IFA_BASE+15)	/* Enable or disable debugging
						   mode for this interface. */

/****************************************************************************/

/* This is used with the 'IFA_HardwareAddress' tag above. */
struct InterfaceHardwareAddress
{
	LONG	iha_Length;	/* Number of bits in address */
	UBYTE *	iha_Address;	/* Points to first address byte */
};

/****************************************************************************/

/*
 * Options available for use with the IFA_PacketFilterMode tag.
 */
#define PFM_Nothing	0	/* Filter is disabled for this interface. */
#define PFM_Local	1	/* Filter is enabled; only packets intended
				   for this interface are filtered */
#define PFM_IPandARP	2	/* Filter is enabled; all IP and ARP packets
				   are filtered, even if they are not intended
				   for this interface */
#define PFM_Everything	3	/* Filter is enabled; all packets are filtered
				   that pass by, regardless of their type and
				   contents */

/****************************************************************************/

/*
 * Options available for use with the IFA_CopyMode tag.
 */
#define CM_SlowWordCopy	0	/* Disables the S2_CopyFromBuff16 SANA-II
				   buffer management option (default). */
#define CM_FastWordCopy	1	/* Enables the S2_CopyFromBuff16 SANA-II
				   buffer management option. */

/****************************************************************************/

/*
 * Tags for use with the interface management API,
 * specifically the ConfigureInterfaceTagList() call.
 */
#define IFC_BASE (TAG_USER+1800)

#define IFC_Address		(IFC_BASE+1)	/* The address to assign to
						   the interface itself */
#define IFC_NetMask		(IFC_BASE+2)	/* The interface's address
						   net mask */
#define IFC_DestinationAddress	(IFC_BASE+3)	/* The address of the other
						   end of a point-to-point
						   link */
#define IFC_BroadcastAddress	(IFC_BASE+4)	/* The broadcast address to
						   be used by the interface */
#define IFC_Metric		(IFC_BASE+5)	/* The routing metric value */

#define IFC_AddAliasAddress	(IFC_BASE+6)	/* Add another alias address
						   for this interface */
#define IFC_DeleteAliasAddress	(IFC_BASE+7)	/* Remove an alias address
						   for this interface */
#define IFC_State		(IFC_BASE+8)	/* Mark an interface up, down,
						   online or offline */
#define IFC_GetPeerAddress	(IFC_BASE+9)	/* If available, obtain the
						   interface's local address
						   and the address of its
						   point to point partner */
#define IFC_GetDNS		(IFC_BASE+10)	/* If available, obtain the
						   domain name servers known
						   to this interface */
#define IFC_AssociatedRoute	(IFC_BASE+12)	/* That interface is associated
						   with a route. */
#define IFC_AssociatedDNS	(IFC_BASE+13)	/* That interface is associated
						   with a DNS. */
#define IFC_ReleaseAddress	(IFC_BASE+14)	/* Release the address allocated
						   for this interface (via the
						   DHCP protocol). */
#define IFC_SetDebugMode	(IFC_BASE+15)	/* Enable or disable debugging
						   mode for this interface. */
#define IFC_Complete		(IFC_BASE+16)	/* Indicate that the configuration
						   for this interface is complete */

/****************************************************************************/

/*
 * Tags for use with the interface management API,
 * specifically the QueryInterfaceTagList() call.
 */
#define IFQ_BASE (TAG_USER+1900)

#define IFQ_DeviceName		(IFQ_BASE+1)	/* Query the associated
						   SANA-II device name */
#define IFQ_DeviceUnit		(IFQ_BASE+2)	/* Query the associated
						   SANA-II unit number */
#define IFQ_HardwareAddressSize	(IFQ_BASE+3)	/* Query the hardware address
						   size (in bytes) */
#define IFQ_HardwareAddress	(IFQ_BASE+4)	/* Query the hardware
						   address */
#define IFQ_MTU			(IFQ_BASE+5)	/* Query the maximum
						   transmission unit */
#define IFQ_BPS			(IFQ_BASE+6)	/* Query the transmission
						   speed */
#define IFQ_HardwareType	(IFQ_BASE+7)	/* Query the SANA-II hardware
						   type */
#define IFQ_PacketsReceived	(IFQ_BASE+8)	/* Query the number of
						   packets received by this
						   interface */
#define IFQ_PacketsSent		(IFQ_BASE+9)	/* Query the number of
						   packets sent by this
						   interface */
#define IFQ_BadData		(IFQ_BASE+10)	/* Query the number of bad
						   packets dropped by this
						   interface */
#define IFQ_Overruns		(IFQ_BASE+11)	/* Query the number of buffer
						   overruns */
#define IFQ_UnknownTypes	(IFQ_BASE+12)	/* Query the number of unknown
						   packet types dropped by this
						   interface */
#define IFQ_LastStart		(IFQ_BASE+13)	/* Query the last time the
						   interface went online */
#define IFQ_Address		(IFQ_BASE+14)	/* Query the IP address
						   associated with this
						   interface */
#define IFQ_DestinationAddress	(IFQ_BASE+15)	/* Query the interface's
						   destination address */
#define IFQ_BroadcastAddress	(IFQ_BASE+16)	/* Query the interface's
						   broadcast address */
#define IFQ_NetMask		(IFQ_BASE+17)	/* Query the interface's
						   subnet mask */
#define IFQ_Metric		(IFQ_BASE+18)	/* Query the interface's
						   metric value */
#define IFQ_State		(IFQ_BASE+19)	/* Query the interface's
						   status */
#define IFQ_AddressBindType	(IFQ_BASE+20)	/* Query whether the address
						   bound to this interface
						   is statically, manually
						   or dynamically bound. */
#define IFQ_AddressLeaseExpires	(IFQ_BASE+21)	/* Find out whether and when
						   the address bound to this
						   interface expires. */
#define IFQ_PrimaryDNSAddress	(IFQ_BASE+22)	/* Query the primary domain
						   name server address known
						   to this interface */
#define IFQ_SecondaryDNSAddress	(IFQ_BASE+23)	/* Query the secondary domain
						   name server address known
						   to this interface */
#define IFQ_NumReadRequests	(IFQ_BASE+24)	/* Number of read I/O requests
						   allocated for this
						   interface */
#define IFQ_MaxReadRequests	(IFQ_BASE+25)	/* Maximum number of read I/O
						   requests in use at a time
						   on this interface */
#define IFQ_NumWriteRequests	(IFQ_BASE+26)	/* Number of write I/O requests
						   allocated for this
						   interface */
#define IFQ_MaxWriteRequests	(IFQ_BASE+27)	/* Maximum number of write I/O
						   requests in use at a time
						   on this interface */
#define IFQ_GetBytesIn		(IFQ_BASE+28)	/* Query the number of bytes
						   received */
#define IFQ_GetBytesOut		(IFQ_BASE+29)	/* Query the number of bytes
						   sent */
#define IFQ_GetDebugMode	(IFQ_BASE+30)	/* Check if this interface has
						   the debug mode enabled */
#define IFQ_GetSANA2CopyStats	(IFQ_BASE+31)	/* Obtain the SANA-II data
						   copy statistics */
#define IFQ_NumReadRequestsPending (IFQ_BASE+32)
						/* Number of read I/O requests
						   still pending to be
						   satisfied on this
						   interface */
#define IFQ_NumWriteRequestsPending (IFQ_BASE+33)
						/* Number of write I/O requests
						   still pending to be
						   satisfied on this
						   interface */

/****************************************************************************/

/* This is used with the 'IFQ_GetSANA2CopyStats' tag above. */
struct SANA2CopyStats
{
	ULONG	s2cs_DMAIn;	/* How many times data was received via the
				   DMA transfer function */
	ULONG	s2cs_DMAOut;	/* How many times data was sent via the
				   DMA transfer function */

	ULONG	s2cs_ByteIn;	/* How many times data was received via the
				   byte wide copy function */
	ULONG	s2cs_ByteOut;	/* How many times data was sent via the
				   byte wide copy function */

	ULONG	s2cs_WordOut;	/* How many times data was sent via the
				   word wide copy function */
};

/****************************************************************************/

/* The different types of interface address binding. */
#define IFABT_Unknown	0	/* The interface address has not been bound
				   or is in transitional state; check later
				   for more information. */
#define IFABT_Static	1	/* The address was assigned manually, or
				   by an automated configuration process
				   and is not expected to change. */
#define IFABT_Dynamic	2	/* The address was assigned by an automated
				   configuration process and may change in the
				   future. */

/****************************************************************************/

/*
 * The 64 bit integer value used by the IFQ_GetBytesIn..IFQ_GetResetBytesOut
 * query tags.
 */
typedef struct
{
	ULONG sbq_High;
	ULONG sbq_Low;
} SBQUAD_T;

/****************************************************************************/

/*
 * Options available for use with the IFC_State and IFQ_State tags.
 */
#define SM_Offline	0	/* Interface is offline and not ready to
				   receive and transmit data */
#define SM_Online	1	/* Interface is online and ready to receive
				   and transmit data */
#define SM_Down		2	/* Interface is not ready to receive and
				   transmit data, but might still be
				   online. */
#define SM_Up		3	/* Interface is ready to receive and transmit
				   data, but not necessarily online. */

/****************************************************************************/

/*
 * Types of monitoring hooks that can be installed.
 */
#define MHT_ICMP	0	/* ICMP message monitoring */
#define MHT_UDP		1	/* UDP datagram monitoring */
#define MHT_TCP_Connect	2	/* TCP connection monitoring */
#define MHT_Connect	3	/* connect() call monitoring */
#define MHT_Send	4	/* sendto() and sendmsg() monitoring */
#define MHT_Packet	5	/* Low level packet monitoring */
#define MHT_Bind	6	/* bind() call monitoring */

/****************************************************************************/

/*
 * Parameters passed to the different monitoring hooks.
 */

/* This type of message parameter is passed to 'MHT_Connect'
 * type monitoring hooks.
 */
struct ConnectMonitorMsg
{
	LONG		cmm_Size;	/* Size of this data
					   structure */
	STRPTR		cmm_Caller;	/* The name of the calling
					   program, if it chose to be
					   identified */
	LONG		cmm_Socket;	/* The socket to connect to
					   the address */
	struct sockaddr *
			cmm_Name;	/* The address to connect
					   to */
	LONG		cmm_NameLen;	/* The size of the address */
};

/* This type of message parameter is passed to 'MHT_Bind'
 * type monitoring hooks.
 */
struct BindMonitorMsg
{
	LONG		bmm_Size;	/* Size of this data
					   structure */
	STRPTR		bmm_Caller;	/* The name of the calling
					   program, if it chose to be
					   identified */
	LONG		bmm_Socket;	/* The socket to bind to the
					   address */
	struct sockaddr *
			bmm_Name;	/* The address to bind */
	LONG		bmm_NameLen;	/* The size of the address */
};

/* This type of message is passed to 'MHT_Send' type monitoring hooks. */
struct SendMonitorMessage
{
	LONG		smm_Size;	/* Size of this data structure */
	STRPTR		smm_Caller;	/* The name of the calling
					   program, if it chose to be
					   identified */
	LONG		smm_Socket;	/* The socket to connect to the
					   address */
	APTR		smm_Buffer;	/* Data to be sent */
	LONG		smm_Len;	/* Amount of data to be sent */
	LONG		smm_Flags;	/* Control flags, including
					   MSG_OOB or MSG_DONTROUTE */
	struct sockaddr *
			smm_To;		/* The address to send the
					   data to; this may be NULL */
	LONG		smm_ToLen;	/* The size of the address to send
					   this data to; this may be
					   NULL */
	struct msghdr *	smm_Msg;	/* The message to send; this may
					   be NULL. */
};

/* This type of message is passed to 'MHT_TCP_Connect' type
 * monitoring hooks.
 */
struct TCPConnectMonitorMessage
{
	LONG		tcmm_Size;	/* Size of this data structure */
	struct in_addr *
			tcmm_Src;	/* Source internet address */
	struct in_addr *
			tcmm_Dst;	/* Destination internet address */
	struct tcphdr *	tcmm_TCP;	/* TCP header */
};

/* This type of message is passed to 'MHT_UDP' type monitoring hooks. */
struct UDPMonitorMessage
{
	LONG		umm_Size;	/* Size of this data structure */
	struct in_addr *
			umm_Src;	/* Source internet address */
	struct in_addr *
			umm_Dst;	/* Destination internet address */
	struct udphdr *	umm_UDP;	/* UDP header */
};

/* This type of message is passed to 'MHT_ICMP' type monitoring hooks. */
struct ICMPMonitorMessage
{
	LONG		imm_Size;	/* Size of this data structure */
	struct in_addr *
			imm_Src;	/* Source internet address */
	struct in_addr *
			imm_Dst;	/* Destination internet address */
	struct icmp *	imm_ICMP;	/* ICMP header */
};

/* This type of message is passed to 'MHT_Packet' type monitoring hooks. */
struct PacketMonitorMessage
{
	LONG		pmm_Size;	/* Size of this data structure */
	LONG		pmm_Direction;	/* Whether the packet was received
					   or is about to be sent */
	APTR		pmm_LinkLayerData;
					/* Points to the link layer part
					   of the packet, typically an
					   Ethernet header per RFC 894.
					   This may be NULL. */
	LONG		pmm_LinkLayerSize;
					/* Size of the link layer data part
					   of the packet; this may be 0. */
	APTR		pmm_PacketData;	/* Points to the packet 'payload'. */
	LONG		pmm_PacketSize;	/* Size of the packet 'payload'. */
};

/* Possible values for 'pmm_Direction'. */
#define PMMD_Receive	0	/* Packet was received and is waiting to be
				   processed */
#define PMMD_Send	1	/* Packet is about to be sent */

/****************************************************************************/

/*
 * Possible actions to be taken after a monitoring hook has
 * examined the data it was passed. Any positive return value
 * will cause the data to be dropped and the corresponding
 * errno value to be set to the result.
 */
#define MA_Continue		0	/* Proceed as if no filtering had
					   taken place */
#define MA_Ignore		-1	/* Ignore the data and skip the
					   normal route processing would
					   take */
#define MA_Drop			-2	/* Drop the data */
#define MA_DropWithReset	-3	/* Drop the data and also reset the
					   connection */

/****************************************************************************/

/*
 * Parameters for use with the GetNetworkStatus() function.
 */

/* What version of the statistics data should be returned; so far
   there is only version #1. */
#define NETWORKSTATUS_VERSION 1

/* What statistics should be provided */

#define NETSTATUS_icmp		0	/* Internet control message
					   protocol statistics */
#define NETSTATUS_igmp		1	/* Internet group management
					   protocol statistics */
#define NETSTATUS_ip		2	/* Internet protocol statistics */
#define NETSTATUS_mb		3	/* Memory buffer statistics */
#define NETSTATUS_mrt		4	/* Multicast routing statistics */
#define NETSTATUS_rt		5	/* Routing statistics */
#define NETSTATUS_tcp		6	/* Transmission control protocol
					   statistics */
#define NETSTATUS_udp		7	/* User datagram protocol
					   statistics */
#define NETSTATUS_tcp_sockets	9	/* TCP socket statistics */
#define NETSTATUS_udp_sockets	10	/* UDP socket statistics */

/* Protocol connection data returned for each TCP/UDP socket. */
struct protocol_connection_data
{
	struct in_addr
		pcd_foreign_address;	/* Foreign host address */
	UWORD	pcd_foreign_port;	/* Foreign port number */
	struct in_addr
		pcd_local_address;	/* Local host address */
	UWORD	pcd_local_port;		/* Local port number */
	ULONG	pcd_receive_queue_size;	/* Socket receive queue size */
	ULONG	pcd_send_queue_size;	/* Socket send queue size */
	LONG	pcd_tcp_state;		/* Socket TCP state */
};

/****************************************************************************/

/*
 * Interface address allocation (BOOTP and DHCP).
 */

/* The message to send to the interface configuration process
   to start looking for an IP address. */
struct AddressAllocationMessage
{
	struct Message
		aam_Message;
	LONG	aam_Reserved;
	LONG	aam_Result;		/* What kind of result this
					   request produced. */
	LONG	aam_Version;		/* Version number associated with
					   this data structure. */
	LONG	aam_Protocol;		/* What kind of configuration
					   protocol should be used. */
	char	aam_InterfaceName[16];	/* Name of interface an
					   address is to be
					   assigned to. */
	LONG	aam_Timeout;		/* Number of seconds to
					   wait before a result
					   materializes. */
	ULONG	aam_LeaseTime;		/* Requested lease time in
					   seconds; 0 to accept the
					   default. */
	ULONG	aam_RequestedAddress;	/* Interface address that should
					   be assigned, if possible; 0
					   to accept the server's choice. */
	STRPTR	aam_ClientIdentifier;	/* Unique identifier for this
					   host */
	ULONG	aam_Address;		/* Interface address
					   returned upon
					   success. */
	ULONG	aam_ServerAddress;	/* Address of BOOTP server. */
	ULONG	aam_SubnetMask;		/* Interface subnet
					   mask; ignore if zero. */

	STRPTR	aam_NAKMessage;		/* In case of failure,
					   put the explanation
					   text here. */
	LONG	aam_NAKMessageSize;	/* Maximum length of the
					   negative ack message. */

	ULONG *	aam_RouterTable;	/* A list of router addresses
					   will be put here. */
	LONG	aam_RouterTableSize;	/* Maximum number of valid
					   router addresses. */
	
	ULONG *	aam_DNSTable;		/* A list of domain name servers
					   will be put here. */
	LONG	aam_DNSTableSize;	/* Maximum number of valid
					   domain name server
					   addresses. */

	ULONG *	aam_StaticRouteTable;	/* A list of static routes
					   will be put here. */
	LONG	aam_StaticRouteTableSize;
					/* Maximum number of valid
					   static routes. */

	STRPTR	aam_HostName;		/* If available, the name
					   assigned to this host will
					   be put here. */
	LONG	aam_HostNameSize;	/* Maximum size of the host
					   name. */

	STRPTR	aam_DomainName;		/* If available, the name
					   of the domain assigned to
					   this host will be put here. */
	LONG	aam_DomainNameSize;	/* Maximum size of the domain
					   name. */
	UBYTE *	aam_BOOTPMessage;	/* Points to buffer to place
					   the BOOTP message in. */
	LONG	aam_BOOTPMessageSize;	/* Size of the buffer to place
					   the BOOTP message in. */
	struct DateStamp *
		aam_LeaseExpires;	/* Points to buffer to place the
					   lease expire date and time in.
					   in. This will be 0 if no data
					   is provided or if the lease
					   never expires. */
};

/* This data structure version. */
#define AAM_VERSION 1

/* Available result codes. */
#define AAMR_Success		0	/* Allocation has succeeded */
#define AAMR_Aborted		1	/* The allocation was aborted */
#define AAMR_InterfaceNotKnown	2	/* The interface name is not
					   known */
#define AAMR_InterfaceWrongType	3	/* The interface must support
					   broadcast access. */
#define AAMR_AddressKnown	4	/* The interface address is already
					   known. */
#define AAMR_VersionUnknown	5	/* The data structure version is
					   not supported. */
#define AAMR_NoMemory		6	/* Not enough memory to process
					   the request. */
#define AAMR_Timeout		7	/* The allocation request did not
					   succeed in the requested time
					   span. */
#define AAMR_AddressInUse	8	/* The address to be allocated is
					   already in use. */
#define AAMR_AddrChangeFailed	9	/* The interface address could
					   not be changed. */
#define AAMR_MaskChangeFailed	10	/* The interface subnet mask could
					   not be changed. */
#define AAMR_Ignored		-1	/* The message type was not
					   understood */

/* The minimum timeout value supported for an allocation to succeed. */
#define AAM_TIMEOUT_MIN 10

/* The special DHCP lease times. */
#define DHCP_DEFAULT_LEASE_TIME		0
#define DHCP_INFINITE_LEASE_TIME	0xFFFFFFFF

/* The configuration protocols to use. */
#define AAMP_BOOTP	0	/* Bootstrap Protocol (RFC 951) */
#define AAMP_DHCP	1	/* Dynamic Host Configuration Protocol
				   (RFC 2131) */
#define AAMP_SLOWAUTO	2	/* Automatic address allocation; slower
				   version for Ethernet networks with
				   switches which use the IEEE Spanning
				   Tree Protocol (802.1d) */
#define AAMP_FASTAUTO	3	/* Automatic address allocation; faster
				   version for wireless devices */

/* Tags for use with CreateAddressAllocationMessage(). */
#define CAAMTA_BASE (TAG_USER+2000)

/* This corresponds to the 'aam_Timeout' member of the
 * AddressAllocationMessage.
 */
#define CAAMTA_Timeout			(CAAMTA_BASE+1)

/* This corresponds to the 'aam_LeaseTime' member of the
 * AddressAllocationMessage.
 */
#define CAAMTA_LeaseTime		(CAAMTA_BASE+2)

/* This corresponds to the 'aam_RequestedAddress' member of the
 * AddressAllocationMessage.
 */
#define CAAMTA_RequestedAddress		(CAAMTA_BASE+3)

/* Pointer to the client identifier string to be used; this
 * string must be at least 2 characters long. The string will
 * be duplicated and stored in the 'aam_ClientIdentifier' member
 * of the AddressAllocationMessage.
 */
#define CAAMTA_ClientIdentifier		(CAAMTA_BASE+4)

/* Size of the buffer to allocate for the NAK message, as
 * stored in the 'aam_NAKMessage' member of the AddressAllocationMessage.
 */
#define CAAMTA_NAKMessageSize		(CAAMTA_BASE+5)

/* Size of the buffer to allocate for the router address table, as
 * stored in the 'aam_RouterTable' member of the
 * AddressAllocationMessage.
 */
#define CAAMTA_RouterTableSize		(CAAMTA_BASE+6)

/* Size of the buffer to allocate for the DNS address table, as
 * stored in the 'aam_DNSTable' member of the AddressAllocationMessage.
 */
#define CAAMTA_DNSTableSize		(CAAMTA_BASE+7)

/* Size of the buffer to allocate for the static route address table, as
 * stored in the 'aam_StaticRouteTable' member of the
 * AddressAllocationMessage.
 */
#define CAAMTA_StaticRouteTableSize	(CAAMTA_BASE+8)

/* Size of the buffer to allocate for the host name, as stored in
 * the 'aam_HostName' member of the AddressAllocationMessage.
 */
#define CAAMTA_HostNameSize		(CAAMTA_BASE+9)

/* Size of the buffer to allocate for the domain name, as stored in
 * the 'aam_DomainName' member of the AddressAllocationMessage.
 */
#define CAAMTA_DomainNameSize		(CAAMTA_BASE+10)

/* Size of the buffer to allocate for the BOOTP message, as stored in
 * the 'aam_BOOTPMessage' member of the AddressAllocationMessage.
 */
#define CAAMTA_BOOTPMessageSize		(CAAMTA_BASE+11)

/* Either FALSE or TRUE; if TRUE, will allocate a buffer for a
 * DateStamp and put its address into the 'aam_LeaseExpires'
 * member of the AddressAllocationMessage.
 */
#define CAAMTA_RecordLeaseExpiration	(CAAMTA_BASE+12)

/* The MsgPort to send the AddressAllocationMessage to after
 * the configuration has finished.
 */
#define CAAMTA_ReplyPort		(CAAMTA_BASE+13)

/* Codes returned by CreateAddressAllocationMessage(). */
#define CAAME_Success			0	/* It worked */
#define CAAME_Invalid_result_ptr	1	/* No proper result pointer
						   provided */
#define CAAME_Not_enough_memory		2	/* No enough memory available */
#define CAAME_Invalid_version		3	/* The version number is not
						   in order */
#define CAAME_Invalid_protocol		4	/* The protocol is neither BOOTP
						   nor DHCP */
#define CAAME_Invalid_interface_name	5	/* The interface name is
						   not OK */
#define CAAME_Interface_not_found	6	/* The name of the interface
						   is not known */
#define CAAME_Invalid_client_identifier	7	/* The client identifier is not
						   OK */
#define CAAME_Client_identifier_too_short 8	/* The client identifier is too
						   short */
#define CAAME_Client_identifier_too_long 9	/* The client identifier is too
						   long */

/****************************************************************************/

/*
 * The DNS management data structures.
 */

/* These nodes are returned by the DNS API. */
struct DomainNameServerNode
{
	struct MinNode
		dnsn_MinNode;
	LONG	dnsn_Size;	/* Size of this data structure */
	STRPTR	dnsn_Address;	/* Points to NUL-terminated string
				   which holds the IP address in
				   dotted-decimal notation */
	LONG	dnsn_UseCount;	/* Usage count of this address;
				   negative values indicate
				   statically-configured servers. */
};

/****************************************************************************/

/*
 * Filtering for incoming and outgoing IP packets.
 */

/* This identifies whether a packet was received or is about
 * to be sent. Check the IPFilterMsg->ifm_Direction field to
 * find out.
 */
#define IFMD_Incoming 0	/* Packet was received */
#define IFMD_Outgoing 1	/* Packet is about to be sent */

/* The packet filter hook is invoked with a message of
 * this type:
 */
struct IPFilterMsg
{
	LONG		ifm_Size;	/* Size of this data structure */
	struct ip *	ifm_IP;		/* Points to IP packet header */
	LONG		ifm_IPLength;	/* Size of the IP packet header */
	struct ifnet *	ifm_Interface;	/* Interface this packet either
					   came in from or is to be
					   sent to */
	LONG		ifm_Direction;	/* Whether this packet came in
					   or is about to go out */
	struct mbuf *	ifm_Packet;	/* The entire packet, as stored
					   in a memory buffer */
};

/****************************************************************************/

/*
 * Network shutdown
 */

/* To shut down the network, send a message of the following form to the
 * network controller message port.
 */
struct NetShutdownMessage
{
	struct Message	nsm_Message;	/* Standard Message header */

	ULONG		nsm_Command;	/* The action to be performed */

	APTR		nsm_Data;	/* Payload */
	ULONG		nsm_Length;	/* Payload size */

	LONG		nsm_Error;	/* Whether or not the command
					   suceeded */
	ULONG		nsm_Actual;	/* How much data was transferred */
};

/* The command to be sent to the network controller message port must
 * be one of the following:
 */

#define NSMC_Shutdown	1	/* Shut down the network; a pointer to an
				   ULONG may be placed in nsm_Data (if the
				   shutdown does not succeed, this is where
				   the number of active clients will be
				   placed). */

#define NSMC_Cancel	2	/* Cancel a shutdown request; this recalls
				   a shutdown message, to which a pointer must
				   be placed in nsm_Data. */

/* Error codes that may be set when a message returns: */

#define NSME_Success	0	/* Command was processed successfully */

#define NSME_Aborted	1	/* Command was aborted */

#define NSME_InUse	2	/* Network is still running, since clients are
				   still using it */

#define NSME_Ignored	3	/* Command was ignored (network may be shutting
				   down right now) */

#define NSME_NotFound	4	/* Shutdown command to be cancelled could not
				   be recalled */

/* The name of the public network controller message port: */
#define NETWORK_CONTROLLER_PORT_NAME "TCP/IP Control"

/* The network controller message port data structure; check the magic
 * cookie before you post a message to it!
 */
struct NetControlPort
{
	struct MsgPort	ncp_Port;
	ULONG		ncp_Magic;
};

/* The magic cookie stored in ncp_Magic: */
#define NCPM_Cookie	0x20040306

/****************************************************************************/

#ifdef __GNUC__
 #ifdef __PPC__
  #pragma pack()
 #endif
#elif defined(__VBCC__)
 #pragma default-align
#endif

/****************************************************************************/

#ifdef __cplusplus
}
#endif /* __cplusplus */

/****************************************************************************/

#endif /* _LIBRARIES_BSDSOCKET_H */
