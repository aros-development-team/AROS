/*
    Copyright � 1995-2010, The AROS Development Team. All rights reserved.
    $Id$
*/

#define USE_BOOPSI_STUBS
#include <aros/macros.h>
#include <datatypes/datatypes.h>
#include <proto/alib.h>
#include <proto/utility.h>
#include <proto/dos.h>
#include <proto/iffparse.h>
#include <proto/intuition.h>
#include <proto/locale.h>
#include <utility/name.h>
#include <intuition/cghooks.h>
#include <libraries/locale.h>
#include <datatypes/datatypesclass.h>
#include "datatypes_intern.h"
#include <clib/boopsistubs.h>

#include <aros/debug.h>

/************************** ASCII/BINARY RECOGNITION *************************/

UBYTE const ASCIITable[256]=
{
   0,0,0,0,0,0,0,1,1,1,1,1,1,1,0,0,0,1,0,1,0,0,0,0,0,0,0,1,0,0,0,0,
   1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
   1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
   1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
};


UBYTE const BinaryTable[256]=
{
   1,1,1,1,1,1,1,0,0,0,0,0,0,0,1,1,1,0,1,0,1,1,1,1,1,1,1,0,1,1,1,1,
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
   1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,1,1,1,1,
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
};



struct DataTypesList *GetDataTypesList(struct DataTypesBase *DataTypesBase)
{
    struct NamedObject   *no;
    struct DataTypesList *dtl = NULL;
    
    if((no = FindNamedObject(NULL, DATATYPESLIST, NULL)))
    {
	dtl = (struct DataTypesList *)no->no_Object;
    }

    if(!dtl)
    {
    	struct TagItem tags[] =
	{
	    {ANO_NameSpace  , TRUE  	    	    	    },
	    {ANO_UserSpace  , sizeof(struct DataTypesList)  },
	    {ANO_Flags	    , NSF_NODUPS | NSF_CASE 	    },
	    {TAG_DONE	    	    	    	    	    }
	};
	
	if((no = AllocNamedObjectA(DATATYPESLIST, tags)))
	    {
		if(!(dtl = (struct DataTypesList*)no->no_Object))
		{
		    FreeNamedObject(no);
		    no = NULL;
		}
	    }
	
	if(dtl)
	{
	    InitSemaphore(&dtl->dtl_Lock);
	    NewList(&dtl->dtl_SortedList);
	    NewList(&dtl->dtl_BinaryList);
	    NewList(&dtl->dtl_ASCIIList);
	    NewList(&dtl->dtl_IFFList);
	    NewList(&dtl->dtl_MiscList);
	    
	    if(!AddNamedObject(NULL, no))
	    {
		FreeNamedObject(no);
		no = NULL;
		dtl = NULL;
	    }
	}
    }

    if (no)
	ReleaseNamedObject(no);

    return(dtl);
}


struct Node *FindNameNoCase(struct Library *DataTypesBase, struct List *list,
			    STRPTR name)
{
    struct Node *node;
    struct Node *result = NULL;
    
    for (node = list->lh_Head; node->ln_Succ; node = node->ln_Succ)
    {
	if (!Stricmp(node->ln_Name, name))
	{
	    result = node;
	    break;
	}
    }

    return result;
}



BPTR NewOpen(struct Library *DataTypesBase, STRPTR name, ULONG SourceType,
	     ULONG Length)
{
    BPTR returnfh = NULL;
    //    struct XpkFib *xpkfib=NULL;
    BPTR dosfile;

    D(bug("datatypes.library/NewOpen: name = %s\n", name));
	
    if((dosfile = Open(name, MODE_OLDFILE)))
    {
	D(bug("datatypes.library/NewOpen: open okay\n"));

	returnfh = dosfile;

#if 0
	if(XpkBase)
	{
	    if(xpkfib = AllocVec(sizeof(struct XpkFib), 
				  MEMF_PUBLIC|MEMF_CLEAR))
	    {
		if(!xpkexaminetags(DataTypesBase, xpkfib, XPK_InFH, dosfile,
				   TAG_DONE))
		{
		    switch (xpkfib->Type)
		    {
		    case XPKTYPE_UNPACKED:
			returnfh=dosfile;
			break;
			
		    case XPKTYPE_PACKED:
			Message(DataTypesBase,"file is XPK packed","okay");
			
			if (xpkfib->Flags&XPKFLAGS_PASSWORD)
			    Message(DataTypesBase,"file needs password","okay");
			
			if (xpkfib->Flags&XPKFLAGS_NOSEEK)
			    Message(DataTypesBase,"file does not support seeking",
				    "okay");
			if (xpkfib->Flags&XPKFLAGS_NONSTD)
			    Message(DataTypesBase,"file is non standard","okay");

			SetIoErr(ERROR_NOT_IMPLEMENTED);
			break;
			
		    case XPKTYPE_ARCHIVE:
			SetIoErr(ERROR_NOT_IMPLEMENTED);
			break;
		    }
		}
		FreeVec(xpkfib);
	    }
	}
	if (returnfh != dosfile)
	{
	    Close(dosfile);
	}
#endif

    }

    return returnfh;
}




#define getDTLIST (GPB(DataTypesBase)->dtb_DTList)

struct CompoundDatatype *ExamineLock(BPTR lock, struct FileInfoBlock *fib,
				     struct Library *DataTypesBase)
{
    struct CompoundDatatype *cdt = NULL;

    D(bug("datatypes.library/ExamineLock\n"));
    
    ObtainSemaphoreShared(&getDTLIST->dtl_Lock);

    if(Examine(lock, fib))
    {
        D(bug("datatypes.library/ExamineLock: Examine okay\n"));
	if (fib->fib_DirEntryType > 0)
	{
   	    D(bug("datatypes.library/ExamineLock: is a directory\n"));
	    cdt = (struct CompoundDatatype *)FindNameNoCase(DataTypesBase, 
							    &getDTLIST->dtl_MiscList,
							    "directory");
	}
	else
	{
	    if (fib->fib_DirEntryType < 0)
	    {
		UBYTE namebuf[510];
		
    		D(bug("datatypes.library/ExamineLock: is a file\n"));
		
		if (NameFromLock(lock, namebuf, sizeof(namebuf)))
		{
		    BPTR file;
		    
    		    D(bug("datatypes.library/ExamineLock: NameFromLock okay. Name = \"%s\"\n", namebuf));
		    
		    if((file = NewOpen(DataTypesBase, namebuf, DTST_FILE, 0)))
		    {
			UBYTE *CheckArray;
			UWORD CheckSize = (getDTLIST->dtl_LongestMask > 64) ?
			    getDTLIST->dtl_LongestMask : 64;
			
    			D(bug("datatypes.library/ExamineLock: NewOpen okay\n"));

			if((CheckArray = AllocVec((ULONG)(CheckSize)+1,
						  MEMF_CLEAR)))
			{
   			    D(bug("datatypes.library/ExamineLock: Alloced CheckArray\n"));
			    
			    if((CheckSize = Read(file, CheckArray, 
						 (ULONG)CheckSize)) > 0)
			    {
				struct DTHookContext dthc;
				struct IFFHandle *iff;
				
    				D(bug("datatypes.library/ExamineLock: Read in CheckArray size = %d\n", CheckSize));
				
				Seek(file, 0, OFFSET_BEGINNING);
				
				dthc.dthc_SysBase = (struct Library *)SysBase;
				dthc.dthc_DOSBase = (struct Library *)DOSBase;
				dthc.dthc_IFFParseBase = IFFParseBase;
				dthc.dthc_UtilityBase = (struct Library *)UtilityBase;
				dthc.dthc_Lock = lock;
				dthc.dthc_FIB = fib;
				dthc.dthc_FileHandle = file;
				dthc.dthc_Buffer = CheckArray;
				dthc.dthc_BufferLength = CheckSize;
				
				if(!(iff=dthc.dthc_IFF = AllocIFF()))
				    SetIoErr(ERROR_NO_FREE_STORE);
				else
				{
    				    D(bug("datatypes.library/ExamineLock: AllocIFF okay: iff = %x\n", iff));

				    iff->iff_Stream = (IPTR)file;   /* Hmm? */
				    InitIFFasDOS(iff);
				    
				    if (!OpenIFF(iff, IFFF_READ))
				    {
    					D(bug("datatypes.library/ExamineLock: OpenIFF okay. Now calling ExamineData\n"));

					cdt = ExamineData(DataTypesBase,
							  &dthc,
							  CheckArray,
							  CheckSize,
							  fib->fib_FileName,
							  fib->fib_Size);
					
    					D(bug("datatypes.library/ExamineLock: ExamineData() returned %x\n", cdt));

					CloseIFF(iff);
					
				    } /* OpenIFF okay */
				    
				    FreeIFF(iff); /* AROS BUG FIX: was dthc.dthc_IFF) */
				    
				} /* AllocIFF okay */
				
			    } /* if (CheckSize = Read(... */
			    		    
			    FreeVec(CheckArray);
			    
			} /* if (CheckArray = AllocVec(... */
			
			Close(file);
			
		    } /* if file opened */
		    
		} /* if I got the name from the lock */
		
	    } /* it is a file */
	    
	} /* it is not a directory */
	
    } /* if(Examine(lock, fib)) */

    ReleaseSemaphore(&getDTLIST->dtl_Lock);
    
    return cdt;
}


struct CompoundDatatype *FindDtInList(struct Library *DataTypesBase,
				      struct DTHookContext *dthc,
				      struct List *list,
				      UBYTE *CheckArray,
				      UWORD CheckSize,
				      UBYTE *Filename)
{
    struct CompoundDatatype *cdt = NULL;
    BOOL                   found = FALSE;

    if (list)
    {
	struct CompoundDatatype *cur;

	for(cur = (struct CompoundDatatype *)list->lh_Head; 
		cur->DT.dtn_Node1.ln_Succ;
		cur = (struct CompoundDatatype *)cur->DT.dtn_Node1.ln_Succ)
	{
	    if(CheckSize >= cur->DTH.dth_MaskLen)
	    {
		WORD *msk = cur->DTH.dth_Mask;
		UBYTE *cmp = CheckArray;
		UWORD count;

		found=TRUE;

		for(count = cur->DTH.dth_MaskLen; count--; msk++, cmp++)
		{
		    if(*msk>0)
		    {
			if(cur->DTH.dth_Flags & DTF_CASE)
			{
			    if (*msk != *cmp)
			    {
				found=FALSE;
				break;
			    }
			}
			else
			{
			    if(*msk != *cmp &&
				    *msk != ToUpper((ULONG)*cmp) &&
				    *msk != ToLower((ULONG)*cmp))
			    {
				found=FALSE;
				break;
			    }
			}
		    }
		}

		if(found)
		{
		    if((!(cur->FlagLong & CFLGF_PATTERN_UNUSED)) &&
			    cur->DTH.dth_Pattern)
		    {
			if(cur->FlagLong & CFLGF_IS_WILD)
			{
			    if(cur->ParsePatMem)
			    {
				if(!MatchPatternNoCase(cur->ParsePatMem, 
					    Filename))
				{
				    found = FALSE;
				}
			    }
			}
			else
			{
			    if(Stricmp(cur->DTH.dth_Pattern, Filename))
			    {
				found = FALSE;
			    }
			}
		    }

		    if(found)
		    {
			if(cur->Function)
			{
			    found = (cur->Function)(dthc);

			    if (dthc->dthc_IFF)
			    {
				CloseIFF(dthc->dthc_IFF);
				OpenIFF(dthc->dthc_IFF, IFFF_READ);
			    }
			    else
			    {
				Seek(dthc->dthc_FileHandle, 0, 
					OFFSET_BEGINNING);
			    }
			}
		    }
		}
	    }
	    if(found)
	    {
		cdt = cur;
		break;
	    }
	}
    }
    return cdt;
}


struct CompoundDatatype *ExamineData(struct Library *DataTypesBase,
				     struct DTHookContext *dthc,
				     UBYTE *CheckArray, UWORD CheckSize,
				     UBYTE *Filename, ULONG Size)
{
    struct CompoundDatatype *cdt = NULL;
    struct CompoundDatatype *cdt_bin = NULL;
    struct CompoundDatatype *cdt_asc = NULL;

    UWORD        type;
/* Used only below
    struct List *list  = NULL;*/

    ULONG IFF_ID   = AROS_BE2LONG(*((ULONG*)CheckArray));
    ULONG IFF_Size = AROS_BE2LONG(*((ULONG*)(CheckArray+4)));

    if(((!dthc->dthc_FileHandle) && (dthc->dthc_IFF)) ||
	    (((Size*3/4 < IFF_Size) && (Size*4/3 > IFF_Size)) &&
	     (IFF_ID==ID_FORM || IFF_ID==ID_CAT || IFF_ID==ID_LIST)) )
    {
        D(bug("[ExamineData] IFF detected\n"));
	type = DTF_IFF;
	cdt = FindDtInList(DataTypesBase, dthc, &getDTLIST->dtl_IFFList, CheckArray, CheckSize, Filename);
    }
    else
    {
	UBYTE *ptr;
	UWORD count;
	UWORD ascii;

	dthc->dthc_IFF = NULL;

	for (ptr=CheckArray,count=CheckSize,ascii=0 ; count-- ; ptr++)
	{
	    if (ASCIITable[*ptr])
		ascii++;
	    else
	    {
		if (BinaryTable[*ptr])
		{
		    ascii=0;
		    break;
		}
	    }
	}

	D(bug("[ExamineData] ASCII characters: %u of %u\n", ascii, CheckSize));
	if (ascii > CheckSize*3/4)
	{
	    D(bug("[ExamineData] Recognized as ASCII\n"));
	    type = DTF_ASCII;
	    cdt_asc = FindDtInList(DataTypesBase, dthc, &getDTLIST->dtl_ASCIIList, CheckArray, CheckSize, Filename);
	    D(bug("[ExamineData] ASCII datatype: 0x%p\n", cdt_asc));
	    cdt = cdt_asc;
	    /* if the found datatype is 'only' ascii we have to look additionally in the binary list */
	    if (cdt_asc && !strcmp(cdt_asc->DTH.dth_Name, "ascii"))
	    {
	        D(bug("[ExamineData] Trying binary list\n"));
		cdt_bin = FindDtInList(DataTypesBase, dthc, &getDTLIST->dtl_BinaryList, CheckArray, CheckSize, Filename);
		D(bug("[[ExamineData] Binary datatype: 0x%p\n"));
		/* if we find in the binary list something which is better than just 'binary' we use it */
		if (cdt_bin && strcmp(cdt_bin->DTH.dth_Name, "binary"))
		{
		    cdt = cdt_bin;
		}
	    }
	}
	else
	{
	    D(bug("[ExamineData] Recognized as binary\n"));
	    type = DTF_BINARY;
	    cdt = FindDtInList(DataTypesBase, dthc, &getDTLIST->dtl_BinaryList, CheckArray, CheckSize, Filename);
	}
    }
    D(bug("[ExamineData] Found datatype 0x%p, type 0x%u\n", cdt, type));

/* The following caused crashes with empty datatypes lists. What was it supposed to to?
   list pointer is initialized to NULL and never changes - sonic

    if(!cdt)
    {
	switch(type)
	{
	    case DTF_BINARY:
		cdt = (struct CompoundDatatype *)FindNameNoCase(DataTypesBase, list,
			"binary");
		break;

	    case DTF_ASCII:
		cdt = (struct CompoundDatatype *)FindNameNoCase(DataTypesBase, list,
			"ascii");
		break;

	    case DTF_IFF:
		cdt = (struct CompoundDatatype *)FindNameNoCase(DataTypesBase, list,
			"iff");
		break;
	}
    }*/
    return cdt;
}


#undef getDTLIST


/* Putchar procedure needed by RawDoFmt() */

AROS_UFH2(void, putchr,
    AROS_UFHA(UBYTE,    chr, D0),
    AROS_UFHA(STRPTR *, p,   A3))
{
    AROS_USERFUNC_INIT
    *(*p)++=chr;
    AROS_USERFUNC_EXIT
}


#warning these work only with stack growing downwards and should therefore be fixed to use macros in utility/tagitem.h

#define AROS_TAGRETURNTYPE ULONG
ULONG setattrs(struct Library *DataTypesBase, Object *object, Tag firstTag,...)
{
    AROS_SLOWSTACKTAGS_PRE(firstTag)
    retval = SetAttrsA(object, (struct TagItem *)&firstTag);
    AROS_SLOWSTACKTAGS_POST
}
#undef  AROS_TAGRETURNTYPE

#define AROS_TAGRETURNTYPE ULONG
ULONG Do_OM_NOTIFY(struct Library *DataTypesBase, Object *object,
		   struct GadgetInfo *ginfo, ULONG flags, Tag firstTag,...)
{
/*    struct opUpdate opu;
    
    opu.MethodID = OM_NOTIFY;
    opu.opu_AttrList = (struct TagItem *)&firstTag;
    opu.opu_GInfo = ginfo;
    opu.opu_Flags = flags;
    
    return DoMethodA(object, (Msg)&opu); */
    
    AROS_SLOWSTACKTAGS_PRE(firstTag)
    retval = DoMethod(object, OM_NOTIFY, AROS_SLOWSTACKTAGS_ARG(firstTag), ginfo, flags);
    AROS_SLOWSTACKTAGS_POST
}
#undef  AROS_TAGRETURNTYPE


#define AROS_TAGRETURNTYPE ULONG
ULONG DoGad_OM_NOTIFY(struct Library *DataTypesBase, Object *object,
		      struct Window *win, struct Requester *req,
		      ULONG flags, Tag firstTag, ...)
{
//    return(dogadgetmethod(DataTypesBase, (struct Gadget*)object, win, req, OM_NOTIFY,
//			  &firstTag, NULL, flags));
    AROS_SLOWSTACKTAGS_PRE(firstTag)
    retval = DoGadgetMethod((struct Gadget*)object, win, req, OM_NOTIFY, AROS_SLOWSTACKTAGS_ARG(firstTag), NULL, flags);
    AROS_SLOWSTACKTAGS_POST
}
#undef  AROS_TAGRETURNTYPE


//ULONG dogadgetmethod(struct Library *DataTypesBase, struct Gadget *gad,
//		     struct Window *win, struct Requester *req,
//		     ULONG MethodID, ...)
//{
//    return(DoGadgetMethodA(gad, win, req, (Msg)&MethodID));
//}

#define AROS_TAGRETURNTYPE struct Catalog *
struct Catalog *opencatalog(struct Library *DataTypesBase, struct Locale *locale,
			    STRPTR name, Tag firstTag, ...)
{
//    return(OpenCatalogA(locale, name, (struct TagItem *)&firstTag));
    AROS_SLOWSTACKTAGS_PRE(firstTag)
    retval = OpenCatalogA(locale, name, AROS_SLOWSTACKTAGS_ARG(firstTag));
    AROS_SLOWSTACKTAGS_POST
}
#undef  AROS_TAGRETURNTYPE
