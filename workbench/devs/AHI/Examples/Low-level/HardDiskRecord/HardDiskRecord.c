/* This is a quickly made example of how you can use
   AHI to record AIFF/AIFC samples directly to hard disk.
   Like always, it's pooly documentated and quite messy... Sorry.
   This source and its executable is Public Domain.
   Feel free to base any own work on it.
   Done by Martin Blom in 1996.

   Notes: Keep your hands away from the mouse and keyboard while the program
   is recording. If the countdown gadget cannot be updated, the program will
   stall and you'll miss samples.

   Yeah, yeah, I know the program sucks. It's just an example.

*/

#define NO_PROTOS
#define NO_SAS_PRAGMAS
#include <iffp/8svx.h>
#undef NO_PROTOS
#undef NO_SAS_PRAGMAS

#include <exec/lists.h>
#include <exec/memory.h>
#include <dos/dos.h>
#include <devices/ahi.h>
#include <intuition/intuition.h>
#include <intuition/gadgetclass.h>
#include <libraries/asl.h>
#include <libraries/gadtools.h>
#include <proto/ahi.h>
#include <proto/asl.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/gadtools.h>

#include <stdio.h>
#include <math.h>
#include "GUI.h"
#include "GUI.extras.h"

#include "HardDiskRecord.h"

#define VERSION_STRING {"\0$VER: HardDiskRecord 1.1 "__AMIGADATE__"\r\n"}
static char VerString[] = VERSION_STRING;

//extern void KPrintF(char *fmt,...);

struct Library    *AHIBase;
struct MsgPort    *AHImp=NULL;
struct AHIRequest *AHIio=NULL;
BYTE               AHIDevice=-1;
struct AHIAudioModeRequester *amreq=NULL;
struct FileRequester *filereq=NULL;
struct AHIAudioCtrl *actrl=NULL;

struct EasyStruct req = {
  sizeof (struct EasyStruct),
  0,
  NULL,
  NULL,
  NULL,
  };

char directory[1024],filename[108],fullname[1024];
BPTR file=NULL;

#define STRINGNODE_ID 100
#define STRINGNODE_LENGTH 32
struct StringNode
{
  struct  Node sn_Node;
  char    sn_String[STRINGNODE_LENGTH];
};
struct List InputList,OutputList;
char   amtext1[32],amtext2[32];

ULONG  audioid=AHI_INVALID_ID,recfreq=0;
ULONG  source=0,dest=0,vol=0,gain=100,duration=0;
ULONG  type=1; //type: 0 = AIFF, 1 = AIFC 
ULONG  stereo=0; //stereo: 0=no, 1 = yes
ULONG  samplesize=0;

struct TagItem filtertags[]=
{
  AHIDB_Record,TRUE,
  TAG_DONE
};

struct {
  ULONG                 FORMid;
  ULONG                 FORMsize;
  ULONG                 AIFCid;

  ULONG                 FVERid;
  ULONG                 FVERsize;
  FormatVersionHeader   FVERchunk;

  ULONG                 COMMid;
  ULONG                 COMMsize;
  ExtCommonChunk        COMMchunk;

  ULONG                 SSNDid;
  ULONG                 SSNDsize;
  SampledSoundHeader    SSNDchunk;
} AIFCheader = 
  { // All NULLs will be filled later.
    ID_FORM,NULL,ID_AIFC,
    ID_FVER,sizeof(FormatVersionHeader),{AIFCVersion1},
    ID_COMM,sizeof(ExtCommonChunk),{NULL,NULL,16,{NULL},NO_COMPRESSION,
        sizeof("not compressed")-1,'n','o','t',' ','c','o','m','p','r','e','s','s','e','d'},
    ID_SSND,NULL,{0,0}
  };
struct {
  ULONG                 FORMid;
  ULONG                 FORMsize;
  ULONG                 AIFFid;

  ULONG                 COMMid;
  ULONG                 COMMsize;
  CommonChunk           COMMchunk;

  ULONG                 SSNDid;
  ULONG                 SSNDsize;
  SampledSoundHeader    SSNDchunk;
} AIFFheader = 
  { // All NULLs will be filled later.
    ID_FORM,NULL,ID_AIFF,
    ID_COMM,sizeof(CommonChunk),{NULL,NULL,16,{NULL}},
    ID_SSND,NULL,{0,0}
  };

struct {
  BYTE  signal,pad;
  ULONG signalflag;
  WORD *buffer1;
  WORD *buffer2;
  ULONG bufferlen;
  ULONG count;
  ULONG offs;
  APTR  task;
} RecordData;

/*
  The RecordFunc is written in assembler in order to
  keep the CPU usage as low as possible. You are advised
  to do the same.

__asm __saveds ULONG RecordFuncS(register __a0 struct Hook *hook,
    register __a2 struct AHIAudioCtrl *actrl,
    register __a1 struct AHIRecordMessage *chan)
{
  return NULL;
}
*/

extern ULONG RecordFuncS();
extern ULONG RecordFuncM();

struct Hook recordhook = 
{
  0,0,
  NULL,
  NULL,
  &RecordData,
};

BOOL openAHI(void);
void closeAHI(void);
void updatetopgadgets(void);
long IDCMPhandler(void);
void calcsamplesize(void);
BOOL ProcessWindowWin0( LONG Class, UWORD Code, APTR IAddress );
void record(void);
void ulong2extended (ULONG in, extended *ex);

int main(void)
{
  struct StringNode *node;

  NewList(&InputList);
  NewList(&OutputList);

  if (OpenLibs()==0)
  {
    if(openAHI())
    {
      OpenHardDiskRecordCatalog(NULL,NULL);
      if (OpenWindowWin0("")==0)
      {
        if(filereq=AllocAslRequestTags(ASL_FileRequest,
            ASLFR_Window,Win0,
            ASLFR_SleepWindow,TRUE,
            ASLFR_DoSaveMode,TRUE,
            ASLFR_RejectIcons,TRUE,
            TAG_DONE))
        {
          WindowLimits(Win0,200,Win0->BorderTop,0,0);
          updatetopgadgets();
          calcsamplesize();
          IDCMPhandler();
          FreeAslRequest(filereq);
        }
        CloseWindowWin0();
      }
      else
        Printf("Cannot open window.\n");
      CloseHardDiskRecordCatalog();
      closeAHI();
    }
    else
      Printf("Cannot open ahi.device\n");
    CloseLibs();
  }
  else
    Printf("Cannot open libraries.\n");


  // These nodes are allocated in updatetopgadgets().
  while(node=(struct StringNode *) RemHead(&InputList))
    FreeVec(node);
  while(node=(struct StringNode *) RemHead(&OutputList))
    FreeVec(node);
}

BOOL openAHI(void)
{
  if(AHImp=CreateMsgPort())
  {
    if(AHIio=(struct AHIRequest *)CreateIORequest(AHImp,sizeof(struct AHIRequest)))
    {
      AHIio->ahir_Version=2;
      if((AHIDevice=OpenDevice(AHINAME,AHI_NO_UNIT,(struct IORequest *)AHIio,NULL)) == NULL)
      {
        AHIBase=(struct Library *)AHIio->ahir_Std.io_Device;
        if(amreq=AHI_AllocAudioRequest(TAG_DONE))
        {
          return TRUE;
        }
      }
    }
  }
  return FALSE;
}

void closeAHI(void)
{
  if(amreq)
    AHI_FreeAudioRequest(amreq);
  if(!AHIDevice)
    CloseDevice((struct IORequest *)AHIio);
  DeleteIORequest((struct IORequest *)AHIio);
  DeleteMsgPort(AHImp);
}


void updatetopgadgets(void)
{
  long i;
  long inputs=0, outputs=0;
  long minmon=0x00000, maxmon=0x00000, mingain=0x10000, maxgain=0x10000;
  struct StringNode *node;

  amtext2[0]='\0';
  sprintf(amtext1,GetString(SelectAudioID));

  AHI_GetAudioAttrs(audioid,NULL,
      AHIDB_BufferLen,32,
      AHIDB_Name,amtext1,
      AHIDB_Inputs,&inputs,
      AHIDB_Outputs,&outputs,
      AHIDB_MinMonitorVolume,&minmon,
      AHIDB_MaxMonitorVolume,&maxmon,
      AHIDB_MinInputGain,&mingain,
      AHIDB_MaxInputGain,&maxgain,
      TAG_DONE);

  if(recfreq)
    sprintf(amtext2,"%ld Hz",recfreq);

  // Create input and output lists

    // Free old lists first
  while(node=(struct StringNode *) RemHead(&InputList))
    FreeVec(node);
  while(node=(struct StringNode *) RemHead(&OutputList))
    FreeVec(node);

    // Add new nodes
  for(i=0;i<inputs;i++)
  {
    if(node=AllocVec(sizeof(struct StringNode),MEMF_CLEAR))
    {
      AHI_GetAudioAttrs(audioid,NULL,
        AHIDB_BufferLen,STRINGNODE_LENGTH,
        AHIDB_InputArg,i,
        AHIDB_Input,node->sn_String,
        TAG_DONE);
      node->sn_Node.ln_Name=node->sn_String;
      node->sn_Node.ln_Type=STRINGNODE_ID;
      AddTail(&InputList,(struct Node *) node);
    }
  }
  for(i=0;i<outputs;i++)
  {
    if(node=AllocVec(sizeof(struct StringNode),MEMF_CLEAR))
    {
      AHI_GetAudioAttrs(audioid,NULL,
        AHIDB_BufferLen,STRINGNODE_LENGTH,
        AHIDB_OutputArg,i,
        AHIDB_Output,node->sn_String,
        TAG_DONE);
      node->sn_Node.ln_Name=node->sn_String;
      node->sn_Node.ln_Type=STRINGNODE_ID;
      AddTail(&OutputList,(struct Node *) node);
    }
  }

  // Set gadget attributes

  GT_SetGadgetAttrs(Win0Gadgets[Win0_amtext1],Win0,NULL,
      GTTX_Text,amtext1,
      TAG_DONE);
  GT_SetGadgetAttrs(Win0Gadgets[Win0_amtext2],Win0,NULL,
      GTTX_Text,amtext2,
      TAG_DONE);

  if(source >= inputs)
    source=0;
  GT_SetGadgetAttrs(Win0Gadgets[Win0_srclist],Win0,NULL,
      GA_Disabled,!inputs,
      GTLV_Labels,&InputList,
      GTLV_Selected,source,
      TAG_DONE);

  if(dest >= outputs)
    dest=0;
  GT_SetGadgetAttrs(Win0Gadgets[Win0_dstlist],Win0,NULL,
      GA_Disabled,!outputs,
      GTLV_Labels,&OutputList,
      GTLV_Selected,dest,
      TAG_DONE);

  if(vol<(minmon*100/0x10000))
    vol=minmon*100/0x10000;
  if(vol>(maxmon*100/0x10000))
    vol=maxmon*100/0x10000;
  GT_SetGadgetAttrs(Win0Gadgets[Win0_volslider],Win0,NULL,
      GA_Disabled, minmon == maxmon,
      GTSL_Min,minmon*100/0x10000,
      GTSL_Max,maxmon*100/0x10000,
      GTSL_Level,vol,
      TAG_DONE);

  if(gain<(mingain*100/0x10000))
    gain=mingain*100/0x10000;
  if(gain>(maxgain*100/0x10000))
    gain=maxgain*100/0x10000;
  GT_SetGadgetAttrs(Win0Gadgets[Win0_gainslider],Win0,NULL,
      GA_Disabled, mingain == maxgain,
      GTSL_Min,mingain*100/0x10000,
      GTSL_Max,maxgain*100/0x10000,
      GTSL_Level,gain,
      TAG_DONE);
}

long IDCMPhandler(void)
{
  int done=0;
  ULONG clas;
  UWORD code;
  struct Gadget *pgsel;
  struct IntuiMessage *imsg;

  while(done==0)
  {
    Wait(1L << Win0->UserPort->mp_SigBit);
    imsg=GT_GetIMsg(Win0->UserPort);
    while (imsg != NULL )
    {
      clas=imsg->Class;
      code=imsg->Code;
      pgsel=(struct Gadget *)imsg->IAddress; /* Only reference if it is a gadget message */
      GT_ReplyIMsg(imsg);
      if(ProcessWindowWin0(clas, code, pgsel))
        done=1;
      imsg=GT_GetIMsg(Win0->UserPort);
    }
  }
  return NULL;
}

void calcsamplesize(void)
{
  samplesize = (type ? sizeof(AIFCheader) : sizeof(AIFFheader)) +\
      (((struct StringInfo *) Win0Gadgets[Win0_duration]->SpecialInfo)->LongInt )*\
      recfreq*(stereo ? 4 : 2);
  GT_SetGadgetAttrs(Win0Gadgets[Win0_length],Win0,NULL,
      GTNM_Number,samplesize/1024,
      TAG_DONE);
}

BOOL ProcessWindowWin0( LONG Class, UWORD Code, APTR IAddress )
{
  struct Gadget *gad;

  switch ( Class )
  {
  case IDCMP_GADGETUP :
  case IDCMP_GADGETDOWN :
    /* Gadget message, gadget = gad. */
    gad = (struct Gadget *)IAddress;
    switch ( gad->GadgetID ) 
      {
      case Win0_srclist :
        /* ListView pressed, Text of gadget : Source */
        source=Code;
        break;
      case Win0_dstlist :
        /* ListView pressed, Text of gadget : Loopback dest. */
        dest=Code;
        break;
      case Win0_volslider :
        /* Slider changed  , Text of gadget : Loopback volume */
        vol=Code;
        break;
      case Win0_ambutton :
        /* Button pressed  , Text of gadget : Select audio mode... */
        if(AHI_AudioRequest(amreq,
            AHIR_Window,Win0,
            AHIR_SleepWindow,TRUE,
            AHIR_InitialAudioID,audioid,
            AHIR_InitialMixFreq,recfreq,
            AHIR_DoMixFreq,TRUE,
            AHIR_FilterTags,&filtertags,
            TAG_DONE))
        {
          audioid=amreq->ahiam_AudioID;
          recfreq=amreq->ahiam_MixFreq;
          AHI_GetAudioAttrs(audioid,NULL,
              AHIDB_Stereo,&stereo,
              TAG_DONE);
          updatetopgadgets();
          calcsamplesize();
        }
        break;
      case Win0_gainslider :
        /* Slider changed  , Text of gadget : Input gain */
        gain=Code;
        break;
      case Win0_duration :
        /* Integer entered , Text of gadget : Sample duration */
        calcsamplesize();
        break;
      case Win0_filename :
        /* String entered  , Text of gadget :  */
        break;
      case Win0_fnbutton :
        /* Button pressed  , Text of gadget : File name... */

        // Copy the path to directory and the file name to filename
        stccpy(directory,((struct StringInfo *) Win0Gadgets[Win0_filename]->SpecialInfo)->Buffer,1024);
        ((char *) PathPart(directory))[0]='\0'; // Strip file name
        stccpy(filename,FilePart(((struct StringInfo *) Win0Gadgets[Win0_filename]->SpecialInfo)->Buffer),1024);

        if(AslRequestTags(filereq,
            ASLFR_InitialDrawer,directory,
            ASLFR_InitialFile,filename,
            TAG_DONE))
        {
          stccpy(fullname,filereq->fr_Drawer,1024);
          AddPart(fullname,filereq->fr_File,1024);
          GT_SetGadgetAttrs(Win0Gadgets[Win0_filename],Win0,NULL,
              GTST_String,fullname,
              TAG_DONE);
        }
        break;
      case Win0_create :
        /* Button pressed  , Text of gadget : Prepare sample file */
        calcsamplesize();
        if(file=Open(((struct StringInfo *) Win0Gadgets[Win0_filename]->SpecialInfo)->Buffer,
            MODE_NEWFILE))
        {
          if(SetFileSize(file, samplesize, OFFSET_BEGINNING) == -1)
            DisplayBeep(Win0->WScreen);
          Close(file);
          file=NULL;
        }
        break;
      case Win0_begin :
        /* Button pressed  , Text of gadget : Begin recording */
        record();
        break;
      case Win0_format :
        /* Cycle changed   , Text of gadget : File format */
        type=Code;
        calcsamplesize();
        break;
      }
    break;
  case IDCMP_CLOSEWINDOW :
    return TRUE; // Return and signal quit
  case IDCMP_REFRESHWINDOW :
    GT_BeginRefresh( Win0);
    /* Refresh window. */
    RendWindowWin0( Win0, Win0VisualInfo );
    GT_EndRefresh( Win0, TRUE);
    GT_RefreshWindow( Win0, NULL);
    RefreshGList( Win0GList, Win0, NULL, ~0);
    break;
  }
  return FALSE;
}

/*
** This is the function that does the actual recording.
*/

void record(void)
{
  WORD  error=-1,taskpri;
  ULONG signalset,buffersize;
  LONG  samples=(((struct StringInfo *) Win0Gadgets[Win0_duration]->SpecialInfo)->LongInt )*recfreq;

  taskpri=SetTaskPri(FindTask(NULL),1);
  if(taskpri>1)
    SetTaskPri(FindTask(NULL),taskpri);

  RecordData.offs=0;

// RecordData.task is the task that the RecordFunc will signal

  RecordData.task=FindTask(NULL);

// Open the (hopefully prepared)

  if(file=Open(((struct StringInfo *) Win0Gadgets[Win0_filename]->SpecialInfo)->Buffer,
      MODE_OLDFILE))
  {

// Test if file size is correct (i.e., is the file prepared?).

    Seek(file,0,OFFSET_END);
    if(Seek(file,0,OFFSET_BEGINNING) == samplesize)
    {

// Allocate the hardware

      if(actrl=AHI_AllocAudio(
          AHIA_AudioID,audioid,
          AHIA_MixFreq,recfreq,
          AHIA_Channels,1,
          AHIA_Sounds,1,
          AHIA_RecordFunc,&recordhook,
          TAG_DONE))
      {

// Get the actual mixing/recording frequency

        AHI_ControlAudio(actrl,
            AHIC_MixFreq_Query,&recfreq,
            TAG_DONE);

// Allocate a signal for communication between this process and the RecordFunc.

        if(RecordData.signal=AllocSignal(-1) != -1)
        {
          RecordData.signalflag=(1L << RecordData.signal);

// Chose the correct RecordFunc

          if(stereo)
            recordhook.h_Entry=&RecordFuncS;
          else
            recordhook.h_Entry=&RecordFuncM;

// Make sure our own buffers are larger (or equal) than AHI's.

          AHI_GetAudioAttrs(AHI_INVALID_ID,actrl,
              AHIDB_MaxRecordSamples,&RecordData.bufferlen);
          RecordData.bufferlen=max(RecordData.bufferlen,8192);

// Init RecordData.count

          RecordData.count=RecordData.bufferlen;

// buffersize is the size in bytes instead of sample frames.

          buffersize=RecordData.bufferlen*2*(stereo ? 2 : 1);

// Allocate own buffers (2 of them).

          if(RecordData.buffer1=AllocVec(buffersize,MEMF_PUBLIC))
          {
            if(RecordData.buffer2=AllocVec(buffersize,MEMF_PUBLIC))
            {

// Write IFF headers.

// Why support both AIFF and AIFC? AIFF is obsolete, but still many Amiga programs
// cannot handle AIFC files. That's why.

              if(type == 0)
              {
                // Fill rest of the AIFFheader structure
                AIFFheader.FORMsize=samplesize-8;
                AIFFheader.COMMchunk.numChannels=(stereo ? 2 : 1);
                AIFFheader.COMMchunk.numSampleFrames=samples;
                ulong2extended(recfreq,&AIFFheader.COMMchunk.sampleRate);
                AIFFheader.SSNDsize=sizeof(SampledSoundHeader)+2*samples*AIFFheader.COMMchunk.numChannels;
                Write(file,&AIFFheader,sizeof(AIFFheader));
              }
              else if (type == 1)
              {
                // Fill rest of the AIFCheader structure
                AIFCheader.FORMsize=samplesize-8;
                AIFCheader.COMMchunk.numChannels=(stereo ? 2 : 1);
                AIFCheader.COMMchunk.numSampleFrames=samples;
                ulong2extended(recfreq,&AIFCheader.COMMchunk.sampleRate);
                AIFCheader.SSNDsize=sizeof(SampledSoundHeader)+2*samples*AIFCheader.COMMchunk.numChannels;
                Write(file,&AIFCheader,sizeof(AIFCheader));
              }

// Why do I have two calls here? It's just to make sure no samples are
// collected until the hardware is set up correctly.

              AHI_ControlAudio(actrl,
                  AHIC_MonitorVolume,vol*65536/100,
                  AHIC_InputGain,gain*65536/100,
                  AHIC_Input,source,
                  AHIC_Output,dest,
                  TAG_DONE);

              if(!AHI_ControlAudio(actrl,
                  AHIC_Record,TRUE,
                  TAG_DONE))
              {

                do
                {

// Update the 'seconds left' gadget
                  GT_SetGadgetAttrs(Win0Gadgets[Win0_secleft],Win0,NULL,
                      GTNM_Number,samples/recfreq,
                      TAG_DONE);

// Wait until our RecordFunc has filled first buffer
// RecordFunc always fills buffer1, which means that we should be saving buffer2.

                  signalset=Wait( RecordData.signalflag | SIGBREAKF_CTRL_C);

// Check if we've reached the end.

                  if( (samples -= RecordData.bufferlen)>0 )
                  {
// Write buffer 
                    Write(file, RecordData.buffer2, buffersize);
                  }
                  else
                  {

// Write the last part, update the 'seconds left' gadget and break from the do-while block

                    Write(file, RecordData.buffer2, 2*(samples+RecordData.bufferlen)*(stereo ? 2:1) );
                    GT_SetGadgetAttrs(Win0Gadgets[Win0_secleft],Win0,NULL,
                        GTNM_Number,0,
                        TAG_DONE);
                    break;
                  }
                } while(!(signalset & SIGBREAKF_CTRL_C));
                error=0;
              }

// Turn of RecordFunc

              AHI_ControlAudio(actrl,
                  AHIC_Record,FALSE,
                  TAG_DONE);

// Clean up...

              FreeVec(RecordData.buffer2);
            }
            FreeVec(RecordData.buffer1);
          }
          FreeSignal(RecordData.signal);
        }
        AHI_FreeAudio(actrl);
      }
      else
        error=2;
    }
    else
      error=1;
    Close(file);
  }
  else
    error=1;

// If an error occured, display a message.

  switch(error)
  {
    case 0:
      req.es_TextFormat=GetString(Finished);
      req.es_GadgetFormat=GetString(OKtext);
      EasyRequestArgs(Win0, &req, NULL, NULL);
      break;
    case 1:
      req.es_TextFormat=GetString(FileNotPrepared);
      req.es_GadgetFormat=GetString(OKtext);
      EasyRequestArgs(Win0, &req, NULL, NULL);
      break;
    case 2:
      req.es_TextFormat=GetString(NoHardware);
      req.es_GadgetFormat=GetString(OKtext);
      EasyRequestArgs(Win0, &req, NULL, NULL);
      break;
    default:
      req.es_TextFormat=GetString(UnknownErr);
      req.es_GadgetFormat=GetString(OKtext);
      EasyRequestArgs(Win0, &req, NULL, NULL);
      break;
  }
// Restore the process priority.
  SetTaskPri(FindTask(NULL),taskpri);
}

// This function translates an ULONG to Apples SANE Extended used in AIFF/AIFC files.

void ulong2extended (ULONG in, extended *ex)
{
  ex->exponent=31+16383;
  ex->mantissa[1]=0;
  while(!(in & 0x80000000))
  {
    ex->exponent--;
    in<<=1;
  }
  ex->mantissa[0]=in;
}
