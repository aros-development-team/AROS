/*
    Copyright © 2000-2019, The AROS Development Team. All rights reserved.
    $Id$

    Desc: DataTypesDescriptorCreator
    Lang: English.
*/

#include "createdtdesc.h"
#include "parser.h"

int main(int argc, char **argv)
{
 struct DTDesc *TheDTDesc;

 if(Init(argc, argv, &TheDTDesc))
 {
  Work(TheDTDesc);
 }

 Cleanup(TheDTDesc);

 return(0);
}

int HandleName(struct DTDesc *TheDTDesc)
{
 uint8_t *DataPtr;

 if(!TheDTDesc)
 {
  return(FALSE);
 }

 TheDTDesc->ReadBuffer[READBUFFERSIZE-1]='\0';

 DataPtr=TheDTDesc->ReadBuffer+KeywordLength[Name];

 /*
  *  What, who yells buffer-overflow here?
  */

 strcpy(TheDTDesc->Name, DataPtr);

 TheDTDesc->DTH.dth_Name=TheDTDesc->Name;

 return(TRUE);
}

int HandleVersion(struct DTDesc *TheDTDesc)
{
 uint8_t *DataPtr;

 if(!TheDTDesc)
 {
  return(FALSE);
 }

 TheDTDesc->ReadBuffer[READBUFFERSIZE-1]='\0';

 DataPtr=TheDTDesc->ReadBuffer+KeywordLength[Version];

 /*
  *  What, who yells buffer-overflow here?
  */

 strcpy(TheDTDesc->Version, DataPtr);

 return(TRUE);
}

int HandleBaseName(struct DTDesc *TheDTDesc)
{
 uint8_t *DataPtr;

 if(!TheDTDesc)
 {
  return(FALSE);
 }

 TheDTDesc->ReadBuffer[READBUFFERSIZE-1]='\0';

 DataPtr=TheDTDesc->ReadBuffer+KeywordLength[BaseName];

 /*
  *  What, who yells buffer-overflow here?
  */

 strcpy(TheDTDesc->BaseName, DataPtr);

 TheDTDesc->DTH.dth_BaseName=TheDTDesc->BaseName;

 return(TRUE);
}

int HandlePattern(struct DTDesc *TheDTDesc)
{
 uint8_t *DataPtr;

 if(!TheDTDesc)
 {
  return(FALSE);
 }

 TheDTDesc->ReadBuffer[READBUFFERSIZE-1]='\0';

 DataPtr=TheDTDesc->ReadBuffer+KeywordLength[Pattern];

 /*
  *  What, who yells buffer-overflow here?
  */

 strcpy(TheDTDesc->Pattern, DataPtr);

 TheDTDesc->DTH.dth_Pattern=TheDTDesc->Pattern;

 return(TRUE);
}

int HandleMask(struct DTDesc *TheDTDesc)
{
 uint8_t *DataPtr;
 int i;

 if(!TheDTDesc)
 {
  return(FALSE);
 }

 TheDTDesc->ReadBuffer[READBUFFERSIZE-1]='\0';

 DataPtr=TheDTDesc->ReadBuffer+KeywordLength[Mask];

#if 1
 {
  uint8_t *NewDataPtr;
  int done = 0;
  
  TheDTDesc->DTH.dth_MaskLen = 0;
  
  while(!done)
  {
   uint8_t c = *DataPtr++;

   switch(c)
   {
    case '\0':
     done = 1;
     break;
     
    case '\'':
     c = *DataPtr++;
     TheDTDesc->Mask[TheDTDesc->DTH.dth_MaskLen++] = c;
     if (c) c = *DataPtr++;
     if (!c) done = 1;
     break;
     
    case 'A':
     c = *DataPtr++;
     if (!c) done = 1;
     if (c != 'N') break;
     c = *DataPtr++;
     if (!c) done = 1;
     if (c != 'Y') break;
     TheDTDesc->Mask[TheDTDesc->DTH.dth_MaskLen++] = 0xFFFF;
     break;
     
    case ' ':
    case '\t':
     break;
     
    default:
     DataPtr--;
     i = strtol(DataPtr, (char **)(&NewDataPtr), 0);
     if (DataPtr != NewDataPtr)
     {
      DataPtr = NewDataPtr;
      TheDTDesc->Mask[TheDTDesc->DTH.dth_MaskLen++] = i;      
     }
     else
     {
      DataPtr++;
     }
     break;
     
   }
  }
 }
 
#else
 TheDTDesc->DTH.dth_MaskLen=(uint16_t) strlen(DataPtr);
 if(!TheDTDesc->DTH.dth_MaskLen)
 {
  return(TRUE);
 }

 for(i=0; i<TheDTDesc->DTH.dth_MaskLen; i++)
 {
  TheDTDesc->Mask[i] = (DataPtr[i]==0xFF) ? 0xFFFF : (uint16_t) DataPtr[i];
 }
#endif

 TheDTDesc->DTH.dth_Mask=TheDTDesc->Mask;

 return(TRUE);
}

int HandleGroupID(struct DTDesc *TheDTDesc)
{
 uint8_t *DataPtr;

 if(!TheDTDesc)
 {
  return(FALSE);
 }

 TheDTDesc->ReadBuffer[READBUFFERSIZE-1]='\0';

 DataPtr=TheDTDesc->ReadBuffer+KeywordLength[GroupID];

 if(strlen(DataPtr)<4)
 {
  return(FALSE);
 }

 TheDTDesc->DTH.dth_GroupID=MAKE_ID(DataPtr[0], DataPtr[1], DataPtr[2], DataPtr[3]);

 return(TRUE);
}

int HandleID(struct DTDesc *TheDTDesc)
{
 uint8_t *DataPtr;

 if(!TheDTDesc)
 {
  return(FALSE);
 }

 TheDTDesc->ReadBuffer[READBUFFERSIZE-1]='\0';

 DataPtr=TheDTDesc->ReadBuffer+KeywordLength[ID];

 if(strlen(DataPtr)<4)
 {
  return(FALSE);
 }

 TheDTDesc->DTH.dth_ID=MAKE_ID(DataPtr[0], DataPtr[1], DataPtr[2], DataPtr[3]);

 return(TRUE);
}

int HandleFlags(struct DTDesc *TheDTDesc)
{
 uint8_t *DataPtr;
 long Len;
 int i;

 const char *TheFlags[] =
 {
  "DTF_BINARY",
  "DTF_ASCII",
  "DTF_IFF",
  "DTF_MISC",
  "DTF_CASE",
  "DTF_SYSTEM1"
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

 const uint16_t FlagValues[] =
 {
  0x0000,
  0x0001,
  0x0002,
  0x0003,
  0x0010,
  0x1000
 };

 if(!TheDTDesc)
 {
  return(FALSE);
 }

 TheDTDesc->ReadBuffer[READBUFFERSIZE-1]='\0';

 DataPtr=TheDTDesc->ReadBuffer+KeywordLength[Flags];

 TheDTDesc->DTH.dth_Flags=0;

 Len=strlen(DataPtr);
 if(!Len)
 {
  return(TRUE);
 }

 while(Len>0)
 {
  for(i=0; i<NumFlags; i++)
  {
   if(Len<FlagLength[i])
   {
    continue;
   }

   if(strncmp(TheFlags[i], DataPtr, FlagLength[i])==0)
   {
    TheDTDesc->DTH.dth_Flags |= FlagValues[i];

    Len-=(FlagLength[i]-1);
    DataPtr+=(FlagLength[i]-1);

    break;
   }
  }

  Len--;
  DataPtr++;
 }

 return(TRUE);
}

int HandlePriority(struct DTDesc *TheDTDesc)
{
 uint8_t *DataPtr;
 unsigned long Pri;

 if(!TheDTDesc)
 {
  return(FALSE);
 }

 TheDTDesc->ReadBuffer[READBUFFERSIZE-1]='\0';

 DataPtr=TheDTDesc->ReadBuffer+KeywordLength[Priority];

 Pri=strtoul(DataPtr, NULL, 10);

 TheDTDesc->DTH.dth_Priority=(uint16_t) Pri;

 return(TRUE);
}

int HandleLine(struct DTDesc *TheDTDesc)
{
 int RetVal;
 int i;

 if(!TheDTDesc)
 {
  return(FALSE);
 }

 RetVal=TRUE;

 for(i=0; i<NumKeywords; i++)
 {
  if(strncmp(TheDTDesc->ReadBuffer, Keywords[i], KeywordLength[i])==0)
  {
   RetVal=KeywordHandler[i](TheDTDesc);

   break;
  }
 }

 return(RetVal);
}

int RemoveNewLine(struct DTDesc *TheDTDesc)
{
 int Len;

 if(!TheDTDesc)
 {
  return(FALSE);
 }

 Len=strlen(TheDTDesc->ReadBuffer);

 if(TheDTDesc->ReadBuffer[Len-1]=='\n')
 {
  TheDTDesc->ReadBuffer[Len-1]='\0';
 }

 return(TRUE);
}

void Work(struct DTDesc *TheDTDesc)
{

 if(!TheDTDesc)
 {
  return;
 }

 memset(TheDTDesc->ReadBuffer, '\0', READBUFFERSIZE);

 while(fgets(TheDTDesc->ReadBuffer, READBUFFERSIZE, TheDTDesc->Input))
 {
  RemoveNewLine(TheDTDesc);

  if(!HandleLine(TheDTDesc))
  {
   break;
  }

  memset(TheDTDesc->ReadBuffer, '\0', READBUFFERSIZE);
 }

 WriteOutDTD(TheDTDesc);

}

int WriteOutDTD(struct DTDesc *TheDTDesc)
{
 struct IFFHandle *IH;
 struct FileDataTypeHeader FileDTH;
 int i;

 if(!TheDTDesc)
 {
  return(FALSE);
 }

 if(strlen(TheDTDesc->Name)==0)
 {
  return(FALSE);
 }

 if(strlen(TheDTDesc->BaseName)==0)
 {
  return(FALSE);
 }

#if 0
 if(TheDTDesc->DTH.dth_MaskLen==0)
 {
  return(FALSE);
 }
#endif

 if(strlen(TheDTDesc->Pattern)==0)
 {
  TheDTDesc->Pattern[0]='#';
  TheDTDesc->Pattern[1]='?';
  TheDTDesc->Pattern[2]='\0';
 }

 IH=NewIFF(TheDTDesc->OutputName, MAKE_ID('D','T','Y','P'));
 if(!IH)
 {
  return(FALSE);
 }

 if(!NewChunk(IH, MAKE_ID('N','A','M','E')))
 {
  CloseIFF(IH);
  remove(TheDTDesc->Name);
  return(FALSE);
 }

 if(WriteChunkData(IH, TheDTDesc->Name, (strlen(TheDTDesc->Name)+1))<=0)
 {
  EndChunk(IH);
  CloseIFF(IH);
  remove(TheDTDesc->Name);
  return(FALSE);
 }

 EndChunk(IH);

 if(strlen(TheDTDesc->Version) > 0)
 {
  if(!NewChunk(IH, MAKE_ID('F','V','E','R')))
  {
   CloseIFF(IH);
   remove(TheDTDesc->Name);
   return(FALSE);
  }

  if(WriteChunkData(IH, TheDTDesc->Version, (strlen(TheDTDesc->Version)+1))<=0)
  {
   EndChunk(IH);
   CloseIFF(IH);
   remove(TheDTDesc->Name);
   return(FALSE);
  }

  EndChunk(IH);
 }

 if(!NewChunk(IH, MAKE_ID('D','T','H','D')))
 {
  CloseIFF(IH);
  remove(TheDTDesc->Name);
  return(FALSE);
 }

 //ASSERT((sizeof(struct FileDataTypeHeader) & 1) == 0);
 if (TheDTDesc->DTH.dth_MaskLen > 0)
 {
  // Write the mask directly after the header to preserve its word alignment.
  FileDTH.dth_Mask     = (((unsigned int) sizeof(struct FileDataTypeHeader)));
  FileDTH.dth_Name     = (((unsigned int) FileDTH.dth_Mask) + (TheDTDesc->DTH.dth_MaskLen << 1));
  FileDTH.dth_Mask     = Swap32IfLE(((uint32_t) FileDTH.dth_Mask));
 }
 else
 {
  FileDTH.dth_Mask     = 0;
  FileDTH.dth_Name     = (((unsigned int) sizeof(struct FileDataTypeHeader)));
 }
 FileDTH.dth_BaseName = (((unsigned int) FileDTH.dth_Name) + strlen(TheDTDesc->DTH.dth_Name) + 1);
 FileDTH.dth_Pattern  = (((unsigned int) FileDTH.dth_BaseName) + strlen(TheDTDesc->DTH.dth_BaseName) + 1);
 FileDTH.dth_GroupID  = TheDTDesc->DTH.dth_GroupID;
 FileDTH.dth_ID       = TheDTDesc->DTH.dth_ID;
 FileDTH.dth_MaskLen  = TheDTDesc->DTH.dth_MaskLen;
 FileDTH.dth_Pad      = TheDTDesc->DTH.dth_Pad;
 FileDTH.dth_Flags    = TheDTDesc->DTH.dth_Flags;
 FileDTH.dth_Priority = TheDTDesc->DTH.dth_Priority;

 FileDTH.dth_Name     = Swap32IfLE(((uint32_t) FileDTH.dth_Name));
 FileDTH.dth_BaseName = Swap32IfLE(((uint32_t) FileDTH.dth_BaseName));
 FileDTH.dth_Pattern  = Swap32IfLE(((uint32_t) FileDTH.dth_Pattern));
 FileDTH.dth_GroupID  = Swap32IfLE(FileDTH.dth_GroupID);
 FileDTH.dth_ID       = Swap32IfLE(FileDTH.dth_ID);
 FileDTH.dth_MaskLen  = Swap16IfLE(FileDTH.dth_MaskLen);
 FileDTH.dth_Pad      = Swap16IfLE(FileDTH.dth_Pad);
 FileDTH.dth_Flags    = Swap16IfLE(FileDTH.dth_Flags);
 FileDTH.dth_Priority = Swap16IfLE(FileDTH.dth_Priority);

 if(WriteChunkData(IH, (char *) &FileDTH, sizeof(struct FileDataTypeHeader))<=0)
 {
  EndChunk(IH);
  CloseIFF(IH);
  remove(TheDTDesc->Name);
  return(FALSE);
 }

 if (TheDTDesc->DTH.dth_MaskLen)
 {
  for(i=0; i<TheDTDesc->DTH.dth_MaskLen; i++)
  {
   TheDTDesc->DTH.dth_Mask[i]=Swap16IfLE(TheDTDesc->DTH.dth_Mask[i]);
  }

  if(WriteChunkData(IH, (char *) TheDTDesc->DTH.dth_Mask, TheDTDesc->DTH.dth_MaskLen << 1)<=0)
  {
   EndChunk(IH);
   CloseIFF(IH);
   remove(TheDTDesc->Name);
   return(FALSE);
  }
 }

 if(WriteChunkData(IH, TheDTDesc->DTH.dth_Name, (strlen(TheDTDesc->DTH.dth_Name) + 1))<=0)
 {
  EndChunk(IH);
  CloseIFF(IH);
  remove(TheDTDesc->Name);
  return(FALSE);
 }

 if(WriteChunkData(IH, TheDTDesc->DTH.dth_BaseName, (strlen(TheDTDesc->DTH.dth_BaseName) + 1))<=0)
 {
  EndChunk(IH);
  CloseIFF(IH);
  remove(TheDTDesc->Name);
  return(FALSE);
 }

 if(WriteChunkData(IH, TheDTDesc->DTH.dth_Pattern, (strlen(TheDTDesc->DTH.dth_Pattern) + 1))<=0)
 {
  EndChunk(IH);
  CloseIFF(IH);
  remove(TheDTDesc->Name);
  return(FALSE);
 }
 
 EndChunk(IH);

 CloseIFF(IH);

 return(TRUE);
}

int Init(int argc, char **argv, struct DTDesc **TheDTDesc)
{
 struct DTDesc *NewDTDesc;

 if(!(argv && TheDTDesc))
 {
  return(FALSE);
 }

 *TheDTDesc=NULL;

 NewDTDesc=(struct DTDesc *) malloc(sizeof(struct DTDesc));
 if(!NewDTDesc)
 {
  return(FALSE);
 }

 *TheDTDesc=NewDTDesc;

 memset(NewDTDesc, '\0', sizeof(struct DTDesc));

 NewDTDesc->ProgName=argv[0];
 NewDTDesc->Input=stdin;
 NewDTDesc->OutputName=NewDTDesc->Name;

 if(!ParseArgs(argc, argv, NewDTDesc))
 {
  return(FALSE);
 }

 if(!OpenInput(NewDTDesc))
 {
  return(FALSE);
 }

 return(TRUE);
}

int OpenInput(struct DTDesc *TheDTDesc)
{

 if(!TheDTDesc)
 {
  return(FALSE);
 }

 if(TheDTDesc->InputName)
 {
  TheDTDesc->Input=fopen(TheDTDesc->InputName, "r");
  if(!TheDTDesc->Input)
  {
   TheDTDesc->Input=stdin;

   return(FALSE);
  }
 }

 return(TRUE);
}

int ParseArgs(int argc, char **argv, struct DTDesc *TheDTDesc)
{
 int i;

 if(!(argv && TheDTDesc))
 {
  return(FALSE);
 }

 for(i=1; i<argc; i++)
 {
  if(strcmp(argv[i], "-o") == 0)
  {
   if(++i >= argc)
   {
    Usage(TheDTDesc->ProgName);

    return(FALSE);
   }

   TheDTDesc->OutputName=argv[i];
  }
  else
  {
   if(strcmp(argv[i], "-h") == 0)
   {
    Usage(TheDTDesc->ProgName);

    return(FALSE);
   }
   else
   {
    TheDTDesc->InputName=argv[i];
   }
  }
 }

 return(TRUE);
}

void Usage(char *ProgName)
{
 char DefaultName[]="createdtdesc";
 char *NamePtr;

 NamePtr = ProgName ? ProgName : DefaultName;

 fprintf(stderr, "\n"
		 "usage: %s [-o <Output-Name>] <Input-Name>\n"
		 "\n",
	NamePtr);
}

void Cleanup(struct DTDesc *TheDTDesc)
{
 if(TheDTDesc)
 {
  if((TheDTDesc->Input!=NULL) && (TheDTDesc->Input!=stdin))
  {
   fclose(TheDTDesc->Input);
  }

  free((void *) TheDTDesc);
 }
}


