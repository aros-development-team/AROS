/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Code to dynamically load ELF executables and relocate it.
          Take from dos/internalloadseg_elf.c
    Lang: english

    1997/12/13: Changed filename to internalloadseg_elf.c
                Original file was created by digulla.
*/
#include <exec/memory.h>
#include <proto/exec.h>
#include <dos/dosasl.h>
#include <proto/dos.h>
#include <proto/arossupport.h>
#include <aros/asmcall.h>
#include <aros/debug.h>
#include <string.h>

extern struct DosLibrary * DOSBase;


/* Debugging */
#define PRINT_SECTION_NAMES     1
#define PRINT_STRINGTAB         1
#define PRINT_SYMBOLTABLE       1
#define PRINT_SYMBOLS           1
#define LOAD_DEBUG_HUNKS        0
#define PRINT_SECTIONS          1
#define PRINT_HUNKS             1
#define DEBUG_HUNKS             0

#define SHT_PROGBITS    1
#define SHT_SYMTAB      2
#define SHT_STRTAB      3
#define SHT_RELA        4
#define SHT_NOBITS      8
#define SHT_REL         9

#define ET_REL          1

#define EM_68K          4

#define R_68K_32        1
#define R_68K_PC32      4
#define R_68K_PC16      5

#define STT_OBJECT      1
#define STT_FUNC        2

#define SHN_ABS         0xfff1
#define SHN_COMMON      0xfff2
#define SHN_UNDEF       0

#define ELF32_ST_TYPE(i)    ((i) & 0x0F)

struct elfheader
{
    UBYTE ident[16];
    UWORD type;
    UWORD machine;
    ULONG version;
    APTR  entry;
    ULONG phoff;
    ULONG shoff;
    ULONG flags;
    UWORD ehsize;
    UWORD phentsize;
    UWORD phnum;
    UWORD shentsize;
    UWORD shnum;
    UWORD shstrndx;
};

struct sheader
{
    ULONG name;
    ULONG type;
    ULONG flags;
    APTR  addr;
    ULONG offset;
    ULONG size;
    ULONG link;
    ULONG info;
    ULONG addralign;
    ULONG entsize;
};

struct symbol
{
    ULONG name;     /* Offset of the name string in the string table */
    ULONG value;    /* Varies; eg. the offset of the symbol in its hunk */
    ULONG size;     /* How much memory does the symbol occupy */
    UBYTE info;     /* What kind of symbol is this ? (global, variable, etc) */
    UBYTE other;    /* undefined */
    WORD  shindex;  /* In which section is the symbol defined ? */
};

struct relo
{
    ULONG addr;     /* Address of the relocation (relative to the last loaded hunk) */
    ULONG info;     /* Type of the relocation */
    LONG  addend;   /* Constant addend used to compute value */
};

struct hunk
{
    ULONG   size;        /* Size of the hunk */
    UBYTE * memory;      /* First actual byte in the memory of this machine */
    ULONG   destination; /* where it will be located on the destination device */
};


int read_block (BPTR file, ULONG offset, APTR buffer, ULONG size)
{
  LONG    subsize;
  UBYTE * buf     = (UBYTE *)buffer;


  if (Seek (file, offset, OFFSET_BEGINNING) < 0) {
    return 1;
  }

  while (size)
  {
    subsize = Read(file,buf,size);

    if (subsize == 0)
    {
      ((struct Process *)FindTask (NULL))->pr_Result2 = ERROR_BAD_HUNK;
      return 1;
    }

    if (subsize < 0)
      return 1;

    buf  += subsize;
    size -= subsize;
  }

  return 0;
} /* read_block */

BPTR MyInternalLoadSeg_ELF (BPTR file,
                            BPTR table,
                            LONG * stack,
                            struct IOExtSer * IORequest,
                            ULONG * startaddr)

{
/* Currently the only parameter passed to this function that is
   actually used is file. The rest is there for completeness.
   Functionarray will *soon* be used!   */

  struct elfheader eh;
  UBYTE          * shtab    = NULL;
  UBYTE          * shstrtab = NULL;
  UBYTE          * strtab   = NULL;
  struct symbol  * symtab   = NULL;
  struct hunk    * hunks    = NULL;
  struct relo    * reltab   = NULL;
  struct symbol  * symbol;
  struct sheader * sh       = NULL;  /* satisfy GCC */

  ULONG   numsym,
          numrel,
          i;
  WORD    t,
          mint   = 0,
          maxt   = 0;
  UBYTE * loaded, * destin;
  BPTR    last   = 0;
  LONG  * error  = &((struct Process *)FindTask (NULL))->pr_Result2;

#define ERROR(a)    { *error = a; goto end; }

  *error = 0;

  /* Load the header */
  if (read_block (file, 0, &eh, sizeof (eh)))
    goto end;

  /* Check the header of the file */
  if (     eh.ident[0] != 0x7f
        || eh.ident[1] != 'E'
        || eh.ident[2] != 'L'
        || eh.ident[3] != 'F'
     )
    ERROR (ERROR_NOT_EXECUTABLE);

  /* Check file type and the CPU the file is for */
  if (AROS_BE2WORD(eh.type) != ET_REL || AROS_BE2WORD(eh.machine) != EM_68K) {
    ERROR (ERROR_OBJECT_WRONG_TYPE);
  }

  /* Get memory for section headers */
  shtab = AllocVec (AROS_BE2WORD(eh.shentsize) * AROS_BE2WORD(eh.shnum), MEMF_ANY);

  if (shtab == NULL)
    ERROR (ERROR_NO_FREE_STORE);

  /* Read section table */
  if (read_block(file, AROS_BE2LONG(eh.shoff), shtab, AROS_BE2WORD(eh.shentsize) * AROS_BE2WORD(eh.shnum)))
    goto end;

  /* Look up the symbol table */
  for (t=1; t<AROS_BE2WORD(eh.shnum); t++)
  {
    sh = (struct sheader *)(shtab + t*AROS_BE2WORD(eh.shentsize));

#if PRINT_SECTIONS
    kprintf ("sh[%d] = { name=%d type=%d flags=%x addr=%p offset=%d "
             "size=%d link=%d info=%d addralign=%d entsize=%d\n",
    t,
    AROS_BE2LONG(sh->name),
    AROS_BE2LONG(sh->type),
    AROS_BE2LONG(sh->flags),
    AROS_BE2LONG(sh->addr),
    AROS_BE2LONG(sh->offset),
    AROS_BE2LONG(sh->size),
    AROS_BE2LONG(sh->link),
    AROS_BE2LONG(sh->info),
    AROS_BE2LONG(sh->addralign),
    AROS_BE2LONG(sh->entsize));
#endif

    if (AROS_BE2LONG(sh->type) == SHT_SYMTAB)
      break;
  }

  if (t == AROS_BE2WORD(eh.shnum))
    ERROR (ERROR_OBJECT_WRONG_TYPE);

  /* Allocate memory for the symbol table */
  symtab = AllocVec (AROS_BE2LONG(sh->size), MEMF_ANY);

  if (symtab == NULL)
    ERROR (ERROR_NO_FREE_STORE);

  /* Read the symbol table */
  if (read_block (file, AROS_BE2LONG(sh->offset), symtab, AROS_BE2LONG(sh->size)))
    goto end;

  numsym = AROS_BE2LONG(sh->size) / sizeof (struct symbol);

  mint = maxt = AROS_BE2WORD(symtab[1].shindex);

/*  kprintf ("Symbol %d: index=%d\n", 0, symtab[1].shindex); */

  /* Find the minimal number of hunks which are neccessary to satisfy
     all symbol references (ie. all hunks in which a symbol resides) */
  for (i=2; i<numsym; i++)
  {
#if PRINT_SYMBOLTABLE
    kprintf ("Symbol %d: name=%d value=%d size=%d info=%d other=%d shindex=%d\n",
    i,
    AROS_BE2LONG(symtab[i].name),
    AROS_BE2LONG(symtab[i].value),
    AROS_BE2LONG(symtab[i].size),
    symtab[i].info,
    symtab[i].other,
    AROS_BE2WORD(symtab[i].shindex));
#endif

    if ((WORD)AROS_BE2WORD(symtab[i].shindex) < mint) {
      mint = AROS_BE2WORD(symtab[i].shindex);
    }
    if ((WORD)AROS_BE2WORD(symtab[i].shindex) > maxt) {
      maxt = AROS_BE2WORD(symtab[i].shindex);
    }
  }

  /* Allocate memory for information about every hunk */
  hunks = AllocVec (sizeof (struct hunk) * (maxt - mint + 1), MEMF_CLEAR);

  if (hunks == NULL)
    ERROR (ERROR_NO_FREE_STORE);

  /* Offset the base. Now I can simply access the first hunk as
     hunks[t] instead of hunks[t-mint] */
  hunks -= mint;

  /* Load names of sections */
  if (AROS_BE2WORD(eh.shstrndx))
  {
    sh = (struct sheader *)(shtab + AROS_BE2WORD(eh.shstrndx)*AROS_BE2WORD(eh.shentsize));

    shstrtab = AllocVec (AROS_BE2LONG(sh->size), MEMF_ANY);

    if (shstrtab == NULL)
      ERROR (ERROR_NO_FREE_STORE);

    if (read_block (file, AROS_BE2LONG(sh->offset), shstrtab, AROS_BE2LONG(sh->size)))
      goto end;

#if PRINT_SECTION_NAMES
    {
      int n, t;

       for (n=t=0; t<AROS_BE2LONG(sh->size); n++)
       {
         kprintf ("String %d@%d: \"%s\"\n", n, t, &shstrtab[t]);
         t += strlen (&shstrtab[t]) + 1;
       }
    }
#endif
  }

  /* Find the basic size for each hunk */
  for (t=1; t<AROS_BE2WORD(eh.shnum); t++)
  {
    sh = (struct sheader *)(shtab + t*AROS_BE2WORD(eh.shentsize));


//    printf("This is %s\n",&shstrtab[AROS_BE2LONG(sh->name)]);
 
    if (AROS_BE2LONG(sh->type) == SHT_PROGBITS || AROS_BE2LONG(sh->type) == SHT_NOBITS)
    {
#if !LOAD_DEBUG_HUNKS
      /* Don't load debug hunks */
      if ( !strncmp (&shstrtab[AROS_BE2LONG(sh->name)], ".stab", 5)
         || !strcmp (&shstrtab[AROS_BE2LONG(sh->name)], ".comment")
         )
        sh->size = 0;
#endif

      hunks[t].size = AROS_BE2LONG(sh->size);
    }
#if !LOAD_DEBUG_HUNKS
    else if (AROS_BE2LONG(sh->type) == SHT_REL
        && !strcmp (&shstrtab[AROS_BE2LONG(sh->name)], ".rel.stab")
        ) {
        sh->size = 0;
        hunks[t].size = 0;
    }
    else if (AROS_BE2LONG(sh->type) == SHT_RELA
        && !strcmp (&shstrtab[AROS_BE2LONG(sh->name)], ".rela.stab")
        ) {
        sh->size = 0;
        hunks[t].size = 0;
    } else {
      printf("\tIgnoring %s\n",&shstrtab[AROS_BE2LONG(sh->name)]);
    }
#endif
  }

  /* Look for names of symbols */
  for (t=AROS_BE2WORD(eh.shnum); t>0; t--)
  {
    sh = (struct sheader *)(shtab + t*AROS_BE2WORD(eh.shentsize));

    if (AROS_BE2LONG(sh->type) == SHT_STRTAB)
      break;
  }

  /* Found the section with the names ? Load the symbols' names */
  if (t)
  {
    strtab = AllocVec (AROS_BE2LONG(sh->size), MEMF_ANY);

    if (strtab == NULL)
      ERROR (ERROR_NO_FREE_STORE);

    /* kprintf ("Reading StrTab at %d (offset=%ld, size=%ld)\n", eh.shstrndx, sh->offset, sh->size); */

    if (read_block (file, AROS_BE2LONG(sh->offset), strtab, AROS_BE2LONG(sh->size)))
      goto end;

#if PRINT_STRINGTAB
    {
      int n, t;

      for (n=t=0; t<AROS_BE2LONG(sh->size); n++)
      {
        kprintf ("String %d@%d: \"%s\"\n", n, t, &strtab[t]);
        t += strlen (&strtab[t]) + 1;
      }
    }
#endif
  }

  kprintf ("    File has %d sections.\n", AROS_BE2WORD(eh.shnum));

  /* Reserve memory for each global symbol in its hunk */
  for (i=1; i<numsym; i++)
  {
    if ((WORD)AROS_BE2WORD(symtab[i].shindex) < 0)
    {
      symtab[i].value = AROS_BE2LONG(hunks[AROS_BE2WORD(symtab[i].shindex)].size);
      hunks[AROS_BE2WORD(symtab[i].shindex)].size += AROS_BE2LONG(symtab[i].size);
    }
    else if (AROS_BE2WORD(symtab[i].shindex == SHN_UNDEF))
    {
      kprintf ("Symbol %s is undefined\n",
                &strtab[AROS_BE2LONG(symtab[i].name)]);

      ERROR (ERROR_OBJECT_WRONG_TYPE);
    }
  }

  /* Allocate memory for each segment */
  for (t=mint; t<=maxt; t++)
  {
    /* Don't allocate memory for hunks which don't need any */
    if (hunks[t].size)
    {
      hunks[t].memory = AllocMem(hunks[t].size+sizeof(ULONG)+sizeof(BPTR),MEMF_CLEAR);
      hunks[t].destination = GetPilotMem(hunks[t].size);

      if (hunks[t].memory == NULL)
        ERROR (ERROR_NO_FREE_STORE);
      
      if (hunks[t].destination == NULL) {
        printf("Not enough memory to hold AROS on the Palm (TM) device!\n");
      }

      *((BPTR *)(hunks[t].memory)) = (BPTR)(hunks[t].size + sizeof(ULONG) + sizeof(BPTR));

      hunks[t].memory += sizeof(ULONG) + sizeof(BPTR);

    }
  }

#if PRINT_SYMBOLS
  /* Show the final addresses of global symbols */
  if (strtab)
  {
    for (i=1; i<numsym; i++)
    {
      /* Print the symbol if it has a name and if it's a variable
         or function */
      if (strtab[AROS_BE2LONG(symtab[i].name)]
          && (
               ELF32_ST_TYPE(symtab[i].info) == STT_OBJECT
            || ELF32_ST_TYPE(symtab[i].info) == STT_FUNC
             )
         )
      {
        if (!strcmp ("entry", &strtab[AROS_BE2LONG(symtab[i].name)]))
        {
          kprintf ("    Symbol at 0x%p: %s\n"
                    , hunks[AROS_BE2WORD(symtab[i].shindex)].memory + AROS_BE2LONG(symtab[i].value)
                    , &strtab[AROS_BE2LONG(symtab[i].name)]
                  );

          /* Print only this symbol */
          break;
         }
      }
    }
  }
#endif

  loaded = NULL;

  /* Now load the pieces into memory */
  for (t=1; t<AROS_BE2WORD(eh.shnum); t++)
  {
    sh = (struct sheader *)(shtab + t*AROS_BE2WORD(eh.shentsize));

#if PRINT_HUNKS
    kprintf ("sh=%s hunk[%d] = { size=%d (%d) memory=%p, &hunk[%d]=%p }\n",
              &shstrtab[AROS_BE2LONG(sh->name)],
              t,
              hunks[t].size, AROS_BE2LONG(sh->size),
              hunks[t].memory, t, &hunks[t]);
#endif

    if (!AROS_BE2LONG(sh->size))
      continue;

    switch(AROS_BE2LONG(sh->type))
    {
      case SHT_PROGBITS: /* Code */
printf("hunks[%d] is code.\n",t);
          if (read_block (file, AROS_BE2LONG(sh->offset), hunks[t].memory, AROS_BE2LONG(sh->size)))
            goto end;

          loaded = hunks[t].memory;
          destin = hunks[t].destination;

#if DEBUG_HUNKS
          if (strtab
              /* && (
                   !strcmp (".text", &shstrtab[AROS_BE2LONG(sh->name)])
                || !strcmp (".rodata", &shstrtab[AROS_BE2LONG(sh->name)])
                || !strcmp (".data", &shstrtab[AROS_BE2LONG(sh->name)])
                    ) */
             )
          {
            kprintf ("    Section at 0x%p ... 0x%p: %s\n"
                      , loaded
                      , loaded + AROS_BE2LONG(sh->size) - 1
                      , &shstrtab[AROS_BE2LONG(sh->name)]
                    );
          }
#endif

        break;

      case SHT_RELA:
      case SHT_REL: /* Relocation table */

          if (loaded == NULL)
            ERROR (ERROR_OBJECT_WRONG_TYPE);

          /* Get memory for the relocation table */
          reltab = AllocVec (AROS_BE2LONG(sh->size), MEMF_ANY);

          if (reltab == NULL)
            ERROR (ERROR_NO_FREE_STORE);

          /* Load it */
          if (read_block (file, AROS_BE2LONG(sh->offset), reltab, AROS_BE2LONG(sh->size)))
            goto end;

          numrel = AROS_BE2LONG(sh->size) / sizeof (struct relo);

          /* For each relocation ... */

kprintf("Relocating %d items. (%d/%d)\n",numrel,AROS_BE2LONG(sh->size),sizeof (struct relo));
          for (i=0; i<numrel; i++)
          {
            symbol = &symtab[AROS_BE2LONG(reltab[i].info) >> 8];

kprintf("symbol->shindex: %d,hunks[...].memory=%x\n",
        AROS_BE2WORD(symbol->shindex),
        (ULONG)hunks[AROS_BE2WORD(symbol->shindex)].memory);

            switch (AROS_BE2LONG(reltab[i].info) & 0xFF)
            {
              case R_68K_32:
kprintf("R_68K_32! %x,symbol->value: %x,added: %x\n",AROS_BE2LONG(reltab[i].addr),AROS_BE2LONG(symbol->value),AROS_BE2LONG(reltab[i].addend));

                 *(ULONG *)&loaded[AROS_BE2LONG(reltab[i].addr)] =
                   AROS_BE2LONG(hunks[AROS_BE2WORD(symbol->shindex)].destination +
                                AROS_BE2LONG(symbol->value) +
                                AROS_BE2LONG(reltab[i].addend));
              break;

              case R_68K_PC32:
kprintf("R_68K_PC32! %x,symbol->value: %x\n",AROS_BE2LONG(reltab[i].addr),AROS_BE2LONG(symbol->value),AROS_BE2LONG(reltab[i].addend));

                *(ULONG *)&loaded[AROS_BE2LONG(reltab[i].addr)] = 
                   AROS_BE2LONG(hunks[AROS_BE2WORD(symbol->shindex)].destination +
                                AROS_BE2LONG(symbol->value) +
                                AROS_BE2LONG(reltab[i].addend) -
                                (ULONG)&destin[AROS_BE2LONG(reltab[i].addr)]);

#if 0
printf("reltab[%d].addr=%x\n",i,AROS_BE2LONG(reltab[i].addr));
printf("hunks[%d].destination = %x\n",
       AROS_BE2WORD(symbol->shindex),
       hunks[AROS_BE2WORD(symbol->shindex)].destination);
printf("symbol->value: %x\n",AROS_BE2LONG(symbol->value));
printf("reltab[%d].addend: %x\n",i,AROS_BE2LONG(reltab[i].addend));
printf("&destin[reltab[i].addr]=%x\n",(ULONG)&destin[AROS_BE2LONG(reltab[i].addr)]);
#endif
              break;

              case R_68K_PC16:
kprintf("\t\tR_68K_PC16 not implemented, yet!\n");
kprintf("R_68K_PC16! %x,symbol->value: %x\n",AROS_BE2LONG(reltab[i].addr),AROS_BE2LONG(symbol->value));
/* is this correct? */

                *(UWORD *)&loaded[AROS_BE2LONG(reltab[i].addr)] =
                  AROS_BE2WORD(hunks[AROS_BE2WORD(symbol->shindex)].destination + 
                               AROS_BE2LONG(symbol->value) +
                               AROS_BE2LONG(reltab[i].addend) - 
                               (ULONG)&destin[AROS_BE2LONG(reltab[i].addr)]);
              break;

              default:
kprintf("Unknown relocation type %d!\n",AROS_BE2LONG(reltab[i].info) & 0xFF);
                ERROR (ERROR_BAD_HUNK);
            } /* switch */
          } /* for */

            /* Release memory */
          FreeVec (reltab);
          reltab = NULL;
          loaded = NULL;

          break;
          
       default:
          printf("Untreated type: %d\n",AROS_BE2LONG(sh->type));
    } /* switch */
  }

  /* link hunks */
  for (t=mint; t<0; t++)
  {
    if (hunks[t].size)
    {
	    ((BPTR *)hunks[t].memory)[-1] = last;
	    last = MKBADDR((BPTR *)hunks[t].memory - 1);
            printf("Sending code of size %d(0x%x), hunk %d, destination address 0x%x.\n",hunks[t].size,hunks[t].size,t,hunks[t].destination);
            send_chunk(IORequest, hunks[t].destination, hunks[t].memory,hunks[t].size);
            *startaddr = hunks[t].destination;
    }
  }

  for (t=maxt; t>=0; t--)
  {
    if (hunks[t].size)
    {
	    ((BPTR *)hunks[t].memory)[-1] = last;
	    last = MKBADDR((BPTR *)hunks[t].memory-1);
            printf("Sending code of size %d(0x%x), hunk %d, destination address 0x%x.\n",hunks[t].size,hunks[t].size,t,hunks[t].destination);
            send_chunk(IORequest, hunks[t].destination, hunks[t].memory,hunks[t].size);
            *startaddr = hunks[t].destination;
    }
  }
#warning Must also free all code here!
  /* Free hunk information table */
  FreeVec (hunks+mint);

  hunks = NULL;

end:
  FreeVec (reltab);

  /* Free all hunks, too ? */
  if (hunks != NULL)
  {
    for (t=mint; t<=maxt; t++)
    {
      if (hunks[t].memory != NULL)
      {
        FreeMem(hunks[t].memory-sizeof(BPTR)-sizeof(ULONG),
                hunks[t].size  +sizeof(BPTR)+sizeof(ULONG));

      }
    }

    FreeVec (hunks + mint);

    /* Fail */
    last = NULL;
  }
  if (shstrtab)
    FreeVec (shstrtab);

  if (strtab)
    FreeVec (strtab);

  FreeVec (symtab);
  FreeVec (shtab);

  return last;
} /* LoadSeg_ELF */
