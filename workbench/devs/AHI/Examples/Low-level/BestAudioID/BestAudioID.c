/* Simple AHI_BestAudioID() example */

#include <devices/ahi.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/ahi.h>
#include <stdlib.h>
#include <string.h>

struct Library    *AHIBase;
struct MsgPort    *AHImp=NULL;
struct AHIRequest *AHIio=NULL;
BYTE               AHIDevice=-1;

void cleanup(LONG rc)
{
  if(!AHIDevice)
    CloseDevice((struct IORequest *)AHIio);
  DeleteIORequest((struct IORequest *)AHIio);
  DeleteMsgPort(AHImp);
  exit(rc);
}

void printmode(ULONG id) {
  char name[40];

  if(id != AHI_INVALID_ID) {
    name[0] = '\0';
    AHI_GetAudioAttrs(id, NULL,
        AHIDB_BufferLen, 40,
        AHIDB_Name, (ULONG) name,
        TAG_DONE);
  }
  else {
    strncpy(name, "Non-existing mode!", 40);
  }

  Printf("\nMode 0x%08lx (%s):\n",  id, name);
}


void main(void)
{
  ULONG mainid, id;

  if(AHImp=CreateMsgPort())
    if(AHIio=(struct AHIRequest *)CreateIORequest(AHImp,sizeof(struct AHIRequest))) {
      AHIio->ahir_Version = 4;
      AHIDevice=OpenDevice(AHINAME,AHI_NO_UNIT,(struct IORequest *)AHIio,NULL);
      }

  if(AHIDevice) {
    Printf("Unable to open %s version 4\n",AHINAME);
    cleanup(RETURN_FAIL);
  }
  AHIBase=(struct Library *)AHIio->ahir_Std.io_Device;

  mainid = AHI_BestAudioID(AHIDB_Stereo,TRUE,
                     AHIDB_Volume,TRUE,
                     AHIDB_Bits,8,
                     AHIDB_MaxChannels,4,
                     TAG_DONE);
  printmode(mainid);
  Printf("Stereo, Volume, >= 8 bits and >= 4 channels.\n");

  {
    struct TagItem tags[] = {
      AHIDB_Bits, 12,
      AHIDB_Panning, TRUE,
      AHIDB_HiFi, TRUE,
      TAG_DONE
    };
    id = AHI_BestAudioID(AHIDB_Stereo,TRUE,
                       AHIDB_Volume,TRUE,
                       AHIDB_Bits,8,
                       AHIDB_MaxChannels,4,
                       AHIB_Dizzy, (ULONG) &tags,
                       TAG_DONE);
    printmode(id);
    Printf("Same as 0x%08lx but also, if possible, "
           ">= 12 bits, free stereo panning and HiFi mixing.\n", mainid);
  }

  {
    struct TagItem tags[] = {
      AHIDB_Bits, 16,
      AHIDB_Stereo, FALSE,
      TAG_DONE
    };

    id = AHI_BestAudioID(AHIDB_AudioID,id,
                       AHIB_Dizzy, (ULONG) &tags,
                       TAG_DONE);

    printmode(id);
    Printf("Using the same audio hardware at last mode, "
           "this modes may have >= 16 bit resolution and be mono. But "
           "I'm not sure.\n");
  }

  id = AHI_BestAudioID(AHIDB_Realtime, FALSE,
                     AHIDB_MaxChannels, 4,
                    TAG_DONE);
  printmode(id);
  Printf("A non-realtime mode with 4 channels.\n");

  cleanup(0);
}
