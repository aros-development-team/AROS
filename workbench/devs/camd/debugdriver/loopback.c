/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: CAMD driver whose ports receive what is sent to them.
*/

/*
  Whatever is sent to loopback.out.<n> arrives at loopback.in.<n>, so two
  programs can talk through a port without hardware, and the receiving side
  of CAMD can be tested.

  The bytes are handed over in the task that sends them. A port only loops
  while it is open, which CAMD arranges for as long as something is linked
  to either of its clusters.
*/

#include <proto/exec.h>
#include <exec/types.h>
#include <midi/camddevices.h>
#include <libcore/compiler.h>

#define NUMPORTS 2

struct ExecBase *SysBase;


int main(void){
  /* A camd mididriver is not supposed to be run directly, so we return an error. */
  return -1;
}


/*    Prototypes    */

SAVEDS ASM BOOL Init(REG(a6) APTR sysbase);
void Expunge(void);
SAVEDS ASM struct MidiPortData *OpenPort(
                                         REG(a3) struct MidiDeviceData *data,
                                         REG(d0) LONG portnum,
                                         REG(a0) ULONG (* ASM transmitfunc)(APTR REG(a2) userdata),
                                         REG(a1) void (* ASM receivefunc)(UWORD REG(d0) input,APTR REG(a2) userdata),
                                         REG(a2) APTR userdata
                                         );
ASM void ClosePort(
                   REG(a3) struct MidiDeviceData *data,
                   REG(d0) LONG portnum
                   );

/*   End prototypes  */


const struct MidiDeviceData mididevicedata={
  MDD_Magic,
  "loopback",
  "Loopback V1.0 (c) 2026 AROS - The AROS Research OS",
  1,
  0,
  Init,
  Expunge,
  OpenPort,
  ClosePort,
  NUMPORTS,
  1
};

SAVEDS ASM BOOL Init(REG(a6) APTR sysbase){
  SysBase=sysbase;
  return TRUE;
}

void Expunge(void){
  return;
}


struct LoopPort{
  ULONG (ASM *transmit)(REG(a2) APTR userdata);
  void (ASM *receive)(REG(d0) UWORD input,REG(a2) APTR userdata);
  APTR userdata;
};

static struct LoopPort ports[NUMPORTS];


/* Takes every byte CAMD has for the port and gives it straight back. */
SAVEDS ASM void ActivateXmit(REG(a2) APTR userdata,ULONG REG(d0) portnum){
  struct LoopPort *port;
  ULONG data;

  if(portnum>=NUMPORTS) return;
  port=&ports[portnum];
  if(port->transmit==NULL) return;

  for(;;){
    data=(port->transmit)(port->userdata);
    if(data==0x100) return;
    (port->receive)((UWORD)data,port->userdata);
  }
}

struct MidiPortData midiportdata={
  ActivateXmit
};

SAVEDS ASM struct MidiPortData *OpenPort(
                                         REG(a3) struct MidiDeviceData *data,
                                         REG(d0) LONG portnum,
                                         REG(a0) ULONG (* ASM transmitfunc)(APTR REG(a2) userdata),
                                         REG(a1) void (* ASM receivefunc)(UWORD REG(d0) input,APTR REG(a2) userdata),
                                         REG(a2) APTR userdata
                                         ){
  if(portnum<0 || portnum>=NUMPORTS) return NULL;

  ports[portnum].userdata=userdata;
  ports[portnum].receive=receivefunc;
  ports[portnum].transmit=transmitfunc;
  return &midiportdata;
}

ASM void ClosePort(
                   REG(a3) struct MidiDeviceData *data,
                   REG(d0) LONG portnum
                   ){
  if(portnum<0 || portnum>=NUMPORTS) return;

  ports[portnum].transmit=NULL;
}
