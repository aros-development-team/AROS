/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2010  Free Software Foundation, Inc.
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

#ifdef GRUB_DSDT_TEST
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#define grub_dprintf(cond, args...) printf ( args )
#define grub_printf printf
typedef uint64_t grub_uint64_t;
typedef uint32_t grub_uint32_t;
typedef uint16_t grub_uint16_t;
typedef uint8_t grub_uint8_t;

#endif

#include <grub/acpi.h>
#ifndef GRUB_DSDT_TEST
#include <grub/i18n.h>
#else
#define _(x) x
#define N_(x) x
#endif

#ifndef GRUB_DSDT_TEST
#include <grub/misc.h>
#include <grub/time.h>
#include <grub/cpu/io.h>
#endif

static inline grub_uint32_t
decode_length (const grub_uint8_t *ptr, int *numlen)
{
  int num_bytes, i;
  grub_uint32_t ret;
  if (*ptr < 64)
    {
      if (numlen)
	*numlen = 1;
      return *ptr;
    }
  num_bytes = *ptr >> 6;
  if (numlen)
    *numlen = num_bytes + 1;
  ret = *ptr & 0xf;
  ptr++;
  for (i = 0; i < num_bytes; i++)
    {
      ret |= *ptr << (8 * i + 4);
      ptr++;
    }
  return ret;
}

static inline grub_uint32_t
skip_name_string (const grub_uint8_t *ptr, const grub_uint8_t *end)
{
  const grub_uint8_t *ptr0 = ptr;
  
  while (ptr < end && (*ptr == '^' || *ptr == '\\'))
    ptr++;
  switch (*ptr)
    {
    case '.':
      ptr++;
      ptr += 8;
      break;
    case '/':
      ptr++;
      ptr += 1 + (*ptr) * 4;
      break;
    case 0:
      ptr++;
      break;
    default:
      ptr += 4;
      break;
    }
  return ptr - ptr0;
}

static inline grub_uint32_t
skip_data_ref_object (const grub_uint8_t *ptr, const grub_uint8_t *end)
{
  grub_dprintf ("acpi", "data type = 0x%x\n", *ptr);
  switch (*ptr)
    {
    case GRUB_ACPI_OPCODE_PACKAGE:
    case GRUB_ACPI_OPCODE_BUFFER:
      return 1 + decode_length (ptr + 1, 0);
    case GRUB_ACPI_OPCODE_ZERO:
    case GRUB_ACPI_OPCODE_ONES:
    case GRUB_ACPI_OPCODE_ONE:
      return 1;
    case GRUB_ACPI_OPCODE_BYTE_CONST:
      return 2;
    case GRUB_ACPI_OPCODE_WORD_CONST:
      return 3;
    case GRUB_ACPI_OPCODE_DWORD_CONST:
      return 5;
    case GRUB_ACPI_OPCODE_STRING_CONST:
      {
	const grub_uint8_t *ptr0 = ptr;
	for (ptr++; ptr < end && *ptr; ptr++);
	if (ptr == end)
	  return 0;
	return ptr - ptr0 + 1;
      }
    default:
      if (*ptr == '^' || *ptr == '\\' || *ptr == '_'
	  || (*ptr >= 'A' && *ptr <= 'Z'))
	return skip_name_string (ptr, end);
      grub_printf ("Unknown opcode 0x%x\n", *ptr);
      return 0;
    }
}

static inline grub_uint32_t
skip_ext_op (const grub_uint8_t *ptr, const grub_uint8_t *end)
{
  const grub_uint8_t *ptr0 = ptr;
  int add;
  grub_dprintf ("acpi", "Extended opcode: 0x%x\n", *ptr);
  switch (*ptr)
    {
    case GRUB_ACPI_EXTOPCODE_MUTEX:
      ptr++;
      ptr += skip_name_string (ptr, end);
      ptr++;
      break;
    case GRUB_ACPI_EXTOPCODE_OPERATION_REGION:
      ptr++;
      ptr += skip_name_string (ptr, end);
      ptr++;
      ptr += add = skip_data_ref_object (ptr, end);
      if (!add)
	return 0;
      ptr += add = skip_data_ref_object (ptr, end);
      if (!add)
	return 0;
      break;
    case GRUB_ACPI_EXTOPCODE_FIELD_OP:
    case GRUB_ACPI_EXTOPCODE_INDEX_FIELD_OP:
      ptr++;
      ptr += decode_length (ptr, 0);
      break;
    default:
      grub_printf ("Unexpected extended opcode: 0x%x\n", *ptr);
      return 0;
    }
  return ptr - ptr0;
}

static int
get_sleep_type (grub_uint8_t *table, grub_uint8_t *end)
{
  grub_uint8_t *ptr, *prev = table;
  int sleep_type = -1;
  
  ptr = table + sizeof (struct grub_acpi_table_header);
  while (ptr < end && prev < ptr)
    {
      int add;
      prev = ptr;
      grub_dprintf ("acpi", "Opcode 0x%x\n", *ptr);
      grub_dprintf ("acpi", "Tell %x\n", (unsigned) (ptr - table));
      switch (*ptr)
	{
	case GRUB_ACPI_OPCODE_EXTOP:
	  ptr++;
	  ptr += add = skip_ext_op (ptr, end);
	  if (!add)
	    return -1;
	  break;
	case GRUB_ACPI_OPCODE_CREATE_WORD_FIELD:
	case GRUB_ACPI_OPCODE_CREATE_BYTE_FIELD:
	  {
	    ptr += 5;
	    ptr += add = skip_data_ref_object (ptr, end);
	    if (!add)
	      return -1;
	    ptr += 4;
	    break;
	  }
	case GRUB_ACPI_OPCODE_NAME:
	  ptr++;
	  if (memcmp (ptr, "_S5_", 4) == 0 || memcmp (ptr, "\\_S5_", 4) == 0)
	    {
	      int ll;
	      grub_uint8_t *ptr2 = ptr;
	      grub_dprintf ("acpi", "S5 found\n");
	      ptr2 += skip_name_string (ptr, end);
	      if (*ptr2 != 0x12)
		{
		  grub_printf ("Unknown opcode in _S5: 0x%x\n", *ptr2);
		  return -1;
		}
	      ptr2++;
	      decode_length (ptr2, &ll);
	      ptr2 += ll;
	      ptr2++;
	      switch (*ptr2)
		{
		case GRUB_ACPI_OPCODE_ZERO:
		  sleep_type = 0;
		  break;
		case GRUB_ACPI_OPCODE_ONE:
		  sleep_type = 1;
		  break;
		case GRUB_ACPI_OPCODE_BYTE_CONST:
		  sleep_type = ptr2[1];
		  break;
		default:
		  grub_printf ("Unknown data type in _S5: 0x%x\n", *ptr2);
		  return -1;
		}
	    }
	  ptr += add = skip_name_string (ptr, end);
	  if (!add)
	    return -1;
	  ptr += add = skip_data_ref_object (ptr, end);
	  if (!add)
	    return -1;
	  break;
	case GRUB_ACPI_OPCODE_SCOPE:
	case GRUB_ACPI_OPCODE_IF:
	case GRUB_ACPI_OPCODE_METHOD:
	  {
	    ptr++;
	    ptr += decode_length (ptr, 0);
	    break;
	  }
	default:
	  grub_printf ("Unknown opcode 0x%x\n", *ptr);
	  return -1;	  
	}
    }

  grub_dprintf ("acpi", "TYP = %d\n", sleep_type);
  return sleep_type;
}

#ifdef GRUB_DSDT_TEST
int
main (int argc, char **argv)
{
  FILE *f;
  size_t len;
  unsigned char *buf;
  if (argc < 2)
    printf ("Usage: %s FILE\n", argv[0]);
  f = fopen (argv[1], "rb");
  if (!f)
    {
      printf ("Couldn't open file\n");
      return 1;
    }
  fseek (f, 0, SEEK_END);
  len = ftell (f);
  fseek (f, 0, SEEK_SET);
  buf = malloc (len);
  if (!buf)
    {
      printf ("Couldn't malloc buffer\n");
      fclose (f);
      return 2;
    }
  if (fread (buf, 1, len, f) != len)
    {
      printf ("Read failed\n");
      free (buf);
      fclose (f);
      return 2;
    }

  printf ("Sleep type = %d\n", get_sleep_type (buf, buf + len));
  free (buf);
  fclose (f);
  return 0;
}

#else

void
grub_acpi_halt (void)
{
  struct grub_acpi_rsdp_v20 *rsdp2;
  struct grub_acpi_rsdp_v10 *rsdp1;
      struct grub_acpi_table_header *rsdt;
      grub_uint32_t *entry_ptr;

  rsdp2 = grub_acpi_get_rsdpv2 ();
  if (rsdp2)
    rsdp1 = &(rsdp2->rsdpv1);
  else
    rsdp1 = grub_acpi_get_rsdpv1 ();
  grub_dprintf ("acpi", "rsdp1=%p\n", rsdp1);
  if (!rsdp1)
    return;

  rsdt = (struct grub_acpi_table_header *) (grub_addr_t) rsdp1->rsdt_addr;
  for (entry_ptr = (grub_uint32_t *) (rsdt + 1);
       entry_ptr < (grub_uint32_t *) (((grub_uint8_t *) rsdt)
				      + rsdt->length);
       entry_ptr++)
    {
      if (grub_memcmp ((void *) (grub_addr_t) *entry_ptr, "FACP", 4) == 0)
	{
	  grub_uint32_t port;
	  struct grub_acpi_fadt *fadt
	    = ((struct grub_acpi_fadt *) (grub_addr_t) *entry_ptr);
	  struct grub_acpi_table_header *dsdt
	    = (struct grub_acpi_table_header *) (grub_addr_t) fadt->dsdt_addr;
	  int sleep_type = -1;

	  port = fadt->pm1a;

	  grub_dprintf ("acpi", "PM1a port=%x\n", port);

	  if (grub_memcmp (dsdt->signature, "DSDT",
			   sizeof (dsdt->signature)) != 0)
	    break;

	  sleep_type = get_sleep_type ((grub_uint8_t *) dsdt,
				       (grub_uint8_t *) dsdt + dsdt->length);

	  if (sleep_type < 0 || sleep_type >= 8)
	    break;

	  grub_dprintf ("acpi", "SLP_TYP = %d, port = 0x%x\n",
			sleep_type, port);

	  grub_outw (GRUB_ACPI_SLP_EN
	  	     | (sleep_type << GRUB_ACPI_SLP_TYP_OFFSET), port & 0xffff);
	}
    }

  grub_millisleep (1500);

  /* TRANSLATORS: It's computer shutdown using ACPI, not disabling ACPI.  */
  grub_puts_ (N_("ACPI shutdown failed"));
}
#endif
