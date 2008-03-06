/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2007  Free Software Foundation, Inc.
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

#include <grub/machine/console.h>
#include <grub/cpu/io.h>
#include <grub/types.h>

#define COLS	80
#define ROWS	25

static int grub_curr_x, grub_curr_y;

#define VGA_TEXT_SCREEN		0xb8000

#define CRTC_ADDR_PORT		0x3D4
#define CRTC_DATA_PORT		0x3D5

#define CRTC_CURSOR		0x0a
#define CRTC_CURSOR_ADDR_HIGH	0x0e
#define CRTC_CURSOR_ADDR_LOW	0x0f

static void
screen_write_char (int x, int y, short c)
{
  ((short *) VGA_TEXT_SCREEN)[y * COLS + x] = c;
}

static short
screen_read_char (int x, int y)
{
  return ((short *) VGA_TEXT_SCREEN)[y * COLS + x];
}

static void
update_cursor (void)
{
  unsigned int pos = grub_curr_y * COLS + grub_curr_x;
  grub_outb (CRTC_CURSOR_ADDR_HIGH, CRTC_ADDR_PORT);
  grub_outb (pos >> 8, CRTC_DATA_PORT);
  grub_outb (CRTC_CURSOR_ADDR_LOW, CRTC_ADDR_PORT);
  grub_outb (pos & 0xFF, CRTC_DATA_PORT);
}

static void
inc_y (void)
{
  grub_curr_x = 0;
  if (grub_curr_y < ROWS - 1)
    grub_curr_y++;
  else
    {
      int x, y;
      for (y = 0; y < ROWS; y++)
        for (x = 0; x < COLS; x++)
          screen_write_char (x, y, screen_read_char (x, y + 1));
    }
}

static void
inc_x (void)
{
  if (grub_curr_x >= COLS - 2)
    inc_y ();
  else
    grub_curr_x++;
}

void
grub_console_real_putchar (int c)
{
  switch (c)
    {
      case '\b':
	if (grub_curr_x != 0)
	  screen_write_char (grub_curr_x--, grub_curr_y, ' ');
	break;
      case '\n':
	inc_y ();
	break;
      case '\r':
	grub_curr_x = 0;
	break;
      default:
	screen_write_char (grub_curr_x,
			   grub_curr_y, c | (grub_console_cur_color << 8));
	inc_x ();
    }

  update_cursor ();
}

grub_uint16_t
grub_console_getxy (void)
{
  return (grub_curr_x << 8) | grub_curr_y;
}

void
grub_console_gotoxy (grub_uint8_t x, grub_uint8_t y)
{
  grub_curr_x = x;
  grub_curr_y = y;
  update_cursor ();
}

void
grub_console_cls (void)
{
  int i;
  for (i = 0; i < ROWS * COLS; i++)
    ((short *) VGA_TEXT_SCREEN)[i] = ' ' | (grub_console_cur_color << 8);
}

void
grub_console_setcursor (int on)
{
  grub_uint8_t old;
  grub_outb (CRTC_CURSOR, CRTC_ADDR_PORT);
  old = grub_inb (CRTC_DATA_PORT);
  grub_outb ((old & ~(on << 5)), CRTC_DATA_PORT);
}
