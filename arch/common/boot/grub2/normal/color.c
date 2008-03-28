/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 1999,2000,2001,2002,2003,2004,2008  Free Software Foundation, Inc.
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

#include <grub/misc.h>
#include <grub/mm.h>
#include <grub/normal.h>
#include <grub/term.h>

/* Borrowed from GRUB Legacy */
static char *color_list[16] =
{
  "black",
  "blue",
  "green",
  "cyan",
  "red",
  "magenta",
  "brown",
  "light-gray",
  "dark-gray",
  "light-blue",
  "light-green",
  "light-cyan",
  "light-red",
  "light-magenta",
  "yellow",
  "white"
};

static int
parse_color_name (grub_uint8_t *ret, char *name)
{
  grub_uint8_t i;
  for (i = 0; i < sizeof (color_list) / sizeof (*color_list); i++)
    if (! grub_strcmp (name, color_list[i]))
      {
        *ret = i;
        return 0;
      }
  return -1;
}

void
grub_parse_color_name_pair (grub_uint8_t *ret, const char *name)
{
  grub_uint8_t fg, bg;
  char *fg_name, *bg_name;

  /* nothing specified by user */
  if (name == NULL)
    return;

  fg_name = grub_strdup (name);
  if (fg_name == NULL)
    {
      /* "out of memory" message was printed by grub_strdup() */
      grub_wait_after_message ();
      return;
    }

  bg_name = grub_strchr (fg_name, '/');
  if (bg_name == NULL)
    {
      grub_printf ("Warning: syntax error (missing slash) in `%s'\n", fg_name);
      grub_wait_after_message ();
      goto free_and_return;
    }

  *(bg_name++) = '\0';

  if (parse_color_name (&fg, fg_name) == -1)
    {
      grub_printf ("Warning: invalid foreground color `%s'\n", fg_name);
      grub_wait_after_message ();
      goto free_and_return;
    }
  if (parse_color_name (&bg, bg_name) == -1)
    {
      grub_printf ("Warning: invalid background color `%s'\n", bg_name);
      grub_wait_after_message ();
      goto free_and_return;
    }

  *ret = (bg << 4) | fg;

free_and_return:
  grub_free (fg_name);
}

/* Replace default `normal' colors with the ones specified by user (if any).  */
char *
grub_env_write_color_normal (struct grub_env_var *var __attribute__ ((unused)),
			     const char *val)
{
  grub_uint8_t color_normal, color_highlight;

  /* Use old settings in case grub_parse_color_name_pair() has no effect.  */
  grub_getcolor (&color_normal, &color_highlight);

  grub_parse_color_name_pair (&color_normal, val);

  /* Reloads terminal `normal' and `highlight' colors.  */
  grub_setcolor (color_normal, color_highlight);

  /* Propagates `normal' color to terminal current color.  */
  grub_setcolorstate (GRUB_TERM_COLOR_NORMAL);

  return grub_strdup (val);
}

/* Replace default `highlight' colors with the ones specified by user (if any).  */
char *
grub_env_write_color_highlight (struct grub_env_var *var __attribute__ ((unused)),
				const char *val)
{
  grub_uint8_t color_normal, color_highlight;

  /* Use old settings in case grub_parse_color_name_pair() has no effect.  */
  grub_getcolor (&color_normal, &color_highlight);

  grub_parse_color_name_pair (&color_highlight, val);

  /* Reloads terminal `normal' and `highlight' colors.  */
  grub_setcolor (color_normal, color_highlight);

  /* Propagates `normal' color to terminal current color.
     Note: Using GRUB_TERM_COLOR_NORMAL here rather than
     GRUB_TERM_COLOR_HIGHLIGHT is intentional.  We don't want to switch
     to highlight state just because color was reloaded.  */
  grub_setcolorstate (GRUB_TERM_COLOR_NORMAL);

  return grub_strdup (val);
}
