/*
 *	wave.datatype
 *	(c) Fredrik Wikstrom
 */

#include <aros/debug.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <exec/types.h>
#include <exec/memory.h>
#include <dos/dostags.h>
#include <intuition/imageclass.h>
#include <intuition/icclass.h>
#include <intuition/gadgetclass.h>
#include <intuition/cghooks.h>
#include <datatypes/datatypesclass.h>
#include <datatypes/soundclass.h>

#include <clib/alib_protos.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/iffparse.h>
#include <proto/datatypes.h>

#include <aros/symbolsets.h>

#include "wave_class.h"
#include "endian.h"
#include "riff-wave.h"
#include "decoders.h"

ADD2LIBS("datatypes/sound.datatype", 0, struct Library *, SoundBase);

IPTR WAVE__DTM_WRITE(Class *cl, Object *obj, struct dtWrite *msg)
{
    IPTR retval = FALSE;

D(bug("[wave.dt]: %s()\n", __PRETTY_FUNCTION__));

    // check if dt's native format should be used
    if (msg->dtw_Mode == DTWM_RAW)
    {
	LONG Error = OK;
	BPTR file = (BPTR)msg->dtw_FileHandle;
	BYTE *Channel[2] = {0};
	LONG samplesPerSec = 0, totalFrames = 0, numChannels;
	LONG encodeBufferSize, encodeFrames;
	BYTE *encodeBuffer = NULL;
	LONG Status, pad_byte=0;
	struct RIFFChunk chunk;
	struct WaveFormat fmt;

	if (!file)
	{
	    Error = ERROR_REQUIRED_ARG_MISSING;
	    goto out;
	}

	{
	    BYTE *Sample = NULL;
	    GetDTAttrs(obj,
		    SDTA_Sample,		&Sample,
		    SDTA_LeftSample,	&Channel[0],
		    SDTA_RightSample,	&Channel[1],
		    SDTA_SamplesPerSec,	&samplesPerSec,
		    SDTA_SampleLength,	&totalFrames,
	    TAG_END);

	    if (Sample) {
		Channel[0] = Sample;
		Channel[1] = NULL;
	    }
	    if (!Channel[0] && Channel[1]) {
		Channel[0] = Channel[1];
		Channel[1] = NULL;
	    }
	}

	if ((!Channel[0] && !Channel[1])
		|| samplesPerSec == 0 || totalFrames == 0)
	{
		Error = ERROR_REQUIRED_ARG_MISSING;
		goto out;
	}

	if (Channel[1])
		numChannels = 2;
	else
		numChannels = 1;

	encodeFrames = SND_BUFFER_SIZE / numChannels;
	do {
		encodeBufferSize = encodeFrames * numChannels;
		encodeBuffer = AllocVec(encodeBufferSize, MEMF_CLEAR);
		if (!encodeBuffer) encodeFrames >>= 1;
	} while (!encodeBuffer && encodeFrames > 0);
	if (!encodeBuffer) {
		Error = ERROR_NO_FREE_STORE;
		goto out;
	}

	if ((numChannels == 1) && (totalFrames & 1)) pad_byte=1;

	/* write RIFF-WAVE header */
	{
		LONG HDR[3];
		HDR[0] = ID_RIFF;
		write_le32(&HDR[1], 2*sizeof(struct RIFFChunk) + sizeof(struct WaveFormat) + totalFrames*numChannels
			+ pad_byte);
		HDR[2] = ID_WAVE;
		Status = Write(file, HDR, 12);
	}
	if (Status != 12) {
		Error = WriteError(Status);
		goto out;
	}

	/* Write fmt chunk */
	chunk.type = ID_fmt;
	write_le32(&chunk.size, sizeof(struct WaveFormat));
	Status = Write(file, &chunk, sizeof(struct RIFFChunk));
	if (Status != sizeof(struct RIFFChunk)) {
	    Error = WriteError(Status);
	    goto out;
	}
	fmt.formatTag = WAVE_FORMAT_PCM;
	write_le16(&fmt.numChannels, numChannels);
	write_le32(&fmt.samplesPerSec, samplesPerSec);
	write_le32(&fmt.avgBytesPerSec, samplesPerSec * numChannels);
	write_le16(&fmt.blockAlign, numChannels);
	write_le16(&fmt.bitsPerSample, 8);
	Status = Write(file, &fmt, sizeof(struct WaveFormat));
	if (Status != sizeof(struct WaveFormat)) {
	    Error = WriteError(Status);
	    goto out;
	}

	/* Write PCM data */
	chunk.type = ID_data;
	write_le32(&chunk.size, totalFrames*numChannels);
	Status = Write(file, &chunk, sizeof(struct RIFFChunk));
	if (Status != sizeof(struct RIFFChunk)) {
		Error = WriteError(Status);
		goto out;
	}

	{
	    LONG frame, frame2, chan;
	    BYTE *Dst;
	    for (frame=totalFrames;frame>0;frame-=encodeFrames) {
		Dst = encodeBuffer;

		if (frame < encodeFrames)
		    encodeFrames = frame;

		for (frame2=0;frame2<encodeFrames;frame2++) {
		    for (chan=0;chan<numChannels;chan++) {
			*Dst++ = (*Channel[chan]++) + 128; // convert signed -> unsigned (8-bit)
		    }
		}

		Status = Write(file, encodeBuffer, encodeFrames * numChannels);
		if (Status != (encodeFrames * numChannels)) {
		    Error = WriteError(Status);
		    goto out;
		}
	    }
	}

	/* Write pad byte */

	if (pad_byte) {
	    BYTE null_byte=0;
	    Write(file, &null_byte, 1);
	}

out:
	FreeVec(encodeBuffer);
	
/////
	if (Error == OK) {
		retval = TRUE;
	} else {
		SetIoErr(Error);
		retval = FALSE;
	}
    }
    return retval;
}

static LONG ConvertWAVE (BPTR file, BYTE **ChannelsPtr,
	struct VoiceHeader *vhdr)
{
    BOOL Done = FALSE;
    LONG Error = OK, Status;

    LONG totalFrames = 0;
    LONG numBlocks = 0;
    LONG decodeFrames = 0;
    LONG decodeBufferSize = 0, sampleBufferSize = 0;
    UBYTE *decodeBuffer = NULL;

    struct RIFFChunk chunk;
    struct WaveFormatEx *fmt = NULL;

    struct Decoder *	decoder = NULL;
    struct DecoderData	dec_data = {0};

D(bug("[wave.dt]: %s()\n", __PRETTY_FUNCTION__));

    DECODERPROTO((*Decode)); // decoder routine

    /* Check for RIFF-WAVE header */
    {
	    ULONG HDR[3]={0};
	    Status = Read(file, HDR, 12);
	    if (Status != 12)
	    {
		Error = ReadError(Status);
D(bug("[wave.dt] %s: Failed to read header! (expected 12bytes, read %d, error %d)\n", __PRETTY_FUNCTION__, Status, Error));
	    }
	    else if (HDR[0] != ID_RIFF || HDR[2] != ID_WAVE)
	    {
		Error = ERROR_OBJECT_WRONG_TYPE;
D(bug("[wave.dt] %s: File isnt RIFF-WAVE format\n", __PRETTY_FUNCTION__));
	    }
    }

    while (!Done && Error == OK) {
D(bug("[wave.dt] %s: Reading RIFF Chunk ... ", __PRETTY_FUNCTION__));

	    Status = Read(file, &chunk, sizeof(struct RIFFChunk));

D(bug("done, %d bytes\n", Status));

	    if (Status != sizeof(struct RIFFChunk))
		    Error = ReadError(Status);

	    chunk.size = read_le32(&chunk.size);

D(bug("[wave.dt] %s: chunk size = %d\n", __PRETTY_FUNCTION__, chunk.size));
	    
	    if (Error == OK) {
		    switch(chunk.type) {

			    // Format header chunk
			    case ID_fmt:
				    if (chunk.size < sizeof(struct WaveFormat)) {
					    Error = NOTOK;
					    break;
				    }
				    dec_data.fmt = fmt =
					    AllocVec((chunk.size >= 18) ? chunk.size : 18, MEMF_CLEAR);
				    if (!fmt) {
					    Error = ERROR_NO_FREE_STORE;
					    break;
				    }

				    Status = Read(file, fmt, chunk.size);
				    if (Status != chunk.size) {
					    Error = ReadError(Status);
					    break;
				    }

D(bug("[wave.dt] %s: FMT Chunk Type\n", __PRETTY_FUNCTION__));

				    /* Convert to proper endianness */
				    fmt->numChannels = read_le16(&fmt->numChannels);
				    fmt->samplesPerSec = read_le32(&fmt->samplesPerSec);
				    fmt->blockAlign = read_le16(&fmt->blockAlign);
				    fmt->bitsPerSample = read_le16(&fmt->bitsPerSample);

D(bug("[wave.dt] %s: %d channels, %d samples/sec\n", __PRETTY_FUNCTION__, fmt->numChannels, fmt->samplesPerSec));
				    
				    if (chunk.size == 16) { // unextended
					    fmt->extraSize = 0;
				    } else if (chunk.size >= 18) { // extended
					    fmt->extraSize = chunk.size - 18;
				    } else {
					    Error = NOTOK;
					    break;
				    }

D(bug("[wave.dt] %s: format %x\n", __PRETTY_FUNCTION__, fmt->formatTag));

				    /* check formatTag */
				    dec_data.chunk = chunk;
				    decoder = GetDecoder(fmt->formatTag);
				    if (decoder) {
D(bug("[wave.dt] %s: using decoder @ 0x%p\n", __PRETTY_FUNCTION__, decoder));
					    dec_data.Decode = decoder->Decode;
					    dec_data.DecodeFrames = decoder->DecodeFrames;
					    Error = decoder->Setup(&dec_data);
					    if (Error != OK) break;
				    } else {
D(bug("[wave.dt] %s: unknown format!\n", __PRETTY_FUNCTION__));
					    Error = DTERROR_UNKNOWN_COMPRESSION;
					    break;
				    }

				    /* Only mono/stereo is supported currently */
				    if ((fmt->numChannels < 1) || (fmt->numChannels > MAX_CHANNELS)) {
					    Error = ERROR_NOT_IMPLEMENTED;
					    break;
				    }

				    break;

			    // Contains numSampleFrames (necessary if not PCM!)
			    case ID_fact:
D(bug("[wave.dt] %s: FACT Chunk Type\n", __PRETTY_FUNCTION__));
				    if (chunk.size != 4) {
					    Error = NOTOK;
				    }
				    Status = Read(file, &totalFrames, 4);
				    if (Status != 4) {
					    Error = ReadError(Status);
					    break;
				    }
				    totalFrames = read_le32(&totalFrames);
				    break;

			    // Sample data
			    case ID_data:
D(bug("[wave.dt] %s: DATA Chunk Type\n", __PRETTY_FUNCTION__));
				    /* check if fmt chunk was successfully parsed */
				    if (!fmt) {
					    Error = NOTOK;
					    break;
				    }
				    /* Do we have enough information to calculate a "theoretical" maximum
					value for totalFrames to check with? */
				    if (fmt->blockAlign && dec_data.blockFrames) {
					    LONG totalFramesMax =
						    (chunk.size / fmt->blockAlign) * dec_data.blockFrames;

					    /* check that totalFrames is within "reasonable bounds */

					    if (totalFrames <= 0 || totalFramesMax > (totalFrames + dec_data.blockFrames)
						    || totalFrames > totalFramesMax)
					    {
						    /* it wasn't, so we assume it to be faulty/corrupt */
						    totalFrames = totalFramesMax;
					    }
				    }
				    sampleBufferSize = totalFrames;

				    // allocate buffers

				    {
					    LONG i;
					    for (i=0;i<fmt->numChannels;i++) {
						    ChannelsPtr[i] = AllocVec(sampleBufferSize, MEMF_CLEAR);
						    if (!ChannelsPtr[i]) {
							    Error = ERROR_NO_FREE_STORE;
							    break;
						    }
					    }
				    }
				    numBlocks = (SND_BUFFER_SIZE + (fmt->blockAlign >> 1)) / fmt->blockAlign;
				    if (!numBlocks) numBlocks++;
				    do {
					    decodeBufferSize = numBlocks * fmt->blockAlign;
					    decodeBuffer = AllocVec(decodeBufferSize, MEMF_CLEAR);
					    if (!decodeBuffer) numBlocks >>= 1;
				    } while (!decodeBuffer && numBlocks > 0);
				    if (!decodeBuffer) {
					    Error = ERROR_NO_FREE_STORE;
					    break;
				    }
				    decodeFrames = numBlocks * dec_data.blockFrames;

				    {
					    BYTE *ChanPtr[2];
					    {
						    LONG i;
						    for (i=0;i<fmt->numChannels;i++)
							    ChanPtr[i] = ChannelsPtr[i];
					    }
					    {
						    LONG read_size;
						    LONG frames_left;
						    LONG frames;
						    LONG blocks_left;
						    LONG blocks;

						    read_size = decodeBufferSize;
						    frames_left = totalFrames;
						    frames = decodeFrames;
						    blocks_left = chunk.size / fmt->blockAlign;
						    blocks = numBlocks;

						    while (blocks_left > 0 && frames_left > 0) {
							    if (blocks_left < blocks) {
								    blocks = blocks_left;
								    read_size = blocks*fmt->blockAlign;
							    }
							    if (frames_left < frames) frames = frames_left;

							    Status = Read(file, decodeBuffer, read_size);
							    if (Status != read_size) {
								    Error = ReadError(Status);
								    break;
							    }
							    Status = dec_data.Decode(&dec_data, fmt, decodeBuffer, ChanPtr, blocks, frames);
							    if (Status != frames) {
								    Error = NOTOK;
								    break;
							    }

							    blocks_left -= blocks;
							    frames_left -= frames;
						    }
					    }

				    }
				    if (Error == OK) Done = TRUE;
				    break;

			    // skip unknown chunks!
			    default:
D(bug("[wave.dt] %s: Unknown Chunk Type - Skipping\n", __PRETTY_FUNCTION__));
				    if (Seek(file, (chunk.size+1)&~1, OFFSET_CURRENT) == -1)
					    Error = IoErr();
				    break;

		    }
	    }
	    D(bug("[wave.dt] %s: Chunk done\n", __PRETTY_FUNCTION__));
    }

D(bug("[wave.dt] %s: Finished\n", __PRETTY_FUNCTION__));
    
    FreeVec(decodeBuffer);

    if (decoder) {
	    if (dec_data.state) {
		    if (decoder->Cleanup)
			    decoder->Cleanup(&dec_data);
		    else
		    {
			    FreeVec(dec_data.state);
		    }
	    }
    }


    if (Error == OK) {
	    vhdr->vh_OneShotHiSamples = totalFrames;
	    vhdr->vh_SamplesPerSec = fmt->samplesPerSec;
	    vhdr->vh_Octaves = 1;
	    vhdr->vh_Compression = 0; // none
	    vhdr->vh_Volume = 0x10000; // max volume
    } else {
	    LONG i;
	    for (i=0;i<fmt->numChannels && i<MAX_CHANNELS;i++) {
		    FreeVec(ChannelsPtr[i]);
		    ChannelsPtr[i] = NULL;
	    }
    }

    FreeVec(fmt);

    return(Error);
}

IPTR WAVE__OM_NEW (Class *cl, Object *self, Msg msg)
{
D(bug("[wave.dt]: %s()\n", __PRETTY_FUNCTION__));

    self = (IPTR)DoSuperMethodA(cl, self, msg);
    if (self) 
    {
	struct VoiceHeader *vhdr = NULL;
	char	* FileName;
	IPTR SourceType = -1;
	IPTR Error = ERROR_REQUIRED_ARG_MISSING;
	BPTR file = BNULL;

	D(bug("[wave.dt] %s: obj @ 0x%p\n", __PRETTY_FUNCTION__, self));

	FileName = (char *)GetTagData(DTA_Name, (IPTR)"Untitled", ((struct opSet *)msg)->ops_AttrList);

	D(bug("[wave.dt] %s: filename '%s'\n", __PRETTY_FUNCTION__, FileName));

	GetDTAttrs(self,
		SDTA_VoiceHeader,	&vhdr,
		DTA_Handle,		&file,
		DTA_SourceType,		&SourceType,
	TAG_END);

	/* Do we have everything we need? */
	if (vhdr && file && SourceType == DTST_FILE)
	{
	    BYTE *Channel[MAX_CHANNELS] = {0};
	    /* Convert the audio file */
	    Error = ConvertWAVE(file, Channel, vhdr);
	    if (Error == OK)
	    {
		BYTE *Sample = NULL;

		if (!Channel[0] || !Channel[1]) {
		    if (Channel[0])
			Sample = Channel[0];
		    else
			Sample = Channel[1];
		}

		/* Fill in the remaining information */
		SetDTAttrs(self, NULL, NULL,
			DTA_ObjName, FilePart(FileName),
			SDTA_Sample, Sample,
			SDTA_LeftSample, Channel[0],
			SDTA_RightSample, Channel[1],
			SDTA_SampleLength, vhdr->vh_OneShotHiSamples,
			SDTA_SamplesPerSec, vhdr->vh_SamplesPerSec,
			SDTA_Volume,	 64,
			SDTA_Cycles, 1,
			TAG_END);
	    }
	} else if (SourceType != DTST_FILE) Error = ERROR_NOT_IMPLEMENTED;

	if (Error != OK) {
	    CoerceMethod(cl, (Object *)self, OM_DISPOSE);

	    self = (Object *)NULL;
	    SetIoErr(Error);
	}
    }

    return self;
}
