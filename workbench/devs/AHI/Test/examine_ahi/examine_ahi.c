

#include <stdio.h>
#include <exec/execbase.h>
#include <proto/exec.h>

#include "ahi_def.h"
//#include "ahi_device.h"

extern struct ExecBase *SysBase;

void printlist(struct MinList *);

int main(void)
{
  struct AHIBase *AHIBase;
  struct AHIDevUnit *iounit;

  Disable();
  AHIBase = (struct AHIBase *) FindName(& SysBase->DeviceList, AHINAME);
  Enable();

  printf("Base: 0x%08lx\n", AHIBase);
  if(AHIBase == NULL)
    return -1;

  iounit = AHIBase->ahib_DevUnits[0];
  printf("iounit 0: 0x%08lx\n", iounit);
  if(iounit != NULL) {

    if(iounit->IsPlaying)
      printf("Is playing.\n");
    
    if(iounit->IsRecording)
      printf("Is recording.\n");

    printf("ReadList\n");
    printlist(&iounit->ReadList);
    printf("PlayingList\n");
    printlist(&iounit->PlayingList);
    printf("SilentList\n");
    printlist(&iounit->SilentList);
    printf("WaitingList\n");
    printlist(&iounit->WaitingList);

    printf("S: %ld, R: %ld, R:%ld\n", iounit->SampleSignal,
        iounit->RecordSignal, iounit->PlaySignal);
  }

  iounit = AHIBase->ahib_DevUnits[1];
  printf("iounit 1: 0x%08lx\n", iounit);
  if(iounit != NULL) {

    if(iounit->IsPlaying)
      printf("Is playing.\n");
    
    if(iounit->IsRecording)
      printf("Is recording.\n");

    printf("ReadList\n");
    printlist(&iounit->ReadList);
    printf("PlayingList\n");
    printlist(&iounit->PlayingList);
    printf("SilentList\n");
    printlist(&iounit->SilentList);
    printf("WaitingList\n");
    printlist(&iounit->WaitingList);

    printf("S: %ld, R: %ld, R:%ld\n", iounit->SampleSignal,
        iounit->RecordSignal, iounit->PlaySignal);
  }

  return 0;
}

const static char * commands[] =
{
  "CMD_INVALID",
  "CMD_RESET",
  "CMD_READ",
  "CMD_WRITE",
  "CMD_UPDATE",
  "CMD_CLEAR",
  "CMD_STOP",
  "CMD_START",
  "CMD_FLUSH"
};

void printlist(struct MinList *list)
{
  struct AHIRequest *ioreq;

  ioreq = (struct AHIRequest *) list->mlh_Head;
  while (ioreq->ahir_Std.io_Message.mn_Node.ln_Succ)
  {
    printf("iorequest: 0x%08lx\n", ioreq);
    printf("command  : %s (0x%lx)\n",
        (ioreq->ahir_Std.io_Command < CMD_NONSTD ?
            commands[ioreq->ahir_Std.io_Command] :
            "Annan..."),
        ioreq->ahir_Std.io_Command);
    ioreq = (struct AHIRequest *) ioreq->ahir_Std.io_Message.mn_Node.ln_Succ;
  }
}
