/*
 *  c_iff - a portable IFF-parser
 *
 *  Copyright (C) 2000 Joerg Dietrich
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/*
 *  example.c - shows how to use c_iff
 */

/*
 *  includes
 */

#include <stdio.h>
#include <string.h>

/*
 *  include c_iff.h when you want to use c_iff
 */

#include "c_iff.h"

/*
 *  prototypes
 */

void PrintID(unsigned long ID);

/*
 *  the main-program
 */

int main(int argc, char **argv)
{
 /*
  *  struct IFFHandle is the basic struct of c_iff.
  */
 struct IFFHandle *IH;

 if(!(argc==2))
 {
  fprintf(stderr, "usage: %s IFF-file\n", argv[0]);
 }

 /*
  *  OpenIFF() is used to open an existing IFF
  */
 IH=OpenIFF(argv[1]);
 if(!IH)
 {
  return(0);
 }

 printf("Type: ");
 PrintID(IH->IFFType);
 printf("\n");

 while(TRUE)
 {
  /*
   *  Read the header of the next chunk.
   */
  if(!ReadChunkHeader(IH))
  {
   break;
  }

  printf(" Chunk: ");
  PrintID(IH->ChunkID);
  printf("\n");

  /*
   *  If you are not interessted in the Data, skip it with SkipChunkData() .
   *  If you need the Data use ReadChunkData() .
   */
  SkipChunkData(IH);
 }

 /*
  *  Alway close an IFF with CloseIFF() !
  */
 CloseIFF(IH);

 /*
  *  Use NewIFF() to open a new IFF for writing.
  */
 IH=NewIFF("test1.iff", MAKE_ID('A','N','I','M'));
 if(!IH)
 {
  return(0);
 }

 /*
  *  Open a new SubFORM, an IFF inside an IFF.
  */
 if(NewSubFORM(IH, MAKE_ID('I','L','B','M')))
 {
  /*
   *  Open a new Chunk.
   */
  if(NewChunk(IH, MAKE_ID('F','V','E','R')))
  {
   /*
    *  Write some data to the chunk.
    *  The chunk-sizes are automatically fixed.
    */
   WriteChunkData(IH, "$VER: test1 1.10", 17);

   /*
    *  You must always end a chunk with EndChunk() .
    */
   EndChunk(IH);
  }

  if(NewChunk(IH, MAKE_ID('A','U','T','H')))
  {
   WriteChunkData(IH, "Jörg Dietrich", 13);
   EndChunk(IH);
  }

  /*
   *  EndChunk is used too, to end a SubFORM.
   */
  EndChunk(IH);
 }

 /*
  *  And always close your IFF.
  */
 CloseIFF(IH);
 return(0);
}

void PrintID(unsigned long ID)
{
 char Buffer[5];

 Buffer[0]=(ID&0xFF000000)>>24;
 Buffer[1]=(ID&0xFF0000)  >>16;
 Buffer[2]=(ID&0xFF00)    >> 8;
 Buffer[3]=(ID&0xFF);
 Buffer[4]='\0';

 printf("%s", Buffer);
}


