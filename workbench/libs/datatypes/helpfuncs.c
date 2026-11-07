/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.
*/

//#define DEBUG 1
#include <aros/debug.h>

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

#include <string.h>

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



/*
 * Internal: Return the shared DataTypesList named object, creating it when
 * absent.  Creation initializes its semaphore and the sorted, binary,
 * ASCII, IFF, and miscellaneous lists before registering the object.
 * DataTypesBase is the owning library.  The returned list belongs to the
 * shared named object; callers do not free it.  NULL indicates failure.
 */
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
            {ANO_NameSpace  , TRUE                          },
            {ANO_UserSpace  , sizeof(struct DataTypesList)  },
            {ANO_Flags      , NSF_NODUPS | NSF_CASE         },
            {TAG_DONE                                       }
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


/*
 * Internal: Find the first node whose name matches name without regard to
 * case in the supplied list.  DataTypesBase is not otherwise consulted.
 * Requires a valid list and name strings.  Returns a borrowed list node or
 * NULL; the caller does not acquire ownership of the node.
 */
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


/*
 * Internal: Open name for reading through DOS Open().  SourceType and Length
 * do not change the active implementation.  Returns a DOS file handle on
 * success or BNULL on failure.  The caller must Close() a returned handle.
 * This helper does not currently perform XPK decoding.
 */
BPTR NewOpen(struct Library *DataTypesBase, STRPTR name, ULONG SourceType,
             ULONG Length)
{
    BPTR returnfh = BNULL;
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

/*
 * Internal: Examine a DOS lock and locate a matching registered datatype.
 * The caller owns lock and the supplied FileInfoBlock.  For directories,
 * return the registered "directory" type.  For ordinary files, open the
 * path resolved from the lock and read the recognition prefix.  Only a
 * strictly positive signed Read() result enters ExamineData().  Pass the
 * prior datatype and group filter through to matching, close temporary
 * DOS resources, and release the shared datatype-list semaphore.
 * Returns a borrowed CompoundDataType or NULL.  A NULL result does not
 * necessarily set IoErr().  The caller must not free the returned object.
 */
struct CompoundDataType *ExamineLock(BPTR lock, struct FileInfoBlock *fib, const struct DataType *prevdt,
                                     ULONG grpid, struct Library *DataTypesBase)
{
    struct CompoundDataType *cdt = NULL;

    D(bug("datatypes.library/ExamineLock\n"));
    
    ObtainSemaphoreShared(&getDTLIST->dtl_Lock);

    if(Examine(lock, fib))
    {
        D(bug("datatypes.library/ExamineLock: Examine okay\n"));
        if (fib->fib_DirEntryType > 0)
        {
            D(bug("datatypes.library/ExamineLock: is a directory\n"));
            if (!prevdt)
            {
                cdt = (struct CompoundDataType *)FindNameNoCase(DataTypesBase,
                                                                &getDTLIST->dtl_MiscList,
                                                                "directory");
                if (cdt && grpid && cdt->DTH.dth_GroupID != grpid)
                    cdt = NULL;
            }
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
                        LONG ReadSize;
                        UWORD CheckSize = (getDTLIST->dtl_LongestMask > 64) ?
                            getDTLIST->dtl_LongestMask : 64;
                        
                        D(bug("datatypes.library/ExamineLock: NewOpen okay\n"));

                        if((CheckArray = AllocVec((ULONG)(CheckSize)+1,
                                                  MEMF_CLEAR)))
                        {
                            D(bug("datatypes.library/ExamineLock: Allocated CheckArray\n"));
                            
                            if((ReadSize = Read(file, CheckArray,
                                                (ULONG)CheckSize)) > 0)
                            {
                                struct DTHookContext dthc;
                                struct IFFHandle *iff;

                                CheckSize = (UWORD)ReadSize;
                                
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
                                                          prevdt,
                                                          grpid,
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


/*
 * Internal: Search a datatype category list using the supplied data prefix,
 * filename, optional previous datatype, and group filter.  A non-NULL
 * prevdt skips list entries through that exact entry; a nonzero grpid
 * excludes descriptors from other groups.  A zero-length mask with a
 * checker function is evaluated once, not followed by a second validation
 * of the same descriptor.  Masked matches may additionally check the name
 * pattern and an optional function; that function's stream is reset for
 * further matching.  Returns a borrowed descriptor or NULL.
 * The caller owns the surrounding datatype-list synchronization.
 */
struct CompoundDataType *FindDtInList(struct Library *DataTypesBase,
                                      struct DTHookContext *dthc,
                                      struct List *list,
                                      const struct DataType *prevdt, ULONG grpid,
                                      UBYTE *CheckArray,
                                      UWORD CheckSize,
                                      UBYTE *Filename)
{
    struct CompoundDataType *cdt = NULL;
    BOOL                   found = FALSE;

    if (list)
    {
        struct CompoundDataType *cur;

        for(cur = (struct CompoundDataType *)list->lh_Head;
                cur->DT.dtn_Node1.ln_Succ;
                cur = (struct CompoundDataType *)cur->DT.dtn_Node1.ln_Succ)
        {
            if (prevdt)
            {
                if (&cur->DT == prevdt)
                    prevdt = NULL;
                continue;
            }

            if (grpid && cur->DTH.dth_GroupID != grpid)
                continue;
            
            if (!found && !(cur->DTH.dth_MaskLen) && (cur->Function))
            {
                D(bug("[FindDtInList] *** Calling %s Match Function @ 0x%p\n", cur->DT.dtn_Node1.ln_Name, cur->Function));
                found = (cur->Function)(dthc);
            }

            else if (CheckSize >= cur->DTH.dth_MaskLen)
            {
                WORD *msk = cur->DTH.dth_Mask;
                UBYTE *cmp = CheckArray;
                UWORD count;

                found=TRUE;

                for(count = cur->DTH.dth_MaskLen; count--; msk++, cmp++)
                {
                    if(*msk >= 0)
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
                            D(bug("[FindDtInList] *** Calling %s Validation Function @ 0x%p\n", cur->DT.dtn_Node1.ln_Name, cur->Function));

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


/*
 * Internal: Classify the supplied prefix and search the applicable datatype
 * lists.  IFF signatures and the supplied IFF context select the IFF list;
 * otherwise the ASCII/binary heuristic selects the initial list, with a
 * binary-specific search after the generic ASCII descriptor where required.
 * prevdt and grpid constrain continuation and group matching.  CheckArray
 * must provide at least eight readable bytes for the initial IFF header
 * inspection even if CheckSize is smaller.  The data buffer and DTHookContext
 * remain caller-owned.  Returns a borrowed CompoundDataType or NULL; this
 * helper does not acquire or release the shared datatype-list semaphore.
 */
struct CompoundDataType *ExamineData(struct Library *DataTypesBase,
                                     struct DTHookContext *dthc,
                                     const struct DataType *prevdt, ULONG grpid,
                                     UBYTE *CheckArray, UWORD CheckSize,
                                     UBYTE *Filename, ULONG Size)
{
    struct CompoundDataType *cdt = NULL;
    struct CompoundDataType *cdt_bin = NULL;
    struct CompoundDataType *cdt_asc = NULL;
    UWORD prevtype = prevdt && prevdt->dtn_Header ?
        prevdt->dtn_Header->dth_Flags & DTF_TYPE_MASK : 0;

    D(UWORD        type);

    ULONG IFF_ID   = AROS_BE2LONG(*((ULONG*)CheckArray));
    ULONG IFF_Size = AROS_BE2LONG(*((ULONG*)(CheckArray+4)));

    if(((!dthc->dthc_FileHandle) && (dthc->dthc_IFF)) ||
        (((IFF_Size + 8 == Size) && (Size > 21)) &&
        (IFF_ID==ID_FORM || IFF_ID==ID_CAT || IFF_ID==ID_LIST)) )
    {
        D(bug("[ExamineData] IFF detected\n"));
        D(type = DTF_IFF);
        if (!prevdt || prevtype == DTF_IFF)
            cdt = FindDtInList(DataTypesBase, dthc, &getDTLIST->dtl_IFFList,
                               prevdt, grpid, CheckArray, CheckSize, Filename);
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
            D(type = DTF_ASCII);
            if (!prevdt || prevtype == DTF_ASCII)
            {
                cdt_asc = FindDtInList(DataTypesBase, dthc, &getDTLIST->dtl_ASCIIList,
                                       prevdt, grpid, CheckArray, CheckSize, Filename);
                D(bug("[ExamineData] ASCII datatype: 0x%p\n", cdt_asc));
                cdt = cdt_asc;

                /* After ASCII matches, continue with more specific binary matches. */
                if (!cdt_asc || !strcmp(cdt_asc->DTH.dth_Name, "ascii"))
                {
                    D(bug("[ExamineData] Trying binary list\n"));
                    cdt_bin = FindDtInList(DataTypesBase, dthc, &getDTLIST->dtl_BinaryList,
                                           NULL, grpid, CheckArray, CheckSize, Filename);
                    D(bug("[[ExamineData] Binary datatype: 0x%p\n"));
                    if (cdt_bin && strcmp(cdt_bin->DTH.dth_Name, "binary"))
                        cdt = cdt_bin;
                }
            }
            else if (prevtype == DTF_BINARY)
            {
                cdt_bin = FindDtInList(DataTypesBase, dthc, &getDTLIST->dtl_BinaryList,
                                       prevdt, grpid, CheckArray, CheckSize, Filename);
                if (cdt_bin && strcmp(cdt_bin->DTH.dth_Name, "binary"))
                    cdt = cdt_bin;
            }
        }
        else
        {
            D(bug("[ExamineData] Recognized as binary\n"));
            D(type = DTF_BINARY);
            if (!prevdt || prevtype == DTF_BINARY)
                cdt = FindDtInList(DataTypesBase, dthc, &getDTLIST->dtl_BinaryList,
                                   prevdt, grpid, CheckArray, CheckSize, Filename);
        }
    }

    D(bug("[ExamineData] Found datatype 0x%p, type 0x%u\n", cdt, type));
    return cdt;
}


#undef getDTLIST




/*
 * Internal RawDoFmt() output callback: append chr at the byte pointer held
 * in p and advance that pointer.  The caller provides the destination
 * storage and sufficient capacity; this callback does not add a terminator.
 */
AROS_UFH2(void, putchr,
    AROS_UFHA(UBYTE,    chr, D0),
    AROS_UFHA(STRPTR *, p,   A3))
{
    AROS_USERFUNC_INIT
    *(*p)++=chr;
    AROS_USERFUNC_EXIT
}
