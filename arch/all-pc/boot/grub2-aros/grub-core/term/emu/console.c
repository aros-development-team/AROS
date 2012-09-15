/*  console.c -- Ncurses console for GRUB.  */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2003,2005,2007,2008  Free Software Foundation, Inc.
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
#include <config-util.h>

/* For compatibility.  */
#ifndef A_NORMAL
# define A_NORMAL	0
#endif /* ! A_NORMAL */
#ifndef A_STANDOUT
# define A_STANDOUT	0
#endif /* ! A_STANDOUT */

#include <grub/emu/console.h>
#include <grub/term.h>
#include <grub/types.h>

#if defined(HAVE_NCURSES_CURSES_H)
# include <ncurses/curses.h>
#elif defined(HAVE_NCURSES_H)
# include <ncurses.h>
#elif defined(HAVE_CURSES_H)
# include <curses.h>
#else
#error What the hell?
#endif

static int grub_console_attr = A_NORMAL;

grub_uint8_t grub_console_cur_color = 7;

static const grub_uint8_t grub_console_standard_color = 0x7;

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
grub_ncurses_putchar (struct grub_term_output *term __attribute__ ((unused)),
		      const struct grub_unicode_glyph *c)
{
  addch (c->base | grub_console_attr);
}

static void
grub_ncurses_setcolorstate (struct grub_term_output *term,
			    grub_term_color_state state)
{
  switch (state)
    {
    case GRUB_TERM_COLOR_STANDARD:
      grub_console_cur_color = grub_console_standard_color;
      grub_console_attr = A_NORMAL;
      break;
    case GRUB_TERM_COLOR_NORMAL:
      grub_console_cur_color = term->normal_color;
      grub_console_attr = A_NORMAL;
      break;
    case GRUB_TERM_COLOR_HIGHLIGHT:
      grub_console_cur_color = term->highlight_color;
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

static int
grub_ncurses_getkey (struct grub_term_input *term __attribute__ ((unused)))
{
  int c;

  wtimeout (stdscr, 100);
  c = getch ();

  switch (c)
    {
    case ERR:
      return GRUB_TERM_NO_KEY;
    case KEY_LEFT:
      c = GRUB_TERM_KEY_LEFT;
      break;

    case KEY_RIGHT:
      c = GRUB_TERM_KEY_RIGHT;
      break;

    case KEY_UP:
      c = GRUB_TERM_KEY_UP;
      break;

    case KEY_DOWN:
      c = GRUB_TERM_KEY_DOWN;
      break;

    case KEY_IC:
      c = 24;
      break;

    case KEY_DC:
      c = GRUB_TERM_KEY_DC;
      break;

    case KEY_BACKSPACE:
      /* XXX: For some reason ncurses on xterm does not return
	 KEY_BACKSPACE.  */
    case 127:
      c = '\b';
      break;

    case KEY_HOME:
      c = GRUB_TERM_KEY_HOME;
      break;

    case KEY_END:
      c = GRUB_TERM_KEY_END;
      break;

    case KEY_NPAGE:
      c = GRUB_TERM_KEY_NPAGE;
      break;

    case KEY_PPAGE:
      c = GRUB_TERM_KEY_PPAGE;
      break;
    }

  return c;
}

static grub_uint16_t
grub_ncurses_getxy (struct grub_term_output *term __attribute__ ((unused)))
{
  int x;
  int y;

  getyx (stdscr, y, x);

  return (x << 8) | y;
}

static grub_uint16_t
grub_ncurses_getwh (struct grub_term_output *term __attribute__ ((unused)))
{
  int x;
  int y;

  getmaxyx (stdscr, y, x);

  return (x << 8) | y;
}

static void
grub_ncurses_gotoxy (struct grub_term_output *term __attribute__ ((unused)),
		     grub_uint8_t x, grub_uint8_t y)
{
  move (y, x);
}

static void
grub_ncurses_cls (struct grub_term_output *term __attribute__ ((unused)))
{
  clear ();
  refresh ();
}

static void
grub_ncurses_setcursor (struct grub_term_output *term __attribute__ ((unused)),
			int on)
{
  curs_set (on ? 1 : 0);
}

static void
grub_ncurses_refresh (struct grub_term_output *term __attribute__ ((unused)))
{
  refresh ();
}

static grub_err_t
grub_ncurses_init (struct grub_term_output *term __attribute__ ((unused)))
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
grub_ncurses_fini (struct grub_term_output *term __attribute__ ((unused)))
{
  endwin ();
  return 0;
}


static struct grub_term_input grub_ncurses_term_input =
  {
    .name = "console",
    .getkey = grub_ncurses_getkey,
  };

static struct grub_term_output grub_ncurses_term_output =
  {
    .name = "console",
    .init = grub_ncurses_init,
    .fini = grub_ncurses_fini,
    .putchar = grub_ncurses_putchar,
    .getxy = grub_ncurses_getxy,
    .getwh = grub_ncurses_getwh,
    .gotoxy = grub_ncurses_gotoxy,
    .cls = grub_ncurses_cls,
    .setcolorstate = grub_ncurses_setcolorstate,
    .setcursor = grub_ncurses_setcursor,
    .refresh = grub_ncurses_refresh,
    .flags = GRUB_TERM_CODE_TYPE_ASCII
  };

void
grub_console_init (void)
{
  grub_term_register_output ("console", &grub_ncurses_term_output);
  grub_term_register_input ("console", &grub_ncurses_term_input);
}

void
grub_console_fini (void)
{
  grub_ncurses_fini (&grub_ncurses_term_output);
}
