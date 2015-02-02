/* acpi.c - modify acpi tables. */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2009  Free Software Foundation, Inc.
 *
 *  GRUB is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  GRUB is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with GRUB.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <grub/dl.h>
#include <grub/extcmd.h>
#include <grub/file.h>
#include <grub/disk.h>
#include <grub/term.h>
#include <grub/misc.h>
#include <grub/acpi.h>
#include <grub/mm.h>
#include <grub/memory.h>
#include <grub/i18n.h>

#ifdef GRUB_MACHINE_EFI
#include <grub/efi/efi.h>
#include <grub/efi/api.h>
#endif

#pragma GCC diagnostic ignored "-Wcast-align"

GRUB_MOD_LICENSE ("GPLv3+");

static const struct grub_arg_option options[] = {
  {"exclude", 'x', 0,
   N_("Don't load host tables specified by comma-separated list."),
   0, ARG_TYPE_STRING},
  {"load-only", 'n', 0,
   N_("Load only tables specified by comma-separated list."), 0, ARG_TYPE_STRING},
  {"v1", '1', 0, N_("Export version 1 tables to the OS."), 0, ARG_TYPE_NONE},
  {"v2", '2', 0, N_("Export version 2 and version 3 tables to the OS."), 0, ARG_TYPE_NONE},
  {"oemid", 'o', 0, N_("Set OEMID of RSDP, XSDT and RSDT."), 0, ARG_TYPE_STRING},
  {"oemtable", 't', 0,
   N_("Set OEMTABLE ID of RSDP, XSDT and RSDT."), 0, ARG_TYPE_STRING},
  {"oemtablerev", 'r', 0,
   N_("Set OEMTABLE revision of RSDP, XSDT and RSDT."), 0, ARG_TYPE_INT},
  {"oemtablecreator", 'c', 0,
   N_("Set creator field of RSDP, XSDT and RSDT."), 0, ARG_TYPE_STRING},
  {"oemtablecreatorrev", 'd', 0,
   N_("Set creator revision of RSDP, XSDT and RSDT."), 0, ARG_TYPE_INT},
  /* TRANSLATORS: "hangs" here is a noun, not a verb.  */
  {"no-ebda", 'e', 0, N_("Don't update EBDA. May fix failures or hangs on some "
   "BIOSes but makes it ineffective with OS not receiving RSDP from GRUB."),
   0, ARG_TYPE_NONE},
  {0, 0, 0, 0, 0, 0}
};

/* Simple checksum by summing all bytes. Used by ACPI and SMBIOS. */
grub_uint8_t
grub_byte_checksum (void *base, grub_size_t size)
{
  grub_uint8_t *ptr;
  grub_uint8_t ret = 0;
  for (ptr = (grub_uint8_t *) base; ptr < ((grub_uint8_t *) base) + size;
       ptr++)
    ret += *ptr;
  return ret;
}

/* rev1 is 1 if ACPIv1 is to be generated, 0 otherwise.
   rev2 contains the revision of ACPIv2+ to generate or 0 if none. */
static int rev1, rev2;
/* OEMID of RSDP, RSDT and XSDT. */
static char root_oemid[6];
/* OEMTABLE of the same tables. */
static char root_oemtable[8];
/* OEMREVISION of the same tables. */
static grub_uint32_t root_oemrev;
/* CreatorID of the same tables. */
static char root_creator_id[4];
/* CreatorRevision of the same tables. */
static grub_uint32_t root_creator_rev;
static struct grub_acpi_rsdp_v10 *rsdpv1_new = 0;
static struct grub_acpi_rsdp_v20 *rsdpv2_new = 0;
static char *playground = 0, *playground_ptr = 0;
static int playground_size = 0;

/* Linked list of ACPI tables. */
struct efiemu_acpi_table
{
  void *addr;
  grub_size_t size;
  struct efiemu_acpi_table *next;
};
static struct efiemu_acpi_table *acpi_tables = 0;

/* DSDT isn't in RSDT. So treat it specially. */
static void *table_dsdt = 0;
/* Pointer to recreated RSDT. */
static void *rsdt_addr = 0;

/* Allocation handles for different tables. */
static grub_size_t dsdt_size = 0;

/* Address of original FACS. */
static grub_uint32_t facs_addr = 0;

struct grub_acpi_rsdp_v20 *
grub_acpi_get_rsdpv2 (void)
{
  if (rsdpv2_new)
    return rsdpv2_new;
  if (rsdpv1_new)
    return 0;
  return grub_machine_acpi_get_rsdpv2 ();
}

struct grub_acpi_rsdp_v10 *
grub_acpi_get_rsdpv1 (void)
{
  if (rsdpv1_new)
    return rsdpv1_new;
  if (rsdpv2_new)
    return 0;
  return grub_machine_acpi_get_rsdpv1 ();
}

static inline int
iszero (grub_uint8_t *reg, int size)
{
  int i;
  for (i = 0; i < size; i++)
    if (reg[i])
      return 0;
  return 1;
}

#if defined (__i386__) || defined (__x86_64__)
/* Context for grub_acpi_create_ebda.  */
struct grub_acpi_create_ebda_ctx {
  int ebda_len;
  grub_uint64_t highestlow;
};

/* Helper for grub_acpi_create_ebda.  */
static int
find_hook (grub_uint64_t start, grub_uint64_t size, grub_memory_type_t type,
	   void *data)
{
  struct grub_acpi_create_ebda_ctx *ctx = data;
  grub_uint64_t end = start + size;
  if (type != GRUB_MEMORY_AVAILABLE)
    return 0;
  if (end > 0x100000)
    end = 0x100000;
  if (end > start + ctx->ebda_len
      && ctx->highestlow < ((end - ctx->ebda_len) & (~0xf)) )
    ctx->highestlow = (end - ctx->ebda_len) & (~0xf);
  return 0;
}

grub_err_t
grub_acpi_create_ebda (void)
{
  struct grub_acpi_create_ebda_ctx ctx = {
    .highestlow = 0
  };
  int ebda_kb_len = 0;
  int mmapregion = 0;
  grub_uint8_t *ebda, *v1inebda = 0, *v2inebda = 0;
  grub_uint8_t *targetebda, *target;
  struct grub_acpi_rsdp_v10 *v1;
  struct grub_acpi_rsdp_v20 *v2;

  ebda = (grub_uint8_t *) (grub_addr_t) ((*((grub_uint16_t *)0x40e)) << 4);
  if (ebda)
    ebda_kb_len = *(grub_uint16_t *) ebda;
  if (ebda_kb_len > 16)
    ebda_kb_len = 0;
  ctx.ebda_len = (ebda_kb_len + 1) << 10;

  /* FIXME: use low-memory mm allocation once it's available. */
  grub_mmap_iterate (find_hook, &ctx);
  targetebda = (grub_uint8_t *) (grub_addr_t) ctx.highestlow;
  grub_dprintf ("acpi", "creating ebda @%llx\n",
		(unsigned long long) ctx.highestlow);
  if (! ctx.highestlow)
    return grub_error (GRUB_ERR_OUT_OF_MEMORY,
		       "couldn't find space for the new EBDA");

  mmapregion = grub_mmap_register ((grub_addr_t) targetebda, ctx.ebda_len,
				   GRUB_MEMORY_RESERVED);
  if (! mmapregion)
    return grub_errno;

  /* XXX: EBDA is unstandardized, so this implementation is heuristical. */
  if (ebda_kb_len)
    grub_memcpy (targetebda, ebda, 0x400);
  else
    grub_memset (targetebda, 0, 0x400);
  *((grub_uint16_t *) targetebda) = ebda_kb_len + 1;
  target = targetebda;

  v1 = grub_acpi_get_rsdpv1 ();
  v2 = grub_acpi_get_rsdpv2 ();
  if (v2 && v2->length > 40)
    v2 = 0;

  /* First try to replace already existing rsdp. */
  if (v2)
    {
      grub_dprintf ("acpi", "Scanning EBDA for old rsdpv2\n");
      for (; target < targetebda + 0x400 - v2->length; target += 0x10)
	if (grub_memcmp (target, GRUB_RSDP_SIGNATURE, GRUB_RSDP_SIGNATURE_SIZE) == 0
	    && grub_byte_checksum (target,
				   sizeof (struct grub_acpi_rsdp_v10)) == 0
	    && ((struct grub_acpi_rsdp_v10 *) target)->revision != 0
	    && ((struct grub_acpi_rsdp_v20 *) target)->length <= v2->length)
	  {
	    grub_memcpy (target, v2, v2->length);
	    grub_dprintf ("acpi", "Copying rsdpv2 to %p\n", target);
	    v2inebda = target;
	    target += v2->length;
	    target = (grub_uint8_t *) ALIGN_UP((grub_addr_t) target, 16);
	    v2 = 0;
	    break;
	  }
    }

  if (v1)
    {
      grub_dprintf ("acpi", "Scanning EBDA for old rsdpv1\n");
      for (; target < targetebda + 0x400 - sizeof (struct grub_acpi_rsdp_v10);
	   target += 0x10)
	if (grub_memcmp (target, GRUB_RSDP_SIGNATURE, GRUB_RSDP_SIGNATURE_SIZE) == 0
	    && grub_byte_checksum (target,
				   sizeof (struct grub_acpi_rsdp_v10)) == 0)
	  {
	    grub_memcpy (target, v1, sizeof (struct grub_acpi_rsdp_v10));
	    grub_dprintf ("acpi", "Copying rsdpv1 to %p\n", target);
	    v1inebda = target;
	    target += sizeof (struct grub_acpi_rsdp_v10);
	    target = (grub_uint8_t *) ALIGN_UP((grub_addr_t) target, 16);
	    v1 = 0;
	    break;
	  }
    }

  target = targetebda + 0x100;

  /* Try contiguous zeros. */
  if (v2)
    {
      grub_dprintf ("acpi", "Scanning EBDA for block of zeros\n");
      for (; target < targetebda + 0x400 - v2->length; target += 0x10)
	if (iszero (target, v2->length))
	  {
	    grub_dprintf ("acpi", "Copying rsdpv2 to %p\n", target);
	    grub_memcpy (target, v2, v2->length);
	    v2inebda = target;
	    target += v2->length;
	    target = (grub_uint8_t *) ALIGN_UP((grub_addr_t) target, 16);
	    v2 = 0;
	    break;
	  }
    }

  if (v1)
    {
      grub_dprintf ("acpi", "Scanning EBDA for block of zeros\n");
      for (; target < targetebda + 0x400 - sizeof (struct grub_acpi_rsdp_v10);
	   target += 0x10)
	if (iszero (target, sizeof (struct grub_acpi_rsdp_v10)))
	  {
	    grub_dprintf ("acpi", "Copying rsdpv1 to %p\n", target);
	    grub_memcpy (target, v1, sizeof (struct grub_acpi_rsdp_v10));
	    v1inebda = target;
	    target += sizeof (struct grub_acpi_rsdp_v10);
	    target = (grub_uint8_t *) ALIGN_UP((grub_addr_t) target, 16);
	    v1 = 0;
	    break;
	  }
    }

  if (v1 || v2)
    {
      grub_mmap_unregister (mmapregion);
      return grub_error (GRUB_ERR_OUT_OF_MEMORY,
			 "couldn't find suitable spot in EBDA");
    }

  /* Remove any other RSDT. */
  for (target = targetebda;
       target < targetebda + 0x400 - sizeof (struct grub_acpi_rsdp_v10);
       target += 0x10)
    if (grub_memcmp (target, GRUB_RSDP_SIGNATURE, GRUB_RSDP_SIGNATURE_SIZE) == 0
	&& grub_byte_checksum (target,
			       sizeof (struct grub_acpi_rsdp_v10)) == 0
	&& target != v1inebda && target != v2inebda)
      *target = 0;

  grub_dprintf ("acpi", "Switching EBDA\n");
  (*((grub_uint16_t *) 0x40e)) = ((grub_addr_t) targetebda) >> 4;
  grub_dprintf ("acpi", "EBDA switched\n");

  return GRUB_ERR_NONE;
}
#endif

/* Create tables common to ACPIv1 and ACPIv2+ */
static void
setup_common_tables (void)
{
  struct efiemu_acpi_table *cur;
  struct grub_acpi_table_header *rsdt;
  grub_uint32_t *rsdt_entry;
  int numoftables;

  /* Treat DSDT. */
  grub_memcpy (playground_ptr, table_dsdt, dsdt_size);
  grub_free (table_dsdt);
  table_dsdt = playground_ptr;
  playground_ptr += dsdt_size;

  /* Treat other tables. */
  for (cur = acpi_tables; cur; cur = cur->next)
    {
      struct grub_acpi_fadt *fadt;

      grub_memcpy (playground_ptr, cur->addr, cur->size);
      grub_free (cur->addr);
      cur->addr = playground_ptr;
      playground_ptr += cur->size;

      /* If it's FADT correct DSDT and FACS addresses. */
      fadt = (struct grub_acpi_fadt *) cur->addr;
      if (grub_memcmp (fadt->hdr.signature, GRUB_ACPI_FADT_SIGNATURE,
		       sizeof (fadt->hdr.signature)) == 0)
	{
	  fadt->dsdt_addr = (grub_addr_t) table_dsdt;
	  fadt->facs_addr = facs_addr;

	  /* Does a revision 2 exist at all? */
	  if (fadt->hdr.revision >= 3)
	    {
	      fadt->dsdt_xaddr = (grub_addr_t) table_dsdt;
	      fadt->facs_xaddr = facs_addr;
	    }

	  /* Recompute checksum. */
	  fadt->hdr.checksum = 0;
	  fadt->hdr.checksum = 1 + ~grub_byte_checksum (fadt, fadt->hdr.length);
	}
    }

  /* Fill RSDT entries. */
  numoftables = 0;
  for (cur = acpi_tables; cur; cur = cur->next)
    numoftables++;

  rsdt_addr = rsdt = (struct grub_acpi_table_header *) playground_ptr;
  playground_ptr += sizeof (struct grub_acpi_table_header) + sizeof (grub_uint32_t) * numoftables;

  rsdt_entry = (grub_uint32_t *) (rsdt + 1);

  /* Fill RSDT header. */
  grub_memcpy (&(rsdt->signature), "RSDT", 4);
  rsdt->length = sizeof (struct grub_acpi_table_header) + sizeof (grub_uint32_t) * numoftables;
  rsdt->revision = 1;
  grub_memcpy (&(rsdt->oemid), root_oemid, sizeof (rsdt->oemid));
  grub_memcpy (&(rsdt->oemtable), root_oemtable, sizeof (rsdt->oemtable));
  rsdt->oemrev = root_oemrev;
  grub_memcpy (&(rsdt->creator_id), root_creator_id, sizeof (rsdt->creator_id));
  rsdt->creator_rev = root_creator_rev;

  for (cur = acpi_tables; cur; cur = cur->next)
    *(rsdt_entry++) = (grub_addr_t) cur->addr;

  /* Recompute checksum. */
  rsdt->checksum = 0;
  rsdt->checksum = 1 + ~grub_byte_checksum (rsdt, rsdt->length);
}

/* Regenerate ACPIv1 RSDP */
static void
setv1table (void)
{
  /* Create RSDP. */
  rsdpv1_new = (struct grub_acpi_rsdp_v10 *) playground_ptr;
  playground_ptr += sizeof (struct grub_acpi_rsdp_v10);
  grub_memcpy (&(rsdpv1_new->signature), GRUB_RSDP_SIGNATURE,
	       sizeof (rsdpv1_new->signature));
  grub_memcpy (&(rsdpv1_new->oemid), root_oemid, sizeof  (rsdpv1_new->oemid));
  rsdpv1_new->revision = 0;
  rsdpv1_new->rsdt_addr = (grub_addr_t) rsdt_addr;
  rsdpv1_new->checksum = 0;
  rsdpv1_new->checksum = 1 + ~grub_byte_checksum (rsdpv1_new,
						  sizeof (*rsdpv1_new));
  grub_dprintf ("acpi", "Generated ACPIv1 tables\n");
}

static void
setv2table (void)
{
  struct grub_acpi_table_header *xsdt;
  struct efiemu_acpi_table *cur;
  grub_uint64_t *xsdt_entry;
  int numoftables;

  numoftables = 0;
  for (cur = acpi_tables; cur; cur = cur->next)
    numoftables++;

  /* Create XSDT. */
  xsdt = (struct grub_acpi_table_header *) playground_ptr;
  playground_ptr += sizeof (struct grub_acpi_table_header) + sizeof (grub_uint64_t) * numoftables;

  xsdt_entry = (grub_uint64_t *)(xsdt + 1);
  for (cur = acpi_tables; cur; cur = cur->next)
    *(xsdt_entry++) = (grub_addr_t) cur->addr;
  grub_memcpy (&(xsdt->signature), "XSDT", 4);
  xsdt->length = sizeof (struct grub_acpi_table_header) + sizeof (grub_uint64_t) * numoftables;
  xsdt->revision = 1;
  grub_memcpy (&(xsdt->oemid), root_oemid, sizeof (xsdt->oemid));
  grub_memcpy (&(xsdt->oemtable), root_oemtable, sizeof (xsdt->oemtable));
  xsdt->oemrev = root_oemrev;
  grub_memcpy (&(xsdt->creator_id), root_creator_id, sizeof (xsdt->creator_id));
  xsdt->creator_rev = root_creator_rev;
  xsdt->checksum = 0;
  xsdt->checksum = 1 + ~grub_byte_checksum (xsdt, xsdt->length);

  /* Create RSDPv2. */
  rsdpv2_new = (struct grub_acpi_rsdp_v20 *) playground_ptr;
  playground_ptr += sizeof (struct grub_acpi_rsdp_v20);
  grub_memcpy (&(rsdpv2_new->rsdpv1.signature), GRUB_RSDP_SIGNATURE,
	       sizeof (rsdpv2_new->rsdpv1.signature));
  grub_memcpy (&(rsdpv2_new->rsdpv1.oemid), root_oemid,
	       sizeof (rsdpv2_new->rsdpv1.oemid));
  rsdpv2_new->rsdpv1.revision = rev2;
  rsdpv2_new->rsdpv1.rsdt_addr = (grub_addr_t) rsdt_addr;
  rsdpv2_new->rsdpv1.checksum = 0;
  rsdpv2_new->rsdpv1.checksum = 1 + ~grub_byte_checksum
    (&(rsdpv2_new->rsdpv1), sizeof (rsdpv2_new->rsdpv1));
  rsdpv2_new->length = sizeof (*rsdpv2_new);
  rsdpv2_new->xsdt_addr = (grub_addr_t) xsdt;
  rsdpv2_new->checksum = 0;
  rsdpv2_new->checksum = 1 + ~grub_byte_checksum (rsdpv2_new,
						  rsdpv2_new->length);
  grub_dprintf ("acpi", "Generated ACPIv2 tables\n");
}

static void
free_tables (void)
{
  struct efiemu_acpi_table *cur, *t;
  if (table_dsdt)
    grub_free (table_dsdt);
  for (cur = acpi_tables; cur;)
    {
      t = cur;
      grub_free (cur->addr);
      cur = cur->next;
      grub_free (t);
    }
  acpi_tables = 0;
  table_dsdt = 0;
}

static grub_err_t
grub_cmd_acpi (struct grub_extcmd_context *ctxt, int argc, char **args)
{
  struct grub_arg_list *state = ctxt->state;
  struct grub_acpi_rsdp_v10 *rsdp;
  struct efiemu_acpi_table *cur, *t;
  int i, mmapregion;
  int numoftables;

  /* Default values if no RSDP is found. */
  rev1 = 1;
  rev2 = 3;

  facs_addr = 0;
  playground = playground_ptr = 0;
  playground_size = 0;

  rsdp = (struct grub_acpi_rsdp_v10 *) grub_machine_acpi_get_rsdpv2 ();

  if (! rsdp)
    rsdp = grub_machine_acpi_get_rsdpv1 ();

  if (rsdp)
    {
      grub_uint32_t *entry_ptr;
      char *exclude = 0;
      char *load_only = 0;
      char *ptr;
      /* RSDT consists of header and an array of 32-bit pointers. */
      struct grub_acpi_table_header *rsdt;

      exclude = state[0].set ? grub_strdup (state[0].arg) : 0;
      if (exclude)
	{
	  for (ptr = exclude; *ptr; ptr++)
	    *ptr = grub_tolower (*ptr);
	}

      load_only = state[1].set ? grub_strdup (state[1].arg) : 0;
      if (load_only)
	{
	  for (ptr = load_only; *ptr; ptr++)
	    *ptr = grub_tolower (*ptr);
	}

      /* Set revision variables to replicate the same version as host. */
      rev1 = ! rsdp->revision;
      rev2 = rsdp->revision;
      rsdt = (struct grub_acpi_table_header *) (grub_addr_t) rsdp->rsdt_addr;
      /* Load host tables. */
      for (entry_ptr = (grub_uint32_t *) (rsdt + 1);
	   entry_ptr < (grub_uint32_t *) (((grub_uint8_t *) rsdt)
					  + rsdt->length);
	   entry_ptr++)
	{
	  char signature[5];
	  struct efiemu_acpi_table *table;
	  struct grub_acpi_table_header *curtable
	    = (struct grub_acpi_table_header *) (grub_addr_t) *entry_ptr;
	  signature[4] = 0;
	  for (i = 0; i < 4;i++)
	    signature[i] = grub_tolower (curtable->signature[i]);

	  /* If it's FADT it contains addresses of DSDT and FACS. */
	  if (grub_strcmp (signature, "facp") == 0)
	    {
	      struct grub_acpi_table_header *dsdt;
	      struct grub_acpi_fadt *fadt = (struct grub_acpi_fadt *) curtable;

	      /* Set root header variables to the same values
		 as FADT by default. */
	      grub_memcpy (&root_oemid, &(fadt->hdr.oemid),
			   sizeof (root_oemid));
	      grub_memcpy (&root_oemtable, &(fadt->hdr.oemtable),
			   sizeof (root_oemtable));
	      root_oemrev = fadt->hdr.oemrev;
	      grub_memcpy (&root_creator_id, &(fadt->hdr.creator_id),
			   sizeof (root_creator_id));
	      root_creator_rev = fadt->hdr.creator_rev;

	      /* Load DSDT if not excluded. */
	      dsdt = (struct grub_acpi_table_header *)
		(grub_addr_t) fadt->dsdt_addr;
	      if (dsdt && (! exclude || ! grub_strword (exclude, "dsdt"))
		  && (! load_only || grub_strword (load_only, "dsdt"))
		  && dsdt->length >= sizeof (*dsdt))
		{
		  dsdt_size = dsdt->length;
		  table_dsdt = grub_malloc (dsdt->length);
		  if (! table_dsdt)
		    {
		      free_tables ();
		      grub_free (exclude);
		      grub_free (load_only);
		      return grub_errno;
		    }
		  grub_memcpy (table_dsdt, dsdt, dsdt->length);
		}

	      /* Save FACS address. FACS shouldn't be overridden. */
	      facs_addr = fadt->facs_addr;
	    }

	  /* Skip excluded tables. */
	  if (exclude && grub_strword (exclude, signature))
	    continue;
	  if (load_only && ! grub_strword (load_only, signature))
	    continue;

	  /* Sanity check. */
	  if (curtable->length < sizeof (*curtable))
	    continue;

	  table = (struct efiemu_acpi_table *) grub_malloc
	    (sizeof (struct efiemu_acpi_table));
	  if (! table)
	    {
	      free_tables ();
	      grub_free (exclude);
	      grub_free (load_only);
	      return grub_errno;
	    }
	  table->size = curtable->length;
	  table->addr = grub_malloc (table->size);
	  playground_size += table->size;
	  if (! table->addr)
	    {
	      free_tables ();
	      return grub_errno;
	    }
	  table->next = acpi_tables;
	  acpi_tables = table;
	  grub_memcpy (table->addr, curtable, table->size);
	}
      grub_free (exclude);
      grub_free (load_only);
    }

  /* Does user specify versions to generate? */
  if (state[2].set || state[3].set)
    {
      rev1 = state[2].set;
      if (state[3].set)
	rev2 = rev2 ? : 2;
      else
	rev2 = 0;
    }

  /* Does user override root header information? */
  if (state[4].set)
    grub_strncpy (root_oemid, state[4].arg, sizeof (root_oemid));
  if (state[5].set)
    grub_strncpy (root_oemtable, state[5].arg, sizeof (root_oemtable));
  if (state[6].set)
    root_oemrev = grub_strtoul (state[6].arg, 0, 0);
  if (state[7].set)
    grub_strncpy (root_creator_id, state[7].arg, sizeof (root_creator_id));
  if (state[8].set)
    root_creator_rev = grub_strtoul (state[8].arg, 0, 0);

  /* Load user tables */
  for (i = 0; i < argc; i++)
    {
      grub_file_t file;
      grub_size_t size;
      char *buf;

      file = grub_file_open (args[i]);
      if (! file)
	{
	  free_tables ();
	  return grub_errno;
	}

      size = grub_file_size (file);
      if (size < sizeof (struct grub_acpi_table_header))
	{
	  grub_file_close (file);
	  free_tables ();
	  return grub_error (GRUB_ERR_BAD_OS, N_("premature end of file %s"),
			     args[i]);
	}

      buf = (char *) grub_malloc (size);
      if (! buf)
	{
	  grub_file_close (file);
	  free_tables ();
	  return grub_errno;
	}

      if (grub_file_read (file, buf, size) != (int) size)
	{
	  grub_file_close (file);
	  free_tables ();
	  if (!grub_errno)
	    grub_error (GRUB_ERR_BAD_OS, N_("premature end of file %s"),
			args[i]);
	  return grub_errno;
	}
      grub_file_close (file);

      if (grub_memcmp (((struct grub_acpi_table_header *) buf)->signature,
		       "DSDT", 4) == 0)
	{
	  grub_free (table_dsdt);
	  table_dsdt = buf;
	  dsdt_size = size;
	}
      else
	{
	  struct efiemu_acpi_table *table;
	  table = (struct efiemu_acpi_table *) grub_malloc
	    (sizeof (struct efiemu_acpi_table));
	  if (! table)
	    {
	      free_tables ();
	      return grub_errno;
	    }

	  table->size = size;
	  table->addr = buf;
	  playground_size += table->size;

	  table->next = acpi_tables;
	  acpi_tables = table;
	}
    }

  numoftables = 0;
  for (cur = acpi_tables; cur; cur = cur->next)
    numoftables++;

  /* DSDT. */
  playground_size += dsdt_size;
  /* RSDT. */
  playground_size += sizeof (struct grub_acpi_table_header) + sizeof (grub_uint32_t) * numoftables;
  /* RSDPv1. */
  playground_size += sizeof (struct grub_acpi_rsdp_v10);
  /* XSDT. */
  playground_size += sizeof (struct grub_acpi_table_header) + sizeof (grub_uint64_t) * numoftables;
  /* RSDPv2. */
  playground_size += sizeof (struct grub_acpi_rsdp_v20);

  playground = playground_ptr
    = grub_mmap_malign_and_register (1, playground_size, &mmapregion,
				     GRUB_MEMORY_ACPI, 0);

  if (! playground)
    {
      free_tables ();
      return grub_error (GRUB_ERR_OUT_OF_MEMORY,
			 "couldn't allocate space for ACPI tables");
    }

  setup_common_tables ();

  /* Request space for RSDPv1. */
  if (rev1)
    setv1table ();

  /* Request space for RSDPv2+ and XSDT. */
  if (rev2)
    setv2table ();

  for (cur = acpi_tables; cur;)
    {
      t = cur;
      cur = cur->next;
      grub_free (t);
    }
  acpi_tables = 0;

#if defined (__i386__) || defined (__x86_64__)
  if (! state[9].set)
    {
      grub_err_t err;
      err = grub_acpi_create_ebda ();
      if (err)
	{
	  rsdpv1_new = 0;
	  rsdpv2_new = 0;
	  grub_mmap_free_and_unregister (mmapregion);
	  return err;
	}
    }
#endif

#ifdef GRUB_MACHINE_EFI
  {
    struct grub_efi_guid acpi = GRUB_EFI_ACPI_TABLE_GUID;
    struct grub_efi_guid acpi20 = GRUB_EFI_ACPI_20_TABLE_GUID;

    grub_efi_system_table->boot_services->install_configuration_table
      (&acpi20, grub_acpi_get_rsdpv2 ());
    grub_efi_system_table->boot_services->install_configuration_table
      (&acpi, grub_acpi_get_rsdpv1 ());
  }
#endif

  return GRUB_ERR_NONE;
}

static grub_extcmd_t cmd;

GRUB_MOD_INIT(acpi)
{
  cmd = grub_register_extcmd ("acpi", grub_cmd_acpi, 0,
			      N_("[-1|-2] [--exclude=TABLE1,TABLE2|"
			      "--load-only=TABLE1,TABLE2] FILE1"
			      " [FILE2] [...]"),
			      N_("Load host ACPI tables and tables "
			      "specified by arguments."),
			      options);
}

GRUB_MOD_FINI(acpi)
{
  grub_unregister_extcmd (cmd);
}
