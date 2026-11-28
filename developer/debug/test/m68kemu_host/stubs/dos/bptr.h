#pragma once
#include <stddef.h>
/* Exercise the seglist layout from the tree with host pointer BPTRs. */
#define AROS_FAST_BPTR 1
#ifndef __WORDSIZE
#define __WORDSIZE (__SIZEOF_POINTER__ * 8)
#endif
#include "../../../../../../compiler/include/dos/bptr.h"

/* The production C API returns APTR; this C++ host fixture needs the byte-pointer type. */
#ifdef __cplusplus
#undef AROS_SEG_DATA
#define AROS_SEG_DATA(seg) ((UBYTE *)AROS_SEG_FROMBPTR(seg)->seg_Data)
static_assert(sizeof(BPTR) == sizeof(void *), "AROS_FAST_BPTR expects host pointer width");
static_assert(sizeof(BPTR) * 8 == __WORDSIZE, "AROSSegment __WORDSIZE must match BPTR width");
static_assert(offsetof(struct AROSSegment, seg_Next) == AROS_SEG_PREFIX,
              "AROSSegment seg_Next must sit at AROS_SEG_PREFIX");
static_assert(offsetof(struct AROSSegment, seg_Size) + sizeof(ULONG) == AROS_SEG_PREFIX,
              "AROSSegment seg_Size must sit immediately before seg_Next");
static_assert(offsetof(struct AROSSegment, seg_Data) == AROS_SEG_HEADER,
              "AROSSegment seg_Data must sit at AROS_SEG_HEADER");
#endif
