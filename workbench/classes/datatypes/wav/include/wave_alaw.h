/*
 *	wave.datatype
 *	(c) Fredrik Wikstrom
 */

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif

#ifndef WAVE_CLASS_H
#include "wave_class.h"
#endif

struct ALaw_Format {
    UWORD formatTag; /* WAVE_FORMAT_ALAW or WAVE_FORMAT_MULAW */
    WORD numChannels;
    LONG samplesPerSec;
    LONG avgBytesPerSec;
    WORD blockAlign; /* amount to read for each block */
    WORD bitsPerSample; /* 8 */

    WORD extraSize; /* 0 */
};

DEC_SETUPPROTO(SetupALaw);
DECODERPROTO(DecodeALaw);
DECODERPROTO(DecodeMuLaw);
