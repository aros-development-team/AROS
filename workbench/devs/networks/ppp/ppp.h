/*
 * $Id$
 */

#ifndef _PPP_H
#define _PPP_H

#define DEBUG 1

#include <exec/types.h>
#include <exec/libraries.h>
#include <exec/semaphores.h>
#include <exec/devices.h>
#include <exec/interrupts.h>
#include <dos/bptr.h>

#include <oop/oop.h>

#include <hidd/pci.h>
#include <hidd/irq.h>

#include <devices/timer.h>
#include <devices/sana2.h>
#include <devices/sana2specialstats.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <aros/debug.h>

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

#include LC_LIBDEFS_FILE


/*
** Maximum Transmission Unit
*/
#define PPP_MTU 1500

/*
** Max # of Units allowed
*/
#define SD_MAXUNITS 1

#define PPP_MAXBUFF 4096

// phases
#define PPP_PHASE_DEAD 1
#define PPP_PHASE_CONFIGURATION 2
#define PPP_PHASE_AUTHENTICATION 3
#define PPP_PHASE_PROTOCOL_CONF 4
#define PPP_PHASE_NETWORK 5
#define PPP_PHASE_TERMINATE 6

// config stuff
#define PPP_MAXATCOM 30       
#define PPP_MAXARGLEN 100
#define COM_WAIT 1
#define COM_SEND 2
#define COM_DELAY 3

struct at_command {
  BYTE command,arg;     
  BYTE str[PPP_MAXARGLEN];
};  
    
    
struct PPPBase {
    struct Device sd_Device;
    ULONG        sd_OpenCnt;
    UBYTE        sd_Flags;
    
    BOOL ppp_online;
    BOOL serial_ok;
    BOOL device_up;
    BOOL sdu_Proc_run;
    
    struct Unit     *sd_Unit;
    struct SignalSemaphore sd_Lock;
    
    struct Process      *sdu_Proc;
      
    struct MsgPort     *TimeMsg;
    struct timerequest *TimeReq;
    
    struct IOExtSer     *sdu_SerRx;     /* Serial IORequest for CMD_READ's */
    struct IOExtSer     *sdu_SerTx;     /* Serial IORequest for CMD_WRITE's */
    struct MsgPort      *sdu_RxPort;    /* Serial CMD_READ IORequest reply port */
    struct MsgPort      *sdu_TxPort;    /* Serial CMD_WRITE IORequest reply port */
    
    UBYTE           *sdu_RxBuff;        /* Buffer for holding incoming data */
    UBYTE           *sdu_TxBuff;        /* Buffer for hold outgoing packets */
    
    BYTE DeviceName[PPP_MAXARGLEN];
    BYTE SerUnitNum;
    struct at_command atc[PPP_MAXATCOM]; 
    
    BYTE username[PPP_MAXARGLEN]; 
    BYTE password[PPP_MAXARGLEN];
    BOOL enable_dns;
    
    BOOL        (*CopyFromBuffer)(APTR, APTR, ULONG);
    BOOL        (*CopyToBuffer)(APTR, APTR, ULONG);
    
    struct SignalSemaphore   sdu_ListLock;      /* A Semaphore for access to all of our queues. */
    struct MinList       sdu_Rx;                /* Pending CMD_READ's */
    struct MinList       sdu_Tx;                /* Pending CMD_WRITE's */
    
};
 
 

#endif

