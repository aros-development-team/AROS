#ifndef WAVE_FORMATS_H
#define WAVE_FORMATS_H 1

#ifdef __X86__
#define LITTLE_ENDIAN 1
#endif

#if defined(LITTLE_ENDIAN)
#include "wave_formats_le.h"
#else
#include "wave_formats_be.h"
#endif

#endif
