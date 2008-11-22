/*  console.c -- Ncurses console for GRUB.  */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2003,2005,2007  Free Software Foundation, Inc.
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

#include <config.h>

#if defined(HAVE_NCURSES_CURSES_H)
# include <ncurses/curses.h>
#elif defined(HAVE_NCURSES_H)
# include <ncurses.h>
#elif defined(HAVE_CURSES_H)
# include <curses.h>
#endif

/* For compatibility.  */
#ifndef A_NORMAL
# define A_NORMAL	0
#endif /* ! A_NORMAL */
#ifndef A_STANDOUT
# define A_STANDOUT	0
#endif /* ! A_STANDOUT */

#include <grub/machine/console.h>
#include <grub/term.h>
#include <grub/types.h>

static int grub_console_attr = A_NORMAL;

grub_uint8_t grub_console_cur_color = 7;

static grub_uint8_t grub_console_standard_color = 0x7;
static grub_uint8_t grub_console_normal_color = 0x7;
static grub_uint8_t grub_console_highlight_color = 0x70;

#define NUM_COLORS	8

static grub_uint8_t color_map[NUM_COLORS] =
{
  COLOR_BLACK,
  COLOR_BLUE,
  COLOR_GREEN,
  COLOR_CYAN,
  COLOR_RED,
  COLOR_MAGENTA,
  COLOR_YELLOW,
  COLOR_WHITE
};

static int use_color;

static void
grub_ncurses_putchar (grub_uint32_t c)
{
  /* Better than nothing.  */
  switch (c)
    {
    case GRUB_TERM_DISP_LEFT:
      c = '<';
      break;

    case GRUB_TERM_DISP_UP:
      c = '^';
      break;

    case GRUB_TERM_DISP_RIGHT:
      c = '>';
      break;

    case GRUB_TERM_DISP_DOWN:
      c = 'v';
      break;

    case GRUB_TERM_DISP_HLINE:
      c = '-';
      break;

    case GRUB_TERM_DISP_VLINE:
      c = '|';
      break;

    case GRUB_TERM_DISP_UL:
    case GRUB_TERM_DISP_UR:
    case GRUB_TERM_DISP_LL:
    case GRUB_TERM_DISP_LR:
      c = '+';
      break;

    default:
      /* ncurses does not support Unicode.  */
      if (c > 0x7f)
	c = '?';
      break;
    }
  
  addch (c | grub_console_attr);
}

static grub_ssize_t
grub_ncurses_getcharwidth (grub_uint32_t code __attribute__ ((unused)))
{
  return 1;
}

static void
grub_ncurses_setcolorstate (grub_term_color_state state)
{
  switch (state) 
    {
    case GRUB_TERM_COLOR_STANDARD:
      grub_console_cur_color = grub_console_standard_color;
      grub_console_attr = A_NORMAL;
      break;
    case GRUB_TERM_COLOR_NORMAL:
      grub_console_cur_color = grub_console_normal_color;
      grub_console_attr = A_NORMAL;
      break;
    case GRUB_TERM_COLOR_HIGHLIGHT:
      grub_console_cur_color = grub_console_highlight_color;
      grub_console_attr = A_STANDOUT;
      break;
    default:
      break;
    }

  if (use_color)
    {
      grub_uint8_t fg, bg;

      fg = (grub_console_cur_color & 7);
      bg = (grub_console_cur_color >> 4) & 7;

      grub_console_attr = (grub_console_cur_color & 8) ? A_BOLD : A_NORMAL;
      color_set ((bg << 3) + fg, 0);
    }
}

/* XXX: This function is never called.  */
static void
grub_ncurses_setcolor (grub_uint8_t normal_color, grub_uint8_t highlight_color)
{
  grub_console_normal_color = normal_color;
  grub_console_highlight_color = highlight_color;
}

static void
grub_ncurses_getcolor (grub_uint8_t *normal_color, grub_uint8_t *highlight_color)
{
  *normal_color = grub_console_normal_color;
  *highlight_color = grub_console_highlight_color;
}

static int saved_char = ERR;

static int
grub_ncurses_checkkey (void)
{
  int c;
  
  /* Check for SAVED_CHAR. This should not be true, because this
     means checkkey is called twice continuously.  */
  if (saved_char != ERR)
    return saved_char;
  
  wtimeout (stdscr, 100);
  c = getch ();
  /* If C is not ERR, then put it back in the input queue.  */
  if (c != ERR)
    {
      saved_char = c;
      return c;
    }

  return -1;
}

static int
grub_ncurses_getkey (void)
{
  int c;
  
  /* If checkkey has already got a character, then return it.  */
  if (saved_char != ERR)
    {
      c = saved_char;
      saved_char = ERR;
    }
  else
    {
      wtimeout (stdscr, -1);
      c = getch ();
    }

  switch (c)
    {
    case KEY_LEFT:
      c = 2;
      break;

    case KEY_RIGHT:
      c = 6;
      break;
      
    case KEY_UP:
      c = 16;
      break;

    case KEY_DOWN:
      c = 14;
      break;

    case KEY_IC:
      c = 24;
      break;

    case KEY_DC:
      c = 4;
      break;

    case KEY_BACKSPACE:
      /* XXX: For some reason ncurses on xterm does not return
	 KEY_BACKSPACE.  */
    case 127: 
      c = 8;
      break;

    case KEY_HOME:
      c = 1;
      break;

    case KEY_END:
      c = 5;
      break;

    case KEY_NPAGE:
      c = 3;
      break;

    case KEY_PPAGE:
      c = 7;
      break;
    }

  return c;
}

static grub_uint16_t
grub_ncurses_getxy (void)
{
  int x;
  int y;

  getyx (stdscr, y, x);

  return (x << 8) | y;
}

static grub_uint16_t
grub_ncurses_getwh (void)
{
  int x;
  int y;

  getmaxyx (stdscr, y, x);

  return (x << 8) | y;
}

static void
grub_ncurses_gotoxy (grub_uint8_t x, grub_uint8_t y)
{
  move (y, x);
}

static void
grub_ncurses_cls (void)
{
  clear ();
  refresh ();
}

static void
grub_ncurses_setcursor (int on)
{
  curs_set (on ? 1 : 0);
}

static void
grub_ncurses_refresh (void)
{
  refresh ();
}

static grub_err_t
grub_ncurses_init (void)
{
  initscr ();
  raw ();
  noecho ();
  scrollok (stdscr, TRUE);

  nonl ();
  intrflush (stdscr, FALSE);
  keypad (stdscr, TRUE);

  if (has_colors ())
    {
      start_color ();

      if ((COLORS >= NUM_COLORS) && (COLOR_PAIRS >= NUM_COLORS * NUM_COLORS))
        {
          int i, j, n;

          n = 0;
          for (i = 0; i < NUM_COLORS; i++)
            for (j = 0; j < NUM_COLORS; j++)
              init_pair(n++, color_map[j], color_map[i]);

          use_color = 1;
        }
    }

  return 0;
}

static grub_err_t
grub_ncurses_fini (void)
{
  endwin ();
  return 0;
}


static struct grub_term_input grub_ncurses_term_input =
  {
    .name = "console",
    .checkkey = grub_ncurses_checkkey,
    .getkey = grub_ncurses_getkey,
  };

static struct grub_term_output grub_ncurses_term_output =
  {
    .name = "console",
    .init = grub_ncurses_init,
    .fini = grub_ncurses_fini,
    .putchar = grub_ncurses_putchar,
    .getcharwidth = grub_ncurses_getcharwidth,
    .getxy = grub_ncurses_getxy,
    .getwh = grub_ncurses_getwh,
    .gotoxy = grub_ncurses_gotoxy,
    .cls = grub_ncurses_cls,
    .setcolorstate = grub_ncurses_setcolorstate,
    .setcolor = grub_ncurses_setcolor,
    .getcolor = grub_ncurses_getcolor,
    .setcursor = grub_ncurses_setcursor,
    .refresh = grub_ncurses_refresh,
    .flags = 0,
  };

void
grub_console_init (void)
{
  grub_term_register_output (&grub_ncurses_term_output);
  grub_term_register_input (&grub_ncurses_term_input);
  grub_term_set_current_output (&grub_ncurses_term_output);
  grub_term_set_current_input (&grub_ncurses_term_input);
}

void
grub_console_fini (void)
{
  grub_ncurses_fini ();
}
