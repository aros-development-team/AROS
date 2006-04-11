/*  ofconsole.c -- Open Firmware console for GRUB.  */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2003, 2004, 2005 Free Software Foundation, Inc.
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

#include <grub/term.h>
#include <grub/types.h>
#include <grub/misc.h>
#include <grub/mm.h>
#include <grub/machine/console.h>
#include <grub/ieee1275/ieee1275.h>

static grub_ieee1275_ihandle_t stdout_ihandle;
static grub_ieee1275_ihandle_t stdin_ihandle;

static int grub_curr_x;
static int grub_curr_y;

static int grub_keybuf;
static int grub_buflen;

struct color
{
  int red;
  int green;
  int blue;
};

#define	MAX 0xff
static struct color colors[8] =
  {
    { 0,   0,   0},
    { MAX, 0,   0},
    { 0,   MAX, 0},
    { MAX, MAX, 0},
    { 0,   0,   MAX},
    { MAX, 0,   MAX},
    { 0,   MAX, MAX},
    { MAX, MAX, MAX}
  };

static int fgcolor = 7;
static int bgcolor = 0;

/* Write control characters to the console.  */
static void
grub_ofconsole_writeesc (const char *str)
{
  while (*str)
    {
      char chr = *(str++);
      grub_ieee1275_write (stdout_ihandle, &chr, 1, 0);
    }
  
}

static void
grub_ofconsole_putchar (grub_uint32_t c)
{
  char chr = c;
  if (c == '\n')
    {
      grub_curr_y++;
      grub_curr_x = 0;
    }
  else
    grub_curr_x++;
  grub_ieee1275_write (stdout_ihandle, &chr, 1, 0);
}

static grub_ssize_t
grub_ofconsole_getcharwidth (grub_uint32_t c __attribute__((unused)))
{
  return 1;
}

static void
grub_ofconsole_setcolorstate (grub_term_color_state state)
{
  char setcol[20];
  int fg;
  int bg;

  switch (state)
    {
    case GRUB_TERM_COLOR_STANDARD:
    case GRUB_TERM_COLOR_NORMAL:
      fg = fgcolor;
      bg = bgcolor;
      break;
    case GRUB_TERM_COLOR_HIGHLIGHT:
      fg = bgcolor;
      bg = fgcolor;
      break;
    default:
      return;
    }

  grub_sprintf (setcol, "\e[3%dm\e[4%dm", fg, bg);
  grub_ofconsole_writeesc (setcol);
}

static void
grub_ofconsole_setcolor (grub_uint8_t normal_color,
			 grub_uint8_t highlight_color)
{
  fgcolor = normal_color;
  bgcolor = highlight_color;
}

static int
grub_ofconsole_readkey (int *key)
{
  char c;
  grub_ssize_t actual = 0;

  grub_ieee1275_read (stdin_ihandle, &c, 1, &actual);

  if (actual > 0 && c == '\e')
    {
      grub_ieee1275_read (stdin_ihandle, &c, 1, &actual);
      if (actual <= 0)
	{
	  *key = '\e';
	  return 1;
	}
      
      if (c != 91)
	return 0;
      
      grub_ieee1275_read (stdin_ihandle, &c, 1, &actual);
      if (actual <= 0)
	return 0;
      
      switch (c)
	{
	case 65:
	  /* Up: Ctrl-p.  */
	  c = 16; 
	  break;
	case 66:
	  /* Down: Ctrl-n.  */
	  c = 14;
	  break;
	case 67:
	  /* Right: Ctrl-f.  */
	  c = 6;
	  break;
	case 68:
	  /* Left: Ctrl-b.  */
	  c = 2;
	  break;
	}
    }
  
  *key = c;
  return actual > 0;
}

static int
grub_ofconsole_checkkey (void)
{
  int key;
  int read;
  
  if (grub_buflen)
    return 1;

  read = grub_ofconsole_readkey (&key);
  if (read)
    {
      grub_keybuf = key;
      grub_buflen = 1;
      return 1;
    }
    
  return 0;
}

static int
grub_ofconsole_getkey (void)
{
  int key;

  if (grub_buflen)
    {
      grub_buflen  =0;
      return grub_keybuf;
    }

  while (! grub_ofconsole_readkey (&key));
  
  return key;
}

static grub_uint16_t
grub_ofconsole_getxy (void)
{
  return ((grub_curr_x - 1) << 8) | grub_curr_y;
}

static grub_uint16_t
grub_ofconsole_getwh (void)
{
  grub_ieee1275_ihandle_t options;
  char *val;
  grub_ssize_t lval;
  static grub_uint8_t w, h;

  /* Once we have them, don't ask them again.  */
  if (!w || !h)
    {
      if (! grub_ieee1275_finddevice ("/options", &options)
	  && options != (grub_ieee1275_ihandle_t) -1)
        {
          if (! grub_ieee1275_get_property_length (options, "screen-#columns",
                                                   &lval) && lval != -1)
            {
	      val = grub_malloc (lval);
	      if (val)
                {
                  if (! grub_ieee1275_get_property (options, "screen-#columns",
                                                    val, lval, 0))
                    w = (grub_uint8_t) grub_strtoul (val, 0, 10);

                  grub_free (val);
                }
            }
          if (! grub_ieee1275_get_property_length (options, "screen-#rows",
                                                   &lval) && lval != -1)
            {
	      val = grub_malloc (lval);
	      if (val)
                {
                  if (! grub_ieee1275_get_property (options, "screen-#rows",
                                                    val, lval, 0))
                    h = (grub_uint8_t) grub_strtoul (val, 0, 10);

                  grub_free (val);
                }
            }
	}
    }

  /* Use a small console by default.  */
  if (! w)
    w = 80;
  if (! h)
    h = 24;

  return (w << 8) | h;
}

static void
grub_ofconsole_gotoxy (grub_uint8_t x, grub_uint8_t y)
{
  char s[11]; /* 5 + 3 + 3.  */
  grub_curr_x = x;
  grub_curr_y = y;

  grub_sprintf (s, "\e[%d;%dH", y + 1, x + 1);
  grub_ofconsole_writeesc (s);
}

static void
grub_ofconsole_cls (void)
{
  /* Clear the screen.  */
  grub_ofconsole_writeesc ("\e[2J");
  grub_gotoxy (0, 0);
}

static void
grub_ofconsole_setcursor (int on __attribute ((unused)))
{
  /* XXX: Not supported.  */
}

static void
grub_ofconsole_refresh (void)
{
  /* Do nothing, the current console state is ok.  */
}

static grub_err_t
grub_ofconsole_init (void)
{
  unsigned char data[4];
  grub_ssize_t actual;
  int col;

  if (grub_ieee1275_get_property (grub_ieee1275_chosen, "stdout", data,
				  sizeof data, &actual)
      || actual != sizeof data)
    return grub_error (GRUB_ERR_UNKNOWN_DEVICE, "Cannot find stdout");

  stdout_ihandle = grub_ieee1275_decode_int_4 (data);
  
  if (grub_ieee1275_get_property (grub_ieee1275_chosen, "stdin", data,
				  sizeof data, &actual)
      || actual != sizeof data)
    return grub_error (GRUB_ERR_UNKNOWN_DEVICE, "Cannot find stdin");

  stdin_ihandle = grub_ieee1275_decode_int_4 (data);

  /* Initialize colors.  */
  for (col = 0; col < 7; col++)
    grub_ieee1275_set_color (stdout_ihandle, col, colors[col].red,
			     colors[col].green, colors[col].blue);

  /* Set the right fg and bg colors.  */
  grub_ofconsole_setcolorstate (GRUB_TERM_COLOR_NORMAL);

  return 0;
}

static grub_err_t
grub_ofconsole_fini (void)
{
  return 0;
}



static struct grub_term grub_ofconsole_term =
  {
    .name = "ofconsole",
    .init = grub_ofconsole_init,
    .fini = grub_ofconsole_fini,
    .putchar = grub_ofconsole_putchar,
    .getcharwidth = grub_ofconsole_getcharwidth,
    .checkkey = grub_ofconsole_checkkey,
    .getkey = grub_ofconsole_getkey,
    .getxy = grub_ofconsole_getxy,
    .getwh = grub_ofconsole_getwh,
    .gotoxy = grub_ofconsole_gotoxy,
    .cls = grub_ofconsole_cls,
    .setcolorstate = grub_ofconsole_setcolorstate,
    .setcolor = grub_ofconsole_setcolor,
    .setcursor = grub_ofconsole_setcursor,
    .refresh = grub_ofconsole_refresh,
    .flags = 0,
    .next = 0
  };

void
grub_console_init (void)
{
  grub_term_register (&grub_ofconsole_term);
  grub_term_set_current (&grub_ofconsole_term);
}

void
grub_console_fini (void)
{
  grub_term_unregister (&grub_ofconsole_term);
}
