
/* Demo of AHI's Audio mode requester */

#include <devices/ahi.h>
#include <dos/dos.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/ahi.h>
//#include <stdio.h>
#include <stdlib.h>

struct Library    *AHIBase;
struct MsgPort    *AHImp=NULL;
struct AHIRequest *AHIio=NULL;
BYTE   AHIDevice=-1;

struct TagItem ReqFilterTags[] = {
//    {AHIDB_Realtime, TRUE},   // Remove the FILESAVE modes (among others?)
    {TAG_DONE,}
  };

void cleanup(LONG rc)
{
  if(!AHIDevice)
    CloseDevice((struct IORequest *)AHIio);
  DeleteIORequest((struct IORequest *)AHIio);
  DeleteMsgPort(AHImp);
  exit(rc);
}

void main(void)
{
  struct AHIAudioModeRequester *req;
  BOOL res;

  if(AHImp=CreateMsgPort())
    if(AHIio=(struct AHIRequest *)CreateIORequest(AHImp,sizeof(struct AHIRequest))) {
      AHIio->ahir_Version = 2;
      AHIDevice=OpenDevice(AHINAME,AHI_NO_UNIT,(struct IORequest *)AHIio,NULL);
      }

  if(AHIDevice) {
    Printf("Unable to open %s version 2\n",AHINAME);
    cleanup(RETURN_FAIL);
  }
  AHIBase=(struct Library *)AHIio->ahir_Std.io_Device;

  req=AHI_AllocAudioRequest(
      AHIR_SleepWindow,TRUE,
      AHIR_UserData,999,
      AHIR_PubScreenName,NULL,
      TAG_DONE);

  res=AHI_AudioRequest(req,
      AHIR_TitleText,       "Select a mode or cancel",
      AHIR_NegativeText,    "Abort",
      AHIR_DoMixFreq,       TRUE,
      AHIR_DoDefaultMode,   TRUE,
      AHIR_InitialAudioID,  0x20003,
      AHIR_InitialMixFreq,  30000,
      AHIR_FilterTags,      ReqFilterTags,
      TAG_DONE);

  if(!res)
  {
    if(IoErr() == ERROR_NO_FREE_STORE)
      Printf("AHI ran out of memory!\n");
    else if(IoErr() == ERROR_NO_MORE_ENTRIES)
      Printf("No available modes!\n");
    else
     Printf("Requester cancelled!\n");
  }
  else
    Printf("Selected AudioMode: 0x%08lx, %ld Hz\n",req->ahiam_AudioID,req->ahiam_MixFreq);

  AHI_FreeAudioRequest(req);

  cleanup(RETURN_OK);
}
