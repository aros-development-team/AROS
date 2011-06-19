#ifndef WAVE_FORMATS_H
#define WAVE_FORMATS_H 1

#include "endian.h"

#if defined(CPU_IS_LITTLE_ENDIAN)
#include "wave_formats_le.h"
#else
#include "wave_formats_be.h"
#endif

#endif
