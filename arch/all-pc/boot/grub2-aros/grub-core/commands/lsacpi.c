/* acpi.c  - Display acpi tables.  */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2008  Free Software Foundation, Inc.
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
#include <grub/types.h>
#include <grub/mm.h>
#include <grub/misc.h>
#include <grub/normal.h>
#include <grub/acpi.h>
#include <grub/extcmd.h>
#include <grub/i18n.h>
#include <grub/dl.h>

#pragma GCC diagnostic ignored "-Wcast-align"

GRUB_MOD_LICENSE ("GPLv3+");

static void
print_strn (grub_uint8_t *str, grub_size_t len)
{
  for (; *str && len; str++, len--)
    grub_printf ("%c", *str);
  for (len++; len; len--)
    grub_printf (" ");  
}

#define print_field(x) print_strn(x, sizeof (x))

static void
disp_acpi_table (struct grub_acpi_table_header *t)
{
  print_field (t->signature);
  grub_printf ("%4" PRIuGRUB_UINT32_T "B rev=%u chksum=0x%02x (%s) OEM=", t->length, t->revision, t->checksum,
	       grub_byte_checksum (t, t->length) == 0 ? "valid" : "invalid");
  print_field (t->oemid);
  print_field (t->oemtable);
  grub_printf ("OEMrev=%08" PRIxGRUB_UINT32_T " ", t->oemrev);
  print_field (t->creator_id);
  grub_printf (" %08" PRIxGRUB_UINT32_T "\n", t->creator_rev);
}

static void
disp_madt_table (struct grub_acpi_madt *t)
{
  struct grub_acpi_madt_entry_header *d;
  grub_uint32_t len;

  disp_acpi_table (&t->hdr);
  grub_printf ("Local APIC=%08" PRIxGRUB_UINT32_T "  Flags=%08"
	       PRIxGRUB_UINT32_T "\n",
	       t->lapic_addr, t->flags);
  len = t->hdr.length - sizeof (struct grub_acpi_madt);
  d = t->entries;
  for (;len > 0; len -= d->len, d = (void *) ((grub_uint8_t *) d + d->len))
    {
      switch (d->type)
	{
	case GRUB_ACPI_MADT_ENTRY_TYPE_LAPIC:
	  {
	    struct grub_acpi_madt_entry_lapic *dt = (void *) d;
	    grub_printf ("  LAPIC ACPI_ID=%02x APIC_ID=%02x Flags=%08x\n",
			 dt->acpiid, dt->apicid, dt->flags);
	    if (dt->hdr.len != sizeof (*dt))
	      grub_printf ("   table size mismatch %d != %d\n", dt->hdr.len,
			   (int) sizeof (*dt));
	    break;
	  }

	case GRUB_ACPI_MADT_ENTRY_TYPE_IOAPIC:
	  {
	    struct grub_acpi_madt_entry_ioapic *dt = (void *) d;
	    grub_printf ("  IOAPIC ID=%02x address=%08x GSI=%08x\n",
			 dt->id, dt->address, dt->global_sys_interrupt);
	    if (dt->hdr.len != sizeof (*dt))
	      grub_printf ("   table size mismatch %d != %d\n", dt->hdr.len,
			   (int) sizeof (*dt));
	    if (dt->pad)
	      grub_printf ("   non-zero pad: %02x\n", dt->pad);
	    break;
	  }

	case GRUB_ACPI_MADT_ENTRY_TYPE_INTERRUPT_OVERRIDE:
	  {
	    struct grub_acpi_madt_entry_interrupt_override *dt = (void *) d;
	    grub_printf ("  Int Override bus=%x src=%x GSI=%08x Flags=%04x\n",
			 dt->bus, dt->source, dt->global_sys_interrupt,
			 dt->flags);
	    if (dt->hdr.len != sizeof (*dt))
	      grub_printf ("   table size mismatch %d != %d\n", dt->hdr.len,
			   (int) sizeof (*dt));
	  }
	  break;

	case GRUB_ACPI_MADT_ENTRY_TYPE_LAPIC_NMI:
	  {
	    struct grub_acpi_madt_entry_lapic_nmi *dt = (void *) d;
	    grub_printf ("  LAPIC_NMI ACPI_ID=%02x Flags=%04x lint=%02x\n",
			 dt->acpiid, dt->flags, dt->lint);
	    if (dt->hdr.len != sizeof (*dt))
	      grub_printf ("   table size mismatch %d != %d\n", dt->hdr.len,
			   (int) sizeof (*dt));
	    break;
	  }

	case GRUB_ACPI_MADT_ENTRY_TYPE_SAPIC:
	  {
	    struct grub_acpi_madt_entry_sapic *dt = (void *) d;
	    grub_printf ("  IOSAPIC Id=%02x GSI=%08x Addr=%016" PRIxGRUB_UINT64_T
			 "\n",
			 dt->id, dt->global_sys_interrupt_base,
			 dt->addr);
	    if (dt->hdr.len != sizeof (*dt))
	      grub_printf ("   table size mismatch %d != %d\n", dt->hdr.len,
			   (int) sizeof (*dt));
	    if (dt->pad)
	      grub_printf ("   non-zero pad: %02x\n", dt->pad);

	  }
	  break;
	case GRUB_ACPI_MADT_ENTRY_TYPE_LSAPIC:
	  {
	    struct grub_acpi_madt_entry_lsapic *dt = (void *) d;
	    grub_printf ("  LSAPIC ProcId=%02x ID=%02x EID=%02x Flags=%x",
			 dt->cpu_id, dt->id, dt->eid, dt->flags);
	    if (dt->flags & GRUB_ACPI_MADT_ENTRY_SAPIC_FLAGS_ENABLED)
	      grub_printf (" Enabled\n");
	    else
	      grub_printf (" Disabled\n");
	    if (d->len > sizeof (struct grub_acpi_madt_entry_sapic))
	      grub_printf ("  UID val=%08x, Str=%s\n", dt->cpu_uid,
			   dt->cpu_uid_str);
	    if (dt->hdr.len != sizeof (*dt) + grub_strlen ((char *) dt->cpu_uid_str) + 1)
	      grub_printf ("   table size mismatch %d != %d\n", dt->hdr.len,
			   (int) sizeof (*dt));
	    if (dt->pad[0] || dt->pad[1] || dt->pad[2])
	      grub_printf ("   non-zero pad: %02x%02x%02x\n", dt->pad[0], dt->pad[1], dt->pad[2]);
	  }
	  break;
	case GRUB_ACPI_MADT_ENTRY_TYPE_PLATFORM_INT_SOURCE:
	  {
	    struct grub_acpi_madt_entry_platform_int_source *dt = (void *) d;
	    static const char * const platint_type[] =
	      {"Nul", "PMI", "INIT", "CPEI"};

	    grub_printf ("  Platform INT flags=%04x type=%02x (%s)"
			 " ID=%02x EID=%02x\n",
			 dt->flags, dt->inttype,
			 (dt->inttype < ARRAY_SIZE (platint_type))
			 ? platint_type[dt->inttype] : "??", dt->cpu_id,
			 dt->cpu_eid);
	    grub_printf ("  IOSAPIC Vec=%02x GSI=%08x source flags=%08x\n",
			 dt->sapic_vector, dt->global_sys_int, dt->src_flags);
	  }
	  break;
	default:
	  grub_printf ("  type=%x l=%u ", d->type, d->len);
	  grub_printf (" ??\n");
	}
    }
}

static void
disp_acpi_xsdt_table (struct grub_acpi_table_header *t)
{
  grub_uint32_t len;
  grub_uint64_t *desc;

  disp_acpi_table (t);
  len = t->length - sizeof (*t);
  desc = (grub_uint64_t *) (t + 1);
  for (; len >= sizeof (*desc); desc++, len -= sizeof (*desc))
    {
#if GRUB_CPU_SIZEOF_VOID_P == 4
      if (*desc >= (1ULL << 32))
	{
	  grub_printf ("Unreachable table\n");
	  continue;
	}
#endif
      t = (struct grub_acpi_table_header *) (grub_addr_t) *desc;

      if (t == NULL)
	continue;

      if (grub_memcmp (t->signature, GRUB_ACPI_MADT_SIGNATURE,
		       sizeof (t->signature)) == 0)
	disp_madt_table ((struct grub_acpi_madt *) t);
      else
	disp_acpi_table (t);
    }
}

static void
disp_acpi_rsdt_table (struct grub_acpi_table_header *t)
{
  grub_uint32_t len;
  grub_uint32_t *desc;

  disp_acpi_table (t);
  len = t->length - sizeof (*t);
  desc = (grub_uint32_t *) (t + 1);
  for (; len >= sizeof (*desc); desc++, len -= sizeof (*desc))
    {
      t = (struct grub_acpi_table_header *) (grub_addr_t) *desc;

      if (t == NULL)
	continue;

      if (grub_memcmp (t->signature, GRUB_ACPI_MADT_SIGNATURE,
		       sizeof (t->signature)) == 0)
	disp_madt_table ((struct grub_acpi_madt *) t);
      else
	disp_acpi_table (t);
    }
}

static void
disp_acpi_rsdpv1 (struct grub_acpi_rsdp_v10 *rsdp)
{
  print_field (rsdp->signature);
  grub_printf ("chksum:%02x (%s), OEM-ID: ", rsdp->checksum, grub_byte_checksum (rsdp, sizeof (*rsdp)) == 0 ? "valid" : "invalid");
  print_field (rsdp->oemid);
  grub_printf ("rev=%d\n", rsdp->revision);
  grub_printf ("RSDT=%08" PRIxGRUB_UINT32_T "\n", rsdp->rsdt_addr);
}

static void
disp_acpi_rsdpv2 (struct grub_acpi_rsdp_v20 *rsdp)
{
  disp_acpi_rsdpv1 (&rsdp->rsdpv1);
  grub_printf ("len=%d chksum=%02x (%s) XSDT=%016" PRIxGRUB_UINT64_T "\n", rsdp->length, rsdp->checksum, grub_byte_checksum (rsdp, rsdp->length) == 0 ? "valid" : "invalid",
	       rsdp->xsdt_addr);
  if (rsdp->length != sizeof (*rsdp))
    grub_printf (" length mismatch %d != %d\n", rsdp->length,
		 (int) sizeof (*rsdp));
  if (rsdp->reserved[0] || rsdp->reserved[1] || rsdp->reserved[2])
    grub_printf (" non-zero reserved %02x%02x%02x\n", rsdp->reserved[0], rsdp->reserved[1], rsdp->reserved[2]);
}

static const struct grub_arg_option options[] = {
  {"v1", '1', 0, N_("Show version 1 tables only."), 0, ARG_TYPE_NONE},
  {"v2", '2', 0, N_("Show version 2 and version 3 tables only."), 0, ARG_TYPE_NONE},
  {0, 0, 0, 0, 0, 0}
};

static grub_err_t
grub_cmd_lsacpi (struct grub_extcmd_context *ctxt,
		 int argc __attribute__ ((unused)),
		 char **args __attribute__ ((unused)))
{
  if (!ctxt->state[1].set)
    {
      struct grub_acpi_rsdp_v10 *rsdp1 = grub_acpi_get_rsdpv1 ();
      if (!rsdp1)
	grub_printf ("No RSDPv1\n");
      else
	{
	  grub_printf ("RSDPv1 signature:");
	  disp_acpi_rsdpv1 (rsdp1);
	  disp_acpi_rsdt_table ((void *) (grub_addr_t) rsdp1->rsdt_addr);
	}
    }

  if (!ctxt->state[0].set)
    {
      struct grub_acpi_rsdp_v20 *rsdp2 = grub_acpi_get_rsdpv2 ();
      if (!rsdp2)
	grub_printf ("No RSDPv2\n");
      else
	{
#if GRUB_CPU_SIZEOF_VOID_P == 4
	  if (rsdp2->xsdt_addr >= (1ULL << 32))
	    grub_printf ("Unreachable RSDPv2\n");
	  else
#endif
	    {
	      grub_printf ("RSDPv2 signature:");
	      disp_acpi_rsdpv2 (rsdp2);
	      disp_acpi_xsdt_table ((void *) (grub_addr_t) rsdp2->xsdt_addr);
	      grub_printf ("\n");
	    }
	}
    }
  return GRUB_ERR_NONE;
}

static grub_extcmd_t cmd;

GRUB_MOD_INIT(lsapi)
{
  cmd = grub_register_extcmd ("lsacpi", grub_cmd_lsacpi, 0, "[-1|-2]",
			      N_("Show ACPI information."), options);
}

GRUB_MOD_FINI(lsacpi)
{
  grub_unregister_extcmd (cmd);
}


