
#include <config.h>

#include <devices/ahi.h>
#include <proto/ahi.h>
#include <proto/exec.h>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "version.h"

static const char version[] = "$VER: Test-7.1 " VERS "\n\r";

struct Library* AHIBase = NULL;

static int Test71( ULONG );

int
main( int argc, char* argv[] ) {
  int rc = RETURN_OK;

  if( argc != 2 ) {
    fprintf( stderr, "Usage: %s <audio mode id>\n", argv[ 0 ] );
    rc = RETURN_ERROR;
  }
  else {
    struct MsgPort* mp = CreateMsgPort();
    
    if( mp != NULL ) {
      struct AHIRequest* io = (struct AHIRequest *)
	CreateIORequest( mp, sizeof( struct AHIRequest ) );

      if( io != NULL ) {
	// We use 32 bit, 8 channel samples, so we need version 6.
	io->ahir_Version = 6;

	if( OpenDevice( AHINAME, AHI_NO_UNIT, (struct IORequest *) io, 0 )
	    == 0 ) {
	  AHIBase = (struct Library *) io->ahir_Std.io_Device;

	  rc = Test71( strtol( argv[ 1 ], NULL, 0 ) );

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

#define LENGTH 1000
//#define SINE

int
Test71( ULONG mode_id ) {
  char  name[ 64 ];
  int   rc = RETURN_OK;
  LONG* sample;

  sample = malloc( sizeof(LONG) * 8 * (LENGTH+1) );
  
  if( sample != NULL ) {
    struct AHIAudioCtrl* actrl;
    int   i;

    for( i = 0; i < (LENGTH+1); ++i ) {
      sample[ i * 8 + 0 ] = 0;
      sample[ i * 8 + 1 ] = 0;
      sample[ i * 8 + 2 ] = 0;
      sample[ i * 8 + 3 ] = 0;
      sample[ i * 8 + 4 ] = 0;
      sample[ i * 8 + 5 ] = 0;
      sample[ i * 8 + 6 ] = 0;
#ifdef SINE
      sample[ i * 8 + 7 ] = 0x7fffffff*sin(2*M_PI*i/LENGTH);
#else
      sample[ i * 8 + 7 ] = i < (LENGTH/2) ? 0x7fffffff : 0x80000000;
#endif
    }


    name[0] = 0;
    AHI_GetAudioAttrs( mode_id, NULL,
		       AHIDB_Name, (ULONG) &name,
		       AHIDB_BufferLen, 64,
		       TAG_DONE );
    
    printf( "Testing mode 0x%08lx (%s)\n", mode_id, name );
    
    actrl = AHI_AllocAudio( AHIA_AudioID,   mode_id,
			    AHIA_MixFreq,   44100,
			    AHIA_Channels,  1,
			    AHIA_Sounds,    8,
			    TAG_DONE );

    if( actrl != NULL ) {
      struct AHISampleInfo sample_fl  = { AHIST_L7_1, sample + 7, LENGTH };
      struct AHISampleInfo sample_fr  = { AHIST_L7_1, sample + 6, LENGTH };
      struct AHISampleInfo sample_rl  = { AHIST_L7_1, sample + 5, LENGTH };
      struct AHISampleInfo sample_rr  = { AHIST_L7_1, sample + 4, LENGTH };
      struct AHISampleInfo sample_sl  = { AHIST_L7_1, sample + 3, LENGTH };
      struct AHISampleInfo sample_sr  = { AHIST_L7_1, sample + 2, LENGTH };
      struct AHISampleInfo sample_c   = { AHIST_L7_1, sample + 1, LENGTH };
      struct AHISampleInfo sample_lfe = { AHIST_L7_1, sample + 0, LENGTH };
	
      if( AHI_LoadSound( 0, AHIST_SAMPLE, &sample_fl,  actrl) == AHIE_OK &&
	  AHI_LoadSound( 1, AHIST_SAMPLE, &sample_fr,  actrl) == AHIE_OK &&
	  AHI_LoadSound( 2, AHIST_SAMPLE, &sample_rl, actrl) == AHIE_OK &&
	  AHI_LoadSound( 3, AHIST_SAMPLE, &sample_rr, actrl) == AHIE_OK &&
	  AHI_LoadSound( 4, AHIST_SAMPLE, &sample_sl, actrl) == AHIE_OK &&
	  AHI_LoadSound( 5, AHIST_SAMPLE, &sample_sr, actrl) == AHIE_OK &&
	  AHI_LoadSound( 6, AHIST_SAMPLE, &sample_c, actrl) == AHIE_OK &&
	  AHI_LoadSound( 7, AHIST_SAMPLE, &sample_lfe, actrl) == AHIE_OK ) {

	if( AHI_ControlAudio( actrl,
			      AHIC_Play, TRUE,
			      TAG_DONE ) == AHIE_OK ) {
	  AHI_Play( actrl,
		    AHIP_BeginChannel, 0,
		    AHIP_Sound,        0,
		    AHIP_Freq,         44100,
		    AHIP_Vol,          0x10000,
		    AHIP_Pan,          0x8000,
		    AHIP_EndChannel,   0,
		    TAG_DONE );
	  printf( "Front Left.\n" );
	  sleep( 1 );
	  
	  AHI_SetSound( 0, 1, 0, 0, actrl, AHISF_IMM );
	  printf( "Front Right.\n" );
	  sleep( 1 );

	  AHI_SetSound( 0, 2, 0, 0, actrl, AHISF_IMM );
	  printf( "Rear Left.\n" );
	  sleep( 1 );

	  AHI_SetSound( 0, 3, 0, 0, actrl, AHISF_IMM );
	  printf( "Rear Right.\n" );
	  sleep( 1 );

	  AHI_SetSound( 0, 4, 0, 0, actrl, AHISF_IMM );
	  printf( "Surround Left.\n" );
	  sleep( 1 );

	  AHI_SetSound( 0, 5, 0, 0, actrl, AHISF_IMM );
	  printf( "Surround Right.\n" );
	  sleep( 1 );

	  AHI_SetSound( 0, 6, 0, 0, actrl, AHISF_IMM );
	  printf( "Center.\n" );
	  sleep( 1 );

	  AHI_SetSound( 0, 7, 0, 0, actrl, AHISF_IMM );
	  printf( "LFE.\n" );
	  sleep( 1 );
	  
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
	fprintf( stderr, "Unable load sounds.\n" );
	rc = RETURN_ERROR;
      }
	
      AHI_FreeAudio( actrl );
    }
    else {
      fprintf( stderr, "Unable to allocate audio.\n" );
      rc = RETURN_ERROR;
    }
  }
  
  else {
    fprintf( stderr, "Unable to allocate memory for sample.\n" );
  }

  free( sample );

  return rc;
}
