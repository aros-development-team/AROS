/*
    (C) 2000 AROS - The Amiga Research OS
    $Id$

    Desc: DataTypesDescriptorCreator
    Lang: English.
*/

/*
 *  includes
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <c_iff.h>

/*
 *  structs
 */

struct DataTypeHeader
{
 char *dth_Name;
 char *dth_BaseName;
 char *dth_Pattern;
 short *dth_Mask;
 unsigned long dth_GroupID;
 unsigned long dth_ID;
 short dth_MaskLen;
 short dth_Pad;
 unsigned short dth_Flags;
 unsigned short dth_Priority;
};

struct DTDesc
{
 struct DataTypeHeader DTH;
 char *Name;
 char *Version;
};

/*
 *  prototypes
 */

struct DTDesc *Parse(char *FileName);
void FreeDTD(struct DTDesc *DTD);
int SkipLine(unsigned char **Buffer, size_t *BufferSize);
int KeywordPart(unsigned char **Buffer, size_t *BufferSize);
unsigned char *DataPart(unsigned char **Buffer, size_t *BufferSize);
unsigned short ParseFlags(unsigned char **Buffer, size_t *BufferSize);
int WriteDescriptor(struct DTDesc *DTD);

/*
 *  functions
 */

int main(int argc, char **argv)
{
 struct DTDesc *DTD;

 if(!(argc==2))
 {
  fprintf(stderr, "usage: %s description\n", argv[0]);
  return(0);
 }

 DTD=Parse(argv[1]);
 if(!DTD)
 {
  fprintf(stderr, "Could not read description\n");
  return(0);
 }

 if(!WriteDescriptor(DTD))
 {
  fprintf(stderr, "Error writing DataType-Descriptor\n");
 }

 FreeDTD(DTD);
 return(0);
}

struct DTDesc *Parse(char *FileName)
{
 struct DTDesc *DTD;
 FILE *TheFile;
 size_t TheFileSize;
 unsigned char *FileBuffer, *BufPtr;

 DTD=NULL;

 if(!FileName)
 {
  return(NULL);
 }

 TheFile=fopen(FileName, "rb");
 if(!TheFile)
 {
  return(0);
 }

 TheFileSize=FileSize(TheFile);
 if(!TheFileSize)
 {
  fclose(TheFile);
  return(NULL);
 }

 FileBuffer=malloc(TheFileSize+1);
 if(!FileBuffer)
 {
  fclose(TheFile);
  return(NULL);
 }

 if(!(fread(FileBuffer, 1, TheFileSize, TheFile)==TheFileSize))
 {
  free((void *) FileBuffer);
  fclose(TheFile);
  return(NULL);
 }

 fclose(TheFile);

 FileBuffer[TheFileSize]='\0';

 DTD=malloc(sizeof(struct DTDesc));
 if(!DTD)
 {
  free((void *) FileBuffer);
  return(NULL);
 }

 DTD->Name=NULL;
 DTD->Version=NULL;
 DTD->DTH.dth_Name=NULL;
 DTD->DTH.dth_BaseName=NULL;
 DTD->DTH.dth_Pattern=NULL;
 DTD->DTH.dth_Mask=NULL;
 DTD->DTH.dth_GroupID=0;
 DTD->DTH.dth_ID=0;
 DTD->DTH.dth_MaskLen=0;
 DTD->DTH.dth_Pad=0;
 DTD->DTH.dth_Flags=0;
 DTD->DTH.dth_Priority=0;

 BufPtr=FileBuffer;

 while(TRUE)
 {
  int RetVal;

  RetVal=KeywordPart(&BufPtr, &TheFileSize);

  switch(RetVal)
  {
   case 0: /* Name */
   {
    DTD->Name=DataPart(&BufPtr, &TheFileSize);
    DTD->DTH.dth_Name=DTD->Name;

    break;
   }

   case 1: /* Version */
   {
    DTD->Version=DataPart(&BufPtr, &TheFileSize);

    break;
   }

   case 2: /* BaseName */
   {
    DTD->DTH.dth_BaseName=DataPart(&BufPtr, &TheFileSize);

    break;
   }

   case 3: /* Pattern */
   {
    DTD->DTH.dth_Pattern=DataPart(&BufPtr, &TheFileSize);

    break;
   }

   case 4: /* Mask */
   {
    unsigned char *CharMask;
    short *ShortMask;
    short MaskLen;
    short i;

    CharMask=(unsigned char *) DataPart(&BufPtr, &TheFileSize);
    if(!CharMask)
    {
     goto BREAK;
    }

    MaskLen=(short) strlen(CharMask);
    if(!MaskLen)
    {
     goto BREAK;
    }

    ShortMask=malloc(MaskLen*sizeof(short));
    if(!ShortMask)
    {
     goto BREAK;
    }

    for(i=0; i<MaskLen; i++)
    {
     if(CharMask[i]==0xFF)
     {
      ShortMask[i]=0xFFFF;
     }
     else
     {
      ShortMask[i]=(short) CharMask[i];
     }
    }

    DTD->DTH.dth_Mask=ShortMask;
    DTD->DTH.dth_MaskLen=MaskLen;
    free((void *) CharMask);
    break;
   }

   case 5: /* GroupID */
   {
    unsigned char IDString[4];

    if(TheFileSize<4)
    {
     goto BREAK;
    }

    IDString[0]=*BufPtr;
    BufPtr++;
    TheFileSize--;

    IDString[1]=*BufPtr;
    BufPtr++;
    TheFileSize--;

    IDString[2]=*BufPtr;
    BufPtr++;
    TheFileSize--;

    IDString[3]=*BufPtr;
    BufPtr++;
    TheFileSize--;

    if(!SkipLine(&BufPtr, &TheFileSize))
    {
     goto BREAK;
    }

    DTD->DTH.dth_GroupID=MAKE_ID(IDString[0], IDString[1], IDString[2], IDString[3]);

    break;
   }

   case 6: /* ID */
   {
    unsigned char IDString[4];

    if(TheFileSize<4)
    {
     goto BREAK;
    }

    IDString[0]=*BufPtr;
    BufPtr++;
    TheFileSize--;

    IDString[1]=*BufPtr;
    BufPtr++;
    TheFileSize--;

    IDString[2]=*BufPtr;
    BufPtr++;
    TheFileSize--;

    IDString[3]=*BufPtr;
    BufPtr++;
    TheFileSize--;

    if(!SkipLine(&BufPtr, &TheFileSize))
    {
     goto BREAK;
    }

    DTD->DTH.dth_ID=MAKE_ID(IDString[0], IDString[1], IDString[2], IDString[3]);

    break;
   }

   case 7: /* Flags */
   {
    DTD->DTH.dth_Flags=ParseFlags(&BufPtr, &TheFileSize);

    break;
   }

   case 8: /* Priority */
   {
    unsigned long Pri;

    Pri=0;

    Pri=strtoul(BufPtr, NULL, 10);

    if(!SkipLine(&BufPtr, &TheFileSize))
    {
     goto BREAK;
    }

    DTD->DTH.dth_Priority=(unsigned short) Pri;

    break;
   }

   default:
   {
    if(!SkipLine(&BufPtr, &TheFileSize))
    {
     goto BREAK;
    }
   }
  }
 }

BREAK:

 if(!(DTD->Name && DTD->DTH.dth_BaseName && DTD->DTH.dth_Mask && DTD->DTH.dth_MaskLen))
 {
  FreeDTD(DTD);
  return(NULL);
 }

 if(!DTD->DTH.dth_Pattern)
 {
  char *Pat;

  Pat=malloc(3);
  if(!Pat)
  {
   FreeDTD(DTD);
   return(NULL);
  }

  Pat[0]='#';
  Pat[1]='?';
  Pat[2]='\0';

  DTD->DTH.dth_Pattern=Pat;
 }

 free((void *) FileBuffer);
 return(DTD);
}

void FreeDTD(struct DTDesc *DTD)
{
 if(DTD)
 {
  if(DTD->Name)
  {
   free((void *) DTD->Name);
  }

  if(DTD->Version)
  {
   free((void *) DTD->Version);
  }

  if(DTD->DTH.dth_BaseName)
  {
   free((void *) DTD->DTH.dth_BaseName);
  }

  if(DTD->DTH.dth_Pattern)
  {
   free((void *) DTD->DTH.dth_Pattern);
  }

  if(DTD->DTH.dth_Mask)
  {
   free((void *) DTD->DTH.dth_Mask);
  }

  free((void *) DTD);
 }
}

unsigned short ParseFlags(unsigned char **Buffer, size_t *BufferSize)
{
 unsigned short Ret;
 unsigned char *TheFlags, *FlagsPtr;
 size_t Len, BytesLeft;
 unsigned int i, j;

 const char *Flags[] =
 {
  "DTF_BINARY",
  "DTF_ASCII",
  "DTF_IFF",
  "DTF_MISC",
  "DTF_CASE",
  "DTF_SYSTEM1"
 };

 const unsigned short FlagValues[] =
 {
  0x0000,
  0x0001,
  0x0002,
  0x0003,
  0x0010,
  0x1000
 };

 const int FlagLength[] =
 {
  10,
  9,
  7,
  8,
  8,
  11
 };

 const int NumFlags=6;

 Ret=0;

 TheFlags=(unsigned char *) DataPart(Buffer, BufferSize);
 if(!TheFlags)
 {
  return(0);
 }

 Len=strlen(TheFlags);
 if(!Len)
 {
  return(0);
 }

 BytesLeft=Len;
 FlagsPtr=TheFlags;

 for(i=0; i<Len; i++)
 {
  for(j=0; j<NumFlags; j++)
  {
   if(BytesLeft<FlagLength[j])
   {
    continue;
   }

   if(!(strncmp(Flags[j], FlagsPtr, FlagLength[j])))
   {
    Ret|=FlagValues[j];

    BytesLeft-=(FlagLength[j]-1);
    FlagsPtr+=(FlagLength[j]-1);
    break;
   }
  }

  BytesLeft--;
  FlagsPtr++;
 }

 free((void *) TheFlags);

 return(Ret);
}

int SkipLine(unsigned char **Buffer, size_t *BufferSize)
{
 unsigned char *LB;
 size_t LBS;
 int i;

 if(!(Buffer && BufferSize))
 {
  return(FALSE);
 }

 LB=*Buffer;
 LBS=*BufferSize;

 if(!(LB && LBS))
 {
  return(FALSE);
 }

 for(i=0; i<LBS; i++)
 {
  if(LB[i]=='\n')
  {
   break;
  }
 }

 LB+=i;
 LB++;
 LBS-=i;
 LBS--;

 *Buffer=LB;
 *BufferSize=LBS;

 return(TRUE);
}

int KeywordPart(unsigned char **Buffer, size_t *BufferSize)
{
 int Ret;
 unsigned char *LB;
 size_t LBS;
 int i;

#include "parser.h"

 Ret=-1;

 if(!(Buffer && BufferSize))
 {
  return(-1);
 }

 LB=*Buffer;
 LBS=*BufferSize;

 if(!(LB && LBS))
 {
  return(-1);
 }

 for(i=0; i<NumKeywords; i++)
 {
  if(LBS<KeywordLength[i])
  {
   continue;
  }

  if(!(strncmp(Keywords[i], LB, KeywordLength[i])))
  {
   Ret=i;
   LB+=KeywordLength[i];
   LBS-=KeywordLength[i];
   break;
  }
 }

 *Buffer=LB;
 *BufferSize=LBS;

 return(Ret);
}

unsigned char *DataPart(unsigned char **Buffer, size_t *BufferSize)
{
 unsigned char *Ret;
 unsigned char *LB;
 size_t LBS;
 int i;

 Ret=NULL;

 if(!(Buffer && BufferSize))
 {
  return(NULL);
 }

 LB=*Buffer;
 LBS=*BufferSize;

 if(!(LB && LBS))
 {
  return(NULL);
 }

 for(i=0; i<LBS; i++)
 {
  if(LB[i]=='\n')
  {
   break;
  }
 }

 if(!i)
 {
  return(NULL);
 }

 Ret=malloc(i+1);
 if(!Ret)
 {
  return(NULL);
 }

 memcpy(Ret, LB, i);
 Ret[i]='\0';

 LB+=i;
 LB++;
 LBS-=i;
 LBS--;

 *Buffer=LB;
 *BufferSize=LBS;

 return(Ret);
}

int WriteDescriptor(struct DTDesc *DTD)
{
 struct IFFHandle *IH;
 struct DataTypeHeader DTH;
 short i;

 if(!DTD)
 {
  return(FALSE);
 }

 /*
  *  At this moment all pointers in DTD must be valid!
  *  We don't check this.
  */

 IH=NewIFF(DTD->Name, MAKE_ID('D','T','Y','P'));
 if(!IH)
 {
  return(FALSE);
 }

 if(!NewChunk(IH, MAKE_ID('N','A','M','E')))
 {
  CloseIFF(IH);
  remove(DTD->Name);
  return(FALSE);
 }

 if(WriteChunkData(IH, DTD->Name, (strlen(DTD->Name)+1))<=0)
 {
  EndChunk(IH);
  CloseIFF(IH);
  remove(DTD->Name);
  return(FALSE);
 }

 EndChunk(IH);

 /*
  *  The Version-chunk is optional.
  */
 if(DTD->Version)
 {
  if(!NewChunk(IH, MAKE_ID('F','V','E','R')))
  {
   CloseIFF(IH);
   remove(DTD->Name);
   return(FALSE);
  }

  if(WriteChunkData(IH, DTD->Version, (strlen(DTD->Version)+1))<=0)
  {
   EndChunk(IH);
   CloseIFF(IH);
   remove(DTD->Name);
   return(FALSE);
  }

  EndChunk(IH);
 }

 if(!NewChunk(IH, MAKE_ID('D','T','H','D')))
 {
  CloseIFF(IH);
  remove(DTD->Name);
  return(FALSE);
 }

 DTH.dth_Name     = (char *) ((unsigned long) sizeof(struct DataTypeHeader));
 DTH.dth_BaseName = (char *) (((unsigned long) DTH.dth_Name) + strlen(DTD->DTH.dth_Name) + 1);
 DTH.dth_Pattern  = (char *) (((unsigned long) DTH.dth_BaseName) + strlen(DTD->DTH.dth_BaseName) + 1);
 DTH.dth_Mask     = (short *) (((unsigned long) DTH.dth_Pattern) + strlen(DTD->DTH.dth_Pattern) + 1);
 DTH.dth_GroupID  = DTD->DTH.dth_GroupID;
 DTH.dth_ID       = DTD->DTH.dth_ID;
 DTH.dth_MaskLen  = DTD->DTH.dth_MaskLen;
 DTH.dth_Pad      = DTD->DTH.dth_Pad;
 DTH.dth_Flags    = DTD->DTH.dth_Flags;
 DTH.dth_Priority = DTD->DTH.dth_Priority;

 DTH.dth_Name     = Swap32IfLE(DTH.dth_Name);
 DTH.dth_BaseName = Swap32IfLE(DTH.dth_BaseName);
 DTH.dth_Pattern  = Swap32IfLE(DTH.dth_Pattern);
 DTH.dth_Mask     = Swap32IfLE(DTH.dth_Mask);
 DTH.dth_GroupID  = Swap32IfLE(DTH.dth_GroupID);
 DTH.dth_ID       = Swap32IfLE(DTH.dth_ID);
 DTH.dth_MaskLen  = Swap16IfLE(DTH.dth_MaskLen);
 DTH.dth_Pad      = Swap16IfLE(DTH.dth_Pad);
 DTH.dth_Flags    = Swap16IfLE(DTH.dth_Flags);
 DTH.dth_Priority = Swap16IfLE(DTH.dth_Priority);

 if(WriteChunkData(IH, (char *) &DTH, sizeof(struct DataTypeHeader))<=0)
 {
  EndChunk(IH);
  CloseIFF(IH);
  remove(DTD->Name);
  return(FALSE);
 }

 if(WriteChunkData(IH, DTD->DTH.dth_Name, (strlen(DTD->DTH.dth_Name)+1))<=0)
 {
  EndChunk(IH);
  CloseIFF(IH);
  remove(DTD->Name);
  return(FALSE);
 }

 if(WriteChunkData(IH, DTD->DTH.dth_BaseName, (strlen(DTD->DTH.dth_BaseName)+1))<=0)
 {
  EndChunk(IH);
  CloseIFF(IH);
  remove(DTD->Name);
  return(FALSE);
 }

 if(WriteChunkData(IH, DTD->DTH.dth_Pattern, (strlen(DTD->DTH.dth_Pattern)+1))<=0)
 {
  EndChunk(IH);
  CloseIFF(IH);
  remove(DTD->Name);
  return(FALSE);
 }

 for(i=0; i<DTD->DTH.dth_MaskLen; i++)
 {
  DTD->DTH.dth_Mask[i]=Swap16IfLE(DTD->DTH.dth_Mask[i]);
 }

 if(WriteChunkData(IH, (char *) DTD->DTH.dth_Mask, DTD->DTH.dth_MaskLen*sizeof(short))<=0)
 {
  EndChunk(IH);
  CloseIFF(IH);
  remove(DTD->Name);
  return(FALSE);
 }

 EndChunk(IH);

 CloseIFF(IH);
 return(TRUE);
}

