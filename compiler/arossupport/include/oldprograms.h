#define USHORT UWORD
#define SHORT WORD

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/mathffp.h>
#include <proto/mathtrans.h>
/*
  #include <proto/layers.h>
*/
#include <intuition/iobsolete.h>

#ifndef TOPAZ_EIGHTY
  #define TOPAZ_EIGHTY 8
#endif

#ifndef TOPAZ_SIXTY
  #define TOPAZ_SIXTY 6
#endif

#define RASSIZE(w,h)	((ULONG)(h)*( ((ULONG)(w)+15)>>3&0xFFFE))

