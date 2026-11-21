#ifndef MIDI_CAMDDEVICES_H
#define MIDI_CAMDDEVICES_H

/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef EXEC_TYPES_H
#  include <exec/types.h>
#endif

#ifndef LIBCORE_COMPILER_H
#  include <libcore/compiler.h>
#endif


struct MidiPortData{
	void (* ASM ActivateXmit)(APTR REG(a2) userdata,ULONG REG(d0) portnum);
};

struct CAMDUMPEventV1;
struct CAMDGroupInfoV1;
struct CAMDFunctionBlockInfoV1;

#define CAMD_UMP_DRIVER_OK              0
#define CAMD_UMP_DRIVER_INVALID         1
#define CAMD_UMP_DRIVER_UNSUPPORTED     2
#define CAMD_UMP_DRIVER_BUSY            3
#define CAMD_UMP_DRIVER_OFFLINE         4
#define CAMD_UMP_DRIVER_FAILED          5

/* Information copied by CAMD for one port. The topology pointers are
 * borrowed only for the duration of QueryPort(). */
struct MidiUMPDriverPortInfoV1 {
  ULONG Size;
  ULONG Version;
  ULONG Directions;
  ULONG State;
  ULONG ProtocolCapabilities;
  ULONG CurrentProtocol;
  CONST_STRPTR ProductInstance;
  struct CAMDGroupInfoV1 *Groups;
  ULONG GroupCount;
  struct CAMDFunctionBlockInfoV1 *FunctionBlocks;
  ULONG FunctionBlockCount;
};

struct MidiUMPDriverHostV1 {
  ULONG Size;
  ULONG Version;
  LONG (ASM *SubmitUMP)(REG(a0) APTR hostcontext, REG(d0) ULONG portnum,
                        REG(a1) const struct CAMDUMPEventV1 *events,
                        REG(d1) ULONG eventcount, REG(a2) ULONG *dropped);
  LONG (ASM *SetPortState)(REG(a0) APTR hostcontext, REG(d0) ULONG portnum,
                           REG(d1) ULONG state);
  LONG (ASM *SetPortTopology)(REG(a0) APTR hostcontext,
                              REG(d0) ULONG portnum,
                              REG(a1) struct CAMDGroupInfoV1 *groups,
                              REG(d1) ULONG groupcount,
                              REG(a2) struct CAMDFunctionBlockInfoV1 *blocks,
                              REG(d2) ULONG blockcount);
  void (ASM *OutputReady)(REG(a0) APTR hostcontext, REG(d0) ULONG portnum);
};

/* Optional record immediately following MidiDeviceData when Flags contains
 * MDD_F_UMP_EXTENSION. It is not part of MidiDeviceData: CAMD 41/42 keep
 * seeing the exact legacy record and may use the fallback callbacks. All
 * calls are task-context in version 1. */
struct MidiUMPDriverExtensionV1 {
  ULONG Size;
  ULONG Version;
  ULONG Flags;
  CONST_STRPTR Transport;
  APTR DriverContext;
  LONG (ASM *Attach)(REG(a0) APTR drivercontext,
                      REG(a1) const struct MidiUMPDriverHostV1 *host,
                      REG(a2) APTR hostcontext);
  void (ASM *Detach)(REG(a0) APTR drivercontext);
  LONG (ASM *OpenPortUMP)(REG(a0) APTR drivercontext,
                          REG(d0) ULONG portnum, REG(d1) ULONG direction);
  void (ASM *ClosePortUMP)(REG(a0) APTR drivercontext,
                           REG(d0) ULONG portnum, REG(d1) ULONG direction);
  LONG (ASM *SendUMP)(REG(a0) APTR drivercontext, REG(d0) ULONG portnum,
                      REG(a1) const struct CAMDUMPEventV1 *events,
                      REG(d1) ULONG eventcount);
  LONG (ASM *QueryPort)(REG(a0) APTR drivercontext, REG(d0) ULONG portnum,
                        REG(a1) struct MidiUMPDriverPortInfoV1 *info);
};

struct MidiDeviceData{
  ULONG Magic;
  
  char *Name;
  char *IDString;
  
  UWORD Version;
  UWORD Revision;

  /* Called right after LoadSeg() */
  BOOL (ASM *Init)(
		   /* Added. -Kjetil M. */
		   REG(a6) APTR SysBase
		   );
  

  /* Called right before UnLoadSeg() */
  void (*Expunge)(void);

  struct MidiPortData *(ASM *OpenPort)(
	 	REG(a3) struct MidiDeviceData *data,
		REG(d0) LONG portnum,
		REG(a0) ULONG (* ASM transmitfunc)(APTR REG(a2) userdata),
		REG(a1) void (* ASM receivefunc)(UWORD REG(d0) input, APTR REG(a2) userdata),
		REG(a2) APTR userdata
		);

  void (ASM *ClosePort)(
			REG(a3) struct MidiDeviceData *data,
			REG(d0) LONG portnum
			);
  
  /* Number of ports. Cam be set in the Init-function if prefered. */
  UBYTE NPorts;

  /* MDD_F_* below. AROS requires MDD_F_NEW_FORMAT. */
  UBYTE Flags;
};

#define MDD_F_NEW_FORMAT       (1u << 0)
#define MDD_F_UMP_EXTENSION    (1u << 1)

struct MidiDeviceDataUMPV1 {
  struct MidiDeviceData Device;
  struct MidiUMPDriverExtensionV1 UMP;
};

#define MDD_UMP_EXTENSION(data) \
  ((struct MidiUMPDriverExtensionV1 *)((UBYTE *)(data) + \
                                        sizeof(struct MidiDeviceData)))


#define MDD_Magic ((ULONG)'M'<<24|(ULONG)'D'<<16|'E'<<8|'V')


#endif


