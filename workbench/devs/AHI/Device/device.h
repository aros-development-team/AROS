/*
     AHI - Hardware independent audio subsystem
     Copyright (C) 1996-2005 Martin Blom <martin@blom.org>
     
     This library is free software; you can redistribute it and/or
     modify it under the terms of the GNU Library General Public
     License as published by the Free Software Foundation; either
     version 2 of the License, or (at your option) any later version.
     
     This library is distributed in the hope that it will be useful,
     but WITHOUT ANY WARRANTY; without even the implied warranty of
     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
     Library General Public License for more details.
     
     You should have received a copy of the GNU Library General Public
     License along with this library; if not, write to the
     Free Software Foundation, Inc., 59 Temple Place - Suite 330, Cambridge,
     MA 02139, USA.
*/

#ifndef ahi_device_h
#define ahi_device_h	

#include <exec/types.h>
#include <dos/dos.h>

struct AHIRequest;
struct AHIBase;

/*** New Style Device definitions ***/

#define NSCMD_DEVICEQUERY   0x4000

#define NSDEVTYPE_UNKNOWN       0   /* No suitable category, anything */
#define NSDEVTYPE_GAMEPORT      1   /* like gameport.device */
#define NSDEVTYPE_TIMER         2   /* like timer.device */
#define NSDEVTYPE_KEYBOARD      3   /* like keyboard.device */
#define NSDEVTYPE_INPUT         4   /* like input.device */
#define NSDEVTYPE_TRACKDISK     5   /* like trackdisk.device */
#define NSDEVTYPE_CONSOLE       6   /* like console.device */
#define NSDEVTYPE_SANA2         7   /* A >=SANA2R2 networking device */
#define NSDEVTYPE_AUDIOARD      8   /* like audio.device */
#define NSDEVTYPE_CLIPBOARD     9   /* like clipboard.device */
#define NSDEVTYPE_PRINTER       10  /* like printer.device */
#define NSDEVTYPE_SERIAL        11  /* like serial.device */
#define NSDEVTYPE_PARALLEL      12  /* like parallel.device */

struct NSDeviceQueryResult
{
  /*
  ** Standard information
  */
  ULONG   DevQueryFormat;         /* this is type 0               */
  ULONG   SizeAvailable;          /* bytes available              */

  /*
  ** Common information (READ ONLY!)
  */
  UWORD   DeviceType;             /* what the device does         */
  UWORD   DeviceSubType;          /* depends on the main type     */
  UWORD   *SupportedCommands;     /* 0 terminated list of cmd's   */

  /* May be extended in the future! Check SizeAvailable! */
};

#define DRIVE_NEWSTYLE  (0x4E535459L)   /* 'NSTY' */
#define NSCMD_TD_READ64     0xc000
#define NSCMD_TD_WRITE64    0xc001
#define NSCMD_TD_SEEK64     0xc002
#define NSCMD_TD_FORMAT64   0xc003


/*** My own stuff ***/

#define AHI_PRI         50      /* Priority for the device process */

#define PLAYERFREQ      100     /* How often the PlayerFunc is called */

#define AHICMD_END      CMD_NONSTD

#define AHICMD_WRITTEN  (0x8000 | CMD_WRITE)

#define ahir_Extras     ahir_Private[0]
#define GetExtras(req)  ((struct Extras *) req->ahir_Private[0])
#define NOCHANNEL       65535

struct Extras
{
        UWORD   Channel;
        UWORD   Sound;
        Fixed   VolumeScale;
};

/* Voice->Flags definitions */

/* Set by the interrupt when a new sound has been started */
#define VB_STARTED              0
#define VF_STARTED              (1<<0)

struct Voice
{
        UWORD                    NextSound;
        UBYTE                    Flags;
        UBYTE                    Pad;
        Fixed                    NextVolume;
        Fixed                    NextPan;
        ULONG                    NextFrequency;
        ULONG                    NextOffset;
        ULONG                    NextLength;
        struct AHIRequest       *NextRequest;
        
        struct AHIRequest       *QueuedRequest;
        struct AHIRequest       *PlayingRequest;
};

/* Special Offset values */

#define FREE    -1      /* Channel is not playing anything */
#define MUTE    -2      /* Channel will be muted when current sound is finished */
#define PLAY    -3      /* Channel will play more when current sound is finished */

#define MAXSOUNDS       128
#define SOUND_FREE      0
#define SOUND_IN_USE    1

struct AHIDevUnit
{
        struct Unit              Unit;
        UBYTE                    UnitNum;
        BYTE                     PlaySignal;
        BYTE                     RecordSignal;
        BYTE                     SampleSignal;
        struct Process          *Process;
        BYTE                     SyncSignal;
        struct Process          *Master;
        struct Hook              PlayerHook;
        struct Hook              RecordHook;
        struct Hook              SoundHook;
        struct Hook              ChannelInfoHook;
        
        struct AHIEffChannelInfo *ChannelInfoStruct;

        WORD                     RecordOffDelay;
        WORD                    *RecordBuffer;
        ULONG                    RecordSize;

        BOOL                     IsPlaying;     // Currently playing (or want to)
        BOOL                     IsRecording;   // Currently recording
        BOOL                     ValidRecord;   // The record buffer contains valid data
        BOOL                     FullDuplex;    // Mode is full duplex
        BOOL                     PseudoStereo;  // Mode is Paula-like stereo
        UWORD                    StopCnt;       // CMD_STOP count

	/* Lock is used to serialize access to StopCnt, ReadList, PlayingList,
	   SilentList, WaitingList and RequestQueue */

        struct SignalSemaphore   Lock;

        struct MinList           ReadList;
        struct MinList           PlayingList;
        struct MinList           SilentList;
        struct MinList           WaitingList;

        struct MinList           RequestQueue;

        struct Voice            *Voices;

        struct AHIAudioCtrl     *AudioCtrl;
        ULONG                    AudioMode;
        ULONG                    Frequency;
        UWORD                    Channels;
        UWORD                    Pad;
        Fixed                    MonitorVolume;
        Fixed                    InputGain;
        Fixed                    OutputVolume;
        ULONG                    Input;
        ULONG                    Output;

        UBYTE                    Sounds[MAXSOUNDS];
};

ULONG
_DevOpen ( struct AHIRequest* ioreq,
	   ULONG              unit,
	   ULONG              flags,
	   struct AHIBase*    AHIBase );

BPTR
_DevClose ( struct AHIRequest* ioreq,
	    struct AHIBase*    AHIBase );

BOOL
ReadConfig ( struct AHIDevUnit *iounit,
             struct AHIBase *AHIBase );

BOOL
AllocHardware ( struct AHIDevUnit *iounit,
                struct AHIBase *AHIBase );

void
FreeHardware ( struct AHIDevUnit *iounit,
               struct AHIBase *AHIBase );

void
DevProc( void );

#endif /* ahi_device_h */
