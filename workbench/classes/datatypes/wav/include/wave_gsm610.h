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

#include "gsm/gsm.h"

DEC_SETUPPROTO(SetupGSM610);
DEC_CLEANUPPROTO(CleanupGSM610);
DECODERPROTO(DecodeGSM610);
