struct FFSFileHeader
  {
  LONG  Type;
  ULONG OwnKey;
  ULONG BlocksInTable;
  ULONG Reserved1;
  ULONG FirstDataBlock;
  ULONG Checksum;
  ULONG DataBlocks [72];
  ULONG Reserved2 [2];
  ULONG ProtBits;
  ULONG Size;
  ULONG Comment [23];
  ULONG CreatedDays;
  ULONG CreatedMins;
  ULONG CreatedTicks;
  ULONG Name [8];
  ULONG Reserved3 [2];
  ULONG HardLinkBack;
  ULONG Reserved4 [5];
  ULONG HashChainPtr;
  ULONG ParentKey;
  ULONG ExtBlock;
  LONG  SecondaryType;
  };

struct FFSExtBlock
  {
  LONG  Type;
  ULONG OwnKey;
  ULONG BlocksInTable;
  ULONG Reserved1 [2];
  ULONG Checksum;
  ULONG DataBlocks [72];
  ULONG Reserved2 [47];
  ULONG HeaderKey;
  ULONG ExtBlock;
  LONG  SecondaryType;
  };

struct FFSDirBlock
  {
  ULONG  Type;
  ULONG  OwnKey;
  ULONG  SeqNum;
  ULONG  DataSize;
  ULONG  NextBlock;
  ULONG  Checksum;
  ULONG  HashTable[72];
  ULONG  Reserved;
  ULONG  OwnerXID;
  ULONG  ProtBits;
  ULONG  Junk;
  char   Comment[92];
  ULONG  Days;
  ULONG  Mins;
  ULONG  Ticks;
  char   DirName[36];
  ULONG  HardLink;
  ULONG  HardLinkBack;
  ULONG  Reserved2 [5];
  ULONG  HashChainPtr;
  ULONG  ParentKey;
  ULONG  DirList;        /* These are used by DC-FFS */
  ULONG  SecondaryType;
  };

struct FFSRootBlock
  {
  LONG  Type;
  ULONG Reserved1 [2];
  ULONG HashTableSize;
  ULONG Reserved2;
  ULONG Checksum;
  ULONG HashEntries [72];
  LONG  BitmapValid;
  ULONG BitmapBlocks [25];
  ULONG BitmapExtBlock;
  ULONG RootModifiedDays;
  ULONG RootModifiedMins;
  ULONG RootModifiedTicks;
  ULONG VolumeName [8];
  ULONG Reserved3 [2];
  ULONG VolumeModifiedDays;
  ULONG VolumeModifiedMins;
  ULONG VolumeModifiedTicks;
  ULONG VolumeCreatedDays;
  ULONG VolumeCreatedMins;
  ULONG VolumeCreatedTicks;
  ULONG Reserved4 [3];
  LONG  SecondaryType;
  };

struct FFSBitmapBlock
  {
  ULONG Checksum;
  ULONG BitmapData [127];
  };

struct FFSBitmapExtBlock
  {
  ULONG BitmapBlocks [127];
  ULONG NextBitmapExtBlock;
  };

struct DCFFSCacheBlock
  {
  ULONG  Type;
  ULONG  OwnKey;
  ULONG  Parent;
  ULONG  NumEntries;
  ULONG  NextBlock;
  ULONG  Checksum;
  };

struct ListEntry
  {
  ULONG Key;
  ULONG Size;
  ULONG Protection;
  ULONG OwnerXID;
  UWORD Days;
  UWORD Min;
  UWORD Tick;
  UBYTE Type;
  UBYTE FileNameLength;
  };

#define T_SHORT   0x0002
#define T_LIST    0x0010
#define T_DIRLIST 0x0021
