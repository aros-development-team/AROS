
/****** protocols/arp *******************************************************
*
*   NAME
*       arp - Address Resolution Protocol
*
*   CONFIG
*       Any SANA-II device driver using ARP
*
*   SYNOPSIS
*       #include <sys/socket.h>
*       #include <net/if_arp.h>
*       #include <netinet/in.h>
*
*       s = socket(AF_INET, SOCK_DGRAM, 0);
*
*   DESCRIPTION
*       ARP is a protocol used to dynamically map between Internet
*       Protocol (IP) and hardware addresses. It can be used by most
*       the SANA-II network interface drivers. The current
*       implementation supports only Internet Protocol (and is tested
*       only with Ethernet).  However, ARP is not limited to only that
*       combination.
*
*       ARP caches IP-to-hardware address mappings. When an interface
*       requests a mapping for an address not in the cache, ARP queues
*       the message which requires the mapping and broadcasts a
*       message on the associated network requesting the address
*       mapping. If a response is provided, the new mapping is cached
*       and any pending message is transmitted. ARP will queue at most
*       one packet while waiting for a mapping request to be responded
*       to; only the most recently transmitted packet is kept.
*
*       The address mapping caches are separate for each interface. The
*       amount of mappings in the cache may be specified with an
*       IoctlSocket() request. 
*
*       To facilitate communications with systems which do not use ARP,
*       IoctlSocket() requests are provided to enter and delete entries
*       in the IP-to-Ethernet tables.
*
*   USAGE
*       #include <sys/ioctl.h>
*       #include <sys/socket.h>
*       #include <net/if.h>
*       #include <net/if_arp.h>
*
*       struct arpreq arpreq;
*
*       IoctlSocket(s, SIOCSARP, (caddr_t)&arpreq);
*       IoctlSocket(s, SIOCGARP, (caddr_t)&arpreq);
*       IoctlSocket(s, SIOCDARP, (caddr_t)&arpreq);
*
*       These three IoctlSocket()s take the same structure as an argument.
*       SIOCSARP sets an ARP entry, SIOCGARP gets an ARP entry, and SIOCDARP
*       deletes an ARP entry. These IoctlSocket() requests may be applied to
*       any socket descriptor (s). The arpreq structure contains:
*
*       \* Maximum number of octets in protocol/hw address *\
*       #define MAXADDRARP  16 
*
*       \*
*        * ARP ioctl request. 
*        *\
*       struct arpreq {
*               struct  sockaddr arp_pa;  \* protocol address *\
*               struct  {                 \* hardware address *\
*                 u_char sa_len;         \* actual length + 2 *\
*                 u_char sa_family;             
*                 char   sa_data[MAXADDRARP];           
*               }  arp_ha;              
*               int     arp_flags;                   \* flags *\
*       };
*
*       \*  arp_flags and at_flags field values *\
*       #define ATF_INUSE       0x01          \* entry in use *\
*       #define ATF_COM         0x02       \* completed entry *\
*       #define ATF_PERM        0x04       \* permanent entry *\
*       #define ATF_PUBL        0x08         \* publish entry *\
*
*
*       The interface whose ARP table is manipulated is specified by
*       arp_pa sockaddr. The address family for the arp_pa sockaddr
*       must be AF_INET; for the arp_ha sockaddr it must be AF_UNSPEC.
*       The length of arp_ha must match the length of used hardware
*       address. Maximum length for the hardware address is MAXADDRARP
*       bytes. The only flag bits which may be written are ATF_PERM
*       and ATF_PUBL. ATF_PERM makes the entry permanent if the
*       IoctlSocket() call succeeds. ATF_PUBL specifies that the ARP
*       code should respond to ARP requests for the indicated host
*       coming from other machines.  This allows a host to act as an
*       ARP server which may be useful in convincing an ARP-only
*       machine to talk to a non-ARP machine.
*
*   UNSUPPORTED IN AmiTCP/IP
*
*   AmiTCP/IP EXTENSIONS
*       There is an extension to the standard BSD4.4 ARP ioctl interface to
*       access the contents of the whole ARP mapping cache. (In the BSD4.4
*       the static ARP table is accessed via the /dev/kmem.) The SIOCGARPT
*       ioctl takes the following arptabreq structure as an argument:
*
*       \* 
*        * An AmiTCP/IP specific ARP table ioctl request
*        *\
*       struct arptabreq {
*               struct arpreq atr_arpreq;  \* To identify the interface *\
*               long   atr_size;          \* # of elements in atr_table *\
*               long   atr_inuse;               \* # of elements in use *\
*               struct arpreq *atr_table;
*       };
*
*       The atr_arpreq specifies the used interface. The hardware address
*       for the interface is returned in the arp_ha field of atr_arpreq
*       structure.
*       
*       The SIOCGARPT ioctl reads at most atr_size entries from the cache
*       into the user supplied buffer atr_table, if it is not NULL. Actual
*       amount of returned entries is returned in atr_size. The current
*       amount of cached mappings is returned in the atr_inuse.
*       
*       The SIOCGARPT ioctl has following usage:
*
*       struct arpreq cache[N];
*       struct arptabreq arptab = { N, 0, cache };
*
*       IoctlSocket(s, SIOCGARPT, (caddr_t)&arptabreq);
*
*   DIAGNOSTICS
*       ARP watches passively for hosts impersonating the local host
*       (that  is,  a  host which responds to an ARP mapping request
*       for the local host's address).
*
*       "duplicate IP address a.b.c.d!!"
*       "sent from hardware address: %x:%x:...:%x:%x"
*
*       ARP  has  discovered  another host on the local network
*       which responds to mapping requests for its own Internet
*       address.
*
*   BUGS
*       The ARP is tested only with Ethernet. Other network hardware may
*       require special ifconfig configuration.
*
*   SEE ALSO
*       inet, netutil/arp, netutil/ifconfig, <net/if_arp.h>
*
*       Plummer, Dave, ``An  Ethernet  Address  Resolution  Protocol
*       -or-  Converting Network Protocol Addresses to 48.bit Ether-
*       net Addresses for Transmission on Ethernet  Hardware,''  RFC
*       826,  Network  Information  Center, SRI International, Menlo
*       Park, Calif., November 1982. (Sun 800-1059-10)
*
*****************************************************************************
*
*/

/****** protocols/icmp ******************************************************
*
*   NAME
*       icmp - Internet Control Message Protocol
*   
*   SYNOPSIS
*       #include <sys/socket.h>
*       #include <netinet/in.h>
*   
*       int
*       socket(AF_INET, SOCK_RAW, proto)
*   
*   DESCRIPTION
*       ICMP is the error and control message protocol used by IP and the
*       Internet protocol family.  It may be accessed through a ``raw
*       socket'' for network monitoring and diagnostic functions.  The proto
*       parameter to the socket call to create an ICMP socket is obtained
*       from getprotobyname().  ICMP sockets are connectionless, and are
*       normally used with the sendto() and recvfrom() calls, though the
*       connect() call may also be used to fix the destination for future
*       packets (in which case the recv() and send() socket library calls
*       may be used).
*
*       Outgoing packets automatically have an IP header prepended to them
*       (based on the destination address).  Incoming packets are received
*       with the IP header and options intact.
*   
*   DIAGNOSTICS
*       A socket operation may fail with one of the following errors
*       returned:
*   
*       [EISCONN]        when trying to establish a connection on a socket
*                        which already has one, or when trying to send a
*                        datagram with the destination address specified and
*                        the socket is already connected;
*   
*       [ENOTCONN]       when trying to send a datagram, but no destination
*                        address is specified, and the socket hasn't been
*                        connected;
*   
*       [ENOBUFS]        when the system runs out of memory for an internal
*                        data structure;
*   
*       [EADDRNOTAVAIL]  when an attempt is made to create a socket with a
*                        network address for which no network interface
*                        exists.
*   
*   SEE ALSO
*       bsdsocket.library/send(),  bsdsocket.library/recv(), inet,  ip
*   
*   HISTORY
*       The icmp protocol is originally from 4.3BSD.
*
*****************************************************************************
*
*/

/****** protocols/if ********************************************************
*
*   NAME
*       if - Network Interface to SANA-II devices
*
*   DESCRIPTION
*       Each network interface in the AmiTCP/IP corresponds to a path
*       through which messages may be sent and received.  A network
*       interface usually has a SANA-II device driver associated with it,
*       though the loopback interface, "lo", do not. The network interface
*       in the AmiTCP/IP (sana_softc) is superset of the BSD Unix network
*       interface.
*
*       When the network interface is first time referenced, AmiTCP/IP tries
*       to open the corresponding SANA-II device driver. If successful, a
*       software interface to the SANA-II device is created. The "network/"
*       prefix is added to the SANA-II device name, if needed. Once the
*       interface has acquired its address, it is expected to install a
*       routing table entry so that messages can be routed through it.
*
*       The SANA-II interfaces must be configured before they will allow
*       traffic to flow through them. It is done after the interface is
*       assigned a protocol address with a SIOCSIFADDR ioctl. Some
*       interfaces may use the protocol address or a part of it as their
*       hardware address. On interfaces where the network-link layer address
*       mapping is static, only the network number is taken from the ioctl;
*       the remainder is found in a hardware specific manner. On interfaces
*       which provide dynamic network-link layer address mapping facilities
*       (for example, Ethernets or Arcnets using ARP), the entire address
*       specified in the ioctl is used.
*
*       The following ioctl calls may be used to manipulate network
*       interfaces. Unless specified otherwise, the request takes an ifreq
*       structure as its parameter. This structure has the form
*
*       struct ifreq {
*         char ifr_name[IFNAMSIZ]; \* interface name (eg. "slip.device/0")*\
*         union {
*           struct sockaddr ifru_addr;
*           struct sockaddr ifru_dstaddr;
*           short           ifru_flags;
*         } ifr_ifru;
*       #define ifr_addr    ifr_ifru.ifru_addr                 \* address *\
*       #define ifr_dstaddr ifr_ifru.ifru_dstaddr   \* end of p-to-p link *\
*       #define ifr_flags   ifr_ifru.ifru_flags                  \* flags *\
*       };
*
*       SIOCSIFADDR      Set interface address. Following the address
*                        assignment, the ``initialization'' routine for
*                        the interface is called.
*
*       SIOCGIFADDR      Get interface address.
*
*       SIOCSIFDSTADDR   Set point to point address for interface.
*
*       SIOCGIFDSTADDR   Get point to point address for interface.
*
*       SIOCSIFFLAGS     Set interface flags field. If the interface is
*                        marked down, any processes currently routing
*                        packets through the interface are notified.
*
*       SIOCGIFFLAGS     Get interface flags.
*
*       SIOCGIFCONF      Get interface configuration list. This request
*                        takes an ifconf structure (see below) as a
*                        value-result parameter. The ifc_len field should be
*                        initially set to the size of the buffer pointed to
*                        by ifc_buf. On return it will contain the length,
*                        in bytes, of the configuration list.
*
*       \*
*        * Structure used in SIOCGIFCONF request.
*        * Used to retrieve interface configuration
*        * for machine (useful for programs which
*        * must know all networks accessible).
*        *\
*       struct ifconf {
*         int  ifc_len;                      \* size of associated buffer *\
*         union {
*           caddr_t       ifcu_buf;
*           struct ifreq *ifcu_req;
*         } ifc_ifcu;
*       #define ifc_buf ifc_ifcu.ifcu_buf               \* buffer address *\
*       #define ifc_req ifc_ifcu.ifcu_req \* array of structures returned *\
*       };
*
*
*   UNSUPPORTED IN AmiTCP/IP
*       These standard BSD ioctl codes are not currently supported:
*
*       SIOCADDMULTI     Enable a multicast address for the interface. 
*
*       SIOCDELMULTI     Disable a previously set multicast address.
*
*       SIOCSPROMISC     Toggle promiscuous mode.
*
*   AmiTCP/IP EXTENSIONS
*       The following ioctls are used to configure protocol and hardware
*       specific properties of a sana_softc interface. They are used in the
*       AmiTCP/IP only.
*
*       SIOCSSANATAGS    Set SANA-II specific properties with a tag list.
*
*       SIOCGSANATAGS    Get SANA-II specific properties into a
*                        wiretype_parameters structure and a user tag list.
*
*       struct wiretype_parameters
*       {
*         ULONG  wiretype;               \* the wiretype of the interface *\
*         WORD   flags;                                      \* iff_flags *\
*         struct TagItem *tags;                 \* tag list user provides *\
*       };
*       
*   SEE ALSO
*       arp, lo, netutil/arp, netutil/ifconfig, <sys/ioctl.h>, <net/if.h>, 
*       <net/sana2tags.h>
*
*****************************************************************************
*
*/

/****** protocols/inet ******************************************************
*
*   NAME
*       inet - Internet protocol family
*   
*   SYNOPSIS
*       #include <sys/types.h>
*       #include <netinet/in.h>
*   
*   DESCRIPTION
*       The Internet protocol family implements a collection of protocols
*       which are centered around the Internet Protocol (IP) and which share
*       a common address format.  The Internet family provides protocol
*       support for the SOCK_STREAM, SOCK_DGRAM, and SOCK_RAW socket types.
*   
*   PROTOCOLS
*       The Internet protocol family is comprised of the Internet Protocol
*       (IP), the Address Resolution Protocol (ARP), the Internet Control
*       Message Protocol (ICMP), the Transmission Control Protocol (TCP),
*       and the User Datagram Protocol (UDP).
*   
*       TCP is used to support the SOCK_STREAM abstraction while UDP is used
*       to support the SOCK_DGRAM abstraction; (SEE ALSO tcp, SEE ALSO udp).
*       A raw interface to IP is available by creating an Internet socket of
*       type SOCK_RAW; (SEE ALSO ip).  ICMP is used by the kernel to handle
*       and report errors in protocol processing.  It is also accessible to
*       user programs; (SEE ALSO icmp).  ARP is used to translate 32-bit IP
*       addresses into varying length hardware addresses; (SEE ALSO arp).
*   
*       The 32-bit IP address is divided into network number and host number
*       parts.  It is frequency-encoded; the most significant bit is zero in
*       Class A addresses, in which the high-order 8 bits are the network
*       number.  Class B addresses have their high order two bits set to 10
*       and use the highorder 16 bits as the network number field.  Class C
*       addresses have a 24-bit network number part of which the high order
*       three bits are 110.  Sites with a cluster of local networks may
*       chose to use a single network number for the cluster; this is done
*       by using subnet addressing.  The local (host) portion of the address
*       is further subdivided into subnet number and host number parts.
*       Within a subnet, each subnet appears to be an individual network;
*       externally, the entire cluster appears to be a single, uniform
*       network requiring only a single routing entry.  Subnet addressing is
*       enabled and examined by the following ioctl commands on a datagram
*       socket in the Internet domain; they have the same form as the
*       SIOCIFADDR (SEE ALSO if) command.
*   
*       SIOCSIFNETMASK      Set interface network mask.  The network mask
*                           defines the network part of the address; if it
*                           contains more of the address than the address
*                           type would indicate, then subnets are in use.
*   
*       SIOCGIFNETMASK      Get interface network mask.
*   
*   ADDRESSING
*       IP addresses are four byte quantities, stored in network byte order
*       (the native Amiga byte order)
*   
*       Sockets in the Internet protocol family  use  the  following
*       addressing structure:
*            struct sockaddr_in {
*                 short     sin_family;
*                 u_short   sin_port;
*                 struct    in_addr sin_addr;
*                 char sin_zero[8];
*            };
*   
*       Functions in bsdsocket.library are provided to manipulate structures
*       of this form.
*   
*       The sin_addr field of the sockaddr_in structure specifies a local or
*       remote IP address.  Each network interface has its own unique IP
*       address.  The special value INADDR_ANY may be used in this field to
*       effect "wildcard" matching.  Given in a bind() call, this value
*       leaves the local IP address of the socket unspecified, so that the
*       socket will receive connections or messages directed at any of the
*       valid IP addresses of the system.  This can prove useful when a
*       process neither knows nor cares what the local IP address is or when
*       a process wishes to receive requests using all of its network
*       interfaces.  The sockaddr_in structure given in the bind() call must
*       specify an in_addr value of either IPADDR_ANY or one of the system's
*       valid IP addresses.  Requests to bind any other address will elicit
*       the error EADDRNOTAVAIL. When a connect() call is made for a socket
*       that has a wildcard local address, the system sets the sin_addr
*       field of the socket to the IP address of the network interface that
*       the packets for that connection are routed via.
*   
*       The sin_port field of the sockaddr_in structure specifies a port
*       number used by TCP or UDP. The local port address specified in a
*       bind() call is restricted to be greater than IPPORT_RESERVED
*       (defined in <netinet/in.h>) unless the creating process is running
*       as the super-user, providing a space of protected port numbers.  In
*       addition, the local port address must not be in use by any socket of
*       same address family and type.  Requests to bind sockets to port
*       numbers being used by other sockets return the error EADDRINUSE.  If
*       the local port address is specified as 0, then the system picks a
*       unique port address greater than IPPORT_RESERVED.  A unique local
*       port address is also picked when a socket which is not bound is used
*       in a connect() or send() call.  This allows programs which do not
*       care which local port number is used to set up TCP connections by
*       sim- ply calling socket() and then connect(), and to send UDP
*       datagrams with a socket() call followed by a send() call.
*   
*       Although this implementation restricts sockets to unique local port
*       numbers, TCP allows multiple simultaneous connections involving the
*       same local port number so long as the remote IP addresses or port
*       numbers are different for each connection.  Programs may explicitly
*       override the socket restriction by setting the SO_REUSEADDR socket
*       option with setsockopt (see getsockopt()).
*   
*   SEE ALSO
*       bsdsocket.library/bind(), bsdsocket.library/connect(),
*       bsdsocket.library/getsockopt(), bsdsocket.library/IoctlSocket(),
*       bsdsocket.library/send(), bsdsocket.library/socket(),
*       bsdsocket.library/gethostent(), bsdsocket.library/getnetent(),
*       bsdsocket.library/getprotoent(), bsdsocket.library/getservent(),
*       bsdsocket.library/inet_addr(), arp, icmp, ip, tcp, udp
*   
*       Network Information Center, DDN Protocol Handbook (3 vols.),
*       Network  Information  Center, SRI International, Menlo Park,
*       Calif., 1985.
*       A AmiTCP/IP Interprocess Communication Primer
*   
*   WARNING
*       The Internet protocol support is subject to change as the Internet
*       protocols develop.  Users should not depend on details of the
*       current implementation, but rather the services exported.
*
*****************************************************************************
*
*/

/****** protocols/ip ********************************************************
*
*   NAME
*       ip - Internet Protocol
*   
*   SYNOPSIS
*       #include <sys/socket.h>
*       #include <netinet/in.h>
*   
*       int
*       socket(AF_INET, SOCK_RAW, proto)
*   
*   DESCRIPTION
*       IP is the transport layer protocol used by the Internet protocol
*       family.  Options may be set at the IP level when using higher-level
*       protocols that are based on IP (such as TCP and UDP). It may also be
*       accessed through a ``raw socket'' when developing new protocols, or
*       special purpose applica- tions.
*   
*       A single generic option is supported at the IP level, IP_OPTIONS,
*       that may be used to provide IP options to be transmitted in the IP
*       header of each outgoing packet.  Options are set with setsockopt()
*       and examined with getsockopt().  The format of IP options to be sent
*       is that specified by the IP protocol specification, with one
*       exception: the list of addresses for Source Route options must
*       include the first-hop gateway at the beginning of the list of
*       gateways.  The first-hop gateway address will be extracted from the
*       option list and the size adjusted accordingly before use.  IP
*       options may be used with any socket type in the Internet family.
*   
*       Raw IP sockets are connectionless, and are normally used with the
*       sendto and recvfrom calls, though the connect() call may also be
*       used to fix the destination for future packets (in which case the
*       recv() and send() system calls may be used).
*   
*       If proto is 0, the default protocol IPPROTO_RAW is used for outgoing
*       packets, and only incoming packets destined for that protocol are
*       received.  If proto is non-zero, that protocol number will be used
*       on outgoing packets and to filter incoming packets.
*   
*       Outgoing packets automatically have an IP header prepended to them
*       (based on the destination address and the protocol number the socket
*       is created with).  Incoming packets are received with IP header and
*       options intact.
*   
*   DIAGNOSTICS
*       A socket operation may fail with one of the following errors
*       returned:
*   
*       [EISCONN]        when trying to establish a connection on a socket
*                        which already has one, or when trying to send a
*                        datagram with the destination address specified and
*                        the socket is already connected;
*   
*       [ENOTCONN]       when trying to send a datagram, but no destination
*                        address is specified, and the socket hasn't been
*                        connected;
*   
*       [ENOBUFS]        when the system runs out of memory for an internal
*                        data structure;
*   
*       [EADDRNOTAVAIL]  when an attempt is made to create a socket with a
*                        network address for which no network interface
*                        exists.
*   
*       The following errors specific to IP may occur when setting or
*       getting IP options:
*   
*       [EINVAL]         An unknown socket option name was given.
*   
*       [EINVAL]         The IP option field was improperly formed; an
*                        option field was shorter than the minimum value or
*                        longer than the option buffer provided.
*   
*   SEE ALSO
*       bsdsocket.library/getsockopt(), bsdsocket.library/send(),
*       bsdsocket.library/recv(), icmp, inet
*
*   HISTORY
*       The ip protocol appeared in 4.2BSD.
*
*****************************************************************************
*
*/

/****** protocols/lo ********************************************************
*
*   NAME
*       lo - Software Loopback Network Interface
*
*   SYNOPSIS
*       pseudo-device
*       loop
*
*   DESCRIPTION
*       The loop interface is a software loopback mechanism which may be
*       used for performance analysis, software testing, and/or local
*       communication.  There is no SANA-II interface associated with lo.
*       As with other network interfaces, the loopback interface must have
*       network addresses assigned for each address family with which it is
*       to be used.  These addresses may be set or changed with the
*       SIOCSIFADDR ioctl. The loopback interface should be the last
*       interface configured, as protocols may use the order of
*       configuration as an indication of priority.  The loopback should
*       never be configured first unless no hardware interfaces exist.
*
*   DIAGNOSTICS
*       "lo%d: can't handle af%d."
*       The interface was handed a message with ad- dresses formatted in an
*       unsuitable address family; the packet was dropped.
*
*   SEE ALSO
*       inet, if, netutil/ifconfig
*
*   BUGS 
*       Older BSD Unix systems enabled the loopback interface
*       automatically, using a nonstandard Internet address (127.1).  Use
*       of that address is now discouraged; a reserved host address for the
*       local network should be used instead.
*
*****************************************************************************
*
*/

/****** protocols/routing ***************************************************
*
*   NAME
*       routing - system supporting for local network packet routing
*   
*   DESCRIPTION
*       The network facilities provided general packet routing,
*       leaving routing table maintenance to applications processes.
*   
*       A simple set of data structures comprise a ``routing table''
*       used in selecting the appropriate network interface when
*       transmitting packets.  This table contains a single entry for
*       each route to a specific network or host.  A user process, the
*       routing daemon, maintains this data base with the aid of two
*       socket specific ioctl commands, SIOCADDRT and SIOCDELRT.
*       The commands allow the addition and deletion of a single
*       routing table entry, respectively.  Routing table
*       manipulations may only be carried out by super-user.
*   
*       A routing table entry has the following form, as defined  in
*       <net/route.h>:
*            struct rtentry {
*                 u_long    rt_hash;
*                 struct    sockaddr rt_dst;
*                 struct    sockaddr rt_gateway;
*                 short     rt_flags;
*                 short     rt_refcnt;
*                 u_long    rt_use;
*                 struct    ifnet *rt_ifp;
*            };
*       with rt_flags defined from:
*         #define   RTF_UP         0x1              \* route usable *\
*         #define   RTF_GATEWAY    0x2  \* destination is a gateway *\
*         #define   RTF_HOST  0x4     \* host entry (net otherwise) *\
*   
*       Routing table entries come in three flavors: for a specific
*       host, for all hosts on a specific network, for any destination
*       not matched by entries of the first two types (a wildcard
*       route).  When the system is booted, each network interface
*       autoconfigured installs a routing table entry when it wishes
*       to have packets sent through it.  Normally the interface
*       specifies the route through it is a ``direct'' connection to
*       the destination host or network.  If the route is direct, the
*       transport layer of a protocol family usually requests the
*       packet be sent to the same host specified in the packet.
*       Otherwise, the interface may be requested to address the
*       packet to an entity different from the eventual recipient
*       (that is, the packet is forwarded).
*   
*       Routing table entries installed by a user process may not
*       specify the hash, reference count, use, or interface fields;
*       these are filled in by the routing routines.  If a route is in
*       use when it is deleted (rt_refcnt is non-zero), the resources
*       associated with it will not be reclaimed until all references
*       to it are removed.
*   
*       The routing code returns EEXIST if requested to duplicate an
*       existing entry, ESRCH if requested to delete a non-existent
*       entry, or ENOBUFS if insufficient resources were available to
*       install a new route.
*   
*       The rt_use field contains the number of packets sent along the
*       route.  This value is used to select among multiple routes to
*       the same destination.  When multiple routes to the same
*       destination exist, the least used route is selected.
*   
*       A wildcard routing entry is specified with a zero destination
*       address value.  Wildcard routes are used only when the system
*       fails to find a route to the destination host and network.
*       The combination of wildcard routes and routing redirects can
*       provide an economical mechanism for routing traffic.
*   
*   SEE ALSO
*       bsdsocket.library/IoctlSocket(), netutil/route
*   
*****************************************************************************
*
*/

/****** protocols/tcp *******************************************************
*
*   NAME
*       tcp - Internet Transmission Control Protocol
*   
*   SYNOPSIS
*       #include <sys/socket.h>
*       #include <netinet/in.h>
*   
*       int
*       socket(AF_INET, SOCK_STREAM, 0)
*   
*   DESCRIPTION
*       The TCP protocol provides reliable, flow-controlled, two-way
*       transmission of data.  It is a byte-stream protocol used to support
*       the SOCK_STREAM abstraction.  TCP uses the standard Internet address
*       format and, in addition, provides a per-host collection of ``port
*       addresses''. Thus, each address is composed of an Internet address
*       specifying the host and network, with a specific TCP port on the
*       host identifying the peer entity.
*   
*       Sockets utilizing the tcp protocol are either ``active'' or
*       ``passive''.  Active sockets initiate connections to passive
*       sockets.  By default TCP sockets are created active; to create a
*       passive socket the listen() bsdsocket.library function call must be
*       used after binding the socket with the bind() bsdsocket.library
*       function call.  Only passive sockets may use the accept() call to
*       accept incoming connections.  Only active sockets may use the
*       connect() call to initiate connections.
*   
*       Passive sockets may ``underspecify'' their location to match
*       incoming connection requests from multiple networks.  This
*       technique, termed ``wildcard addressing'', allows a single server to
*       provide service to clients on multiple networks.  To create a socket
*       which listens on all networks, the Internet address INADDR_ANY must
*       be bound.  The TCP port may still be specified at this time; if the
*       port is not specified the bsdsocket.library function will assign
*       one.  Once a connection has been established the socket's address is
*       fixed by the peer entity's location.  The address assigned the
*       socket is the address associated with the network interface through
*       which packets are being transmitted and received.  Normally this
*       address corresponds to the peer entity's network.
*   
*       TCP supports one socket option which is set with setsockopt() and
*       tested with getsockopt().  Under most circumstances, TCP sends data
*       when it is presented; when outstanding data has not yet been
*       acknowledged, it gathers small amounts of output to be sent in a
*       single packet once an acknowledgement is received.  For a small
*       number of clients, such as X Window System functions that
*       send a stream of mouse events which receive no replies, this
*       packetization may cause significant delays.  Therefore, TCP provides
*       a boolean option, TCP_NODELAY (from <netinet/tcp.h>, to defeat this
*       algorithm.  The option level for the setsockopt call is the protocol
*       number for TCP, available from getprotobyname().
*   
*       Options at the IP transport level may be used with TCP; SEE ALSO ip.
*       Incoming connection requests that are source-routed are noted, and
*       the reverse source route is used in responding.
*   
*   DIAGNOSTICS
*       A socket operation may fail with one of the following errors
*       returned:
*   
*       [EISCONN]        when trying to establish a connection on a socket
*                        which already has one;
*   
*       [ENOBUFS]        when the AmiTCP/IP runs out of memory for an internal
*                        data structure;
*   
*       [ETIMEDOUT]      when a connection was dropped due to excessive
*                        retransmissions;
*   
*       [ECONNRESET]     when the remote peer forces the connection to be
*                        closed;
*   
*       [ECONNREFUSED]   when the remote peer actively refuses connection
*                        establishment (usually because no process is
*                        listening to the port);
*   
*       [EADDRINUSE]     when an attempt is made to create a socket with a
*                        port which has already been allocated;
*   
*       [EADDRNOTAVAIL]  when an attempt is made to create a socket with a
*                        network address for which no network interface
*                        exists.
*   
*   SEE ALSO 
*       bsdsocket.library/getsockopt(), bsdsocket.library/socket(),
*       bsdsocket.library/bind(), bsdsocket.library/listen(),
*       bsdsocket.library/accept(), bsdsocket.library/connect(), inet,
*       ip, <sys/socket.h>, <netinet/tcp.h>, <netinet/in.h>
*   
*   HISTORY
*       The tcp protocol stack appeared in 4.2BSD.
*
*****************************************************************************
*
*/

/****** protocols/udp *******************************************************
*
*   NAME
*       udp - Internet User Datagram Protocol
*   
*   SYNOPSIS
*       #include <sys/socket.h>
*       #include <netinet/in.h>
*   
*       int
*       socket(AF_INET, SOCK_DGRAM, 0)
*   
*   DESCRIPTION
*       UDP is a simple, unreliable datagram protocol which is used to
*       support the SOCK_DGRAM abstraction for the Internet protocol family.
*       UDP sockets are connectionless, and are normally used with the
*       sendto() and recvfrom() calls, though the connect() call may also be
*       used to fix the destination for future packets (in which case the
*       recv() and send() function calls may be used).
*   
*       UDP address formats are identical to those used by TCP. In
*       particular UDP provides a port identifier in addition to the normal
*       Internet address format.  Note that the UDP port space is separate
*       from the TCP port space (i.e. a UDP port may not be ``connected'' to
*       a TCP port). In addition broadcast packets may be sent (assuming the
*       underlying network supports this) by using a reserved ``broadcast
*       address''; this address is network interface dependent.
*   
*       Options at the IP transport level may be used with UDP; SEE ALSO ip.
*   
*   DIAGNOSTICS
*       A socket operation may fail with one of the following errors
*       returned:
*   
*       [EISCONN]        when trying to establish a connection on a socket
*                        which already has one, or when trying to send a
*                        datagram with the destination address specified and
*                        the socket is already connected;
*   
*       [ENOTCONN]       when trying to send a datagram, but no destination
*                        address is specified, and the socket hasn't been
*                        connected;
*   
*       [ENOBUFS]        when the system runs out of memory for an
*                        internal data structure;
*   
*       [EADDRINUSE]     when an attempt is made to create a socket with a
*                        port which has already been allocated;
*   
*       [EADDRNOTAVAIL]  when an attempt is made to create a socket with a
*                        network address for which no network interface
*                        exists.
*   
*   SEE ALSO
*       bsdsocket.library/getsockopt(), bsdsocket.library/recv(),
*       bsdsocket.library/send(), bsdsocket.library/socket(), inet, ip
*   
*   HISTORY
*       The udp protocol appeared in 4.2BSD.
*
*****************************************************************************
*
*/
