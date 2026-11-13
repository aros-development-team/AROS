
#define WDH 130

#define VERSION  1
#define REVISION 23
#define DATE     11.02.2026
#define BUILD    45

#define NUM2STR(a) NUM2STR_SUB(a)
#define NUM2STR_SUB(a) #a

#define VERSION_STRING {"\0$VER: Advanced AHI Recorder "NUM2STR(VERSION)"."NUM2STR(REVISION)" Build #"NUM2STR(BUILD)" ("NUM2STR(DATE)")\r\n"}
#define PRECISION1 PRECISION_EXACT
#define PRECISION2 PRECISION_GUI
#define BIGBUFFERSIZE 32768

#define USE_FLAC

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>          /** for abs() **/

#include <exec/lists.h>
#include <exec/tasks.h>
#include <exec/memory.h>

#include <libraries/asl.h>
#include <libraries/asyncio.h>
#include <libraries/gadtools.h>

#include <intuition/screens.h>
#include <intuition/intuition.h>
#include <intuition/gadgetclass.h>

#include <graphics/gfxbase.h>

#ifdef USE_FLAC
#include "stream_decoder.h"
#include "stream_encoder.h"
#endif

#ifndef __amigaos4__
#if !defined(__AROS__)
#include <cybergraphics/cybergraphics.h>
#else
#include <cybergraphx/cybergraphics.h>
#endif
#include <proto/cybergraphics.h>
#endif

#include <dos/dosextens.h>
#include <dos/dostags.h>

#include <devices/ahi.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/keymap.h>
#include <proto/graphics.h>
#include <proto/gadtools.h>
#include <proto/icon.h>
#include <proto/asl.h>
#include <proto/ahi.h>
#include <proto/asyncio.h>

#include <utility/tagitem.h>

#include <clib/alib_protos.h>

#ifdef __amigaos4__
#define ASYNCIOVERSION 50
#else
#define ASYNCIOVERSION 38
#endif

#define WinFirstID         0
#define InputList          0
#define MonitorList        1
#define InputGain          2
#define MonitorVolume      3
#define ModeButton         4
#define FileFormat         5
#define PrepareFileButton  6
#define RecordButton       7
#define StopButton         8
#define CloseButton        9

long __stack = 131072;

const char VerString[] = VERSION_STRING;

ULONG IsCyber = 0;
ULONG TrackNumber = 0;
char FileNameWithTrackNumber[1024];
char SaveAsPath[1024];
char SaveAsName[1024];
char CmdDefaultNameStr[1024];
LONG CmdDefaultMode;
LONG CmdDefaultFreq;
LONG CmdDefaultSavePri;
LONG CmdDefaultFormat;
BOOL CmdCreateIcons;
LONG CmdDefaultInput;
LONG CmdDefaultGain;
char *CmdDefaultName;
BOOL CmdTotalTime;
BOOL CmdAutoLevels;
BOOL CmdAutobias;
BOOL CmdAutoNum;
LONG CmdCompLevel;


char *SaveFileName;

/*****************************************************************************/
/*****************************************************************************/
//#ifdef __SASC
//typedef char* CONST_STRPTR;
//#endif

IPTR BevelTags[] = {
    (GTBB_Recessed), TRUE,
    (GT_VisualInfo), 0,
    (TAG_DONE)
};

UBYTE WinFirstScaleTexts = 0;

struct IntuiText WinTexts[] = {
    1, 3, JAM1,  96, 105, NULL, (char *)"Level Meter", &WinTexts[1], //  0
    1, 3, JAM1, 320, 110, NULL, (char *)"Status:",     NULL,          //  1

    0, 0,    0,   0,  0, NULL, (char *)"Select type, mode ", NULL,   //  2
    0, 0,    0,   0,  0, NULL, (char *)"or prepare file",    NULL,   //  3
    0, 0,    0,   0,  0, NULL, (char *)"Select source",      NULL,   //  4
    0, 0,    0,   0,  0, NULL, (char *)"and set levels",     NULL,   //  5
    0, 0,    0,   0,  0, NULL, (char *)"Select mode or",     NULL,   //  6
    0, 0,    0,   0,  0, NULL, (char *)"prepare file",       NULL,   //  7
    0, 0,    0,   0,  0, NULL, (char *)"Select filetype",    NULL,   //  8
    0, 0,    0,   0,  0, NULL, (char *)"and audiomode",      NULL,   //  9

    0, 0,    0,   0,  0, NULL, "Advanced AHI Recorder "NUM2STR(VERSION)"."NUM2STR(REVISION)" by Thomas Wenzel", NULL, //  10
};

#define NUMGADGETS 10

char *PubScreenName = NULL;
struct Screen *Scr  = NULL;
struct Window *Win  = NULL;
APTR WinVisualInfo;
APTR WinDrawInfo;
struct Gadget *WinGList;
struct Gadget *WinGadgets[NUMGADGETS];
UBYTE WinFirstRun = 0;

#define AIFF16 0
#define AIFF24 1
#define WAVE16 2
#define WAVE24 3
#ifdef USE_FLAC
#define FLAC16 4
#endif

STRPTR FileFormatLabels[] = {
    (STRPTR)"AIFF 16-Bit",
    (STRPTR)"AIFF 24-Bit",
    (STRPTR)"WAVE 16-Bit",
    (STRPTR)"WAVE 24-Bit",
#ifdef USE_FLAC
    (STRPTR)"FLAC 16-Bit",
#endif
    NULL
};

enum {
    Tag_InputList_ShowSelected,
    Val_InputList_ShowSelected,
    Tag_InputList_Selected,
    Val_InputList_Selected,
    Tag_InputList_MakeVisible,
    Val_InputList_MakeVisible,
    Tag_InputList_End,

    Tag_MonitorList_ShowSelected,
    Val_MonitorList_ShowSelected,
    Tag_MonitorList_Selected,
    Val_MonitorList_Selected,
    Tag_MonitorList_MakeVisible,
    Val_MonitorList_MakeVisible,
    Tag_MonitorList_End,

    Tag_InputGain_Immediate,
    Val_InputGain_Immediate,
    Tag_InputGain_RelVerify,
    Val_InputGain_RelVerify,
    Tag_InputGain_End,

    Tag_MonitorVolume_Immediate,
    Val_MonitorVolume_Immediate,
    Tag_MonitorVolume_RelVerify,
    Val_MonitorVolume_RelVerify,
    Tag_MonitorVolume_End,

    Tag_FileFormat_Labels,
    Val_FileFormat_Labels,
    Tag_FileFormat_Active,
    Val_FileFormat_Active,
    Tag_FileFormat_End,
};

IPTR WinGadgetTags[] = {
    (GTLV_ShowSelected), 0,
    (GTLV_Selected),     0,
    (GTLV_MakeVisible),  0,
    (TAG_END),

    (GTLV_ShowSelected), 0,
    (GTLV_Selected),     0,
    (GTLV_MakeVisible),  0,
    (TAG_END),

    (GA_Immediate),      TRUE,
    (GA_RelVerify),      TRUE,
    (TAG_END),

    (GA_Immediate),      TRUE,
    (GA_RelVerify),      TRUE,
    (TAG_END),

    (GTCY_Labels), (IPTR) &FileFormatLabels[0],       // 20, 21
    (GTCY_Active), 0,                                  // 22, 23
    (TAG_END),
};

IPTR ButtonGadgetTags[] = {
    (GT_Underscore),	'_',
    (TAG_END)
};

UWORD WinGadgetTypes[] = {
    LISTVIEW_KIND,
    LISTVIEW_KIND,
    SLIDER_KIND,
    SLIDER_KIND,
    BUTTON_KIND,
    CYCLE_KIND,
    BUTTON_KIND,
    BUTTON_KIND,
    BUTTON_KIND,
    BUTTON_KIND,
};

struct NewGadget WinNewGadgets[] = {
    14,  20, 115,  46, (char *)"Input",          NULL, InputList,          4, NULL, (APTR) &WinGadgetTags[Tag_InputList_ShowSelected],
    138,  20, 115,  46, (char *)"Monitor",        NULL, MonitorList,        4, NULL, (APTR) &WinGadgetTags[Tag_MonitorList_ShowSelected],
    14,  71, 115,  12, (char *)"Input Gain",     NULL, InputGain,          8, NULL, (APTR) &WinGadgetTags[Tag_InputGain_Immediate],
    138,  71, 115,  12, (char *)"Monitor Volume", NULL, MonitorVolume,      8, NULL, (APTR) &WinGadgetTags[Tag_MonitorVolume_Immediate],
    267,  22, WDH,  14, (char *)"_Audio Mode",    NULL, ModeButton,        16, NULL,  ButtonGadgetTags,
    267,   5, WDH,  14, NULL,                    NULL, FileFormat,         1, NULL, (APTR) &WinGadgetTags[Tag_FileFormat_Labels],
    267,  39, WDH,  14, (char *)"_Prepare file",  NULL, PrepareFileButton, 16, NULL,  ButtonGadgetTags,
    267,  56, WDH,  14, (char *)"_Rec/Pause",     NULL, RecordButton,      16, NULL,  ButtonGadgetTags,
    267,  73, WDH,  14, (char *)"_Stop/Cancel",   NULL, StopButton,        16, NULL,  ButtonGadgetTags,
    267,  90, WDH,  14, (char *)"_Close AHI",     NULL, CloseButton,       16, NULL,  ButtonGadgetTags,
};

ULONG scalex, scaley;

void AllocAHI(void);
void FreeAHI(void);
void IncreaseGain(void);
void DecreaseGain(void);


void RendWindowWin(struct Window *Win, void *vi)
{
    int loop;
    UWORD offx, offy;
    offx = Win->BorderLeft;
    offy = Win->BorderTop;
    if(Win != NULL) {
        BevelTags[3] = (IPTR)vi;
        DrawBevelBoxA(Win->RPort, 6 * scalex / 65535 + offx, 5 * scaley / 65535 + offy, 256 * scalex / 65535,
                      95 * scaley / 65535, (struct TagItem *)(&BevelTags[0]));
        // LevelFrame
        DrawBevelBoxA(Win->RPort, 6 * scalex / 65535 + offx, 102 * scaley / 65535 + offy, 256 * scalex / 65535,
                      42 * scaley / 65535, (struct TagItem *)(&BevelTags[0]));
        // Level1
        DrawBevelBoxA(Win->RPort, 11 * scalex / 65535 + offx, 114 * scaley / 65535 + offy, 246 * scalex / 65535,
                      13 * scaley / 65535, (struct TagItem *)(&BevelTags[2]));
        // Level2
        DrawBevelBoxA(Win->RPort, 11 * scalex / 65535 + offx, 128 * scaley / 65535 + offy, 246 * scalex / 65535,
                      13 * scaley / 65535, (struct TagItem *)(&BevelTags[2]));
        // Status
        DrawBevelBoxA(Win->RPort, 267 * scalex / 65535 + offx, 119 * scaley / 65535 + offy, WDH * scalex / 65535,
                      25 * scaley / 65535, (struct TagItem *)(&BevelTags[0]));
        for(loop = 0; loop < 2; loop++) {
            if(WinTexts[loop].ITextFont == NULL) WinTexts[loop].ITextFont = Win->WScreen->Font;
            if(WinFirstScaleTexts == 0) {
                WinTexts[loop].LeftEdge = WinTexts[loop].LeftEdge * scalex / 65535;
                WinTexts[loop].TopEdge  = WinTexts[loop].TopEdge * scaley / 65535;
            }
        }
        WinFirstScaleTexts = 1;
        PrintIText(Win->RPort, WinTexts, offx, offy);
    }
}

int OpenWindowWin(void)
{
    ULONG i;
    UWORD offx, offy;
    UWORD loop;
    ULONG value;
    APTR handle;
    struct NewGadget newgad;
    struct Gadget *Gad;
    float scalem;
    static ULONG bpp;
#ifndef __amigaos4__
    struct	TagItem	BitmapTags[] = {
        LBMI_BYTESPERPIX,	(IPTR) &bpp,
        TAG_DONE
    };
#endif

    if(WinFirstRun == 0) {
        WinFirstRun = 1;
    }
    if(Win == NULL) {
        Scr = LockPubScreen(PubScreenName);
        if(Scr == NULL) Scr = LockPubScreen(NULL);
        if(NULL != Scr) {
#ifndef __amigaos4__
            if(CyberGfxBase) value = GetCyberMapAttr(Scr->RastPort.BitMap, CYBRMATTR_ISCYBERGFX);
            else             value = 0;

            if(value) {
                handle = LockBitMapTagList(Scr->RastPort.BitMap, BitmapTags);
                if(handle) {
                    UnLockBitMap(handle);
                    if(bpp > 1) IsCyber = 1;
                }
            }
#endif

            offx = Scr->WBorLeft;
            offy = Scr->WBorTop + Scr->Font->ta_YSize + 1;

            scalem = 0;
            for(i = 0; i < 11; i++) {
                UWORD chars;
                UWORD pixels;
                float scalef;

                chars  = strlen(WinTexts[i].IText);
                pixels = TextLength(&Scr->RastPort, WinTexts[i].IText, chars);
                scalef = (float)pixels / (8.0 * (float)chars - 4.0);
                if(scalef > scalem) scalem = scalef;
            }
            for(i = 0; i < 4; i++) {
                UWORD chars;
                UWORD pixels;
                float scalef;

                chars  = strlen(FileFormatLabels[i]);
                pixels = TextLength(&Scr->RastPort, FileFormatLabels[i], chars);
                scalef = (float)pixels / (8.0 * (float)chars - 4.0);
                if(scalef > scalem) scalem = scalef;
            }
//		if(scalem < 1.0) scalem=1.0;
            scalex = (ULONG)(65535.0 * scalem);
            if((float)scalex < (float)scaley * 0.75) scalex = (ULONG)((float)scaley * 0.75);
            scaley = 65535 * Scr->RastPort.Font->tf_YSize / 8;

            if(NULL != (WinVisualInfo = GetVisualInfoA(Scr, NULL))) {
                if(NULL != (WinDrawInfo = GetScreenDrawInfo(Scr))) {
                    WinGList = NULL;
                    Gad = CreateContext(&WinGList);
                    for(loop = 0; loop < NUMGADGETS; loop++) {
                        CopyMem((char *)&WinNewGadgets[loop], (char *)&newgad, (long)sizeof(struct NewGadget));
                        newgad.ng_VisualInfo = WinVisualInfo;
                        newgad.ng_LeftEdge = newgad.ng_LeftEdge * scalex / 65535;
                        newgad.ng_TopEdge = newgad.ng_TopEdge * scaley / 65535;
                        if(WinGadgetTypes[loop] != GENERIC_KIND) {
                            newgad.ng_Width = newgad.ng_Width * scalex / 65535;
                            newgad.ng_Height = newgad.ng_Height * scaley / 65535;
                        }
                        newgad.ng_TextAttr = Scr->Font;
                        newgad.ng_LeftEdge += offx;
                        newgad.ng_TopEdge += offy;
                        WinGadgets[ loop ] = NULL;
                        WinGadgets[ newgad.ng_GadgetID - WinFirstID ] = Gad = CreateGadgetA(WinGadgetTypes[loop], Gad, &newgad,
                                newgad.ng_UserData);
                    }
                    if(Gad != NULL) {
                        if(NULL != (Win = OpenWindowTags(NULL,
                                                         (WA_PubScreen), (IPTR)Scr,
                                                         (WA_Left),         232,
                                                         (WA_Top),          70,
                                                         (WA_InnerWidth), (270 + WDH) * scalex / 65535,
                                                         (WA_InnerHeight),  148 * scaley / 65535,
                                                         (WA_Title), (IPTR)WinTexts[10].IText,
                                                         (WA_DragBar),      TRUE,
                                                         (WA_DepthGadget),  TRUE,
                                                         (WA_CloseGadget),  TRUE,
                                                         (WA_Activate),     TRUE,
                                                         (WA_Dummy + 0x30),   TRUE,
                                                         (WA_SmartRefresh), TRUE,
                                                         (WA_AutoAdjust),   TRUE,
                                                         (WA_Gadgets), (IPTR)WinGList,
                                                         (WA_IDCMP),        IDCMP_REFRESHWINDOW
                                                             | IDCMP_MOUSEBUTTONS
                                                             | IDCMP_MOUSEMOVE
                                                             | IDCMP_GADGETDOWN
                                                             | IDCMP_GADGETUP
                                                             | IDCMP_CLOSEWINDOW
                                                             | IDCMP_RAWKEY
                                                             | IDCMP_INTUITICKS,
                                                         (TAG_END)))) {
                            RendWindowWin(Win, WinVisualInfo);
                            GT_RefreshWindow(Win, NULL);
                            RefreshGList(WinGList, Win, NULL, ~0);
                            UnlockPubScreen(NULL, Scr);
                            return(0L);
                        }
                    }
                    FreeGadgets(WinGList);
                    FreeScreenDrawInfo(Scr, WinDrawInfo);
                }
                FreeVisualInfo(WinVisualInfo);
            }
            UnlockPubScreen(NULL, Scr);
        }
    } else {
        WindowToFront(Win);
        ActivateWindow(Win);
        return(0L);
    }
    return(1L);
}

void CloseWindowWin(void)
{
    if(Win != NULL) {
        FreeScreenDrawInfo(Win->WScreen, WinDrawInfo);
        WinDrawInfo = NULL;
        CloseWindow(Win);
        Win = NULL;
        FreeVisualInfo(WinVisualInfo);
        FreeGadgets(WinGList);
    }
}

/*****************************************************************************/
/*****************************************************************************/

void updatetopgadgets(void);
void updaterightgadgets(void);
void updatebottomgadgets(void);
BOOL StartSampling(void);
void StopSampling(void);
void IDCMPhandler(void);
void SetGain(ULONG SliderGain);
void SetVolume(ULONG SliderVolume);
BOOL openAHI(void);
void closeAHI(void);
void GetPens(void);
void FreePens(void);
void ComputeLevelCoordinates(void);
void PrintStatus(char *, char *);
void ConvertToIeeeExtended(double, char *);
void ConvLong(ULONG *);
void ConvWord(UWORD *);
#ifdef __SASC
void __asm __saveds LevelSlave(void);
void __asm __saveds SaveSlave(void);
#else
void LevelSlave(void);
void SaveSlave(void);
#endif

ULONG	AsyncBuffers;

BOOL	DrawLevel = FALSE;

struct DiskObject *diskobj;


#ifdef __amigaos4__
struct Library *GadToolsBase;
struct GadToolsIFace *IGadTools;
#else
struct GfxBase *GfxBase;
struct Library *CyberGfxBase;
#endif

const char ErrorText[] = "AHIRecord Error";
const char AbortText[] = "Abort";
char RequesterText[128];

#ifndef __amigaos4__
struct EasyStruct AmigaOSRequester = {
    sizeof(struct EasyStruct),
    0,
    (char *)ErrorText,
    "I need AmigaOS 3.0 or greater!",
    (char *)AbortText
};
#endif

struct EasyStruct AsyncIORequester = {
    sizeof(struct EasyStruct),
    0,
    (char *)ErrorText,
    (char *)RequesterText,
    (char *)AbortText
};

struct EasyStruct AHIRequester = {
    sizeof(struct EasyStruct),
    0,
    (char *)ErrorText,
    "Cannot open ahi.device v4!",
    (char *)AbortText
};

struct EasyStruct RecErrorRequester = {
    sizeof(struct EasyStruct),
    0,
    (char *)ErrorText,
    "Error writing to file!",
    (char *)AbortText
};

struct EasyStruct RenameRequester = {
    sizeof(struct EasyStruct),
    0,
    (char *)ErrorText,
    "Could not rename file!\nKept old name instead.",
    "Ok"
};

struct EasyStruct AllocRequester = {
    sizeof(struct EasyStruct),
    0,
    (char *)ErrorText,
    "Cannot allocate AHI!",
    (char *)AbortText
};


LONG MinAHIGain = 0x00000, MaxAHIGain = 0x10000;
LONG GainDiv = 2048;
LONG MaxSliderGain = 32;

LONG MinAHIVolume = 0x00000, MaxAHIVolume = 0x10000;
LONG VolumeDiv = 2048;
LONG MaxSliderVolume = 32;


BYTE	AutoLevelSignal = -1;
BYTE	LevelSlaveSignal = -1;
BYTE	SaveSlaveSignal = -1;
ULONG	BytesWritten;
BOOL	WriteError;
ULONG	RecBits;

struct aiffheader {
    ULONG	FORMid;
    ULONG	FORMsize;		// 04
    ULONG	AIFFid;			// 04

    ULONG	COMMid;			// 04
    ULONG	COMMsize;		// 04

    UWORD	nTracks;		// 02
    ULONG	nFrames;		// 04
    UWORD	bpSmpl;			// 02
    char	freq[10];		// 10

    ULONG	ANNOid;			// 04
    ULONG	ANNOsize;		// 04
    char	ANNOtext[40];	// 40

    ULONG	SSNDid;			// 04
    ULONG	SSNDsize;		// 04
    ULONG	offset;			// --
    ULONG	blockSize;		// 90
} AIFFheader = {
    0x464F524D, // FORM
    0,
    0x41494646, // AIFF

    0x434F4D4D, // COMM
    18,
    2,
    0,
    16,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,

    0x414E4E4F, // ANNO
    40,
    "Program: AHIRecord, Platform: AmigaOS\0\0",

    0x53534E44,
    0,
    0,
    0
};

struct waveheader {
    ULONG	RIFFid;
    ULONG	RIFFsize;
    ULONG	WAVEid;

    ULONG	LISTid;
    ULONG	LISTsize;
    ULONG	INFOid;
    ULONG	ISFTid;
    ULONG	ISFTsize;
    char	ISFTtext[40];


    ULONG	fmtid;
    ULONG	fmtsize;
    UWORD	format;							/* 1=pcm 2=msadpcm 6=A-Law 7=u-Law    */
    UWORD	mode;								/* 1=mono 2=stereo                    */
    ULONG	freq;								/* Sampling Frequency                 */
    ULONG	bytes_sec;					/* = freq * bytes_per_smpl            */
    UWORD	bytes_smpl;					/* 16bit stereo = 4                   */
    UWORD	bits_smpl;					/* (8|12|16)                          */


    ULONG	dataid;
    ULONG	datasize;
} WAVEheader = {
    0x52494646,
    0,
    0x57415645,

    0x4C495354,
    0x34000000,	// 52
    0x494E464F,
    0x49534654,
    0x28000000,	// 40
    "Program: AHIRecord, Platform: AmigaOS\0\0",

    0x666D7420,	// fmtid
    0x10000000,	// fmtsize : 16
    0x0100,			// format
    0x0200,			// mode
    0,					// freq
    0,					// bytes_per_sec
    0x0400,			// bytes_per_smpl
    0x1000,			// bits_per_smpl

    0x64617461,	// dataid
    0						// datasize
};


#if 0
#ifdef __amigaos4__
struct Device    *AHIBase;
struct AHIIFace *IAHI;
#else
#ifdef __SASC
struct Library	 *AHIBase;
#else
struct Device    *AHIBase;
#endif
#endif
#endif
struct Library	 *AHIBase;
struct AHIIFace *IAHI;

struct MsgPort    *AHImp = NULL;
struct AHIRequest *AHIio = NULL;
BYTE               AHIDevice = -1;
struct AHIAudioModeRequester *amreq = NULL;
struct FileRequester *filereq = NULL;
struct AHIAudioCtrl *actrl = NULL;

//#ifdef __SASC
//struct Library *AsyncIOBase;
//#else
//struct AsyncIOBase * AsyncIOBase;
//#endif

#ifdef __amigaos3__
struct Library *AsyncIOBase;
#else
struct AsyncIOBase *AsyncIOBase;
#endif
#ifdef __amigaos4__
struct AsyncIOIFace *IAsyncIO;
#endif
struct AsyncFile *outfile = NULL;
BOOL FileOpen = FALSE;

char	fullname[1024];
char	iconname[1024];

BOOL	AhiAllocd     = FALSE;
BOOL	FileSelected  = FALSE;
BOOL	ReadyToRecord = FALSE;
BOOL	Recording     = FALSE;
BOOL	Pause         = FALSE;

#define STRINGNODE_ID 100
#define STRINGNODE_LENGTH 32
struct StringNode {
    struct  Node sn_Node;
    char    sn_String[STRINGNODE_LENGTH];
};

struct List InList, OutList;
char   amtext1[32], amtext2[32];

ULONG  audioid = AHI_DEFAULT_ID, recfreq = AHI_DEFAULT_FREQ;
ULONG  LevelDivide = 2;
ULONG  MaxRecSmp = 0;
ULONG  source = 0, dest = 0, AHIVolume = 0, AHIGain = 0, duration = 0;
ULONG  SliderGain = 0, SliderVolume = 0;
ULONG  FileType = 0;
ULONG  stereo = 0; //stereo: 0=no, 1 = yes
ULONG  samplesize = 0;

int done = 0;

struct TagItem filtertags[] = {
    AHIDB_Record, TRUE,
    AHIDB_Realtime, TRUE,
    AHIDB_Stereo, TRUE,
    TAG_DONE
};

struct RData {
    BYTE	bigsignal;
    BYTE	smallsignal;
    ULONG	bigsignalf;
    ULONG	smallsignalf;

    WORD	*cbuffer;
    ULONG	cblen;
    WORD	*buffer1;
    WORD	*buffer2;
    WORD	*bufferS;
    WORD	*dstptr;

    ULONG	bufferlen;
    ULONG	type;
    ULONG	count;
    APTR	mastertask;
    APTR	leveltask;
    APTR	savetask;

    ULONG	fastcount;
    ULONG	pending;
} RecordData;


#if defined(__amigaos4__) || (defined(__AROS__) && !defined(__mc680000__))
ULONG RecordFuncS(struct Hook *hook, struct AHIAudioCtrl *actrl, struct AHIRecordMessage *chan)
#else
#ifdef __SASC
__asm __saveds ULONG RecordFuncS(register __a0 struct Hook *hook, register __a2 struct AHIAudioCtrl *actrl, register __a1 struct AHIRecordMessage *chan)
#else
ULONG RecordFuncS(register struct Hook *hook
                  __asm("a0"), register struct AHIAudioCtrl *actrl __asm("a2"), register struct AHIRecordMessage *chan __asm("a1"))
#endif
#endif
{
    struct RData *RecordData;
    ULONG Type;
    ULONG Frames;
    ULONG Count;
    ULONG i;
    LONG *src;
    LONG *dst;

    RecordData = (struct RData *)hook->h_Data;

    Type = chan->ahirm_Type;
    if((Type != AHIST_S16S) && (Type != AHIST_S32S)) return 0;

    Frames = chan->ahirm_Length;
    Count = RecordData->count;
    src = (LONG *)chan->ahirm_Buffer;
    dst = (LONG *)RecordData->dstptr;

    RecordData->type = Type;
    // Increase frame counter which is used by time counter
    if(CmdTotalTime) {
        RecordData->fastcount += Frames;
    } else {
        if(!Pause) RecordData->fastcount += Frames;
    }
    // Tell level Task about current buffer and its length
    RecordData->cblen = Frames;
    RecordData->cbuffer = (WORD *)src;
    // Tell level task buffer is ready
    Signal(RecordData->leveltask, RecordData->smallsignalf);

    if(Frames > Count) {
        for(i = 0; i < Count; i++) {
            *dst++ = *src++;
            if(Type == AHIST_S32S) *dst++ = *src++;
        }
        Frames -= Count;

        dst = (LONG *)RecordData->buffer2;
        RecordData->buffer2 = RecordData->buffer1;
        RecordData->buffer1 = (WORD *)dst;
        Count = RecordData->bufferlen;
        RecordData->pending++;
        Signal(RecordData->savetask, RecordData->bigsignalf);
    }
    for(i = 0; i < Frames; i++) {
        *dst++ = *src++;
        if(Type == AHIST_S32S) *dst++ = *src++;
    }
    Count -= Frames;
    RecordData->dstptr = (WORD *)dst;
    RecordData->count = Count;
    return 0;
}

struct Hook recordhook = {
    0, 0,
    NULL,
    NULL,
    &RecordData,
};

UWORD ox, oy; /* Offsets  */
ULONG sx, sy; /* Scalings */

/* The screen's font */
struct TextFont *ScreenFont;

/* Assigned pen numbers for level meter */
BYTE	GreenPen;
BYTE	YellowPen;
BYTE	RedPen;

/* Pen numbers we got from OS */
BYTE	RGreenPen;
BYTE	RYellowPen;
BYTE	RRedPen;

/* Desired pen numbers by user */
BYTE	DGreenPen;
BYTE	DYellowPen;
BYTE	DRedPen;

/* X-Coordinates for level meter */
UBYTE	LevelP[16];
ULONG LevelC[16];
UWORD LevelX1[16];
UWORD	LevelX2[16];
UWORD	LevelXS[16];
UWORD	LevelY1l;
UWORD	LevelY2l;
UWORD	LevelYSl;
UWORD	LevelY1r;
UWORD	LevelY2r;
UWORD	LevelYSr;
UWORD	TextX;
UWORD	TextY1;
UWORD	TextY2;

ULONG	LevelFrames;

ULONG RecordingTime;

BYTE LevelAdjustDir;
ULONG LevelAdjustTimer;

char Counter[20] = "Time: 0:00:00\0";

/* implementation of strcmp that ignores cases */
int ic_strcmp(char *s1, char *s2)
{
    int i;
    for(i = 0; s1[i] && s2[i]; ++i) {
        /* If characters are same or inverting the
           6th bit makes them same */
        if(s1[i] == s2[i] || (s1[i] ^ 32) == s2[i])
            continue;
        else
            break;
    }

    /* Compare the last(or first mismatching in
       case of not same) characters */
    if(s1[i] == s2[i])
        return 0;

    // Set the 6th bit in both, then compare
    if((s1[i] | 32) < (s2[i] | 32))
        return -1;
    return 1;
}

#ifdef USE_FLAC
FLAC__StreamEncoderWriteStatus FlacWriteCallback(const FLAC__StreamEncoder *encoder, const FLAC__byte buffer[],
        size_t bytes, unsigned samples, unsigned current_frame, void *client_data)
{
    LONG BytesToWrite;
    LONG BytesWritten;
    struct AsyncFile *OutFile;

    OutFile = (struct AsyncFile *)client_data;
    if(!OutFile) return FLAC__STREAM_ENCODER_WRITE_STATUS_FATAL_ERROR;
    BytesToWrite = (LONG)bytes;
    BytesWritten = WriteAsync(OutFile, (APTR)buffer, BytesToWrite);
    if(BytesWritten != BytesToWrite) return FLAC__STREAM_ENCODER_WRITE_STATUS_FATAL_ERROR;
    return FLAC__STREAM_ENCODER_WRITE_STATUS_OK;
}

FLAC__StreamEncoderSeekStatus FlacSeekCallback(const FLAC__StreamEncoder *encoder, FLAC__uint64 absolute_byte_offset,
        void *client_data)
{
    LONG pos;
    struct AsyncFile *OutFile;

    OutFile = (struct AsyncFile *)client_data;
    if(!OutFile) return FLAC__STREAM_ENCODER_SEEK_STATUS_ERROR;
    pos = SeekAsync(OutFile, absolute_byte_offset, MODE_START);
    if(pos < 0) return FLAC__STREAM_ENCODER_SEEK_STATUS_ERROR;
    return FLAC__STREAM_ENCODER_SEEK_STATUS_OK;
}

FLAC__StreamEncoderTellStatus FlacTellCallback(const FLAC__StreamEncoder *encoder, FLAC__uint64 *absolute_byte_offset,
        void *client_data)
{
    LONG pos;
    struct AsyncFile *OutFile;

    OutFile = (struct AsyncFile *)client_data;
    if(!OutFile) return FLAC__STREAM_ENCODER_TELL_STATUS_ERROR;
    pos = SeekAsync(OutFile, 0, MODE_CURRENT);
    if(pos < 0) return FLAC__STREAM_ENCODER_TELL_STATUS_ERROR;
    *absolute_byte_offset = (FLAC__uint64)pos;
    return FLAC__STREAM_ENCODER_TELL_STATUS_OK;
}


FLAC__StreamEncoderWriteStatus FlacWriteBufferedCallback(const FLAC__StreamEncoder *encoder, const FLAC__byte buffer[],
        size_t bytes, unsigned samples, unsigned current_frame, void *client_data)
{
    FILE *f = (FILE *)client_data;
    if(fwrite(buffer, 1, bytes, f) != bytes) {
        return FLAC__STREAM_ENCODER_WRITE_STATUS_FATAL_ERROR;
    } else {
        return FLAC__STREAM_ENCODER_WRITE_STATUS_OK;
    }

}

FLAC__StreamEncoderSeekStatus FlacSeekBufferedCallback(const FLAC__StreamEncoder *encoder,
        FLAC__uint64 absolute_byte_offset, void *client_data)
{
    FILE *f = (FILE *)client_data;
#ifdef __amigaos4__
    if(fseeko(f, (off_t)absolute_byte_offset, SEEK_SET) < 0)
#else
    if(fseek(f, (long)absolute_byte_offset, SEEK_SET) < 0)
#endif
    {
        return FLAC__STREAM_ENCODER_SEEK_STATUS_ERROR;
    } else {
        return FLAC__STREAM_ENCODER_SEEK_STATUS_OK;
    }
}

FLAC__StreamEncoderTellStatus FlacTellBufferedCallback(const FLAC__StreamEncoder *encoder,
        FLAC__uint64 *absolute_byte_offset, void *client_data)
{
    FILE *f = (FILE *)client_data;
#ifdef __amigaos4__
    off_t pos;
    if((pos = ftello(f)) < 0)
#else
    long pos;
    if((pos = ftell(f)) < 0)
#endif
    {
        return FLAC__STREAM_ENCODER_TELL_STATUS_ERROR;
    } else {
        *absolute_byte_offset = (FLAC__uint64)pos;
        return FLAC__STREAM_ENCODER_TELL_STATUS_OK;
    }
}


#endif

char *CreateNumberedFileName(char *FileName)
{
    int LenMinusExtension;
    char TrackNumberStr[4];

    sprintf(TrackNumberStr, "%03ld", TrackNumber);

    strcpy(FileNameWithTrackNumber, FileName);
    strcat(FileNameWithTrackNumber, TrackNumberStr);

    LenMinusExtension = strlen(FileName) - 4;
    if(ic_strcmp(&FileName[LenMinusExtension], ".wav") == 0) {
        memset(FileNameWithTrackNumber, 0, 1024);
        strncpy(FileNameWithTrackNumber, FileName, LenMinusExtension);
        strcat(FileNameWithTrackNumber, TrackNumberStr);
        strcat(FileNameWithTrackNumber, ".wav");
    }

    LenMinusExtension = strlen(FileName) - 5;
    if(ic_strcmp(&FileName[LenMinusExtension], ".aiff") == 0) {
        memset(FileNameWithTrackNumber, 0, 1024);
        strncpy(FileNameWithTrackNumber, FileName, LenMinusExtension);
        strcat(FileNameWithTrackNumber, TrackNumberStr);
        strcat(FileNameWithTrackNumber, ".aiff");
    }
    if(ic_strcmp(&FileName[LenMinusExtension], ".flac") == 0) {
        memset(FileNameWithTrackNumber, 0, 1024);
        strncpy(FileNameWithTrackNumber, FileName, LenMinusExtension);
        strcat(FileNameWithTrackNumber, TrackNumberStr);
        strcat(FileNameWithTrackNumber, ".flac");
    }

    return FileNameWithTrackNumber;
}

void CreateFile(char *FileName)
{
    SaveFileName = FileName;
    if(TrackNumber) {
        SaveFileName = CreateNumberedFileName(FileName);
    }
    /* Create the save slave task */
    SaveSlaveSignal = AllocSignal(-1);
#ifdef DEBUG
    printf("\n\nMaster task says: I'll create the save slave task. (Signal: %d)\n", SaveSlaveSignal);
    printf("Master task closes his eyes and looks concentrated.\n");
#endif

    RecordData.mastertask = FindTask(NULL);
    Forbid();
    RecordData.savetask = CreateNewProcTags(
                              NP_Entry, (IPTR)SaveSlave,
                              NP_Name, (IPTR)"AHIRec saver",
                              NP_Priority, CmdDefaultSavePri,
                              NP_StackSize, 131072,
                              TAG_DONE);
    Permit();

    /* Wait for slave to come alive */
    if(RecordData.savetask) {
        Wait(1L << SaveSlaveSignal);
    }
#ifdef DEBUG
    printf("Master task smiles.\n\n");
#endif

    if(!FileOpen) {
        if(SaveSlaveSignal != -1) FreeSignal(SaveSlaveSignal);
        SaveSlaveSignal = -1;
    }
}

void FinishFile(void)
{
    /* Kill save slave task */
    if(RecordData.savetask) {
#ifdef DEBUG
        printf("Trying to remove Save task.\n"); /* !!! */
#endif
        Signal((struct Task *)RecordData.savetask, SIGBREAKF_CTRL_C);
        Wait(1L << SaveSlaveSignal);
        RecordData.savetask = NULL;
    }
    if(SaveSlaveSignal != -1) FreeSignal(SaveSlaveSignal);
    SaveSlaveSignal = -1;
#ifdef DEBUG
    printf("Save task successfully removed.\n"); /* !!! */
#endif
}

void PrepareFile(void)
{
    if(!actrl) AllocAHI();
    if(!actrl) return;
    if(CmdDefaultName) {
        FileSelected = TRUE;
        if(CmdAutoNum) {
            TrackNumber = 1;
        } else {
            TrackNumber = 0;
        }

        if(FileOpen) {
            FinishFile();
            FileOpen = FALSE;
        }

        WriteError = FALSE;
        CreateFile(CmdDefaultName);
        if(FileOpen) {
            if(CmdCreateIcons) {
                if(diskobj = GetDiskObject("ENVARC:sys/def_sound")) {
                    PutDiskObject(CmdDefaultName, diskobj);
                    FreeDiskObject(diskobj);
                }
            }

            BytesWritten = 0;
            ReadyToRecord = StartSampling();
            updaterightgadgets();
            updatebottomgadgets();
        } else {
            WriteError = TRUE;
        }
    } else {
        if(AslRequestTags(filereq,
                          ASLFR_SleepWindow,   TRUE,
                          ASLFR_DoSaveMode,    TRUE,
                          ASLFR_RejectIcons,   TRUE,
                          TAG_DONE)) {
            strncpy(fullname, filereq->fr_Drawer, 1024);
            AddPart(fullname, filereq->fr_File, 1024);
        } else fullname[0] = 0;

        if(fullname[0] != 0) {

            FileSelected = TRUE;
            if(FileOpen) {
                FinishFile();
                FileOpen = FALSE;
            }

            WriteError = FALSE;
            CreateFile(fullname);
            if(FileOpen) {
                if(CmdCreateIcons) {
                    if(diskobj = GetDiskObject("ENVARC:sys/def_sound")) {
                        PutDiskObject(fullname, diskobj);
                        FreeDiskObject(diskobj);
                    }
                }

                BytesWritten = 0;
                ReadyToRecord = StartSampling();
                updaterightgadgets();
                updatebottomgadgets();
            } else {
                WriteError = TRUE;
            }
        } else FileSelected = FALSE;
    }
}


void SelectMode(void)
{
    if(!actrl) {
        // Dirty trick: Alloc default audio mode to get default audio id & frequency
        AllocAHI();
    }

    if(actrl) {
        AHI_ControlAudio(actrl, AHIC_MixFreq_Query, (IPTR)&recfreq, TAG_DONE);
        AHI_GetAudioAttrs(AHI_INVALID_ID, actrl, AHIDB_AudioID, (IPTR)&audioid, TAG_DONE);
        AHI_FreeAudio(actrl);
        actrl = NULL;
        AhiAllocd = FALSE;
    } else {
        if(recfreq == AHI_DEFAULT_FREQ) recfreq = 44100;
    }

    if(AHI_AudioRequest(amreq,
                        AHIR_Window, (IPTR)Win,
                        AHIR_SleepWindow, TRUE,
                        AHIR_InitialAudioID, audioid,
                        AHIR_InitialMixFreq, recfreq,
                        AHIR_DoMixFreq, TRUE,
                        AHIR_FilterTags, (IPTR)&filtertags,
                        TAG_DONE)) {
        audioid = amreq->ahiam_AudioID;
        recfreq = amreq->ahiam_MixFreq;
        AllocAHI();
        if(actrl) {
            if(CmdDefaultName) {
                FileSelected = TRUE;
                PrepareFile();
                updatetopgadgets();
                updatebottomgadgets();
            } else {
                updatetopgadgets();
                updaterightgadgets();
                updatebottomgadgets();
            }
        }
    }
}

void RecPause(void)
{
    if(!Recording) {
        Recording = TRUE;
        RecordData.fastcount = 0;
        updaterightgadgets();
        sprintf(Counter, "Time: 0:00:00");
        PrintStatus("Recording...", Counter);
    } else {
        if(Pause) {
            Pause = FALSE;
            PrintStatus("Recording...", Counter);
        } else {
            Pause = TRUE;
            PrintStatus("Paused.", Counter);
        }
    }
}

void StopRecording(void)
{
    BOOL WasRecording;
    BOOL RenameSuccessful;
    BOOL ContinueCreatingFiles;

    WasRecording = Recording;

    GT_SetGadgetAttrs(WinGadgets[StopButton], Win, NULL,
                      GA_Disabled, TRUE,
                      TAG_DONE);

#ifdef DEBUG
    printf("Calling StopSampling.\n"); /* !!! */
#endif
    StopSampling();
#ifdef DEBUG
    printf("StopSampling returned.\n"); /* !!! */
#endif

    if(CmdDefaultName) {
        if(WasRecording) {
            ContinueCreatingFiles = FALSE;
            if(FileOpen) {
                FinishFile();
                FileOpen = FALSE;
            }
            WriteError = FALSE;

            if(CmdAutoNum) {
                // In case of auto numbering just increase the track number.
                TrackNumber++;
                ContinueCreatingFiles = TRUE;
            } else {
                // Otherwise assk user to rename file.
                char *filepart;
                TrackNumber = 0;
                strncpy(SaveAsPath, CmdDefaultName, 1024);
                filepart = FilePart(SaveAsPath);
                *filepart = 0;
                strncpy(SaveAsName, FilePart(CmdDefaultName), 1024);
                if(AslRequestTags(filereq,
                                  ASLFR_SleepWindow,   TRUE,
                                  ASLFR_DoSaveMode,    TRUE,
                                  ASLFR_RejectIcons,   TRUE,
                                  ASLFR_InitialDrawer, SaveAsPath,
                                  ASLFR_InitialFile,   SaveAsName,
                                  TAG_DONE)) {
                    // If the user selected a name then try to rename the file.
                    strncpy(fullname, filereq->fr_Drawer, 1024);
                    AddPart(fullname, filereq->fr_File, 1024);
                    RenameSuccessful = Rename(CmdDefaultName, fullname);
                    // If it worked then continue creating new files.
                    if(RenameSuccessful) {
                        ContinueCreatingFiles = TRUE;
                    }
                    // If it didn't then show error requester instead.
                    else {
                        EasyRequest(NULL, &RenameRequester, 0);
                        ContinueCreatingFiles = FALSE;
                    }
                }
            }

            if(ContinueCreatingFiles) {
                CreateFile(CmdDefaultName);
                if(FileOpen) {
                    FileSelected = TRUE;
                    BytesWritten = 0;
                    ReadyToRecord = StartSampling();
                    updaterightgadgets();
                    updatebottomgadgets();
                } else {
                    WriteError = TRUE;
                }
            } else {
                FileSelected  = FALSE;
                ReadyToRecord = FALSE;
                Recording	  = FALSE;
                updaterightgadgets();
                PrintStatus(WinTexts[2].IText, WinTexts[3].IText);
            }
        } else {
            FileSelected  = FALSE;
            ReadyToRecord = FALSE;
            Recording     = FALSE;
            updaterightgadgets();
            PrintStatus(WinTexts[2].IText, WinTexts[3].IText);
            CmdDefaultName = NULL;
        }
    } else {
        FileSelected  = FALSE;
        ReadyToRecord = FALSE;
        Recording     = FALSE;
        updaterightgadgets();
        PrintStatus(WinTexts[2].IText, WinTexts[3].IText);
    }
}

void AllocAHI(void)
{
    if(actrl) return;

    // If no mode has been selected yet then use the default
    if(audioid == AHI_INVALID_ID) {
        audioid = AHI_DEFAULT_ID;
    }

    // Allocate the hardware
    if(actrl = AHI_AllocAudio(
                   AHIA_AudioID, audioid,
                   AHIA_MixFreq, recfreq,
                   AHIA_Channels, 1,
                   AHIA_Sounds, 1,
                   AHIA_RecordFunc, (IPTR)&recordhook,
                   TAG_DONE)) {
        // From ahi.doc:
        // Always set ID to AHI_INVALID_ID and use
        // audioctrl if you have allocated a valid AHIAudioCtrl structure.
        AHI_GetAudioAttrs(AHI_INVALID_ID, actrl,
                          AHIDB_MaxRecordSamples, (IPTR)&MaxRecSmp,
                          TAG_DONE);
        LevelDivide = 2;
        if(MaxRecSmp != 0) LevelDivide = LevelDivide * (recfreq / 5512) / (MaxRecSmp / 512) * 2;
        if(LevelDivide < 2) LevelDivide = 2;

        AHI_ControlAudio(actrl, AHIC_MixFreq_Query, (IPTR)&recfreq, TAG_DONE);

        AHI_GetAudioAttrs(AHI_INVALID_ID, actrl,
                          AHIDB_Stereo, (IPTR)&stereo,
                          TAG_DONE);
        AHI_GetAudioAttrs(AHI_INVALID_ID, actrl,
                          AHIDB_Bits, (IPTR)&RecBits,
                          TAG_DONE);
        AhiAllocd = TRUE;
    } else {
        EasyRequest(NULL, &AllocRequester, 0);
    }
}

void FreeAHI(void)
{
    StopSampling();
    if(actrl) {
        AHI_FreeAudio(actrl);
        actrl = NULL;
        AhiAllocd = FALSE;
    }
    FileSelected  = FALSE;
    ReadyToRecord = FALSE;
    Recording     = FALSE;
    updatetopgadgets();
    updaterightgadgets();
    updatebottomgadgets();
    PrintStatus(WinTexts[8].IText, WinTexts[9].IText);
}

void DoRawKey(ULONG Code, ULONG Qualifier, ULONG RawLen, ULONG Vanilla)
{
//	printf("DoRawKey: Code=%d Quali=0x%08X, RawLen=%d Vanilla=%d\n", Code, Qualifier, RawLen, Vanilla);

    if(Qualifier & (IEQUALIFIER_LCOMMAND | IEQUALIFIER_RCOMMAND)) {
        if(RawLen == 1) {
            switch(Vanilla) {
            case 'q':
                done = 1;
                break;
            }
        }
    }

    if(RawLen == 1) {
        switch(Vanilla) {
        case 'a':
            SelectMode();
            break;
        case 'p':
            PrepareFile();
            break;
        case 'r':
        case ' ':
            RecPause();
            break;
        case 's':
            StopRecording();
            break;
        case 'c':
            FreeAHI();
            break;
        case '+':
            IncreaseGain();
            break;
        case '-':
            DecreaseGain();
            break;
        }
    }
}

void ProcessWindowWin(LONG Class, UWORD Code, UWORD Qualifier, APTR IAddress)
{
    struct Gadget *gad;
    UBYTE Vanilla;
    struct InputEvent ie;
    WORD RawLen;

    switch(Class) {
    case IDCMP_RAWKEY:
        ie.ie_Class        = IECLASS_RAWKEY;
        ie.ie_SubClass     = 0;
        ie.ie_Code         = Code;
        ie.ie_Qualifier    = Qualifier;
        ie.ie_EventAddress = IAddress;
        RawLen = MapRawKey(&ie, &Vanilla, 1, 0);
        DoRawKey(Code, Qualifier, RawLen, Vanilla);
        break;

    case IDCMP_GADGETUP :
    case IDCMP_MOUSEMOVE :
        /* Gadget message, gadget = gad. */
        gad = (struct Gadget *)IAddress;
        switch(gad->GadgetID) {
        case InputList :
            /* ListView pressed, Text of gadget : Input */
            source = Code;
            if(actrl) AHI_ControlAudio(actrl, AHIC_Input, source, TAG_DONE);
            break;
        case MonitorList :
            /* ListView pressed, Text of gadget : Monitor */
            dest = Code;
            if(actrl) AHI_ControlAudio(actrl, AHIC_Output, dest, TAG_DONE);
            break;
        case InputGain :
            /* Slider changed  , Text of gadget : Input Gain */
            SetGain(Code);
            break;
        case MonitorVolume :
            /* Slider changed  , Text of gadget : Monitor Volume */
            SetVolume(Code);
            break;
        case ModeButton :
            /* Button pressed  , Text of gadget : Select Audio Mode */
            SelectMode();
            break;
        case FileFormat :
            /* Cycle changed   , Text of gadget :  */
            FileType = Code;
            break;
        case PrepareFileButton :
            PrepareFile();
            break;
        case RecordButton :
            /* Button pressed  , Text of gadget : Record */
            RecPause();
            break;
        case StopButton :
            /* Button pressed  , Text of gadget : Stop */
            StopRecording();
            break;
        case CloseButton :
            /* Button pressed  , Text of gadget : Select Audio Mode */
            FreeAHI();
            break;
        }
        break;
    case IDCMP_CLOSEWINDOW :
        /* CloseWindow Now */
        break;
    case IDCMP_REFRESHWINDOW :
        GT_BeginRefresh(Win);
        /* Refresh window. */
        RendWindowWin(Win, WinVisualInfo);
        GT_EndRefresh(Win, TRUE);
        GT_RefreshWindow(Win, NULL);
        RefreshGList(WinGList, Win, NULL, ~0);
        break;
    }
}


int main(int argc, char **argv)
{
    STRPTR *ttypes;
    STRPTR str;

    AsyncBuffers = 4;
    DGreenPen = -1;
    DYellowPen = -1;
    DRedPen = -1;
    CmdCreateIcons = FALSE;
    RecordData.bigsignal = -1;
    RecordData.smallsignal = -1;
    CmdDefaultMode = AHI_DEFAULT_ID;
#ifdef __amigaos4__
    CmdCompLevel = 8;
#else
    CmdCompLevel = 5;
#endif
    CmdDefaultGain = 0;
#if !defined(__AROS__)
    CONST_STRPTR *aaargv = (CONST_STRPTR *)argv;
#else
    UBYTE **aaargv = (UBYTE **)argv;
#endif

    if(ttypes = ArgArrayInit((LONG)argc, aaargv)) {
#if !defined(__AROS__)
        CONST_STRPTR *astt = (CONST_STRPTR *)ttypes;
#else
        UBYTE **astt = (UBYTE **)ttypes;
#endif
        str = ArgString(astt, "MODE", "0");
        CmdDefaultMode = atol(str);

        str = ArgString(astt, "FREQ", "0");
        CmdDefaultFreq = atol(str);

        str = ArgString(astt, "INPUT", "1");
        CmdDefaultInput = atol(str);

        str = ArgString(astt, "FORMAT", "AIFF_16bit");
        CmdDefaultFormat = 0;
        if(stricmp(str, "AIFF_16bit") == 0) CmdDefaultFormat = 0;
        if(stricmp(str, "AIFF_24bit") == 0) CmdDefaultFormat = 1;
        if(stricmp(str, "WAVE_16bit") == 0) CmdDefaultFormat = 2;
        if(stricmp(str, "WAVE_24bit") == 0) CmdDefaultFormat = 3;
        if(stricmp(str, "FLAC_16bit") == 0) CmdDefaultFormat = 4;

        FileType = CmdDefaultFormat;
        WinGadgetTags[Val_FileFormat_Active] = CmdDefaultFormat;

#ifdef __amigaos4__
        str = ArgString(astt, "COMPLEVEL", "8");
#else
        str = ArgString(astt, "COMPLEVEL", "5");
#endif
        CmdCompLevel = atol(str);

        if(str = ArgString(astt, "NAME", NULL)) {
            strncpy(CmdDefaultNameStr, str, 1024);
            CmdDefaultName = CmdDefaultNameStr;
        } else CmdDefaultName = NULL;

        str = ArgString(astt, "AUTONUM", "NO");
        if(stricmp(str, "yes") == 0) CmdAutoNum = TRUE;
        else                        CmdAutoNum = FALSE;

        str = ArgString(astt, "BUFFERS", "4");
        AsyncBuffers = atol(str);

        str = ArgString(astt, "GPEN", "-1");
        DGreenPen = atoi(str);

        str = ArgString(astt, "YPEN", "-1");
        DYellowPen = atoi(str);

        str = ArgString(astt, "RPEN", "-1");
        DRedPen = atoi(str);

        str = ArgString(astt, "ICONS", "NO");
        if(stricmp(str, "yes") == 0) CmdCreateIcons = TRUE;
        else                        CmdCreateIcons = FALSE;

        str = ArgString(astt, "TIMER", "0");
        RecordingTime = atol(str);

        str = ArgString(astt, "TOTALTIME", "NO");
        if(stricmp(str, "yes") == 0) CmdTotalTime = TRUE;
        else                        CmdTotalTime = FALSE;

        str = ArgString(astt, "ALC", "NO");
        if(stricmp(str, "yes") == 0) CmdAutoLevels = TRUE;
        else                        CmdAutoLevels = FALSE;

        str = ArgString(astt, "SAVEPRI", "5");
        CmdDefaultSavePri = atol(str);

        str = ArgString(astt, "GAIN", "0");
        CmdDefaultGain = atol(str);
        if(CmdDefaultGain > 100) CmdDefaultGain = 100;

        str = ArgString(astt, "AUTOBIAS", "NO");
        if(stricmp(str, "yes") == 0) CmdAutobias = TRUE;
        else                        CmdAutobias = FALSE;

        PubScreenName = ArgString(astt, "PUBSCREEN", NULL);

        ArgArrayDone();
    }

#ifdef __amigaos4__
    GadToolsBase = OpenLibrary("gadtools.library", 0);
    IGadTools = (struct GadToolsIFace *)GetInterface(GadToolsBase, "main", 1, NULL);
#else
    if(GfxBase = (struct GfxBase *)OpenLibrary("graphics.library", 39L))
#endif
    {
#ifndef __amigaos4__
        CyberGfxBase = OpenLibrary("cybergraphics.library", 37);
#endif

#if !defined(__AROS__)
        if(AsyncIOBase = (struct AsyncIOBase *)OpenLibrary("asyncio.library", ASYNCIOVERSION)) {
#else
        if(AsyncIOBase = OpenLibrary("asyncio.library", ASYNCIOVERSION)) {
#endif
#ifdef __amigaos4__
            IAsyncIO = (struct AsyncIOIFace *)GetInterface((struct Library *)AsyncIOBase, "main", 1, NULL);
#endif
            if(openAHI()) {
                if(OpenWindowWin() == 0) {
                    if(filereq = AllocAslRequestTags(ASL_FileRequest,
                                                     ASLFR_Window, (IPTR)Win,
                                                     ASLFR_SleepWindow, TRUE,
                                                     ASLFR_DoSaveMode, TRUE,
                                                     ASLFR_RejectIcons, TRUE,
                                                     TAG_DONE)) {

                        NewList(&InList);
                        NewList(&OutList);

                        GetPens();
                        ComputeLevelCoordinates();

                        if(CmdDefaultMode && CmdDefaultFreq) {
                            ULONG	inputs;
                            audioid = CmdDefaultMode;
                            recfreq = CmdDefaultFreq;

                            if(actrl = AHI_AllocAudio(
                                           AHIA_AudioID, audioid,
                                           AHIA_MixFreq, recfreq,
                                           AHIA_Channels, 1,
                                           AHIA_Sounds, 1,
                                           AHIA_RecordFunc, (IPTR)&recordhook,
                                           TAG_DONE)) {

                                AHI_GetAudioAttrs(AHI_INVALID_ID, actrl, AHIDB_MaxRecordSamples, (IPTR)&MaxRecSmp, TAG_DONE);
                                LevelDivide = 2;
                                if(MaxRecSmp != 0) LevelDivide = LevelDivide * (recfreq / 5512) / (MaxRecSmp / 512) * 2;
                                if(LevelDivide < 2) LevelDivide = 2;

                                if(AHI_GetAudioAttrs(AHI_INVALID_ID, actrl, AHIDB_Stereo, (IPTR)&stereo, AHIDB_Inputs, (IPTR)&inputs, TAG_DONE)) {
                                    if((CmdDefaultInput > 0) && (CmdDefaultInput <= inputs)) {
                                        source = CmdDefaultInput - 1;
                                    }

                                    if(CmdDefaultName) {
                                        WriteError = FALSE;
                                        CreateFile(CmdDefaultName);
                                        if(FileOpen) {
                                            FileSelected = TRUE;
                                            BytesWritten = 0;
                                            ReadyToRecord = StartSampling();
                                            if(RecordingTime) {
                                                Recording = TRUE;
                                                RecordData.fastcount = 0;
                                                updaterightgadgets();
                                                PrintStatus("Recording...", "Time: 0:00:00");
                                            }
                                        }
                                    }
                                }
                                AhiAllocd = TRUE;
                            }
                        }

                        updatetopgadgets();
                        updaterightgadgets();
                        updatebottomgadgets();

                        IDCMPhandler();
                        FreeAslRequest(filereq);
                        FreePens();
                    }
                    CloseWindowWin();
                } else {
#ifdef DEBUG
                    printf("Cannot open window.\n");
#endif
                }
                closeAHI();
            } else {
                EasyRequest(NULL, &AHIRequester, 0);
#ifdef DEBUG
                printf("Cannot open ahi.device\n");
#endif
            }
#ifdef __amigaos4__
            if(IAsyncIO) DropInterface((struct Interface *)IAsyncIO);
#endif
            CloseLibrary((struct Library *)AsyncIOBase);
        } else {
            sprintf(RequesterText, "Cannot open asyncio.library v%d!", ASYNCIOVERSION);
            EasyRequest(NULL, &AsyncIORequester, 0);
#ifdef DEBUG
            printf("Cannot open asyncio.library\n");
#endif
        }
#ifdef __amigaos4__
        DropInterface((struct Interface *)IGadTools);
        if(GadToolsBase) CloseLibrary((struct Library *)GadToolsBase);
#else
        if(CyberGfxBase) CloseLibrary(CyberGfxBase);
        CloseLibrary((struct Library *)GfxBase);
#endif
    }
#ifndef __amigaos4__
    else {
        EasyRequest(NULL, &AmigaOSRequester, 0);
#ifdef DEBUG
        printf("I need AmigaOS 3.0!\n");
#endif
    }
#endif
}

void IncreaseGain(void)
{
    if(SliderGain < MaxSliderGain) SliderGain++;
    GT_SetGadgetAttrs(WinGadgets[InputGain], Win, NULL, GTSL_Level, SliderGain, TAG_DONE);
    SetGain(SliderGain);
}

void DecreaseGain(void)
{
    if(SliderGain > 0) SliderGain--;
    GT_SetGadgetAttrs(WinGadgets[InputGain], Win, NULL, GTSL_Level, SliderGain, TAG_DONE);
    SetGain(SliderGain);
}

void IDCMPhandler(void)
{
    ULONG Signals;
    ULONG class;
    UWORD code;
    UWORD quali;
    struct Gadget *pgsel;
    struct IntuiMessage *imsg;

    while(done == 0) {
        Signals = Wait(1L << Win->UserPort->mp_SigBit | 1L << AutoLevelSignal | SIGBREAKF_CTRL_C);
        if(Signals & SIGBREAKF_CTRL_C) {
            done = 1;
            break;
        }

        if(Signals & 1L << AutoLevelSignal) {
            if(LevelAdjustDir == 1) {
                IncreaseGain();
            }
            if(LevelAdjustDir == -1) {
                DecreaseGain();
            }
            if(LevelAdjustDir == -2) {
                if(SliderGain > 0) SliderGain--;
                if(SliderGain > 0) SliderGain--;
                GT_SetGadgetAttrs(WinGadgets[InputGain], Win, NULL, GTSL_Level, SliderGain, TAG_DONE);
                SetGain(SliderGain);
            }

        }

        if(Signals & 1L << Win->UserPort->mp_SigBit) {
            imsg = GT_GetIMsg(Win->UserPort);
            while(imsg != NULL) {
                class = imsg->Class;
                code = imsg->Code;
                quali = imsg->Qualifier;
                pgsel = (struct Gadget *)imsg->IAddress; /* Only reference if it is a gadget message */
                GT_ReplyIMsg(imsg);
                ProcessWindowWin(class, code, quali, pgsel);

                /* The next line is just so you can quit, remove when proper method implemented. */
                if(class == IDCMP_CLOSEWINDOW) done = 1;
                imsg = GT_GetIMsg(Win->UserPort);
            }
        }
    }
    StopSampling();
    if(actrl) {
        AHI_FreeAudio(actrl);
        actrl = NULL;
        AhiAllocd = FALSE;
    }
}



void updatetopgadgets(void)
{
    long i;
    long inputs = 0, outputs = 0;
    struct StringNode *node;
    ULONG SelectedSource, SelectedDest;

    amtext2[0] = '\0';

    if(actrl) {
        AHI_GetAudioAttrs(AHI_INVALID_ID, actrl,
                          AHIDB_BufferLen, 32,
                          AHIDB_Name, (IPTR)amtext1,
                          AHIDB_Inputs, (IPTR)&inputs,
                          AHIDB_Outputs, (IPTR)&outputs,
                          AHIDB_MinMonitorVolume, (IPTR)&MinAHIVolume,
                          AHIDB_MaxMonitorVolume, (IPTR)&MaxAHIVolume,
                          AHIDB_MinInputGain, (IPTR)&MinAHIGain,
                          AHIDB_MaxInputGain, (IPTR)&MaxAHIGain,
                          TAG_DONE);
    }

    // Create input and output lists
    // Free old lists first
    while(node = (struct StringNode *) RemHead(&InList)) FreeVec(node);
    while(node = (struct StringNode *) RemHead(&OutList)) FreeVec(node);

    if(actrl) {
        // Add new nodes
        for(i = 0; i < inputs; i++) {
            if(node = AllocVec(sizeof(struct StringNode), MEMF_CLEAR)) {
                AHI_GetAudioAttrs(AHI_INVALID_ID, actrl,
                                  AHIDB_BufferLen, STRINGNODE_LENGTH,
                                  AHIDB_InputArg, i,
                                  AHIDB_Input, (IPTR)node->sn_String,
                                  TAG_DONE);
                node->sn_Node.ln_Name = node->sn_String;
                node->sn_Node.ln_Type = STRINGNODE_ID;
                AddTail(&InList, (struct Node *) node);
            }
        }
        for(i = 0; i < outputs; i++) {
            if(node = AllocVec(sizeof(struct StringNode), MEMF_CLEAR)) {
                AHI_GetAudioAttrs(AHI_INVALID_ID, actrl,
                                  AHIDB_BufferLen, STRINGNODE_LENGTH,
                                  AHIDB_OutputArg, i,
                                  AHIDB_Output, (IPTR)node->sn_String,
                                  TAG_DONE);
                node->sn_Node.ln_Name = node->sn_String;
                node->sn_Node.ln_Type = STRINGNODE_ID;
                AddTail(&OutList, (struct Node *) node);
            }
        }
    }

    // Set gadget attributes
    SelectedSource = source;
    if(SelectedSource >= inputs) SelectedSource = 0;
    GT_SetGadgetAttrs(WinGadgets[InputList], Win, NULL,
                      GA_Disabled, !inputs,
                      GTLV_Labels, (IPTR)&InList,
                      GTLV_Selected, SelectedSource,
                      GTLV_MakeVisible, SelectedSource,
                      TAG_DONE);

    SelectedDest = dest;
    if(SelectedDest >= outputs) SelectedDest = 0;
    GT_SetGadgetAttrs(WinGadgets[MonitorList], Win, NULL,
                      GA_Disabled, !outputs,
                      GTLV_Labels, (IPTR)&OutList,
                      GTLV_Selected, SelectedDest,
                      GTLV_MakeVisible, SelectedDest,
                      TAG_DONE);

    //
    // Init Gain
    //
    AHIGain = 0;
    SliderGain = 0;

    GainDiv = 1;
    MaxSliderGain = MaxAHIGain - MinAHIGain;
    while((MaxAHIGain - MinAHIGain) / GainDiv > 32) {
        GainDiv += 1;
        MaxSliderGain = (MaxAHIGain - MinAHIGain) / GainDiv;
    }

    SliderGain = MaxSliderGain * CmdDefaultGain / 100;
    SetGain(SliderGain);

//	printf("hgf MaxAHIGain = %d\n", MaxAHIGain);
//	printf("hgf MinAHIGain = %d\n", MinAHIGain);
//	printf("hgf GainDiv = %d\n", GainDiv);
//	printf("hgf MaxSliderGain = %d\n", MaxSliderGain);
//	printf("hgf SliderGain = %d\n", SliderGain);

    GT_SetGadgetAttrs(WinGadgets[InputGain], Win, NULL,
                      GA_Disabled, MinAHIGain == MaxAHIGain,
                      GTSL_Min, 0,
                      GTSL_Max, MaxSliderGain,
                      GTSL_Level, SliderGain,
                      TAG_DONE);

    //
    // Init Monitor Volume
    //
    AHIVolume = 0;
    SliderVolume = 0;

    VolumeDiv = 1;
    MaxSliderVolume = MaxAHIVolume - MinAHIVolume;
    while((MaxAHIVolume - MinAHIVolume) / VolumeDiv > 32) {
        VolumeDiv += 1;
        MaxSliderVolume = (MaxAHIVolume - MinAHIVolume) / VolumeDiv;
    }

//	printf("MaxAHIVolume = %d\n", MaxAHIVolume);
//	printf("MinAHIVolume = %d\n", MinAHIVolume);
//	printf("VolumeDiv = %d\n", VolumeDiv);
//	printf("MaxSliderVolume = %d\n", MaxSliderVolume);

    GT_SetGadgetAttrs(WinGadgets[MonitorVolume], Win, NULL,
                      GA_Disabled, MinAHIVolume == MaxAHIVolume,
                      GTSL_Min, 0,
                      GTSL_Max, MaxSliderVolume,
                      GTSL_Level, SliderVolume,
                      TAG_DONE);
}

void updaterightgadgets(void)
{
    BOOL EnableFileType;
    BOOL EnableAudioMode;
    BOOL EnablePrepareFile;
    BOOL EnableRecord;
    BOOL EnableStop;
    BOOL EnableClose;

    EnableFileType    = !FileSelected;
    EnableAudioMode   = !FileSelected;
    EnablePrepareFile = !FileSelected;
    EnableRecord      = FileSelected && ReadyToRecord;
    EnableStop        = EnableRecord;
    EnableClose       = AhiAllocd && !Recording;

    GT_SetGadgetAttrs(WinGadgets[FileFormat], Win, NULL,
                      GA_Disabled, !EnableFileType,
                      TAG_DONE);
    GT_SetGadgetAttrs(WinGadgets[ModeButton], Win, NULL,
                      GA_Disabled, !EnableAudioMode,
                      TAG_DONE);
    GT_SetGadgetAttrs(WinGadgets[PrepareFileButton], Win, NULL,
                      GA_Disabled, !EnablePrepareFile,
                      TAG_DONE);
    GT_SetGadgetAttrs(WinGadgets[RecordButton], Win, NULL,
                      GA_Disabled, !EnableRecord,
                      TAG_DONE);
    GT_SetGadgetAttrs(WinGadgets[StopButton], Win, NULL,
                      GA_Disabled, !EnableStop,
                      TAG_DONE);
    GT_SetGadgetAttrs(WinGadgets[CloseButton], Win, NULL,
                      GA_Disabled, !EnableClose,
                      TAG_DONE);
}

void updatebottomgadgets(void)
{
    if(!Recording) {
        if(FileSelected) PrintStatus(WinTexts[4].IText, WinTexts[5].IText);
        else {
            PrintStatus(WinTexts[6].IText, WinTexts[7].IText);
        }
    }
}

void WriteAIFFHeader(struct AsyncFile *OutFile, struct aiffheader *AIFFheader)
{
    WriteAsync(OutFile, &AIFFheader->FORMid,	4);
    WriteAsync(OutFile, &AIFFheader->FORMsize,	4);
    WriteAsync(OutFile, &AIFFheader->AIFFid,	4);
    WriteAsync(OutFile, &AIFFheader->COMMid,	4);
    WriteAsync(OutFile, &AIFFheader->COMMsize, 4);
    WriteAsync(OutFile, &AIFFheader->nTracks,	2);
    WriteAsync(OutFile, &AIFFheader->nFrames,	4);
    WriteAsync(OutFile, &AIFFheader->bpSmpl,	2);
    WriteAsync(OutFile, &AIFFheader->freq,		10);
    WriteAsync(OutFile, &AIFFheader->ANNOid,	4);
    WriteAsync(OutFile, &AIFFheader->ANNOsize,	4);
    WriteAsync(OutFile, &AIFFheader->ANNOtext,	40);
    WriteAsync(OutFile, &AIFFheader->SSNDid,	4);
    WriteAsync(OutFile, &AIFFheader->SSNDsize,	4);
    WriteAsync(OutFile, &AIFFheader->offset,	4);
    WriteAsync(OutFile, &AIFFheader->blockSize,	4);
}

void WriteWAVEHeader(struct AsyncFile *OutFile, struct waveheader *WAVEheader)
{
    WriteAsync(OutFile, &WAVEheader->RIFFid,		4);
    WriteAsync(OutFile, &WAVEheader->RIFFsize,		4);
    WriteAsync(OutFile, &WAVEheader->WAVEid,		4);
    WriteAsync(OutFile, &WAVEheader->LISTid,		4);
    WriteAsync(OutFile, &WAVEheader->LISTsize,		4);
    WriteAsync(OutFile, &WAVEheader->INFOid,		4);
    WriteAsync(OutFile, &WAVEheader->ISFTid,		4);
    WriteAsync(OutFile, &WAVEheader->ISFTsize,		4);
    WriteAsync(OutFile, &WAVEheader->ISFTtext,		40);
    WriteAsync(OutFile, &WAVEheader->fmtid,		4);
    WriteAsync(OutFile, &WAVEheader->fmtsize,		4);
    WriteAsync(OutFile, &WAVEheader->format,		2);
    WriteAsync(OutFile, &WAVEheader->mode,		2);
    WriteAsync(OutFile, &WAVEheader->freq,		4);
    WriteAsync(OutFile, &WAVEheader->bytes_sec,	4);
    WriteAsync(OutFile, &WAVEheader->bytes_smpl,	2);
    WriteAsync(OutFile, &WAVEheader->bits_smpl,	2);
    WriteAsync(OutFile, &WAVEheader->dataid,		4);
    WriteAsync(OutFile, &WAVEheader->datasize,		4);
}

BOOL StartSampling()
{
    BOOL	EnoughBufferMem;
    ULONG	buffersize;

    RecordData.fastcount = 0;
    RecordData.pending = 0;

    DrawLevel = TRUE;

    AutoLevelSignal = AllocSignal(-1);

    /* Create the level slave task */
    LevelSlaveSignal = AllocSignal(-1);
#ifdef DEBUG
    printf("\n\nMaster task says: I'll create the level slave task. (Signal: %d)\n", LevelSlaveSignal);
#endif

    RecordData.mastertask = FindTask(NULL);
#ifdef DEBUG
    printf("Master task closes his eyes and looks concentrated.\n");
#endif

    Forbid();
    RecordData.leveltask = CreateNewProcTags(
                               NP_Entry, (IPTR)LevelSlave,
                               NP_Name, (IPTR)"AHIRec Status Display",
                               NP_Priority, -1,
                               TAG_DONE);
    Permit();

    /* Wait for slave to come alive */
    if(RecordData.leveltask) {
        Wait(1L << LevelSlaveSignal);
    }
#ifdef DEBUG
    printf("Master task smiles.\n\n");
#endif

    if(actrl) {

#ifdef DEBUG
        printf("Hardware online, ");
#endif

        /** Get the actual mixing/recording frequency **/
        AHI_ControlAudio(actrl,
                         AHIC_MixFreq_Query, (IPTR)&recfreq,
                         TAG_DONE);

        /** Chose the correct RecordFunc **/
        recordhook.h_Entry = (APTR)&RecordFuncS;

        /** Make sure our own buffers are larger (or equal) than AHI's. **/
        AHI_GetAudioAttrs(AHI_INVALID_ID, actrl, AHIDB_MaxRecordSamples, (IPTR)&RecordData.bufferlen, TAG_DONE);

        LevelFrames = RecordData.bufferlen;
#ifdef DEBUG
        printf("%d samples per cycle.\n", LevelFrames);
#endif

        if(BIGBUFFERSIZE > RecordData.bufferlen) RecordData.bufferlen = BIGBUFFERSIZE;

#ifdef DEBUG
        printf("Selected bufferlen: %d, ", RecordData.bufferlen);
#endif

        /** Init RecordData.count **/
        RecordData.count = RecordData.bufferlen;

        if(RecBits < 16) RecBits = 16;

        /** buffersize is the size in bytes instead of sample frames.       **/
        /** we allocate 32bits/sample to make sure it's always large anough **/
        buffersize = RecordData.bufferlen * 4 * 2;

        /** Allocate own buffers **/
        EnoughBufferMem = FALSE;
        RecordData.buffer1 = NULL;
        RecordData.buffer2 = NULL;
        RecordData.bufferS = NULL;
        if(RecordData.buffer1 = AllocVec(buffersize + LevelFrames * 4, MEMF_PUBLIC | MEMF_CLEAR)) {
            if(RecordData.buffer2 = AllocVec(buffersize + LevelFrames * 4, MEMF_PUBLIC | MEMF_CLEAR)) {
                if(RecordData.bufferS = AllocVec(buffersize + LevelFrames * 4, MEMF_PUBLIC | MEMF_CLEAR)) {
                    RecordData.dstptr = RecordData.buffer1;
                    EnoughBufferMem = TRUE;
                }
            }
        }


        if(EnoughBufferMem) {

#ifdef DEBUG
            printf("buffers online. ");
#endif

            /** Set up the hardware **/

            AHI_ControlAudio(actrl,
                             AHIC_MonitorVolume, AHIVolume,
                             AHIC_InputGain, AHIGain,
                             AHIC_Input, source,
                             AHIC_Output, dest,
                             TAG_DONE);

#ifdef DEBUG
            printf("All systems nominal!\n");
#endif

            if(!AHI_ControlAudio(actrl, AHIC_Record, TRUE, TAG_DONE)) {

#ifdef DEBUG
                printf("Sampling started.\n");
#endif

                return TRUE;
            }
        }
    }
    StopSampling();
    return FALSE;
}

void StopSampling(void)
{
    int i;

    DrawLevel = FALSE;
    Delay(5);

    /** Clean up **/
    if(actrl) {
        AHI_ControlAudio(actrl, AHIC_MixFreq_Query, (IPTR)&recfreq, TAG_DONE);
#ifdef DEBUG
        printf("Stopping record.\n"); /* !!! */
#endif
        AHI_ControlAudio(actrl, AHIC_Record, FALSE, TAG_DONE);
    }
#ifdef DEBUG
    printf("Record stopped.\n"); /* !!! */
#endif

    /* Kill level slave task */
    if(RecordData.leveltask) {
#ifdef DEBUG
        printf("Trying to remove LevelMeter task.\n"); /* !!! */
#endif
        Signal((struct Task *)RecordData.leveltask, SIGBREAKF_CTRL_C);
        Delay(5);
        if(RecordData.leveltask) for(i = 0; i < 50000; i++) {
                Delay(1);
                if(RecordData.leveltask == NULL) break;
            }
        if(RecordData.leveltask) {
            RemTask((struct Task *)RecordData.leveltask);
            RecordData.leveltask = NULL;
        }
    }
    if(LevelSlaveSignal != -1) FreeSignal(LevelSlaveSignal);
    LevelSlaveSignal = -1;
    if(AutoLevelSignal != -1) FreeSignal(AutoLevelSignal);
    AutoLevelSignal = -1;
#ifdef DEBUG
    printf("LevelMeter task successfully removed.\n"); /* !!! */
#endif

    if(FileOpen) {
        FinishFile();
        FileOpen = FALSE;
    }

    Recording = FALSE;
    Pause    = FALSE;


    if(RecordData.buffer1) {
        FreeVec(RecordData.buffer1);
        RecordData.buffer1 = NULL;
    }
    if(RecordData.buffer2) {
        FreeVec(RecordData.buffer2);
        RecordData.buffer2 = NULL;
    }
    if(RecordData.bufferS) {
        FreeVec(RecordData.bufferS);
        RecordData.bufferS = NULL;
    }
}

void SetGain(ULONG value)
{
    SliderGain = value;
    AHIGain = value * GainDiv + MinAHIGain;
    if(AHIGain > MaxAHIGain) AHIGain = MaxAHIGain;
//	printf("hgf SetGain(%d->%d)\n", value, AHIGain);
    if(actrl) AHI_ControlAudio(actrl, AHIC_InputGain, AHIGain, TAG_DONE);
}

void SetVolume(ULONG value)
{
    SliderVolume = value;
    AHIVolume = value * VolumeDiv + MinAHIVolume;
    if(AHIVolume > MaxAHIVolume) AHIVolume = MaxAHIVolume;
//	printf("hgf SetVolume(%d->%d)\n", value, AHIVolume);
    if(actrl) AHI_ControlAudio(actrl, AHIC_MonitorVolume, AHIVolume, TAG_DONE);
}

BOOL openAHI(void)
{
    struct TagItem EmptyTagList[] = {TAG_DONE};
    if(AHImp = CreateMsgPort()) {
        if(AHIio = (struct AHIRequest *)CreateIORequest(AHImp, sizeof(struct AHIRequest))) {
            AHIio->ahir_Version = 4;
            if((AHIDevice = OpenDevice(AHINAME, AHI_NO_UNIT, (struct IORequest *)AHIio, 0)) == 0) {
                AHIBase = (struct Library *)AHIio->ahir_Std.io_Device;
#ifdef __amigaos4__
                IAHI = (struct AHIIFace *)GetInterface(AHIBase, "main", 1, NULL);
#endif
                amreq = AHI_AllocAudioRequestA(EmptyTagList);
                if(amreq) {
                    return TRUE;
                }
            }
        }
    }
    return FALSE;
}

void closeAHI(void)
{
    if(actrl) AHI_FreeAudio(actrl);
    AhiAllocd = FALSE;
    if(amreq) AHI_FreeAudioRequest(amreq);
#ifdef __amigaos4__
    if(IAHI) DropInterface((struct Interface *)IAHI);
#endif
    if(!AHIDevice) CloseDevice((struct IORequest *)AHIio);
    if(AHIio) DeleteIORequest((struct IORequest *)AHIio);
    if(AHImp) DeleteMsgPort(AHImp);

    actrl = (struct AHIAudioCtrl *)NULL;
    amreq = (struct AHIAudioModeRequester *)NULL;
    AHIio = (struct AHIRequest *)NULL;
#ifdef __amigaos4__
    IAHI = (struct AHIIFace *)NULL;
#endif
    AHImp = (struct MsgPort *)NULL;
    AHIDevice = -1;
}

void GetPens(void)
{
    /** Grab some pens **/
    /*-------------------------------------------------------------------------------------------------------------------------------------------------------*/
    if(DGreenPen == -1) {
        RGreenPen  = ObtainBestPen(Win->WScreen->ViewPort.ColorMap, 21 * 1 << 26, 55 * 1 << 26, 21 * 1 << 26, OBP_Precision,
                                   PRECISION1, OBP_FailIfBad, TRUE, TAG_DONE);
        if(RGreenPen  == -1) RGreenPen  = ObtainBestPen(Win->WScreen->ViewPort.ColorMap, 21 * 1 << 26, 55 * 1 << 26,
                                              21 * 1 << 26, OBP_Precision, PRECISION2, OBP_FailIfBad, TRUE, TAG_DONE);
        if(RGreenPen  == -1) GreenPen  = 2;
        else GreenPen  = RGreenPen;
    } else {
        RGreenPen  = -1;
        GreenPen   = DGreenPen;
    }
    /*-------------------------------------------------------------------------------------------------------------------------------------------------------*/
    if(DYellowPen == -1) {
        RYellowPen  = ObtainBestPen(Win->WScreen->ViewPort.ColorMap, 59 * 1 << 26, 37 * 1 << 26,  0 * 1 << 26, OBP_Precision,
                                    PRECISION1, OBP_FailIfBad, TRUE, TAG_DONE);
        if(RYellowPen  == -1) RYellowPen  = ObtainBestPen(Win->WScreen->ViewPort.ColorMap, 59 * 1 << 26, 37 * 1 << 26,
                                                0 * 1 << 26, OBP_Precision, PRECISION2, OBP_FailIfBad, TRUE, TAG_DONE);
        if(RYellowPen  == -1) YellowPen  = 2;
        else YellowPen  = RYellowPen;
    } else {
        RYellowPen  = -1;
        YellowPen   = DYellowPen;
    }
    /*-------------------------------------------------------------------------------------------------------------------------------------------------------*/
    if(DRedPen == -1) {
        RRedPen  = ObtainBestPen(Win->WScreen->ViewPort.ColorMap, 59 * 1 << 26, 17 * 1 << 26, 17 * 1 << 26, OBP_Precision,
                                 PRECISION1, OBP_FailIfBad, TRUE, TAG_DONE);
        if(RRedPen  == -1) RRedPen  = ObtainBestPen(Win->WScreen->ViewPort.ColorMap, 59 * 1 << 26, 17 * 1 << 26, 17 * 1 << 26,
                                          OBP_Precision, PRECISION2, OBP_FailIfBad, TRUE, TAG_DONE);
        if(RRedPen  == -1) RedPen  = 2;
        else RedPen  = RRedPen;
    } else {
        RRedPen  = -1;
        RedPen   = DRedPen;
    }
    /*-------------------------------------------------------------------------------------------------------------------------------------------------------*/

    /* Use default if no suitable pen was found */

#ifdef DEBUG
    printf("DPens: %d, %d, %d\n", DGreenPen, DYellowPen, DRedPen);
    printf(" Pens: %d, %d, %d\n", RGreenPen, RYellowPen, RRedPen);
#endif
}

void FreePens(void)
{
    if(RGreenPen  != -1) ReleasePen(Win->WScreen->ViewPort.ColorMap, RGreenPen);
    if(RYellowPen != -1) ReleasePen(Win->WScreen->ViewPort.ColorMap, RYellowPen);
    if(RRedPen    != -1) ReleasePen(Win->WScreen->ViewPort.ColorMap, RRedPen);
}

void ComputeLevelCoordinates(void)
{
    struct DrawInfo *dri;
    ULONG xmin, xmax;
    int		i;

    sx = scalex;
    sy = scaley;
    ox = Win->BorderLeft;
    oy = Win->BorderTop;

    xmin = 65535 * (11 * sx / 65535 + ox + 2);
    xmax = 65535 * (11 * sx / 65535 + ox + 246 * sx / 65535 - 3);

    LevelY1l = 114 * sy / 65535 + oy + 2;
    LevelY2l = 114 * sy / 65535 + oy + 13 * sy / 65535 - 3;
    LevelYSl = LevelY2l - LevelY1l + 1;

    LevelY1r = 128 * sy / 65535 + oy + 2;
    LevelY2r = 128 * sy / 65535 + oy + 13 * sy / 65535 - 3;
    LevelYSr = LevelY2r - LevelY1r + 1;

    for(i = 0; i < 16; i++) {
        LevelX1[i] = (xmin + i    * ((xmax - xmin) / 16)) / 65535 + 2;
        LevelX2[i] = (xmin + (i + 1) * ((xmax - xmin) / 16)) / 65535 - 1;
        LevelXS[i] = LevelX2[i] - LevelX1[i] + 1;
    }

    for(i = 0; i < 13; i++) {
        LevelP[i] = GreenPen;
        LevelC[i] = 0x0033DD33;
    }
    LevelP[13] = YellowPen;
    LevelC[13] = 0x00EEAA33;
    LevelP[14] = YellowPen;
    LevelC[14] = 0x00EEAA33;
    LevelP[15] = RedPen;
    LevelC[15] = 0x00EE4444;

    dri = GetScreenDrawInfo(Win->WScreen);
    ScreenFont = dri->dri_Font;
    SetFont(Win->RPort, ScreenFont);

    TextX  = 272 * sx / 65535 + ox;
    TextY1 = 129 * sy / 65535 + oy;
    TextY2 = 138 * sy / 65535 + oy;
}

void PrintStatus(char *string1, char *string2)
{
    WORD x1, y1, x2, y2;

    x1 = 267 * sx / 65535 + ox + 2;
    y1 = 119 * sy / 65535 + oy + 1;

    x2 = x1 + WDH * sx / 65535 - 5;
    y2 = y1 + 25 * sy / 65535 - 3;

    SetAPen(Win->RPort, 0);
    RectFill(Win->RPort, x1, y1, x2, y2);
    SetABPenDrMd(Win->RPort, 1, 0, JAM2);
    Move(Win->RPort, TextX, TextY1);
    Text(Win->RPort, string1, strlen(string1));
    if(string2) {
        Move(Win->RPort, TextX, TextY2);
        Text(Win->RPort, string2, strlen(string2));
    }
}

#ifdef __SASC
void __asm __saveds LevelSlave(void)
{
#else
void LevelSlave(void)
{
#endif
//                       -60 -50 -45 -40 -35 -30  -25 -22.5 -18.5 -14.5 -11.5  -8.5  -5.5  -3.5  -1.5  -0.5
    UWORD LevelQuants[17] = { 40, 98, 174, 309, 550, 978, 1550, 2457, 3894, 6172, 8718, 12315, 17395, 21900, 27570, 30934, 32769};
    WORD	*LevelTable;

    register LONG		i;
    register UWORD	MaxL;
    register UWORD	MaxR;

    UBYTE	SegmentL[16];
    UBYTE	SegmentR[16];

    ULONG		signals;

    ULONG		prevsecs;

    BYTE		TPeakL;
    BYTE		TPeakR;
    BYTE		PeakL;
    BYTE		PeakR;

#ifdef DEBUG
    printf("Scope slave task arrives in a puff of smoke.\n");
#endif

    LevelAdjustTimer = 0;

    /* Get some SigBits for receiving signals from MasterTask */
    RecordData.smallsignal = AllocSignal(-1);
    if(RecordData.smallsignal == -1) goto QuitSlave;
    RecordData.smallsignalf = (1L << RecordData.smallsignal);

    MaxL = 0;
    MaxR = 0;
    TPeakL = 0;
    TPeakR = 0;
    prevsecs = 0;

    for(i = 0; i < 16; i++) {
        SegmentL[i] = 0;
        SegmentR[i] = 0;
    }

    LevelTable = AllocVec(65538, MEMF_PUBLIC | MEMF_CLEAR);
    {
        UWORD	from, to;
        LONG	j;
        from = 0;
        for(i = 0; i < 17; i++) {
            to = LevelQuants[i];
            for(j = from; j < to; j++) {
                LevelTable[j] = i;
            }
            from = to;
        }
    }

    /* Everything set up. Tell Master we're alive and healthy. */
#ifdef DEBUG
    printf("Scope slave task sings: I'm alive I'm alive la la la...\n");
#endif
    Signal((struct Task *)RecordData.mastertask, 1L << LevelSlaveSignal);


    for(;;) {
        signals = Wait(SIGBREAKF_CTRL_C | RecordData.smallsignalf);
        if(signals & SIGBREAKF_CTRL_C) break;

        if(DrawLevel) {

            {
                register UWORD Lsub;
                register UWORD Rsub;

                Lsub = MaxL / LevelDivide;
                Rsub = MaxR / LevelDivide;

                if(Lsub) MaxL -= Lsub;
                else     MaxL  = 0;

                if(Rsub) MaxR -= Rsub;
                else     MaxR  = 0;
            }

            {
                register UWORD CL;
                register UWORD CR;
                for(i = 0; i < RecordData.cblen; i++) {
                    switch(RecordData.type) {
                    case AHIST_S16S:
                        CL = abs(RecordData.cbuffer[i * 2]);
                        CR = abs(RecordData.cbuffer[i * 2 + 1]);
                        break;
                    case AHIST_S32S:
                        CL = abs(RecordData.cbuffer[i * 4]);
                        CR = abs(RecordData.cbuffer[i * 4 + 2]);
                        break;
                    default:
                        CL = 0;
                        CR = 0;
                        break;
                    }
                    if(CL > MaxL) MaxL = CL;
                    if(CR > MaxR) MaxR = CR;
                }
            }

            PeakL = LevelTable[MaxL];
            PeakR = LevelTable[MaxR];

            if(PeakL != TPeakL || PeakR != TPeakR) {
                register struct RastPort *rp = Win->RPort;

                for(i = 0; i < 16; i++) {

                    if(PeakL > i) {
                        if(SegmentL[i] == 0) {
                            SegmentL[i] = 1;
#ifdef __amigaos4__
                            SetAPen(rp, LevelP[i]);
                            RectFill(rp, LevelX1[i], LevelY1l, LevelX2[i], LevelY2l);
#else
                            if(IsCyber) {
                                FillPixelArray(rp, LevelX1[i], LevelY1l, LevelXS[i], LevelYSl, LevelC[i]);
                            } else {
                                SetAPen(rp, LevelP[i]);
                                RectFill(rp, LevelX1[i], LevelY1l, LevelX2[i], LevelY2l);
                            }
#endif
                        }
                    } else {
                        if(SegmentL[i] == 1) {
                            SegmentL[i] = 0;
                            SetAPen(rp, 0);
                            RectFill(rp, LevelX1[i], LevelY1l, LevelX2[i], LevelY2l);
                        }
                    }

                    if(PeakR > i) {
                        if(SegmentR[i] == 0) {
                            SegmentR[i] = 1;
#ifdef __amigaos4__
                            SetAPen(rp, LevelP[i]);
                            RectFill(rp, LevelX1[i], LevelY1r, LevelX2[i], LevelY2r);
#else
                            if(IsCyber) {
                                FillPixelArray(rp, LevelX1[i], LevelY1r, LevelXS[i], LevelYSr, LevelC[i]);
                            } else {
                                SetAPen(rp, LevelP[i]);
                                RectFill(rp, LevelX1[i], LevelY1r, LevelX2[i], LevelY2r);
                            }
#endif
                        }
                    } else {
                        if(SegmentR[i] == 1) {
                            SegmentR[i] = 0;
                            SetAPen(rp, 0);
                            RectFill(rp, LevelX1[i], LevelY1r, LevelX2[i], LevelY2r);
                        }
                    }
                }
            }

            TPeakL = PeakL;
            TPeakR = PeakR;

            if(CmdAutoLevels) {
                if(MaxL > LevelQuants[15] || MaxR > LevelQuants[15]) {
                    LevelAdjustTimer = 1000;
                    LevelAdjustDir   = -2;
                    Signal(RecordData.mastertask, 1L << AutoLevelSignal);
                } else if(MaxL > LevelQuants[14] || MaxR > LevelQuants[14]) {
                    if(LevelAdjustTimer < 500) LevelAdjustTimer = 500;
                    LevelAdjustDir   = -1;
                    Signal(RecordData.mastertask, 1L << AutoLevelSignal);
                } else if(MaxL > LevelQuants[13] || MaxR > LevelQuants[13]) {
                    if(LevelAdjustTimer < 250) LevelAdjustTimer = 250;
                    LevelAdjustDir   = 0;
                } else {
                    if(LevelAdjustTimer > 0) LevelAdjustTimer--;
                    if(LevelAdjustTimer == 0) {
                        if(MaxL < LevelQuants[13] && MaxR < LevelQuants[13]) {
                            LevelAdjustDir   = 1;
                            Signal(RecordData.mastertask, 1L << AutoLevelSignal);
                            LevelAdjustTimer = 25;
                        }
                    }
                }
            }

            if(Recording) {
                register ULONG seconds;
                register UWORD h, m, s;

                seconds = RecordData.fastcount / recfreq;
                if(seconds > prevsecs) {
                    prevsecs = seconds;
                    h = seconds / 3600;
                    m = (seconds - h * 3600) / 60;
                    s = seconds - h * 3600 - m * 60;

                    if(RecordData.pending == 0) {
                        sprintf(Counter, "Time: %d:%02d:%02d", h, m, s);
                    } else {
                        sprintf(Counter, "OUT OF SYNC! ");
                    }

                    Move(Win->RPort, TextX, TextY2);
                    SetABPenDrMd(Win->RPort, 1, 0, JAM2);
                    Text(Win->RPort, Counter, 13);
                }

                if(RecordingTime) {
                    if(seconds / 60 >= RecordingTime) {
                        Signal(RecordData.mastertask, SIGBREAKF_CTRL_C);
                    }
                }
            }

        } else {
            SetAPen(Win->RPort, 0);
            for(i = 0; i < 16; i++) {
                RectFill(Win->RPort, LevelX1[i], LevelY1l, LevelX2[i], LevelY2l);
                RectFill(Win->RPort, LevelX1[i], LevelY1r, LevelX2[i], LevelY2r);
            }
        }
    }

QuitSlave:
    Forbid();
    SetAPen(Win->RPort, 0);
    for(i = 0; i < 16; i++) {
        RectFill(Win->RPort, LevelX1[i], LevelY1l, LevelX2[i], LevelY2l);
        RectFill(Win->RPort, LevelX1[i], LevelY1r, LevelX2[i], LevelY2r);
    }

    if(RecordData.smallsignal != -1) FreeSignal(RecordData.smallsignal);
    RecordData.smallsignal = -1;
    RecordData.smallsignalf = 0;

    /* Multitaking will resume when we are dead. */
    RecordData.leveltask = NULL;
}

void BiasS16S(WORD *Buf, ULONG Samples)
{
    ULONG i;
    WORD *lbuf;
    WORD *rbuf;
    LONG BiasL;
    LONG BiasR;

    /* Initialize bias counters */
    BiasL = 0;
    BiasR = 0;

    /* Initialize buffer pointers */
    lbuf = (WORD *)Buf;
    rbuf = (WORD *)Buf + 1;

    /* Sum up all samples in buffer */
    for(i = 0; i < Samples; i++) {
        BiasL += *lbuf;
        BiasR += *rbuf;
        lbuf += 2;
        rbuf += 2;
    }

    /* Calculate bias */
    BiasL /= (LONG)Samples;
    BiasR /= (LONG)Samples;

    /* Initialize buffer pointers */
    lbuf = (WORD *)Buf;
    rbuf = (WORD *)Buf + 1;

    /* Neutralize bias */
    for(i = 0; i < Samples; i++) {
        LONG val;

        val = *lbuf;
        val -= BiasL;
        if(val < -32768) val = -32768;
        if(val >  32767) val =  32767;
        *lbuf = (WORD)val;

        val = *rbuf;
        val -= BiasR;
        if(val < -32768) val = -32768;
        if(val >  32767) val =  32767;
        *rbuf = (WORD)val;

        lbuf += 2;
        rbuf += 2;
    }
}


ULONG ConvS16S(WORD *SrcBuf, WORD *DstBuf, ULONG Samples)
{
    ULONG i;
    ULONG BytesConverted;

    BytesConverted = 0;
    switch(FileType) {
    case AIFF16: {
        ULONG *src = (ULONG *)SrcBuf;
        ULONG *dst = (ULONG *)DstBuf;
        BytesConverted = Samples * 4;
        for(i = 0; i < Samples; i++) {
            *dst++ = *src++;
        }
    }
    break;

    case AIFF24: {
        UBYTE *src = (UBYTE *)SrcBuf;
        UBYTE *dst = (UBYTE *)DstBuf;
        BytesConverted = Samples * 6;
        for(i = 0; i < Samples; i++) {
            *dst++ = *src++;
            *dst++ = *src++;
            *dst++ = 0;
            *dst++ = *src++;
            *dst++ = *src++;
            *dst++ = 0;
        }
    }
    break;

    case WAVE16: {
        UBYTE *src = (UBYTE *)SrcBuf;
        UBYTE *dst = (UBYTE *)DstBuf;
        UBYTE lo, hi;
        BytesConverted = Samples * 4;
        for(i = 0; i < Samples; i++) {
            hi = *src++;
            lo = *src++;
            *dst++ = lo;
            *dst++ = hi;
            hi = *src++;
            lo = *src++;
            *dst++ = lo;
            *dst++ = hi;
        }
    }
    break;

    case WAVE24: {
        UBYTE *src = (UBYTE *)SrcBuf;
        UBYTE *dst = (UBYTE *)DstBuf;
        UBYTE lo, hi;
        BytesConverted = Samples * 6;
        for(i = 0; i < Samples; i++) {
            hi = *src++;
            lo = *src++;
            *dst++ = 0;
            *dst++ = lo;
            *dst++ = hi;
            hi = *src++;
            lo = *src++;
            *dst++ = 0;
            *dst++ = lo;
            *dst++ = hi;
        }
    }
    break;

#ifdef USE_FLAC
    case FLAC16: {
        UWORD *src = (UWORD *)SrcBuf;
        ULONG *dst = (ULONG *)DstBuf;
        BytesConverted = Samples * 8;
        for(i = 0; i < Samples; i++) {
            *dst++ = *src++;
            *dst++ = *src++;
        }
    }
    break;
#endif
    }
    return(BytesConverted);
}

ULONG ConvS32S(WORD *SrcBuf, WORD *DstBuf, ULONG Samples)
{
    ULONG i;
    ULONG BytesConverted;

    BytesConverted = 0;
    switch(FileType) {
    case AIFF16: {
        UWORD *src = (UWORD *)SrcBuf;
        UWORD *dst = (UWORD *)DstBuf;
        BytesConverted = Samples * 4;
        for(i = 0; i < Samples; i++) {
            *dst++ = *src++;
            src++;
            *dst++ = *src++;
            src++;
        }
    }
    break;

    case AIFF24: {
        UBYTE *src = (UBYTE *)SrcBuf;
        UBYTE *dst = (UBYTE *)DstBuf;
        BytesConverted = Samples * 6;
        for(i = 0; i < Samples; i++) {
            *dst++ = *src++;
            *dst++ = *src++;
            *dst++ = *src++;
            src++;
            *dst++ = *src++;
            *dst++ = *src++;
            *dst++ = *src++;
            src++;
        }
    }
    break;

    case WAVE16: {
        UBYTE *src = (UBYTE *)SrcBuf;
        UBYTE *dst = (UBYTE *)DstBuf;
        UBYTE lo, hi;
        BytesConverted = Samples * 4;
        for(i = 0; i < Samples; i++) {
            hi = *src++;
            lo = *src++;
            *dst++ = lo;
            *dst++ = hi;
            src += 2;
            hi = *src++;
            lo = *src++;
            *dst++ = lo;
            *dst++ = hi;
            src += 2;
        }
    }
    break;

    case WAVE24: {
        UBYTE *src = (UBYTE *)SrcBuf;
        UBYTE *dst = (UBYTE *)DstBuf;
        UBYTE lo, md, hi;
        BytesConverted = Samples * 6;
        for(i = 0; i < Samples; i++) {
            hi = *src++;
            md = *src++;
            lo = *src++;
            src++;
            *dst++ = lo;
            *dst++ = md;
            *dst++ = hi;
            hi = *src++;
            md = *src++;
            lo = *src++;
            src++;
            *dst++ = lo;
            *dst++ = md;
            *dst++ = hi;
        }
    }
    break;

#ifdef USE_FLAC
    case FLAC16: {
        ULONG *src = (ULONG *)SrcBuf;
        ULONG *dst = (ULONG *)DstBuf;
        BytesConverted = Samples * 8;
        for(i = 0; i < Samples; i++) {
            *dst++ = *src++;
            *dst++ = *src++;
        }
    }
    break;
#endif
    }
    return(BytesConverted);
}

#ifdef __SASC
void __asm __saveds SaveSlave(void)
#else
void SaveSlave(void)
#endif
{
    ULONG		signals;
    ULONG		BytesToWrite;
    ULONG		written;
    struct AsyncFile *AsyncFile = NULL;
    FILE *BufferedFile = NULL;

#ifdef USE_FLAC
    FLAC__StreamEncoder *FlacEncoder = NULL;
    FLAC__StreamEncoderInitStatus Status;
#endif

#ifdef DEBUG
    printf("Save slave task arrives in a puff of smoke.\n");
#endif

    /* Get some SigBits for receiving signals from MasterTask */
    RecordData.bigsignal = AllocSignal(-1);
    if(RecordData.bigsignal == -1) goto QuitSaveSlave;
    RecordData.bigsignalf = (1L << RecordData.bigsignal);

    /* Create File */
    FileOpen = FALSE;
    written = 0;
    switch(FileType) {
    case AIFF16:
    case AIFF24:
        AsyncFile = OpenAsync(SaveFileName, MODE_WRITE, 4 * BIGBUFFERSIZE * AsyncBuffers);
        if(AsyncFile) {
            WriteAIFFHeader(AsyncFile, &AIFFheader);
            FileOpen = TRUE;
        }
        break;
    case WAVE16:
    case WAVE24:
        AsyncFile = OpenAsync(SaveFileName, MODE_WRITE, 4 * BIGBUFFERSIZE * AsyncBuffers);
        if(AsyncFile) {
            WriteWAVEHeader(AsyncFile, &WAVEheader);
            FileOpen = TRUE;
        }
        break;
#ifdef USE_FLAC
    case FLAC16:
        BufferedFile = fopen(SaveFileName, "wb");
        if(BufferedFile) {
            FlacEncoder = FLAC__stream_encoder_new();
            FLAC__stream_encoder_set_compression_level(FlacEncoder, CmdCompLevel);
            FLAC__stream_encoder_set_channels(FlacEncoder, 2);
            FLAC__stream_encoder_set_bits_per_sample(FlacEncoder, 16);
            FLAC__stream_encoder_set_sample_rate(FlacEncoder, recfreq);
            // FLAC__stream_encoder_init_stream(FlacEncoder, FlacWriteCallback, FlacSeekCallback, FlacTellCallback, NULL, AsyncFile);
            FLAC__stream_encoder_init_stream(FlacEncoder, FlacWriteBufferedCallback, FlacSeekBufferedCallback,
                                             FlacTellBufferedCallback, NULL, BufferedFile);
            FileOpen = TRUE;
        }
        break;
#endif
    }

    /* Everything set up. Tell Master we're alive and healthy. */
#ifdef DEBUG
    printf("Save slave task sings: I'm alive I'm alive la la la...\n");
#endif
    Signal((struct Task *)RecordData.mastertask, 1L << SaveSlaveSignal);


    if(FileOpen) for(;;) {
            signals = Wait(SIGBREAKF_CTRL_C | RecordData.bigsignalf);
            if(signals & SIGBREAKF_CTRL_C) break;

            if(RecordData.pending) {
                RecordData.pending--;
            }

            if(RecordData.pending) {
                sprintf(Counter, "OUT OF SYNC! ");
                Move(Win->RPort, TextX, TextY2);
                SetABPenDrMd(Win->RPort, 1, 0, JAM2);
                Text(Win->RPort, Counter, 13);
                Delay(1);
            }

            /* Now the buffer that buffer2 points to is ready to be saved. */
            if(Recording && !Pause) {
                BytesToWrite = 0;
                switch(RecordData.type) {
                case AHIST_S8S:
                    break;
                case AHIST_S16S:
                    if(CmdAutobias) BiasS16S(RecordData.buffer2, BIGBUFFERSIZE);
                    BytesToWrite = ConvS16S(RecordData.buffer2, RecordData.bufferS, BIGBUFFERSIZE);
                    break;
                case AHIST_S32S:
                    BytesToWrite = ConvS32S(RecordData.buffer2, RecordData.bufferS, BIGBUFFERSIZE);
                    break;
                }

                switch(FileType) {
                case AIFF16:
                case AIFF24:
                case WAVE16:
                case WAVE24:
                    written = WriteAsync(AsyncFile, RecordData.bufferS, BytesToWrite);
                    break;
#ifdef USE_FLAC
                case FLAC16:
                    if(FlacEncoder) {
                        if(FLAC__stream_encoder_process_interleaved(FlacEncoder, (FLAC__int32 *)RecordData.bufferS, BIGBUFFERSIZE)) {
                            written = BytesToWrite;
                        } else {
                            written = 0;
                        }
                    }
                    break;
#endif
                }

                if(written != BytesToWrite) WriteError = TRUE;
                BytesWritten += written;
            }
            if(WriteError) {
                Recording = FALSE;
                EasyRequest(NULL, &RecErrorRequester, 0);
                goto QuitSaveSlave;
            }
        }

QuitSaveSlave:

#ifdef DEBUG
    printf("Closing file.\n");
#endif

    switch(FileType) {
    case AIFF16:
    case AIFF24:
        if(AsyncFile) {
            if(BytesWritten != 0) {
                if(FileType == AIFF16) {
                    AIFFheader.bpSmpl   = 16;
                    AIFFheader.nFrames  = BytesWritten / 4;
                } else {
                    AIFFheader.bpSmpl   = 24;
                    AIFFheader.nFrames  = BytesWritten / 6;
                }
                AIFFheader.SSNDsize = BytesWritten + 4 + 4; // +4+4: offset and blockSize
                ConvertToIeeeExtended((double) recfreq, AIFFheader.freq);
                AIFFheader.nTracks  = 2;
                AIFFheader.FORMsize = BytesWritten + 90 + 4 + 4; // +4+4: offset and blockSize
                SeekAsync(AsyncFile, 0, MODE_START);
                WriteAIFFHeader(AsyncFile, &AIFFheader);
            }
            CloseAsync(AsyncFile);
            Delay(10);
        }
        break;

    case WAVE16:
    case WAVE24:
        if(AsyncFile) {
            if(BytesWritten != 0) {
                if(FileType == WAVE16) {
                    WAVEheader.bytes_sec  = recfreq * 4;
                    WAVEheader.bytes_smpl = 0x0400;
                    WAVEheader.bits_smpl  = 0x1000;
                } else {
                    WAVEheader.bytes_sec  = recfreq * 6;
                    WAVEheader.bytes_smpl = 0x0600;
                    WAVEheader.bits_smpl  = 0x1800;
                }
                WAVEheader.datasize   = BytesWritten;
                WAVEheader.freq       = recfreq;
                WAVEheader.mode       = 0x0200;
                WAVEheader.RIFFsize   = BytesWritten + 100;

                ConvLong(&WAVEheader.datasize);
                ConvLong(&WAVEheader.bytes_sec);
                ConvLong(&WAVEheader.freq);
                ConvLong(&WAVEheader.RIFFsize);

                SeekAsync(AsyncFile, 0, MODE_START);
                WriteWAVEHeader(AsyncFile, &WAVEheader);
            }
            CloseAsync(AsyncFile);
            Delay(10);
        }
        break;
#ifdef USE_FLAC
    case FLAC16:
        if(BufferedFile) {
            if(FlacEncoder) {
                FLAC__stream_encoder_finish(FlacEncoder);
                FLAC__stream_encoder_delete(FlacEncoder);
                FlacEncoder = NULL;
            }
            fclose(BufferedFile);
        }
        break;
#endif
    }
    FileOpen = FALSE;

#ifdef DEBUG
    printf("\nSave slave task waves happily.\n");
    printf("Save slave task disappears in a puff of smoke.\n");
#endif

    Forbid();
    if(RecordData.bigsignal != -1) FreeSignal(RecordData.bigsignal);
    RecordData.bigsignal = -1;
    RecordData.bigsignalf = 0;

    /* Tell the Master we're dying */
    Signal((struct Task *)RecordData.mastertask, 1L << SaveSlaveSignal);
    /* Multitaking will resume when we are dead. */
}

/*
 * C O N V E R T   T O   I E E E   E X T E N D E D
 */

/* Copyright (C) 1988-1991 Apple Computer, Inc.
 * All rights reserved.
 *
 * Machine-independent I/O routines for IEEE floating-point numbers.
 *
 * NaN's and infinities are converted to HUGE_VAL or HUGE, which
 * happens to be infinity on IEEE machines.  Unfortunately, it is
 * impossible to preserve NaN's in a machine-independent way.
 * Infinities are, however, preserved on IEEE machines.
 *
 * These routines have been tested on the following machines:
 *    Apple Macintosh, MPW 3.1 C compiler
 *    Apple Macintosh, THINK C compiler
 *    Silicon Graphics IRIS, MIPS compiler
 *    Cray X/MP and Y/MP
 *    Digital Equipment VAX
 *
 *
 * Implemented by Malcolm Slaney and Ken Turkowski.
 *
 * Malcolm Slaney contributions during 1988-1990 include big- and little-
 * endian file I/O, conversion to and from Motorola's extended 80-bit
 * floating-point format, and conversions to and from IEEE single-
 * precision floating-point format.
 *
 * In 1991, Ken Turkowski implemented the conversions to and from
 * IEEE double-precision format, added more precision to the extended
 * conversions, and accommodated conversions involving +/- infinity,
 * NaN's, and denormalized numbers.
 */

# define FloatToUnsigned(f)      ((unsigned long)(((long)(f - 2147483648.0)) + 2147483647L) + 1)

void ConvertToIeeeExtended(double num, char *bytes)
{
    int			sign;
    int			expon;
    double	fMant, fsMant;
    ULONG		hiMant, loMant;

    if(num < 0) {
        sign = 0x8000;
        num *= -1;
    } else sign = 0;

    if(num == 0) {
        expon = 0;
        hiMant = 0;
        loMant = 0;
    } else {
        fMant = frexp(num, &expon);
        if((expon > 16384) || !(fMant < 1)) {     /* Infinity or NaN */
            expon = sign | 0x7FFF;
            hiMant = 0;
            loMant = 0; /* infinity */
        } else {  /* Finite */
            expon += 16382;
            if(expon < 0) {     /* denormalized */
                fMant = ldexp(fMant, expon);
                expon = 0;
            }
            expon |= sign;
            fMant = ldexp(fMant, 32);
            fsMant = floor(fMant);
            hiMant = FloatToUnsigned(fsMant);
            fMant = ldexp(fMant - fsMant, 32);
            fsMant = floor(fMant);
            loMant = FloatToUnsigned(fsMant);
        }
    }
    bytes[0] = expon >> 8;
    bytes[1] = expon;
    bytes[2] = hiMant >> 24;
    bytes[3] = hiMant >> 16;
    bytes[4] = hiMant >> 8;
    bytes[5] = hiMant;
    bytes[6] = loMant >> 24;
    bytes[7] = loMant >> 16;
    bytes[8] = loMant >> 8;
    bytes[9] = loMant;
}

void ConvLong(ULONG *LongPtr)
{
    UBYTE z;													/* Zwischenspeicher */
    UBYTE *BytePtr;										/* Pointer auf byte */
    BytePtr = (UBYTE *) LongPtr;			/* Typenumwandlung! */
    z = BytePtr[0];
    BytePtr[0] = BytePtr[3];
    BytePtr[3] = z;
    z = BytePtr[1];
    BytePtr[1] = BytePtr[2];
    BytePtr[2] = z;
}

void ConvWord(UWORD *WordPtr)
{
    UBYTE z;
    UBYTE *BytePtr;
    BytePtr = (UBYTE *) WordPtr;
    z = BytePtr[0];
    BytePtr[0] = BytePtr[1];
    BytePtr[1] = z;
}
