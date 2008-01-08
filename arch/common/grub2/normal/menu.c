/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2003,2004,2005,2006,2007,2008  Free Software Foundation, Inc.
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
#include <grub/machine/time.h>
#include <grub/env.h>
#include <grub/script.h>

static grub_uint8_t grub_color_menu_normal;
static grub_uint8_t grub_color_menu_highlight;

/* Wait until the user pushes any key so that the user
   can see what happened.  */
void
grub_wait_after_message (void)
{
  grub_printf ("\nPress any key to continue...");
  (void) grub_getkey ();
}

static void
draw_border (void)
{
  unsigned i;
  
  grub_setcolorstate (GRUB_TERM_COLOR_NORMAL);
  
  grub_gotoxy (GRUB_TERM_MARGIN, GRUB_TERM_TOP_BORDER_Y);
  grub_putcode (GRUB_TERM_DISP_UL);
  for (i = 0; i < (unsigned) GRUB_TERM_BORDER_WIDTH - 2; i++)
    grub_putcode (GRUB_TERM_DISP_HLINE);
  grub_putcode (GRUB_TERM_DISP_UR);

  for (i = 0; i < (unsigned) GRUB_TERM_NUM_ENTRIES; i++)
    {
      grub_gotoxy (GRUB_TERM_MARGIN, GRUB_TERM_TOP_BORDER_Y + i + 1);
      grub_putcode (GRUB_TERM_DISP_VLINE);
      grub_gotoxy (GRUB_TERM_MARGIN + GRUB_TERM_BORDER_WIDTH - 1,
		   GRUB_TERM_TOP_BORDER_Y + i + 1);
      grub_putcode (GRUB_TERM_DISP_VLINE);
    }

  grub_gotoxy (GRUB_TERM_MARGIN,
	       GRUB_TERM_TOP_BORDER_Y + GRUB_TERM_NUM_ENTRIES + 1);
  grub_putcode (GRUB_TERM_DISP_LL);
  for (i = 0; i < (unsigned) GRUB_TERM_BORDER_WIDTH - 2; i++)
    grub_putcode (GRUB_TERM_DISP_HLINE);
  grub_putcode (GRUB_TERM_DISP_LR);

  grub_setcolorstate (GRUB_TERM_COLOR_NORMAL);

  grub_gotoxy (GRUB_TERM_MARGIN,
	       (GRUB_TERM_TOP_BORDER_Y + GRUB_TERM_NUM_ENTRIES
		+ GRUB_TERM_MARGIN + 1));
}

static void
print_message (int nested, int edit)
{
  grub_setcolorstate (GRUB_TERM_COLOR_NORMAL);

  if (edit)
    {
      grub_printf ("\n\
      Minimum Emacs-like screen editing is supported. TAB lists\n\
      available completions. Press C-x (\'x\' with Ctrl) to boot,\n\
      C-c (\'c\' with Ctrl) for a command-line or ESC to return menu.");
    }
  else
    {
      grub_printf ("\n\
      Use the %C and %C keys to select which entry is highlighted.\n",
		   (grub_uint32_t) GRUB_TERM_DISP_UP, (grub_uint32_t) GRUB_TERM_DISP_DOWN);
      grub_printf ("\
      Press enter to boot the selected OS, \'e\' to edit the\n\
      commands before booting or \'c\' for a command-line.");
      if (nested)
	grub_printf ("\n\
      ESC to return previous menu.");
    }
  
}

static grub_menu_entry_t
get_entry (grub_menu_t menu, int no)
{
  grub_menu_entry_t e;

  for (e = menu->entry_list; e && no > 0; e = e->next, no--)
    ;

  return e;
}

static void
print_entry (int y, int highlight, grub_menu_entry_t entry)
{
  int x;
  const char *title;
  grub_ssize_t len;
  grub_uint32_t *unicode_title;
  grub_ssize_t i;
  grub_uint8_t old_color_normal, old_color_highlight;

  title = entry ? entry->title : "";
  unicode_title = grub_malloc (grub_strlen (title) * sizeof (*unicode_title));
  if (! unicode_title)
    /* XXX How to show this error?  */
    return;
  
  len = grub_utf8_to_ucs4 (unicode_title, (grub_uint8_t *) title,
			   grub_strlen (title));
  if (len < 0)
    {
      /* It is an invalid sequence.  */
      grub_free (unicode_title);
      return;
    }

  grub_getcolor (&old_color_normal, &old_color_highlight);
  grub_setcolor (grub_color_menu_normal, grub_color_menu_highlight);
  grub_setcolorstate (highlight
		      ? GRUB_TERM_COLOR_HIGHLIGHT
		      : GRUB_TERM_COLOR_NORMAL);

  grub_gotoxy (GRUB_TERM_LEFT_BORDER_X + GRUB_TERM_MARGIN, y);

  for (x = GRUB_TERM_LEFT_BORDER_X + GRUB_TERM_MARGIN + 1, i = 0;
       x < GRUB_TERM_LEFT_BORDER_X + GRUB_TERM_BORDER_WIDTH - GRUB_TERM_MARGIN;
       i++)
    {
      if (i < len
	  && x <= (GRUB_TERM_LEFT_BORDER_X + GRUB_TERM_BORDER_WIDTH
		   - GRUB_TERM_MARGIN - 1))
	{
	  grub_ssize_t width;

	  width = grub_getcharwidth (unicode_title[i]);
	  
	  if (x + width > (GRUB_TERM_LEFT_BORDER_X + GRUB_TERM_BORDER_WIDTH
			   - GRUB_TERM_MARGIN - 1))
	    grub_putcode (GRUB_TERM_DISP_RIGHT);
	  else
	    grub_putcode (unicode_title[i]);

	  x += width;
	}
      else
	{
	  grub_putchar (' ');
	  x++;
	}
    }
  grub_setcolorstate (GRUB_TERM_COLOR_NORMAL);
  grub_putchar (' ');

  grub_gotoxy (GRUB_TERM_CURSOR_X, y);

  grub_setcolor (old_color_normal, old_color_highlight);
  grub_setcolorstate (GRUB_TERM_COLOR_NORMAL);
  grub_free (unicode_title);
}

static void
print_entries (grub_menu_t menu, int first, int offset)
{
  grub_menu_entry_t e;
  int i;
  
  grub_gotoxy (GRUB_TERM_LEFT_BORDER_X + GRUB_TERM_BORDER_WIDTH,
	       GRUB_TERM_FIRST_ENTRY_Y);

  if (first)
    grub_putcode (GRUB_TERM_DISP_UP);
  else
    grub_putchar (' ');

  e = get_entry (menu, first);

  for (i = 0; i < GRUB_TERM_NUM_ENTRIES; i++)
    {
      print_entry (GRUB_TERM_FIRST_ENTRY_Y + i, offset == i, e);
      if (e)
	e = e->next;
    }

  grub_gotoxy (GRUB_TERM_LEFT_BORDER_X + GRUB_TERM_BORDER_WIDTH,
	       GRUB_TERM_TOP_BORDER_Y + GRUB_TERM_NUM_ENTRIES);

  if (e)
    grub_putcode (GRUB_TERM_DISP_DOWN);
  else
    grub_putchar (' ');

  grub_gotoxy (GRUB_TERM_CURSOR_X, GRUB_TERM_FIRST_ENTRY_Y + offset);
}

/* Initialize the screen.  If NESTED is non-zero, assume that this menu
   is run from another menu or a command-line. If EDIT is non-zero, show
   a message for the menu entry editor.  */
void
grub_menu_init_page (int nested, int edit)
{
  grub_uint8_t old_color_normal, old_color_highlight;

  grub_getcolor (&old_color_normal, &old_color_highlight);

  /* By default, use the same colors for the menu.  */
  grub_color_menu_normal = old_color_normal;
  grub_color_menu_highlight = old_color_highlight;

  /* Then give user a chance to replace them.  */
  grub_parse_color_name_pair (&grub_color_menu_normal, grub_env_get ("menu_color_normal"));
  grub_parse_color_name_pair (&grub_color_menu_highlight, grub_env_get ("menu_color_highlight"));

  grub_normal_init_page ();
  grub_setcolor (grub_color_menu_normal, grub_color_menu_highlight);
  draw_border ();
  grub_setcolor (old_color_normal, old_color_highlight);
  print_message (nested, edit);
}

/* Return the current timeout. If the variable "timeout" is not set or
   invalid, return -1.  */
static int
get_timeout (void)
{
  char *val;
  int timeout;
  
  val = grub_env_get ("timeout");
  if (! val)
    return -1;
  
  grub_error_push ();
  
  timeout = (int) grub_strtoul (val, 0, 0);

  /* If the value is invalid, unset the variable.  */
  if (grub_errno != GRUB_ERR_NONE)
    {
      grub_env_unset ("timeout");
      grub_errno = GRUB_ERR_NONE;
      timeout = -1;
    }

  grub_error_pop ();
  
  return timeout;
}

/* Set current timeout in the variable "timeout".  */
static void
set_timeout (int timeout)
{
  /* Ignore TIMEOUT if it is zero, because it will be unset really soon.  */
  if (timeout > 0)
    {
      char buf[16];
      
      grub_sprintf (buf, "%d", timeout);
      grub_env_set ("timeout", buf);
    }
}

/* Get the entry number from the variable NAME.  */
static int
get_entry_number (const char *name)
{
  char *val;
  int entry;
  
  val = grub_env_get (name);
  if (! val)
    return -1;
  
  grub_error_push ();
  
  entry = (int) grub_strtoul (val, 0, 0);

  if (grub_errno != GRUB_ERR_NONE)
    {
      grub_errno = GRUB_ERR_NONE;
      entry = -1;
    }

  grub_error_pop ();
  
  return entry;
}

static int
run_menu (grub_menu_t menu, int nested)
{
  int first, offset;
  unsigned long saved_time;
  int default_entry;
  
  first = 0;
  
  default_entry = get_entry_number ("default");

  /* If DEFAULT_ENTRY is not within the menu entries, fall back to
     the first entry.  */
  if (default_entry < 0 || default_entry >= menu->size)
    default_entry = 0;

  offset = default_entry;
  if (offset > GRUB_TERM_NUM_ENTRIES - 1)
    {
      first = offset - (GRUB_TERM_NUM_ENTRIES - 1);
      offset = GRUB_TERM_NUM_ENTRIES - 1;
    }

  /* Initialize the time.  */
  saved_time = grub_get_rtc ();

 refresh:
  grub_setcursor (0);
  grub_menu_init_page (nested, 0);
  print_entries (menu, first, offset);
  grub_refresh ();

  while (1)
    {
      int c;
      int timeout;

      timeout = get_timeout ();
      
      if (timeout > 0)
	{
	  unsigned long current_time;

	  current_time = grub_get_rtc ();
	  if (current_time - saved_time >= GRUB_TICKS_PER_SECOND)
	    {
	      timeout--;
	      set_timeout (timeout);
	      saved_time = current_time;
	    }
	  
	  grub_gotoxy (0, GRUB_TERM_HEIGHT - 3);
	  /* NOTE: Do not remove the trailing space characters.
	     They are required to clear the line.  */
	  grub_printf ("\
   The highlighted entry will be booted automatically in %d s.    ",
		       timeout);
	  grub_gotoxy (GRUB_TERM_CURSOR_X, GRUB_TERM_FIRST_ENTRY_Y + offset);
	  grub_refresh ();
	}

      if (timeout == 0)
	{
	  grub_env_unset ("timeout");
	  return default_entry;
	}
      
      if (grub_checkkey () >= 0 || timeout < 0)
	{
	  c = GRUB_TERM_ASCII_CHAR (grub_getkey ());
	  
	  if (timeout >= 0)
	    {
	      grub_gotoxy (0, GRUB_TERM_HEIGHT - 3);
              grub_printf ("\
                                                                        ");
	      grub_env_unset ("timeout");
	      grub_env_unset ("fallback");
	      grub_gotoxy (GRUB_TERM_CURSOR_X, GRUB_TERM_FIRST_ENTRY_Y + offset);
	    }
	  
	  switch (c)
	    {
	    case 16:
	    case '^':
	      if (offset > 0)
		{
		  print_entry (GRUB_TERM_FIRST_ENTRY_Y + offset, 0,
			       get_entry (menu, first + offset));
		  offset--;
		  print_entry (GRUB_TERM_FIRST_ENTRY_Y + offset, 1,
			       get_entry (menu, first + offset));
		}
	      else if (first > 0)
		{
		  first--;
		  print_entries (menu, first, offset);
		}
	      break;
	      
	    case 14:
	    case 'v':
	      if (menu->size > first + offset + 1)
		{
		  if (offset < GRUB_TERM_NUM_ENTRIES - 1)
		    {
		      print_entry (GRUB_TERM_FIRST_ENTRY_Y + offset, 0,
				   get_entry (menu, first + offset));
		      offset++;
		      print_entry (GRUB_TERM_FIRST_ENTRY_Y + offset, 1,
				   get_entry (menu, first + offset));
		    }
		  else
		    {
		      first++;
		      print_entries (menu, first, offset);
		    }
		}
	      break;
	      
	    case '\n':
	    case '\r':
	    case 6:
	      grub_setcursor (1);
	      return first + offset;
	      
	    case '\e':
	      if (nested)
		{
		  grub_setcursor (1);
		  return -1;
		}
	      break;
	      
	    case 'c':
	      grub_cmdline_run (1);
	      goto refresh;

	    case 'e':
		{
		  grub_menu_entry_t e = get_entry (menu, first + offset);
		  if (e)
		    grub_menu_entry_run (e);
		}
	      goto refresh;
	      
	    default:
	      break;
	    }
	  
	  grub_refresh ();
	}
    }

  /* Never reach here.  */
  return -1;
}

/* Run a menu entry.  */
static void
run_menu_entry (grub_menu_entry_t entry)
{
  grub_script_execute (entry->commands);
  
  if (grub_errno == GRUB_ERR_NONE && grub_loader_is_loaded ())
    /* Implicit execution of boot, only if something is loaded.  */
    grub_command_execute ("boot", 0);
}

void
grub_menu_run (grub_menu_t menu, int nested)
{
  while (1)
    {
      int boot_entry;
      grub_menu_entry_t e;
      int fallback_entry;
      
      boot_entry = run_menu (menu, nested);
      if (boot_entry < 0)
	break;

      e = get_entry (menu, boot_entry);
      if (! e)
	continue; /* Menu is empty.  */
	
      grub_cls ();
      grub_setcursor (1);

      grub_printf ("  Booting \'%s\'\n\n", e->title);
  
      run_menu_entry (e);

      /* Deal with a fallback entry.  */
      /* FIXME: Multiple fallback entries like GRUB Legacy.  */
      fallback_entry = get_entry_number ("fallback");
      if (fallback_entry >= 0)
	{
	  grub_print_error ();
	  grub_errno = GRUB_ERR_NONE;
	  
	  e = get_entry (menu, fallback_entry);
	  grub_env_unset ("fallback");
	  grub_printf ("\n  Falling back to \'%s\'\n\n", e->title);
	  run_menu_entry (e);
	}

      if (grub_errno != GRUB_ERR_NONE)
	{
	  grub_print_error ();
	  grub_errno = GRUB_ERR_NONE;

	  grub_wait_after_message ();
	}
    }
}
