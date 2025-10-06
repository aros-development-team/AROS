#ifndef _BASE_H_
#define _BASE_H_
/*
 * Netinfo device base. Define the device structure 
 *
 * Copyright © 2025 The AROS Dev Team.
 * Copyright (c) 1993 Pekka Pessi
 */

#include "config.h"

#ifndef EXEC_LIBRARIES_H
#include <exec/libraries.h>
#endif

#ifndef DEVICES_NETINFO_H
#include <devices/netinfo.h>
#endif

#ifndef EXEC_TASKS_H
#include <exec/tasks.h>
#endif

#ifndef UTILITY_TAGITEM_H
#include <utility/tagitem.h>
#endif

#ifndef EXEC_IO_H
#include <exec/io.h>
#endif

#ifndef EXEC_DEVICES_H
#include <exec/devices.h>
#endif

#ifndef EXEC_ERRORS_H
#include <exec/errors.h>
#endif

#ifndef EXEC_MEMORY_H
#include <exec/memory.h>
#endif

#ifndef EXEC_SEMAPHORES_H
#include <exec/semaphores.h>
#endif

#ifdef USE_PRAGMAS
#include <clib/exec_protos.h>
#include <pragmas/exec_sysbase_pragmas.h>

#include <clib/dos_protos.h>
#include <pragmas/dos_pragmas.h>

#define SysBase (nid->nid_ExecBase)
#define DOSBase (nid->nid_DOSBase)
#else
#include <dos/notify.h>
#include <dos/bptr.h>
#endif

/*
 * Device base 
 */
struct NetInfoDevice {
  struct Library         nid_Lib;
  APTR		         nid_SegList;
  /* like there were no reference operator in C... */
  struct SignalSemaphore nid_Lock[1];
  APTR                   nid_ExecBase;
  APTR                   nid_DOSBase;
  struct MsgPort         nid_Port[1];	/* Port to send requests */
  struct Message        *nid_Death;	/* Kill task by this message */
  struct MsgPort         nid_NotifyPort[1]; /* Port for notify messages */
  struct List            nid_Instances[1];
  struct NetInfoMap     *nid_Maps[NETINFO_UNITS];
  const char            *nid_dbuser;
  const char            *nid_dbgroup;
};

#define nid_Task nid_Port->mp_SigTask
#define nid_SigBit nid_Port->mp_SigBit

/* Internal constants */
#define NETINFOSIZE ((sizeof(struct NetInfoDevice) +3) & ~3)
#define NID_PRIORITY 1

typedef void (* DeviceCmd_t)(struct NetInfoDevice *, struct NetInfoReq *, struct NetInfoMap *);

/*
 * Structure for each netinfo map
 */
struct NetInfoMap {
  struct Node            nim_Node[1];
  struct MsgPort        *nim_Port;
  const struct MapMethods *nim_Methods;
  const DeviceCmd_t     *nim_Commands;
  struct SignalSemaphore nim_PointerLock[1];
  struct List            nim_Pointer[1];     /* under nid_Lock */
  struct SignalSemaphore nim_ReqLock[1];
  struct List            nim_Rx[1];
  struct List            nim_Wx[1];
  WORD                   nim_OpenCnt;
  WORD                   nim_Flags;	     
  const UBYTE           *nim_Filename;
  struct SignalSemaphore nim_EntLock[1];
  struct List            nim_Ent[1];
  struct NotifyRequest   nim_Notify[1];
};

#define NIMF_PARSED   0x0001
#define NIMF_CHANGED  0x0002

#define nim_Name nim_Node->ln_Name

/*
 * Define map methods
 */
struct MapMethods {
  struct Ent *        (*parse_ent)(struct NetInfoDevice *, register UBYTE *p);
  int                 (*print_out)(struct NetInfoDevice *, BPTR, struct Ent *); 
  void *              (*copy_out)(struct NetInfoReq *req, struct Ent *e);
  struct Ent *        (*copy_in)(struct NetInfoDevice *, struct NetInfoReq *req);
  void                (*cleanup)(struct NetInfoDevice *, struct NetInfoMap *);
  void                (*membercmd)(struct NetInfoDevice *, struct NetInfoReq *, 
				   struct NetInfoMap *);
  void                (*notify)(struct NetInfoDevice *, struct NetInfoMap *);
};

#define DoNIMethod(cmd, req, unit)\
  ((unit)->nim_Commands[cmd])(nid, (req), (unit))
#define Method(method, unit)\
  (*((unit)->nim_Methods->method))

/*
 * Instance allocated for each opener (stored to io_Unit)
 */
struct NetInfoPointer {
  struct Node        nip_Node[1];
  UBYTE              nip_Flags;
  UBYTE              nip_UnitNumber;
  struct NetInfoMap *nip_Map;		     /* backpointer */
  struct Ent        *nip_Ent;		     /* latest entry read */
};

#define nip_Name nip_Node->ln_Name

/* in server.c */
ASM LONG NetInfoStartup(void);
void NetInfoTask(struct NetInfoDevice *, struct Message *msg);
struct NetInfoMap *InitNetInfoMap(struct NetInfoDevice *, struct MsgPort *, ULONG);
void DeInitNetInfoMap(struct NetInfoDevice *, struct NetInfoMap *);
struct Unit *CreateNewUnit(struct NetInfoDevice *, short unit);
void ExpungeUnit(struct NetInfoDevice *, struct Unit *);
void PerformIO(struct NetInfoDevice *, struct NetInfoReq *req);
void TermIO(struct NetInfoReq *req);
ULONG AbortReq(struct NetInfoDevice *, struct List *, struct NetInfoReq *);

/* Common method */
void UnknownCommand(struct NetInfoDevice *, struct NetInfoReq *, struct NetInfoMap *);   

/* support functions */
struct NetInfoMap *CheckUnit(struct NetInfoDevice *, struct Unit *u);

#endif /* _BASE_H_ */
