#ifndef C_IFF_H
#define C_IFF_H 1

/*
 *  c_iff - a portable IFF-parser
 *
 *  Copyright (C) 2000, 2001 Joerg Dietrich
 *
 *  This is the AROS-version of c_iff.
 *  It is distributed under the AROS Public License.
 *  But I reserve the right to distribute
 *  my own version under other licenses.
 */

/*
 *  c_iff.h - the public headerfile
 */

/*
 *  defines
 */

/*
 *  Why are TRUE and FALSE not defined in a standard C header?!
 */
#ifndef FALSE
#define FALSE (0)
#endif /* FALSE */

#ifndef TRUE
#define TRUE (~0)
#endif /* TRUE */

/*
 *  typedefs
 */

#include <stdint.h>

/*
 *  Do you have a big-endain machine?
 *  Then add it here!
 */

#if defined amiga || defined __PPC__
#define C_IFF_BIG_ENDIAN
#else
#undef C_IFF_BIG_ENDIAN
#endif

/*
 *  Byte-swapping
 *  Always swap shorts and ints before you write to
 *  or after you read from IFFs.
 */

#ifdef C_IFF_BIG_ENDIAN

#define Swap16IfLE(x) (x)

#define Swap32IfLE(x) (x)

#else /* C_IFF_BIG_ENDIAN */

#define Swap16IfLE(x) (((((unsigned short) x) & 0xFF00) >> 8) | ((((unsigned short) x) & 0xFF) << 8))

#define Swap32IfLE(x) (((((unsigned int) x) & 0xFF000000) >> 24) | \
		       ((((unsigned int) x) &   0xFF0000) >>  8) | \
		       ((((unsigned int) x) &     0xFF00) <<  8) | \
		       ((((unsigned int) x) &       0xFF) << 24))


#endif /* C_IFF_BIG_ENDIAN */

/*
 *  macro to create an IFF-ID.
 */

#define MAKE_ID(a,b,c,d) (((a)<<24) | ((b)<<16) | ((c)<<8) | ((d)))

/*
 *  Some predefined IDs.
 */

#define ID_FORM MAKE_ID('F','O','R','M')

/*
 *  This is the invalid ID.
 */

#define INVALID_ID (0)

/*
 *  includes
 */

#include <stdio.h>
#include <stdlib.h>

/*
 *  structs
 */

/*
 *  Struct to chain the open chunks together.
 */

struct ChunkNode
{
 struct ChunkNode *Previous; /* the previous chunk */
 long              Size;     /* size of the chunk */
 long              FilePos;  /* position of the size-uint32_t in the file */
};

/*
 *  struct IFFHandle, the center of c_iff
 */

struct IFFHandle
{
 FILE             *TheFile;          /* filehandle of the IFF */
 uint32_t          IFFType;          /* type of the IFF */
 uint32_t          ChunkID;          /* chunk-ID of the current chunk */
 long              BytesLeftInChunk; /* byte-counter for reading*/
 int               NewIFF;           /* marker for a IFF for writing */
 long              IFFSize;          /* size of the new IFF */
 struct ChunkNode *LastNode;         /* the current chunk */
};

/*
 *  prototypes
 */

extern struct IFFHandle *OpenIFF(char *Name);
extern void CloseIFF(struct IFFHandle *TheHandle);
extern int CheckIFF(struct IFFHandle *TheHandle);
extern int ReadChunkHeader(struct IFFHandle *TheHandle);
extern int SkipChunkData(struct IFFHandle *TheHandle);
extern long ReadChunkData(struct IFFHandle *TheHandle,
			  char *Buffer,
			  size_t BufferSize);

extern struct IFFHandle *NewIFF(char *Name, uint32_t IFFType);
extern int NewChunk(struct IFFHandle *TheHandle, uint32_t ID);
extern int NewSubFORM(struct IFFHandle *TheHandle, uint32_t Type);
extern void EndChunk(struct IFFHandle *TheHandle);
extern long WriteChunkData(struct IFFHandle *TheHandle,
			   char *Buffer,
			   size_t Size);

extern void FixIFFSize(struct IFFHandle *TheHandle);
extern size_t FileSize(FILE *TheFile);

#endif /* C_IFF_H */


