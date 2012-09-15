/* menu_text.c - Basic text menu implementation.  */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2003,2004,2005,2006,2007,2008,2009  Free Software Foundation, Inc.
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

#include <grub/normal.h>
#include <grub/term.h>
#include <grub/misc.h>
#include <grub/loader.h>
#include <grub/mm.h>
#include <grub/time.h>
#include <grub/env.h>
#include <grub/menu_viewer.h>
#include <grub/i18n.h>
#include <grub/charset.h>

static grub_uint8_t grub_color_menu_normal;
static grub_uint8_t grub_color_menu_highlight;

struct menu_viewer_data
{
  int first, offset;
  /* The number of entries shown at a time.  */
  int num_entries;
  grub_menu_t menu;
  struct grub_term_output *term;
};

static inline int
grub_term_cursor_x (struct grub_term_output *term)
{
  return (GRUB_TERM_LEFT_BORDER_X + grub_term_border_width (term) 
	  - GRUB_TERM_MARGIN - 1);
}

grub_ssize_t
grub_getstringwidth (grub_uint32_t * str, const grub_uint32_t * last_position,
		     struct grub_term_output *term)
{
  grub_ssize_t width = 0;

  while (str < last_position)
    {
      struct grub_unicode_glyph glyph;
      str += grub_unicode_aglomerate_comb (str, last_position - str, &glyph);
      width += grub_term_getcharwidth (term, &glyph);
    }
  return width;
}

static int
grub_print_message_indented_real (const char *msg, int margin_left,
				  int margin_right,
				  struct grub_term_output *term, int dry_run)
{
  grub_uint32_t *unicode_msg;
  grub_uint32_t *last_position;
  grub_size_t msg_len = grub_strlen (msg) + 2;
  int ret = 0;

  unicode_msg = grub_malloc (msg_len * sizeof (grub_uint32_t));
 
  if (!unicode_msg)
    return 0;

  msg_len = grub_utf8_to_ucs4 (unicode_msg, msg_len,
			       (grub_uint8_t *) msg, -1, 0);
  
  last_position = unicode_msg + msg_len;
  *last_position++ = '\n';
  *last_position = 0;

  if (dry_run)
    ret = grub_ucs4_count_lines (unicode_msg, last_position, margin_left,
				 margin_right, term);
  else
    grub_print_ucs4 (unicode_msg, last_position, margin_left,
		     margin_right, term);

  grub_free (unicode_msg);

  return ret;
}

void
grub_print_message_indented (const char *msg, int margin_left, int margin_right,
			     struct grub_term_output *term)
{
  grub_print_message_indented_real (msg, margin_left, margin_right, term, 0);
}

static void
draw_border (struct grub_term_output *term, int num_entries)
{
  unsigned i;

  grub_term_setcolorstate (term, GRUB_TERM_COLOR_NORMAL);

  grub_term_gotoxy (term, GRUB_TERM_MARGIN, GRUB_TERM_TOP_BORDER_Y);
  grub_putcode (GRUB_UNICODE_CORNER_UL, term);
  for (i = 0; i < (unsigned) grub_term_border_width (term) - 2; i++)
    grub_putcode (GRUB_UNICODE_HLINE, term);
  grub_putcode (GRUB_UNICODE_CORNER_UR, term);

  for (i = 0; i < (unsigned) num_entries; i++)
    {
      grub_term_gotoxy (term, GRUB_TERM_MARGIN, GRUB_TERM_TOP_BORDER_Y + i + 1);
      grub_putcode (GRUB_UNICODE_VLINE, term);
      grub_term_gotoxy (term, GRUB_TERM_MARGIN + grub_term_border_width (term)
			- 1,
			GRUB_TERM_TOP_BORDER_Y + i + 1);
      grub_putcode (GRUB_UNICODE_VLINE, term);
    }

  grub_term_gotoxy (term, GRUB_TERM_MARGIN,
		    GRUB_TERM_TOP_BORDER_Y + num_entries + 1);
  grub_putcode (GRUB_UNICODE_CORNER_LL, term);
  for (i = 0; i < (unsigned) grub_term_border_width (term) - 2; i++)
    grub_putcode (GRUB_UNICODE_HLINE, term);
  grub_putcode (GRUB_UNICODE_CORNER_LR, term);

  grub_term_setcolorstate (term, GRUB_TERM_COLOR_NORMAL);

  grub_term_gotoxy (term, GRUB_TERM_MARGIN,
		    (GRUB_TERM_TOP_BORDER_Y + num_entries
		     + GRUB_TERM_MARGIN + 1));
}

static int
print_message (int nested, int edit, struct grub_term_output *term, int dry_run)
{
  int ret = 0;
  grub_term_setcolorstate (term, GRUB_TERM_COLOR_NORMAL);

  if (edit)
    {
      if(dry_run)
	ret++;
      else
	grub_putcode ('\n', term);
      ret += grub_print_message_indented_real (_("Minimum Emacs-like screen editing is \
supported. TAB lists completions. Press Ctrl-x or F10 to boot, Ctrl-c or F2 for a \
command-line or ESC to discard edits and return to the GRUB menu."),
					       STANDARD_MARGIN, STANDARD_MARGIN,
					       term, dry_run);
    }
  else
    {
      const char *msg = _("Use the %C and %C keys to select which "
			  "entry is highlighted.");
      char *msg_translated;

      msg_translated = grub_xasprintf (msg, GRUB_UNICODE_UPARROW,
				       GRUB_UNICODE_DOWNARROW);
      if (!msg_translated)
	return 0;
      if(dry_run)
	ret++;
      else
	grub_putcode ('\n', term);
      ret += grub_print_message_indented_real (msg_translated, STANDARD_MARGIN,
					       STANDARD_MARGIN, term, dry_run);

      grub_free (msg_translated);

      if (nested)
	{
	  ret += grub_print_message_indented_real
	    (_("Press enter to boot the selected OS, "
	       "`e' to edit the commands before booting "
	       "or `c' for a command-line. ESC to return previous menu."),
	     STANDARD_MARGIN, STANDARD_MARGIN, term, dry_run);
	}
      else
	{
	  ret += grub_print_message_indented_real
	    (_("Press enter to boot the selected OS, "
	       "`e' to edit the commands before booting "
	       "or `c' for a command-line."),
	     STANDARD_MARGIN, STANDARD_MARGIN, term, dry_run);
	}	
    }
  return ret;
}

static void
print_entry (int y, int highlight, grub_menu_entry_t entry,
	     struct grub_term_output *term)
{
  int x;
  const char *title;
  grub_size_t title_len;
  grub_ssize_t len;
  grub_uint32_t *unicode_title;
  grub_ssize_t i;
  grub_uint8_t old_color_normal, old_color_highlight;

  title = entry ? entry->title : "";
  title_len = grub_strlen (title);
  unicode_title = grub_malloc (title_len * sizeof (*unicode_title));
  if (! unicode_title)
    /* XXX How to show this error?  */
    return;

  len = grub_utf8_to_ucs4 (unicode_title, title_len,
                           (grub_uint8_t *) title, -1, 0);
  if (len < 0)
    {
      /* It is an invalid sequence.  */
      grub_free (unicode_title);
      return;
    }

  grub_term_getcolor (term, &old_color_normal, &old_color_highlight);
  grub_term_setcolor (term, grub_color_menu_normal, grub_color_menu_highlight);
  grub_term_setcolorstate (term, highlight
			   ? GRUB_TERM_COLOR_HIGHLIGHT
			   : GRUB_TERM_COLOR_NORMAL);

  grub_term_gotoxy (term, GRUB_TERM_LEFT_BORDER_X + GRUB_TERM_MARGIN, y);

  int last_printed = 0;

  for (i = 0; i < len; i++)
    if (unicode_title[i] == '\n' || unicode_title[i] == '\b'
	|| unicode_title[i] == '\r' || unicode_title[i] == '\e')
      unicode_title[i] = ' ';

  for (x = GRUB_TERM_LEFT_BORDER_X + GRUB_TERM_MARGIN + 1, i = 0;
       x < (int) (GRUB_TERM_LEFT_BORDER_X + grub_term_border_width (term)
		  - GRUB_TERM_MARGIN);)
    {
      if (i < len
	  && x <= (int) (GRUB_TERM_LEFT_BORDER_X + grub_term_border_width (term)
			 - GRUB_TERM_MARGIN - 1))
	{
	  grub_ssize_t width;
	  struct grub_unicode_glyph glyph;

	  i += grub_unicode_aglomerate_comb (unicode_title + i,
					     len - i, &glyph);

	  width = grub_term_getcharwidth (term, &glyph);
	  grub_free (glyph.combining);

	  if (x + width <= (int) (GRUB_TERM_LEFT_BORDER_X 
				 + grub_term_border_width (term)
				 - GRUB_TERM_MARGIN - 1))
	    last_printed = i;
	  x += width;
	}
      else
	break;
    }

  grub_print_ucs4 (unicode_title,
		   unicode_title + last_printed, 0, 0, term);

  if (last_printed != len)
    {
      grub_putcode (GRUB_UNICODE_RIGHTARROW, term);
      struct grub_unicode_glyph pseudo_glyph = {
	.base = GRUB_UNICODE_RIGHTARROW,
	.variant = 0,
	.attributes = 0,
	.ncomb = 0,
	.combining = 0,
	.estimated_width = 1
      };
      x += grub_term_getcharwidth (term, &pseudo_glyph);
    }

  for (; x < (int) (GRUB_TERM_LEFT_BORDER_X + grub_term_border_width (term)
		    - GRUB_TERM_MARGIN); x++)
    grub_putcode (' ', term);

  grub_term_setcolorstate (term, GRUB_TERM_COLOR_NORMAL);
  grub_putcode (' ', term);

  grub_term_gotoxy (term, grub_term_cursor_x (term), y);

  grub_term_setcolor (term, old_color_normal, old_color_highlight);
  grub_term_setcolorstate (term, GRUB_TERM_COLOR_NORMAL);
  grub_free (unicode_title);
}

static void
print_entries (grub_menu_t menu, const struct menu_viewer_data *data)
{
  grub_menu_entry_t e;
  int i;

  grub_term_gotoxy (data->term,
		    GRUB_TERM_LEFT_BORDER_X + grub_term_border_width (data->term),
		    GRUB_TERM_FIRST_ENTRY_Y);

  if (data->first)
    grub_putcode (GRUB_UNICODE_UPARROW, data->term);
  else
    grub_putcode (' ', data->term);

  e = grub_menu_get_entry (menu, data->first);

  for (i = 0; i < data->num_entries; i++)
    {
      print_entry (GRUB_TERM_FIRST_ENTRY_Y + i, data->offset == i,
		   e, data->term);
      if (e)
	e = e->next;
    }

  grub_term_gotoxy (data->term, GRUB_TERM_LEFT_BORDER_X
		    + grub_term_border_width (data->term),
		    GRUB_TERM_TOP_BORDER_Y + data->num_entries);

  if (e)
    grub_putcode (GRUB_UNICODE_DOWNARROW, data->term);
  else
    grub_putcode (' ', data->term);

  grub_term_gotoxy (data->term, grub_term_cursor_x (data->term),
		    GRUB_TERM_FIRST_ENTRY_Y + data->offset);
}

/* Initialize the screen.  If NESTED is non-zero, assume that this menu
   is run from another menu or a command-line. If EDIT is non-zero, show
   a message for the menu entry editor.  */
void
grub_menu_init_page (int nested, int edit, int *num_entries,
		     struct grub_term_output *term)
{
  grub_uint8_t old_color_normal, old_color_highlight;

  /* 3 lines for timeout message and bottom margin.  2 lines for the border.  */
  *num_entries = grub_term_height (term) - GRUB_TERM_TOP_BORDER_Y
    - (print_message (nested, edit, term, 1) + 3) - 2;

  grub_term_getcolor (term, &old_color_normal, &old_color_highlight);

  /* By default, use the same colors for the menu.  */
  grub_color_menu_normal = old_color_normal;
  grub_color_menu_highlight = old_color_highlight;

  /* Then give user a chance to replace them.  */
  grub_parse_color_name_pair (&grub_color_menu_normal,
			      grub_env_get ("menu_color_normal"));
  grub_parse_color_name_pair (&grub_color_menu_highlight,
			      grub_env_get ("menu_color_highlight"));

  grub_normal_init_page (term);
  grub_term_setcolor (term, grub_color_menu_normal, grub_color_menu_highlight);
  draw_border (term, *num_entries);
  grub_term_setcolor (term, old_color_normal, old_color_highlight);
  print_message (nested, edit, term, 0);
}

static void
menu_text_print_timeout (int timeout, void *dataptr)
{
  const char *msg =
    _("The highlighted entry will be executed automatically in %ds.");
  struct menu_viewer_data *data = dataptr;
  char *msg_translated;
  int posx;

  grub_term_gotoxy (data->term, 0, grub_term_height (data->term) - 3);

  msg_translated = grub_xasprintf (msg, timeout);
  if (!msg_translated)
    {
      grub_print_error ();
      grub_errno = GRUB_ERR_NONE;
      return;
    }

  grub_print_message_indented (msg_translated, 3, 0, data->term);
 
  posx = grub_term_getxy (data->term) >> 8;
  grub_print_spaces (data->term, grub_term_width (data->term) - posx - 1);

  grub_term_gotoxy (data->term,
		    grub_term_cursor_x (data->term),
		    GRUB_TERM_FIRST_ENTRY_Y + data->offset);
  grub_term_refresh (data->term);
}

static void
menu_text_set_chosen_entry (int entry, void *dataptr)
{
  struct menu_viewer_data *data = dataptr;
  int oldoffset = data->offset;
  int complete_redraw = 0;

  data->offset = entry - data->first;
  if (data->offset > data->num_entries - 1)
    {
      data->first = entry - (data->num_entries - 1);
      data->offset = data->num_entries - 1;
      complete_redraw = 1;
    }
  if (data->offset < 0)
    {
      data->offset = 0;
      data->first = entry;
      complete_redraw = 1;
    }
  if (complete_redraw)
    print_entries (data->menu, data);
  else
    {
      print_entry (GRUB_TERM_FIRST_ENTRY_Y + oldoffset, 0,
		   grub_menu_get_entry (data->menu, data->first + oldoffset),
		   data->term);
      print_entry (GRUB_TERM_FIRST_ENTRY_Y + data->offset, 1,
		   grub_menu_get_entry (data->menu, data->first + data->offset),
		   data->term);
    }
  grub_term_refresh (data->term);
}

static void
menu_text_fini (void *dataptr)
{
  struct menu_viewer_data *data = dataptr;

  grub_term_setcursor (data->term, 1);
  grub_term_cls (data->term);

}

static void
menu_text_clear_timeout (void *dataptr)
{
  struct menu_viewer_data *data = dataptr;

  grub_term_gotoxy (data->term, 0, grub_term_height (data->term) - 3);
  grub_print_spaces (data->term, grub_term_width (data->term) - 1);
  grub_term_gotoxy (data->term, grub_term_cursor_x (data->term),
		    GRUB_TERM_FIRST_ENTRY_Y + data->offset);
  grub_term_refresh (data->term);
}

grub_err_t 
grub_menu_try_text (struct grub_term_output *term, 
		    int entry, grub_menu_t menu, int nested)
{
  struct menu_viewer_data *data;
  struct grub_menu_viewer *instance;

  instance = grub_zalloc (sizeof (*instance));
  if (!instance)
    return grub_errno;

  data = grub_zalloc (sizeof (*data));
  if (!data)
    {
      grub_free (instance);
      return grub_errno;
    }

  data->term = term;
  instance->data = data;
  instance->set_chosen_entry = menu_text_set_chosen_entry;
  instance->print_timeout = menu_text_print_timeout;
  instance->clear_timeout = menu_text_clear_timeout;
  instance->fini = menu_text_fini;

  data->menu = menu;

  data->offset = entry;
  data->first = 0;

  grub_term_setcursor (data->term, 0);
  grub_menu_init_page (nested, 0, &data->num_entries, data->term);

  if (data->offset > data->num_entries - 1)
    {
      data->first = data->offset - (data->num_entries - 1);
      data->offset = data->num_entries - 1;
    }

  print_entries (menu, data);
  grub_term_refresh (data->term);
  grub_menu_register_viewer (instance);

  return GRUB_ERR_NONE;
}
