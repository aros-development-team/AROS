/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2005  Free Software Foundation, Inc.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <grub/machine/memory.h>
#include <grub/machine/vga.h>
#include <grub/machine/vbe.h>
#include <grub/machine/console.h>
#include <grub/term.h>
#include <grub/types.h>
#include <grub/dl.h>
#include <grub/misc.h>
#include <grub/normal.h>
#include <grub/font.h>
#include <grub/arg.h>
#include <grub/mm.h>
#include <grub/env.h>

#define DEFAULT_CHAR_WIDTH  8
#define DEFAULT_CHAR_HEIGHT 16

#define DEFAULT_FG_COLOR    0xa
#define DEFAULT_BG_COLOR    0x0

struct grub_colored_char
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

struct grub_virtual_screen
{
  /* Dimensions of the virual screen.  */
  grub_uint32_t width;
  grub_uint32_t height;

  /* Offset in the display.  */
  grub_uint32_t offset_x;
  grub_uint32_t offset_y;

  /* TTY Character sizes.  */
  grub_uint32_t char_width;
  grub_uint32_t char_height;

  /* Virtual screen TTY size.  */
  grub_uint32_t columns;
  grub_uint32_t rows;

  /* Current cursor details.  */
  grub_uint32_t cursor_x;
  grub_uint32_t cursor_y;
  grub_uint8_t cursor_state;
  grub_uint8_t fg_color;
  grub_uint8_t bg_color;

  /* Text buffer for virual screen. Contains (columns * rows) number
     of entries.  */
  struct grub_colored_char *text_buffer;
};

/* Make seure text buffer is not marked as allocated.  */
static struct grub_virtual_screen virtual_screen =
  {
    .text_buffer = 0
  };

static grub_dl_t my_mod;
static unsigned char *vga_font = 0;
static grub_uint32_t old_mode = 0;

static struct grub_vbe_mode_info_block mode_info;
static grub_uint8_t *framebuffer = 0;
static grub_uint32_t bytes_per_scan_line = 0;

static void
grub_virtual_screen_free (void)
{
  /* If virtual screen has been allocated, free it.  */
  if (virtual_screen.text_buffer != 0)
    grub_free (virtual_screen.text_buffer);

  /* Reset virtual screen data.  */
  grub_memset (&virtual_screen, 0, sizeof (virtual_screen));
}

static grub_err_t
grub_virtual_screen_setup (grub_uint32_t width,
			   grub_uint32_t height)
{
  /* Free old virtual screen.  */
  grub_virtual_screen_free ();

  /* Initialize with default data.  */
  virtual_screen.width = width;
  virtual_screen.height = height;
  virtual_screen.offset_x = 0;
  virtual_screen.offset_y = 0;
  virtual_screen.char_width = DEFAULT_CHAR_WIDTH;
  virtual_screen.char_height = DEFAULT_CHAR_HEIGHT;
  virtual_screen.cursor_x = 0;
  virtual_screen.cursor_y = 0;
  virtual_screen.cursor_state = 1;
  virtual_screen.fg_color = DEFAULT_FG_COLOR;
  virtual_screen.bg_color = DEFAULT_BG_COLOR;

  /* Calculate size of text buffer.  */
  virtual_screen.columns = virtual_screen.width / virtual_screen.char_width;
  virtual_screen.rows = virtual_screen.height / virtual_screen.char_height;

  /* Allocate memory for text buffer.  */
  virtual_screen.text_buffer =
    (struct grub_colored_char *) grub_malloc (virtual_screen.columns
					      * virtual_screen.rows
					      * sizeof (*virtual_screen.text_buffer));

  return grub_errno;
}

static grub_err_t
grub_vesafb_init (void)
{
  grub_uint32_t use_mode = GRUB_VBE_DEFAULT_VIDEO_MODE;
  struct grub_vbe_info_block controller_info;
  char *modevar;

  /* Use fonts from VGA bios.  */
  vga_font = grub_vga_get_font ();

  /* Check if we have VESA BIOS installed.  */
  if (grub_vbe_probe (&controller_info) != GRUB_ERR_NONE)
    return grub_errno;

  /* Check existence of vbe_mode environment variable.  */
  modevar = grub_env_get ("vbe_mode");

  if (modevar != 0)
    {
      unsigned long value;

      value = grub_strtoul (modevar, 0, 0);
      if (grub_errno == GRUB_ERR_NONE)
	use_mode = value;
    }

  /* Store initial video mode.  */
  if (grub_vbe_get_video_mode (&old_mode) != GRUB_ERR_NONE)
    return grub_errno;

  /* Setup desired graphics mode.  */
  if (grub_vbe_set_video_mode (use_mode, &mode_info) != GRUB_ERR_NONE)
    return grub_errno;

  /* Determine framebuffer and bytes per scan line.  */
  framebuffer = (grub_uint8_t *) mode_info.phys_base_addr;

  if (controller_info.version >= 0x300)
    bytes_per_scan_line = mode_info.lin_bytes_per_scan_line;
  else
    bytes_per_scan_line = mode_info.bytes_per_scan_line;

  /* Create virtual screen.  */
  if (grub_virtual_screen_setup (mode_info.x_resolution,
				 mode_info.y_resolution) != GRUB_ERR_NONE)
    {
      grub_vbe_set_video_mode (old_mode, 0);
      return grub_errno;
    }

  /* Make sure frame buffer is black.  */
  grub_memset (framebuffer,
	       0,
	       bytes_per_scan_line * mode_info.y_resolution);

  return GRUB_ERR_NONE;
}

static grub_err_t
grub_vesafb_fini (void)
{
  grub_virtual_screen_free ();

  grub_vbe_set_video_mode (old_mode, 0);

  return GRUB_ERR_NONE;
}

static int
grub_virtual_screen_get_glyph (grub_uint32_t code,
			       unsigned char bitmap[32],
			       unsigned *width)
{
  if (code > 0x7f)
    {
      /* Map some unicode characters to the VGA font, if possible.  */
      switch (code)
	{
	case 0x2190:	/* left arrow */
	  code = 0x1b;
	  break;
	case 0x2191:	/* up arrow */
	  code = 0x18;
	  break;
	case 0x2192:	/* right arrow */
	  code = 0x1a;
	  break;
	case 0x2193:	/* down arrow */
	  code = 0x19;
	  break;
	case 0x2501:	/* horizontal line */
	  code = 0xc4;
	  break;
	case 0x2503:	/* vertical line */
	  code = 0xb3;
	  break;
	case 0x250F:	/* upper-left corner */
	  code = 0xda;
	  break;
	case 0x2513:	/* upper-right corner */
	  code = 0xbf;
	  break;
	case 0x2517:	/* lower-left corner */
	  code = 0xc0;
	  break;
	case 0x251B:	/* lower-right corner */
	  code = 0xd9;
	  break;

	default:
	  return grub_font_get_glyph (code, bitmap, width);
	}
    }

  if (bitmap)
    grub_memcpy (bitmap,
		 vga_font + code * virtual_screen.char_height,
		 virtual_screen.char_height);
  *width = 1;
  return 1;
}

static void
grub_virtual_screen_invalidate_char (struct grub_colored_char *p)
{
  p->code = 0xFFFF;
  
  if (p->width)
    {
      struct grub_colored_char *q;

      for (q = p + 1; q <= p + p->width; q++)
	{
	  q->code = 0xFFFF;
	  q->width = 0;
	  q->index = 0;
	}
    }

  p->width = 0;
}

static void
write_char (void)
{
  struct grub_colored_char *p;
  unsigned char bitmap[32];
  unsigned width;
  unsigned y;
  unsigned offset;

  p = (virtual_screen.text_buffer
       + virtual_screen.cursor_x
       + (virtual_screen.cursor_y * virtual_screen.columns));

  p -= p->index;

  if (! grub_virtual_screen_get_glyph (p->code, bitmap, &width))
    {
      grub_virtual_screen_invalidate_char (p);
      width = 0;
    }

  for (y = 0, offset = 0;
       y < virtual_screen.char_height;
       y++, offset++)
    {
      unsigned i;

      for (i = 0;
	   (i < width * virtual_screen.char_width) && (offset < 32);
	   i++)
	{
	  unsigned char color;

	  if (bitmap[offset] & (1 << (8-i)))
	    {
	      color = p->fg_color;
	    }
	  else
	    {
	      color = p->bg_color;
	    }

          grub_vbe_set_pixel_index(i + (virtual_screen.cursor_x
                                        * virtual_screen.char_width),
                                   y + (virtual_screen.cursor_y
                                        * virtual_screen.char_height),
                                   color);
	}
    }
}

static void
write_cursor (void)
{
  grub_uint32_t x;
  grub_uint32_t y;

  for (y = ((virtual_screen.cursor_y + 1) * virtual_screen.char_height) - 3;
       y < ((virtual_screen.cursor_y + 1) * virtual_screen.char_height) - 1;
       y++)
    {
      for (x = virtual_screen.cursor_x * virtual_screen.char_width;
	   x < (virtual_screen.cursor_x + 1) * virtual_screen.char_width;
	   x++)
	{
	  grub_vbe_set_pixel_index(x, y, 10);
	}
    }
}

static void
scroll_up (void)
{
  grub_uint32_t i;

  /* Scroll text buffer with one line to up.  */
  grub_memmove (virtual_screen.text_buffer,
		virtual_screen.text_buffer + virtual_screen.columns,
                sizeof (*virtual_screen.text_buffer)
                * virtual_screen.columns
                * (virtual_screen.rows - 1));

  /* Clear last line in text buffer.  */
  for (i = virtual_screen.columns * (virtual_screen.rows - 1);
       i < virtual_screen.columns * virtual_screen.rows;
       i++)
    {
      virtual_screen.text_buffer[i].code = ' ';
      virtual_screen.text_buffer[i].fg_color = 0;
      virtual_screen.text_buffer[i].bg_color = 0;
      virtual_screen.text_buffer[i].width = 0;
      virtual_screen.text_buffer[i].index = 0;
    }

  /* Scroll frambuffer with one line to up.  */
  grub_memmove (framebuffer,
                framebuffer
                + bytes_per_scan_line * virtual_screen.char_height,
                bytes_per_scan_line
                * (mode_info.y_resolution - virtual_screen.char_height));

  /* Clear last line in framebuffer.  */
  grub_memset (framebuffer
               + (bytes_per_scan_line
                  * (mode_info.y_resolution - virtual_screen.char_height)),
               0,
               bytes_per_scan_line * virtual_screen.char_height);
}

static void
grub_vesafb_putchar (grub_uint32_t c)
{
  if (c == '\a')
    /* FIXME */
    return;

  if (c == '\b' || c == '\n' || c == '\r')
    {
      /* Erase current cursor, if any.  */
      if (virtual_screen.cursor_state)
	write_char ();

      switch (c)
	{
	case '\b':
	  if (virtual_screen.cursor_x > 0)
	    virtual_screen.cursor_x--;
	  break;
	  
	case '\n':
	  if (virtual_screen.cursor_y >= virtual_screen.rows - 1)
	    scroll_up ();
	  else
	    virtual_screen.cursor_y++;
	  break;
	  
	case '\r':
	  virtual_screen.cursor_x = 0;
	  break;
	}

      if (virtual_screen.cursor_state)
	write_cursor ();
    }
  else
    {
      unsigned width;
      struct grub_colored_char *p;
      
      grub_virtual_screen_get_glyph (c, 0, &width);

      if (virtual_screen.cursor_x + width > virtual_screen.columns)
	grub_putchar ('\n');

      p = (virtual_screen.text_buffer +
	   virtual_screen.cursor_x +
	   virtual_screen.cursor_y * virtual_screen.columns);
      p->code = c;
      p->fg_color = virtual_screen.fg_color;
      p->bg_color = virtual_screen.bg_color;
      p->width = width - 1;
      p->index = 0;

      if (width > 1)
	{
	  unsigned i;

	  for (i = 1; i < width; i++)
	    {
	      p[i].code = ' ';
	      p[i].width = width - 1;
	      p[i].index = i;
	    }
	}
	  
      write_char ();
  
      virtual_screen.cursor_x += width;
      if (virtual_screen.cursor_x >= virtual_screen.columns)
	{
	  virtual_screen.cursor_x = 0;
	  
	  if (virtual_screen.cursor_y >= virtual_screen.rows - 1)
	    scroll_up ();
	  else
	    virtual_screen.cursor_y++;
	}

      if (virtual_screen.cursor_state)
	write_cursor ();
    }
}

static grub_ssize_t
grub_vesafb_getcharwidth (grub_uint32_t c)
{
  unsigned width;
  
  if (! grub_virtual_screen_get_glyph (c, 0, &width))
    return 0;

  return width;
}

static grub_uint16_t
grub_virtual_screen_getwh (void)
{
  return (virtual_screen.columns << 8) | virtual_screen.rows;
}

static grub_uint16_t
grub_virtual_screen_getxy (void)
{
  return ((virtual_screen.cursor_x << 8) | virtual_screen.cursor_y);
}

static void
grub_vesafb_gotoxy (grub_uint8_t x, grub_uint8_t y)
{
  if (x >= virtual_screen.columns || y >= virtual_screen.rows)
    {
      grub_error (GRUB_ERR_OUT_OF_RANGE, "invalid point (%u,%u)",
		  (unsigned) x, (unsigned) y);
      return;
    }

  if (virtual_screen.cursor_state)
    write_char ();

  virtual_screen.cursor_x = x;
  virtual_screen.cursor_y = y;

  if (virtual_screen.cursor_state)
    write_cursor ();
}

static void
grub_virtual_screen_cls (void)
{
  grub_uint32_t i;

  for (i = 0; i < virtual_screen.columns * virtual_screen.rows; i++)
    {
      virtual_screen.text_buffer[i].code = ' ';
      virtual_screen.text_buffer[i].fg_color = 0;
      virtual_screen.text_buffer[i].bg_color = 0;
      virtual_screen.text_buffer[i].width = 0;
      virtual_screen.text_buffer[i].index = 0;
    }

  virtual_screen.cursor_x = virtual_screen.cursor_y = 0;
}

static void
grub_vesafb_cls (void)
{
  grub_virtual_screen_cls ();

  grub_memset (framebuffer,
               0, 
	       mode_info.y_resolution * bytes_per_scan_line);
}

static void
grub_virtual_screen_setcolorstate (grub_term_color_state state)
{
  switch (state)
    {
    case GRUB_TERM_COLOR_STANDARD:
    case GRUB_TERM_COLOR_NORMAL:
      virtual_screen.fg_color = DEFAULT_FG_COLOR;
      virtual_screen.bg_color = DEFAULT_BG_COLOR;
      break;
    case GRUB_TERM_COLOR_HIGHLIGHT:
      virtual_screen.fg_color = DEFAULT_BG_COLOR;
      virtual_screen.bg_color = DEFAULT_FG_COLOR;
      break;
    default:
      break;
    }
}

static void
grub_virtual_screen_setcolor (grub_uint8_t normal_color __attribute__ ((unused)),
			      grub_uint8_t highlight_color __attribute__ ((unused)))
{
  /* FIXME */
}

static void
grub_vesafb_setcursor (int on)
{
  if (virtual_screen.cursor_state != on)
    {
      if (virtual_screen.cursor_state)
	write_char ();
      else
	write_cursor ();

      virtual_screen.cursor_state = on;
    }
}

static struct grub_term grub_vesafb_term =
  {
    .name = "vesafb",
    .init = grub_vesafb_init,
    .fini = grub_vesafb_fini,
    .putchar = grub_vesafb_putchar,
    .getcharwidth = grub_vesafb_getcharwidth,
    .checkkey = grub_console_checkkey,
    .getkey = grub_console_getkey,
    .getwh = grub_virtual_screen_getwh,
    .getxy = grub_virtual_screen_getxy,
    .gotoxy = grub_vesafb_gotoxy,
    .cls = grub_vesafb_cls,
    .setcolorstate = grub_virtual_screen_setcolorstate,
    .setcolor = grub_virtual_screen_setcolor,
    .setcursor = grub_vesafb_setcursor,
    .flags = 0,
    .next = 0
  };

GRUB_MOD_INIT
{
  my_mod = mod;
  grub_term_register (&grub_vesafb_term);
}

GRUB_MOD_FINI
{
  grub_term_unregister (&grub_vesafb_term);
}
