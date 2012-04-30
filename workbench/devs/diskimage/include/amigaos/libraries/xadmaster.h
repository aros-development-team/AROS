#ifndef LIBRARIES_XADMASTER_H
#define LIBRARIES_XADMASTER_H

/*
**	$VER: xadmaster.h 12.1 (02.01.2003)
**	xadmaster.library defines and structures
**
**	Copyright © 1998-2002 by Dirk Stöcker
**	All Rights Reserved.
*/

#ifndef EXEC_LIBRARIES_H
#include <exec/libraries.h>
#endif

#ifndef UTILITY_TAGITEM_H
#include <utility/tagitem.h>
#endif

#define  XADNAME  "xadmaster.library"

/* NOTE: Nearly all structures need to be allocated using the
   xadAllocObject function. */

/************************************************************************
*									*
*    library base structure						*
*									*
************************************************************************/

struct xadMasterBase {
  struct Library       xmb_LibNode;
  struct ExecBase *    xmb_SysBase;
  struct DosLibrary *  xmb_DOSBase;
  struct UtilityBase * xmb_UtilityBase;
  ULONG 	       xmb_RecogSize;	/* read only */
  STRPTR	       xmb_DefaultName; /* name for XADFIF_NOFILENAME (V6) */
};

/************************************************************************
*									*
*    tag-function call flags						*
*									*
************************************************************************/

/* input tags for xadGetInfo, only one can be specified per call */
#define XAD_INSIZE		(TAG_USER+  1) /* input data size */
#define XAD_INFILENAME		(TAG_USER+  2)
#define XAD_INFILEHANDLE	(TAG_USER+  3)
#define XAD_INMEMORY		(TAG_USER+  4)
#define XAD_INHOOK		(TAG_USER+  5)
#define XAD_INSPLITTED		(TAG_USER+  6) /* (V2) */
#define XAD_INDISKARCHIVE	(TAG_USER+  7) /* (V4) */
#define XAD_INXADSTREAM 	(TAG_USER+  8) /* (V8) */
#define XAD_INDEVICE		(TAG_USER+  9) /* (V11) */

/* output tags, only one can be specified per call, xadXXXXUnArc */
#define XAD_OUTSIZE		(TAG_USER+ 10) /* output data size */
#define XAD_OUTFILENAME 	(TAG_USER+ 11)
#define XAD_OUTFILEHANDLE	(TAG_USER+ 12)
#define XAD_OUTMEMORY		(TAG_USER+ 13)
#define XAD_OUTHOOK		(TAG_USER+ 14)
#define XAD_OUTDEVICE		(TAG_USER+ 15)
#define XAD_OUTXADSTREAM	(TAG_USER+ 16) /* (V8) */

/* object allocation tags for xadAllocObjectA */
#define XAD_OBJNAMESIZE 	(TAG_USER+ 20) /* XADOBJ_FILEINFO, size of needed name space */
#define XAD_OBJCOMMENTSIZE	(TAG_USER+ 21) /* XADOBJ_FILEINFO, size of needed comment space */
#define XAD_OBJPRIVINFOSIZE	(TAG_USER+ 22) /* XADOBJ_FILEINFO & XADOBJ_DISKINFO, self use size */
#define XAD_OBJBLOCKENTRIES	(TAG_USER+ 23) /* XADOBJ_DISKINFO, number of needed entries */

/* tags for xadGetInfo, xadFileUnArc and xadDiskUnArc */
#define XAD_NOEXTERN		(TAG_USER+ 50) /* do not use extern clients */
#define XAD_PASSWORD		(TAG_USER+ 51) /* password when needed */
#define XAD_ENTRYNUMBER 	(TAG_USER+ 52) /* number of wanted entry */
#define XAD_PROGRESSHOOK	(TAG_USER+ 53) /* the progress hook */
#define XAD_OVERWRITE		(TAG_USER+ 54) /* overwrite file ? */
#define XAD_MAKEDIRECTORY	(TAG_USER+ 55) /* create directory tree */
#define XAD_IGNOREGEOMETRY	(TAG_USER+ 56) /* ignore drive geometry ? */
#define XAD_LOWCYLINDER 	(TAG_USER+ 57) /* lowest cylinder */
#define XAD_HIGHCYLINDER	(TAG_USER+ 58) /* highest cylinder */
#define XAD_VERIFY		(TAG_USER+ 59) /* verify for disk hook */
#define XAD_NOKILLPARTIAL	(TAG_USER+ 60) /* do not delete partial/corrupt files (V3.3) */
#define XAD_FORMAT		(TAG_USER+ 61) /* format output device (V5) */
#define XAD_USESECTORLABELS	(TAG_USER+ 62) /* sector labels are stored on disk (V9) */
#define XAD_IGNOREFLAGS 	(TAG_USER+ 63) /* ignore the client, if certain flags are set (V11) */
#define XAD_ONLYFLAGS		(TAG_USER+ 64) /* ignore the client, if certain flags are NOT set (V11) */

/* input tags for xadConvertDates, only one can be passed */
#define XAD_DATEUNIX		(TAG_USER+ 70) /* unix date variable */
#define XAD_DATEAMIGA		(TAG_USER+ 71) /* amiga date variable */
#define XAD_DATEDATESTAMP	(TAG_USER+ 72) /* struct DateStamp */
#define XAD_DATEXADDATE 	(TAG_USER+ 73) /* struct xadDate */
#define XAD_DATECLOCKDATA	(TAG_USER+ 74) /* struct ClockData */
#define XAD_DATECURRENTTIME	(TAG_USER+ 75) /* input is system time */
#define XAD_DATEMSDOS		(TAG_USER+ 76) /* MS-DOS packed format (V2) */
#define XAD_DATEMAC		(TAG_USER+ 77) /* Mac date variable (V8) */
#define XAD_DATECPM		(TAG_USER+ 78) /* CP/M data structure (V10) */
#define XAD_DATECPM2		(TAG_USER+ 79) /* CP/M data structure type 2 (V10) */
#define XAD_DATEISO9660 	(TAG_USER+300) /* ISO9660 date structure (V11) */

/* output tags, there can be specified multiple tags for one call */
#define XAD_GETDATEUNIX 	(TAG_USER+ 80) /* unix date variable */
#define XAD_GETDATEAMIGA	(TAG_USER+ 81) /* amiga date variable */
#define XAD_GETDATEDATESTAMP	(TAG_USER+ 82) /* struct DateStamp */
#define XAD_GETDATEXADDATE	(TAG_USER+ 83) /* struct xadDate */
#define XAD_GETDATECLOCKDATA	(TAG_USER+ 84) /* struct ClockData */
#define XAD_GETDATEMSDOS	(TAG_USER+ 86) /* MS-DOS packed format (V2) */
#define XAD_GETDATEMAC		(TAG_USER+ 87) /* Mac date variable (V8) */
#define XAD_GETDATECPM		(TAG_USER+ 88) /* CP/M data structure (V10) */
#define XAD_GETDATECPM2 	(TAG_USER+ 89) /* CP/M data structure type 2 (V10) */
#define XAD_GETDATEISO9660	(TAG_USER+320) /* ISO9660 date structure (V11) */

/* following tags need locale.library to be installed */
#define XAD_MAKEGMTDATE 	(TAG_USER+ 90) /* make local to GMT time */
#define XAD_MAKELOCALDATE	(TAG_USER+ 91) /* make GMT to local time */

/* tags for xadHookTagAccess (V3) */
#define XAD_USESKIPINFO 	(TAG_USER+104) /* the hook uses xadSkipInfo (V3) */
#define XAD_SECTORLABELS	(TAG_USER+105) /* pass sector labels with XADAC_WRITE (V9) */

#define XAD_GETCRC16		(TAG_USER+120) /* pointer to UWORD value (V3) */
#define XAD_GETCRC32		(TAG_USER+121) /* pointer to ULONG value (V3) */

#define XAD_CRC16ID		(TAG_USER+130) /* ID for crc calculation (V3) */
#define XAD_CRC32ID		(TAG_USER+131) /* ID for crc calculation (V3) */

/* tags for xadConvertProtection (V4) */
#define XAD_PROTAMIGA		(TAG_USER+160) /* Amiga type protection bits (V4) */
#define XAD_PROTUNIX		(TAG_USER+161) /* protection bits in UNIX mode (V4) */
#define XAD_PROTMSDOS		(TAG_USER+162) /* MSDOS type protection bits (V4) */
#define XAD_PROTFILEINFO	(TAG_USER+163) /* input is a xadFileInfo structure (V11) */

#define XAD_GETPROTAMIGA	(TAG_USER+170) /* return Amiga protection bits (V4) */
#define XAD_GETPROTUNIX 	(TAG_USER+171) /* return UNIX protection bits (V11) */
#define XAD_GETPROTMSDOS	(TAG_USER+172) /* return MSDOS protection bits (V11) */
#define XAD_GETPROTFILEINFO	(TAG_USER+173) /* fill xadFileInfo protection fields (V11) */

/* tags for xadGetDiskInfo (V7) */
#define XAD_STARTCLIENT 	(TAG_USER+180) /* the client to start with (V7) */
#define XAD_NOEMPTYERROR	(TAG_USER+181) /* do not create XADERR_EMPTY (V8) */

/* tags for xadFreeHookAccess (V8) */
#define XAD_WASERROR		(TAG_USER+190) /* error occured, call abort method (V8) */

/* tags for miscellaneous stuff */
#define XAD_ARCHIVEINFO 	(TAG_USER+200) /* xadArchiveInfo for stream hooks (V8) */
#define XAD_ERRORCODE		(TAG_USER+201) /* error code of function (V12) */

/* tags for xadAddFileEntry and xadAddDiskEntry (V10) */
#define XAD_SETINPOS		(TAG_USER+240) /* set xai_InPos after call (V10) */
#define XAD_INSERTDIRSFIRST	(TAG_USER+241) /* insert dirs at list start (V10) */

/* tags for xadConvertName (V12) */
#define XAD_PATHSEPERATOR	(TAG_USER+260) /* UWORD *, default is {'/','\\',0} in source charset (V12) */
#define XAD_CHARACTERSET	(TAG_USER+261) /* the characterset of string (V12) */
#define XAD_STRINGSIZE		(TAG_USER+262) /* maximum size of following (V12) */
#define XAD_CSTRING		(TAG_USER+263) /* zero-terminated string (V12) */
#define XAD_PSTRING		(TAG_USER+264) /* lengthed Pascal string (V12) */
#define XAD_XADSTRING		(TAG_USER+265) /* an xad string (V12) */
#define XAD_ADDPATHSEPERATOR	(TAG_USER+266) /* default is TRUE (V12) */

/* tags for xadGetFilename (V12) */
#define XAD_NOLEADINGPATH	(TAG_USER+280) /* default is FALSE (V12) */
#define XAD_NOTRAILINGPATH	(TAG_USER+281) /* default is FALSE (V12) */
#define XAD_MASKCHARACTERS	(TAG_USER+282) /* default are #?()[]~%*:|",1-31,127-160 (V12) */
#define XAD_MASKINGCHAR 	(TAG_USER+283) /* default is '_' (V12) */
#define XAD_REQUIREDBUFFERSIZE	(TAG_USER+284) /* pointer which should hold buf size (V12) */


/* Places 300-339 used for dates! */

/************************************************************************
*									*
*    objects for xadAllocObjectA					*
*									*
************************************************************************/

#define XADOBJ_ARCHIVEINFO	0x0001 /* struct xadArchiveInfo */
#define XADOBJ_FILEINFO 	0x0002 /* struct xadFileInfo */
#define XADOBJ_DISKINFO 	0x0003 /* struct xadDiskInfo */
#define XADOBJ_HOOKPARAM	0x0004 /* struct HookParam */
#define XADOBJ_DEVICEINFO	0x0005 /* struct xadDeviceInfo */
#define XADOBJ_PROGRESSINFO	0x0006 /* struct xadProgressInfo */
#define XADOBJ_TEXTINFO 	0x0007 /* struct xadTextInfo */
#define XADOBJ_SPLITFILE	0x0008 /* struct xadSplitFile (V2) */
#define XADOBJ_SKIPINFO 	0x0009 /* struct xadSkipInfo (V3) */
#define XADOBJ_IMAGEINFO	0x000A /* struct xadImageInfo (V4) */
#define XADOBJ_SPECIAL		0x000B /* struct xadSpecial (V11) */

/* result type of xadAllocVec */
#define XADOBJ_MEMBLOCK 	0x0100 /* memory of requested size and type */
/* private type */
#define XADOBJ_STRING		0x0101 /* an typed XAD string (V12) */

/************************************************************************
*									*
*    modes for xadCalcCRC126 and xadCalcCRC32				*
*									*
************************************************************************/

#define XADCRC16_ID1		0xA001
#define XADCRC32_ID1		0xEDB88320

/************************************************************************
*									*
*    hook related stuff 						*
*									*
************************************************************************/

#define XADHC_READ	1	/* read data into buffer */
#define XADHC_WRITE	2	/* write buffer data to file/memory */
#define XADHC_SEEK	3	/* seek in file */
#define XADHC_INIT	4	/* initialize the hook */
#define XADHC_FREE	5	/* end up hook work, free stuff */
#define XADHC_ABORT	6	/* an error occured, delete partial stuff */
#define XADHC_FULLSIZE	7	/* complete input size is needed */
#define XADHC_IMAGEINFO 8	/* return disk image info (V4) */

struct xadHookParam {
  ULONG xhp_Command;
  LONG	xhp_CommandData;
  APTR	xhp_BufferPtr;
  ULONG xhp_BufferSize;
  ULONG xhp_DataPos;		/* current seek position */
  APTR	xhp_PrivatePtr;
  APTR	xhp_TagList;		/* allows to transport tags to hook (V9) */
};

/* xadHookAccess commands */
#define XADAC_READ		10	/* get data */
#define XADAC_WRITE		11	/* write data */
#define XADAC_COPY		12	/* copy input to output */
#define XADAC_INPUTSEEK 	13	/* seek in input file */
#define XADAC_OUTPUTSEEK	14	/* seek in output file */

/************************************************************************
*									*
*    support structures 						*
*									*
************************************************************************/

/* Own date structure to cover all possible dates in a human friendly
   format. xadConvertDates may be used to convert between different date
   structures and variables. */
struct xadDate {
  ULONG xd_Micros;	/* values 0 to 999999	  */
  LONG	xd_Year;	/* values 1 to 2147483648 */
  UBYTE xd_Month;	/* values 1 to 12	  */
  UBYTE xd_WeekDay;	/* values 1 to 7	  */
  UBYTE xd_Day; 	/* values 1 to 31	  */
  UBYTE xd_Hour;	/* values 0 to 23	  */
  UBYTE xd_Minute;	/* values 0 to 59	  */
  UBYTE xd_Second;	/* values 0 to 59	  */
};

#define XADDAY_MONDAY		1	/* monday is the first day and */
#define XADDAY_TUESDAY		2
#define XADDAY_WEDNESDAY	3
#define XADDAY_THURSDAY 	4
#define XADDAY_FRIDAY		5
#define XADDAY_SATURDAY 	6
#define XADDAY_SUNDAY		7	/* sunday the last day of a week */

struct xadDeviceInfo { /* for XAD_OUTDEVICE tag */
  STRPTR xdi_DeviceName; /* name of device */
  ULONG  xdi_Unit;	 /* unit of device */
  STRPTR xdi_DOSName;	 /* instead of Device+Unit, dos name without ':' */
};

struct xadSplitFile { /* for XAD_INSPLITTED */
  struct xadSplitFile *xsf_Next;
  ULONG 	       xsf_Type; /* XAD_INFILENAME, XAD_INFILEHANDLE, XAD_INMEMORY, XAD_INHOOK */
  ULONG 	       xsf_Size; /* necessary for XAD_INMEMORY, useful for others */
  ULONG 	       xsf_Data; /* FileName, Filehandle, Hookpointer or Memory */
};

struct xadSkipInfo {
  struct xadSkipInfo *xsi_Next;
  ULONG 	      xsi_Position; /* position, where it should be skipped */
  ULONG 	      xsi_SkipSize; /* size to skip */
};

struct xadImageInfo { /* for XADHC_IMAGEINFO */
  ULONG xii_SectorSize;   /* usually 512 */
  ULONG xii_FirstSector;  /* of the image file */
  ULONG xii_NumSectors;   /* of the image file */
  ULONG xii_TotalSectors; /* of this device type */
};
/* If the image file holds total data of disk xii_TotalSectors equals
   xii_NumSectors and xii_FirstSector is zero. Addition of xii_FirstSector
   and xii_NumSectors cannot exceed xii_TotalSectors value!
*/

/************************************************************************
*									*
*    information structures						*
*									*
************************************************************************/

struct xadArchiveInfo {
  struct xadClient *   xai_Client;   /* pointer to unarchiving client */
  APTR		       xai_PrivateClient; /* private client data */
  STRPTR	       xai_Password; /* password for crypted archives */
  ULONG 	       xai_Flags;    /* read only XADAIF_ flags */
  ULONG 	       xai_LowCyl;   /* lowest cylinder to unarchive */
  ULONG 	       xai_HighCyl;  /* highest cylinder to unarchive */
  ULONG 	       xai_InPos;    /* input position, read only */
  ULONG 	       xai_InSize;   /* input size, read only */
  ULONG 	       xai_OutPos;   /* output position, read only */
  ULONG 	       xai_OutSize;  /* output file size, read only */
  struct xadFileInfo * xai_FileInfo; /* data pointer for file arcs */
  struct xadDiskInfo * xai_DiskInfo; /* data pointer for disk arcs */
  struct xadFileInfo * xai_CurFile;  /* data pointer for current file arc */
  struct xadDiskInfo * xai_CurDisk;  /* data pointer for current disk arc */
  LONG		       xai_LastError;	/* last error, when XADAIF_FILECORRUPT (V2) */
  ULONG *	       xai_MultiVolume; /* array of start offsets from parts (V2) */
  struct xadSkipInfo * xai_SkipInfo;	/* linked list of skip entries (V3) */
  struct xadImageInfo *xai_ImageInfo;	/* for filesystem clients (V5) */
  STRPTR	       xai_InName;   /* Input archive name if available (V7) */
};
/* This structure is nearly complete private to either xadmaster or its
clients. An application program may access for reading only xai_Client,
xai_Flags, xai_FileInfo and xai_DiskInfo. For xai_Flags only XADAIF_CRYPTED
and XADAIF_FILECORRUPT are useful. All the other stuff is private and should
not be accessed! */

#define XADAIB_CRYPTED		 0 /* archive entries are encrypted */
#define XADAIB_FILECORRUPT	 1 /* file is corrupt, but valid entries are in the list */
#define XADAIB_FILEARCHIVE	 2 /* unarchive file entry */
#define XADAIB_DISKARCHIVE	 3 /* unarchive disk entry */
#define XADAIB_OVERWRITE	 4 /* overwrite the file (PRIVATE) */
#define XADAIB_MAKEDIRECTORY	 5 /* create directory when missing (PRIVATE) */
#define XADAIB_IGNOREGEOMETRY	 6 /* ignore drive geometry (PRIVATE) */
#define XADAIB_VERIFY		 7 /* verify is turned on for disk hook (PRIVATE) */
#define XADAIB_NOKILLPARTIAL	 8 /* do not delete partial files (PRIVATE) */
#define XADAIB_DISKIMAGE	 9 /* is disk image extraction (V5) */
#define XADAIB_FORMAT		10 /* format in disk hook (PRIVATE) */
#define XADAIB_NOEMPTYERROR	11 /* do not create empty error (PRIVATE) */
#define XADAIB_ONLYIN		12 /* in stuff only (PRIVATE) */
#define XADAIB_ONLYOUT		13 /* out stuff only (PRIVATE) */
#define XADAIB_USESECTORLABELS	14 /* use SectorLabels (PRIVATE) */

#define XADAIF_CRYPTED		(1<<XADAIB_CRYPTED)
#define XADAIF_FILECORRUPT	(1<<XADAIB_FILECORRUPT)
#define XADAIF_FILEARCHIVE	(1<<XADAIB_FILEARCHIVE)
#define XADAIF_DISKARCHIVE	(1<<XADAIB_DISKARCHIVE)
#define XADAIF_OVERWRITE	(1<<XADAIB_OVERWRITE)
#define XADAIF_MAKEDIRECTORY	(1<<XADAIB_MAKEDIRECTORY)
#define XADAIF_IGNOREGEOMETRY	(1<<XADAIB_IGNOREGEOMETRY)
#define XADAIF_VERIFY		(1<<XADAIB_VERIFY)
#define XADAIF_NOKILLPARTIAL	(1<<XADAIB_NOKILLPARTIAL)
#define XADAIF_DISKIMAGE	(1<<XADAIB_DISKIMAGE)
#define XADAIF_FORMAT		(1<<XADAIB_FORMAT)
#define XADAIF_NOEMPTYERROR	(1<<XADAIB_NOEMPTYERROR)
#define XADAIF_ONLYIN		(1<<XADAIB_ONLYIN)
#define XADAIF_ONLYOUT		(1<<XADAIB_ONLYOUT)
#define XADAIF_USESECTORLABELS	(1<<XADAIB_USESECTORLABELS)

struct xadFileInfo {
  struct xadFileInfo * xfi_Next;
  ULONG 	       xfi_EntryNumber;/* number of entry */
  STRPTR	       xfi_EntryInfo;  /* additional archiver text */
  APTR		       xfi_PrivateInfo;/* client private, see XAD_OBJPRIVINFOSIZE */
  ULONG 	       xfi_Flags;      /* see XADFIF_xxx defines */
  STRPTR	       xfi_FileName;   /* see XAD_OBJNAMESIZE tag */
  STRPTR	       xfi_Comment;    /* see XAD_OBJCOMMENTSIZE tag */
  ULONG 	       xfi_Protection; /* OS 3 bits (including multiuser) */
  ULONG 	       xfi_OwnerUID;   /* user ID */
  ULONG 	       xfi_OwnerGID;   /* group ID */
  STRPTR	       xfi_UserName;   /* user name */
  STRPTR	       xfi_GroupName;  /* group name */
  ULONG 	       xfi_Size;       /* size of this file */
  ULONG 	       xfi_GroupCrSize;/* crunched size of group */
  ULONG 	       xfi_CrunchSize; /* crunched size */
  STRPTR	       xfi_LinkName;   /* name and path of link */
  struct xadDate       xfi_Date;
  UWORD 	       xfi_Generation; /* File Generation [0...0xFFFF] (V3) */
  ULONG 	       xfi_DataPos;    /* crunched data position (V3) */
  struct xadFileInfo * xfi_MacFork;    /* pointer to 2nd fork for Mac (V7) */
  UWORD 	       xfi_UnixProtect;/* protection bits for Unix (V11) */
  UBYTE 	       xfi_DosProtect; /* protection bits for MS-DOS (V11) */
  UBYTE 	       xfi_FileType;   /* XADFILETYPE to define type of exe files (V11) */
  struct xadSpecial *  xfi_Special;    /* pointer to special data (V11) */
};

/* These are used for xfi_FileType to define file type. (V11) */
#define XADFILETYPE_DATACRUNCHER     1	 /* infile was only one data file */
#define XADFILETYPE_TEXTLINKER	     2	 /* infile was text-linked */

#define XADFILETYPE_AMIGAEXECRUNCHER 11  /* infile was an Amiga exe cruncher */
#define XADFILETYPE_AMIGAEXELINKER   12  /* infile was an Amiga exe linker */
#define XADFILETYPE_AMIGATEXTLINKER  13  /* infile was an Amiga text-exe linker */
#define XADFILETYPE_AMIGAADDRESS     14  /* infile was an Amiga address cruncher */

#define XADFILETYPE_UNIXBLOCKDEVICE  21  /* this file is a block device */
#define XADFILETYPE_UNIXCHARDEVICE   22  /* this file is a character device */
#define XADFILETYPE_UNIXFIFO	     23  /* this file is a named pipe */
#define XADFILETYPE_UNIXSOCKET	     24  /* this file is a socket */

#define XADFILETYPE_MSDOSEXECRUNCHER 31  /* infile was an MSDOS exe cruncher */

#define XADSPECIALTYPE_UNIXDEVICE    1 /* xadSpecial entry is xadSpecialUnixDevice */
#define XADSPECIALTYPE_AMIGAADDRESS  2 /* xadSpecial entry is xadSpecialAmigaAddress */
#define XADSPECIALTYPE_CBM8BIT	     3 /* xadSpecial entry is xadSpecialCBM8bit */

struct xadSpecialUnixDevice
{
  ULONG xfis_MajorVersion;    /* major device version */
  ULONG xfis_MinorVersion;    /* minor device version */
};

struct xadSpecialAmigaAddress
{
  ULONG xfis_JumpAddress;     /* code executaion start address */
  ULONG xfis_DecrunchAddress; /* decrunch start of code */
};

struct xadSpecialCBM8bit
{
  UBYTE xfis_FileType;	      /* File type XADCBM8BITTYPE_xxx */
  UBYTE xfis_RecordLength;    /* record length if relative file */
};
#define XADCBM8BITTYPE_UNKNOWN	0x00	/*	  Unknown / Unused */
#define XADCBM8BITTYPE_BASIC	0x01	/* Tape - BASIC program file */
#define XADCBM8BITTYPE_DATA	0x02	/* Tape - Data block (SEQ file) */
#define XADCBM8BITTYPE_FIXED	0x03	/* Tape - Fixed addres program file */
#define XADCBM8BITTYPE_SEQDATA	0x04	/* Tape - Sequential data file */
#define XADCBM8BITTYPE_SEQ	0x81	/* Disk - Sequential file "SEQ" */
#define XADCBM8BITTYPE_PRG	0x82	/* Disk - Program file "PRG" */
#define XADCBM8BITTYPE_USR	0x83	/* Disk - User-defined file "USR" */
#define XADCBM8BITTYPE_REL	0x84	/* Disk - Relative records file "REL" */
#define XADCBM8BITTYPE_CBM	0x85	/* Disk - CBM (partition) "CBM" */

struct xadSpecial
{
  ULONG 	     xfis_Type; /* XADSPECIALTYPE to define type of block (V11) */
  struct xadSpecial *xfis_Next; /* pointer to next entry */
  union
  {
    struct xadSpecialUnixDevice   xfis_UnixDevice;
    struct xadSpecialAmigaAddress xfis_AmigaAddress;
    struct xadSpecialCBM8bit	  xfis_CBM8bit;
  } xfis_Data;
};

/* Multiuser fields (xfi_OwnerUID, xfi_OwnerUID, xfi_UserName, xfi_GroupName)
   and multiuser bits (see <dos/dos.h>) are currently not supported with normal
   Amiga filesystem. But the clients support them, if archive format holds
   such information.

   The protection bits (all 3 fields) should always be set using the
   xadConvertProtection procedure. Call it with as much protection information
   as possible. It extracts the relevant data at best (and also sets the 2 flags).
   DO NOT USE these fields directly, but always through xadConvertProtection
   call.
*/

#define XADFIB_CRYPTED		0 /* entry is crypted */
#define XADFIB_DIRECTORY	1 /* entry is a directory */
#define XADFIB_LINK		2 /* entry is a link */
#define XADFIB_INFOTEXT 	3 /* file is an information text */
#define XADFIB_GROUPED		4 /* file is in a crunch group */
#define XADFIB_ENDOFGROUP	5 /* crunch group ends here */
#define XADFIB_NODATE		6 /* no date supported, CURRENT date is set */
#define XADFIB_DELETED		7 /* file is marked as deleted (V3) */
#define XADFIB_SEEKDATAPOS	8 /* before unarchiving the datapos is set (V3) */
#define XADFIB_NOFILENAME	9 /* there was no filename, using internal one (V6) */
#define XADFIB_NOUNCRUNCHSIZE  10 /* file size is unknown and thus set to zero (V6) */
#define XADFIB_PARTIALFILE     11 /* file is only partial (V6) */
#define XADFIB_MACDATA	       12 /* file is Apple data fork (V7) */
#define XADFIB_MACRESOURCE     13 /* file is Apple resource fork (V7) */
#define XADFIB_EXTRACTONBUILD  14 /* allows extract file during scanning (V10) */
#define XADFIB_UNIXPROTECTION  15 /* UNIX protection bits are present (V11) */
#define XADFIB_DOSPROTECTION   16 /* MSDOS protection bits are present (V11) */
#define XADFIB_ENTRYMAYCHANGE  17 /* this entry may change until GetInfo is finished (V11) */
#define XADFIB_XADSTRFILENAME  18 /* the xfi_FileName fields is an XAD string (V12) */
#define XADFIB_XADSTRLINKNAME  19 /* the xfi_LinkName fields is an XAD string (V12) */
#define XADFIB_XADSTRCOMMENT   20 /* the xfi_Comment fields is an XAD string (V12) */

#define XADFIF_CRYPTED		(1<<XADFIB_CRYPTED)
#define XADFIF_DIRECTORY	(1<<XADFIB_DIRECTORY)
#define XADFIF_LINK		(1<<XADFIB_LINK)
#define XADFIF_INFOTEXT 	(1<<XADFIB_INFOTEXT)
#define XADFIF_GROUPED		(1<<XADFIB_GROUPED)
#define XADFIF_ENDOFGROUP	(1<<XADFIB_ENDOFGROUP)
#define XADFIF_NODATE		(1<<XADFIB_NODATE)
#define XADFIF_DELETED		(1<<XADFIB_DELETED)
#define XADFIF_SEEKDATAPOS	(1<<XADFIB_SEEKDATAPOS)
#define XADFIF_NOFILENAME	(1<<XADFIB_NOFILENAME)
#define XADFIF_NOUNCRUNCHSIZE	(1<<XADFIB_NOUNCRUNCHSIZE)
#define XADFIF_PARTIALFILE	(1<<XADFIB_PARTIALFILE)
#define XADFIF_MACDATA		(1<<XADFIB_MACDATA)
#define XADFIF_MACRESOURCE	(1<<XADFIB_MACRESOURCE)
#define XADFIF_EXTRACTONBUILD	(1<<XADFIB_EXTRACTONBUILD)
#define XADFIF_UNIXPROTECTION	(1<<XADFIB_UNIXPROTECTION)
#define XADFIF_DOSPROTECTION	(1<<XADFIB_DOSPROTECTION)
#define XADFIF_ENTRYMAYCHANGE	(1<<XADFIB_ENTRYMAYCHANGE)
#define XADFIF_XADSTRFILENAME	(1<<XADFIB_XADSTRFILENAME)
#define XADFIF_XADSTRLINKNAME	(1<<XADFIB_XADSTRLINKNAME)
#define XADFIF_XADSTRCOMMENT	(1<<XADFIB_XADSTRCOMMENT)

/* NOTE: the texts passed with that structure must not always be printable.
   Although the clients should add an additional (not counted) zero at the text
   end, the whole file may contain other unprintable stuff (e.g. for DMS).
   So when printing this texts do it on a byte for byte base including
   printability checks.
*/
struct xadTextInfo {
  struct xadTextInfo *	xti_Next;
  ULONG 		xti_Size;  /* maybe zero - no text - e.g. when crypted */
  STRPTR		xti_Text;  /* and there is no password in xadGetInfo() */
  ULONG 		xti_Flags; /* see XADTIF_xxx defines */
};

#define XADTIB_CRYPTED		0 /* entry is empty, as data was crypted */
#define XADTIB_BANNER		1 /* text is a banner */
#define XADTIB_FILEDIZ		2 /* text is a file description */

#define XADTIF_CRYPTED		(1<<XADTIB_CRYPTED)
#define XADTIF_BANNER		(1<<XADTIB_BANNER)
#define XADTIF_FILEDIZ		(1<<XADTIB_FILEDIZ)

struct xadDiskInfo {
  struct xadDiskInfo *	xdi_Next;
  ULONG 		xdi_EntryNumber;  /* number of entry */
  STRPTR		xdi_EntryInfo;	  /* additional archiver text */
  APTR			xdi_PrivateInfo;  /* client private, see XAD_OBJPRIVINFOSIZE */
  ULONG 		xdi_Flags;	  /* see XADDIF_xxx defines */
  ULONG 		xdi_SectorSize;
  ULONG 		xdi_TotalSectors; /* see devices/trackdisk.h */
  ULONG 		xdi_Cylinders;	  /* to find out what these */
  ULONG 		xdi_CylSectors;   /* fields mean, they are equal */
  ULONG 		xdi_Heads;	  /* to struct DriveGeometry */
  ULONG 		xdi_TrackSectors;
  ULONG 		xdi_LowCyl;	  /* lowest cylinder stored */
  ULONG 		xdi_HighCyl;	  /* highest cylinder stored */
  ULONG 		xdi_BlockInfoSize;/* number of BlockInfo entries */
  UBYTE *		xdi_BlockInfo;	  /* see XADBIF_xxx defines and XAD_OBJBLOCKENTRIES tag */
  struct xadTextInfo *	xdi_TextInfo;	  /* linked list with info texts */
  ULONG 		xdi_DataPos;	  /* crunched data position (V3) */
};

/* BlockInfo points to a UBYTE field for every track from first sector of
   lowest cylinder to last sector of highest cylinder. When not used,
   pointer must be 0. Do not use it, when there are no entries!
   This is just for information. The applications still asks the client
   to unarchive whole cylinders and not archived blocks are cleared for
   unarchiving.
*/

#define XADDIB_CRYPTED		  0 /* entry is crypted */
#define XADDIB_SEEKDATAPOS	  1 /* before unarchiving the datapos is set (V3) */
#define XADDIB_SECTORLABELS	  2 /* the clients delivers sector labels (V9) */
#define XADDIB_EXTRACTONBUILD	  3 /* allows extract disk during scanning (V10) */
#define XADDIB_ENTRYMAYCHANGE	  4 /* this entry may change until GetInfo is finished (V11) */

/* Some of the crunchers do not store all necessary information, so it
may be needed to guess some of them. Set the following flags in that case
and geometry check will ignore these fields. */
#define XADDIB_GUESSSECTORSIZE	  5 /* sectorsize is guessed (V10) */
#define XADDIB_GUESSTOTALSECTORS  6 /* totalsectors number is guessed (V10) */
#define XADDIB_GUESSCYLINDERS	  7 /* cylinder number is guessed */
#define XADDIB_GUESSCYLSECTORS	  8 /* cylsectors is guessed */
#define XADDIB_GUESSHEADS	  9 /* number of heads is guessed */
#define XADDIB_GUESSTRACKSECTORS 10 /* tracksectors is guessed */
#define XADDIB_GUESSLOWCYL	 11 /* lowcyl is guessed */
#define XADDIB_GUESSHIGHCYL	 12 /* highcyl is guessed */

/* If it is impossible to set some of the fields, you need to set some of
these flags. NOTE: XADDIB_NOCYLINDERS is really important, as this turns
of usage of lowcyl and highcyl keywords. When you have cylinder information,
you should not use these and instead use guess flags and calculate
possible values for the missing fields. */
#define XADDIB_NOCYLINDERS	 15 /* cylinder number is not set */
#define XADDIB_NOCYLSECTORS	 16 /* cylsectors is not set */
#define XADDIB_NOHEADS		 17 /* number of heads is not set */
#define XADDIB_NOTRACKSECTORS	 18 /* tracksectors is not set */
#define XADDIB_NOLOWCYL 	 19 /* lowcyl is not set */
#define XADDIB_NOHIGHCYL	 20 /* highcyl is not set */

#define XADDIF_CRYPTED		 (1<<XADDIB_CRYPTED)
#define XADDIF_SEEKDATAPOS	 (1<<XADDIB_SEEKDATAPOS)
#define XADDIF_SECTORLABELS	 (1<<XADDIB_SECTORLABELS)
#define XADDIF_EXTRACTONBUILD	 (1<<XADDIB_EXTRACTONBUILD)
#define XADDIF_ENTRYMAYCHANGE	 (1<<XADDIB_ENTRYMAYCHANGE)

#define XADDIF_GUESSSECTORSIZE	 (1<<XADDIB_GUESSSECTORSIZE)
#define XADDIF_GUESSTOTALSECTORS (1<<XADDIB_GUESSTOTALSECTORS)
#define XADDIF_GUESSCYLINDERS	 (1<<XADDIB_GUESSCYLINDERS)
#define XADDIF_GUESSCYLSECTORS	 (1<<XADDIB_GUESSCYLSECTORS)
#define XADDIF_GUESSHEADS	 (1<<XADDIB_GUESSHEADS)
#define XADDIF_GUESSTRACKSECTORS (1<<XADDIB_GUESSTRACKSECTORS)
#define XADDIF_GUESSLOWCYL	 (1<<XADDIB_GUESSLOWCYL)
#define XADDIF_GUESSHIGHCYL	 (1<<XADDIB_GUESSHIGHCYL)

#define XADDIF_NOCYLINDERS	 (1<<XADDIB_NOCYLINDERS)
#define XADDIF_NOCYLSECTORS	 (1<<XADDIB_NOCYLSECTORS)
#define XADDIF_NOHEADS		 (1<<XADDIB_NOHEADS)
#define XADDIF_NOTRACKSECTORS	 (1<<XADDIB_NOTRACKSECTORS)
#define XADDIF_NOLOWCYL 	 (1<<XADDIB_NOLOWCYL)
#define XADDIF_NOHIGHCYL	 (1<<XADDIB_NOHIGHCYL)

/* defines for BlockInfo */
#define XADBIB_CLEARED		0 /* this block was cleared for archiving */
#define XADBIB_UNUSED		1 /* this block was not archived */

#define XADBIF_CLEARED		(1<<XADBIB_CLEARED)
#define XADBIF_UNUSED		(1<<XADBIB_UNUSED)

/************************************************************************
*									*
*    progress report stuff						*
*									*
************************************************************************/

struct xadProgressInfo {
  ULONG 		xpi_Mode;	/* work modus */
  struct xadClient *	xpi_Client;	/* the client doing the work */
  struct xadDiskInfo *	xpi_DiskInfo;	/* current diskinfo, for disks */
  struct xadFileInfo *	xpi_FileInfo;	/* current info for files */
  ULONG 		xpi_CurrentSize;/* current filesize */
  ULONG 		xpi_LowCyl;	/* for disks only */
  ULONG 		xpi_HighCyl;	/* for disks only */
  ULONG 		xpi_Status;	/* see XADPIF flags */
  LONG			xpi_Error;	/* any of the error codes */
  STRPTR		xpi_FileName;	/* name of file to overwrite (V2) */
  STRPTR		xpi_NewName;	/* new name buffer, passed by hook (V2) */
};
/* NOTE: For disks CurrentSize is Sector*SectorSize, where SectorSize can
be found in xadDiskInfo structure. So you may output the sector value. */

/* different progress modes */
#define XADPMODE_ASK		1
#define XADPMODE_PROGRESS	2
#define XADPMODE_END		3
#define XADPMODE_ERROR		4
#define XADPMODE_NEWENTRY	5 /* (V10) */
#define XADPMODE_GETINFOEND	6 /* (V11) */

/* flags for progress hook and ProgressInfo status field */
#define XADPIB_OVERWRITE	 0 /* overwrite the file */
#define XADPIB_MAKEDIRECTORY	 1 /* create the directory */
#define XADPIB_IGNOREGEOMETRY	 2 /* ignore drive geometry */
#define XADPIB_ISDIRECTORY	 3 /* destination is a directory (V10) */
#define XADPIB_RENAME		10 /* rename the file (V2) */
#define XADPIB_OK		16 /* all ok, proceed */
#define XADPIB_SKIP		17 /* skip file */

#define XADPIF_OVERWRITE	(1<<XADPIB_OVERWRITE)
#define XADPIF_MAKEDIRECTORY	(1<<XADPIB_MAKEDIRECTORY)
#define XADPIF_IGNOREGEOMETRY	(1<<XADPIB_IGNOREGEOMETRY)
#define XADPIF_ISDIRECTORY	(1<<XADPIB_ISDIRECTORY)
#define XADPIF_RENAME		(1<<XADPIB_RENAME)
#define XADPIF_OK		(1<<XADPIB_OK)
#define XADPIF_SKIP		(1<<XADPIB_SKIP)

/************************************************************************
*									*
*    errors								*
*									*
************************************************************************/

#define XADERR_OK		0x0000 /* no error */
#define XADERR_UNKNOWN		0x0001 /* unknown error */
#define XADERR_INPUT		0x0002 /* input data buffers border exceeded */
#define XADERR_OUTPUT		0x0003 /* output data buffers border exceeded */
#define XADERR_BADPARAMS	0x0004 /* function called with illegal parameters */
#define XADERR_NOMEMORY 	0x0005 /* not enough memory available */
#define XADERR_ILLEGALDATA	0x0006 /* data is corrupted */
#define XADERR_NOTSUPPORTED	0x0007 /* command is not supported */
#define XADERR_RESOURCE 	0x0008 /* required resource missing */
#define XADERR_DECRUNCH 	0x0009 /* error on decrunching */
#define XADERR_FILETYPE 	0x000A /* unknown file type */
#define XADERR_OPENFILE 	0x000B /* opening file failed */
#define XADERR_SKIP		0x000C /* file, disk has been skipped */
#define XADERR_BREAK		0x000D /* user break in progress hook */
#define XADERR_FILEEXISTS	0x000E /* file already exists */
#define XADERR_PASSWORD 	0x000F /* missing or wrong password */
#define XADERR_MAKEDIR		0x0010 /* could not create directory */
#define XADERR_CHECKSUM 	0x0011 /* wrong checksum */
#define XADERR_VERIFY		0x0012 /* verify failed (disk hook) */
#define XADERR_GEOMETRY 	0x0013 /* wrong drive geometry */
#define XADERR_DATAFORMAT	0x0014 /* unknown data format */
#define XADERR_EMPTY		0x0015 /* source contains no files */
#define XADERR_FILESYSTEM	0x0016 /* unknown filesystem */
#define XADERR_FILEDIR		0x0017 /* name of file exists as directory */
#define XADERR_SHORTBUFFER	0x0018 /* buffer was to short */
#define XADERR_ENCODING 	0x0019 /* text encoding was defective */

/************************************************************************
*									*
*    characterset and filename conversion				*
*									*
************************************************************************/

#define CHARSET_HOST			  0 /* this is the ONLY destination setting for clients! */

#define CHARSET_UNICODE_UCS2_HOST	  10 /* 16bit Unicode (usually no source type) */
#define CHARSET_UNICODE_UCS2_BIGENDIAN	  11 /* 16bit Unicode big endian storage */
#define CHARSET_UNICODE_UCS2_LITTLEENDIAN 12 /* 16bit Unicode little endian storage */
#define CHARSET_UNICODE_UTF8		  13 /* variable size unicode encoding */

/* all the 1xx types are generic types which also maybe a bit dynamic */
#define CHARSET_AMIGA			100 /* the default Amiga charset */
#define CHARSET_MSDOS 			101 /* the default MSDOS charset */
#define CHARSET_MACOS 			102 /* the default MacOS charset */
#define CHARSET_C64			103 /* the default C64 charset */
#define CHARSET_ATARI_ST		104 /* the default Atari ST charset */
#define CHARSET_WINDOWS			105 /* the default Windows charset */

/* all the 2xx to 9xx types are real charsets, use them whenever you know
   what the data really is */
#define CHARSET_ASCII			200 /* the lower 7 bits of ASCII charsets */
#define CHARSET_ISO_8859_1		201 /* the base charset */
#define CHARSET_ISO_8859_15		215 /* Euro-sign fixed ISO variant */
#define CHARSET_ATARI_ST_US		300 /* Atari ST (US) charset */
#define CHARSET_PETSCII_C64_LC		301 /* C64 lower case charset */
#define CHARSET_CODEPAGE_437		400 /* IBM Codepage 437 charset */
#define CHARSET_CODEPAGE_1252		401 /* Windows Codepage 1252 charset */

/************************************************************************
*									*
*    client related stuff						*
*									*
************************************************************************/

struct xadForeman {
  ULONG 	     xfm_Security;    /* should be XADFOREMAN_SECURITY */
  ULONG 	     xfm_ID;	      /* must be XADFOREMAN_ID */
  UWORD 	     xfm_Version;     /* set to XADFOREMAN_VERSION */
  UWORD 	     xfm_Reserved;
  STRPTR	     xfm_VersString;  /* pointer to $VER: string */
  struct xadClient * xfm_FirstClient; /* pointer to first client */
};

#define XADFOREMAN_SECURITY	0x70FF4E75 /* MOVEQ #-1,D0 and RTS */
#define XADFOREMAN_ID		0x58414446 /* 'XADF' identification ID */
#define XADFOREMAN_VERSION	1

struct xadClient {
  struct xadClient * xc_Next;
  UWORD 	     xc_Version;    /* set to XADCLIENT_VERSION */
  UWORD 	     xc_MasterVersion;
  UWORD 	     xc_ClientVersion;
  UWORD 	     xc_ClientRevision;
  ULONG 	     xc_RecogSize;  /* needed size to recog the type */
  ULONG 	     xc_Flags;	    /* see XADCF_xxx defines */
  ULONG 	     xc_Identifier; /* ID of internal clients */
  STRPTR	     xc_ArchiverName;
  BOOL		  (* xc_RecogData)();
  LONG		  (* xc_GetInfo)();
  LONG		  (* xc_UnArchive)();
  void		  (* xc_Free)();
};

/* function interface
ASM(BOOL) xc_RecogData(REG(d0, ULONG size), REG(a0, STRPTR data),
		REG(a6, struct xadMasterBase *xadMasterBase));
ASM(LONG) xc_GetInfo(REG(a0, struct xadArchiveInfo *ai),
		REG(a6, struct xadMasterBase *xadMasterBase));
ASM(LONG) xc_UnArchive(REG(a0, struct xadArchiveInfo *ai),
		REG(a6, struct xadMasterBase *xadMasterBase));
ASM(void) xc_Free(REG(a0, struct xadArchiveInfo *ai),
		REG(a6, struct xadMasterBase *xadMasterBase));
*/

/* xc_RecogData returns 1 when recognized and 0 when not, all the others
   return 0 when ok and XADERR values on error. xc_Free has no return
   value.

   Filesystem clients need to clear xc_RecogSize and xc_RecogData. The
   recognition is automatically done by GetInfo. XADERR_FILESYSTEM is
   returned in case of unknown format. If it is known detection should
   go on and any other code may be returned, if it fails.
   The field xc_ArchiverName means xc_FileSystemName for filesystem
   clients.
*/

#define XADCLIENT_VERSION	1

#define XADCB_FILEARCHIVER	 0 /* archiver is a file archiver */
#define XADCB_DISKARCHIVER	 1 /* archiver is a disk archiver */
#define XADCB_EXTERN		 2 /* external client, set by xadmaster */
#define XADCB_FILESYSTEM	 3 /* filesystem clients (V5) */
#define XADCB_NOCHECKSIZE	 4 /* do not check size for recog call (V6) */
#define XADCB_DATACRUNCHER	 5 /* file archiver is plain data file (V11) */
#define XADCB_EXECRUNCHER	 6 /* file archiver is executable file (V11) */
#define XADCB_ADDRESSCRUNCHER	 7 /* file archiver is address crunched file (V11) */
#define XADCB_LINKER		 8 /* file archiver is a linker file (V11) */
#define XADCB_FREEXADSTRINGS	25 /* master frees XAD strings (V12) */
#define XADCB_FREESPECIALINFO	26 /* master frees xadSpecial  structures (V11) */
#define XADCB_FREESKIPINFO	27 /* master frees xadSkipInfo structures (V3) */
#define XADCB_FREETEXTINFO	28 /* master frees xadTextInfo structures (V2) */
#define XADCB_FREETEXTINFOTEXT	29 /* master frees xadTextInfo text block (V2) */
#define XADCB_FREEFILEINFO	30 /* master frees xadFileInfo structures (V2) */
#define XADCB_FREEDISKINFO	31 /* master frees xadDiskInfo structures (V2) */

#define XADCF_FILEARCHIVER	(1<<XADCB_FILEARCHIVER)
#define XADCF_DISKARCHIVER	(1<<XADCB_DISKARCHIVER)
#define XADCF_EXTERN		(1<<XADCB_EXTERN)
#define XADCF_FILESYSTEM	(1<<XADCB_FILESYSTEM)
#define XADCF_NOCHECKSIZE	(1<<XADCB_NOCHECKSIZE)
#define XADCF_DATACRUNCHER	(1<<XADCB_DATACRUNCHER)
#define XADCF_EXECRUNCHER	(1<<XADCB_EXECRUNCHER)
#define XADCF_ADDRESSCRUNCHER	(1<<XADCB_ADDRESSCRUNCHER)
#define XADCF_LINKER		(1<<XADCB_LINKER)
#define XADCF_FREEXADSTRINGS	(1<<XADCB_FREEXADSTRINGS)
#define XADCF_FREESPECIALINFO	(1<<XADCB_FREESPECIALINFO)
#define XADCF_FREESKIPINFO	(1<<XADCB_FREESKIPINFO)
#define XADCF_FREETEXTINFO	(1<<XADCB_FREETEXTINFO)
#define XADCF_FREETEXTINFOTEXT	(1<<XADCB_FREETEXTINFOTEXT)
#define XADCF_FREEFILEINFO	(1<<XADCB_FREEFILEINFO)
#define XADCF_FREEDISKINFO	(1<<XADCB_FREEDISKINFO)

/* The types 5 to 9 always need XADCB_FILEARCHIVER set also. These only specify
the type of the archiver somewhat better. Do not mix real archivers and these
single file data clients. */

/************************************************************************
*									*
*    client ID's							*
*									*
************************************************************************/

/* If an external client has set the xc_Identifier field, the internal
client is replaced. */

/* disk archivers start with 1000 */
#define XADCID_XMASH			1000
#define XADCID_SUPERDUPER3		1001
#define XADCID_XDISK			1002
#define XADCID_PACKDEV			1003
#define XADCID_ZOOM			1004
#define XADCID_ZOOM5			1005
#define XADCID_CRUNCHDISK		1006
#define XADCID_PACKDISK 		1007
#define XADCID_MDC			1008
#define XADCID_COMPDISK 		1009
#define XADCID_LHWARP			1010
#define XADCID_SAVAGECOMPRESSOR 	1011
#define XADCID_WARP			1012
#define XADCID_GDC			1013
#define XADCID_DCS			1014

/* file archivers start with 5000 */
#define XADCID_TAR			5000
#define XADCID_SDSSFX			5001
#define XADCID_LZX			5002
#define XADCID_MXMSIMPLEARC		5003
#define XADCID_LHPAK			5004
#define XADCID_AMIGAPLUSUNPACK		5005
#define XADCID_AMIPACK			5006
#define XADCID_LHA			5007
#define XADCID_LHASFX			5008
#define XADCID_PCOMPARC 		5009
#define XADCID_SOMNI			5010
#define XADCID_LHSFX			5011
#define XADCID_XPKARCHIVE		5012
#define XADCID_SHRINK			5013
#define XADCID_SPACK			5014
#define XADCID_SPACKSFX 		5015
#define XADCID_ZIP			5016
#define XADCID_WINZIPEXE		5017
#define XADCID_GZIP			5018
#define XADCID_ARC			5019
#define XADCID_ZOO			5020
#define XADCID_LHAEXE			5021
#define XADCID_ARJ			5022
#define XADCID_ARJEXE			5023
#define XADCID_ZIPEXE			5024
#define XADCID_LHF			5025
#define XADCID_COMPRESS 		5026
#define XADCID_ACE			5027
#define XADCID_ACEEXE			5028
#define XADCID_GZIPSFX			5029
#define XADCID_HA			5030
#define XADCID_SQ			5031
#define XADCID_LHAC64SFX		5032
#define XADCID_SIT			5033
#define XADCID_SIT5			5034
#define XADCID_SIT5EXE			5035
#define XADCID_MACBINARY		5036
#define XADCID_CPIO			5037
#define XADCID_PACKIT			5038
#define XADCID_CRUNCH			5039
#define XADCID_ARCCBM			5040
#define XADCID_ARCCBMSFX		5041

/* filesystem client start with 8000 */

#define XADCID_FSAMIGA			8000
#define XADCID_FSSANITYOS		8001
#define XADCID_FSFAT			8002

/* mixed archivers start with 9000 */
#define XADCID_DMS			9000
#define XADCID_DMSSFX			9001

#endif /* LIBRARIES_XADMASTER_H */
