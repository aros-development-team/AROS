/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Load an a.out format image into memory.
    Lang: English.

    1997/12/13: Changed filename to internalloadseg_aout.c
                Original file was created by digulla (iaint actually).
*/
#include <exec/memory.h>
#include <proto/exec.h>
#include <dos/dosasl.h>
#include <proto/dos.h>
#include <proto/arossupport.h>
#include "dos_intern.h"
#include <aros/debug.h>
#include <aros/asmcall.h>
#include "internalloadseg.h"

extern struct DosLibrary * DOSBase;

/*
    a.out files are much simpler than ELF format files or AmigaOS files.
    This is probably due to the age of the format (AT&T V7 I think).

    We load the text and combined data/bss segments randomly into memory
    in two huge chunks. This is because it would be rather tricky to split
    them up, as they are designed really for virtual memory based-machines
    (so they can be loaded at the same address).

    This has the unfortunate side effect of that for large programs, if
    your memory is quite fragmented, you will not have enough memory to
    load the program into memory.

*/

/* The structure defining the file */
struct aout_hdr
{
    UWORD   a_magic;            /* Magic number                         */
    UWORD   a_mid;              /* Machine ID and flags (flags not used */
    ULONG   a_text;             /* Length of text segment               */
    ULONG   a_data;             /* Length of data segment               */
    ULONG   a_bss;              /* Length of BSS space required         */
    ULONG   a_syms;             /* Symbol table length (bytes)          */
    ULONG   a_entry;            /* Program start point                  */
    ULONG   a_trsize;           /* Size of text relocations (bytes)     */
    ULONG   a_drsize;           /* Size of data relocations (bytes)     */
};

/* A relocation record */
struct reloc
{
    LONG    r_address;          /* Offset in the section to the reloc   */
    ULONG   r_symbolnum : 24,   /* Actually the segment number          */
            r_pcrel : 1,        /* PC relative (do nothing)             */
            r_length : 2,       /* Length of relocation - should be 2   */
            r_extern : 1,       /* External relocation - not supported  */
            r_pad   : 4;
};

#define OMAGIC      0407        /* Not used */
#define NMAGIC      0410        /* The format we use. */
#define ZMAGIC      0413        /* Not used */

#define MID_i386    134         /* i386 binary (386BSD) */

#define N_EXT       0x01        /* External flag - symbol can be accessed
                                   externally */
#define N_ABS       0x02        /* Absolute Symbol - Not used */
#define N_TEXT      0x04        /* Text symbol */
#define N_DATA      0x06        /* Data symbol */
#define N_BSS       0x08        /* BSS symbol */

/*  This is used so that we can jump over any constant stuff at the
    beginning of the text hunk.

    GCC tends to put string constants for a function before the function
    in the text segment, so the very first byte of the text segment is
    not actually code, but a string. This jumps over that.
*/
struct JumpHunk
{
    ULONG   size;
    BPTR    next;
    struct JumpVec vec;
};

/* relocate(exec, reloc, current, refer):
    exec    -   The files exec header.
    reloc   -   The relocation data.
    current -   The base of the hunk we are currently relocating in.
    refer   -   The base of the hunk that the data references.

    The hunk bases point to the "next hunk pointer", so we have to
    add the size of a BPTR to the hunk to get the real address.

    PC relative relocations are do-nothings because, no matter what the
    load address of the code is, the data at that location is still the
    same.
*/

static LONG relocate(   struct aout_hdr *head,
                        struct reloc *reloc,
                        UBYTE  *currentHunk,
                        UBYTE  *referHunk
                    )
{
  /*  I don't test whether the currentHunk is valid, since if it isn't
      we should never have got here.

      It could however be possible to say get a reference into a
      data or bss hunk that doesn't exist.
  */
  if(referHunk == NULL)
  {
    D(bug("LoadSeg_AOUT: Trying to refer to a non-existant hunk.\n"));
    return ERROR_BAD_HUNK;
  }

  /*
     References are relative to the file offset, so if this is a data
     or BSS hunk, then we have to subtract the text segment size from
     the address. It is effectively added back on by the stored value.

     If BSS hunks were not contiguous with the data hunk then we would
     have to do a similar thing there.
  */
  if(reloc->r_symbolnum != N_TEXT)
  {
    /* We should check whether we are doing a non-text PC rel here. */
    referHunk -= head->a_text;
  }

  if(reloc->r_length != 2)
  {
    D(bug("LoadSeg_AOUT: Cannot relocate, bad reloc length at offset %ld\n",
           reloc->r_address));
    return ERROR_BAD_HUNK;
  }

  /* If we try this on a PC relative reloc, nothing will work */
  if(!reloc->r_pcrel)
    *(ULONG *)&currentHunk[reloc->r_address] += (ULONG)referHunk;

  return 0;
}

BPTR InternalLoadSeg_AOUT(BPTR file,
                          BPTR table,
                          LONG * functionarray,
                          LONG * stack,
                          struct DosLibrary * DOSBase)
{
/* Currently the only parameter passed to this function that is
   actually used is file. The rest is there for completeness.
   Functionarray will *soon* be used!   */

  struct reloc     rel;
  UBYTE           *texthunk = NULL;
  UBYTE           *datahunk = NULL;
  struct JumpHunk *jumphunk = NULL;
  struct aout_hdr  header;
  LONG             rel_remain;
  LONG             err;
  LONG            *error = &(((struct Process *)FindTask(NULL))->pr_Result2);

#define ERROR(a)    { *error = a; goto end; }

  /* In case we have already had something attempt to load the file. */
  Seek(file, 0, OFFSET_BEGINNING);

  if ( AROS_CALL3(LONG, functionarray[0] /* Read */,
  	 AROS_LCA(BPTR  , file                                , D1),
  	 AROS_LCA(void *, &header                             , D2),
  	 AROS_LCA(LONG  , sizeof(struct aout_hdr)             , D3),
  	 struct Library *, (struct Library *)DOSBase) != 
       sizeof(struct aout_hdr))
  {
    D(bug("LoadSeg_AOUT: Can't read all of header\n"));
    ERROR(ERROR_FILE_NOT_OBJECT);
  }

  /*
     The format that we use is an NMAGIC format with relocation
     information. The important things about this is that the
     text/data/bss segments are not page aligned (although that
     doesn't really matter at this point in time). And most
     importantly, the file thinks its being loaded at address
     0x00000000 (rather than 0x00001000 for ZMAGIC files).
  */
  if( ((header.a_mid) != MID_i386) || (header.a_magic != NMAGIC))
  {
    D(bug("LoadSeg_AOUT: Bad magic number 0x%4x 0x%4x\n", header.a_magic, header.a_mid));
    ERROR(ERROR_OBJECT_WRONG_TYPE);
  }

  /* It appears that GCC is putting some constant strings in the text
     segment before the entry point. So what we have to do is jump
     over those strings to the actual entry. To do this I will use
     a struct JumpVec (yes the same as in the the library bases).
  */
  jumphunk = AROS_CALL2(void *, functionarray[1] /* AllocMem */,
  		AROS_LCA(ULONG, sizeof(struct JumpHunk)              , D0),
  		AROS_LCA(ULONG, MEMF_CLEAR|MEMF_ANY                  , D1),
  		struct Library *, (struct Library *)SysBase);
  if(jumphunk == NULL)
    ERROR(ERROR_NO_FREE_STORE);

  /* Text segment is required. */
  texthunk = AROS_CALL2(void *, functionarray[1] /* AllocMem */,
  		AROS_LCA(ULONG, header.a_text+sizeof(ULONG)+sizeof(BPTR) , D0),
  		AROS_LCA(ULONG, MEMF_CLEAR|MEMF_ANY                      , D1),
  		struct Library *, (struct Library *)SysBase);

  if(texthunk == NULL)
    ERROR(ERROR_NO_FREE_STORE);

  *((ULONG *)texthunk) = header.a_text + sizeof(ULONG) + sizeof(BPTR);
  /* Link and Bump the text hunk past the next hunk pointer. */
  jumphunk->size = sizeof(struct JumpHunk);
  jumphunk->next = MKBADDR(texthunk + sizeof(ULONG));
  texthunk += sizeof(ULONG) + sizeof(BPTR);

#ifdef __AROS_SET_JMP
  __AROS_SET_JMP(&jumphunk->vec);
#endif
  __AROS_SET_VEC(&jumphunk->vec, texthunk + header.a_entry);

  if ( AROS_CALL3(LONG, functionarray[0] /* Read */,
  	 AROS_LCA(BPTR  , file                                , D1),
  	 AROS_LCA(void *, texthunk                            , D2),
  	 AROS_LCA(LONG  , header.a_text                       , D3),
  	 struct Library *, (struct Library *)DOSBase) != 
       header.a_text)
  {
    D(bug("LoadSeg_AOUT: Can't read all of text segment\n"));
    ERROR(ERROR_BAD_HUNK);
  }

  /*
     Data hunk is not required, but probably exists.
     It doesn't for a number of disk based libs and devs that I looked at
  */
  if(header.a_data)
  {
    /* Include BSS with the data hunk. */
    datahunk = AROS_CALL2(void *, functionarray[1] /* AllocMem */,
		 AROS_LCA(ULONG, header.a_data+header.a_bss+sizeof(ULONG)+sizeof(BPTR) , D0),
  		 AROS_LCA(ULONG, MEMF_CLEAR|MEMF_ANY                                   , D1),
  		 struct Library *, (struct Library *)SysBase);

    if(datahunk == NULL)
      ERROR(ERROR_NO_FREE_STORE);
    /* write the size of allocated memory */
    *((ULONG *)datahunk) = header.a_data + header.a_bss + sizeof(ULONG) + sizeof(BPTR);

    datahunk += sizeof(ULONG) + sizeof(BPTR);

    if ( AROS_CALL3(LONG, functionarray[0] /* Read */,
	   AROS_LCA(BPTR  , file                                , D1),
  	   AROS_LCA(void *, datahunk                            , D2),
  	   AROS_LCA(LONG  , header.a_data                       , D3),
  	   struct Library *, (struct Library *)DOSBase) != 
         header.a_data)
    {
      D(bug("LoadSeg_AOUT: Can't read all of data segment\n"));
      ERROR(ERROR_BAD_HUNK);
    }
  }
  else if(header.a_bss)
  {
    datahunk = AROS_CALL2(void *, functionarray[1] /* AllocMem */,
		 AROS_LCA(ULONG, header.a_bss+sizeof(ULONG)+sizeof(BPTR) , D0),
  		 AROS_LCA(ULONG, MEMF_CLEAR|MEMF_ANY                     , D1),
  		 struct Library *, (struct Library *)SysBase);

    if(datahunk == NULL)
      ERROR(ERROR_NO_FREE_STORE);
    /* write the size of allocated memory */
    *datahunk = header.a_bss + sizeof(ULONG) + sizeof(BPTR);

    datahunk += sizeof(ULONG) + sizeof(BPTR);
  }

  /* Link hunks together. If no data or bss, datahunk == NULL */
  ((BPTR *)texthunk)[-1] = MKBADDR( (BPTR *)datahunk -1);

  if(datahunk)
    ((BPTR *)datahunk)[-1] = (BPTR)NULL;

  /* First of all, text relocations. */
  rel_remain = header.a_trsize / sizeof(struct reloc);
  for(; rel_remain > 0; rel_remain-- )
  {
    if ( AROS_CALL3(LONG, functionarray[0] /* Read */,
	   AROS_LCA(BPTR  , file                                , D1),
  	   AROS_LCA(void *, &rel                                , D2),
  	   AROS_LCA(LONG  , sizeof(struct reloc)                , D3),
  	   struct Library *, (struct Library *)DOSBase) != 
         sizeof(struct reloc))

    {
      D(bug("LoadSeg_AOUT: Can't load a text relocation.\n"));
      ERROR(ERROR_BAD_HUNK);
    }

    if(rel.r_extern)
    {
      D(bug("LoadSeg_AOUT: Can't relocate external symbols.\n"));
      ERROR(ERROR_BAD_HUNK);
    }

    switch(rel.r_symbolnum)
    {
      case    N_TEXT | N_EXT:
      case    N_TEXT:
             err = relocate(&header, &rel, texthunk, texthunk);
      break;

      case    N_DATA | N_EXT:
      case    N_DATA:
      case    N_BSS | N_EXT: /* this is a bit silly */
      case    N_BSS:
             err = relocate(&header, &rel, texthunk, datahunk);
      break;

      default:
        D(bug("LoadSeg_AOUT: Can't relocate! Invalid Text SymNum\n"));
        ERROR(ERROR_FILE_NOT_OBJECT);
    }
    if(err)
    {
      ERROR(err);
    }
  } /* for(relocation entry) */

  /* Next of all, data relocations. */
  rel_remain = header.a_drsize / sizeof(struct reloc);
  for(; rel_remain > 0; rel_remain-- )
  {
    if ( AROS_CALL3(LONG, functionarray[0] /* Read */,
	   AROS_LCA(BPTR  , file                                , D1),
  	   AROS_LCA(void *, &rel                                , D2),
  	   AROS_LCA(LONG  , sizeof(struct reloc)                , D3),
  	   struct Library *, (struct Library *)DOSBase) != 
         sizeof(struct reloc))
    {
      D(bug("LoadSeg_AOUT: Can't load a text relocation.\n"));
      ERROR(ERROR_BAD_HUNK);
    }

    if(rel.r_extern)
    {
      D(bug("LoadSeg_AOUT: Can't relocate external symbols.\n"));
      ERROR(ERROR_FILE_NOT_OBJECT);
    }
    switch(rel.r_symbolnum)
    {
      case    N_TEXT|N_EXT:
      case    N_TEXT:
          err = relocate(&header, &rel, datahunk, texthunk);
      break;

      case    N_DATA|N_EXT:
      case    N_DATA:
      case    N_BSS|N_EXT:
      case    N_BSS:
          err = relocate(&header, &rel, datahunk, datahunk);
      break;

      default:
        D(bug("LoadSeg_AOUT: Can't relocate! Invalid Data SymNum\n"));
        ERROR(ERROR_FILE_NOT_OBJECT);
    }
    if(err)
    {
      ERROR(err);
    }
  }

  /* Flush the caches */
  CacheClearE(texthunk - sizeof(ULONG) - sizeof(BPTR), 
		header.a_text + sizeof(ULONG) + sizeof(BPTR),
		CACRF_ClearI|CACRF_ClearD);

  if(datahunk)
    CacheClearE(datahunk - sizeof(ULONG) - sizeof(BPTR),
		header.a_data + header.a_bss + sizeof(ULONG) + sizeof(BPTR),
		CACRF_ClearI|CACRF_ClearD);

  /* Ok, it is relocated, and ready to run. Remember to subtract
     next hunk pointer from the text hunk.
  */

  D(bug("Text Address = %p\tData Address = %p\n", texthunk, datahunk));

  if(header.a_entry != 0)
  {
    /* jumphunk is the address of the next hunk pointer. */
    return MKBADDR(&jumphunk->next);
  }
  else
  {
    /* We don't need it */
    AROS_CALL2(void *, functionarray[2] /* FreeMem */,
      AROS_LCA(void *, jumphunk                            , A1),
      AROS_LCA(ULONG , sizeof(struct JumpHunk)             , D0),
      struct Library *, (struct Library *)SysBase);

    return MKBADDR((BPTR *)texthunk - 1);
  }

end:
  /* If we allocated a text or data hunk, then we should free them */
  if(datahunk)
    AROS_CALL2(void *, functionarray[2] /* FreeMem */,
      AROS_LCA(void *,                   datahunk - sizeof(BPTR) - sizeof(ULONG)  , A1),
      AROS_LCA(ULONG , *(ULONG *)((ULONG)datahunk - sizeof(BPTR) - sizeof(ULONG)) , D0),
      struct Library *, (struct Library *)SysBase);

  if(texthunk)
    AROS_CALL2(void *, functionarray[2] /* FreeMem */,
      AROS_LCA(void *,                   texthunk - sizeof(BPTR) - sizeof(ULONG)  , A1),
      AROS_LCA(ULONG , *(ULONG *)((ULONG)texthunk - sizeof(BPTR) - sizeof(ULONG)) , D0),
      struct Library *, (struct Library *)SysBase);

  if(jumphunk)
    AROS_CALL2(void *, functionarray[2] /* FreeMem */,
      AROS_LCA(void *, jumphunk                            , A1),
      AROS_LCA(ULONG , sizeof(struct JumpHunk)             , D0),
      struct Library *, (struct Library *)SysBase);

  return (BPTR)NULL;
}
