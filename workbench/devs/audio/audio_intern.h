/*
     Copyright 2010, The AROS Development Team. All rights reserved.
     $Id$
 */

/*
 * audio.device
 *
 * by Nexus Development 2003
 * coded by Emanuele Cesaroni
 *
 * $Id: libdata.h,v 1.11 2003/12/17 22:39:02 cesaroni Exp $
 */

#ifndef LIBDATA_H
#define LIBDATA_H

#include <exec/errors.h>
#include <exec/devices.h>
#include <devices/audio.h>
#include <devices/ahi.h>
#include <clib/alib_protos.h>
#include <proto/exec.h>

// Internal flags
#define ADIOB_REPLYDEV     0
#define ADIOF_REPLYDEV     (1<<0)

#define DEVB_DELEXP     0
#define DEVF_DELEXP     (1<<0)

struct audiobase
{
    struct Device Dev;
    UWORD DevState;
};

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

// names etc..

#define STOLEN_SIG SIGBREAKF_CTRL_C		// The stoolen signal... 
#define	PROCESS_STACK	160000

#define  OKEY        1
#define  KEYO        0
#define  IONOERROR   0
#define  MEM_TYPE    MEMF_PUBLIC|MEMF_CLEAR    // the mem i will alloc. Do not modify the list needs a CLEAR.

// Structures of ESYSTEM list of nodes
struct enode
{
    struct enode *prec; // Previous in list.
    struct enode *next; // Next in list.
    unsigned long int size; // Total size of this struct. So sizeof(ENODE) + the size asked during enode_AllocNode().
    unsigned long int name; // A value (used as unique name) here freely usable for system researchs.
    signed char pri; // The priority used by enode_Enqueue(). It is freely usable before add that node into a list -127 to 127.
    signed char dummy_b; // Not used. Align.
    signed short dummy_w; // Not used. Align to long.
};
typedef struct enode ENODE;

// Structure EList
struct elist
{
    struct enode first_node;
    struct enode last_node;
    struct enode *firstnode; // This pointer is the ELIST pointer!!!
    struct enode *lastnode;
};
typedef struct elist ELIST;

// Input value for enode_AllocNode()
#define	ENODE_NONAME 			0
#define	ENODE_NOBODY			0

// Quitting mode for enode_FreeList() asked in enode_AllocList().
#define	ELIST_FREE			0		// Ask to enode_AllocList when enode_FreeList is called to free the enodes the list has.
#define	ELIST_SIMPLE		1		// No free the nodes they will be freed manually.
#define	ELIST_EMPTY			0		// Ask this to enode_AllocList() to get an empty list.

struct EmaSys
{
    ELIST *es_tasklist; // The task list to intercept them at each BeginIO.
    struct SignalSemaphore es_devsem; // the semaphore used by the device.
};
typedef struct EmaSys ESYS;

ESYS emasys0;
ESYS *emasys;

struct TaskSys
{
    struct Task *ts_slavetask;
    struct IOAudio *ts_initio; // The IOAudio used to init the slave process.
    struct Task *ts_opentask; // The task which activate the slave process.
    struct Task *ts_calltask; // The task which is calling some commands as CMD_FLUSH.
};
typedef struct TaskSys TSYS;

TSYS ematsys0;
TSYS *ematsys;

// Some extra commands.
#define		CMD_ABORTIO		1000			// This is my cmd for ioaborting.
#define		CMD_CLOSE			1001			// This is my cmd for closing slave process.
// Some unit states.
#define		UNIT_STOP		1 << 0			// This unit is stopped;

// Function protos

// Init system functions
BOOL InitESYS( VOID);
VOID FreeESYS( VOID);

// Init and end slave process routines.
BOOL InitSLAVE(struct IOAudio *ioaudio);
VOID FreeSLAVE(struct IOAudio *ioaudio);

// The main slave process.
VOID TaskBody( VOID);

// Protos for the ESYSTEM
ENODE *enode_AllocNode(unsigned long int nodesize, unsigned long int name);
VOID enode_FreeNode(ENODE *thenode);
ELIST *enode_AllocList(unsigned long int toalloc, unsigned long int mode);
VOID enode_FreeList(ELIST *thelist);
VOID enode_AddHead(ELIST *thelist, ENODE *thenode);
ENODE *enode_RemHead(ELIST *thelist);
VOID enode_AddTail(ELIST *thelist, ENODE *thenode);
ENODE *enode_RemTail(ELIST *thelist);
VOID enode_Remove(ENODE *thenode);
ENODE *enode_FindNode(ELIST *thelist, unsigned long int name);
ELIST *enode_FindListNode(ENODE *thenode);
VOID enode_Insert(ELIST *thelist, ENODE *insert, ENODE *before);
VOID enode_Enqueue(ELIST *thelist, ENODE *thenode);
ENODE *enode_GetHeadNode(ELIST *thelist);
ENODE *enode_GetTailNode(ELIST *thelist);
ENODE *enode_GetNextNode(ELIST *thelist, ENODE *thenode);
ENODE *enode_GetPrecNode(ELIST *thelist, ENODE *thenode);

// This the structure which represent an audio unit.
struct emaunit
{
    struct AHIRequest eu_ahireq; // An ahi request ready to be used.
    struct MsgPort *eu_port; // My port.
    WORD eu_allockey; // The allok key for this unit.
    BYTE eu_pri; // The priority for this unit.
    UBYTE eu_id; // Number of port, from 0 to 3.
    UBYTE eu_status; // The state.
    ULONG eu_freq; // used by ahi the freq.
    ULONG eu_volume; // used by ahi the volume.
    ULONG eu_len; // used by ahi as the lenght.
    UBYTE *eu_data; // used as data by ahi.
    UWORD eu_cycles; // The cycles.
    UWORD eu_savecycles; // The cycles (save value).
    UWORD eu_actcycles; // The cycles (actual value).
    ULONG eu_repfreq; // used by ahi the freq when infinite loop.
    ULONG eu_repvolume; // used by ahi the volume when infinite loop.
    ULONG eu_replen; // used by ahi as the lenght when infinite loop.
    UBYTE *eu_repdata; // used as data by ahi when infinite loop.
    ULONG eu_clock; // The clock costant.
    struct IOAudio *eu_audioio; // The audioio which is the pre-cache.
    struct IOAudio *eu_usingme; // The one which is effectively running on unit.
    struct List eu_writewaitlist; // The list with all the IOAudio under writing (both precached or effective).
    struct List eu_waitcyclelist; // The list with all the IOAudio under waitcycle command.
};
typedef struct emaunit EUNIT;

// This is a general structure for the task with all the necessary to go.
struct etask
{
    ENODE et_node; // The node of this structure.
    WORD et_key; // The actual key.
    UBYTE et_unitmask; // A bit 1 is a unit under use.
    struct List et_allocatelist; // The list with the IOAudio waitng for allocate.
    UWORD et_allocated; // This is the counter of the one in allocatelist.
    struct GfxBase *et_gfxbase; // Graph library base.
    ULONG et_clock; // Clock costant for PAL or NTSC
    struct MsgPort *et_portunit0; // Channel 0.
    struct MsgPort *et_portunit1; // Channel 1.
    struct MsgPort *et_portunit2; // Channel 2.
    struct MsgPort *et_portunit3; // Channel 3.
    struct MsgPort *et_portunits; // All channels.
    struct MsgPort *et_portahi; // Used to communicate with ahi.device.
    struct AHIRequest *et_openahireq; // Used to open the device and to get the device's data.
    EUNIT *et_units[4]; // The units.
    struct MsgPort *et_ports[4]; // Here copied the unit msgport form 0 to 3.
    struct Task *et_slavetask; // Here the pointer to the slave process.
};
typedef struct etask ETASK;

EUNIT *init_EUnit(ETASK *estruct, UBYTE channel, struct MsgPort *myport);
VOID free_EUnit(ETASK *estruct, EUNIT *unit);

ETASK *global_eta; // For now this is used by the device to find the etask structure.


// Protos of commands which are called form the slave process.
VOID _CMD_WRITE(struct IOAudio *audioio, ETASK *eta, EUNIT *unit);
VOID _CMD_RESET(struct IOAudio *audioio, ETASK *eta);
VOID _CMD_READ(struct IOAudio *audioio, ETASK *eta);
VOID _CMD_STOP(struct IOAudio *audioio, ETASK *eta);
VOID _CMD_START(struct IOAudio *audioio, ETASK *eta);
VOID _CMD_UPDATE(struct IOAudio *audioio, ETASK *eta);
VOID _CMD_CLEAR(struct IOAudio *audioio, ETASK *eta);
VOID _ADCMD_WAITCYCLE(struct IOAudio *audioio, ETASK *eta);
VOID _ADCMD_PERVOL(struct IOAudio *audioio, ETASK *eta);
VOID _ADCMD_ALLOCATE(struct IOAudio *audioio, ETASK *eta);
VOID _ADCMD_FREE(struct IOAudio *audioio, ETASK *eta);
VOID _ADCMD_SETPREC(struct IOAudio *audioio, ETASK *eta);
VOID _ADCMD_FINISH(struct IOAudio *audioio, ETASK *eta);
VOID _ADCMD_LOCK(struct IOAudio *audioio, ETASK *eta);

VOID _CMD_FLUSH(struct IOAudio *audioio, ETASK *eta);
VOID _CMD_ABORTIO(struct IOAudio *audioio, ETASK *eta);
VOID _CMD_CLOSE(struct IOAudio *audioio, ETASK *eta);

// Protos of commands called by the device.
VOID audio_ALLOCATE(struct IOAudio *audioio);
VOID audio_FREE(struct IOAudio *audioio);
VOID audio_LOCK(struct IOAudio *audioio);
VOID audio_FINISH(struct IOAudio *audioio);
VOID audio_PERVOL(struct IOAudio *audioio);
VOID audio_SETPREC(struct IOAudio *audioio);
VOID audio_WAITCYCLE(struct IOAudio *audioio);
VOID audio_READ(struct IOAudio *audioio);
VOID audio_WRITE(struct IOAudio *audioio);
VOID audio_UPDATE(struct IOAudio *audioio);
VOID audio_CLEAR(struct IOAudio *audioio);
VOID audio_STOP(struct IOAudio *audioio);
VOID audio_FLUSH(struct IOAudio *audioio);
VOID audio_RESET(struct IOAudio *audioio);
VOID audio_START(struct IOAudio *audioio);

VOID audio_ABORTIO(struct IOAudio *audioio, ETASK *eta);
VOID audio_CLOSE(struct IOAudio *audioio, ETASK *eta);

// Protos of some utils.
VOID ReplyWaitCycles(EUNIT *unit, ETASK *eta);
VOID UnitCopyData(EUNIT *unit, struct IOAudio *audioio);
VOID UnitAHIPerVol(EUNIT *unit, struct IOAudio *audioio);
VOID UnitInitAhi(EUNIT *unit);
VOID UnitInitRepAhi(EUNIT *unit);
VOID FlushUnit(EUNIT *unit);
VOID ResetUnit(EUNIT *unit);
VOID ReAllocateUnits(ETASK *eta);

#endif /* LIBDATA_H */
