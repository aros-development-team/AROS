#include <devices/timer.h>
#include <dos/dos.h>
#include <exec/memory.h>
#include <exec/ports.h>
#include <exec/types.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/timer.h>

#include "debug.h"
#include "fs.h"

#include "globals.h"
#include "asmsupport.h"

/* Internal prototypes */

#ifdef DEBUGCODE

#ifndef __AROS__
    extern void __asm TOSERIAL(register __a0 UBYTE *);
#endif

void BEGIN(void) {
  _DEBUG(("BEGIN..."));

  ReadEClock(&globals->ecv);
}

void END(UBYTE *name) {
  struct EClockVal ecv2;
  ULONG freq;
  LONG diff;

  freq=ReadEClock(&ecv2);

  diff=ecv2.ev_lo-globals->ecv.ev_lo;
  if(diff<0) {
    diff=-diff;
  }

  _DEBUG(("%s: ticks %ld (f=%ld)\n",name,diff,freq));
}

#ifndef __AROS__

    #ifndef DEBUGKPRINTF
    
      #ifndef DEBUG115200
    
        void debug(UBYTE *fmt, ... ) {
          struct MsgPort *port;
          struct Message *msg;
          UBYTE *dest;
          ULONG *args;
    
          Forbid();
          if((port=FindPort("SFS debug output port"))!=0) {
            if((msg=AllocVec(sizeof(struct Message)+512,MEMF_CLEAR))!=0) {
              dest=(UBYTE *)msg;
              dest+=sizeof(struct Message);
              args=(ULONG *)&fmt;
              args++;
    
              RawDoFmt(fmt,args,putChProc,dest);
    
              PutMsg(port,msg);
            }
          }
          Permit();
        }
    
    
    
        void tdebug(UBYTE *fmt, ... ) {
          struct MsgPort *port;
          struct Message *msg;
          struct DateStamp ds;
          UBYTE *dest;
          ULONG *args;
    
          DateStamp(&ds);
    
          debug("%4ld.%4ld ", ds.ds_Minute, ds.ds_Tick*2);
    
          Forbid();
          if((port=FindPort("SFS debug output port"))!=0) {
            if((msg=AllocVec(sizeof(struct Message)+512,MEMF_CLEAR))!=0) {
              dest=(UBYTE *)msg;
              dest+=sizeof(struct Message);
              args=(ULONG *)&fmt;
              args++;
    
              RawDoFmt(fmt,args,putChProc,dest);
    
              PutMsg(port,msg);
            }
          }
          Permit();
        }
    
    
    
        void xdebug(ULONG type,UBYTE *fmt, ... ) {
          struct MsgPort *port;
          struct Message *msg;
          UBYTE *dest;
          ULONG *args;
        //  ULONG debug=0xFFFFFFFE-DEBUG_CACHEBUFFER-DEBUG_NODES-DEBUG_IO-DEBUG_SEEK-DEBUG_LOCK-DEBUG_BITMAP;
        //  ULONG debugdetailed=0xFFFFFFFE-DEBUG_CACHEBUFFER-DEBUG_NODES-DEBUG_IO-DEBUG_SEEK-DEBUG_LOCK-DEBUG_BITMAP;
        //  ULONG debug=0xFFFFFFFE-DEBUG_CACHEBUFFER-DEBUG_NODES-DEBUG_LOCK-DEBUG_BITMAP;
        //  ULONG debugdetailed=0xFFFFFFFE-DEBUG_CACHEBUFFER-DEBUG_NODES-DEBUG_LOCK-DEBUG_BITMAP;
        //  ULONG debug=0xFFFFFFFE;
        //  ULONG debugdetailed=0xFFFFFFFE;
          ULONG debug=mask_debug;
          ULONG debugdetailed=mask_debug & ~(DEBUG_CACHEBUFFER|DEBUG_NODES|DEBUG_LOCK|DEBUG_BITMAP);
    
          if((debugdetailed & type)!=0 || ((type & 1)==0 && (debug & type)!=0)) {
            Forbid();
            if((port=FindPort("SFS debug output port"))!=0) {
              if((msg=AllocVec(sizeof(struct Message)+512,MEMF_CLEAR))!=0) {
                dest=(UBYTE *)msg;
                dest+=sizeof(struct Message);
                args=(ULONG *)&fmt;
                args++;
    
                RawDoFmt(fmt,args,putChProc,dest);
    
                PutMsg(port,msg);
              }
            }
            Permit();
          }
        }
    
      #else
    
        UBYTE serbuffer[500];
    
        void debug(UBYTE *fmt, ... ) {
          ULONG *args;
    
          args=(ULONG *)&fmt;
          args++;
    
          RawDoFmt(fmt,args,putChProc,serbuffer);
    
          Disable();
    
          TOSERIAL(serbuffer);
    
          Enable();
        }
    
    
    
        void xdebug(ULONG type,UBYTE *fmt, ... ) {
          ULONG *args;
        //  ULONG debug=0xFFFFFFFE-DEBUG_CACHEBUFFER-DEBUG_NODES-DEBUG_IO-DEBUG_SEEK-DEBUG_LOCK-DEBUG_BITMAP;
        //  ULONG debugdetailed=0xFFFFFFFE-DEBUG_CACHEBUFFER-DEBUG_NODES-DEBUG_IO-DEBUG_SEEK-DEBUG_LOCK-DEBUG_BITMAP;
        //  ULONG debug=0xFFFFFFFE-DEBUG_CACHEBUFFER-DEBUG_NODES-DEBUG_LOCK-DEBUG_BITMAP;
        //  ULONG debugdetailed=0xFFFFFFFE-DEBUG_CACHEBUFFER-DEBUG_NODES-DEBUG_LOCK-DEBUG_BITMAP;
        //  ULONG debug=0xFFFFFFFE;
        //  ULONG debugdetailed=0xFFFFFFFE;
          ULONG debug=mask_debug;
          ULONG debugdetailed=mask_debug & ~(DEBUG_CACHEBUFFER|DEBUG_NODES|DEBUG_LOCK|DEBUG_BITMAP);
    
          if((debugdetailed & type)!=0 || ((type & 1)==0 && (debug & type)!=0)) {
    
            args=(ULONG *)&fmt;
            args++;
    
            RawDoFmt(fmt,args,putChProc,serbuffer);
    
            Disable();
    
            TOSERIAL(serbuffer);
    
            Enable();
          }
        }
    
      #endif
    
    
    #else
    
    void xkprintf(ULONG type,const char *fmt, ... ) {
      UBYTE *str;
      UBYTE *dest;
      ULONG *args;
      ULONG debug=mask_debug;
      ULONG debugdetailed=mask_debug & ~(DEBUG_CACHEBUFFER|DEBUG_NODES|DEBUG_LOCK|DEBUG_BITMAP);
    
      if((debugdetailed & type)!=0 || ((type & 1)==0 && (debug & type)!=0)) {
        if((str=AllocVec(512,MEMF_CLEAR))!=0) {
          dest=str;
          args=(ULONG *)&fmt;
          args++;
    
          RawDoFmt((char *)fmt,args,putChProc,dest);
          kprintf("%s",dest);
        }
      }
    }
    
    #endif
#endif

#endif
