/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2002,2003,2005  Free Software Foundation, Inc.
 *
 *  GRUB is free software; you can redistribute it and/or modify
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
 *  along with GRUB; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <grub/term.h>
#include <grub/err.h>
#include <grub/mm.h>
#include <grub/misc.h>

/* The list of terminals.  */
static grub_term_t grub_term_list;

/* The current terminal.  */
static grub_term_t grub_cur_term;

/* The amount of lines counted by the pager.  */
static int grub_more_lines;

/* If the more pager is active.  */
static int grub_more;

/* The current cursor state.  */
static int cursor_state = 1;

void
grub_term_register (grub_term_t term)
{
  term->next = grub_term_list;
  grub_term_list = term;
}

void
grub_term_unregister (grub_term_t term)
{
  grub_term_t *p, q;
  
  for (p = &grub_term_list, q = *p; q; p = &(q->next), q = q->next)
    if (q == term)
      {
        *p = q->next;
	break;
      }
}

void
grub_term_iterate (int (*hook) (grub_term_t term))
{
  grub_term_t p;
  
  for (p = grub_term_list; p; p = p->next)
    if (hook (p))
      break;
}

grub_err_t
grub_term_set_current (grub_term_t term)
{
  if (grub_cur_term && grub_cur_term->fini)
    if ((grub_cur_term->fini) () != GRUB_ERR_NONE)
      return grub_errno;

  if (term->init)
    if ((term->init) () != GRUB_ERR_NONE)
      return grub_errno;
  
  grub_cur_term = term;
  grub_cls ();
  grub_setcursor (grub_getcursor ());
  return GRUB_ERR_NONE;
}

grub_term_t
grub_term_get_current (void)
{
  return grub_cur_term;
}

/* Put a Unicode character.  */
void
grub_putcode (grub_uint32_t code)
{
  int height = grub_getwh () & 255;

  if (code == '\t' && grub_cur_term->getxy)
    {
      int n;
      
      n = 8 - ((grub_getxy () >> 8) & 7);
      while (n--)
	grub_putcode (' ');

      return;
    }
  
  (grub_cur_term->putchar) (code);
  
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
  ret = grub_utf8_to_ucs4 (&code, buf, size);
  
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
  return (grub_cur_term->getcharwidth) (code);
}

int
grub_getkey (void)
{
  return (grub_cur_term->getkey) ();
}

int
grub_checkkey (void)
{
  return (grub_cur_term->checkkey) ();
}

grub_uint16_t
grub_getxy (void)
{
  return (grub_cur_term->getxy) ();
}

grub_uint16_t
grub_getwh (void)
{
  return (grub_cur_term->getwh) ();
}

void
grub_gotoxy (grub_uint8_t x, grub_uint8_t y)
{
  (grub_cur_term->gotoxy) (x, y);
}

void
grub_cls (void)
{
  if (grub_cur_term->flags & GRUB_TERM_DUMB)
    {
      grub_putchar ('\n');
      grub_refresh ();
    }
  else
    (grub_cur_term->cls) ();
}

void
grub_setcolorstate (grub_term_color_state state)
{
  if (grub_cur_term->setcolorstate)
    (grub_cur_term->setcolorstate) (state);
}

void
grub_setcolor (grub_uint8_t normal_color, grub_uint8_t highlight_color)
{
  if (grub_cur_term->setcolor)
    (grub_cur_term->setcolor) (normal_color, highlight_color);
}

int
grub_setcursor (int on)
{
  int ret = cursor_state;

  if (grub_cur_term->setcursor)
    {
      (grub_cur_term->setcursor) (on);
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
  if (grub_cur_term->refresh)
    (grub_cur_term->refresh) ();
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
