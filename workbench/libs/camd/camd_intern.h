#ifndef CAMD_INTERN_H
#define CAMD_INTERN_H

/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
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
#  ifndef __amigaos4__
#    define AROS_LIBBASE_EXT_DECL(a,b)
#  endif
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
#  define AROS_LIBBASE_EXT_DECL(a,b) a b = (a) ICamd->Data.LibBase;
#endif



/****************************************************************************************/

#define OUTBUFFERSIZE 1025
#define OUTBUFFERSIZE_RT 1025
#define RECEIVERPROCBUFFERSIZE 1025
#define SYSEXRECEIVERPROCBUFFERSIZE 1025
#define NUMBEROFSYSEXSTORECIEVE 1025

struct MyMidiMessage2{
	UBYTE status;
	UBYTE data1;
	UBYTE data2;
	UBYTE len;
};

struct MyMidiCluster{
	struct MidiCluster cluster;
	struct SignalSemaphore semaphore;
};

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

	BOOL isInOpen;		// Driver is opened if at least one of these two ones is TRUE
	BOOL isOutOpen;

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

	UWORD *re_start;
	UWORD *re_write;
	UWORD *re_end;
	int unpicked;
	UWORD *re_read;

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

// For sysex recieving. (probably needs some rewriting)

	struct SignalSemaphore sysexsemaphore;
	struct SignalSemaphore sysexsemaphore2;
	UBYTE *sysex_start;
	UBYTE *sysex_write;
	UBYTE *sysex_end;
	UBYTE *sysex_read;
	UBYTE *sysex_laststart;
	BOOL sysex_nextis0;
	UBYTE lastreadstatus;
};

struct CamdBase_intern{
#ifndef __amigaos4__
    struct Library library;
    struct ExecBase *sysbase;
    APTR seglist;
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

void Reciever_SysExSuperTreat(
	struct DriverData *driverdata,
	UBYTE data
);
int GetSysXLen(UBYTE *buffer);
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

void FreeDriverData(struct Drivers *driver,
	struct CamdBase *CamdBase
);
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

