/*
** This program uses the device interface to sample sound data.
** The output is written to THE DEFAULT OUTPUT, make sure you
** start it with "RecordTest > mysample.raw" !
*/

#include <devices/ahi.h>
#include <dos/dosasl.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/ahi.h>
#include <stdlib.h>

#define FREQUENCY 8000
#define TYPE       AHIST_M8S
#define BUFFERSIZE 20000

struct Library    *AHIBase;
struct MsgPort    *AHImp=NULL;
struct AHIRequest *AHIio=NULL;
BYTE               AHIDevice=-1;

BYTE buffer1[BUFFERSIZE];
BYTE buffer2[BUFFERSIZE];

void cleanup(LONG rc)
{
  if(!AHIDevice)
    CloseDevice((struct IORequest *)AHIio);
  DeleteIORequest((struct IORequest *)AHIio);
  DeleteMsgPort(AHImp);
  exit(rc);
}

void main(int argc, char *argv[])
{
  BYTE *p1=buffer1,*p2=buffer2,*tmp;
  ULONG signals;

  if(AHImp=CreateMsgPort()) {
    if(AHIio=(struct AHIRequest *)CreateIORequest(AHImp,sizeof(struct AHIRequest))) {
      AHIio->ahir_Version = 4;
      AHIDevice=OpenDevice(AHINAME,0,(struct IORequest *)AHIio,NULL);
    }
  }

  if(AHIDevice) {
    Printf("Unable to open %s/0 version 4\n",AHINAME);
    cleanup(RETURN_FAIL);
  }

// Initialize the first read
  AHIio->ahir_Std.io_Command=CMD_READ;
  AHIio->ahir_Std.io_Data=&buffer1;
  AHIio->ahir_Std.io_Length=BUFFERSIZE;
  AHIio->ahir_Std.io_Offset=0;
  AHIio->ahir_Frequency=FREQUENCY;
  AHIio->ahir_Type=TYPE;
  if(!DoIO((struct IORequest *) AHIio)) {

// The first buffer is now filled
    SetIoErr(NULL);

    for(;;) {
      ULONG length;
      
      length=AHIio->ahir_Std.io_Actual;

// Initialize the second read (note that io_Offset is not cleared!)
      AHIio->ahir_Std.io_Data=p2;
      AHIio->ahir_Std.io_Length=BUFFERSIZE;
      AHIio->ahir_Frequency=FREQUENCY;
      AHIio->ahir_Type=TYPE;
      SendIO((struct IORequest *) AHIio);

// While the second read is in progress, save the first buffer to stdout
      if(Write(Output(),p1,length) != length) {
        break;
      }

      signals=Wait(SIGBREAKF_CTRL_C | (1L << AHImp->mp_SigBit));

      if(signals & SIGBREAKF_CTRL_C) {
        SetIoErr(ERROR_BREAK);
        break;
      }

// Remove the reply 
      if(WaitIO((struct IORequest *) AHIio)) {
        SetIoErr(ERROR_READ_PROTECTED);
        break;
      }

// Swap buffer pointers and repeat
      tmp=p1;
      p1=p2;
      p2=tmp;
    }

// Abort any pending iorequests
    AbortIO((struct IORequest *) AHIio);
    WaitIO((struct IORequest *) AHIio);
  }

  if(IoErr())
  {
    PrintFault(IoErr(), argv[0] ); // Oh, common! It's not MY fault that this
                                   // routine prints to stdout instead of stderr!
    cleanup(RETURN_ERROR);
  }

  cleanup(RETURN_OK);
}
