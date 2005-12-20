/*
    Copyright © 1995-2004, The AROS Development Team. All rights reserved.
    $Id$
    
    Demo for oss.library to playback audio samples.
    It can play raw audio files containing 16 bit signed PCM data in little endian format.
    Chaning the format isn't possible for now.
    Default number of channels is 2 (stereo) and default playback rate is 44100 (CD quality).
*/

#define __OSS_NOLIBBASE__

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include <dos/dos.h>
#include <proto/oss.h>
#include <proto/exec.h>
#include <proto/dos.h>

#define DSP_DRIVER_NAME "/dev/dsp"

#define BUFFERSIZE 0x57300

static struct Library *OSSBase;

static void cleanup(void *buffer, BPTR infh, BOOL closeoss)
{
    if(closeoss) OSS_Close();
    if(OSSBase) CloseLibrary(OSSBase);
    if(infh) Close(infh);
    free(buffer);

}

int main (int argc, char *argv[])
{
    BPTR infh;
    char *path;
    int format, channels, rate;
    int readlength, written, ok;
    void *buffer, *bufptr;

    OSSBase = NULL;
    format = 0; //AFMT_S16_LE;
    channels = 2;
    rate = 44100;
    if (argc == 5) {
	format = atoi(argv[2]);
	channels = atoi(argv[3]);
	rate = atoi(argv[4]);
    } else if (argc != 2) {
	printf ("Usage:\nplayoss <filename> [<format> <channels> <rate>]\n");
	return -1;
    }

    buffer = malloc(BUFFERSIZE);
    if (!buffer) {
	printf ("malloc error\n");
	return -1;
    }

    path = argv[1];
    infh = Open(path, MODE_OLDFILE);
    if (infh == 0) {
	printf ("error %ld opening file %s\n", IoErr(), path);
	cleanup(buffer, NULL, FALSE);
	return -1;
    }

    OSSBase = OpenLibrary("oss.library", 0);
    if (!OSSBase)
    {
	printf("Could not open oss.library!\n");
	cleanup(buffer, infh, FALSE);
	return -1;
    }

    ok = OSS_Open(DSP_DRIVER_NAME, FALSE, TRUE, TRUE);
    if (!ok) {
	printf ("error opening DSP\n");
	cleanup(buffer, infh, FALSE);
	return -1;
    }

    if( !OSS_SetFormat_S16LE() ) {
	printf("error setting format\n");
	cleanup(buffer, infh, TRUE);
	return -1;
    }

    if( !OSS_SetNumChannels(channels) ) {
	printf("error setting channels\n");
	cleanup(buffer, infh, TRUE);
	return -1;
    }

    if( !OSS_SetWriteRate(rate, &rate) ) {
	printf("error setting write rate\n");
	cleanup(buffer, infh, TRUE);
	return -1;
    }
    printf ("File:     %s\nFormat:   %d\nChannels: %d\nRate:     %d\n", path, format, channels, rate);

    while( (readlength = Read(infh, buffer, BUFFERSIZE)) > 0 ) {
	// printf("read %d\n", readlength);
	bufptr = buffer;
	do {
	    written = OSS_Write(bufptr, readlength);
	    // printf("written %d\n", written);
	    if( written < 0 ) {
		printf ("error writing audio %d\n", written);
		cleanup(buffer, infh, TRUE);
		return -1;
	    }

	    bufptr += written;
	    readlength -= written;
	} while( readlength > 0 );
    }

    if( readlength < 0 ) {
	printf ("error %ld reading file\n", IoErr());
	cleanup(buffer, infh, TRUE);
	return -1;
    }

    cleanup(buffer, infh, TRUE);
    return 0;
}
