/*
    Copyright � 1995-2012, The AROS Development Team. All rights reserved.
    $Id$

    Desc: 
    Lang: English
*/


/******************************************************************************

    NAME

        AddDataTypes (files) [QUIET] [REFRESH] [LIST]

    SYNOPSIS

        FILES/M, QUIET/S, REFRESH/S, LIST/S

    LOCATION

        C:

    FUNCTION

        AddDataTypes allows you to activate a set of specific DataTypes.
        This might be necessary if new DataTypes were installed on your 
        system or were not activated on startup.

    INPUTS

        FILES  --  The name of the file(s) of the corresponding DataType.
        QUIET  --  Won't output any messages
      REFRESH  --  Refreshes the DataTypes list?
         LIST  --  This will display a list of current DataTypes loaded in
                   memory

    RESULT

        Standard DOS error codes.

    NOTES

    EXAMPLE

        AddDataTypes gif.datatype REFRESH

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

******************************************************************************/

#include <aros/macros.h>
#include <aros/bigendianio.h>
#include <exec/types.h>
#include <exec/memory.h>
#include <exec/execbase.h>
#include <dos/dos.h>
#include <dos/dosasl.h>
#include <dos/dosextens.h>
#include <libraries/iffparse.h>
#include <utility/name.h>
#include <utility/hooks.h>
#include <workbench/startup.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/utility.h>
#include <proto/iffparse.h>
#include <proto/alib.h>
#include <proto/arossupport.h>

#include <string.h>

#include "../libs/datatypes/datatypes_intern.h"
#undef DOSBase
#undef UtilityBase
#undef IFFParseBase
#undef LocaleBase
#undef DTList

/******************************** STRUCTURES *********************************/

/* same as datatypes/datatypes.h/struct DataTypeHeader, but always big endian
and 32 bit pointers (which in the file are actually offsets) */

struct FileDataTypeHeader	    
{
    ULONG	 fdth_NameOffset;     /* Name of the data type */
    ULONG	 fdth_BaseNameOffset; /* Base name of the data type */
    ULONG	 fdth_PatternOffset;  /* File name match pattern */
    ULONG	 fdth_MaskOffset;     /* Comparision mask (binary) */
    ULONG	 fdth_GroupID;        /* DataType Group */
    ULONG	 fdth_ID;             /* DataType ID (same as IFF FORM type) */
    WORD	 fdth_MaskLen;        /* Length of the comparision mask */
    WORD	 fdth_Pad;            /* Unused at present (must be 0) */
    UWORD	 fdth_Flags;          /* Flags -- see below */
    UWORD	 fdth_Priority;
};

#define O(x)        offsetof(struct FileDataTypeHeader,x)

static const IPTR FileDataTypeHeaderDesc[] =
{
    sizeof(struct FileDataTypeHeader),
    SDM_ULONG(O(fdth_NameOffset)),
    SDM_ULONG(O(fdth_BaseNameOffset)),
    SDM_ULONG(O(fdth_PatternOffset)),
    SDM_ULONG(O(fdth_MaskOffset)),
    SDM_ULONG(O(fdth_GroupID)),
    SDM_ULONG(O(fdth_ID)),
    SDM_WORD(O(fdth_MaskLen)),
    SDM_WORD(O(fdth_Pad)),
    SDM_UWORD(O(fdth_Flags)),
    SDM_UWORD(O(fdth_Priority)),
    SDM_END
};

/******************************** PROTOTYPES *********************************/

struct StackVars;		/* forward declaration */

BOOL DateScan(struct StackVars *sv);
void ScanDirectory(struct StackVars *sv, STRPTR pattern);
struct DataTypesList *CreateDTList(struct StackVars *sv);
struct CompoundDataType *CreateBasicType(struct StackVars *sv,
					 struct List *list,
					 struct List *globallist, STRPTR name,
					 UWORD Flags, ULONG ID, ULONG GroupID);
void LoadDataType(struct StackVars *sv, STRPTR name);
struct CompoundDataType *CreateDataType(struct StackVars *sv,
					struct IFFHandle *iff);
struct CompoundDataType *AddDataType(struct StackVars *sv, 
				     struct CompoundDataType *cdt);
void DeleteDataType(struct StackVars *sv, struct CompoundDataType *cdt);
void AlphaInsert(struct StackVars *sv, struct List *list, struct Node *node);
struct Node *__FindNameNoCase(struct StackVars *sv, struct List *list,
			      STRPTR name);

AROS_UFP4(LONG, AROS_SLIB_ENTRY(ReadFunc, AddDataTypes, 0), 
	AROS_UFPA(BPTR   , fh        , D1),
	AROS_UFPA(void * , buf       , D2),
	AROS_UFPA(LONG   , size      , D3),
	AROS_UFPA(struct DosLibrary *, DOSBase, A6));
AROS_UFP4(LONG, AROS_SLIB_ENTRY(SeekFunc, AddDataTypes, 0), 
	AROS_UFPA(BPTR   , fh        , D1),
	AROS_UFPA(LONG   , pos       , D2),
	AROS_UFPA(LONG   , mode      , D3),
	AROS_UFPA(struct DosLibrary *, DOSBase, A6));
AROS_UFP3(UBYTE *, AROS_SLIB_ENTRY(AllocFunc, AddDataTypes, 0), 
	AROS_UFPA(ULONG, size, D0),
	AROS_UFPA(ULONG, req,  D1),
	AROS_UFPA(struct DosLibrary *, DOSBase, A6));
AROS_UFP3(void, AROS_SLIB_ENTRY(FreeFunc, AddDataTypes, 0), 
	AROS_UFPA(APTR , memory, A1),
	AROS_UFPA(ULONG, size  , D0),
	AROS_UFPA(struct DosLibrary *, DOSBase, A6));

/********************************* CONSTANTS *********************************/

const TEXT Version[] = "$VER: AddDataTypes 42.1 (8.12.2007)\n";

#define EXCL_LEN 18
UBYTE ExcludePattern[] = "#?.(info|backdrop)";

UBYTE Template[] = "FILES/M,QUIET/S,REFRESH/S,LIST/S";

struct ArgArray
{
    UBYTE **aa_Files;
    IPTR    aa_Quiet;
    IPTR   aa_Refresh;
    IPTR   aa_List;
};

#define  ID_DTCD  MAKE_ID('D','T','C','D')
#define  ID_DTTL  MAKE_ID('D','T','T','L')

#define  NUM_PROP  2

LONG PropArray[2*NUM_PROP]=
{
    ID_DTYP, ID_DTHD,
    ID_DTYP, ID_DTCD
};

#define  NUM_COLL  1

const LONG const CollArray[2*NUM_COLL]=
{
    ID_DTYP, ID_DTTL
};


const LONG_FUNC const FunctionArray[]=
{				/* Note! */
    (LONG_FUNC)AROS_SLIB_ENTRY(ReadFunc, AddDataTypes, 0),
    (LONG_FUNC)AROS_SLIB_ENTRY(AllocFunc, AddDataTypes, 0),
    (LONG_FUNC)AROS_SLIB_ENTRY(FreeFunc, AddDataTypes, 0),
    (LONG_FUNC)AROS_SLIB_ENTRY(SeekFunc, AddDataTypes, 0), /* For ELF */
};


struct StackVars
{
    struct DataTypesList *DTList;
    UBYTE                 ExclPat[2*EXCL_LEN+2+1];
    struct ArgArray  AA;
    UBYTE           *HookBuffer;
    ULONG            HookBufSize;
    ULONG            HookPosition;
};

#undef  SysBase
#define DTList            sv->DTList
#define ExclPat           sv->ExclPat
#define AA                sv->AA
#define HookBuffer        sv->HookBuffer
#define HookBufSize       sv->HookBufSize
#define HookPosition      sv->HookPosition

/****** AddDataTypes/main *****************************************************
*
*   NAME
*        main - well... main
*
*   SYNOPSIS
*
*   FUNCTION
*
*   INPUTS
*
*   RETURNS
*
*   EXAMPLE
*
*   SEE ALSO
*
******************************************************************************
*
*/
int UtilityBase_version  = 37;
int LocaleBase_version   = 37;
int IFFParseBase_version = 37;

int __nocommandline = 1;

int main(void)
{
    extern struct WBStartup *WBenchMsg;
    struct StackVars  vars;
    struct StackVars *sv;
    int    result = RETURN_FAIL;

    memset(&vars, 0, sizeof(struct StackVars));
    sv = &vars;

    if((DTList = CreateDTList(sv)))
    {
    	ParsePatternNoCase(ExcludePattern, ExclPat, sizeof(ExclPat));

	ObtainSemaphore(&DTList->dtl_Lock);

	if(WBenchMsg)
	{
	    UWORD num;
	    struct WBArg *wa = &WBenchMsg->sm_ArgList[1];

	    for(num = 1; num<WBenchMsg->sm_NumArgs; wa++)
	    {
		BPTR olddir = CurrentDir(wa->wa_Lock);
		LoadDataType(sv, wa->wa_Name);
		CurrentDir(olddir);
	    }

	    result = RETURN_OK;
	}
	else
	{
	    struct RDArgs *RDArgs;

	    if(!(RDArgs = ReadArgs(Template, (SIPTR*)&AA, NULL)))
	    {
		PrintFault(IoErr(), NULL);
	    }
	    else
	    {
		if(AA.aa_Refresh)
		{
		    if(DateScan(sv))
		    {
			ScanDirectory(sv, "DEVS:DataTypes");
		    }
		}
		else
		{
		    UBYTE **files = AA.aa_Files;

		    if(files)
		    {
			while(*files)
			{
			    ScanDirectory(sv, *files);
			    files++;
			}
		    }
	        }

		if(AA.aa_List)
		{
			struct DataTypesList *dtl = NULL;
			struct NamedObject   *no  = NULL;
			if((no = FindNamedObject(NULL, DATATYPESLIST, NULL)))
			{
				dtl = (struct DataTypesList*)no->no_Object;
				ReleaseNamedObject(no);
				if(dtl != NULL)
				{
					struct Node *node=dtl->dtl_SortedList.lh_Head;
					while(node->ln_Succ != NULL)
					{
						// sorted list points to DT.dtn_Node2 ....
						struct CompoundDataType *cdt;
						struct DataTypeHeader *dth;
						STRPTR argarray[2];

						if(CheckSignal(SIGBREAKF_CTRL_C))
						{
							Flush(Output());
							PrintFault(ERROR_BREAK,0);
							break;
						}
						cdt=(struct CompoundDataType *)(node-1);
						dth=cdt->DT.dtn_Header;

						argarray[0] = dth->dth_BaseName;
						argarray[1] = dth->dth_Name;

						VPrintf("%s, \"%s\"\n", (IPTR *)argarray);
						node = node->ln_Succ;
					}
				}
			}

		}

		result = RETURN_OK;
		FreeArgs(RDArgs);
	    }
	}

	ReleaseSemaphore(&DTList->dtl_Lock);
    }

    return result;
}



/****** AddDataTypes/DateScan *************************************************
*
*   NAME
*        DateScan - See if DataTypes descriptors need updating
*
*   SYNOPSIS
*
*   FUNCTION
*
*   INPUTS
*
*   RETURNS
*
*   EXAMPLE
*
*   SEE ALSO
*
******************************************************************************
*
*/

BOOL DateScan(struct StackVars *sv)
{
    BOOL   result = TRUE;
    BPTR   lock;
    struct FileInfoBlock *fib;

    if((lock = Lock("DEVS:DataTypes", ACCESS_READ)))
    {
	if((fib = AllocDosObject(DOS_FIB, NULL)))
	{
	    if(Examine(lock, fib))
	    {
		if(!CompareDates(&fib->fib_Date, &DTList->dtl_DateStamp))
		{
		    result = FALSE;
		}
		else
		{
		    DTList->dtl_DateStamp = fib->fib_Date;
		}
	    }

	    FreeDosObject(DOS_FIB,fib);
	}

	UnLock(lock);
    }
    
    return result;
}



/****** AddDataTypes/ScanDirectory ********************************************
*
*   NAME
*        ScanDirectory - Scan a directory recursively for DT descriptors
*
*   SYNOPSIS
*
*   FUNCTION
*
*   INPUTS
*
*   RETURNS
*
*   EXAMPLE
*
*   SEE ALSO
*
******************************************************************************
*
*/

void ScanDirectory(struct StackVars *sv, STRPTR pattern)
{
    struct AnchorPath *AnchorPath;
    LONG               RetVal;
    BPTR               OldDir;
	
    if((AnchorPath = (struct AnchorPath *)AllocVec(sizeof(struct AnchorPath),
						  MEMF_CLEAR)))
    {
	AnchorPath->ap_BreakBits = SIGBREAKF_CTRL_C;
	
	RetVal = MatchFirst(pattern, AnchorPath);

	while(!RetVal)
	{
	    if(CheckSignal(SIGBREAKF_CTRL_C))
	    {
		if(!AA.aa_Quiet)
		{
		    PrintFault(ERROR_BREAK, NULL);
		}

		break;
	    }
	    
	    if(AnchorPath->ap_Info.fib_DirEntryType > 0L)
	    {
		if(!(AnchorPath->ap_Flags & APF_DIDDIR))
		{
		    AnchorPath->ap_Flags |= APF_DODIR;
		}

		AnchorPath->ap_Flags &= ~APF_DIDDIR;
	    }
	    else
	    {
		if(!MatchPatternNoCase(ExclPat, 
				       AnchorPath->ap_Info.fib_FileName))
		{
		    OldDir = CurrentDir(AnchorPath->ap_Current->an_Lock);
		    
		    LoadDataType(sv, AnchorPath->ap_Info.fib_FileName);
		    
		    CurrentDir(OldDir);
		}
	    }

	    RetVal = MatchNext(AnchorPath);
	}

	if(RetVal != ERROR_NO_MORE_ENTRIES)
	{
	    if(!AA.aa_Quiet)
	    {
		PrintFault(RetVal, NULL);
	    }
	}

	MatchEnd(AnchorPath);
	
	FreeVec((APTR)AnchorPath);
    }
}


/****** AddDataTypes/CreateDTList *********************************************
*
*   NAME
*        CreateDTList - Create and initialize the DataTypesList
*
*   SYNOPSIS
*
*   FUNCTION
*
*   INPUTS
*
*   RETURNS
*
*   EXAMPLE
*
*   SEE ALSO
*
******************************************************************************
*
*/

struct DataTypesList *CreateDTList(struct StackVars *sv)
{
    struct DataTypesList *dtl = NULL;
    struct NamedObject   *no  = NULL;
    struct Library *DTBase;

    /* We do this in order to force datatypes.library to get
       loaded and initialized. During this process it will
       create and install DataTypes list object in memory. */
    DTBase = OpenLibrary("datatypes.library", 0);
    if (DTBase)
	CloseLibrary(DTBase);
    
    if((no = FindNamedObject(NULL, DATATYPESLIST, NULL)))
    {
	dtl = (struct DataTypesList*)no->no_Object;
    }
    
    if(dtl)
    {
	if(!__FindNameNoCase(sv, &dtl->dtl_BinaryList, "binary"))
	{
	    CreateBasicType
            (
                sv, &dtl->dtl_BinaryList, &dtl->dtl_SortedList,
                "binary", DTF_BINARY, ID_BINARY, GID_SYSTEM
            );
	}

	if(!__FindNameNoCase(sv, &dtl->dtl_ASCIIList, "ascii"))
	{
	    CreateBasicType
            (
                sv, &dtl->dtl_ASCIIList,  &dtl->dtl_SortedList,
                "ascii", DTF_ASCII, ID_ASCII, GID_TEXT
            );
	}

	if(!__FindNameNoCase(sv, &dtl->dtl_IFFList, "iff"))
	{
	    CreateBasicType
            (
                sv, &dtl->dtl_IFFList, &dtl->dtl_SortedList,
                "iff", DTF_IFF, ID_IFF, GID_SYSTEM
            );
	}

	if(!__FindNameNoCase(sv, &dtl->dtl_MiscList, "directory"))
	{
	    CreateBasicType
            (
                sv, &dtl->dtl_MiscList, &dtl->dtl_SortedList,
                "directory", DTF_MISC, ID_DIRECTORY, GID_SYSTEM
            );
	}
    }
    
    if(no)
    {
	ReleaseNamedObject(no);
    }
    
    
    return dtl;
}



/****** AddDataTypes/CreateBasicType ******************************************
*
*   NAME
*        CreateBasicType - Initialize one of the basic types
*
*   SYNOPSIS
*
*   FUNCTION
*
*   INPUTS
*
*   RETURNS
*
*   EXAMPLE
*
*   SEE ALSO
*
******************************************************************************
*
*/

struct CompoundDataType *CreateBasicType(struct StackVars *sv, 
					 struct List *list, 
					 struct List *globallist, STRPTR name,
					 UWORD Flags, ULONG ID, ULONG GroupID)
{
    struct CompoundDataType *cdt;
    ULONG AllocLen = sizeof(struct CompoundDataType) + strlen(name) + 1;
    
    if((cdt = AllocVec(AllocLen, MEMF_PUBLIC | MEMF_CLEAR)))
    {
	cdt->DT.dtn_Header = &cdt->DTH;
	
	strcpy((UBYTE*)(cdt + 1), name);
	
	cdt->DTH.dth_Name=
	    cdt->DTH.dth_BaseName=
	    cdt->DT.dtn_Node1.ln_Name=
	    cdt->DT.dtn_Node2.ln_Name=(UBYTE*)(cdt + 1);

	cdt->DTH.dth_GroupID = GroupID;
	cdt->DTH.dth_ID = ID;

	cdt->DTH.dth_Flags = Flags;

	NewList(&cdt->DT.dtn_ToolList);
		
	cdt->DT.dtn_Length = AllocLen;
		
	cdt->DT.dtn_Node1.ln_Pri = -128;
	Enqueue(list, &cdt->DT.dtn_Node1);
		
	AlphaInsert(sv, globallist, &cdt->DT.dtn_Node2);
    }

    return cdt;
}



/****** AddDataTypes/LoadDataType *********************************************
*
*   NAME
*        LoadDataType - Load and install a single DataType descriptor
*
*   SYNOPSIS
*
*   FUNCTION
*
*   INPUTS
*
*   RETURNS
*
*   EXAMPLE
*
*   SEE ALSO
*
******************************************************************************
*
*/

void LoadDataType(struct StackVars *sv, STRPTR name)
{
    struct IFFHandle *iff;

    if((iff = AllocIFF()))
    {
	if((iff->iff_Stream = (IPTR)Open(name, MODE_OLDFILE))) /* Why IPTR? */
	{
	    InitIFFasDOS(iff);
	    
	    if(!OpenIFF(iff, IFFF_READ))
	    {
		if(!PropChunks(iff, PropArray, NUM_PROP))
		{
		    if(!CollectionChunks(iff, CollArray, NUM_COLL))
		    {
			if(!StopOnExit(iff, ID_DTYP, ID_FORM))
			{
			    LONG error;
			    
			    while((error = ParseIFF(iff, IFFPARSE_SCAN)) == IFFERR_EOC) 
			    {
				CreateDataType(sv, iff);
				/* FIXME: The while ParseIFF loop here crashes the 2nd time inside the loop, therefore the break below as temp fix */
				break;
			    }
			}
		    }
		}
		
		CloseIFF(iff);
	    }

	    Close((BPTR)iff->iff_Stream);
	}

	FreeIFF(iff);
    }
    else
    {
	SetIoErr(ERROR_NO_FREE_STORE);
    }
}


/****** AddDataTypes/MemStreamHook *******************************************
*
*   NAME
*        MemStreamHook - needed by ReadStruct
*
*   SYNOPSIS
*
*   FUNCTION
*
*   INPUTS
*
*   RETURNS
*
*   EXAMPLE
*
*   SEE ALSO
*
******************************************************************************
*
*/

LONG MemStreamHook(struct Hook * hook, UBYTE **memptr, Msg msg)
{
    LONG rc;

    switch (msg->MethodID)
    {
	case BEIO_READ:
	    rc = **memptr;
	    (*memptr)++;
	    break;
	    
	default:
	    rc = -1;
	    break;
    }

    return rc;
}

/****** AddDataTypes/CreateDataType *******************************************
*
*   NAME
*        CreateDataType - create a DataType from IFF chunks
*
*   SYNOPSIS
*
*   FUNCTION
*
*   INPUTS
*
*   RETURNS
*
*   EXAMPLE
*
*   SEE ALSO
*
******************************************************************************
*
*/

struct CompoundDataType *CreateDataType(struct StackVars *sv,
					struct IFFHandle *iff)
{
    struct CompoundDataType *cdt = NULL;
    struct StoredProperty *prop;
    ULONG  AllocLen;
    UBYTE *func;
    LONG   DefaultStack = AROS_STACKSIZE, i;
    BPTR   SegList;
 
    if((prop = FindProp(iff, ID_DTYP, ID_DTHD)))
    {
	AllocLen = sizeof(struct CompoundDataType) - 
	          32 + /* was sizeof(struct DataTypeHeader), but we must use struct size as it would be on Amiga */
		  prop->sp_Size;
	
	if(!(cdt = AllocVec(AllocLen, MEMF_PUBLIC | MEMF_CLEAR)))
	{
	    SetIoErr(ERROR_NO_FREE_STORE);
	}
	else
	{
	    struct FileDataTypeHeader *fdh = NULL;
	    UBYTE *memptr = (UBYTE *)prop->sp_Data;
	    struct Hook hook;
	    
	    hook.h_Entry = (HOOKFUNC)HookEntry;
	    hook.h_SubEntry = (HOOKFUNC)MemStreamHook;
	    
	    if (ReadStruct(&hook, (APTR *) &fdh, &memptr, FileDataTypeHeaderDesc))
	    {
		IPTR extraoffset = sizeof(struct DataTypeHeader) - 32;
		
		cdt->DT.dtn_Header= &cdt->DTH;

		cdt->DTH.dth_Name = (STRPTR)(fdh->fdth_NameOffset + extraoffset + (IPTR)&cdt->DTH);
		cdt->DTH.dth_BaseName = (STRPTR)(fdh->fdth_BaseNameOffset + extraoffset + (IPTR)&cdt->DTH);
		cdt->DTH.dth_Pattern = (STRPTR)(fdh->fdth_PatternOffset + extraoffset + (IPTR)&cdt->DTH);
		cdt->DTH.dth_Mask = (WORD *)(fdh->fdth_MaskOffset + extraoffset + (IPTR)&cdt->DTH);
		cdt->DTH.dth_GroupID = fdh->fdth_GroupID;
		cdt->DTH.dth_ID = fdh->fdth_ID;
		cdt->DTH.dth_MaskLen = fdh->fdth_MaskLen;
		cdt->DTH.dth_Pad = fdh->fdth_Pad;
		cdt->DTH.dth_Flags = fdh->fdth_Flags;
		cdt->DTH.dth_Priority = fdh->fdth_Priority;
		
		CopyMem(prop->sp_Data + 32, cdt + 1, prop->sp_Size - 32);
		
		for(i = 0; i < cdt->DTH.dth_MaskLen; i++)
		{
	            cdt->DTH.dth_Mask[i] = AROS_BE2WORD(cdt->DTH.dth_Mask[i]);
#if 0
		    kprintf("mask[%d] = %04x (%c %c)\n", i, 
			    cdt->DTH.dth_Mask[i],
			    cdt->DTH.dth_Mask[i] & 255,
			    (cdt->DTH.dth_Mask[i] >> 8) & 255);
#endif
		}

#if 0	    
		kprintf("groupid  = %c%c%c%c\n", cdt->DTH.dth_GroupID >> 24,
						 cdt->DTH.dth_GroupID >> 16,
						 cdt->DTH.dth_GroupID >> 8,
						 cdt->DTH.dth_GroupID);
		kprintf("id       = %c%c%c%c\n", cdt->DTH.dth_ID >> 24,
						 cdt->DTH.dth_ID >> 16,
						 cdt->DTH.dth_ID >> 8,
						 cdt->DTH.dth_ID);
		kprintf("flags    = %x\n",	 cdt->DTH.dth_Flags);
		kprintf("pri      = %d\n",	 cdt->DTH.dth_Priority);
		kprintf("name     = %s\n",	 cdt->DTH.dth_Name);
		kprintf("basename = %s\n",	 cdt->DTH.dth_BaseName);
		kprintf("pattern  = %s\n",	 cdt->DTH.dth_Pattern);
		kprintf("masklen  = %d\n",	 cdt->DTH.dth_MaskLen);
#endif

		NewList(&cdt->DT.dtn_ToolList);

		cdt->DT.dtn_Length = AllocLen;

		if((prop = FindProp(iff, ID_DTYP, ID_DTCD)))
		{
		    if((func = AllocVec(prop->sp_Size, MEMF_PUBLIC | MEMF_CLEAR)))
		    {
			cdt->DTCDChunk = func;
			cdt->DTCDSize = prop->sp_Size;

			CopyMem(prop->sp_Data,func,prop->sp_Size);

			HookBuffer = cdt->DTCDChunk;
			HookBufSize = cdt->DTCDSize;
			HookPosition = 0;

			/* We use a cast to BPTR, since sv may not
			 * be ULONG aligned. Since only our ReadFunc
			 * is going to be looking at it, this is ok.
			 */
			if((SegList = InternalLoadSeg((BPTR)(sv), BNULL,
						      (LONG_FUNC *)FunctionArray,
						      &DefaultStack)))
			{
			    cdt->SegList = SegList;
			    cdt->Function = BADDR(SegList) + sizeof(BPTR);
			}

		    } /* if((func = AllocVec(prop->sp_Size, MEMF_PUBLIC | MEMF_CLEAR))) */

		} /* if((prop = FindProp(iff, ID_DTYP, ID_DTCD))) */

		cdt = AddDataType(sv, cdt);

		FreeStruct(fdh, FileDataTypeHeaderDesc);
		
	    } /* if (ReadStruct(&hook, &fdh, &memptr, FileDataTypeHeaderDesc)) */
	    
	} /* cdt AllocVec okay */
	
    } /* if((prop = FindProp(iff, ID_DTYP, ID_DTHD))) */
    
    return cdt;
}



/****** AddDataTypes/AddDataType **********************************************
*
*   NAME
*        AddDataType - add a DataType to the system
*
*   SYNOPSIS
*
*   FUNCTION
*
*        This subroutine tries to add a DataType to the system DataTypes
*        list. If the DataType already exists, it will be replaced or
*        updated. In case of an error, the CompoundDataType will be deleted
*        and a NULL pointer is returned.
*
*        The CompoundDataType pointer you passed in will be invalid after
*        calling this function. Use the returned handle instead.
*        DO NOT USE THE OLD POINTER IN ANY MORE!
*
*   INPUTS
*
*   RETURNS
*
*        A pointer to a CompoundDataType in the system DataTypes list
*        or a NULL pointer for failure
*
*   EXAMPLE
*
*   SEE ALSO
*
******************************************************************************
*
*/

struct CompoundDataType *AddDataType(struct StackVars *sv,
				     struct CompoundDataType *cdt)
{
    struct List *typelist;
    BOOL   Success = FALSE;
    ULONG  AllocSize;
    ULONG  result;
    struct CompoundDataType *oldcdt;
    
    switch(cdt->DTH.dth_Flags & DTF_TYPE_MASK)
    {
    case DTF_BINARY: typelist= &DTList->dtl_BinaryList; break;
    case DTF_ASCII:  typelist= &DTList->dtl_ASCIIList; break;
    case DTF_IFF:    typelist= &DTList->dtl_IFFList; break;
    case DTF_MISC:   typelist= &DTList->dtl_MiscList; break;
    default:         typelist= NULL;
    }
    
    if(typelist)
    {
	cdt->DT.dtn_Node1.ln_Name = cdt->DT.dtn_Node2.ln_Name = cdt->DTH.dth_Name;
	
	Success = TRUE;
	
	if((!Stricmp(cdt->DTH.dth_Pattern, "#?")) || 
	   (!strlen(cdt->DTH.dth_Pattern)) )
	{
	    cdt->FlagLong |= CFLGF_PATTERN_UNUSED;
	}
	else
	{
	    cdt->FlagLong &= ~(CFLGF_PATTERN_UNUSED);
	    
	    AllocSize = 2*strlen(cdt->DTH.dth_Pattern) + 2;

	    if(!(cdt->ParsePatMem = AllocVec(AllocSize,
					     MEMF_PUBLIC | MEMF_CLEAR)))
	    {
		Success = FALSE;
	    }
	    else
	    {
		cdt->ParsePatSize = AllocSize;
		
		result = ParsePatternNoCase(cdt->DTH.dth_Pattern,
					    cdt->ParsePatMem, AllocSize);
		
		if(result == 1)
		{
		    cdt->FlagLong |= CFLGF_IS_WILD;
		}
		else
		{
		    FreeVec(cdt->ParsePatMem);
		    cdt->ParsePatMem = NULL;
		    cdt->ParsePatSize = 0;
		    
		    if(result == 0)
		    {
			cdt->FlagLong &= ~(CFLGF_IS_WILD);
		    }
		    else
		    {
			Success = FALSE;
		    }
		}
	    }
	}

	if (Success)
	{
	    if((oldcdt = (struct CompoundDataType*)__FindNameNoCase(sv,
								    typelist,
								    cdt->DT.dtn_Node1.ln_Name)))
	    {
		if (oldcdt->OpenCount)
		{
		    Success = FALSE;
		}
		else
		{
		    if((Stricmp(oldcdt->DTH.dth_Name, cdt->DTH.dth_Name)) ||
		       (Stricmp(oldcdt->DTH.dth_BaseName, cdt->DTH.dth_BaseName)) ||
		       (Stricmp(oldcdt->DTH.dth_Pattern, cdt->DTH.dth_Pattern)) ||
		       (oldcdt->DTH.dth_Flags != cdt->DTH.dth_Flags) ||
		       (oldcdt->DTH.dth_Priority != cdt->DTH.dth_Priority) ||
		       (oldcdt->DTH.dth_MaskLen != cdt->DTH.dth_MaskLen))
		    {
			DeleteDataType(sv, oldcdt);
			oldcdt = NULL;
		    }
		    else
		    {
			oldcdt->DTH.dth_GroupID = cdt->DTH.dth_GroupID;
			oldcdt->DTH.dth_ID      = cdt->DTH.dth_ID;
			CopyMem(cdt->DTH.dth_Mask,cdt->DTH.dth_Mask,
				(ULONG)(sizeof(WORD)*cdt->DTH.dth_MaskLen));
		    }
		}
	    }
	    
	    if(Success)
	    {
		if(oldcdt)
		{
		    DeleteDataType(sv, cdt);
		    cdt = oldcdt;
		}
		else
		{
		    if(cdt->DT.dtn_FunctionName)
		    {
			LONG DefaultStack = 4096;
			BPTR file;
			ULONG AllocLen;
			BPTR SegList;
			
			if((file = Open(cdt->DT.dtn_FunctionName, MODE_OLDFILE)))
			{
			    if(Seek(file, 0, OFFSET_END) >= 0)
			    {
				if((AllocLen = Seek(file, 0,
						    OFFSET_BEGINNING)) > 0)
				{
				    if((cdt->DTCDChunk = AllocVec(AllocLen,
								  MEMF_PUBLIC | MEMF_CLEAR)))
				    {
					cdt->DTCDSize = AllocLen;

					if(Read(file, cdt->DTCDChunk, AllocLen) == AllocLen)
					{
					    HookBuffer = cdt->DTCDChunk;
					    HookBufSize = cdt->DTCDSize;
					    HookPosition = 0;

					    /* We use a cast to BPTR, since sv may not
					     * be ULONG aligned. Since only our ReadFunc
					     * is going to be looking at it, this is ok.
					     */
					    if((SegList = InternalLoadSeg((BPTR)(sv), BNULL, (LONG_FUNC *)FunctionArray, &DefaultStack)))
					    {
						cdt->SegList = SegList;
						cdt->Function = BADDR(SegList) + sizeof(BPTR);  // FIXME: is this portable?
					    }
					}
					else
					{
					    FreeVec(cdt->DTCDChunk);
					    cdt->DTCDChunk = NULL;
					    cdt->DTCDSize = 0;
					}
				    }
				}
			    }
			}
			cdt->DT.dtn_FunctionName=NULL;
		    }
		    
		    if(cdt->DTH.dth_MaskLen > DTList->dtl_LongestMask)
		    {
			DTList->dtl_LongestMask = cdt->DTH.dth_MaskLen;
		    }
		    
		    cdt->DT.dtn_Node1.ln_Pri = cdt->DTH.dth_Priority;
		    Enqueue(typelist, &cdt->DT.dtn_Node1);
		    
		    AlphaInsert(sv, &DTList->dtl_SortedList, &cdt->DT.dtn_Node2);
		}
	    }
	}
    }
    
    if(!Success)
    {
	DeleteDataType(sv, cdt);
	cdt = NULL;
    }
    
    return cdt;
}



/****** AddDataTypes/DeleteDataType *******************************************
*
*   NAME
*        DeleteDataType - unlink and deallocate a CompoundDataType structure
*
*   SYNOPSIS
*
*   FUNCTION
*
*   INPUTS
*
*   RETURNS
*
*   EXAMPLE
*
*   SEE ALSO
*
******************************************************************************
*
*/

void DeleteDataType(struct StackVars *sv, struct CompoundDataType *cdt)
{
    if(cdt)
    {
	if(cdt->ParsePatMem)
	{
	    FreeVec(cdt->ParsePatMem);
	    cdt->ParsePatMem = NULL;
	    cdt->ParsePatSize = 0;
	}
	
	if(cdt->DTCDChunk)
	{
	    FreeVec(cdt->DTCDChunk);
	    cdt->DTCDChunk = NULL;
	    cdt->DTCDSize = 0;
	}
		
	if(cdt->SegList)
	{
	    UnLoadSeg(cdt->SegList);
	    cdt->SegList = BNULL;
	    cdt->Function = NULL;
	}
		
	if(cdt->DT.dtn_Node1.ln_Succ && cdt->DT.dtn_Node1.ln_Pred)
	{
	    Remove(&cdt->DT.dtn_Node1);
	    Remove(&cdt->DT.dtn_Node2);
	    cdt->DT.dtn_Node1.ln_Succ = cdt->DT.dtn_Node1.ln_Pred =
		cdt->DT.dtn_Node2.ln_Succ = cdt->DT.dtn_Node2.ln_Pred = NULL;
	}

	FreeVec(cdt);
    }
}



/****** AddDataTypes/AlphaInsert **********************************************
*
*   NAME
*        AlphaInsert - enqueue a node alphabetically into a list 
*
*   SYNOPSIS
*
*   FUNCTION
*
*   INPUTS
*
*   RETURNS
*
*   EXAMPLE
*
*   SEE ALSO
*
******************************************************************************
*
*/

void AlphaInsert(struct StackVars *sv, struct List *list, struct Node *node)
{
    struct Node *cur,*prev=NULL;
    
    for(cur = list->lh_Head; cur->ln_Succ; prev = cur, cur = cur->ln_Succ)
    {
	if(Stricmp(cur->ln_Name, node->ln_Name) > 0)
	    break;
    }

    Insert(list, node, prev);
}



/****** AddDataTypes/__FindNameNoCase *****************************************
*
*   NAME
*        __FindNameNoCase - find a node in a list (case insensitive)
*
*   SYNOPSIS
*
*   FUNCTION
*
*   INPUTS
*
*   RETURNS
*
*   EXAMPLE
*
*   SEE ALSO
*
******************************************************************************
*
*/

struct Node *__FindNameNoCase(struct StackVars *sv, struct List *list,
			      STRPTR name)
{
    struct Node *node;
    struct Node *result = NULL;
    
    for(node = list->lh_Head; node->ln_Succ; node = node->ln_Succ)
    {
	if(!Stricmp(node->ln_Name, name))
	{
	    result = node;
	    break;
	}
    }

    return result;
}



/****** AddDataTypes/ReadFunc *************************************************
*
*   NAME
*        ReadFunc - data read hook for InternalLoadSeg
*
*   SYNOPSIS
*
*   FUNCTION
*
*   INPUTS
*
*   RETURNS
*
*   EXAMPLE
*
*   SEE ALSO
*
******************************************************************************
*
*/

AROS_UFH4(LONG, AROS_SLIB_ENTRY(ReadFunc, AddDataTypes, 0),
	AROS_UFHA(BPTR   , fh        , D1),
	AROS_UFHA(void * , buffer    , D2),
	AROS_UFHA(LONG   , length    , D3),
	AROS_UFHA(struct DosLibrary *, DOSBase, A6))
{
    AROS_USERFUNC_INIT

    struct StackVars *sv = (APTR)fh;
    LONG maxlen = HookBufSize-HookPosition;
    LONG actual = length > maxlen ? maxlen : length;
    
    CopyMem(HookBuffer+HookPosition, buffer, actual);
	
    HookPosition += actual;
    
    return actual;

    AROS_USERFUNC_EXIT
}

/****** AddDataTypes/SeekFunc *************************************************
*
*   NAME
*        SeekFunc - seek hook for InternalLoadSeg (ELF only)
*
*   SYNOPSIS
*
*   FUNCTION
*
*   INPUTS
*
*   RETURNS
*
*   EXAMPLE
*
*   SEE ALSO
*
******************************************************************************
*
*/

AROS_UFH4(LONG, AROS_SLIB_ENTRY(SeekFunc, AddDataTypes, 0),
	AROS_UFHA(BPTR   , fh        , D1),
	AROS_UFHA(LONG   , pos       , D2),
	AROS_UFHA(LONG   , mode      , D3),
	AROS_UFHA(struct DosLibrary *, DOSBase, A6))
{
    AROS_USERFUNC_INIT

    struct StackVars *sv = (APTR)fh;
    LONG oldpos = HookPosition;

    switch (mode) {
    case OFFSET_BEGINNING: break;
    case OFFSET_END:       pos = HookBufSize - pos; break;
    case OFFSET_CURRENT:   pos = HookPosition + pos; break;
    default: return -1;
    }
   
    if (pos < 0 || pos >= HookBufSize)
    	return -1;

    HookPosition = pos;
    
    return oldpos;

    AROS_USERFUNC_EXIT
}

/****** AddDataTypes/AllocFunc ************************************************
*
*   NAME
*        AllocFunc - memory allocation hook for InternalLoadSeg
*
*   SYNOPSIS
*
*   FUNCTION
*
*   INPUTS
*
*   RETURNS
*
*   EXAMPLE
*
*   SEE ALSO
*
******************************************************************************
*
*/

AROS_UFH3(UBYTE *, AROS_SLIB_ENTRY(AllocFunc, AddDataTypes, 0),
	AROS_UFHA(ULONG, size, D0),
	AROS_UFHA(ULONG, flags,D1),
	AROS_UFHA(struct DosLibrary *, DOSBase, A6))
{
    AROS_USERFUNC_INIT

    return(AllocMem(size, flags));

    AROS_USERFUNC_EXIT
}



/****** AddDataTypes/FreeFunc *************************************************
*
*   NAME
*        FreeFunc - memory freeing hook for InternalLoadSeg
*
*   SYNOPSIS
*
*   FUNCTION
*
*   INPUTS
*
*   RETURNS
*
*   EXAMPLE
*
*   SEE ALSO
*
******************************************************************************
*
*/

AROS_UFH3(void, AROS_SLIB_ENTRY(FreeFunc, AddDataTypes, 0),
	AROS_UFHA(APTR , memory, A1),
	AROS_UFHA(ULONG, size  , D0),
	AROS_UFHA(struct DosLibrary *, DOSBase, A6))
{
    AROS_USERFUNC_INIT

    FreeMem(memory, size);

    AROS_USERFUNC_EXIT
}



/******************************* STUB ROUTINES ********************************/

struct NamedObject *allocnamedobject(struct StackVars *sv, STRPTR name,
				     Tag FirstTag, ...)
{
    return AllocNamedObjectA(name, (struct TagItem*)&FirstTag);
}
