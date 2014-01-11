#ifndef INETD_H
#define INETD_H \
 	"$Id$"
/*
 *      Internet daemon interface definitions
 *
 *      Copyright © 1994 AmiTCP/IP Group,
 *                       Network Solutions Development, Inc.
 *                       All rights reserved.
 */

/* 
 * The DaemonPort structure and its associated memory
 * must be allocated with AllocVec call 
 */
struct DaemonPort {
  struct MsgPort dp_Port;
  void         (*dp_ExitCode)(void);
};

#define DAEMONPORTNAME "inetd.ipc"

/*
 * A message associated with each launched process 
 */
struct DaemonMessage {
  struct Message dm_Msg;	/* Message name is FreeVec()'ed by inetd */
  struct Process*dm_Pid;	/* set by the launcher */
  struct Segment*dm_Seg;	/* used only if resident segment */
  LONG           dm_Id;		/* socket id */
  LONG		 dm_Retval;	/* non-zero errorcode */
  UBYTE          dm_Family;	/* address/protocol family */
  UBYTE          dm_Type;
};

/* Daemon types, used as socket types */
#define DMTYPE_UNKNOWN   -1
#define DMTYPE_INTERNAL  0		/* type is within builtin struct */
#define DMTYPE_STREAM    SOCK_STREAM	/* stream socket */
#define DMTYPE_DGRAM     SOCK_DGRAM	/* datagram socket */
#define DMTYPE_RAW       SOCK_RAW	/* raw-protocol interface */
#define DMTYPE_RDM       SOCK_RDM	/* reliably-delivered message */
#define DMTYPE_SEQPACKET SOCK_SEQPACKET /* sequenced packet stream */
 
/* Return values from the startup code */
#define DERR_LIB    0xA0
#define DERR_OBTAIN 0xA1

#endif /* INETD_H */
