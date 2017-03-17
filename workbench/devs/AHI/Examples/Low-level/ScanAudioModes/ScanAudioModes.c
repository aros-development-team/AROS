
/* Shows the current Audio database in memory */

#include <dos/dos.h>
#include <devices/ahi.h>
#include <utility/tagitem.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/ahi.h>

#define LEN 80

LONG __OSlibversion=37;

UBYTE vers[] = "\0$VER: ScanAudioModes 1.1 (28.4.96)";

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

int main(void)
{
  ULONG id=AHI_INVALID_ID;
  LONG volume,stereo,panning,hifi,pingpong,record,realtime,fullduplex,
       bits,channels,minmix,maxmix,freqs,inputs,outputs;
  Fixed minmon,maxmon,mingain,maxgain,minout,maxout;
  UBYTE name[LEN],driver[LEN],author[LEN],copyright[LEN],version[LEN],anno[LEN],
        inputdesc[LEN],outputdesc[LEN];
  LONG i;

  if((AHImp=CreateMsgPort()))
    if((AHIio=(struct AHIRequest *)CreateIORequest(AHImp,sizeof(struct AHIRequest)))) {
      AHIio->ahir_Version=1;
      AHIDevice=OpenDevice(AHINAME,AHI_NO_UNIT,(struct IORequest *)AHIio,NULL);
      }

  if(AHIDevice) {
    Printf("Unable to open %s version 1\n",(IPTR) AHINAME);
    cleanup(RETURN_FAIL);
  }
  AHIBase=(struct Library *)AHIio->ahir_Std.io_Device;

  while ((id=AHI_NextAudioID(id)) != AHI_INVALID_ID) {
    if(SetSignal(0L,SIGBREAKF_CTRL_C) & SIGBREAKF_CTRL_C)
    {
      Printf("***Break\n");
      break;
    }

// Clear all 
    volume=0;stereo=0;panning=0;hifi=0;pingpong=0;record=0;realtime=0;fullduplex=0;
    bits=0;channels=0;minmix=0;maxmix=0;freqs=0;inputs=0;outputs=0;
    name[0]='\0'; driver[0]='\0'; author[0]='\0'; copyright[0]='\0'; version[0]='\0';
    anno[0]='\0'; inputdesc[0]='\0'; outputdesc[0]='\0';

    Printf("\x1b[2mMode 0x%08lx\x1b[0m\n",id);
    AHI_GetAudioAttrs(id, NULL,
                          AHIDB_BufferLen,LEN,
                          AHIDB_Frequencies,(IPTR) &freqs,
                          AHIDB_Volume, (IPTR) &volume,
                          AHIDB_Stereo, (IPTR) &stereo,
                          AHIDB_Panning, (IPTR) &panning,
                          AHIDB_HiFi,(IPTR) &hifi,
                          AHIDB_PingPong,(IPTR) &pingpong,
                          AHIDB_Record,(IPTR) &record,
                          AHIDB_FullDuplex,(IPTR) &fullduplex,
                          AHIDB_Realtime,(IPTR) &realtime,
                          AHIDB_Name,(IPTR) &name,
                          AHIDB_Driver,(IPTR) &driver,
                          AHIDB_Bits,(IPTR) &bits,
                          AHIDB_MaxChannels,(IPTR) &channels,
                          AHIDB_MinMixFreq,(IPTR) &minmix,
                          AHIDB_MaxMixFreq,(IPTR) &maxmix,
                          AHIDB_Author,(IPTR) &author,
                          AHIDB_Copyright,(IPTR) &copyright,
                          AHIDB_Version,(IPTR) &version,
                          AHIDB_Annotation,(IPTR) &anno,
                          AHIDB_MinMonitorVolume,(IPTR) &minmon,
                          AHIDB_MaxMonitorVolume,(IPTR) &maxmon,
                          AHIDB_MinInputGain,(IPTR) &mingain,
                          AHIDB_MaxInputGain,(IPTR) &maxgain,
                          AHIDB_MinOutputVolume,(IPTR) &minout,
                          AHIDB_MaxOutputVolume,(IPTR) &maxout,
                          AHIDB_Inputs,(IPTR) &inputs,
                          AHIDB_Outputs,(IPTR) &outputs,
                          TAG_DONE);
    Printf(" Name: \"%s\"   Driver: \"DEVS:AHI/%s.audio\"\n",
	   (IPTR) name, (IPTR) driver);
    if(author[0])
      Printf(" Driver programmed by: %s\n",(IPTR) author);
    if(copyright[0])
      Printf(" Copyright: %s\n",(IPTR) copyright);
    for(i=0;i<LEN;i++)
      if(version[i]=='\n' || version[i]=='\r')
        version[i]='\0';
    if(version[0])
      Printf(" Version: %s\n",(IPTR) version);
    if(anno[0])
      Printf(" %s\n",(IPTR) anno);
    Printf(" The hardware has %ld output(s)",outputs);
    if(outputs>0)
    {
      Printf(":");
      for(i=0;i<outputs;i++)
      {
        AHI_GetAudioAttrs(id, NULL,
            AHIDB_BufferLen,LEN,
            AHIDB_OutputArg,i,
            AHIDB_Output,(IPTR) &outputdesc,
	    TAG_DONE);
        Printf(" %s",(IPTR) &outputdesc);
      }
    }
    Printf("\n and %ld input(s)",inputs);
    if(inputs>0)
    {
      Printf(":");
      for(i=0;i<inputs;i++)
      {
        AHI_GetAudioAttrs(id, NULL,
            AHIDB_BufferLen,LEN,
            AHIDB_InputArg,i,
            AHIDB_Input,(IPTR) &inputdesc,
	    TAG_DONE);
        Printf(" %s",(IPTR) &inputdesc);
      }
    }
    Printf(".\n");
    if(minmon==maxmon)
      Printf(" No input monitor.\n");
    else
      Printf(" Input monitor volume range: %ld-%ld%%.\n",minmon*100>>16,maxmon*100>>16);
    if(mingain==maxgain)
      Printf(" No input gain.\n");
    else
      Printf(" Input gain range: %ld-%ld%%.\n",mingain*100>>16,maxgain*100>>16);
    if(minout==maxout)
      Printf(" No volume control.\n");
    else
      Printf(" Output volume range: %ld-%ld%%.\n",minout*100>>16,maxout*100>>16);

    Printf("\n %s output%s in %ld bits.\n", (IPTR) (stereo ? "Stereo" : "Mono"),
              (IPTR) (stereo ? (panning ? " (with panning)" : "" ) : ""),
              bits);
    Printf(" Can play samples on max %ld channels at %s, %s.\n", channels,
              (IPTR) (volume ? "any volume" : "full volume only"),
              (IPTR) (pingpong ? "both forwards and backwards" : "forwards only"));
    Printf(" The mixing %sdestructive.\n", (IPTR) (hifi ? "is non-" : "may be "));
    Printf(" %ld mixing frequencies between %ld and %ld are available.\n",freqs,minmix,maxmix);
    Printf(" Recording is %ssupported", (IPTR) (record ? "" : "not "));
    if(record)
      Printf(" (%s duplex)", (IPTR) (fullduplex ? "Full" : "Half"));
    Printf(".\n");
    Printf(" This is a %srealtime mode.\n", (IPTR) (realtime ? "" : "non-"));
   
    Printf("\n");
  }

  Printf("No more modes in the audio database.\n");
  cleanup(RETURN_OK);
  return RETURN_OK; // Make compiler happy
}
