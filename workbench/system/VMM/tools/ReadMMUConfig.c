/* This program generates a listing of the current MMU setup and writes
 * it to the files ENVARC:VMM_MMU.config and ENV:VMM_MMU.config.
 * The file ENV:VMM_MMU.config is read by VMM upon startup to initialize
 * the MMU table the same way it is when running without VMM.
 */

#include <exec/types.h>
#include <exec/execbase.h>
#include <dos/dos.h>
#include <clib/exec_protos.h>
#include <clib/dos_protos.h>
#include <stdio.h>
#ifdef __GNUC__
#include "../protos.h"
#else
#include "/protos.h"
#endif

#define ENV_FILENAME "ENV:VMM_MMU.config"
#define ENVARC_FILENAME "ENVARC:VMM_MMU.config"

struct RegionDescr
  {
  ULONG log_from,
        log_to,
        phys_from,
        phys_to;
  ULONG attr;
  };

#define TYPE_INVALID 0
#define TYPE_RESIDENT 1
#define TYPE_MASK 0x3
#define WP 0x4
#define CM_MASK 0x60
#define TTHIT   0x8           /* Misused USED bit */

ULONG PageSize;

/* Valid attributes differ for different processor types:
 * For the 68851 and 68030 the following bits are valid:
 *   write-protected, page type and cache mode.
 * For the 68040 and 68060 the same bits are valid but the cache mode is
 * represented by two bits.
 */

FILE *EnvFile,
     *EnvArcFile;
BOOL verbose = FALSE;
BOOL debug = FALSE;

ULONG (*GenDescr) (ULONG);

extern struct ExecBase *SysBase;

/************************************************************************/

void WriteRegion (struct RegionDescr *reg)

{
if ((reg->attr & TYPE_MASK) == TYPE_INVALID)
  {
  /* This region is invalid */
  if (debug)
    {
    printf ("Region is marked as invalid\n");
    printf ("Log address start: %08lx\n", reg->log_from);
    printf ("Log address end  : %08lx\n", reg->log_to);
    printf ("Phys address start: %08lx\n", reg->phys_from);
    printf ("Phys address end  : %08lx\n", reg->phys_to);
    printf ("Attributes are %04lx\n", reg->attr);
    }
  return;
  }

if ((reg->phys_from == reg->phys_to) &&
    (reg->log_from  != reg->log_to))
  {
  /* This region is mapped to a single page. Treat it as if it
   * was invalid.
   */
  if (debug)
    {
    printf ("Region maps to a single page\n");
    printf ("Log address start: %08lx\n", reg->log_from);
    printf ("Log address end  : %08lx\n", reg->log_to);
    printf ("Phys address start: %08lx\n", reg->phys_from);
    printf ("Phys address end  : %08lx\n", reg->phys_to);
    printf ("Attributes are %04lx\n", reg->attr);
    }
  return;
  }

if (verbose)
  printf ("Logical address %08lx maps to %08lx, length %08lx, type %04lx\n",
          reg->log_from, reg->phys_from, 
          reg->log_to - reg->log_from + PageSize, reg->attr);

fprintf (EnvFile, "%08lx %08lx %08lx %04lx\n",
         reg->log_from, reg->phys_from,
          reg->log_to - reg->log_from + PageSize, reg->attr);

fprintf (EnvArcFile, "%08lx %08lx %08lx %04lx\n",
         reg->log_from, reg->phys_from,
          reg->log_to - reg->log_from + PageSize, reg->attr);
}

/************************************************************************/

void WriteMMUTable (void)

{
ULONG current_log_addr = 0;
ULONG current_phys_addr;
ULONG current_attr = 0;
struct RegionDescr current_region;

current_region.log_from  = 0;
current_region.log_to    = 0;
current_region.phys_from = 0;
current_region.phys_to   = 0;
current_region.attr      = 0;

do
  {
  current_phys_addr = (*GenDescr) (current_log_addr);
  if (debug)
    printf ("Descriptor returned for address %08lx: %08lx\n", current_log_addr,
            current_phys_addr);

  current_attr = current_phys_addr & (TYPE_MASK | WP | CM_MASK | TTHIT);
  current_phys_addr &= ~(PageSize - 1);

  if (current_region.attr != current_attr || 
      current_phys_addr < current_region.phys_to ||
      current_phys_addr > current_region.phys_to + PageSize)
    {
    WriteRegion (&current_region);
    current_region.log_from  = current_log_addr;
    current_region.log_to    = current_log_addr;
    current_region.phys_from = current_phys_addr;
    current_region.phys_to   = current_phys_addr;
    current_region.attr      = current_attr;
    }
  else
    {
    current_region.log_to  = current_log_addr;
    current_region.phys_to = current_phys_addr;
    }

  current_log_addr += PageSize;
  } while (current_log_addr != 0L);          /* overflowed once */

WriteRegion (&current_region);
}

/************************************************************************/

void main (int argc, char *argv [])

{
int i;

for (i = 1; i < argc; i++)
  {
  if (FindArg ("VERBOSE/S", argv [i]) != -1)
    {
    verbose = TRUE;
    printf ("Verbose mode\n");
    }
  else if (FindArg ("DEBUG/S", argv [i]) != -1)
    {
    debug = TRUE;
    verbose = TRUE;
    printf ("Debug mode enabled\n");
    }
  else
    {
    fprintf (stderr, "Usage: %s VERBOSE/S DEBUG/S\n", argv [0]);
    return;
    }
  }

if ((EnvFile = fopen (ENV_FILENAME, "w")) == NULL)
  {
  fprintf (stderr, "Could not open %s for writing MMU table\n", ENV_FILENAME);
  return;
  }

if ((EnvArcFile = fopen (ENVARC_FILENAME, "w")) == NULL)
  {
  fprintf (stderr, "Could not open %s for writing MMU table\n", ENVARC_FILENAME);
  fclose (EnvFile);
  return;
  }

/* Determine processor type */
if (SysBase->AttnFlags & AFF_68040)
  {
  if (Is68060 ())
    {
    if (verbose)
      printf ("68060 detected\n");
    GenDescr = GenDescr60;
    PageSize = GetPageSize60 ();
    }
  else
    {
    if (verbose)
      printf ("68040 detected\n");
    GenDescr = GenDescr40;
    PageSize = GetPageSize40 ();
    }
  }
else if (SysBase->AttnFlags & AFF_68030)
  {
  if (verbose)
    printf ("68030 detected\n");
  GenDescr = GenDescr30;
  PageSize = GetPageSize30 ();
  }
else if ((SysBase->AttnFlags & AFF_68020) && MMU68851 ())
  {
  if (verbose)
    printf ("68020+68851 detected\n");
  GenDescr = GenDescr851;
  PageSize = GetPageSize30 ();
  }
else
  {
  fprintf (stderr, "You need a MMU to use VMM\n");
  fclose (EnvFile);
  fclose (EnvArcFile);
  return;
  }

if (PageSize < 4096)
  PageSize = 4096;

WriteMMUTable ();

fclose (EnvFile);
fclose (EnvArcFile);

if (verbose)
  printf ("MMU configuration written to " ENV_FILENAME " and " ENVARC_FILENAME "\n");
}

void PrintDebugMsg (void)

{
}

void OrigColdReboot (void)

{
}
