
#include <config.h>

#include <devices/ahi.h>
#include <exec/memory.h>
#include <dos/dos.h>
#include <utility/hooks.h>

#include <clib/alib_protos.h>
#include <proto/exec.h>
#include <proto/ahi.h>

#include <stdio.h>
#include <math.h>
#include <limits.h>

#include "version.h"

static const char version[] = "$VER: PlaySineEverywhere " VERS "\n\r";

long __oslibversion = 37;

struct Library* AHIBase = NULL;

int
PlaySineEverywhere( void );

void
SoundFunc( struct Hook*            hook,
	   struct AHIAudioCtrl*    actrl,
	   struct AHISoundMessage* sm );


int
main( int argc, char* argv[] ) {
  int rc = RETURN_OK;

  if( argc != 1 ) {
    fprintf( stderr, "Usage: %s\n", argv[ 0 ] );
    rc = RETURN_ERROR;
  }
  else {
    struct MsgPort* mp = CreateMsgPort();
    
    if( mp != NULL ) {
      struct AHIRequest* io = (struct AHIRequest *)
	CreateIORequest( mp, sizeof( struct AHIRequest ) );

      if( io != NULL ) {
	// We use 32 bit samples, so we need version 6.
	io->ahir_Version = 6;

	if( OpenDevice( AHINAME, AHI_NO_UNIT, (struct IORequest *) io, 0 )
	    == 0 ) {
	  AHIBase = (struct Library *) io->ahir_Std.io_Device;

	  rc = PlaySineEverywhere();

	  CloseDevice( (struct IORequest *) io );
	}
	else {
	  fprintf( stderr, "Unable to open '" AHINAME "' version 6.\n" );
	  rc = RETURN_FAIL;
	}
	
	DeleteIORequest( (struct IORequest *) io );
      }
      else {
	fprintf( stderr, "Unable to create IO request.\n" );
	rc = RETURN_FAIL;
      }

      DeleteMsgPort( mp );
    }
    else {
      fprintf( stderr, "Unable to create message port.\n" );
      rc = RETURN_FAIL;
    }
  }
  
  return rc;
}

int
PlaySineEverywhere( void ) {
  int   rc = RETURN_OK;
  int   sine_length = 44100 / 100;
  BYTE* sine_m8s;
  BYTE* sine_s8s;
  WORD* sine_s16s;
  WORD* sine_m16s;
  LONG* sine_s32s;
  LONG* sine_m32s;

  sine_m8s  = AllocVec( 1 * sizeof( BYTE ) * sine_length, MEMF_ANY | MEMF_PUBLIC );
  sine_s8s  = AllocVec( 2 * sizeof( BYTE ) * sine_length, MEMF_ANY | MEMF_PUBLIC );
  sine_m16s = AllocVec( 1 * sizeof( WORD ) * sine_length, MEMF_ANY | MEMF_PUBLIC );
  sine_s16s = AllocVec( 2 * sizeof( WORD ) * sine_length, MEMF_ANY | MEMF_PUBLIC );
  sine_m32s = AllocVec( 1 * sizeof( LONG ) * sine_length, MEMF_ANY | MEMF_PUBLIC );
  sine_s32s = AllocVec( 2 * sizeof( LONG ) * sine_length, MEMF_ANY | MEMF_PUBLIC );
  
  if( sine_m8s != NULL &&
      sine_s8s != NULL &&
      sine_m16s != NULL &&
      sine_s16s != NULL &&
      sine_m32s != NULL &&
      sine_s32s != NULL ) {
    ULONG mode = AHI_INVALID_ID;
    int   i;

    for( i = 0; i < sine_length; ++i ) {
      double value = sin( i * 2 * M_PI / sine_length );

      sine_m8s[ i ] = (BYTE) ( SCHAR_MAX * value );
      sine_m16s[ i ] = (WORD) ( SHRT_MAX * value );
      sine_m32s[ i ] = (LONG) ( LONG_MAX * value );

      sine_s8s[ i * 2 + 0 ] = (BYTE) ( SCHAR_MAX * value );
      sine_s8s[ i * 2 + 1 ] = (BYTE) ( SCHAR_MAX * value );
      sine_s16s[ i * 2 + 0 ] = (WORD) ( SHRT_MAX * value );
      sine_s16s[ i * 2 + 1 ] = (WORD) ( SHRT_MAX * value );
      sine_s32s[ i * 2 + 0 ] = (LONG) ( LONG_MAX * value );
      sine_s32s[ i * 2 + 1 ] = (LONG) ( LONG_MAX * value );
    }

    while( rc == RETURN_OK &&
	   ( mode = AHI_NextAudioID( mode ) ) != AHI_INVALID_ID ) {
      struct AHIAudioCtrl* actrl;
      char                 name[ 64 ];
      struct Hook          sound_hook = {
	{ NULL, NULL },
	HookEntry,
	(HOOKFUNC) SoundFunc,
	FindTask( NULL )
      };
    
      AHI_GetAudioAttrs( mode, NULL,
			 AHIDB_Name, (ULONG) &name,
			 AHIDB_BufferLen, 64,
			 TAG_DONE );
    
      printf( "Mode 0x%08lx: %s\n", mode, name );

      actrl = AHI_AllocAudio( AHIA_AudioID,   mode,
			      AHIA_MixFreq,   44100,
			      AHIA_Channels,  1,
			      AHIA_Sounds,    6,
			      AHIA_SoundFunc, (ULONG) &sound_hook,
			      AHIA_UserData,  0,
			      TAG_DONE );

      if( actrl != NULL ) {
	struct AHISampleInfo sample_m8s  = { AHIST_M8S,  sine_m8s,  sine_length };
	struct AHISampleInfo sample_s8s  = { AHIST_S8S,  sine_s8s,  sine_length };
	struct AHISampleInfo sample_m16s = { AHIST_M16S, sine_m16s, sine_length };
	struct AHISampleInfo sample_s16s = { AHIST_S16S, sine_s16s, sine_length };
	struct AHISampleInfo sample_m32s = { AHIST_M32S, sine_m32s, sine_length };
	struct AHISampleInfo sample_s32s = { AHIST_S32S, sine_s32s, sine_length };
	
	if( AHI_LoadSound( 0, AHIST_SAMPLE, &sample_m8s,  actrl) == AHIE_OK &&
	    AHI_LoadSound( 1, AHIST_SAMPLE, &sample_s8s,  actrl) == AHIE_OK &&
	    AHI_LoadSound( 2, AHIST_SAMPLE, &sample_m16s, actrl) == AHIE_OK &&
	    AHI_LoadSound( 3, AHIST_SAMPLE, &sample_s16s, actrl) == AHIE_OK &&
	    AHI_LoadSound( 4, AHIST_SAMPLE, &sample_m32s, actrl) == AHIE_OK &&
	    AHI_LoadSound( 5, AHIST_SAMPLE, &sample_s32s, actrl) == AHIE_OK ) {

	  AHI_Play( actrl,
		    AHIP_BeginChannel, 0,
		    AHIP_Sound,        0,
		    AHIP_Freq,         44100,
		    AHIP_Vol,          0x10000,
		    AHIP_Pan,          0x00000,
		    AHIP_EndChannel,   0,
		    TAG_DONE );

	  // Now, when everything is "armed", lets start processing.

	  SetSignal( 0, SIGF_SINGLE );

	  if( AHI_ControlAudio( actrl,
				AHIC_Play, TRUE,
				TAG_DONE ) == AHIE_OK ) {
	    Wait( SIGF_SINGLE );

	    AHI_ControlAudio( actrl,
			      AHIC_Play, FALSE,
			      TAG_DONE );
	  }
	  else {
	    fprintf( stderr, "Unable start playback.\n" );
	    rc = RETURN_ERROR;
	  }

	  // AHI_FreeAudio() will unload the sounds
	}
	else {
	  fprintf( stderr, "Unable load sound.\n" );
	  rc = RETURN_ERROR;
	}
	
	AHI_FreeAudio( actrl );
      }
      else {
	fprintf( stderr, "Unable to allocate audio.\n" );
	rc = RETURN_ERROR;
      }
    }
  }
  else {
    fprintf( stderr, "Unable to allocate memory for sine\n" );
  }

  FreeVec( sine_m8s );
  FreeVec( sine_s8s );
  FreeVec( sine_s16s );
  FreeVec( sine_m16s );
  FreeVec( sine_s32s );
  FreeVec( sine_m32s );

  return rc;
}

void
SoundFunc( struct Hook*            hook,
	   struct AHIAudioCtrl*    actrl,
	   struct AHISoundMessage* sm ) {
  struct Task* task = hook->h_Data;
  ULONG        cnt;

  ++( (ULONG) actrl->ahiac_UserData );
  cnt = (ULONG) actrl->ahiac_UserData;
  
  if( cnt == 100 ) {
    AHI_SetSound( 0, AHI_NOSOUND, 0, 0, actrl, AHISF_NONE );
  }
  else if( cnt == 101 ) {
    Signal( task, SIGF_SINGLE );
  }
  else {
    UWORD sound = cnt % 6;

    AHI_SetSound( 0, sound, 0, 0, actrl, AHISF_NONE );

    if( ( sound == 0 ) ) {
      AHI_SetVol( 0, 0x10000, 0x10000 * cnt / 6 / ( 100 / 6 ), actrl, AHISF_NONE );
    }
  }
}

