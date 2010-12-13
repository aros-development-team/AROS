/*
** This program uses the device interface to play a sampled sound.
** The input is read from THE DEFAULT INPUT, make sure you
** start it with "PlayTest pri < mysample.raw" !
** Where pri is a number from -128 to +127 (may be omitted)
** The sample should be 8 bit signed, mono (see TYPE).
**
** PLEASE NOTE that earlier versions of this example contained a bug
** that sometimes DeleteIORequest'ed a pointer that was AllocMem'ed!
*/

#include <devices/ahi.h>
#include <dos/dosasl.h>
#include <exec/memory.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/ahi.h>
#include <stdlib.h>

#define FREQUENCY  8000
#define TYPE       AHIST_M8S
#define BUFFERSIZE 20000

struct MsgPort    *AHImp     = NULL;
struct AHIRequest *AHIios[2] = {NULL,NULL};
struct AHIRequest *AHIio     = NULL;
APTR               AHIiocopy = NULL;
BYTE               AHIDevice = -1;

BYTE buffer1[BUFFERSIZE];
BYTE buffer2[BUFFERSIZE];

void cleanup(LONG rc)
{
  if(!AHIDevice)
    CloseDevice((struct IORequest *)AHIio);
  DeleteIORequest((struct IORequest *)AHIio);
  FreeMem(AHIiocopy,sizeof(struct AHIRequest));
  DeleteMsgPort(AHImp);
  exit(rc);
}

int main(int argc, char *argv[])
{
  BYTE *p1=buffer1,*p2=buffer2;
  void *tmp;
  ULONG signals,length;
  struct AHIRequest *link = NULL;
  LONG priority = 0;
  BYTE pri;

  if(argc == 2)
  {
    StrToLong(argv[1], &priority);
  }
  pri = priority;
  Printf("Sound priority: %ld\n", pri);

  if((AHImp=CreateMsgPort()) != NULL) {
    if((AHIio=(struct AHIRequest *)CreateIORequest(AHImp,sizeof(struct AHIRequest))) != NULL) {
      AHIio->ahir_Version = 4;
      AHIDevice=OpenDevice(AHINAME,0,(struct IORequest *)AHIio,0);
    }
  }

  if(AHIDevice) {
    Printf("Unable to open %s/0 version 4\n",AHINAME);
    cleanup(RETURN_FAIL);
  }

// Make a copy of the request (for double buffering)
  AHIiocopy = AllocMem(sizeof(struct AHIRequest), MEMF_ANY);
  if(! AHIiocopy) {
    cleanup(RETURN_FAIL);
  }
  CopyMem(AHIio, AHIiocopy, sizeof(struct AHIRequest));
  AHIios[0]=AHIio;
  AHIios[1]=AHIiocopy;

  SetIoErr(0);

  for(;;) {

// Fill buffer
    length = Read(Input(),p1,BUFFERSIZE);

// Play buffer
    AHIios[0]->ahir_Std.io_Message.mn_Node.ln_Pri = pri;
    AHIios[0]->ahir_Std.io_Command  = CMD_WRITE;
    AHIios[0]->ahir_Std.io_Data     = p1;
    AHIios[0]->ahir_Std.io_Length   = length;
    AHIios[0]->ahir_Std.io_Offset   = 0;
    AHIios[0]->ahir_Frequency       = FREQUENCY;
    AHIios[0]->ahir_Type            = TYPE;
    AHIios[0]->ahir_Volume          = 0x10000;          // Full volume
    AHIios[0]->ahir_Position        = 0x8000;           // Centered
    AHIios[0]->ahir_Link            = link;
    SendIO((struct IORequest *) AHIios[0]);

    if(link) {

// Wait until the last buffer is finished (== the new buffer is started)
      signals=Wait(SIGBREAKF_CTRL_C | (1L << AHImp->mp_SigBit));

// Check for Ctrl-C and abort if pressed
      if(signals & SIGBREAKF_CTRL_C) {
        SetIoErr(ERROR_BREAK);
        break;
      }

// Remove the reply and abort on error
      if(WaitIO((struct IORequest *) link)) {
        SetIoErr(ERROR_WRITE_PROTECTED);
        break;
      }
    }

// Check for end-of-sound, and wait until it is finished before aborting
    if(length != BUFFERSIZE) {
      WaitIO((struct IORequest *) AHIios[0]);
      break;
    }

    link = AHIios[0];

// Swap buffer and request pointers, and restart
    tmp    = p1;
    p1     = p2;
    p2     = tmp;

    tmp    = AHIios[0];
    AHIios[0] = AHIios[1];
    AHIios[1] = tmp;
  }
  

// Abort any pending iorequests
  AbortIO((struct IORequest *) AHIios[0]);
  WaitIO((struct IORequest *) AHIios[0]);

  if(link) { // Only if the second request was started
    AbortIO((struct IORequest *) AHIios[1]);
    WaitIO((struct IORequest *) AHIios[1]);
  }

  if(IoErr()) {
    PrintFault(IoErr(), argv[0] );
    cleanup(RETURN_ERROR);
  }

  cleanup(RETURN_OK);
  return RETURN_OK; // Make compiler happy
}
