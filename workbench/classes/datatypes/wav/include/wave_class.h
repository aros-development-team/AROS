/*
 *	wave.datatype
 *	(c) Fredrik Wikstrom
 */

#ifndef WAVE_CLASS_H
#define WAVE_CLASS_H 1

#define GLOBAL_IFACES			(FALSE)
#define SND_BUFFER_SIZE 		(16 << 10) // size of buffer used for decoding/encoding
#define MAX_CHANNELS			(2)

#include <intuition/intuition.h>
#include <intuition/gadgetclass.h>
#include <intuition/classes.h>
#include <intuition/cghooks.h>

#include <dos/dosextens.h>

#include <exec/libraries.h>
#include <exec/execbase.h>
#include <exec/resident.h>
#include <exec/memory.h>

#include <devices/clipboard.h>

#include <datatypes/soundclass.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/datatypes.h>
#include <proto/dtclass.h>

#include "riff-wave.h"
#include "decoders.h"

#define OK (0)
#define NOTOK DTERROR_INVALID_DATA
#define ERROR_EOF DTERROR_NOT_ENOUGH_DATA

#define ReadError(C) ((C == -1) ? IoErr() : ERROR_EOF)
#define WriteError(C) IoErr()
#endif
