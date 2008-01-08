/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2000,2001,2002,2003,2004,2005,2007  Free Software Foundation, Inc.
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

#include <grub/machine/vga.h>
#include <grub/machine/console.h>
#include <grub/cpu/io.h>
#include <grub/term.h>
#include <grub/types.h>
#include <grub/dl.h>
#include <grub/misc.h>
#include <grub/normal.h>
#include <grub/font.h>
#include <grub/arg.h>

#define DEBUG_VGA	0

#define VGA_WIDTH	640
#define VGA_HEIGHT	350
#define CHAR_WIDTH	8
#define CHAR_HEIGHT	16
#define TEXT_WIDTH	(VGA_WIDTH / CHAR_WIDTH)
#define TEXT_HEIGHT	(VGA_HEIGHT / CHAR_HEIGHT)
#define VGA_MEM		((unsigned char *) 0xA0000)
#define PAGE_OFFSET(x)	((x) * (VGA_WIDTH * VGA_HEIGHT / 8))

#define DEFAULT_FG_COLOR	0xa
#define DEFAULT_BG_COLOR	0x0

struct colored_char
{
  /* An Unicode codepoint.  */
  grub_uint32_t code;

  /* Color indexes.  */
  unsigned char fg_color;
  unsigned char bg_color;

  /* The width of this character minus one.  */
  unsigned char width;

  /* The column index of this character.  */
  unsigned char index;
};

static grub_dl_t my_mod;
static unsigned char text_mode;
static unsigned xpos, ypos;
static int cursor_state;
static unsigned char fg_color, bg_color;
static struct colored_char text_buf[TEXT_WIDTH * TEXT_HEIGHT];
static unsigned char saved_map_mask;
static int page = 0;

#define SEQUENCER_ADDR_PORT	0x3C4
#define SEQUENCER_DATA_PORT	0x3C5
#define MAP_MASK_REGISTER	0x02

#define CRTC_ADDR_PORT		0x3D4
#define CRTC_DATA_PORT		0x3D5
#define START_ADDR_HIGH_REGISTER 0x0C
#define START_ADDR_LOW_REGISTER	0x0D

#define GRAPHICS_ADDR_PORT	0x3CE
#define GRAPHICS_DATA_PORT	0x3CF
#define READ_MAP_REGISTER	0x04

#define INPUT_STATUS1_REGISTER	0x3DA
#define INPUT_STATUS1_VERTR_BIT 0x08

static inline void
wait_vretrace (void)
{
  /* Wait until there is a vertical retrace.  */
  while (! (grub_inb (INPUT_STATUS1_REGISTER) & INPUT_STATUS1_VERTR_BIT));
}

/* Get Map Mask Register.  */
static unsigned char
get_map_mask (void)
{
  unsigned char old_addr;
  unsigned char old_data;
  
  old_addr = grub_inb (SEQUENCER_ADDR_PORT);
  grub_outb (MAP_MASK_REGISTER, SEQUENCER_ADDR_PORT);
  
  old_data = grub_inb (SEQUENCER_DATA_PORT);
  
  grub_outb (old_addr, SEQUENCER_ADDR_PORT);

  return old_data;
}

/* Set Map Mask Register.  */
static void
set_map_mask (unsigned char mask)
{
  unsigned char old_addr;
  
  old_addr = grub_inb (SEQUENCER_ADDR_PORT);
  grub_outb (MAP_MASK_REGISTER, SEQUENCER_ADDR_PORT);
  
  grub_outb (mask, SEQUENCER_DATA_PORT);
  
  grub_outb (old_addr, SEQUENCER_ADDR_PORT);
}

/* Set Read Map Register.  */
static void
set_read_map (unsigned char map)
{
  unsigned char old_addr;
  
  old_addr = grub_inb (GRAPHICS_ADDR_PORT);

  grub_outb (READ_MAP_REGISTER, GRAPHICS_ADDR_PORT);
  grub_outb (map, GRAPHICS_DATA_PORT);

  grub_outb (old_addr, GRAPHICS_ADDR_PORT);
}

/* Set start address.  */
static void
set_start_address (unsigned int start)
{
  unsigned char old_addr;
  
  old_addr = grub_inb (CRTC_ADDR_PORT);
  
  grub_outb (START_ADDR_LOW_REGISTER, CRTC_ADDR_PORT);
  grub_outb (start & 0xFF, CRTC_DATA_PORT);
  
  grub_outb (START_ADDR_HIGH_REGISTER, CRTC_ADDR_PORT);
  grub_outb (start >> 8, CRTC_DATA_PORT);

  grub_outb (old_addr, CRTC_ADDR_PORT);
}

static grub_err_t
grub_vga_mod_init (void)
{
  text_mode = grub_vga_set_mode (0x10);
  cursor_state = 1;
  fg_color = DEFAULT_FG_COLOR;
  bg_color = DEFAULT_BG_COLOR;
  saved_map_mask = get_map_mask ();
  set_map_mask (0x0f);
  set_start_address (PAGE_OFFSET (page));
  
  return GRUB_ERR_NONE;
}

static grub_err_t
grub_vga_mod_fini (void)
{
  set_map_mask (saved_map_mask);
  grub_vga_set_mode (text_mode);
  return GRUB_ERR_NONE;
}

static int
check_vga_mem (void *p)
{
  return (p >= (void *) (VGA_MEM + PAGE_OFFSET (page))
	  && p <= (void *) (VGA_MEM + PAGE_OFFSET (page)
			    + VGA_WIDTH * VGA_HEIGHT / 8));
}

static void
write_char (void)
{
  struct colored_char *p = text_buf + xpos + ypos * TEXT_WIDTH;
  struct grub_font_glyph glyph;
  unsigned char *mem_base;
  unsigned plane;

  mem_base = (VGA_MEM + xpos + 
	      ypos * CHAR_HEIGHT * TEXT_WIDTH + PAGE_OFFSET (page)) - p->index;
  p -= p->index;

  /* Get glyph for character.  */
  grub_font_get_glyph (p->code, &glyph);
  
  for (plane = 0x01; plane <= 0x08; plane <<= 1)
    {
      unsigned y;
      unsigned offset;
      unsigned char *mem;

      set_map_mask (plane);

      for (y = 0, offset = 0, mem = mem_base;
	   y < CHAR_HEIGHT;
	   y++, mem += TEXT_WIDTH)
	{
	  unsigned i;

	  for (i = 0; i < glyph.char_width && offset < 32; i++)
	    {
	      unsigned char fg_mask, bg_mask;
	      
	      fg_mask = (p->fg_color & plane) ? glyph.bitmap[offset] : 0;
	      bg_mask = (p->bg_color & plane) ? ~(glyph.bitmap[offset]) : 0;
	      offset++;

	      if (check_vga_mem (mem + i))
		mem[i] = (fg_mask | bg_mask);
	    }
	}
    }

  set_map_mask (0x0f);
}

static void
write_cursor (void)
{
  unsigned char *mem = (VGA_MEM + PAGE_OFFSET (page) + xpos
			+ (ypos * CHAR_HEIGHT + CHAR_HEIGHT - 3) * TEXT_WIDTH);
  if (check_vga_mem (mem))
    *mem = 0xff;
  
  mem += TEXT_WIDTH;
  if (check_vga_mem (mem))
    *mem = 0xff;
}

static void
scroll_up (void)
{
  unsigned i;
  unsigned plane;
  
  /* Do all the work in the other page.  */
  grub_memmove (text_buf, text_buf + TEXT_WIDTH,
		sizeof (struct colored_char) * TEXT_WIDTH * (TEXT_HEIGHT - 1));
      
  for (i = TEXT_WIDTH * (TEXT_HEIGHT - 1); i < TEXT_WIDTH * TEXT_HEIGHT; i++)
    {
      text_buf[i].code = ' ';
      text_buf[i].fg_color = 0;
      text_buf[i].bg_color = 0;
      text_buf[i].width = 0;
      text_buf[i].index = 0;
    }

  for (plane = 1; plane <= 4; plane++)
    {
      set_read_map (plane);
      set_map_mask (1 << plane);
      grub_memmove (VGA_MEM + PAGE_OFFSET (1 - page), VGA_MEM
		    + PAGE_OFFSET (page) + VGA_WIDTH * CHAR_HEIGHT / 8,
		    VGA_WIDTH * (VGA_HEIGHT - CHAR_HEIGHT) / 8);
    }

  set_map_mask (0x0f);
  grub_memset (VGA_MEM + PAGE_OFFSET (1 - page)
	       + VGA_WIDTH * (VGA_HEIGHT - CHAR_HEIGHT) / 8, 0,
	       VGA_WIDTH * CHAR_HEIGHT / 8);
  
  /* Activate the other page.  */
  page = 1 - page;
  wait_vretrace ();
  set_start_address (PAGE_OFFSET (page));
}

static void
grub_vga_putchar (grub_uint32_t c)
{
#if DEBUG_VGA
  static int show = 1;
#endif
  
  if (c == '\a')
    /* FIXME */
    return;

  if (c == '\b' || c == '\n' || c == '\r')
    {
      /* Erase current cursor, if any.  */
      if (cursor_state)
	write_char ();
  
      switch (c)
	{
	case '\b':
	  if (xpos > 0)
	    xpos--;
	  break;
	  
	case '\n':
	  if (ypos >= TEXT_HEIGHT - 1)
	    scroll_up ();
	  else
	    ypos++;
	  break;
	  
	case '\r':
	  xpos = 0;
	  break;
	}

      if (cursor_state)
	write_cursor ();
    }
  else
    {
      struct grub_font_glyph glyph;
      struct colored_char *p;
      
      grub_font_get_glyph(c, &glyph);

      if (xpos + glyph.char_width > TEXT_WIDTH)
	grub_putchar ('\n');

      p = text_buf + xpos + ypos * TEXT_WIDTH;
      p->code = c;
      p->fg_color = fg_color;
      p->bg_color = bg_color;
      p->width = glyph.char_width - 1;
      p->index = 0;

      if (glyph.char_width > 1)
	{
	  unsigned i;

	  for (i = 1; i < glyph.char_width; i++)
	    {
	      p[i].code = ' ';
	      p[i].width = glyph.char_width - 1;
	      p[i].index = i;
	    }
	}
	  
      write_char ();
  
      xpos += glyph.char_width;
      if (xpos >= TEXT_WIDTH)
	{
	  xpos = 0;
	  
	  if (ypos >= TEXT_HEIGHT - 1)
	    scroll_up ();
	  else
	    ypos++;
	}

      if (cursor_state)
	write_cursor ();
    }

#if DEBUG_VGA
  if (show)
    {
      grub_uint16_t pos = grub_getxy ();

      show = 0;
      grub_gotoxy (0, 0);
      grub_printf ("[%u:%u]", (unsigned) (pos >> 8), (unsigned) (pos & 0xff));
      grub_gotoxy (pos >> 8, pos & 0xff);
      show = 1;
    }
#endif
}

static grub_ssize_t
grub_vga_getcharwidth (grub_uint32_t c)
{
  struct grub_font_glyph glyph;
  
  grub_font_get_glyph (c, &glyph);
  
  return glyph.char_width;
}

static grub_uint16_t
grub_vga_getwh (void)
{
  return (TEXT_WIDTH << 8) | TEXT_HEIGHT;
}

static grub_uint16_t
grub_vga_getxy (void)
{
  return ((xpos << 8) | ypos);
}

static void
grub_vga_gotoxy (grub_uint8_t x, grub_uint8_t y)
{
  if (x >= TEXT_WIDTH || y >= TEXT_HEIGHT)
    {
      grub_error (GRUB_ERR_OUT_OF_RANGE, "invalid point (%u,%u)",
		  (unsigned) x, (unsigned) y);
      return;
    }

  if (cursor_state)
    write_char ();

  xpos = x;
  ypos = y;

  if (cursor_state)
    write_cursor ();
}

static void
grub_vga_cls (void)
{
  unsigned i;

  wait_vretrace ();
  for (i = 0; i < TEXT_WIDTH * TEXT_HEIGHT; i++)
    {
      text_buf[i].code = ' ';
      text_buf[i].fg_color = 0;
      text_buf[i].bg_color = 0;
      text_buf[i].width = 0;
      text_buf[i].index = 0;
    }

  grub_memset (VGA_MEM + PAGE_OFFSET (page), 0, VGA_WIDTH * VGA_HEIGHT / 8);

  xpos = ypos = 0;
}

static void
grub_vga_setcolorstate (grub_term_color_state state)
{
  switch (state)
    {
    case GRUB_TERM_COLOR_STANDARD:
    case GRUB_TERM_COLOR_NORMAL:
      fg_color = DEFAULT_FG_COLOR;
      bg_color = DEFAULT_BG_COLOR;
      break;
    case GRUB_TERM_COLOR_HIGHLIGHT:
      fg_color = DEFAULT_BG_COLOR;
      bg_color = DEFAULT_FG_COLOR;
      break;
    default:
      break;
    }
}

static void
grub_vga_setcursor (int on)
{
  if (cursor_state != on)
    {
      if (cursor_state)
	write_char ();
      else
	write_cursor ();

      cursor_state = on;
    }
}

static struct grub_term grub_vga_term =
  {
    .name = "vga",
    .init = grub_vga_mod_init,
    .fini = grub_vga_mod_fini,
    .putchar = grub_vga_putchar,
    .getcharwidth = grub_vga_getcharwidth,
    .checkkey = grub_console_checkkey,
    .getkey = grub_console_getkey,
    .getwh = grub_vga_getwh,
    .getxy = grub_vga_getxy,
    .gotoxy = grub_vga_gotoxy,
    .cls = grub_vga_cls,
    .setcolorstate = grub_vga_setcolorstate,
    .setcursor = grub_vga_setcursor,
    .flags = 0,
    .next = 0
  };

GRUB_MOD_INIT(vga)
{
#ifndef GRUB_UTIL
  my_mod = mod;
#endif
  grub_term_register (&grub_vga_term);
}

GRUB_MOD_FINI(vga)
{
  grub_term_unregister (&grub_vga_term);
}
