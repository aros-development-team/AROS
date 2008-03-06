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

#ifndef GRUB_AOUT_HEADER
#define GRUB_AOUT_HEADER 1

#include <grub/types.h>

struct grub_aout32_header
{
  grub_uint32_t a_midmag;	/* htonl(flags<<26 | mid<<16 | magic) */
  grub_uint32_t a_text;		/* text segment size */
  grub_uint32_t a_data;		/* initialized data size */
  grub_uint32_t a_bss;		/* uninitialized data size */
  grub_uint32_t a_syms;		/* symbol table size */
  grub_uint32_t a_entry;	/* entry point */
  grub_uint32_t a_trsize;	/* text relocation size */
  grub_uint32_t a_drsize;	/* data relocation size */
};

struct grub_aout64_header
{
  grub_uint32_t a_midmag;	/* htonl(flags<<26 | mid<<16 | magic) */
  grub_uint64_t a_text;		/* text segment size */
  grub_uint64_t a_data;		/* initialized data size */
  grub_uint64_t a_bss;		/* uninitialized data size */
  grub_uint64_t a_syms;		/* symbol table size */
  grub_uint64_t a_entry;	/* entry point */
  grub_uint64_t a_trsize;	/* text relocation size */
  grub_uint64_t a_drsize;	/* data relocation size */
};

union grub_aout_header
{
  struct grub_aout32_header aout32;
  struct grub_aout64_header aout64;
};

#define AOUT_TYPE_NONE		0
#define AOUT_TYPE_AOUT32	1
#define AOUT_TYPE_AOUT64	6

#define	AOUT32_OMAGIC		0x107	/* 0407 old impure format */
#define	AOUT32_NMAGIC		0x108	/* 0410 read-only text */
#define	AOUT32_ZMAGIC		0x10b	/* 0413 demand load format */
#define AOUT32_QMAGIC		0xcc	/* 0314 "compact" demand load format */

#define AOUT64_OMAGIC		0x1001
#define AOUT64_ZMAGIC		0x1002
#define AOUT64_NMAGIC		0x1003

#define	AOUT_MID_ZERO		0	/* unknown - implementation dependent */
#define	AOUT_MID_SUN010		1	/* sun 68010/68020 binary */
#define	AOUT_MID_SUN020		2	/* sun 68020-only binary */
#define AOUT_MID_I386		134	/* i386 BSD binary */
#define AOUT_MID_SPARC		138	/* sparc */
#define	AOUT_MID_HP200		200	/* hp200 (68010) BSD binary */
#define	AOUT_MID_HP300		300	/* hp300 (68020+68881) BSD binary */
#define	AOUT_MID_HPUX		0x20C	/* hp200/300 HP-UX binary */
#define	AOUT_MID_HPUX800	0x20B	/* hp800 HP-UX binary */

#define AOUT_FLAG_PIC		0x10	/* contains position independant code */
#define AOUT_FLAG_DYNAMIC	0x20	/* contains run-time link-edit info */
#define AOUT_FLAG_DPMASK	0x30	/* mask for the above */

#define AOUT_GETMAGIC(header) ((header).a_midmag & 0xffff)
#define AOUT_GETMID(header) ((header).a_midmag >> 16) & 0x03ff)
#define AOUT_GETFLAG(header) ((header).a_midmag >> 26) & 0x3f)

int EXPORT_FUNC(grub_aout_get_type) (union grub_aout_header *header);

grub_err_t EXPORT_FUNC(grub_aout_load) (grub_file_t file, int offset,
                                        grub_addr_t load_addr, int load_size,
                                        grub_addr_t bss_end_addr);

#endif /* ! GRUB_AOUT_HEADER */
