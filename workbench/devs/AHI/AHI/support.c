/*
     AHI - The AHI preferences program
     Copyright (C) 1996-2005 Martin Blom <martin@blom.org>
     
     This program is free software; you can redistribute it and/or
     modify it under the terms of the GNU General Public License
     as published by the Free Software Foundation; either version 2
     of the License, or (at your option) any later version.
     
     This program is distributed in the hope that it will be useful,
     but WITHOUT ANY WARRANTY; without even the implied warranty of
     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
     GNU General Public License for more details.
     
     You should have received a copy of the GNU General Public License
     along with this program; if not, write to the Free Software
     Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

#include <config.h>

#include <devices/ahi.h>
#include <exec/memory.h>
#include <intuition/intuition.h>
#include <prefs/prefhdr.h>
#include <workbench/workbench.h>

#include <clib/alib_protos.h>
#include <proto/ahi.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/gadtools.h>
#include <proto/icon.h>
#include <proto/iffparse.h>
#include <proto/utility.h>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>

#include "ahi.h"
#include "ahiprefs_Cat.h"
#include "support.h"

static BOOL AddUnit(struct List *, int);
static void FillUnitName(struct UnitNode *);

struct AHIGlobalPrefs     globalprefs;

#ifdef __AMIGAOS4__
struct Library           *GadToolsBase= NULL;
struct GadToolsIFace     *IGadTools   = NULL;
struct LocaleIFace       *ILocale     = NULL;
struct AHIIFace          *IAHI        = NULL;
#endif

struct Library           *LocaleBase= NULL;
struct Library           *AHIBase   = NULL;
static struct MsgPort    *AHImp     = NULL;
static struct AHIRequest *AHIio     = NULL;
static BYTE               AHIDevice = -1;


static char deftoolname[] = {"AHI"};

static UWORD DiskI1Data[] =
{
/* Plane */
  0x0000,0x0000,0x0004,0x0000,0x0000,0x0000,0x0001,0x0000,
  0x0000,0x07ff,0x8000,0x4000,0x0000,0x1800,0x6000,0x1000,
  0x0000,0x20fc,0x1000,0x0800,0x0000,0x4102,0x0800,0x0c00,
  0x0000,0x4082,0x0800,0x0c00,0x0000,0x4082,0x0800,0x0c00,
  0x0000,0x2104,0x0800,0x0c00,0x0000,0x1e18,0x1000,0x0c00,
  0x0000,0x0060,0x2000,0x0c00,0x0000,0x0080,0xc000,0x0c00,
  0x0000,0x0103,0x0000,0x0c00,0x0000,0x021c,0x0000,0x0c00,
  0x0000,0x0108,0x0000,0x0c00,0x0000,0x00f0,0x0000,0x0c00,
  0x0000,0x0108,0x0000,0x0c00,0x0000,0x0108,0x0000,0x0c00,
  0x4000,0x00f0,0x0000,0x0c00,0x1000,0x0000,0x0000,0x0c00,
  0x0400,0x0000,0x0000,0x0c00,0x01ff,0xffff,0xffff,0xfc00,
/* Plane */
  0xffff,0xffff,0xfff8,0x0000,0xd555,0x5555,0x5556,0x0000,
  0xd555,0x5000,0x5555,0x8000,0xd555,0x47ff,0x9555,0x6000,
  0xd555,0x5f03,0xe555,0x5000,0xd555,0x3e55,0xf555,0x5000,
  0xd555,0x3f55,0xf555,0x5000,0xd555,0x3f55,0xf555,0x5000,
  0xd555,0x5e53,0xf555,0x5000,0xd555,0x4147,0xe555,0x5000,
  0xd555,0x551f,0xd555,0x5000,0xd555,0x557f,0x1555,0x5000,
  0xd555,0x54fc,0x5555,0x5000,0xd555,0x55e1,0x5555,0x5000,
  0xd555,0x54f5,0x5555,0x5000,0xd555,0x5505,0x5555,0x5000,
  0xd555,0x54f5,0x5555,0x5000,0xd555,0x54f5,0x5555,0x5000,
  0x3555,0x5505,0x5555,0x5000,0x0d55,0x5555,0x5555,0x5000,
  0x0355,0x5555,0x5555,0x5000,0x0000,0x0000,0x0000,0x0000

};

static struct Image image1 =
{
    0,0,
    54,22,2,
    DiskI1Data,
    0x0004,0x0000,
    NULL
};

static STRPTR toolTypes[] = {
  "ACTION=USE",
  NULL
};

static struct DiskObject projIcon = {
  WB_DISKMAGIC,                   /* Magic Number */
  WB_DISKVERSION,                 /* Version */
  {                               /* Embedded Gadget Structure */
    NULL,                         /* Next Gadget Pointer */
    97,12,52,23,                  /* Left,Top,Width,Height */
    GFLG_GADGIMAGE|GFLG_GADGHBOX, /* Flags */
    GACT_IMMEDIATE|GACT_RELVERIFY,/* Activation Flags */
    GTYP_BOOLGADGET,              /* Gadget Type */
    (APTR)&image1,                /* Render Image */
    NULL,                         /* Select Image */
    NULL,                         /* Gadget Text */
    0,                            /* Mutual Exclude */
    NULL,                         /* Special Info */
    0,                            /* Gadget ID */
    NULL                          /* User Data */
  },  
  WBPROJECT,                      /* Icon Type */
  deftoolname,                    /* Default Tool */
  toolTypes,                      /* Tool Type Array */
  NO_ICON_POSITION,               /* Current X */
  NO_ICON_POSITION,               /* Current Y */
  NULL,                           /* Drawer Structure */
  NULL,                           /* Tool Window */
  4096                            /* Stack Size */
};


/******************************************************************************
**** Endian support code. *****************************************************
******************************************************************************/

/* See the header file for macros */

#if !defined( WORDS_BIGENDIAN )

static UWORD
EndianSwapUWORD( UWORD x ) {
  return ((((x) >> 8) & 0x00ffU) |
	  (((x) << 8) & 0xff00U) );
}

static ULONG
EndianSwapULONG( ULONG x ) {
  return ((((x) >> 24) & 0x000000ffUL) |
	  (((x) >> 8)  & 0x0000ff00UL) |
	  (((x) << 8)  & 0x00ff0000UL) |
	  (((x) << 24) & 0xff000000UL) );
}

void
EndianSwap( size_t size, void* data) {
  switch( size ) {
    case 1:
      break;

    case 2:
      *((UWORD*) data) = EndianSwapUWORD( *((UWORD*) data) );
      break;

    case 4:
      *((ULONG*) data) = EndianSwapULONG( *((ULONG*) data) );
      break;

    case 8: {
      ULONG tmp;

      tmp = EndianSwapULONG( *((ULONG*) data) );
      *((ULONG*) data) = EndianSwapULONG( *((ULONG*) (data + 4)) );
      *((ULONG*) (data + 4)) = tmp;
      break;
    }
      
    default:
      Printf( "Unknown size: %ld\n", size );
      abort();
  }
}

#else
# define EndianSwap(s, x)
#endif

#define CopyIfValid( s, f, from, to, size )		\
  do {							\
    if( (size_t) size > offsetof( s, f ) ) {		\
      (to).f = (from).f;				\
      EndianSwap( sizeof (to).f, &((to).f) );		\
    }							\
  } while( 0 )


/******************************************************************************
**** Call to open ahi.device etc. *********************************************
******************************************************************************/

BOOL Initialize(void) {
  LocaleBase = OpenLibrary("locale.library", 38);

#ifdef __AMIGAOS4__
  ILocale = (struct LocaleIFace *) GetInterface(LocaleBase, "main", 1, NULL);

  GadToolsBase = OpenLibrary("gadtools.library", 37);

  if( GadToolsBase == NULL) {
    Printf((char *) msgTextNoOpen, (ULONG) "gadtools.library", 37);
    Printf("\n");
    return FALSE;
  }
  
  IGadTools = (struct GadToolsIFace *) GetInterface(GadToolsBase, "main", 1, NULL);
#endif
  
  OpenahiprefsCatalog();

  AHImp=CreateMsgPort();

  if( AHImp != NULL ) {
    AHIio = (struct AHIRequest *)CreateIORequest(
        AHImp,sizeof(struct AHIRequest));

    if( AHIio != NULL ) {
      AHIio->ahir_Version = 4;
      AHIDevice = OpenDevice(AHINAME,AHI_NO_UNIT,(struct IORequest *)AHIio,0);
      if(AHIDevice == 0) {
        AHIBase   = (struct Library *)AHIio->ahir_Std.io_Device;
#ifdef __AMIGAOS4__
	IAHI = (struct AHIIFace *) GetInterface(AHIBase, "main", 1, NULL);
#endif
        return TRUE;
      }
    }
  }

  Printf((char *) msgTextNoOpen, (ULONG) "ahi.device", 4);
  Printf("\n");
  return FALSE;
}

/******************************************************************************
**** Call to close ahi.device etc. ********************************************
******************************************************************************/

void CleanUp(void) {
#ifdef __AMIGAOS4__
  DropInterface((struct Interface*) IAHI);
  DropInterface((struct Interface*) IGadTools);
  CloseLibrary(GadToolsBase);
#endif

  if(!AHIDevice)
    CloseDevice((struct IORequest *)AHIio);
  DeleteIORequest((struct IORequest *)AHIio);
  DeleteMsgPort(AHImp);

  AHIBase = NULL; AHImp = NULL; AHIio = NULL; AHIDevice = -1;

  CloseahiprefsCatalog();

  CloseLibrary(LocaleBase);
  LocaleBase = NULL;
}


/***** Local function to create a Unit structure  ****************************/

static BOOL AddUnit(struct List *list, int unit) {
  struct UnitNode *u;

  u = AllocVec(sizeof(struct UnitNode), MEMF_CLEAR);

  if(u == NULL) {
    return FALSE;
  }

  u->prefs.ahiup_Unit           = unit;
  u->prefs.ahiup_Channels       = 1;
  u->prefs.ahiup_AudioMode      = AHI_BestAudioID(AHIDB_Realtime, TRUE, TAG_DONE);
  u->prefs.ahiup_Frequency      = 8000;
  u->prefs.ahiup_MonitorVolume  = 0x00000;
  u->prefs.ahiup_InputGain      = 0x10000;
  u->prefs.ahiup_OutputVolume   = 0x10000;
  u->prefs.ahiup_Input          = 0;
  u->prefs.ahiup_Output         = 0;

  FillUnitName(u);

  u->node.ln_Pri = -unit;
  Enqueue(list, (struct Node *) u);

  return TRUE;
}

/***** Local function to create a Unit name **********************************/

static void FillUnitName(struct UnitNode *u) {
  if(u->prefs.ahiup_Unit != AHI_NO_UNIT) {
    sprintf((char *) &u->name, msgUnitDevice, u->prefs.ahiup_Unit);
  }
  else {
    sprintf((char *) &u->name, msgUnitMusic);
  }
  u->node.ln_Name = (char *) &u->name;
}

/******************************************************************************
**** Returns a list with all avalable units, fills globalprefs ****************
******************************************************************************/

#define UNITNODES 4

// Call with name == NULL to get defaults

struct List *GetUnits(char *name) {
  struct List *list;
  struct IFFHandle *iff;
  BOOL devnodes[UNITNODES] = { FALSE, FALSE, FALSE, FALSE } ;
  BOOL lownode = FALSE;
  int i;

  // Reasonable defaults
  globalprefs.ahigp_DebugLevel       = AHI_DEBUG_NONE;
  globalprefs.ahigp_DisableSurround  = FALSE;
  globalprefs.ahigp_DisableEcho      = FALSE;
  globalprefs.ahigp_FastEcho         = FALSE;
  globalprefs.ahigp_MaxCPU           = (90 << 16) / 100;
  globalprefs.ahigp_ClipMasterVolume = FALSE;
  globalprefs.ahigp_Pad              = 0;
  globalprefs.ahigp_AntiClickTime    = 0;
  globalprefs.ahigp_ScaleMode        = AHI_SCALE_FIXED_0_DB;

  list = AllocVec(sizeof(struct List), MEMF_CLEAR);
  
  if(list) {
    NewList(list);
    
    if(name && (iff = AllocIFF())) {
      iff->iff_Stream = (ULONG) Open(name, MODE_OLDFILE);
      if(iff->iff_Stream) {
        InitIFFasDOS(iff);
        if(!OpenIFF(iff, IFFF_READ)) {
          if(!(PropChunk      (iff, ID_PREF, ID_AHIG) ||
               CollectionChunk(iff, ID_PREF, ID_AHIU) ||
               StopOnExit     (iff, ID_PREF, ID_FORM))) {
            if(ParseIFF(iff, IFFPARSE_SCAN) == IFFERR_EOC) {
              struct StoredProperty *global = FindProp(iff, ID_PREF, ID_AHIG);
              struct CollectionItem *ci = FindCollection(iff, ID_PREF, ID_AHIU);
	      int ci_cnt = 0;

              if(global != NULL) {
		struct AHIGlobalPrefs *p = global->sp_Data;

		CopyIfValid( struct AHIGlobalPrefs, ahigp_DebugLevel,
			     *p, globalprefs, global->sp_Size );
		CopyIfValid( struct AHIGlobalPrefs, ahigp_DisableSurround,
			     *p, globalprefs, global->sp_Size );
		CopyIfValid( struct AHIGlobalPrefs, ahigp_DisableEcho,
			     *p, globalprefs, global->sp_Size );
		CopyIfValid( struct AHIGlobalPrefs, ahigp_FastEcho,
			     *p, globalprefs, global->sp_Size );
		CopyIfValid( struct AHIGlobalPrefs, ahigp_MaxCPU,
			     *p, globalprefs, global->sp_Size );
		CopyIfValid( struct AHIGlobalPrefs, ahigp_ClipMasterVolume, 
			     *p, globalprefs, global->sp_Size );
		CopyIfValid( struct AHIGlobalPrefs, ahigp_Pad,
			     *p, globalprefs, global->sp_Size );
		CopyIfValid( struct AHIGlobalPrefs, ahigp_AntiClickTime,
			     *p, globalprefs, global->sp_Size );
		CopyIfValid( struct AHIGlobalPrefs, ahigp_ScaleMode,
			     *p, globalprefs, global->sp_Size );


		/* Set upsupported options to their defaults */

		if( AHIBase->lib_Version <= 4 )
		{
		  globalprefs.ahigp_AntiClickTime = 0;
		  globalprefs.ahigp_ScaleMode     = AHI_SCALE_FIXED_SAFE;
		}
		
		if( AHIBase->lib_Version >= 5 )
		{
		  globalprefs.ahigp_DisableSurround  = FALSE;
		  globalprefs.ahigp_DisableEcho      = FALSE;
		  globalprefs.ahigp_FastEcho         = FALSE;
		  globalprefs.ahigp_ClipMasterVolume = TRUE;
		}
              }

              while(ci) {
                struct AHIUnitPrefs *p = ci->ci_Data;
                struct UnitNode     *u;

                u = AllocVec(sizeof(struct UnitNode), MEMF_CLEAR);
                if(u == NULL)
                  break;

		u->prefs.ahiup_Unit          = ci_cnt;
		u->prefs.ahiup_Channels      = 1;
		u->prefs.ahiup_AudioMode     = AHI_DEFAULT_ID;
		u->prefs.ahiup_Frequency     = AHI_DEFAULT_FREQ;
		u->prefs.ahiup_MonitorVolume = 0;
		u->prefs.ahiup_InputGain     = 0;
		u->prefs.ahiup_OutputVolume  = 0x10000;
		u->prefs.ahiup_Input         = 0;
		u->prefs.ahiup_Output        = 0;

		++ci_cnt;
		
		CopyIfValid( struct AHIUnitPrefs, ahiup_Unit,
			     *p, u->prefs, ci->ci_Size );
		CopyIfValid( struct AHIUnitPrefs, ahiup_Channels,
			     *p, u->prefs, ci->ci_Size );
		CopyIfValid( struct AHIUnitPrefs, ahiup_AudioMode,
			     *p, u->prefs, ci->ci_Size );
		CopyIfValid( struct AHIUnitPrefs, ahiup_Frequency,
			     *p, u->prefs, ci->ci_Size );
		CopyIfValid( struct AHIUnitPrefs, ahiup_MonitorVolume,
			     *p, u->prefs, ci->ci_Size );
		CopyIfValid( struct AHIUnitPrefs, ahiup_InputGain,
			     *p, u->prefs, ci->ci_Size );
		CopyIfValid( struct AHIUnitPrefs, ahiup_OutputVolume,
			     *p, u->prefs, ci->ci_Size );
		CopyIfValid( struct AHIUnitPrefs, ahiup_Input,
			     *p, u->prefs, ci->ci_Size );
		CopyIfValid( struct AHIUnitPrefs, ahiup_Output,
			     *p, u->prefs, ci->ci_Size );
		
                FillUnitName(u);
                
                u->node.ln_Pri = -(u->prefs.ahiup_Unit);
                Enqueue(list, (struct Node *) u);
                
                if(u->prefs.ahiup_Unit == AHI_NO_UNIT) {
                  lownode = TRUE;
                }
                else if(u->prefs.ahiup_Unit < UNITNODES) {
                  devnodes[u->prefs.ahiup_Unit] = TRUE;
                }
                
                ci=ci->ci_Next;
              }
            }
          }
          CloseIFF(iff);
        }
        Close((BPTR) iff->iff_Stream);
      }
      FreeIFF(iff);
    }


    // Fill up to lowlevel + UNITNODES device nodes, if not found in prefs file

    if(!lownode) AddUnit(list, AHI_NO_UNIT);
    for(i = 0; i < UNITNODES; i++) {
      if(!devnodes[i]) AddUnit(list, i);
    }

  }

  return list;
}

/******************************************************************************
**** Returns a list with all available modes **********************************
******************************************************************************/

struct List *GetModes(struct AHIUnitPrefs *prefs) {
  struct List *list;

  list = AllocVec(sizeof(struct List), MEMF_CLEAR);
  
  if(list) {
    ULONG id = AHI_NextAudioID(AHI_INVALID_ID);

    NewList(list);

    while(id != AHI_INVALID_ID) {
      struct ModeNode *t;
      struct Node     *node;
      
      t = AllocVec( sizeof(struct ModeNode), MEMF_CLEAR);

      if( t != NULL ) {
        LONG realtime;

        t->node.ln_Name = t->name;
        t->ID = id;
        
        realtime = FALSE;
        
        AHI_GetAudioAttrs(id, NULL,
            AHIDB_BufferLen,  80,
            AHIDB_Name,       (ULONG) t->node.ln_Name,
            AHIDB_Realtime,   (ULONG) &realtime,
            TAG_DONE);

        if((prefs->ahiup_Unit == AHI_NO_UNIT) ||
	   (realtime && (id & 0x00ff0000) != 0x00030000 /* Argh!! */)) {
          // Insert node alphabetically
          for(node = list->lh_Head;
              node->ln_Succ;
              node = node->ln_Succ) {
            if(Stricmp(t->node.ln_Name,node->ln_Name) < 0)
              break;
          }
          Insert(list, (struct Node *) t, node->ln_Pred);
        }
        else {
          FreeVec(t);
        }
      }
      id = AHI_NextAudioID(id);
    }
  }
  return list;
}

/******************************************************************************
**** Creates a char* array from a list ****************************************
******************************************************************************/

char **List2Array(struct List *list) {
  char **strings, **rstrings;
  int i;
  struct Node *n;

  for(n = list->lh_Head, i = 0; n->ln_Succ; n = n->ln_Succ) {
    i++;
  }

  strings = AllocVec(sizeof(char *) * (i + 1), MEMF_CLEAR);
  rstrings = strings;

  if(strings) {
    for(n = list->lh_Head; n->ln_Succ; n = n->ln_Succ) {
      *strings++ = n->ln_Name;
    }
    *strings = NULL;
  }
  return rstrings;
}

/******************************************************************************
**** Returns a char* array with inputs names **********************************
******************************************************************************/

char **GetInputs(ULONG id) {
  char **strings, **rstrings;
  LONG inputs = 0, i;


  AHI_GetAudioAttrs(id, NULL,
      AHIDB_Inputs, (ULONG) &inputs,
      TAG_DONE);

  strings = AllocVec(sizeof(char *) * (inputs + 1) + (32 * inputs), MEMF_CLEAR);
  rstrings = strings;

  if(strings) {
    char *string = (char *) &strings[inputs + 1];

    for(i = 0; i < inputs; i++) {
      if(AHI_GetAudioAttrs(id, NULL,
          AHIDB_BufferLen, 32,
          AHIDB_InputArg,  i,
          AHIDB_Input,     (ULONG) string,
          TAG_DONE)) {
        *strings++ = string;
        while(*string++);
      }
    }
    *strings = NULL;
  }
  return rstrings;
}

/******************************************************************************
**** Returns a char* array with outputs names *********************************
******************************************************************************/

char **GetOutputs(ULONG id) {
  char **strings, **rstrings;
  LONG outputs = 0, i;


  AHI_GetAudioAttrs(id, NULL,
      AHIDB_Outputs, (ULONG) &outputs,
      TAG_DONE);

  strings = AllocVec(sizeof(char *) * (outputs + 1) + (32 * outputs), MEMF_CLEAR);
  rstrings = strings;
  
  if(strings) {
    char *string = (char *) &strings[outputs + 1];

    for(i = 0; i < outputs; i++) {
      if(AHI_GetAudioAttrs(id, NULL,
          AHIDB_BufferLen, 32,
          AHIDB_OutputArg,  i,
          AHIDB_Output,     (ULONG) string,
          TAG_DONE)) {
        *strings++ = string;
        while(*string++);
      }
    }
    *strings = NULL;
  }
  return rstrings;
}

/******************************************************************************
**** Saves all units and globalprefs ******************************************
******************************************************************************/

BOOL SaveSettings(char *name, struct List *list) {
  struct IFFHandle  *iff;
  struct PrefHeader  header = { 0, 0, 0 };
  struct Node       *n;
  BOOL               success = FALSE;

  if(name && (iff = AllocIFF())) {
    iff->iff_Stream = (ULONG) Open(name, MODE_NEWFILE);
    if(iff->iff_Stream) {
      InitIFFasDOS(iff);
      if(!OpenIFF(iff, IFFF_WRITE)) {
        if(! PushChunk(iff, ID_PREF, ID_FORM, IFFSIZE_UNKNOWN)) {

          success = TRUE;

          // Prefs header
          if(! PushChunk(iff, ID_PREF, ID_PRHD, sizeof header)) {
	    EndianSwap( sizeof (UBYTE), &header.ph_Version );
	    EndianSwap( sizeof (UBYTE), &header.ph_Type );
	    EndianSwap( sizeof (ULONG), &header.ph_Flags );
            WriteChunkBytes(iff, &header, sizeof header);
            PopChunk(iff);
          }
          else success = FALSE;

          // Global prefs
          if(! PushChunk(iff, ID_PREF, ID_AHIG, sizeof globalprefs)) {
	    struct AHIGlobalPrefs p;
	    
	    CopyIfValid( struct AHIGlobalPrefs, ahigp_DebugLevel,
			 globalprefs, p, sizeof p );
	    CopyIfValid( struct AHIGlobalPrefs, ahigp_DisableSurround,
			 globalprefs, p, sizeof p );
	    CopyIfValid( struct AHIGlobalPrefs, ahigp_DisableEcho,
			 globalprefs, p, sizeof p );
	    CopyIfValid( struct AHIGlobalPrefs, ahigp_FastEcho,
			 globalprefs, p, sizeof p );
	    CopyIfValid( struct AHIGlobalPrefs, ahigp_MaxCPU,
			 globalprefs, p, sizeof p );
	    CopyIfValid( struct AHIGlobalPrefs, ahigp_ClipMasterVolume, 
			 globalprefs, p, sizeof p );
	    CopyIfValid( struct AHIGlobalPrefs, ahigp_Pad,
			 globalprefs, p, sizeof p );
	    CopyIfValid( struct AHIGlobalPrefs, ahigp_AntiClickTime,
			 globalprefs, p, sizeof p );
	    CopyIfValid( struct AHIGlobalPrefs, ahigp_ScaleMode,
			 globalprefs, p, sizeof p );

		
            WriteChunkBytes(iff, &p, sizeof p);
            PopChunk(iff);
          }
          else success = FALSE;

          // Units
          if(list != NULL) {    
            for(n = list->lh_Head; n->ln_Succ; n = n->ln_Succ) {
              if(! PushChunk(iff, ID_PREF, ID_AHIU, sizeof(struct AHIUnitPrefs))) {
		struct AHIUnitPrefs p;

		CopyIfValid( struct AHIUnitPrefs, ahiup_Unit,
			     ((struct UnitNode *) n)->prefs, p, sizeof p );
		CopyIfValid( struct AHIUnitPrefs, ahiup_Channels,
			     ((struct UnitNode *) n)->prefs, p, sizeof p );
		CopyIfValid( struct AHIUnitPrefs, ahiup_AudioMode,
			     ((struct UnitNode *) n)->prefs, p, sizeof p );
		CopyIfValid( struct AHIUnitPrefs, ahiup_Frequency,
			     ((struct UnitNode *) n)->prefs, p, sizeof p );
		CopyIfValid( struct AHIUnitPrefs, ahiup_MonitorVolume,
			     ((struct UnitNode *) n)->prefs, p, sizeof p );
		CopyIfValid( struct AHIUnitPrefs, ahiup_InputGain,
			     ((struct UnitNode *) n)->prefs, p, sizeof p );
		CopyIfValid( struct AHIUnitPrefs, ahiup_OutputVolume,
			     ((struct UnitNode *) n)->prefs, p, sizeof p );
		CopyIfValid( struct AHIUnitPrefs, ahiup_Input,
			     ((struct UnitNode *) n)->prefs, p, sizeof p );
		CopyIfValid( struct AHIUnitPrefs, ahiup_Output,
			     ((struct UnitNode *) n)->prefs, p, sizeof p );
		
                WriteChunkBytes(iff, &p, sizeof p);
                PopChunk(iff);
              }
              else success = FALSE;
            }
          }
        }
        CloseIFF(iff);
      }
      Close((BPTR) iff->iff_Stream);
    }
    FreeIFF(iff);
  }
  return success;
}


/******************************************************************************
**** Write a project icon to disk *********************************************
******************************************************************************/

BOOL WriteIcon(char *name) {
  struct DiskObject *dobj;
  char *olddeftool;
  STRPTR* oldtooltypes;
  BOOL success = FALSE;

  /* Use the already present icon */
  
  dobj=GetDiskObject(name);
  
  if( dobj != NULL ) {
    oldtooltypes = dobj->do_ToolTypes;
    olddeftool = dobj->do_DefaultTool;

    dobj->do_ToolTypes = toolTypes;
    dobj->do_DefaultTool = deftoolname;

    success = PutDiskObject(name,dobj);

    /* we must restore the original pointers before freeing */
    dobj->do_ToolTypes = oldtooltypes;
    dobj->do_DefaultTool = olddeftool;
    FreeDiskObject(dobj);
  }

  /* Try the user's default prefs icon */
  if((! success) && (dobj=GetDiskObject("ENV:Sys/def_Pref"))) {
    oldtooltypes = dobj->do_ToolTypes;
    olddeftool = dobj->do_DefaultTool;

    dobj->do_ToolTypes = toolTypes;
    dobj->do_DefaultTool = deftoolname;

    success = PutDiskObject(name,dobj);

    /* we must restore the original pointers before freeing */
    dobj->do_ToolTypes = oldtooltypes;
    dobj->do_DefaultTool = olddeftool;
    FreeDiskObject(dobj);
  }

  /* Else, put our default icon */
  if(! success) {
    success = PutDiskObject(name,&projIcon);
  }

  return success;
}


/******************************************************************************
**** Frees a list and all nodes ***********************************************
******************************************************************************/

void FreeList(struct List *list) {
  struct Node *n;

  if(list == NULL)
    return;

  for( n = RemHead(list);
       n != NULL;
       n = RemHead(list) )
  {
    FreeVec(n);
  }

  FreeVec(list);
}

/******************************************************************************
**** Returns the index:th node in a list  *************************************
******************************************************************************/

struct Node *GetNode(int index, struct List *list) {
  struct Node *n;

  if(list == NULL || list->lh_Head->ln_Succ == NULL)
    return NULL;
    
  for(n = list->lh_Head; n->ln_Succ; n = n->ln_Succ) {
    if(index == 0) {
      break;
    }
    index--;
  }
  return n;
}

/******************************************************************************
**** Plays a test sound using the current selected mode and settings **********
******************************************************************************/

BOOL PlaySound( struct AHIUnitPrefs* prefs )
{
  struct AHIAudioCtrl* actrl;
  BOOL                 rc = FALSE;
  
  actrl = AHI_AllocAudio( AHIA_AudioID,  prefs->ahiup_AudioMode,
                          AHIA_MixFreq,  prefs->ahiup_Frequency,
                          AHIA_Channels, 1,
                          AHIA_Sounds,   1,
                          TAG_DONE );

  if( actrl != NULL )
  {
    WORD*                sample;
    int                  length = 48000/440;
    struct AHISampleInfo sound;
      
    sample = AllocVec( length * sizeof( *sample ), MEMF_PUBLIC );
      
    if( sample != NULL )
    {
      int i;
        
      for( i = 0; i < length; ++i )
      {
        sample[ i ] = 0x7fff * sin( i * 2 * M_PI / length );
      }
        
      sound.ahisi_Type    = AHIST_M16S;
      sound.ahisi_Address = sample;
      sound.ahisi_Length  = length;
        
      if( AHI_LoadSound( 0, AHIST_SAMPLE, &sound, actrl ) == AHIE_OK )
      {
        if( AHI_ControlAudio( actrl, AHIC_Play,          FALSE,
                                     AHIC_MonitorVolume, prefs->ahiup_MonitorVolume,
                                     AHIC_InputGain,     prefs->ahiup_InputGain,
                                     AHIC_OutputVolume,  prefs->ahiup_OutputVolume,
                                     AHIC_Input,         prefs->ahiup_Input,
                                     AHIC_Output,        prefs->ahiup_Output,
                                     TAG_DONE ) == AHIE_OK )
        {
	  ULONG volume = 0x10000;
	  
	  switch( globalprefs.ahigp_ScaleMode )
	  {
	    case AHI_SCALE_DYNAMIC_SAFE:
	      volume = 0x10000;
	      break;

	    case AHI_SCALE_FIXED_SAFE:
	      if( prefs->ahiup_Channels != 0 )
	      {
		volume = 0x10000 / prefs->ahiup_Channels;
	      }
	      break;

	    case AHI_SCALE_FIXED_0_DB:
	      volume = 0x10000;
	      break;
	
	    case AHI_SCALE_FIXED_3_DB:
	      volume = 0xB505;
	      break;

	    case AHI_SCALE_FIXED_6_DB:
	      volume = 0x8000;
	      break;
	  }
	  
          AHI_Play( actrl, AHIP_BeginChannel, 0,
                           AHIP_Freq,         48000,
                           AHIP_Vol,          volume,
                           AHIP_Pan,          0x8000,
                           AHIP_Sound,        0,
                           AHIP_EndChannel,   0,
                           TAG_DONE );

	  if( AHI_ControlAudio( actrl, AHIC_Play, TRUE, TAG_DONE ) == AHIE_OK )
	  {
	    if( globalprefs.ahigp_ScaleMode == AHI_SCALE_DYNAMIC_SAFE )
	    {
	      Delay( 10 );
	      AHI_SetVol( 0, volume / 2, 0x8000, actrl, AHISF_IMM );
	      Delay( 30 );
	      AHI_SetVol( 0, volume, 0x8000, actrl, AHISF_IMM );
	      Delay( 10 );
	    }
	    else
	    {
	      Delay( 50 );
	    }
          
	    AHI_Play( actrl, AHIP_BeginChannel, 0,
                             AHIP_Sound,        AHI_NOSOUND,
                             AHIP_EndChannel,   0,
                             TAG_DONE );

	    // Give the anti-click code a chance to do it's work ...
	    Delay( 1 );

	    AHI_ControlAudio( actrl, AHIC_Play, FALSE,
                                     TAG_DONE );
	  }
        }

        AHI_UnloadSound( 0, actrl );
      }
        
      FreeVec( sample );
    }
    
    AHI_FreeAudio( actrl );
  }

  return rc;
}
