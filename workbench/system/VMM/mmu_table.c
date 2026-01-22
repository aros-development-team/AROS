#define DEBUG 1
#include <exec/types.h>
#include "defs.h"

static char rcsid [] = "$Id: mmu_table.c,v 3.7 95/12/16 18:36:57 Martin_Apel Exp $";

PRIVATE ULONG *MyRootTable;
PRIVATE BOOL RegsWritten = FALSE;
PRIVATE ULONG *MappedROM;
PRIVATE struct List MemRangeList;
PRIVATE BOOL MemRangeListUsed = FALSE;

#define ROMSTART 0xf80000
#define ROMSIZE  0x80000

struct MemRange
  {
  struct MinNode mr_Node;
  ULONG VirtStart,
        Length,
        PhysStart;
  UWORD Attr;
  };
        
/************************************************************************/

PRIVATE BOOL FindMemRange (ULONG StartAddress, ULONG *Length, 
                           ULONG *PhysStart, UWORD *Attr)

{
/* This tries to find a given memory range in the list. 
 * It returns the attributes and the physical start address
 * if an entry is found. Additionally it checks if the length
 * parameter is valid. If it's not it changes it accordingly
 * so this function can be called again with the next part
 * of the chunk.
 */

struct MemRange *range;

for (range = (struct MemRange*)MemRangeList.lh_Head; 
     range->mr_Node.mln_Succ != NULL;
     range = (struct MemRange*)range->mr_Node.mln_Succ)
  {
  if (range->VirtStart <= StartAddress && 
      range->VirtStart + range->Length > StartAddress)
    {
    *PhysStart = range->PhysStart + (StartAddress - range->VirtStart);
    *Attr = range->Attr;
    if (StartAddress + *Length > range->VirtStart + range->Length)
      *Length = range->VirtStart + range->Length - StartAddress;

    return (TRUE);
    }
  else if (range->VirtStart > StartAddress &&
           range->VirtStart < StartAddress + *Length)
    {
    *Length = range->VirtStart - StartAddress;
    return (TRUE);
    }
  }

return (FALSE);
}

/************************************************************************/

PRIVATE void *NewPointerTable (BOOL Small)

{
ULONG *PointerTable;
int i;

/* Mark all table entries as invalid. Each time we come across an
 * invalid table entry subsequently, a new pointer/page table is built
 */

if ((PointerTable = AllocAligned (POINTERS_PER_TABLE * sizeof (IPTR),
                  MEMF_PUBLIC | MEMF_REVERSE, POINTERTABALIGN, 0L)) == NULL)
  return (NULL);


  bug("[VMM-Handler] %s: PointerTable @ 0x%p\n", __func__, PointerTable);

for (i = 0; i < POINTERS_PER_TABLE; i++)
  *(PointerTable + i) = BUILD_DESCR (LOCUNUSED, PAGED_OUT, PT_TABLE);

if (!Small)
  MarkPage ((IPTR)PointerTable, NONCACHEABLE);

return (PointerTable);
}

/************************************************************************/

PRIVATE void *NewPageTable (BOOL Small)

{
ULONG *PageTable;
int i;
ULONG Size,
      Align;

if (Small)
  {
  Size = PAGES_PER_TABLE * sizeof (ULONG);
  Align = PAGETABALIGN;
  }
else
  {
  Size = PAGESIZE;
  Align = PAGEALIGN;
  }

if ((PageTable = AllocAligned (Size, MEMF_PUBLIC | MEMF_REVERSE,
                               Align, 0L)) == NULL)
  return (NULL);

for (i = 0; i < Size / sizeof (ULONG); i++)
  *(PageTable + i) = BUILD_DESCR (LOCUNUSED, PAGED_OUT, PT_PAGE);

if (!Small)
  MarkPage ((IPTR)PageTable, NONCACHEABLE);

return (PageTable);
}

/************************************************************************/

#define ALIGNED(addr,align) (((addr) & ((align) - 1)) == 0)

int MarkAddress (IPTR start, ULONG length, ULONG type, IPTR phys_addr,
                  BOOL Small)

/* Marks all pages starting from "start" to "start" + "length" to
 * be of type "type". Constructs intermediate pointer and page tables
 * if necessary. If type represents a resident page, 'phys_addr' is the
 * address, where the first page in this range maps to. 'phys_addr' is
 * then incremented for each page.
 * 'Small' decides, if the smallest possible MMU table is built or not.
 * For memory that will be paged, 'Small' must be set to FALSE.
 */
{
ULONG cur_addr;
ULONG root_index,
      table_index,
      page_index;
ULONG *PointerTable,
      *PageTable;

PRINT_DEB ("MarkAddress from address %08lx called", start);
PRINT_DEB ("                 length  %08lx", length);
PRINT_DEB ("                 type %lx", type);
PRINT_DEB ("Physical address %08lx", phys_addr);

#if !defined(__AROS__)
if (Small && MemRangeListUsed)
  {
  UWORD new_type;
  ULONG new_phys_addr;
  ULONG new_length;
  int rc;

  new_length = length;
  new_phys_addr = phys_addr;
  new_type = type;
  if (FindMemRange (start, &new_length, &new_phys_addr, &new_type))
    {
    if (new_length != length)
      {
      rc = MarkAddress (start + new_length, length - new_length, type, 
                        phys_addr + new_length, Small);
      if (rc != SUCCESS)
        return (rc);
      }
    phys_addr = new_phys_addr;
    type = new_type;
    length = new_length;
    PRINT_DEB ("New physical address is %lx", phys_addr);
    PRINT_DEB ("New length is %lx", length);
    PRINT_DEB ("New attributes are %lx", (ULONG)type);
    }
  }

cur_addr = start;
if (PAGE_INVALID_P (type))
  phys_addr = 0;              /* Needed for some tests */
else
  phys_addr = PAGEADDR (phys_addr);

while (cur_addr < start + length)
  {
  /* Check for valid root entry */
  root_index = ROOTINDEX (cur_addr);
  if (TABLE_INVALID_P (*(RootTable + root_index)))
    {
    if (Small)
      {
      /* Check if the current range is larger than a root table range */
      if (ALIGNED (cur_addr, POINTERS_PER_TABLE * PAGES_PER_TABLE * PAGESIZE) &&
          ALIGNED (phys_addr, POINTERS_PER_TABLE * PAGES_PER_TABLE * PAGESIZE) &&
          (cur_addr + POINTERS_PER_TABLE * PAGES_PER_TABLE * PAGESIZE <= 
          start + length))
        {
        if (PAGE_INVALID_P (type))
          {
          PRINT_DEB ("Making root table entry invalid for address range"
                     " starting at %08lx", cur_addr);
          *(RootTable + root_index) = INVALID;
          cur_addr += POINTERS_PER_TABLE * PAGES_PER_TABLE * PAGESIZE;
          continue;
          }
        else if (ProcessorType <= PROC_68030)
          {
          PRINT_DEB ("Using early-termination root table entry for "
                     "address range starting at %08lx", cur_addr);
          *(RootTable + root_index) = phys_addr | type;
          cur_addr += POINTERS_PER_TABLE * PAGES_PER_TABLE * PAGESIZE;
          phys_addr += POINTERS_PER_TABLE * PAGES_PER_TABLE * PAGESIZE;
          continue;
          }
        }
      }

    if ((PointerTable = NewPointerTable (Small)) == NULL)
      {
      PRINT_DEB ("No mem for pointer table", 0L);
      return (ERR_NOT_ENOUGH_MEM);
      }
    PRINT_DEB ("Allocated new pointer table for address range from %08lx", cur_addr);

    *(RootTable + root_index) = (ULONG)PointerTable | TABLE_RESIDENT;
    }
  else
    PointerTable = (ULONG*)ALIGN_DOWN(*(RootTable + root_index),
                                      POINTERTABALIGN);

  table_index = POINTERINDEX (cur_addr);

  /* Check for valid pointer entry */
  if (TABLE_INVALID_P (*(PointerTable + table_index)))
    {
    int i;
    ULONG *TableStart;

    if (Small)
      {
      if (ALIGNED (cur_addr, PAGES_PER_TABLE * PAGESIZE) &&
          ALIGNED (phys_addr, PAGES_PER_TABLE * PAGESIZE) &&
          (cur_addr + PAGES_PER_TABLE * PAGESIZE <= start + length))
        {
        if (PAGE_INVALID_P (type))
          {
          PRINT_DEB ("Making pointer table entry invalid for address range"
                     " starting at %08lx", cur_addr);
          *(PointerTable + table_index) = type | (PT_TABLE << 2) | INVALID;
          cur_addr += PAGES_PER_TABLE * PAGESIZE;
          continue;
          }
        else if (ProcessorType <= PROC_68030)
          {
          PRINT_DEB ("Using early-termination pointer table entry for "
                     "address range starting at %08lx", cur_addr);
          *(PointerTable + table_index) = phys_addr | type;
          cur_addr += PAGES_PER_TABLE * PAGESIZE;
          phys_addr += PAGES_PER_TABLE * PAGESIZE;
          continue;
          }
        }
      }

    if ((PageTable = NewPageTable (Small)) == NULL)
      {
      PRINT_DEB ("No mem for page table", 0L);
      return (ERR_NOT_ENOUGH_MEM);
      }

    PRINT_DEB ("Allocated new page table for address range from %08lx", cur_addr);

    if (!Small)
      {
      TableStart = PointerTable + ALIGN_DOWN (table_index,
                       PAGESIZE / sizeof (ULONG) / PAGES_PER_TABLE);

      for (i = 0; i < PAGESIZE / sizeof (ULONG) / PAGES_PER_TABLE; i++)
        {
        *(TableStart + i) = (ULONG)(PageTable + i * PAGES_PER_TABLE) | TABLE_RESIDENT;
        }
      }
    else
      *(PointerTable + table_index) = (ULONG)PageTable | TABLE_RESIDENT;
    }

  PageTable = (ULONG*) ALIGN_DOWN(*(PointerTable + table_index),
                                  PAGETABALIGN);

  page_index = PAGEINDEX (cur_addr);
  *(PageTable + page_index) = type;
  cur_addr += PAGESIZE;

  if (PAGE_RESIDENT_P (type))
    {
    *(PageTable + page_index) |= phys_addr;
    phys_addr += PAGESIZE;
    }
  else
    *(PageTable + page_index) |= (PT_PAGE << 2) | INVALID;
  }
#endif
return (SUCCESS);
}

/************************************************************************/

#define TT40_ENABLED (1L<<15)
#define TC40_ENABLED (1L<<15)
#define TC40_PAGESIZE (1L<<14)
#define TC40_PAGE_4K 0
#define TC40_PAGE_8K (1L<<14)

#define TT30_ENABLED (1L<<15)
#define TC30_ENABLED (1L<<15)
#define TC30_PAGE_4K 0
#define TC30_PAGE_8K (1L<<14)

/************************************************************************/

PRIVATE BOOL TTHit (ULONG address)

{
/* Determines if a given address hits into a transparent translation
 * register. 
 */

return ((BOOL)((*GenDescr) (address) & USED));   /* U bit used as TT hit indicator */
}

/************************************************************************/

PRIVATE int BuildMemRangeList (BPTR MMUConfigFile)

{
char line [80],
     buffer [20];
struct MemRange *range;

PRINT_DEB ("Reading MMU configuration from VMM_MMU.config", 0L);

MemRangeListUsed = TRUE;
while (FGets (MMUConfigFile, line, 79) != NULL)
  {
  ULONG log_start,
        phys_start,
        length,
        attr;

  StrToHex (line, &log_start);

  GetNthString (line, buffer, 2);
  StrToHex (buffer,  &phys_start);

  GetNthString (line, buffer, 3);
  StrToHex (buffer, &length);

  GetNthString (line, buffer, 4);
  StrToHex (buffer, &attr);

  if ((range = DoOrigAllocMem (sizeof (struct MemRange), MEMF_PUBLIC)) == NULL)
    return (ERR_NOT_ENOUGH_MEM);
  AddTail (&MemRangeList, (struct Node*)range);

  if ((log_start == 0) && (phys_start == 0) && (length == 0))
    {
    range->VirtStart = 0L;
    range->Length    = 0x80000000;
    range->PhysStart = 0L;
    range->Attr      = attr;

    if ((range = DoOrigAllocMem (sizeof (struct MemRange), MEMF_PUBLIC)) == NULL)
      return (ERR_NOT_ENOUGH_MEM);
    range->VirtStart = 0x80000000;
    range->Length    = 0x80000000;
    range->PhysStart = 0x80000000;
    range->Attr      = attr;
    AddTail (&MemRangeList, (struct Node*)range);
    continue;

    }

  /* Do just a small consistency check here */
#ifdef PAGE4K
  if (length == 0 || ((length % PAGESIZE) != 0))
#else
  if (length == 0 || ((length % (PAGESIZE / 2)) != 0))
#endif
    {
    Close (MMUConfigFile);
    return (ERR_CORRUPT_CFG_FILE);
    }

  range->VirtStart = log_start;
  range->Length    = length;
  range->PhysStart = phys_start;
  range->Attr      = attr;
  }

return (SUCCESS);
}

/************************************************************************/

int SetupMMUTable (void)

{
struct MemHeader *mem;
struct ConfigDev *cd=NULL;
ULONG PhysROMStart;
BOOL OutOfMem = FALSE;
BPTR MMUConfigFile;
struct MMUState40 MMUState40;
struct MMUState30 MMUState30;
ULONG *VectorTable;

NewList (&MemRangeList);

#if !defined(__AROS__)
if (IsA3000)
  {
  UBYTE *ForceReset = (UBYTE*)0xde0002;

  *ForceReset |= 0x80;
  }
#endif

if ((MMUConfigFile = Open (MMUCFG_FILENAME, MODE_OLDFILE)) != NULL)
  {
  int rc;

  bug("[VMM-Handler] %s: opened '%s'\n", __func__, MMUCFG_FILENAME);

  rc = BuildMemRangeList (MMUConfigFile);
  Close (MMUConfigFile);
  if (rc != SUCCESS)
    return (rc);
  }

#if !defined(__AROS__)
switch (ProcessorType)
  {
  case PROC_68040:
  case PROC_68060:
    ReadMMUState40 (&MMUState40);

    RootTable = (ULONG*)MMUState40.URP;

    /* Check if Zorro III autoconfiguration space is mapped by the 
     * transparent translation registers. If not there is no need
     * to set up our own MMU table unless the pagesize is different.
     */

    #if PAGESIZE==4096
      if (((MMUState40.TC & (TC40_ENABLED | TC40_PAGESIZE)) == (TC40_ENABLED | TC40_PAGE_4K)) && 
          !TTHit (EZ3_CONFIGAREA) && 
          (MMUConfigFile == NULL))
           return (SUCCESS);
    #else

      if (((MMUState40.TC & (TC40_ENABLED | TC40_PAGESIZE)) == (TC40_ENABLED | TC40_PAGE_8K)) && 
         !TTHit (EZ3_CONFIGAREA) &&
          (MMUConfigFile == NULL))
           return (SUCCESS);

    #endif

    break;

  case PROC_68030:
  case PROC_68851:
    if (ProcessorType == PROC_68030)
      ReadMMUState30 (&MMUState30);
    else
      ReadMMUState851 (&MMUState30);

    RootTable = (ULONG*)MMUState30.CRP_Lo;

    if ((MMUState30.TC == (MAKE_MASK(TC_E, TC_E, 1L ) |
                           MAKE_MASK( TC_SRE, TC_SRE, 0L ) |
                           MAKE_MASK( TC_FCL, TC_FCL, 0L ) |
                           MAKE_MASK( TC_PS_START, TC_PS_END, PAGE_BITS ) |
                           MAKE_MASK( TC_IS_START, TC_IS_END, 0L ) |
                           MAKE_MASK( TC_TIA_START, TC_TIA_END, LEVEL_A_BITS ) |
                           MAKE_MASK( TC_TIB_START, TC_TIB_END, LEVEL_B_BITS ) |
                           MAKE_MASK( TC_TIC_START, TC_TIC_END, LEVEL_C_BITS ) |
                           MAKE_MASK( TC_TID_START, TC_TID_END, 0L ))) &&
        !TTHit (EZ3_CONFIGAREA) &&
        (MMUConfigFile == NULL))
         return (SUCCESS);
    break;

#ifdef DEBUG
  default:
    PRINT_DEB ("SetupMMUTable: Unknown processor type", 0L);
    ColdReboot ();
#endif
  }
#endif

/* Allocate memory for the root table */

if ((MyRootTable = NewPointerTable (TRUE)) == NULL)
  {
  PRINT_DEB ("SetupMMUTable: No mem for root table", 0L);
  return (ERR_NOT_ENOUGH_MEM);
  }

RootTable = MyRootTable;

PRINT_DEB ("Initialized root table at %08lx", (ULONG) MyRootTable);

/* The following code is taken from the Enforcer documentation */

OutOfMem |= MarkAddress (0L, PAGESIZE, NONCACHEABLE | PAGE_RESIDENT, 0L, TRUE) != SUCCESS;

#if !defined(__AROS__)
/*
 * Map in the free memory
 */
Forbid();
mem=(struct MemHeader *)SysBase->MemList.lh_Head;
while (mem->mh_Node.ln_Succ)
  {
  ULONG CM;

  CM = (*GenDescr) ((ULONG)mem->mh_Lower) & CM_MASK;
  OutOfMem |= MarkAddress (PAGEADDR ((ULONG)mem->mh_Lower),
              (ULONG)mem->mh_Upper - PAGEADDR((ULONG)mem->mh_Lower),
              CM | PAGE_RESIDENT,
              PAGEADDR ((ULONG)mem->mh_Lower), TRUE) != SUCCESS;

  mem=(struct MemHeader *)(mem->mh_Node.ln_Succ);
  }
Permit();
#endif

PRINT_DEB ("RAM entered into MMU table", 0L);

/*
 * Now for the control areas...
 */
OutOfMem |= MarkAddress (0x00BC0000, 0x00040000, PAGE_RESIDENT | NONCACHEABLE,
                         0x00BC0000, TRUE) != SUCCESS;
OutOfMem |= MarkAddress (0x00D80000, 0x00080000, PAGE_RESIDENT | NONCACHEABLE,
                         0x00D80000, TRUE) != SUCCESS;
OutOfMem |= MarkAddress (0x00E80000, 0x00080000, PAGE_RESIDENT | NONCACHEABLE,
                         0x00E80000, TRUE) != SUCCESS;

PRINT_DEB ("Control areas entered into MMU table", 0L);

/*
 * Map in the autoconfig boards
 */
#if !defined(__AROS__)
while (cd=FindConfigDev(cd,-1L,-1L))
{
  /* Skip memory boards... */
  if (!(cd->cd_Rom.er_Type & ERTF_MEMLIST))
    {
    ULONG CM;

    PRINT_DEB ("Adding Autoconfig device at %08lx", (ULONG)cd->cd_BoardAddr);
    PRINT_DEB ("                  size      %08lx", cd->cd_BoardSize);

    CM = (*GenDescr) ((ULONG)(cd->cd_BoardAddr)) & CM_MASK;

    OutOfMem |= MarkAddress((ULONG)(cd->cd_BoardAddr), cd->cd_BoardSize,
                            PAGE_RESIDENT | CM,
                            (ULONG)(cd->cd_BoardAddr), TRUE) != SUCCESS;
    }
}

PRINT_DEB ("AutoConfig boards entered into MMU table", 0L);


/*
 * and the ROM...
 */

PhysROMStart = PAGEADDR ((*GenDescr) (ROMSTART));
PRINT_DEB ("Physical kickstart address = %08lx", PhysROMStart);

OutOfMem |= MarkAddress(ROMSTART, ROMSIZE,
                        PAGE_RESIDENT | CACHEABLE | WRITEPROTECT,
                        PhysROMStart, TRUE) != SUCCESS;

PRINT_DEB ("ROM entered into MMU table", 0L);

/*
 * If the credit card resource, make the addresses valid...
 */
if (OpenResource("card.resource"))
  {
  OutOfMem |= MarkAddress(0x00600000,0x00440000,
                          PAGE_RESIDENT | NONCACHEABLE, 0x00600000, TRUE) != SUCCESS;
  PRINT_DEB ("Entered card.resource into MMU table", 0L);
  }

/*
 * Check for ReKick/ZKick/KickIt
 */
if ((((IPTR)(SysBase->LibNode.lib_Node.ln_Name)) >> 16) == 0x20)
  {
  OutOfMem |= MarkAddress(0x00200000, 0x00080000,
                          PAGE_RESIDENT | CACHEABLE | WRITEPROTECT,
                          0x00200000, TRUE) != SUCCESS;
  }
#endif
  
/* OK, now the MMU table is set up like the one constructed by Enforcer.
 * Enable the MMU
 */

if (OutOfMem)
  {
  PRINT_DEB ("Not enough memory for pagetables", 0L);
  return (ERR_NOT_ENOUGH_MEM);
  }

PRINT_DEB ("MMU table ready for usage", 0L);

#if !defined(__AROS__)
CacheClearU ();
Disable ();
VectorTable = (ULONG*)ReadVBR ();
PRINT_DEB ("VBR = %lx", (ULONG)VectorTable);
OrigDynMMUTrap = (void (*) ()) *(VectorTable + 2);

switch (ProcessorType)
  {
  case PROC_68040:
  case PROC_68060:
    #if PAGESIZE==4096
      MMUState40.TC = TC40_ENABLED | TC40_PAGE_4K;
    #else
      MMUState40.TC = TC40_ENABLED | TC40_PAGE_8K;
    #endif

    MMUState40.URP = (ULONG)MyRootTable;
    MMUState40.SRP = (ULONG)MyRootTable;
    MMUState40.ITT0 = 
    MMUState40.ITT1 = 
    MMUState40.DTT0 = 
    MMUState40.DTT1 = 0L;

    SaveMMUState40 ();
    PRINT_DEB ("MMU state saved", 0L);
    SetMMUState40 (&MMUState40);
    PRINT_DEB ("MMU state set", 0L);

    if (MemRangeListUsed)    
      {
      if (ProcessorType == PROC_68040)
        {
        PRINT_DEB ("Installing dynamic MMU trap for 68040", 0L);
        *(VectorTable + 2) = (ULONG)&DynMMUTrap40;
        PRINT_DEB ("Trap-vector set", 0L);
        }
      else
        {
        PRINT_DEB ("Installing dynamic MMU trap for 68060", 0L);
        *(VectorTable + 2) = (ULONG)&DynMMUTrap60;
        PRINT_DEB ("Trap-vector set", 0L);
        }
      }
    break;

  case PROC_68030:
  case PROC_68851:
    if (MemRangeListUsed)    
      {
      PRINT_DEB ("Installing dynamic MMU trap for 68030/68851", 0L);
      *(VectorTable + 2) = (ULONG)&DynMMUTrap30;
      }

    MMUState30.TC = MAKE_MASK(TC_E, TC_E, 1L ) |
                    MAKE_MASK( TC_SRE, TC_SRE, 0L ) |
                    MAKE_MASK( TC_FCL, TC_FCL, 0L ) |
                    MAKE_MASK( TC_PS_START, TC_PS_END, PAGE_BITS ) |
                    MAKE_MASK( TC_IS_START, TC_IS_END, 0L ) |
                    MAKE_MASK( TC_TIA_START, TC_TIA_END, LEVEL_A_BITS ) |
                    MAKE_MASK( TC_TIB_START, TC_TIB_END, LEVEL_B_BITS ) |
                    MAKE_MASK( TC_TIC_START, TC_TIC_END, LEVEL_C_BITS ) |
                    MAKE_MASK( TC_TID_START, TC_TID_END, 0L );

    PRINT_DEB("TC30=%08lx", MMUState30.TC);

    MMUState30.CRP_Hi = MAKE_MASK( RP_LU-32, RP_LU-32, 1L ) |
                        MAKE_MASK( RP_LIMIT_START-32, RP_LIMIT_END-32, 0L ) |
                        MAKE_MASK( RP_DT_START-32, RP_DT_END-32, DT_VALID4BYTE );
    MMUState30.CRP_Lo = MAKE_MASK( RP_TA_START, RP_TA_END, ((ULONG)MyRootTable)
                                   >> TABLE_ADDRESS_SHIFT);

    PRINT_DEB("CRP30.HI=%08lx", MMUState30.CRP_Hi);
    PRINT_DEB("CRP30.LO=%08lx", MMUState30.CRP_Lo);

    MMUState30.SRP_Hi = MMUState30.CRP_Hi;
    MMUState30.SRP_Lo = MMUState30.CRP_Lo;
    /* TT registers are ignored for the 68851 */
    MMUState30.TT0 = 
    MMUState30.TT1 = 0;
 
    if (ProcessorType == PROC_68030)
      {
      SaveMMUState30 ();
      SetMMUState30(&MMUState30);
      }
    else
      {
      SaveMMUState851 ();
      SetMMUState851(&MMUState30);
      }
    break;

#ifdef DEBUG
  default:
    PRINT_DEB ("SetupMMUTable: Unknown processor type 2", 0L);
    ColdReboot ();
#endif
  }

CacheClearU ();
Enable ();
#endif
RegsWritten = TRUE;

PRINT_DEB ("MMU table fully set up", 0L);
return (SUCCESS);
}

/************************************************************************/

BOOL KillPageTable (ULONG *pt, BOOL FreePages)

{
int i;
BOOL PagesLocked = FALSE;

for (i = 0; i < PAGES_PER_TABLE; i++)
  {
  if (PAGE_RESIDENT_P (*(pt + i)))
    {
    if (LOCKED_P (*(pt + i)))
      PagesLocked = TRUE;
    else if (FreePages)
      FreeMem ((APTR)PAGEADDR (*(pt + i)), PAGESIZE);
    }
  }

if (PagesLocked)
  return (FALSE);

FreeMem (pt, PAGES_PER_TABLE * sizeof (ULONG));
return (TRUE);
}

/************************************************************************/

BOOL KillPointerTable (ULONG *pt, BOOL FreePages)

{
int i;
ULONG *address;
BOOL do_kill = TRUE;

for (i = 0; i < POINTERS_PER_TABLE; i++)
  {
  if (TABLE_RESIDENT_P (*(pt + i)))
    {
    address = (ULONG*) ALIGN_DOWN (*(pt + i), PAGETABALIGN);
    do_kill = KillPageTable (address, FreePages) && do_kill;
    }
  }

if (do_kill)
  {
  PRINT_DEB ("Freeing pointer table", 0L);
  FreeMem (pt, POINTERS_PER_TABLE * sizeof (ULONG));
  }
return (do_kill);
}

/************************************************************************/

void KillMMUTable (void)

{
int i;
ULONG *address;
struct MemRange *range;

#if !defined(__AROS__)
if (RegsWritten)
  {
  Disable ();

  if (IsA3000)
    {
    UBYTE *ForceReset = (UBYTE*)0xde0002;

    *ForceReset &= ~0x80;
    }

  switch (ProcessorType)
    {
    case PROC_68060:
    case PROC_68040: RestoreMMUState40 ();
                     break;
    case PROC_68030: RestoreMMUState30 ();
                     break;
    case PROC_68851: RestoreMMUState851 ();
                     break;
#ifdef DEBUG
    default:
      PRINT_DEB ("KillMMUTable: Unknown processor type", 0L);
      ColdReboot ();
#endif
    }

  Enable ();
  }
#endif

if (MyRootTable != NULL)
  {
  for (i = 0; i < POINTERS_PER_TABLE; i++)
    {
    if (TABLE_RESIDENT_P (*(MyRootTable + i)))
      {
      address = (ULONG*) ALIGN_DOWN (*(MyRootTable + i), POINTERTABALIGN);
      KillPointerTable (address, FALSE);
      }
    }
  FreeMem (MyRootTable, POINTERS_PER_TABLE * sizeof (ULONG));
  }

if (MemRangeListUsed)
  {
  ULONG *VectorTable;

  while ((range = (struct MemRange*)RemHead (&MemRangeList)) != NULL)
    FreeMem (range, sizeof (struct MemRange));

#if !defined(__AROS__)
  Forbid ();
  PRINT_DEB ("Removing dynamic MMU trap", 0L);
  VectorTable = (ULONG*)ReadVBR ();
  *(VectorTable + 2) = (ULONG) OrigDynMMUTrap;
  CacheClearU ();
  Permit ();
#endif
  }
}

/************************************************************************/

void AllowZorroIICaching (BOOL Allowed)

{
struct MemHeader *mem;
int rc;

if (MemRangeListUsed)
  return;

Forbid();
mem=(struct MemHeader *)SysBase->MemList.lh_Head;
while (mem->mh_Node.ln_Succ)
  {
  if ((IPTR)mem->mh_Lower < 0x01000000 && !(mem->mh_Attributes & MEMF_CHIP))
    {
    if (Allowed)
      {
      PRINT_DEB ("Marking chunk as cacheable", 0L);
      if ((rc = MarkAddress(PAGEADDR ((IPTR)mem->mh_Lower),
                   (IPTR)mem->mh_Upper-PAGEADDR((IPTR)mem->mh_Lower),
                   CACHEABLE | PAGE_RESIDENT, 
                   PAGEADDR((IPTR)mem->mh_Lower), TRUE)) != SUCCESS)
        RunTimeError (rc);
      }
    else
      {
      PRINT_DEB ("Marking chunk as non-cacheable", 0L);
      if ((rc = MarkAddress(PAGEADDR((IPTR)mem->mh_Lower),
                   (IPTR)mem->mh_Upper-PAGEADDR((IPTR)mem->mh_Lower),
                   NONCACHEABLE | PAGE_RESIDENT, 
                   PAGEADDR((IPTR)mem->mh_Lower), TRUE)) != SUCCESS)
        RunTimeError (rc);
      }
    }
  mem=(struct MemHeader *)(mem->mh_Node.ln_Succ);
  }
Permit();

(*PFlushA) ();
}

/************************************************************************/

int AllocAddressRange (void)

{
/* Get NUM_PTR_TABLES * 32 MB of address space starting on a 32 MB boundary.
 * Additionally this checks if some badly designed processor card already
 * used the address range without notifying the autoconfig system about
 * it.
 */
IPTR start_slot;

#if !defined(__AROS__)
if ((start_slot = (IPTR)AllocExpansionMem (NUM_PTR_TABLES * 512L, 0L)) == 0)
  return (ERR_NO_ADDR_SPACE);

VirtAddrStart = (APTR)EC_MEMADDR (start_slot);
VirtAddrEnd   = (APTR)EC_MEMADDR (start_slot) + NUM_PTR_TABLES * 0x02000000;
PRINT_DEB ("AllocAddressRange: VirtAddrStart = %08lx", VirtAddrStart);
#else
#warning TODO: set VirtAddrStart && VirtAddrEnd
#endif

return (SUCCESS);
}

/************************************************************************/

int SwitchFastROM (BOOL On)

{
/* Turning FastROM on:
 * Copies the ROM memory to FAST memory and adjusts the memory maps
 * such that the FAST image is used instead. The $f80000 range as well
 * as the mapped image is FAST mem is set to write-protected.
 * It is safe to call this if it wsa already turned on.
 * Returns TRUE if it was possible to set up everything as needed.
 * Turning FastROM off:
 * If it was turned on previously, it is turned off. Otherwise
 * nothing will happen.
 */

if (MemRangeListUsed)
  return (SUCCESS);

if (On)
  {
  if (PAGEADDR ((*GenDescr) (ROMSTART)) != ROMSTART)
    {
    PRINT_DEB ("SwitchFastROM (On): FastROM was already on", 0L);
    return (TRUE);         /* Already mapped */
    }

  /* The alignment to ROMSIZE is necessary because of problems with early-termination
   * descriptor on the 68030. If the alignment is not kept, the page tables will
   * be trashed.
   */
  if ((MappedROM = AllocAligned (ROMSIZE, MEMF_PUBLIC | MEMF_FAST, ROMSIZE, 0)) == NULL)
    {
    PRINT_DEB ("SwitchFastROM: No Mem for Fast ROM", 0L);
    return (ERR_NOT_ENOUGH_MEM);
    }

  CopyMemQuick ((APTR)ROMSTART, MappedROM, ROMSIZE);
  CacheClearU ();

  Forbid ();
  /* Mark copied are under original address as write-protected */
  if (MarkAddress ((IPTR)MappedROM, ROMSIZE, PAGE_RESIDENT | CACHEABLE | WRITEPROTECT,
                   (IPTR)MappedROM, TRUE) != SUCCESS)
    {
    Permit ();
    return (ERR_INTERNAL);
    }

  if (MarkAddress (ROMSTART, ROMSIZE, PAGE_RESIDENT | CACHEABLE | WRITEPROTECT, 
                   (IPTR)MappedROM, TRUE) != SUCCESS)
    {
    FreeMem (MappedROM, ROMSIZE);
    MappedROM = NULL;
    Permit ();
    return (ERR_NOT_ENOUGH_MEM);
    }

  Permit ();
  PRINT_DEB ("Successfully turned on FastROM", 0L);
  }
else
  {
  if (MappedROM == NULL)
    {
    PRINT_DEB ("SwitchFastROM (Off): FastROM was already off", 0L);
    return (SUCCESS);
    }

  Forbid ();
  if (MarkAddress (ROMSTART, ROMSIZE, PAGE_RESIDENT | CACHEABLE | WRITEPROTECT,
                   ROMSTART, TRUE) != SUCCESS)
    {
    Permit ();
    return (ERR_INTERNAL);
    }

  if (MarkAddress ((IPTR)MappedROM, ROMSIZE, PAGE_RESIDENT | CACHEABLE, 
                   (IPTR)MappedROM, TRUE) != SUCCESS)
    {
    return (ERR_INTERNAL);
    }

  Permit ();
  FreeMem (MappedROM, ROMSIZE);
  MappedROM = NULL;
  PRINT_DEB ("Successfully turned off FastROM", 0L);
  }

(*PFlushA) ();
return (SUCCESS);
}

/************************************************************************/

void InstallMapping (ULONG Address)

/* This function gets called from a special traphandler whenever an address
 * is used for which no valid mapping exists, but which is neither in pageable
 * memory. 'InstallMapping' installs a mapping for the pagetable in which this
 * address lies. It should not be called if no MMU config file exists.
 */

{
    int rc;

    PRINT_DEB ("InstallMapping with address %lx called", Address);

    Address = ALIGN_DOWN (Address, PAGES_PER_TABLE * PAGESIZE);
    rc = MarkAddress (Address, PAGES_PER_TABLE * PAGESIZE, 
                      NONCACHEABLE | PAGE_RESIDENT, Address, TRUE);
    CacheClearU ();
    (*PFlushA) ();
    if (rc != SUCCESS)
    {
        RunTimeError (ERR_DYN_MAP_FAILED);
        Wait (0L);
    }
    PRINT_DEB ("Successfully installed mapping for address range starting at %lx", Address);
}

/************************************************************************/

void MarkPage (IPTR addr, ULONG CacheMode)

/* Marks a page used for holding page or pointer tables as noncacheable
 * or cacheable.
 * It searches the current MMU table for the right entry and changes
 * the cache mode settings.
 */

{
    ULONG root_index,
        table_index,
        page_index;
    ULONG *PointerTable,
        *PageTable;
    ULONG entry;

    PRINT_DEB ("MarkPage (%lx) called", addr);

    if (CacheMode == NONCACHEABLE)
        (*CPushP) (addr);

#ifdef DEBUG
    if (ProcessorType < PROC_68040)
    PRINT_DEB ("MarkPage: Called for 68030 or worse", 0L);
#endif

    root_index = ROOTINDEX (addr);
#ifdef DEBUG
    if (TABLE_INVALID_P (*(RootTable + root_index)))
    {
        PRINT_DEB ("MarkPage: Invalid root entry = %08lx", *(RootTable + root_index));
    }
#endif

    PointerTable = (ULONG*)ALIGN_DOWN(*(RootTable + root_index),
                                    POINTERTABALIGN);

    table_index = POINTERINDEX (addr);
#ifdef DEBUG
    if (TABLE_INVALID_P (*(PointerTable + table_index)))
    PRINT_DEB ("MarkPage: Invalid pointer entry", 0L);
#endif

    PageTable = (ULONG*) ALIGN_DOWN(*(PointerTable + table_index),
                                  PAGETABALIGN);

    page_index = PAGEINDEX (addr);
#ifdef DEBUG
    if (PAGE_INVALID_P (*(PageTable + page_index)))
        PRINT_DEB ("MarkPage: Invalid page entry", 0L);
#endif

    entry = *(PageTable + page_index) & ~CM_MASK;          /* remove old caching bits */
    PRINT_DEB ("Setting page table entry to %08lx", entry | CacheMode);
    *(PageTable + page_index) = entry | CacheMode;
    (*CPushL) ((ULONG)(PageTable + page_index));
    (*PFlushP) (addr);
}
