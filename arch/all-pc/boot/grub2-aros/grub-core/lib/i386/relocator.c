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

#include <grub/mm.h>
#include <grub/misc.h>

#include <grub/types.h>
#include <grub/err.h>
#include <grub/term.h>

#include <grub/i386/relocator.h>
#include <grub/relocator_private.h>
#include <grub/i386/relocator_private.h>
#include <grub/i386/pc/int.h>

extern grub_uint8_t grub_relocator_forward_start;
extern grub_uint8_t grub_relocator_forward_end;
extern grub_uint8_t grub_relocator_backward_start;
extern grub_uint8_t grub_relocator_backward_end;

extern void *grub_relocator_backward_dest;
extern void *grub_relocator_backward_src;
extern grub_size_t grub_relocator_backward_chunk_size;

extern void *grub_relocator_forward_dest;
extern void *grub_relocator_forward_src;
extern grub_size_t grub_relocator_forward_chunk_size;

extern grub_uint8_t grub_relocator16_start;
extern grub_uint8_t grub_relocator16_end;
extern grub_uint16_t grub_relocator16_cs;
extern grub_uint16_t grub_relocator16_ip;
extern grub_uint16_t grub_relocator16_ds;
extern grub_uint16_t grub_relocator16_es;
extern grub_uint16_t grub_relocator16_fs;
extern grub_uint16_t grub_relocator16_gs;
extern grub_uint16_t grub_relocator16_ss;
extern grub_uint16_t grub_relocator16_sp;
extern grub_uint32_t grub_relocator16_edx;
extern grub_uint32_t grub_relocator16_ebx;
extern grub_uint32_t grub_relocator16_esi;

extern grub_uint16_t grub_relocator16_keep_a20_enabled;

extern grub_uint8_t grub_relocator32_start;
extern grub_uint8_t grub_relocator32_end;
extern grub_uint32_t grub_relocator32_eax;
extern grub_uint32_t grub_relocator32_ebx;
extern grub_uint32_t grub_relocator32_ecx;
extern grub_uint32_t grub_relocator32_edx;
extern grub_uint32_t grub_relocator32_eip;
extern grub_uint32_t grub_relocator32_esp;
extern grub_uint32_t grub_relocator32_ebp;
extern grub_uint32_t grub_relocator32_esi;
extern grub_uint32_t grub_relocator32_edi;

extern grub_uint8_t grub_relocator64_start;
extern grub_uint8_t grub_relocator64_end;
extern grub_uint64_t grub_relocator64_rax;
extern grub_uint64_t grub_relocator64_rbx;
extern grub_uint64_t grub_relocator64_rcx;
extern grub_uint64_t grub_relocator64_rdx;
extern grub_uint64_t grub_relocator64_rip;
extern grub_uint64_t grub_relocator64_rip_addr;
extern grub_uint64_t grub_relocator64_rsp;
extern grub_uint64_t grub_relocator64_rsi;
extern grub_addr_t grub_relocator64_cr3;
extern struct grub_i386_idt grub_relocator16_idt;

#define RELOCATOR_SIZEOF(x)	(&grub_relocator##x##_end - &grub_relocator##x##_start)

grub_size_t grub_relocator_align = 1;
grub_size_t grub_relocator_forward_size;
grub_size_t grub_relocator_backward_size;
#ifdef __x86_64__
grub_size_t grub_relocator_jumper_size = 12;
#else
grub_size_t grub_relocator_jumper_size = 7;
#endif

void
grub_cpu_relocator_init (void)
{
  grub_relocator_forward_size = RELOCATOR_SIZEOF(_forward);
  grub_relocator_backward_size = RELOCATOR_SIZEOF(_backward);
}

void
grub_cpu_relocator_jumper (void *rels, grub_addr_t addr)
{
  grub_uint8_t *ptr;
  ptr = rels;
#ifdef __x86_64__
  /* movq imm64, %rax (for relocator) */
  *(grub_uint8_t *) ptr = 0x48;
  ptr++;
  *(grub_uint8_t *) ptr = 0xb8;
  ptr++;
  *(grub_uint64_t *) ptr = addr;
  ptr += sizeof (grub_uint64_t);
#else
  /* movl imm32, %eax (for relocator) */
  *(grub_uint8_t *) ptr = 0xb8;
  ptr++;
  *(grub_uint32_t *) ptr = addr;
  ptr += sizeof (grub_uint32_t);
#endif
  /* jmp $eax/$rax */
  *(grub_uint8_t *) ptr = 0xff;
  ptr++;
  *(grub_uint8_t *) ptr = 0xe0;
  ptr++;
}

void
grub_cpu_relocator_backward (void *ptr, void *src, void *dest,
			     grub_size_t size)
{
  grub_relocator_backward_dest = dest;
  grub_relocator_backward_src = src;
  grub_relocator_backward_chunk_size = size;

  grub_memmove (ptr,
		&grub_relocator_backward_start,
		RELOCATOR_SIZEOF (_backward));
}

void
grub_cpu_relocator_forward (void *ptr, void *src, void *dest,
			     grub_size_t size)
{
  grub_relocator_forward_dest = dest;
  grub_relocator_forward_src = src;
  grub_relocator_forward_chunk_size = size;

  grub_memmove (ptr,
		&grub_relocator_forward_start,
		RELOCATOR_SIZEOF (_forward));
}

grub_err_t
grub_relocator32_boot (struct grub_relocator *rel,
		       struct grub_relocator32_state state,
		       int avoid_efi_bootservices)
{
  grub_err_t err;
  void *relst;
  grub_relocator_chunk_t ch;

  err = grub_relocator_alloc_chunk_align (rel, &ch, 0,
					  (0xffffffff - RELOCATOR_SIZEOF (32))
					  + 1, RELOCATOR_SIZEOF (32), 16,
					  GRUB_RELOCATOR_PREFERENCE_NONE,
					  avoid_efi_bootservices);
  if (err)
    return err;

  grub_relocator32_eax = state.eax;
  grub_relocator32_ebx = state.ebx;
  grub_relocator32_ecx = state.ecx;
  grub_relocator32_edx = state.edx;
  grub_relocator32_eip = state.eip;
  grub_relocator32_esp = state.esp;
  grub_relocator32_ebp = state.ebp;
  grub_relocator32_esi = state.esi;
  grub_relocator32_edi = state.edi;

  grub_memmove (get_virtual_current_address (ch), &grub_relocator32_start,
		RELOCATOR_SIZEOF (32));

  err = grub_relocator_prepare_relocs (rel, get_physical_target_address (ch),
				       &relst, NULL);
  if (err)
    return err;

  asm volatile ("cli");
  ((void (*) (void)) relst) ();

  /* Not reached.  */
  return GRUB_ERR_NONE;
}

grub_err_t
grub_relocator16_boot (struct grub_relocator *rel,
		       struct grub_relocator16_state state)
{
  grub_err_t err;
  void *relst;
  grub_relocator_chunk_t ch;

  /* Put it higher than the byte it checks for A20 check.  */
  err = grub_relocator_alloc_chunk_align (rel, &ch, 0x8010,
					  0xa0000 - RELOCATOR_SIZEOF (16)
					  - GRUB_RELOCATOR16_STACK_SIZE,
					  RELOCATOR_SIZEOF (16)
					  + GRUB_RELOCATOR16_STACK_SIZE, 16,
					  GRUB_RELOCATOR_PREFERENCE_NONE,
					  0);
  if (err)
    return err;

  grub_relocator16_cs = state.cs;  
  grub_relocator16_ip = state.ip;

  grub_relocator16_ds = state.ds;
  grub_relocator16_es = state.es;
  grub_relocator16_fs = state.fs;
  grub_relocator16_gs = state.gs;

  grub_relocator16_ss = state.ss;
  grub_relocator16_sp = state.sp;

  grub_relocator16_ebx = state.ebx;
  grub_relocator16_edx = state.edx;
  grub_relocator16_esi = state.esi;
#ifdef GRUB_MACHINE_PCBIOS
  grub_relocator16_idt = *grub_realidt;
#else
  grub_relocator16_idt.base = 0;
  grub_relocator16_idt.limit = 0;
#endif

  grub_relocator16_keep_a20_enabled = state.a20;

  grub_memmove (get_virtual_current_address (ch), &grub_relocator16_start,
		RELOCATOR_SIZEOF (16));

  err = grub_relocator_prepare_relocs (rel, get_physical_target_address (ch),
				       &relst, NULL);
  if (err)
    return err;

  asm volatile ("cli");
  ((void (*) (void)) relst) ();

  /* Not reached.  */
  return GRUB_ERR_NONE;
}

grub_err_t
grub_relocator64_boot (struct grub_relocator *rel,
		       struct grub_relocator64_state state,
		       grub_addr_t min_addr, grub_addr_t max_addr)
{
  grub_err_t err;
  void *relst;
  grub_relocator_chunk_t ch;

  err = grub_relocator_alloc_chunk_align (rel, &ch, min_addr,
					  max_addr - RELOCATOR_SIZEOF (64),
					  RELOCATOR_SIZEOF (64), 16,
					  GRUB_RELOCATOR_PREFERENCE_NONE,
					  0);
  if (err)
    return err;

  grub_relocator64_rax = state.rax;
  grub_relocator64_rbx = state.rbx;
  grub_relocator64_rcx = state.rcx;
  grub_relocator64_rdx = state.rdx;
  grub_relocator64_rip = state.rip;
  grub_relocator64_rsp = state.rsp;
  grub_relocator64_rsi = state.rsi;
  grub_relocator64_cr3 = state.cr3;

  grub_memmove (get_virtual_current_address (ch), &grub_relocator64_start,
		RELOCATOR_SIZEOF (64));

  err = grub_relocator_prepare_relocs (rel, get_physical_target_address (ch),
				       &relst, NULL);
  if (err)
    return err;

  asm volatile ("cli");
  ((void (*) (void)) relst) ();

  /* Not reached.  */
  return GRUB_ERR_NONE;
}
