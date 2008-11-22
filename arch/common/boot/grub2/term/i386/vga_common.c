/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2002,2003,2005,2007,2008  Free Software Foundation, Inc.
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

#include <grub/i386/vga_common.h>
#include <grub/term.h>
#include <grub/types.h>

grub_uint8_t grub_console_cur_color = 0x7;
static grub_uint8_t grub_console_standard_color = 0x7;
static grub_uint8_t grub_console_normal_color = 0x7;
static grub_uint8_t grub_console_highlight_color = 0x70;

static grub_uint32_t
map_char (grub_uint32_t c)
{
  if (c > 0x7f)
    {
      /* Map some unicode characters to the VGA font, if possible.  */
      switch (c)
	{
	case 0x2190:	/* left arrow */
	  c = 0x1b;
	  break;
	case 0x2191:	/* up arrow */
	  c = 0x18;
	  break;
	case 0x2192:	/* right arrow */
	  c = 0x1a;
	  break;
	case 0x2193:	/* down arrow */
	  c = 0x19;
	  break;
	case 0x2501:	/* horizontal line */
	  c = 0xc4;
	  break;
	case 0x2503:	/* vertical line */
	  c = 0xb3;
	  break;
	case 0x250F:	/* upper-left corner */
	  c = 0xda;
	  break;
	case 0x2513:	/* upper-right corner */
	  c = 0xbf;
	  break;
	case 0x2517:	/* lower-left corner */
	  c = 0xc0;
	  break;
	case 0x251B:	/* lower-right corner */
	  c = 0xd9;
	  break;

	default:
	  c = '?';
	  break;
	}
    }

  return c;
}

void
grub_console_putchar (grub_uint32_t c)
{
  grub_console_real_putchar (map_char (c));
}

grub_ssize_t
grub_console_getcharwidth (grub_uint32_t c __attribute__ ((unused)))
{
  /* For now, every printable character has the width 1.  */
  return 1;
}

grub_uint16_t
grub_console_getwh (void)
{
  return (80 << 8) | 25;
}

void
grub_console_setcolorstate (grub_term_color_state state)
{
  switch (state) {
    case GRUB_TERM_COLOR_STANDARD:
      grub_console_cur_color = grub_console_standard_color;
      break;
    case GRUB_TERM_COLOR_NORMAL:
      grub_console_cur_color = grub_console_normal_color;
      break;
    case GRUB_TERM_COLOR_HIGHLIGHT:
      grub_console_cur_color = grub_console_highlight_color;
      break;
    default:
      break;
  }
}

void
grub_console_setcolor (grub_uint8_t normal_color, grub_uint8_t highlight_color)
{
  grub_console_normal_color = normal_color;
  grub_console_highlight_color = highlight_color;
}

void
grub_console_getcolor (grub_uint8_t *normal_color, grub_uint8_t *highlight_color)
{
  *normal_color = grub_console_normal_color;
  *highlight_color = grub_console_highlight_color;
}
