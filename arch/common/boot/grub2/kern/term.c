/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2002,2003,2005,2007,2008,2009  Free Software Foundation, Inc.
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

#include <grub/term.h>
#include <grub/err.h>
#include <grub/mm.h>
#include <grub/misc.h>
#include <grub/env.h>

/* The amount of lines counted by the pager.  */
static int grub_more_lines;

/* If the more pager is active.  */
static int grub_more;

/* The current cursor state.  */
static int cursor_state = 1;

struct grub_handler_class grub_term_input_class =
      {
    .name = "terminal_input"
  };

struct grub_handler_class grub_term_output_class =
      {
    .name = "terminal_output"
  };

#define grub_cur_term_input	grub_term_get_current_input ()
#define grub_cur_term_output	grub_term_get_current_output ()

/* Put a Unicode character.  */
void
grub_putcode (grub_uint32_t code)
{
  int height = grub_getwh () & 255;

  if (code == '\t' && grub_cur_term_output->getxy)
    {
      int n;

      n = 8 - ((grub_getxy () >> 8) & 7);
      while (n--)
	grub_putcode (' ');

      return;
    }

  (grub_cur_term_output->putchar) (code);

  if (code == '\n')
    {
      grub_putcode ('\r');

      grub_more_lines++;

      if (grub_more && grub_more_lines == height - 1)
	{
	  char key;
	  int pos = grub_getxy ();

	  /* Show --MORE-- on the lower left side of the screen.  */
	  grub_gotoxy (1, height - 1);
	  grub_setcolorstate (GRUB_TERM_COLOR_HIGHLIGHT);
	  grub_printf ("--MORE--");
	  grub_setcolorstate (GRUB_TERM_COLOR_STANDARD);

	  key = grub_getkey ();

	  /* Remove the message.  */
	  grub_gotoxy (1, height - 1);
	  grub_printf ("        ");
	  grub_gotoxy (pos >> 8, pos & 0xFF);

	  /* Scroll one lines or an entire page, depending on the key.  */
	  if (key == '\r' || key =='\n')
	    grub_more_lines--;
	  else
	    grub_more_lines = 0;
	}
    }
}

/* Put a character. C is one byte of a UTF-8 stream.
   This function gathers bytes until a valid Unicode character is found.  */
void
grub_putchar (int c)
{
  static grub_size_t size = 0;
  static grub_uint8_t buf[6];
  grub_uint32_t code;
  grub_ssize_t ret;

  buf[size++] = c;
  ret = grub_utf8_to_ucs4 (&code, 1, buf, size, 0);

  if (ret > 0)
    {
      size = 0;
      grub_putcode (code);
    }
  else if (ret < 0)
    {
      size = 0;
      grub_putcode ('?');
    }
}

/* Return the number of columns occupied by the character code CODE.  */
grub_ssize_t
grub_getcharwidth (grub_uint32_t code)
{
  return (grub_cur_term_output->getcharwidth) (code);
}

int
grub_getkey (void)
{
  return (grub_cur_term_input->getkey) ();
}

int
grub_checkkey (void)
{
  return (grub_cur_term_input->checkkey) ();
}

int
grub_getkeystatus (void)
{
  if (grub_cur_term_input->getkeystatus)
    return (grub_cur_term_input->getkeystatus) ();
  else
    return 0;
}

grub_uint16_t
grub_getxy (void)
{
  return (grub_cur_term_output->getxy) ();
}

grub_uint16_t
grub_getwh (void)
{
  return (grub_cur_term_output->getwh) ();
}

void
grub_gotoxy (grub_uint8_t x, grub_uint8_t y)
{
  (grub_cur_term_output->gotoxy) (x, y);
}

void
grub_cls (void)
{
  if ((grub_cur_term_output->flags & GRUB_TERM_DUMB) || (grub_env_get ("debug")))
    {
      grub_putchar ('\n');
      grub_refresh ();
    }
  else
    (grub_cur_term_output->cls) ();
}

void
grub_setcolorstate (grub_term_color_state state)
{
  if (grub_cur_term_output->setcolorstate)
    (grub_cur_term_output->setcolorstate) (state);
}

void
grub_setcolor (grub_uint8_t normal_color, grub_uint8_t highlight_color)
{
  if (grub_cur_term_output->setcolor)
    (grub_cur_term_output->setcolor) (normal_color, highlight_color);
}

void
grub_getcolor (grub_uint8_t *normal_color, grub_uint8_t *highlight_color)
{
  if (grub_cur_term_output->getcolor)
    (grub_cur_term_output->getcolor) (normal_color, highlight_color);
}

int
grub_setcursor (int on)
{
  int ret = cursor_state;

  if (grub_cur_term_output->setcursor)
    {
      (grub_cur_term_output->setcursor) (on);
      cursor_state = on;
    }

  return ret;
}

int
grub_getcursor (void)
{
  return cursor_state;
}

void
grub_refresh (void)
{
  if (grub_cur_term_output->refresh)
    (grub_cur_term_output->refresh) ();
}

void
grub_set_more (int onoff)
{
  if (onoff == 1)
    grub_more++;
  else
    grub_more--;

  grub_more_lines = 0;
}
