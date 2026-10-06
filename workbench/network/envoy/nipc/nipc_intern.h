/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: nipc.library internals.

          Everything that touches the network runs in one process, the
          supervisor ("NIPC"), which owns the sockets of the TCP/IP stack
          (bsdsocket.library bases are per task). Library functions called
          by applications work on shared structures under NIPCBase->Sem and
          hand network work to the supervisor through its request port. The
          wire formats and behaviour follow the specifications in
          re/spec/nipc-*.md of the Envoy project.
*/
#ifndef NIPC_INTERN_H
#define NIPC_INTERN_H

#include <exec/types.h>
#include <exec/libraries.h>
#include <exec/lists.h>
#include <exec/ports.h>
#include <exec/semaphores.h>
#include <dos/dos.h>
#include <devices/timer.h>
#include <utility/tagitem.h>
#include <utility/hooks.h>
#include <aros/debug.h>

#include <envoy/nipc.h>
#include <envoy/nipclowlevel.h>
#include <envoy/errors.h>

#include LC_LIBDEFS_FILE

#ifndef MAKE_ID
#define MAKE_ID(a,b,c,d) ((ULONG)(a)<<24 | (ULONG)(b)<<16 | (ULONG)(c)<<8 | (ULONG)(d))
#endif

#define DEBUG_NAME_STR          "[nipc.library]"
#define NLOG(...)               do { D(bug(__VA_ARGS__);) } while (0)

#define NIPC_VERSION            50
#define NIPC_REVISION           0
#define NIPC_NAMESIZE           64          /* entity names: 63 characters + NUL */
#define NIPC_HOSTSIZE           128
#define NIPC_RESOLVER_PORT      1
#define NIPC_FIRST_DYNPORT      1024
#define NIPC_INQUIRY_PORT       376
#define NIPC_IPPROTO_RDP        27
#define NIPC_HEADER_SIZE        18          /* transaction fragment header */
#define NIPC_TICK_US            100000      /* supervisor tick: 0.1 s */
#define NIPC_TICKS_PER_HB       10          /* heartbeat: 1 s */

/* big-endian access helpers (the wire is big-endian) */
static inline UWORD nipc_get16(const UBYTE *p) { return (UWORD)((p[0] << 8) | p[1]); }
static inline ULONG nipc_get32(const UBYTE *p) { return ((ULONG)p[0] << 24) | ((ULONG)p[1] << 16) | ((ULONG)p[2] << 8) | p[3]; }
static inline void nipc_put16(UBYTE *p, UWORD v) { p[0] = v >> 8; p[1] = v; }
static inline void nipc_put32(UBYTE *p, ULONG v) { p[0] = v >> 24; p[1] = v >> 16; p[2] = v >> 8; p[3] = v; }

struct NIPCBase;
struct RdpConn;
struct Entity;

/*------------------------------------------------------------------------*/
/* Entities. An entity is an exec message port (so that local transactions
 * are plain messages) with nipc state behind it. */

#define ENTF_PUBLIC             (1 << 0)    /* findable                                        */
#define ENTF_OWNSIGNAL          (1 << 1)    /* signal bit allocated by the library             */
#define ENTF_LINK               (1 << 2)    /* link to a remote entity (client or server side) */
#define ENTF_SERVERLINK         (1 << 3)    /* link created by the resolver for a client       */
#define ENTF_DELETED            (1 << 4)    /* going away; refused by lookups                  */
#define ENTF_CONNDEAD           (1 << 5)    /* link: its connection has died                   */

struct Entity
{
    struct MsgPort      Port;               /* arrival port; mp_SigTask = owner                */
    struct MinNode      Node;               /* NIPCBase->Entities, or owner's Links            */
    ULONG               Flags;              /* ENTF_#?                                         */
    LONG                UseCount;           /* references by finders, links, in-service requests */
    UWORD               TimeoutLinks;       /* ENT_TimeoutLinks seconds, 0 = off               */
    UWORD               IdleSeconds;        /* server link: seconds since the last request     */
    char                Name[NIPC_NAMESIZE];
    char                HostName[NIPC_HOSTSIZE];    /* links: the peer's self-name             */
    struct Entity       *Owner;             /* link: the local entity it belongs to            */
    struct MinList      Links;              /* local entity: client links found from it; public entity: server links */
    struct RdpConn      *Conn;              /* link: its RDP connection                        */
    struct MinList      Pending;            /* client link: transactions awaiting a response   */
    struct MinList      Outgoing;           /* link: transactions whose fragments are being sent */
    struct MinList      RxQueue;            /* link: received fragments awaiting reassembly    */
    APTR                Finder;             /* client link: FindEntity in progress             */
    struct NIPCBase     *Base;
};

/* The private part of a transaction (allocated behind struct Transaction) */
struct PrivTransaction
{
    struct Transaction  Pub;
    struct MinNode      Node;               /* on a link's Pending or Outgoing list            */
    ULONG               Magic;
    ULONG               TxOffset;           /* bytes of data already fragmented                */
    UWORD               TxFragNo;           /* next fragment number                            */
    UWORD               Op;                 /* supervisor request type while queued            */
    UWORD               Timer;              /* remaining seconds (trans_Timeout copy)          */
    UWORD               RxFragNo;           /* reassembly: expected fragment number            */
};
#define PRIVTRANS_MAGIC         MAKE_ID('N','T','r','n')
#define PRIVTRANS(t)            ((struct PrivTransaction *)(t))

/*------------------------------------------------------------------------*/
/* Supervisor requests. Library functions send these to NIPCBase->SuperPort
 * and wait for the reply. */

#define SREQ_FINDENTITY         1       /* remote FindEntity: resolve, resolver exchange, connect */
#define SREQ_LOSEENTITY         2       /* close a client link                                   */
#define SREQ_DELETEENTITY       3       /* a public entity goes away: reset its server links     */
#define SREQ_TRANSACT           4       /* send a request on a client link (BeginTransaction)    */
#define SREQ_REPLY              5       /* send a response on a server link (ReplyTransaction)   */
#define SREQ_ABORT              6       /* abort a pending remote transaction                    */
#define SREQ_PING               7       /* PingEntity on a link                                  */
#define SREQ_INQUIRY            8       /* NIPCInquiryA                                          */
#define SREQ_INQUIRYABORT       9       /* NIPCInquiryA with maxTime 0                           */
#define SREQ_QUIT               10      /* last CloseLibrary                                     */
#define SREQ_RECONFIG           11      /* prefs changed                                         */
#define SREQ_GETMTU             12      /* NIPCTAG_GetMTUforIP                                   */

struct SuperReq
{
    struct Message      Msg;
    UWORD               Type;
    UWORD               Pad;
    struct Entity       *Entity;            /* source / link                                   */
    struct Entity       *Result;            /* FINDENTITY: the link                            */
    CONST_STRPTR        Host;               /* FINDENTITY                                      */
    CONST_STRPTR        Name;               /* FINDENTITY                                      */
    ULONG               Error;              /* FINDENTITY detail error, GETMTU result          */
    ULONG               Value;              /* PING: maxTime (us) -> result; GETMTU: address   */
    struct Transaction  *Trans;             /* TRANSACT/REPLY/ABORT                            */
    struct Hook         *Hook;              /* INQUIRY                                         */
    ULONG               MaxTime;            /* INQUIRY                                         */
    ULONG               MaxResponses;       /* INQUIRY                                         */
    struct TagItem      *Tags;              /* INQUIRY                                         */
    struct Task         *Caller;
    APTR                Private;            /* supervisor state while in progress              */
};

/*------------------------------------------------------------------------*/
/* RDP (re/spec/nipc-rdp.md) */

#define RDP_STATE_LISTEN        1
#define RDP_STATE_SYNSENT       2
#define RDP_STATE_SYNRCVD       3
#define RDP_STATE_OPEN          4
#define RDP_STATE_CLOSED        5

#define RDP_HDRLEN              18
#define RDP_SYNHDRLEN           24
#define RDP_FLAG_SYN            0x80
#define RDP_FLAG_ACK            0x40
#define RDP_FLAG_EAK            0x20
#define RDP_FLAG_RST            0x10
#define RDP_FLAG_NUL            0x08
#define RDP_VERSION             2

struct RdpSeg
{
    struct MinNode      Node;
    ULONG               Seq;
    ULONG               Len;                /* whole segment (header + data)                   */
    UWORD               Retries;
    UWORD               Countdown;          /* heartbeats until retransmission                 */
    ULONG               RtoBase;            /* RTO when first sent (1/1024 s)                  */
    struct timeval      SentAt;
    BOOL                Retransmitted;
    UBYTE               Data[0];            /* the segment as sent                             */
};

typedef void (*RdpDataFunc)(struct RdpConn *conn, UBYTE *data, ULONG len, APTR userdata);
typedef void (*RdpStatusFunc)(struct RdpConn *conn, APTR userdata);

struct RdpConn
{
    struct MinNode      Node;               /* NIPCBase->Conns                                 */
    UWORD               State;
    UWORD               LocalPort;
    UWORD               RemotePort;
    UWORD               Pad;
    ULONG               LocalIP;
    ULONG               RemoteIP;
    BOOL                Passive;            /* created by a passive open                       */
    BOOL                Clone;              /* listener spawns a connection per SYN            */
    BOOL                AckPending;
    BOOL                InCallback;
    BOOL                FreeDeferred;
    BOOL                CloseDeferred;      /* RdpClose() from inside a callback: after the ACK   */
    ULONG               SndNxt, SndUna, RcvCur, RcvDlv, RcvAcked;
    UWORD               SndLimit, Out, LocMax, LocMss, Mss;
    ULONG               Rto;                /* 1/1024 s                                        */
    ULONG               Sa, Sv;             /* RTT estimator state                             */
    UWORD               ConnCountdown;      /* heartbeats until handshake retransmission       */
    UWORD               ConnRetries;
    UBYTE               IdleCount;
    UBYTE               LastCtlFlags;       /* flags of the last control segment, for repeats  */
    ULONG               Bps;                /* interface bit rate                              */
    ULONG               Mtu;
    struct MinList      TxQueue;            /* unacknowledged data segments (struct RdpSeg)    */
    struct MinList      RxQueue;            /* out-of-order received segments (struct RdpSeg)  */
    RdpDataFunc         DataIn;
    RdpStatusFunc       Status;
    APTR                UserData;
    struct NIPCBase     *Base;
};

/*------------------------------------------------------------------------*/
/* Inquiry (re/spec/nipc-resolver-inquiry-config.md) */

#define INQ_TYPE_REQUEST        100
#define INQ_TYPE_REPLY          101
#define INQ_TYPE_TRANSIT        102
#define INQ_TYPE_REALMLIST      103
#define INQ_HDRLEN              16

struct Inquiry                              /* a pending query of this host                    */
{
    struct MinNode      Node;
    struct SuperReq     *Req;               /* the application's request (replied at the end)  */
    struct Hook         *Hook;
    struct Task         *Caller;
    UWORD               QueryID;
    UWORD               Remaining;          /* seconds                                         */
    UWORD               Responses;          /* maxResponses left                               */
    UWORD               Type;
    UBYTE               *Packet;            /* the request as sent                             */
    ULONG               PacketLen;
    /* internal (FindEntity name lookup) */
    void                (*Done)(struct NIPCBase *, struct Inquiry *, ULONG ipaddr);
    APTR                DoneData;
    ULONG               ResultIP;
};

struct InqReply                             /* a reply we owe, after the random delay          */
{
    struct MinNode      Node;
    ULONG               DestIP;
    UWORD               DelayTicks;
    UWORD               Len;
    UBYTE               Data[0];
};

/* Realm table entry (from nipc.prefs NLRM/NRRM) */
struct Realm
{
    struct MinNode      Node;
    BOOL                Local;
    ULONG               Address;            /* local: directed broadcast of the network; remote: server */
    char                Name[64];
};

/*------------------------------------------------------------------------*/
/* Interfaces as seen through the stack */
struct Iface
{
    struct MinNode      Node;
    char                Name[16];
    ULONG               Address;
    ULONG               Netmask;
    ULONG               Broadcast;
    ULONG               Mtu;
    ULONG               Bps;
    BOOL                Loopback;
};

/*------------------------------------------------------------------------*/
/* Events (timer API) */
struct NIPCEvent
{
    struct Message      Msg;                /* mn_ReplyPort = event port                       */
    ULONG               Code;
    APTR                Data;
    struct timeval      Due;                /* absolute, while pending                         */
    struct timeval      Scheduled;
    struct MinNode      Node;               /* NIPCBase->Events while pending                  */
    UWORD               State;
};
#define EVENT_IDLE              0
#define EVENT_PENDING           1
#define EVENT_DELIVERED         2

/*------------------------------------------------------------------------*/
/* Configuration (nipc.prefs HOST chunk) */
struct NipcConfig
{
    char                HostName[64];
    char                RealmName[64];
    char                Owner[32];
    ULONG               RealmServer;
    BOOL                UseRealmServer;
    BOOL                IsRealmServer;
    BOOL                Gateway;
};

/* RDP tunables (NIPCControlA) */
struct RdpTunables
{
    ULONG   InactivityCheck, InactivityLimit, TransmitRetries, TransmitMinTO, TransmitMaxTO,
            InitialRoundTripTO, ConnectTO, ConnectRetries;
};

/*------------------------------------------------------------------------*/
struct NIPCBase
{
    struct Library              LibNode;
    BPTR                        SegList;
    struct Library              *nipc_DOSBase;
    struct Library              *nipc_UtilityBase;

    struct SignalSemaphore      OpenSem;        /* serialises open/close                       */
    struct SignalSemaphore      Sem;            /* entities, transactions, events, config      */
    struct MinList              Entities;       /* all entities and links                      */
    struct MinList              Events;         /* pending timer events                        */
    ULONG                       Sequence;       /* trans_Sequence counter                      */
    UWORD                       NextPort;       /* dynamic RDP port counter                    */
    UWORD                       NextQueryID;

    /* supervisor */
    struct Process              *Super;
    struct MsgPort              *SuperPort;     /* requests from library functions             */
    BOOL                        SuperOK;        /* start-up succeeded                          */
    struct Task                 *Closer;        /* task waiting for shut-down                  */

    /* supervisor-private state (only touched by the supervisor) */
    struct Library              *SocketBase;
    struct Library              *nipc_TimerBase;
    struct MsgPort              *TimerPort;
    struct timerequest          *TimerReq;
    struct MinList              Conns;          /* struct RdpConn                              */
    struct MinList              Ifaces;         /* struct Iface                                */
    struct MinList              Inquiries;      /* struct Inquiry                              */
    struct MinList              InqReplies;     /* struct InqReply                             */
    struct MinList              Finders;        /* struct Finder (nipc_link.c)                 */
    struct MinList              Pings;          /* struct PingReq                              */
    struct MinList              Realms;         /* struct Realm                                */
    LONG                        RawSock;        /* protocol 27                                 */
    LONG                        UdpSock;        /* *:376                                       */
    struct RdpConn              *Resolver;      /* cloning listener on port 1                  */
    ULONG                       Ticks;
    struct timeval              LastTick;
    struct NotifyRequest        *PrefsNotify;
    ULONG                       NotifySigBit;

    struct NipcConfig           Config;
    struct RdpTunables          Rdp;
    ULONG                       DefaultTTL, DefaultTOS, FragmentTO, ArpResolveTO, ArpResolveRetries, ArpEntryTO;
};

#define DOSBase                 ((struct DosLibrary *)NIPCBase->nipc_DOSBase)
#define UtilityBase             (NIPCBase->nipc_UtilityBase)

/* nipc_init.c */
extern BOOL StartSupervisor(struct NIPCBase *NIPCBase);
extern void StopSupervisor(struct NIPCBase *NIPCBase);

/* nipc_entity.c */
extern struct Entity *AllocEntity(struct NIPCBase *NIPCBase, CONST_STRPTR name, ULONG flags);
extern void FreeEntity(struct NIPCBase *NIPCBase, struct Entity *e);
extern struct Entity *FindPublicEntity(struct NIPCBase *NIPCBase, CONST_STRPTR name);  /* under Sem */
extern void ReleaseEntity(struct NIPCBase *NIPCBase, struct Entity *e);               /* UseCount-- and deferred delete */
extern void LocalHostName(struct NIPCBase *NIPCBase, STRPTR buf, ULONG size);

/* nipc_trans.c */
extern void ReturnTransaction(struct NIPCBase *NIPCBase, struct Transaction *t, ULONG error);
extern struct Transaction *AllocRequestTransaction(struct NIPCBase *NIPCBase, ULONG reqlen, ULONG resplen, BOOL inplace);
extern LONG SendSuperReq(struct NIPCBase *NIPCBase, struct SuperReq *req);    /* synchronous */
extern BOOL PostSuperReq(struct NIPCBase *NIPCBase, struct SuperReq *req);    /* asynchronous, req freed by supervisor */

/* nipc_super.c */
extern void SuperProcess(void);
extern void GetNow(struct NIPCBase *NIPCBase, struct timeval *tv);
extern ULONG TimevalDiffUs(const struct timeval *a, const struct timeval *b);
extern void ReplySuperReq(struct NIPCBase *NIPCBase, struct SuperReq *req);

/* nipc_net.c */
extern BOOL NetOpen(struct NIPCBase *NIPCBase);
extern void NetClose(struct NIPCBase *NIPCBase);
extern void NetRefreshIfaces(struct NIPCBase *NIPCBase);
extern struct Iface *NetIfaceFor(struct NIPCBase *NIPCBase, ULONG dest);
extern BOOL NetSendRdp(struct NIPCBase *NIPCBase, ULONG dest, const UBYTE *seg, ULONG len);
extern BOOL NetSendUdp(struct NIPCBase *NIPCBase, ULONG dest, const UBYTE *data, ULONG len);
extern void NetPoll(struct NIPCBase *NIPCBase, ULONG waitus, ULONG *sigmask);
extern BOOL NetIsLocalAddress(struct NIPCBase *NIPCBase, ULONG addr);

/* nipc_rdp.c */
extern struct RdpConn *RdpOpenActive(struct NIPCBase *NIPCBase, ULONG ip, UWORD port, RdpDataFunc datain, RdpStatusFunc status, APTR userdata);
extern struct RdpConn *RdpOpenPassive(struct NIPCBase *NIPCBase, UWORD port, BOOL clone, RdpDataFunc datain, RdpStatusFunc status, APTR userdata);
extern BOOL RdpSend(struct RdpConn *conn, const UBYTE *data, ULONG len);
extern void RdpClose(struct RdpConn *conn);
extern void RdpReset(struct RdpConn *conn);
extern BOOL RdpWindowFull(struct RdpConn *conn);
extern ULONG RdpEstimateSeconds(struct RdpConn *conn, ULONG bytes);
extern void RdpInput(struct NIPCBase *NIPCBase, ULONG srcip, ULONG dstip, UBYTE *seg, ULONG len);
extern void RdpHeartbeat(struct NIPCBase *NIPCBase);
extern UWORD RdpAllocPort(struct NIPCBase *NIPCBase);
extern UWORD InetChecksum(const UBYTE *data, ULONG len, ULONG startsum);

/* nipc_link.c: entity connections, FindEntity state machine, pings */
extern void LinkHandleReq(struct NIPCBase *NIPCBase, struct SuperReq *req);
extern void LinkHeartbeat(struct NIPCBase *NIPCBase);
extern void LinkTick(struct NIPCBase *NIPCBase);
extern void LinkPumpAll(struct NIPCBase *NIPCBase);
extern void LinkConnDied(struct NIPCBase *NIPCBase, struct Entity *link);
extern void LinkShutdown(struct NIPCBase *NIPCBase);

/* nipc_resolver.c: the port-1 service */
extern BOOL ResolverStart(struct NIPCBase *NIPCBase);
extern void ResolverStop(struct NIPCBase *NIPCBase);

/* nipc_inquiry.c */
extern void InquiryHandleReq(struct NIPCBase *NIPCBase, struct SuperReq *req);
extern void InquiryInput(struct NIPCBase *NIPCBase, ULONG srcip, UBYTE *data, ULONG len);
extern void InquiryHeartbeat(struct NIPCBase *NIPCBase);
extern void InquiryTick(struct NIPCBase *NIPCBase);
extern void InquiryShutdown(struct NIPCBase *NIPCBase);
extern struct Inquiry *InquiryStart(struct NIPCBase *NIPCBase, struct TagItem *tags, ULONG maxtime, ULONG maxresp,
                                    struct Hook *hook, struct Task *caller, struct SuperReq *req);
extern void InquiryCancel(struct NIPCBase *NIPCBase, struct Inquiry *q);
extern BOOL RealmServerInEffect(struct NIPCBase *NIPCBase);

/* nipc_config.c */
extern void LoadConfig(struct NIPCBase *NIPCBase);
extern void FreeRealms(struct NIPCBase *NIPCBase);

/* nipc_event.c */
extern void EventTick(struct NIPCBase *NIPCBase, const struct timeval *now);

#endif /* NIPC_INTERN_H */
