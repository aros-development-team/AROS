
#define USE_BOOPSI_STUBS
#include <datatypes/datatypes.h>
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



struct DataTypesList *GetDataTypesList(struct DataTypesBase *DTBase)
{
    struct NamedObject   *no;
    struct DataTypesList *dtl = NULL;
    
    if((no = FindNamedObject(NULL, DATATYPESLIST, NULL)))
    {
	ReleaseNamedObject(no);
	dtl = (struct DataTypesList *)no->no_Object;
    }
    
    return(dtl);
}


struct Node *FindNameNoCase(struct Library *DTBase, struct List *list,
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



BPTR NewOpen(struct Library *DTBase, STRPTR name, ULONG SourceType,
	     ULONG Length)
{
    BPTR returnfh = NULL;
    //    struct XpkFib *xpkfib=NULL;
    BPTR dosfile;
	
    if((dosfile = Open(name, MODE_OLDFILE)))
    {
	returnfh = dosfile;

#if 0
	if(XpkBase)
	{
	    if(xpkfib = AllocVec(sizeof(struct XpkFib), 
				  MEMF_PUBLIC|MEMF_CLEAR))
	    {
		if(!xpkexaminetags(DTBase, xpkfib, XPK_InFH, dosfile,
				   TAG_DONE))
		{
		    switch (xpkfib->Type)
		    {
		    case XPKTYPE_UNPACKED:
			returnfh=dosfile;
			break;
			
		    case XPKTYPE_PACKED:
			Message(DTBase,"file is XPK packed","okay");
			
			if (xpkfib->Flags&XPKFLAGS_PASSWORD)
			    Message(DTBase,"file needs password","okay");
			
			if (xpkfib->Flags&XPKFLAGS_NOSEEK)
			    Message(DTBase,"file does not support seeking",
				    "okay");
			if (xpkfib->Flags&XPKFLAGS_NONSTD)
			    Message(DTBase,"file is non standard","okay");

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




#define getDTLIST (GPB(DTBase)->dtb_DTList)

struct CompoundDatatype *ExamineLock(BPTR lock, struct FileInfoBlock *fib,
				     struct Library *DTBase)
{
    struct CompoundDatatype *cdt = NULL;
    
    ObtainSemaphoreShared(&getDTLIST->dtl_Lock);
    
    if(Examine(lock, fib))
    {
	if (fib->fib_DirEntryType > 0)
	{
	    cdt = (struct CompoundDatatype *)FindNameNoCase(DTBase, 
							    &getDTLIST->dtl_MiscList,
							    "directory");
	}
	else
	{
	    if (fib->fib_DirEntryType < 0)
	    {
		UBYTE namebuf[510];
		
		if (NameFromLock(lock, namebuf, sizeof(namebuf)))
		{
		    BPTR file;
		    
		    if((file = NewOpen(DTBase, namebuf, DTST_FILE, 0)))
		    {
			UBYTE *CheckArray;
			UWORD CheckSize = (getDTLIST->dtl_LongestMask > 64) ?
			    getDTLIST->dtl_LongestMask : 64;
			
			if((CheckArray = AllocVec((ULONG)(CheckSize)+1,
						  MEMF_CLEAR)))
			{
			    if((CheckSize = Read(file, CheckArray, 
						 (ULONG)CheckSize)) > 0)
			    {
				struct DTHookContext dthc;
				struct IFFHandle *iff;
				
				Seek(file, 0, OFFSET_BEGINNING);
				
				dthc.dthc_SysBase = (struct Library *)SysBase;
				dthc.dthc_DOSBase = DOSBase;
				dthc.dthc_IFFParseBase = IFFParseBase;
				dthc.dthc_UtilityBase = UtilityBase;
				dthc.dthc_Lock = lock;
				dthc.dthc_FIB = fib;
				dthc.dthc_FileHandle = file;
				dthc.dthc_Buffer = CheckArray;
				dthc.dthc_BufferLength = CheckSize;
				
				if(!(iff=dthc.dthc_IFF = AllocIFF()))
				    SetIoErr(ERROR_NO_FREE_STORE);
				else
				{
				    iff->iff_Stream = (IPTR)file;   /* Hmm? */
				    InitIFFasDOS(iff);
				    
				    if (!OpenIFF(iff, IFFF_READ))
				    {
					cdt = ExamineData(DTBase,
							  &dthc,
							  CheckArray,
							  CheckSize,
							  fib->fib_FileName,
							  fib->fib_Size);
					
					CloseIFF(iff);
				    }
				    
				    FreeIFF(dthc.dthc_IFF);
				}
			    }
			    FreeVec(CheckArray);
			}
			
			Close(file);
		    } /* if file opened */
		} /* if I got the name from the lock */
	    }
	}
    }

    ReleaseSemaphore(&getDTLIST->dtl_Lock);
    
    return cdt;
}


struct CompoundDatatype *ExamineData(struct Library *DTBase,
				     struct DTHookContext *dthc,
				     UBYTE *CheckArray, UWORD CheckSize,
				     UBYTE *Filename, ULONG Size)
{
    struct CompoundDatatype *cdt = NULL;
    
    UWORD        type;
    struct List *list  = NULL;
    BOOL         found = FALSE;
   
    ULONG IFF_ID   = *((ULONG*)CheckArray);
    ULONG IFF_Size = *((ULONG*)(CheckArray+4));
    
    if(((!dthc->dthc_FileHandle) && (dthc->dthc_IFF)) ||
       (((Size*3/4 < IFF_Size) && (Size*4/3 > IFF_Size)) &&
	(IFF_ID==ID_FORM || IFF_ID==ID_CAT || IFF_ID==ID_LIST)) )
    {
	type = DTF_IFF;
	list = &getDTLIST->dtl_IFFList;
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
	
	if (ascii > CheckSize*3/4)
	{
	    type = DTF_ASCII;
	    list = &getDTLIST->dtl_ASCIIList;
	}
	else
	{
	    type = DTF_BINARY;
	    list = &getDTLIST->dtl_BinaryList;
	}
    }
    
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
	
	if(!found)
	{
	    switch(type)
	    {
	    case DTF_BINARY:
		cdt = (struct CompoundDatatype *)FindNameNoCase(DTBase, list,
								"binary");
		break;
		
	    case DTF_ASCII:
		cdt = (struct CompoundDatatype *)FindNameNoCase(DTBase, list,
							       "ascii");
		break;
		
	    case DTF_IFF:
		cdt = (struct CompoundDatatype *)FindNameNoCase(DTBase, list,
								"iff");
		break;
	    }
	}
    }

    return cdt;
}


#undef getDTLIST


/* Putchar procedure needed by RawDoFmt() */

AROS_UFH2(void, putchr,
    AROS_UFHA(UBYTE,    chr, D0),
    AROS_UFHA(STRPTR *, p,   A3))
{
    AROS_LIBFUNC_INIT
    *(*p)++=chr;
    AROS_LIBFUNC_EXIT
}


void dt_sprintf(struct Library *DTBase, UBYTE *buffer, UBYTE *format, ...)
{
    RawDoFmt(format, &format+1, (VOID_FUNC)putchr, &buffer);
}


ULONG setattrs(struct Library *DTBase, Object *object, Tag firstTag,...)
{
    return SetAttrsA(object, (struct TagItem *)&firstTag);
}


ULONG Do_OM_NOTIFY(struct Library *DTBase, Object *object,
		   struct GadgetInfo *ginfo, ULONG flags, Tag firstTag,...)
{
    return DoMethod(object, OM_NOTIFY, &firstTag, ginfo, flags);
}


ULONG DoGad_OM_NOTIFY(struct Library *DTBase, Object *object,
		      struct Window *win, struct Requester *req,
		      ULONG flags, Tag firstTag, ...)
{
    return(dogadgetmethod(DTBase, (struct Gadget*)object, win, req, OM_NOTIFY,
			  &firstTag, NULL, flags));
}


ULONG dogadgetmethod(struct Library *DTBase, struct Gadget *gad,
		     struct Window *win, struct Requester *req,
		     ULONG MethodID, ...)
{
    return(DoGadgetMethodA(gad, win, req, (Msg)&MethodID));
}

struct Catalog *opencatalog(struct Library *DTBase, struct Locale *locale,
			    STRPTR name, Tag firstTag, ...)
{
    return(OpenCatalogA(locale, name, (struct TagItem *)&firstTag));
}
