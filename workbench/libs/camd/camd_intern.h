#ifndef CAMD_INTERN_H
#define CAMD_INTERN_H

/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.
    $Id$

    Desc: 
    Lang: English
*/

#ifndef EXEC_TYPES_H
#   include <exec/types.h>
#endif
#ifndef EXEC_LIBRARIES_H
#   include <exec/libraries.h>
#endif

#ifndef __AROS__
#  define AROS_LIBFUNC_INIT
#  define AROS_LIBFUNC_EXIT
#  ifndef __amigaos4__
#    define aros_print_not_implemented(a) kprintf("camd.library: "a" is not implemented\n");
#  else
#    define aros_print_not_implemented(a) DebugPrintF("camd.library: "a" is not implemented\n");
#  endif
#  define AROS_PTRALIGN 2
#else
#  ifndef AROS_LIBCALL_H
#    include <aros/libcall.h>
#  endif
#endif

#ifndef DOS_DOS_H
#   include <dos/dos.h>
#endif
#ifndef UTILITY_UTILITY_H
#   include <utility/utility.h>
#endif

#ifndef MIDI_CAMD_H
#  include <midi/camd.h>
#endif

#ifndef MIDI_CAMDDEVICES_H
#  include <midi/camddevices.h>
#endif

#ifndef EXEC_SEMAPHORES_H
#  include <exec/semaphores.h>
#endif
#ifndef EXEC_MEMORY_H
#  include <exec/memory.h>
#endif
#ifndef DOS_DOSEXTENS_H
#include <dos/dosextens.h>
#endif

#ifdef __AROS__
#  undef DEBUG
#  define DEBUG 1
#  include <aros/debug.h>
#endif


#ifdef __amigaos4__
#  include <libcore/base.h>
#  include <interfaces/camd.h>
#  define CAMD_LIBBASE_DECL(a,b) a b = (a) ICamd->Data.LibBase;
#endif

#include "legacy_port_refs.h"
#include "native_worker_fanout.h"



/****************************************************************************************/

#define OUTBUFFERSIZE 1025
#define OUTBUFFERSIZE_RT 1025
#define RECEIVERPROCBUFFERSIZE 1025
#define SYSEXRECEIVERPROCBUFFERSIZE 1025
#define ENDPOINT_OUTPUT_SESSIONS 32
#define ENDPOINT_OUTPUT_QUEUE 64
#define ENDPOINT_OUTPUT_SYSEX CAMD_SYSEX_BYTES_DEFAULT
#define ENDPOINT_SYSEX_LIMIT CAMD_SYSEX_BYTES_LIMIT
#define ENDPOINT_OUTPUT_BUDGET 16
#define ENDPOINT_SNAPSHOTS 32
#define ENDPOINT_RECEIVERS 64
#define ENDPOINT_WATCHES 16
#define ENDPOINT_PUBLISHERS 32
#define ENDPOINT_DRAINERS 16

/* What a task can leave behind; see OwnEndpointObject(). */
#define ENDPOINT_OWNED_SNAPSHOT  1
#define ENDPOINT_OWNED_SESSION   2
#define ENDPOINT_OWNED_WATCH     3
#define ENDPOINT_OWNED_PUBLISHER 4
#define ENDPOINT_WATCH_EVENTS 32
#define NUMBEROFSYSEXSTORECEIVE 1025

struct MyMidiMessage2{
	UBYTE status;
	UBYTE data1;
	UBYTE data2;
	UBYTE len;
};

struct MyMidiCluster{
	struct MidiCluster cluster;
	struct SignalSemaphore semaphore;
	char comment[35];	// Highest-priority link's MLINK_Comment, copied here.

	/* Updated atomically; read as diagnostic snapshots by GetClusterAttrsA(). */
	ULONG overflows;	// messages dropped because a receive queue was full
	ULONG sysexdropped;	// SysEx dropped because a SysEx buffer was full
	ULONG recverrors;	// driver receive errors and overruns
};

struct CAMDAppEndpoint;

struct DriverData{
	struct Node outnode;								// Used for the outcluster's receiverlist
	struct Node innode;								// Used for the incluster's senderlist

	struct Drivers *driver;
	struct MyMidiCluster *incluster;				// Clusters we belong to.
	                                				// in  = sender cluster
	                                				// out = receiver cluster
	struct MyMidiCluster *outcluster;
	struct MidiPortData *midiportdata;
	struct MidiDeviceData *mididevicedata;
	LONG portnum;

	/* Legacy cluster directions and private endpoint sessions share one
	   physical OpenPort()/ClosePort() transition through this state. */
	struct CAMDLegacyPortRefs openrefs;

	BOOL isAttached;		// innode and outnode are in their clusters

	ULONG *buffer;				// Sendbuffer start
	ULONG *buffercurrsend;	// Sendbuffer write
	ULONG *buffercurr;		// Sendbuffer read
	ULONG *bufferend;			// Sendbuffer end

	int sendpos;				// 0,1 or 2. Position in current sendmessage. (Is only 0 at startup, so 1 or 2 are the only used values)
	int status;					// Current send running status

	int unsent;					// Number of messages waiting to be sent
	int unsent_rt;				// Number of realtime messages waiting to be sent
	int issending_sx;			// =1 if a sysex message is currently being sent.

	int buffercurrsend_sx;	// Number of bytes in the current sysex message being sent.
	UBYTE *buffer_sx;			// Pointer to user-suplied sysex message
	int unsent_sx;				// Used by miditodriver_oldformat

	UBYTE *buffer_rt;				// Realtime sendbuffer start
	UBYTE *buffercurrsend_rt;	// Realtime sendbuffer write
	UBYTE *buffercurr_rt;		// Realtime sendbuffer read
	UBYTE *bufferend_rt;			// Realtime sendbuffer end
	int isnowtransmitting;		// Used by mididitodriver_oldformat

	int realtimesysx;				// =1 if the current sysex message being sent is realtime.

	ULONG (*transmitfunc)(					// Virtual function for the transmitter driver-code.
		struct DriverData *driverdata
	);

	struct MyMidiMessage2 msg2;		// In-message, to be supplied to the midinodes.

	struct SignalSemaphore sendsemaphore;	// Obtained when putting message to driver
	struct SignalSemaphore sysexsemaphore;	// Obtained when putting sysex to driver.

	void (*Input_Treat)(							// Virtual function for the receiver driver-code.
		struct DriverData *driverdata,
		UBYTE data
	);


// The following datas are for the ReceiverProcess.

	struct Process *ReceiverProc;
	ULONG ReceiverSig;
	LONG EndpointOutputSig;
	struct CAMDNativeWorkerFanout *EndpointOutputFanout;

	/* Set while the port is a published endpoint. The receiver process
	   hands what it parses to the endpoint's input sessions and collects
	   each SysEx message here first. */
	struct CAMDLegacyDriverOutputBackend *EndpointBackend;
	UBYTE *EndpointSysEx;
	ULONG EndpointSysExLength;
	BOOL EndpointSysExOverflow;
	ULONG EndpointInputDropped;	// messages an input session had no room for

	/* Set on the port that stands for a published endpoint in its legacy
	   clusters: what is sent to it goes to that endpoint, not to a driver. */
	struct CAMDAppEndpoint *Projection;

	UWORD *re_start;
	UWORD *re_write;
	UWORD *re_end;
	int unpicked;
	UWORD *re_read;
	ULONG re_overflow;		// input is dropped until the receiver resets

// To know the status of a ReceiverProcess. 0=not alive, or not initialized yet. 1=alive and fine. 2=signal failed.
	LONG isReceiverProcessAlive;
// Keep pointer so it can be freed after the process dies.
	char *ReceiverProcName;

// The following is for ParseMidi
	UBYTE *lastsysex;
};

struct Drivers{
	struct Drivers *next;
	ULONG numports;
	BPTR seglist;
	struct MidiDeviceData *mididevicedata;
	struct DriverData **driverdatas;

	/* Private endpoint publication of the ports; NULL when not published. */
	struct CAMDLegacyDriverOutputBackend *EndpointBackend;
	struct CAMDLegacyDriverAdapter *EndpointAdapter;
};


struct MyMidiNode{
	struct MidiNode midinode;

	ULONG error;

	MidiMsg *in_start;
	MidiMsg *in_curr;
	MidiMsg *in_end;

	int unpicked;

	MidiMsg *in_curr_get;

	ULONG dummytimestamp;	// To avoid reading from NULL if no timestamp was defined.

	struct SignalSemaphore receiversemaphore;

// For sysex receiving. (probably needs some rewriting)

	struct SignalSemaphore sysexsemaphore;
	struct SignalSemaphore sysexsemaphore2;
	UBYTE *sysex_start;
	UBYTE *sysex_write;
	UBYTE *sysex_end;
	UBYTE *sysex_read;
	UBYTE *sysex_laststart;
	BOOL sysex_nextis0;
	BOOL sysex_started_empty;
	BOOL sysex_discarding;
	UBYTE lastreadstatus;

	BOOL systemclock;	// MIDI_SystemClock: stamp with CamdTime()
};

/* A StartClusterWatchA() handle. Events past the queue are lost, and
   the client is then told to look at every cluster again. */
#define CLUSTERWATCH_QUEUE 32

struct ClusterWatch{
	struct MinNode node;
	struct Task *task;
	BYTE sigbit;
	BOOL lost;
	UWORD first;
	UWORD count;
	struct ClusterWatchEvent events[CLUSTERWATCH_QUEUE];
};

struct CAMDEndpointRegistry;
struct CAMDEndpointSnapshot;
struct CAMDNativeEventQueue;
struct CAMDEndpointWatch;

struct CAMDLegacyDriverAdapter;

struct EndpointProjection;

/* A PublishEndpoint() handle. The slot is free while app is NULL; withdrawn
   marks one whose publisher has gone and that waits for its clients. */
struct EndpointPublisher{
	struct CAMDAppEndpoint *app;
	struct EndpointProjection *projection;	// its legacy clusters
	struct CAMDLegacyDriverAdapter *adapter;
	ULONG generation;
	BOOL withdrawn;
	struct Task *task;
	ULONG signal;			// signal number, or CAMD_SIGNAL_NONE
};

/* A StartEndpointWatch() handle. */
struct EndpointWatcher{
	struct CAMDEndpointWatch *watch;	// NULL while the slot is free
	ULONG generation;
	struct Task *task;
	ULONG signal;			// signal number, or CAMD_SIGNAL_NONE
};

/* The client side of an open input session. */
struct EndpointReceiver{
	struct CAMDHandleV1 session;	// generation 0 while the slot is free
	struct CAMDNativeEventQueue *queue;
	struct Task *task;
	ULONG signal;			// signal number, or CAMD_SIGNAL_NONE
	ULONG format;			// CAMD_DATA_FORMAT_MIDI1 or _UMP
	BOOL lost;			// the queue was full since the last read
};
struct CAMDIdentityStoreAROS;

struct CamdBase_intern{
#ifndef __amigaos4__
    struct Library library;
#else
    struct LibHeader		lh;
#endif
    struct Drivers *drivers;
    struct List mymidinodes;
    struct List midiclusters;

    struct List clusnotifynodes;


    /* Lock semaphore. Obtained Shared before reading various lists and
       obtained exclusive before adding/deleting to/from various lists. */

    struct SignalSemaphore *CLSemaphore;

    /* Serialize DEVS:Midi scans without holding CLSemaphore over LoadSeg. */
    struct SignalSemaphore DriverScanSemaphore;

    /* Protects clusnotifynodes. Obtained after CLSemaphore, never before, so
       that StartClusterNotify() and EndClusterNotify() work while the caller
       holds LockCAMD(). */
    struct SignalSemaphore NotifySemaphore;

    /* StartClusterWatchA() handles, also under NotifySemaphore. */
    struct MinList clusterwatches;

    /* Private endpoint core. Its opaque registry owns its semaphore. */
    struct CAMDEndpointRegistry *EndpointRegistry;

    /* Private stable-ID map. Failure is non-fatal for legacy CAMD. */
    struct CAMDIdentityStoreAROS *EndpointIdentity;

    /* ObtainEndpointSnapshot() handles: a slot is free while NULL, and its
       generation tells a handle from an earlier one in the same slot. */
    struct SignalSemaphore EndpointClientSemaphore;
    struct CAMDEndpointSnapshot *EndpointSnapshots[ENDPOINT_SNAPSHOTS];
    ULONG EndpointSnapshotGenerations[ENDPOINT_SNAPSHOTS];

    /* Input sessions' queues, also under EndpointClientSemaphore. */
    struct EndpointReceiver EndpointReceivers[ENDPOINT_RECEIVERS];

    /* Endpoint watches, also under EndpointClientSemaphore. */
    struct EndpointWatcher EndpointWatchers[ENDPOINT_WATCHES];

    /* Published endpoints, also under EndpointClientSemaphore. */
    struct EndpointPublisher EndpointPublishers[ENDPOINT_PUBLISHERS];

    /* Tasks inside DrainEndpointSession(), and the signal each waits for.
       Read and written under Forbid(), since session workers signal them. */
    struct Task *EndpointDrainers[ENDPOINT_DRAINERS];
    ULONG EndpointDrainSignals[ENDPOINT_DRAINERS];

    /* Which task has the library open how often, and which endpoint objects
       it made, so that they go when it closes the library for the last
       time. Under EndpointOwnerSemaphore, taken last. */
    struct SignalSemaphore EndpointOwnerSemaphore;
    struct MinList EndpointOpeners;
    struct MinList EndpointOwned;


};

#ifdef __AMIGAOS__
   extern void kprintf(char *bla,...);
#  ifdef DEBUG
#  ifndef __amigaos4__
#    define bug kprintf
#  else
#    define bug DebugPrintF
#  endif
#    define D(a) a
#  else
#    define D(a)
#  endif
#  ifndef CONST_STRPTR
#    define CONST_STRPTR const APTR
#  endif
#  ifndef VOID_FUNC
#    define VOID_FUNC APTR		// Yeah.
#  endif
#  define AROS_DEBUG_H_FILE "camd_intern.h"		// DoNothing
#else
#  define AROS_DEBUG_H_FILE <aros/debug.h>
#endif

/****************************************************************************************/

#  undef CB
#  define CB(b)		((struct CamdBase_intern *)b)

/****************************************************************************************/


#ifndef NEWLIST
#  define NEWLIST(l) (((struct List *)l)->lh_TailPred = (struct Node *)(l),((struct List *)l)->lh_Tail=0,((struct List *)l)->lh_Head = (struct Node *)&(((struct List *)l)->lh_Tail))
#endif


#ifndef FUNCTIONTABLE
/* Prototypes */

struct CamdBase{struct Library library;};
BYTE GetMsgLen(LONG msg);
void PutMidi2Link(
	struct MidiLink *midilink,
	struct MyMidiMessage2 *msg2,
	ULONG timestamp
);
void UnlinkMidiLink(struct MidiLink *midilink,BOOL unlinkfromnode,struct CamdBase *CamdBase);
BOOL Midi2Driver_internal(
	struct DriverData *driverdata,
	ULONG msg,
	ULONG maxbuff
);
BOOL Midi2Driver_internal_oldformat(
	struct DriverData *driverdata,
	ULONG msg,
	ULONG maxbuff
);
#ifndef __amigaos4__
  ULONG ASM Transmitter(REG(a2) struct DriverData *driverdata);
  ULONG ASM Transmitter_oldformat(REG(a2) struct DriverData *driverdata);
#else
  ULONG Transmitter(struct DriverData *driverdata);
  ULONG ASM Transmitter_oldformat(REG(a2, struct DriverData *driverdata));
#endif
BOOL SysEx2Driver(struct DriverData *driverdata,UBYTE *buffer);
BOOL SysEx2Driver_oldformat(struct DriverData *driverdata, UBYTE *buffer);
void RemoveCluster(struct MidiCluster *cluster,struct CamdBase *CamdBase);
void LinkHasBeenRemovedFromCluster(struct MidiCluster *cluster,struct CamdBase *CamdBase);
struct MidiCluster *NewCluster(char *name,struct CamdBase *CamdBase);
struct DriverData *FindReceiverDriverInCluster(struct MidiCluster *cluster);
struct DriverData *FindSenderDriverInCluster(struct MidiCluster *cluster);
BOOL AddClusterReceiver(
	struct MidiCluster *cluster,
	struct Node *node,
	ULONG *ErrorCode,
	struct CamdBase *CamdBase
);
BOOL AddClusterSender(
	struct MidiCluster *cluster,
	struct Node *node,
	ULONG *ErrorCode,
	struct CamdBase *CamdBase
);
BOOL SetClusterForLink(
	struct MidiLink *midilink,
	char *name,
	ULONG *ErrorCode,
	struct CamdBase *CamdBase
);

BOOL CreateReceiverProc(
	struct DriverData *driverdata,
	char *name,
	LONG portnum,
	ULONG *ErrorCode,
	struct CamdBase *CamdBase
);

void EndReceiverProc(
	struct DriverData *driverdata,
	struct CamdBase *CamdBase
);

void Receiver_SysExSuperTreat(
	struct DriverData *driverdata,
	UBYTE data
);
void Receiver_SysEx(
	struct DriverData *driverdata,
	UBYTE data
);
int GetSysXLen(UBYTE *buffer);
void SetMidiError(struct MyMidiNode *mymidinode,ULONG errorcode);
BOOL PutSysEx2Link(struct MidiLink *midilink,UBYTE data);
void Receiver_SetError(
	struct DriverData *driverdata,
	ULONG errorcode
);
void Receiver_RealTime(
	struct DriverData *driverdata,
	UBYTE status
);
void Receiver_init(
	struct DriverData *driverdata,
	UBYTE data
);

void Receiver_first(
	struct DriverData *driverdata
);

#ifndef __amigaos4__
void ASM Receiver(
	REG(d0) UWORD input,
	REG(a2) struct DriverData *driverdata
);
#else
void ASM Receiver(
	REG(d0, UWORD input),
	REG(a2, struct DriverData *driverdata)
	);
#endif

extern WORD MidiMsgType_status(UBYTE status);
extern WORD MidiMsgType_CMB_Ctrl(UBYTE data1);
extern WORD MidiMsgType_status_data1(UBYTE status,UBYTE data1);

BOOL OpenDriver(struct DriverData *driverdata,ULONG *ErrorCode,struct CamdBase *CamdBase);
void CloseDriver(struct DriverData *driverdata,struct CamdBase *CamdBase);
BOOL SetDriverLegacyUse(struct DriverData *driverdata,ULONG direction,
	BOOL present,ULONG *ErrorCode,struct CamdBase *CamdBase);
enum CAMDProviderResult AcquireDriverEndpointPort(
	struct DriverData *driverdata,ULONG direction,struct CamdBase *CamdBase);
enum CAMDProviderResult ReleaseDriverEndpointPort(
	struct DriverData *driverdata,ULONG direction,struct CamdBase *CamdBase);
BOOL DriverPortInUse(struct DriverData *driverdata);
void PublishDriverEndpoints(struct Drivers *driver,char *path,
	struct CamdBase *CamdBase);
BOOL UnpublishDriverEndpoints(struct Drivers *driver,
	struct CamdBase *CamdBase);
struct CAMDHandleV1;
LONG AddEndpointSnapshot(struct CAMDEndpointSnapshot *snapshot,
	struct CAMDHandleV1 *handle,struct CamdBase *CamdBase);
struct CAMDEndpointSnapshot *FindEndpointSnapshot(
	const struct CAMDHandleV1 *handle,struct CamdBase *CamdBase);
void RemoveEndpointSnapshot(const struct CAMDHandleV1 *handle,
	struct CamdBase *CamdBase);
void FreeEndpointSnapshots(struct CamdBase *CamdBase);
BOOL EndpointClientsRemain(struct CamdBase *CamdBase);
struct timerequest;
struct timerequest *OpenEndpointPause(void);
void EndpointPause(struct timerequest *pause,ULONG milliseconds,ULONG signals);
LONG AddEndpointDrainer(ULONG signals,struct CamdBase *CamdBase);
void RemoveEndpointDrainer(LONG slot,struct CamdBase *CamdBase);
void NotifyEndpointDrained(struct CamdBase *CamdBase);
void CloseEndpointPause(struct timerequest *pause);
struct CAMDMIDI1EventV1;
LONG StartEndpointReceiver(const struct CAMDHandleV1 *session,ULONG format,
	ULONG capacity,ULONG maxsysex,ULONG signal,struct CamdBase *CamdBase);
struct CAMDUMPEventV1;
LONG ReadEndpointReceiverUMP(const struct CAMDHandleV1 *session,
	struct CAMDUMPEventV1 *event,struct CamdBase *CamdBase);
void StopEndpointReceiver(const struct CAMDHandleV1 *session,
	struct CamdBase *CamdBase);
LONG ReadEndpointReceiver(const struct CAMDHandleV1 *session,
	struct CAMDMIDI1EventV1 *event,UBYTE *sysex,ULONG size,ULONG *length,
	struct CamdBase *CamdBase);
struct CAMDGenerationV1;
struct CAMDEndpointWatchEventV1;
LONG StartEndpointWatcher(struct CAMDHandleV1 *handle,ULONG signal,
	struct CAMDGenerationV1 *generation,struct CamdBase *CamdBase);
LONG ReadEndpointWatcher(const struct CAMDHandleV1 *handle,
	struct CAMDEndpointWatchEventV1 *event,struct CamdBase *CamdBase);
void EndEndpointWatcher(const struct CAMDHandleV1 *handle,
	struct CamdBase *CamdBase);
struct CAMDPublishRequestV1;
struct CAMDEndpointIDV1;
LONG PublishAppEndpoint(const struct CAMDPublishRequestV1 *request,
	struct CAMDHandleV1 *handle,struct CAMDEndpointIDV1 *id,
	struct CamdBase *CamdBase);
void WithdrawAppEndpoint(const struct CAMDHandleV1 *handle,
	struct CamdBase *CamdBase);
LONG SetAppEndpointState(const struct CAMDHandleV1 *handle,ULONG state,
	struct CamdBase *CamdBase);
LONG PutAppEndpointMidi(const struct CAMDHandleV1 *handle,
	const struct CAMDMIDI1EventV1 *events,ULONG count,
	struct CamdBase *CamdBase);
LONG PutAppEndpointSysEx(const struct CAMDHandleV1 *handle,
	const UBYTE *bytes,ULONG length,struct CamdBase *CamdBase);
LONG GetAppEndpointMidi(const struct CAMDHandleV1 *handle,
	struct CAMDMIDI1EventV1 *event,UBYTE *sysex,ULONG size,ULONG *length,
	struct CamdBase *CamdBase);
BOOL AppEndpointsRemain(struct CamdBase *CamdBase);
struct CAMDPublishedStatsV1;
LONG GetAppEndpointStats(const struct CAMDHandleV1 *handle,
	struct CAMDPublishedStatsV1 *stats,struct CamdBase *CamdBase);
/* The identity namespaces of published endpoints and their publishers. */
#define NAMESPACE_APP_PROVIDER 0x41505256u  /* 'APRV' */
#define NAMESPACE_APP_ENDPOINT 0x41454e44u  /* 'AEND' */

/* endpoint_identity.c */
LONG ReadIdentityStore(struct CAMDIdentityStoreInfoV1 *info,
	struct CamdBase *CamdBase);
LONG ReadStoredIdentity(ULONG index,struct CAMDStoredIdentityV1 *identity,
	struct CamdBase *CamdBase);
LONG ForgetStoredIdentity(const struct CAMDEndpointIDV1 *id,
	struct CamdBase *CamdBase);

/* Copies a record a caller hands in. See TakeEndpointRecord(). */
BOOL TakeEndpointRecord(const APTR given,APTR record,ULONG size,ULONG least);

void OwnEndpointObject(ULONG kind,const struct CAMDHandleV1 *handle,
	struct CamdBase *CamdBase);
void DisownEndpointObject(ULONG kind,const struct CAMDHandleV1 *handle,
	struct CamdBase *CamdBase);
void TaskOpenedCamd(struct CamdBase *CamdBase);
void TaskClosedCamd(struct CamdBase *CamdBase);
void FreeEndpointOwners(struct CamdBase *CamdBase);
struct CAMDGroupInfoV1;
struct CAMDFunctionBlockInfoV1;
LONG PutAppEndpointUMP(const struct CAMDHandleV1 *handle,
	const struct CAMDUMPEventV1 *events,ULONG count,struct CamdBase *CamdBase);
LONG GetAppEndpointUMP(const struct CAMDHandleV1 *handle,
	struct CAMDUMPEventV1 *event,struct CamdBase *CamdBase);
LONG SetAppEndpointTopology(const struct CAMDHandleV1 *handle,
	struct CAMDGroupInfoV1 *groups,ULONG group_count,
	struct CAMDFunctionBlockInfoV1 *blocks,ULONG block_count,
	struct CamdBase *CamdBase);
struct EndpointProjection *CreateEndpointProjection(
	struct CAMDAppEndpoint *app,const char *name,ULONG directions,
	struct CamdBase *CamdBase);
void DeleteEndpointProjection(struct EndpointProjection *projection);
BOOL SetEndpointProjectionOnline(struct EndpointProjection *projection,
	BOOL online);
const char *EndpointProjectionName(struct EndpointProjection *projection);
BOOL AppEndpointNamed(const char *name,struct CamdBase *CamdBase);
BOOL ProjectMidiToEndpoint(struct DriverData *driverdata,ULONG msg);
BOOL ProjectSysExToEndpoint(struct DriverData *driverdata,UBYTE *buffer);
void Receiver_SuperTreat2(struct DriverData *driverdata,
	struct MyMidiMessage2 *msg2);
BOOL AllocDriverData(struct Drivers *driver,
	struct MidiDeviceData *mididevicedata,struct CamdBase *CamdBase);
void DeliverDriverEndpointMidi(struct DriverData *driverdata,
	struct MyMidiMessage2 *msg2);
void DeliverDriverEndpointSysEx(struct DriverData *driverdata,UBYTE data);

void FreeDriverData(struct Drivers *driver,
	struct CamdBase *CamdBase
);
BOOL AttachDriverData(struct Drivers *driver,
	struct CamdBase *CamdBase
);
void DetachDriverData(struct Drivers *driver,
	struct CamdBase *CamdBase
);
void SignalClusterNotify(struct CamdBase *CamdBase,ULONG event,struct MidiCluster *cluster);
LONG ScanDrivers(struct CamdBase *CamdBase);
#ifndef __amigaos4__
void InitDriverWatch(struct CamdBase *CamdBase);
void DriversChanged(void);
void StopDriverWatch(void);
#endif
void ClusterParticipantsChanged(struct MidiCluster *cluster,struct MidiNode *changer,struct CamdBase *CamdBase);
void UpdateClusterComment(struct MidiCluster *cluster);
BOOL InitCamdClock(void);
void UninitCamdClock(void);
ULONG CamdClock(void);

/* The timestamp CAMD gives a message delivered to a node. */
#define NodeTimeStamp(mymidinode) \
	((mymidinode)->systemclock ? CamdClock() : *(mymidinode)->midinode.mi_TimeStamp)
void LoadDriver(char *name,
	struct CamdBase *CamdBase
);

struct Drivers *FindPrevDriverForMidiDeviceData(
	struct MidiDeviceData *mididevicedata,
	struct CamdBase *CamdBase
);

ULONG mystrlen(char *string);
BOOL mystrcmp(char *one,char *two);
char *findonlyfilename(char *pathfile);
#ifndef __amigaos4__
   void mysprintf(struct CamdBase *camdbase,char *string,char *fmt,...);
#else
#  define mysprintf(camdbase, string, fmt, ...) SNPrintf(string, 256, fmt, __VA_ARGS__)
#endif
struct MidiLink *GetMidiLinkFromOwnerNode(struct MinNode *node);

void CamdWait(void);

#endif

BOOL InitCamdTimer(void);
void UninitCamdTimer(void);

#ifdef __amigaos4__
APTR GoodPutMidi ( struct CamdIFace *ICamd, struct MidiLink * midilink, uint32 msg, uint32 maxbuff );
#endif

#ifdef __amigaos4__
BOOL InitCamd(struct CamdIFace *ICamd);
void UninitCamd(struct CamdIFace *ICamd);
#else
BOOL InitCamd(struct CamdBase *CamdBase);
void UninitCamd(struct CamdBase *CamdBase);
#endif

#endif /* CAMD_INTERN_H */

