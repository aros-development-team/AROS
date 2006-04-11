/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2005  Free Software Foundation, Inc.
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

#include <grub/normal.h>
#include <grub/term.h>
#include <grub/misc.h>
#include <grub/mm.h>
#include <grub/loader.h>

enum update_mode
  {
    NO_LINE,
    SINGLE_LINE,
    ALL_LINES
  };

struct line
{
  /* The line buffer.  */
  char *buf;
  /* The length of the line.  */
  int len;
  /* The maximum length of the line.  */
  int max_len;
};

struct screen
{
  /* The array of lines.  */
  struct line *lines;
  /* The number of lines.  */
  int num_lines;
  /* The current column.  */
  int column;
  /* The real column.  */
  int real_column;
  /* The current line.  */
  int line;
  /* The X coordinate.  */
  int x;
  /* The Y coordinate.  */
  int y;
  /* The kill buffer.  */
  char *killed_text;
  /* The flag of a completion window.  */
  int completion_shown;
};

/* Used for storing completion items temporarily.  */
static struct line completion_buffer;

/* Initialize a line.  */
static int
init_line (struct line *linep)
{
  linep->len = 0;
  linep->max_len = 80; /* XXX */
  linep->buf = grub_malloc (linep->max_len);
  if (! linep->buf)
    return 0;

  return 1;
}

/* Allocate extra space if necessary.  */
static int
ensure_space (struct line *linep, int extra)
{
  if (linep->max_len < linep->len + extra)
    {
      linep->max_len = linep->len + extra + 80; /* XXX */
      linep->buf = grub_realloc (linep->buf, linep->max_len + 1);
      if (! linep->buf)
	return 0;
    }

  return 1;
}

/* Return the number of lines occupied by this line on the screen.  */
static int
get_logical_num_lines (struct line *linep)
{
  return (linep->len / GRUB_TERM_ENTRY_WIDTH) + 1;
}

/* Print a line.  */
static void
print_line (struct line *linep, int offset, int start, int y)
{
  int i;
  char *p;
  
  grub_gotoxy (GRUB_TERM_LEFT_BORDER_X + GRUB_TERM_MARGIN + start + 1,
	       y + GRUB_TERM_FIRST_ENTRY_Y);

  for (p = linep->buf + offset + start, i = start;
       i < GRUB_TERM_ENTRY_WIDTH && offset + i < linep->len;
       p++, i++)
    grub_putchar (*p);

  for (; i < GRUB_TERM_ENTRY_WIDTH; i++)
    grub_putchar (' ');

  if (linep->len >= offset + GRUB_TERM_ENTRY_WIDTH)
    grub_putchar ('\\');
  else
    grub_putchar (' ');
}

/* Print an empty line.  */
static void
print_empty_line (int y)
{
  int i;
  
  grub_gotoxy (GRUB_TERM_LEFT_BORDER_X + GRUB_TERM_MARGIN + 1,
	       y + GRUB_TERM_FIRST_ENTRY_Y);

  for (i = 0; i < GRUB_TERM_ENTRY_WIDTH + 1; i++)
    grub_putchar (' ');
}

/* Print an up arrow.  */
static void
print_up (int flag)
{
  grub_gotoxy (GRUB_TERM_LEFT_BORDER_X + GRUB_TERM_BORDER_WIDTH,
	       GRUB_TERM_FIRST_ENTRY_Y);
  
  if (flag)
    grub_putcode (GRUB_TERM_DISP_UP);
  else
    grub_putchar (' ');
}
	  
/* Print a down arrow.  */
static void
print_down (int flag)
{
  grub_gotoxy (GRUB_TERM_LEFT_BORDER_X + GRUB_TERM_BORDER_WIDTH,
	       GRUB_TERM_TOP_BORDER_Y + GRUB_TERM_NUM_ENTRIES);
  
  if (flag)
    grub_putcode (GRUB_TERM_DISP_DOWN);
  else
    grub_putchar (' ');
}
	  
/* Draw the lines of the screen SCREEN.  */
static void
update_screen (struct screen *screen, int region_start, int region_column,
	       int up, int down, enum update_mode mode)
{
  int up_flag = 0;
  int down_flag = 0;
  int y;
  int i;
  struct line *linep;
  
  /* Check if scrolling is necessary.  */
  if (screen->y < 0 || screen->y >= GRUB_TERM_NUM_ENTRIES)
    {
      if (screen->y < 0)
	screen->y = 0;
      else
	screen->y = GRUB_TERM_NUM_ENTRIES - 1;
      
      region_start = 0;
      region_column = 0;
      up = 1;
      down = 1;
      mode = ALL_LINES;
    }

  if (mode != NO_LINE)
    {
      /* Draw lines. This code is tricky, because this must calculate logical
	 positions.  */
      y = screen->y - screen->column / GRUB_TERM_ENTRY_WIDTH;
      i = screen->line;
      linep = screen->lines + i;
      while (y > 0)
	{
	   i--;
	   linep--;
	   y -= get_logical_num_lines (linep);
	}
      
      if (y < 0 || i > 0)
	up_flag = 1;
      
      do
	{
	  int column;
	  
	  for (column = 0;
	       column <= linep->len && y < GRUB_TERM_NUM_ENTRIES;
	       column += GRUB_TERM_ENTRY_WIDTH, y++)
	    {
	      if (y < 0)
		continue;
	      
	      if (i == region_start)
		{
		  if (region_column >= column
		      && region_column < column + GRUB_TERM_ENTRY_WIDTH)
		    print_line (linep, column, region_column - column, y);
		  else if (region_column < column)
		    print_line (linep, column, 0, y);
		}
	      else if (i > region_start && mode == ALL_LINES)
		print_line (linep, column, 0, y);
	    }
	  
	  if (y == GRUB_TERM_NUM_ENTRIES)
	    {
	      if (column <= linep->len || i + 1 < screen->num_lines)
		down_flag = 1;
	    }
	  
	  linep++;
	  i++;

	  if (mode == ALL_LINES && i == screen->num_lines)
	    for (; y < GRUB_TERM_NUM_ENTRIES; y++)
	      print_empty_line (y);
		  
	}
      while (y < GRUB_TERM_NUM_ENTRIES);
      
      /* Draw up and down arrows.  */
      if (up)
	print_up (up_flag);
      if (down)
	print_down (down_flag);
    }
  
  /* Place the cursor.  */
  grub_gotoxy (GRUB_TERM_LEFT_BORDER_X + GRUB_TERM_MARGIN + 1 + screen->x,
	       GRUB_TERM_FIRST_ENTRY_Y + screen->y);
  
  grub_refresh ();
}

/* Insert the string S into the screen SCREEN. This updates the cursor
   position and redraw the screen. Return zero if fails.  */
static int
insert_string (struct screen *screen, char *s, int update)
{
  int region_start = screen->num_lines;
  int region_column = 0;
  int down = 0;
  enum update_mode mode = NO_LINE;

  while (*s)
    {
      if (*s == '\n')
	{
	  /* LF is special because it creates a new line.  */
	  struct line *current_linep;
	  struct line *next_linep;
	  int size;
	  
	  /* Make a new line.  */
	  screen->num_lines++;
	  screen->lines = grub_realloc (screen->lines,
					screen->num_lines
					* sizeof (struct line));
	  if (! screen->lines)
	    return 0;

	  /* Scroll down. */
	  grub_memmove (screen->lines + screen->line + 2,
			screen->lines + screen->line + 1,
			((screen->num_lines - screen->line - 2)
			 * sizeof (struct line)));

	  if (! init_line (screen->lines + screen->line + 1))
	    return 0;
	  
	  /* Fold the line.  */
	  current_linep = screen->lines + screen->line;
	  next_linep = current_linep + 1;
	  size = current_linep->len - screen->column;
	      
	  if (! ensure_space (next_linep, size))
	    return 0;
	  
	  grub_memmove (next_linep->buf,
			current_linep->buf + screen->column,
			size);
	  current_linep->len = screen->column;
	  next_linep->len = size;

	  /* Update a dirty region.  */
	  if (region_start > screen->line)
	    {
	      region_start = screen->line;
	      region_column = screen->column;
	    }

	  mode = ALL_LINES;
	  down = 1; /* XXX not optimal.  */
	  
	  /* Move the cursor.  */
	  screen->column = screen->real_column = 0;
	  screen->line++;
	  screen->x = 0;
	  screen->y++;

	  s++;
	}
      else
	{
	  /* All but LF.  */
	  char *p;
	  struct line *current_linep;
	  int size;
	  int orig_num, new_num;

	  /* Find a string delimitted by LF.  */
	  p = grub_strchr (s, '\n');
	  if (! p)
	    p = s + grub_strlen (s);

	  /* Insert the string.  */
	  current_linep = screen->lines + screen->line;
	  size = p - s;
	  if (! ensure_space (current_linep, size))
	    return 0;

	  grub_memmove (current_linep->buf + screen->column + size,
			current_linep->buf + screen->column,
			current_linep->len - screen->column);
	  grub_memmove (current_linep->buf + screen->column,
			s,
			size);
	  orig_num = get_logical_num_lines (current_linep);
	  current_linep->len += size;
	  new_num = get_logical_num_lines (current_linep);

	  /* Update the dirty region.  */
	  if (region_start > screen->line)
	    {
	      region_start = screen->line;
	      region_column = screen->column;
	    }
	  
	  if (orig_num != new_num)
	    {
	      mode = ALL_LINES;
	      down = 1; /* XXX not optimal.  */
	    }
	  else if (mode != ALL_LINES)
	    mode = SINGLE_LINE;

	  /* Move the cursor.  */
	  screen->column += size;
	  screen->real_column = screen->column;
	  screen->x += size;
	  screen->y += screen->x / GRUB_TERM_ENTRY_WIDTH;
	  screen->x %= GRUB_TERM_ENTRY_WIDTH;
	  
	  s = p;
	}
    }

  if (update)
    update_screen (screen, region_start, region_column, 0, down, mode);
  
  return 1;
}

/* Release the resource allocated for SCREEN.  */
static void
destroy_screen (struct screen *screen)
{
  int i;

  if (screen->lines)
    for (i = 0; i < screen->num_lines; i++)
      {
	struct line *linep = screen->lines + i;
	
	if (linep)
	  grub_free (linep->buf);
      }

  grub_free (screen->killed_text);
  grub_free (screen->lines);
  grub_free (screen);
}

/* Make a new screen.  */
static struct screen *
make_screen (grub_menu_entry_t entry)
{
  struct screen *screen;
  grub_command_list_t cl;

  /* Initialize the screen.  */
  screen = grub_malloc (sizeof (*screen));
  if (! screen)
    return 0;

  screen->num_lines = 1;
  screen->column = 0;
  screen->real_column = 0;
  screen->line = 0;
  screen->x = 0;
  screen->y = 0;
  screen->killed_text = 0;
  screen->completion_shown = 0;
  screen->lines = grub_malloc (sizeof (struct line));
  if (! screen->lines)
    goto fail;

  /* Initialize the first line which must be always present.  */
  if (! init_line (screen->lines))
    goto fail;
  
  /* Input the entry.  */
  for (cl = entry->command_list; cl; cl = cl->next)
    {
      if (! insert_string (screen, cl->command, 0))
	goto fail;
      
      if (! insert_string (screen, "\n", 0))
	goto fail;
    }

  /* Reset the cursor position.  */
  screen->column = 0;
  screen->real_column = 0;
  screen->line = 0;
  screen->x = 0;
  screen->y = 0;

  return screen;

 fail:
  destroy_screen (screen);
  return 0;
}

static int
forward_char (struct screen *screen, int update)
{
  struct line *linep;
  
  linep = screen->lines + screen->line;
  if (screen->column < linep->len)
    {
      screen->column++;
      screen->x++;
      if (screen->x == GRUB_TERM_ENTRY_WIDTH)
	{
	  screen->x = 0;
	  screen->y++;
	}
    }
  else if (screen->num_lines > screen->line + 1)
    {
      screen->column = 0;
      screen->line++;
      screen->x = 0;
      screen->y++;
    }

  screen->real_column = screen->column;

  if (update)
    update_screen (screen, screen->num_lines, 0, 0, 0, NO_LINE);
  return 1;
}

static int
backward_char (struct screen *screen, int update)
{
  if (screen->column > 0)
    {
      screen->column--;
      screen->x--;
      if (screen->x == -1)
	{
	  screen->x = GRUB_TERM_ENTRY_WIDTH - 1;
	  screen->y--;
	}
    }
  else if (screen->line > 0)
    {
      struct line *linep;
      
      screen->line--;
      linep = screen->lines + screen->line;
      screen->column = linep->len;
      screen->x = screen->column % GRUB_TERM_ENTRY_WIDTH;
      screen->y--;
    }

  screen->real_column = screen->column;

  if (update)
    update_screen (screen, screen->num_lines, 0, 0, 0, NO_LINE);
  
  return 1;
}

static int
previous_line (struct screen *screen, int update)
{
  if (screen->line > 0)
    {
      struct line *linep;
      int dy;
      
      /* How many physical lines from the current position
	 to the first physical line?  */
      dy = screen->column / GRUB_TERM_ENTRY_WIDTH;
      
      screen->line--;
      
      linep = screen->lines + screen->line;
      if (linep->len < screen->real_column)
	screen->column = linep->len;
      else
	screen->column = screen->real_column;
      
      /* How many physical lines from the current position
	 to the last physical line?  */
      dy += (linep->len / GRUB_TERM_ENTRY_WIDTH
	     - screen->column / GRUB_TERM_ENTRY_WIDTH);
      
      screen->y -= dy + 1;
      screen->x = screen->column % GRUB_TERM_ENTRY_WIDTH;
    }
  else
    {
      screen->y -= screen->column / GRUB_TERM_ENTRY_WIDTH;
      screen->column = 0;
      screen->x = 0;
    }

  if (update)
    update_screen (screen, screen->num_lines, 0, 0, 0, NO_LINE);
  
  return 1;
}

static int
next_line (struct screen *screen, int update)
{
  if (screen->line < screen->num_lines - 1)
    {
      struct line *linep;
      int dy;
      
      /* How many physical lines from the current position
	 to the last physical line?  */
      linep = screen->lines + screen->line;
      dy = (linep->len / GRUB_TERM_ENTRY_WIDTH
	    - screen->column / GRUB_TERM_ENTRY_WIDTH);
      
      screen->line++;
      
      linep++;
      if (linep->len < screen->real_column)
	screen->column = linep->len;
      else
	screen->column = screen->real_column;
      
      /* How many physical lines from the current position
	 to the first physical line?  */
      dy += screen->column / GRUB_TERM_ENTRY_WIDTH;
      
      screen->y += dy + 1;
      screen->x = screen->column % GRUB_TERM_ENTRY_WIDTH;
    }
  else
    {
      struct line *linep;

      linep = screen->lines + screen->line;
      screen->y += (linep->len / GRUB_TERM_ENTRY_WIDTH
		    - screen->column / GRUB_TERM_ENTRY_WIDTH);
      screen->column = linep->len;
      screen->x = screen->column % GRUB_TERM_ENTRY_WIDTH;
    }

  if (update)
    update_screen (screen, screen->num_lines, 0, 0, 0, NO_LINE);
  
  return 1;
}

static int
beginning_of_line (struct screen *screen, int update)
{
  screen->y -= screen->column / GRUB_TERM_ENTRY_WIDTH;
  screen->column = screen->real_column = 0;
  screen->x = 0;

  if (update)
    update_screen (screen, screen->num_lines, 0, 0, 0, NO_LINE);
  
  return 1;
}

static int
end_of_line (struct screen *screen, int update)
{
  struct line *linep;
  
  linep = screen->lines + screen->line;
  screen->y += (linep->len / GRUB_TERM_ENTRY_WIDTH
		- screen->column / GRUB_TERM_ENTRY_WIDTH);
  screen->column = screen->real_column = linep->len;
  screen->x = screen->column % GRUB_TERM_ENTRY_WIDTH;

  if (update)
    update_screen (screen, screen->num_lines, 0, 0, 0, NO_LINE);
  
  return 1;
}

static int
delete_char (struct screen *screen, int update)
{
  struct line *linep;
  enum update_mode mode = NO_LINE;
  int start = screen->num_lines;
  int column = 0;
  int down = 0;
  
  linep = screen->lines + screen->line;
  if (linep->len > screen->column)
    {
      int orig_num, new_num;

      orig_num = get_logical_num_lines (linep);
      
      grub_memmove (linep->buf + screen->column,
		    linep->buf + screen->column + 1,
		    linep->len - screen->column - 1);
      linep->len--;

      new_num = get_logical_num_lines (linep);

      if (orig_num != new_num)
	mode = ALL_LINES;
      else
	mode = SINGLE_LINE;

      start = screen->line;
      column = screen->column;
    }
  else if (screen->num_lines > screen->line + 1)
    {
      struct line *next_linep;

      next_linep = linep + 1;
      if (! ensure_space (linep, next_linep->len))
	return 0;

      grub_memmove (linep->buf + linep->len, next_linep->buf, next_linep->len);
      linep->len += next_linep->len;

      grub_free (next_linep->buf);
      grub_memmove (next_linep,
		    next_linep + 1,
		    (screen->num_lines - screen->line - 2)
		    * sizeof (struct line));
      screen->num_lines--;
      
      mode = ALL_LINES;
      start = screen->line;
      column = screen->column;
      down = 1;
    }

  screen->real_column = screen->column;

  if (update)
    update_screen (screen, start, column, 0, down, mode);

  return 1;
}

static int
backward_delete_char (struct screen *screen, int update)
{
  int saved_column;
  int saved_line;

  saved_column = screen->column;
  saved_line = screen->line;
  
  if (! backward_char (screen, 0))
    return 0;

  if (saved_column != screen->column || saved_line != screen->line)
    if (! delete_char (screen, update))
      return 0;

  return 1;
}

static int
kill_line (struct screen *screen, int continuous, int update)
{
  struct line *linep;
  char *p;
  int size;
  int offset;
  
  p = screen->killed_text;
  if (! continuous && p)
    p[0] = '\0';
  
  linep = screen->lines + screen->line;
  size = linep->len - screen->column;
  
  if (p)
    offset = grub_strlen (p);
  else
    offset = 0;
      
  if (size > 0)
    {
      enum update_mode mode = SINGLE_LINE;
      int down = 0;
      int orig_num, new_num;
      
      p = grub_realloc (p, offset + size + 1);
      if (! p)
	return 0;

      grub_memmove (p + offset, linep->buf + screen->column, size);
      p[offset + size - 1] = '\0';

      screen->killed_text = p;
      
      orig_num = get_logical_num_lines (linep);
      linep->len = screen->column;
      new_num = get_logical_num_lines (linep);

      if (orig_num != new_num)
	{
	  mode = ALL_LINES;
	  down = 1;
	}

      if (update)
	update_screen (screen, screen->line, screen->column, 0, down, mode);
    }
  else if (screen->line + 1 < screen->num_lines)
    {
      p = grub_realloc (p, offset + 1 + 1);
      if (! p)
	return 0;

      p[offset] = '\n';
      p[offset + 1] = '\0';

      screen->killed_text = p;
      
      return delete_char (screen, update);
    }
    
  return 1;
}

static int
yank (struct screen *screen, int update)
{
  if (screen->killed_text)
    return insert_string (screen, screen->killed_text, update);

  return 1;
}

static int
open_line (struct screen *screen, int update)
{
  int saved_y = screen->y;
  
  if (! insert_string (screen, "\n", 0))
    return 0;

  if (! backward_char (screen, 0))
    return 0;

  screen->y = saved_y;

  if (update)
    update_screen (screen, screen->line, screen->column, 0, 1, ALL_LINES);

  return 1;
}

/* A completion hook to print items.  */
static void
store_completion (const char *item, grub_completion_type_t type, int count)
{
  char *p;
  
  if (count == 0)
    {
      /* If this is the first time, print a label.  */
      const char *what;

      switch (type)
	{
	case GRUB_COMPLETION_TYPE_COMMAND:
	  what = "commands";
	  break;
	case GRUB_COMPLETION_TYPE_DEVICE:
	  what = "devices";
	  break;
	case GRUB_COMPLETION_TYPE_FILE:
	  what = "files";
	  break;
	case GRUB_COMPLETION_TYPE_PARTITION:
	  what = "partitions";
	  break;
	case GRUB_COMPLETION_TYPE_ARGUMENT:
	  what = "arguments";
	  break;
	default:
	  what = "things";
	  break;
	}
	    
      grub_gotoxy (0, GRUB_TERM_HEIGHT - 3);
      grub_printf ("   Possible %s are:\n    ", what);
    }

  /* Make sure that the completion buffer has enough room.  */
  if (completion_buffer.max_len < (completion_buffer.len
				   + (int) grub_strlen (item) + 1 + 1))
    {
      grub_size_t new_len;
      
      new_len = completion_buffer.len + grub_strlen (item) + 80;
      p = grub_realloc (completion_buffer.buf, new_len);
      if (! p)
	{
	  /* Possibly not fatal.  */
	  grub_errno = GRUB_ERR_NONE;
	  return;
	}
      p[completion_buffer.len] = 0;
      completion_buffer.buf = p;
      completion_buffer.max_len = new_len;
    }

  p = completion_buffer.buf + completion_buffer.len;
  if (completion_buffer.len != 0)
    {
      *p++ = ' ';
      completion_buffer.len++;
    }
  grub_strcpy (p, item);
  completion_buffer.len += grub_strlen (item);
}

static int
complete (struct screen *screen, int continuous, int update)
{
  grub_uint16_t pos;
  char saved_char;
  struct line *linep;
  int restore;
  char *insert;
  static int count = -1;

  if (continuous)
    count++;
  else
    count = 0;
  
  pos = grub_getxy ();
  grub_gotoxy (0, GRUB_TERM_HEIGHT - 3);
  
  completion_buffer.buf = 0;
  completion_buffer.len = 0;
  completion_buffer.max_len = 0;

  linep = screen->lines + screen->line;
  saved_char = linep->buf[screen->column];
  linep->buf[screen->column] = '\0';

  insert = grub_normal_do_completion (linep->buf, &restore, store_completion);

  linep->buf[screen->column] = saved_char;
  
  if (restore)
    {
      char *p = completion_buffer.buf;

      screen->completion_shown = 1;
      
      if (p)
	{
	  int num_sections = ((completion_buffer.len + GRUB_TERM_WIDTH - 8 - 1)
			      / (GRUB_TERM_WIDTH - 8));
	  char *endp;

	  p += (count % num_sections) * (GRUB_TERM_WIDTH - 8);
	  endp = p + (GRUB_TERM_WIDTH - 8);

	  if (p != completion_buffer.buf)
	    grub_putcode (GRUB_TERM_DISP_LEFT);
	  else
	    grub_putchar (' ');
	  
	  while (*p && p < endp)
	    grub_putchar (*p++);
	  
	  if (*p)
	    grub_putcode (GRUB_TERM_DISP_RIGHT);
	}
    }

  grub_gotoxy (pos >> 8, pos & 0xFF);
  
  if (insert)
    {
      insert_string (screen, insert, update);
      count = -1;
      grub_free (insert);
    }
  else if (update)
    grub_refresh ();
    
  grub_free (completion_buffer.buf);
  return 1;
}

/* Clear displayed completions.  */
static void
clear_completions (void)
{
  grub_uint16_t pos;
  int i, j;
  
  pos = grub_getxy ();
  grub_gotoxy (0, GRUB_TERM_HEIGHT - 3);
  
  for (i = 0; i < 2; i++)
    {
      for (j = 0; j < GRUB_TERM_WIDTH - 1; j++)
	grub_putchar (' ');
      grub_putchar ('\n');
    }
  
  grub_gotoxy (pos >> 8, pos & 0xFF);
  grub_refresh ();
}

/* Execute the command list in the screen SCREEN.  */
static int
run (struct screen *screen)
{
  int i;
  
  grub_cls ();
  grub_printf ("  Booting a command list\n\n");
  
  for (i = 0; i < screen->num_lines; i++)
    {
      struct line *linep = screen->lines + i;
      char *p;
      
      /* Trim down space characters.  */
      for (p = linep->buf + linep->len - 1;
	   p >= linep->buf && grub_isspace (*p);
	   p--)
	;
      *++p = '\0';
      linep->len = p - linep->buf;
      
      for (p = linep->buf; grub_isspace (*p); p++)
	;
      
      if (*p == '\0')
	/* Ignore an empty command line.  */
	continue;

      if (grub_command_execute (p, 0) != 0)
	break;
    }
  
  if (grub_errno == GRUB_ERR_NONE && grub_loader_is_loaded ())
    /* Implicit execution of boot, only if something is loaded.  */
    grub_command_execute ("boot", 0);

  if (grub_errno != GRUB_ERR_NONE)
    {
      grub_print_error ();
      grub_errno = GRUB_ERR_NONE;
      /* Wait until the user pushes any key so that the user
	 can see what happened.  */
      grub_printf ("\nPress any key to continue...");
      (void) grub_getkey ();
    }
  
  return 1;
}

/* Edit a menu entry with an Emacs-like interface.  */
void
grub_menu_entry_run (grub_menu_entry_t entry)
{
  struct screen *screen;
  int prev_c;
  
  screen = make_screen (entry);
  if (! screen)
    return;

 refresh:
  /* Draw the screen.  */
  grub_menu_init_page (0, 1);
  update_screen (screen, 0, 0, 1, 1, ALL_LINES);
  grub_setcursor (1);
  prev_c = '\0';
  
  while (1)
    {
      int c = GRUB_TERM_ASCII_CHAR (grub_getkey ());

      if (screen->completion_shown)
	{
	  clear_completions ();
	  screen->completion_shown = 0;
	}
      
      switch (c)
	{
	case 16: /* C-p */
	  if (! previous_line (screen, 1))
	    goto fail;
	  break;

	case 14: /* C-n */
	  if (! next_line (screen, 1))
	    goto fail;
	  break;

	case 6: /* C-f */
	  if (! forward_char (screen, 1))
	    goto fail;
	  break;
	  
	case 2: /* C-b */
	  if (! backward_char (screen, 1))
	    goto fail;
	  break;

	case 1: /* C-a */
	  if (! beginning_of_line (screen, 1))
	    goto fail;
	  break;

	case 5: /* C-e */
	  if (! end_of_line (screen, 1))
	    goto fail;
	  break;
	  
	case '\t': /* C-i */
	  if (! complete (screen, prev_c == c, 1))
	    goto fail;
	  break;
	  
	case 4: /* C-d */
	  if (! delete_char (screen, 1))
	    goto fail;
	  break;

	case 8: /* C-h */
	  if (! backward_delete_char (screen, 1))
	    goto fail;
	  break;
	  
	case 11: /* C-k */
	  if (! kill_line (screen, prev_c == c, 1))
	    goto fail;
	  break;
	  
	case 21: /* C-u */
	  /* FIXME: What behavior is good for this key?  */
	  break;
	  
	case 25: /* C-y */
	  if (! yank (screen, 1))
	    goto fail;
	  break;

	case 12: /* C-l */
	  /* FIXME: centering.  */
	  goto refresh;
	  
	case 15: /* C-o */
	  if (! open_line (screen, 1))
	    goto fail;
	  break;

	case '\n':
	case '\r':
	  if (! insert_string (screen, "\n", 1))
	    goto fail;
	  break;

	case '\e':
	  destroy_screen (screen);
	  return;
	  
	case 3: /* C-c */
	  grub_cmdline_run (1);
	  goto refresh;
	  
	case 24: /* C-x */
	  if (! run (screen))
	    goto fail;
	  goto refresh;

	case 18: /* C-r */
	case 19: /* C-s */
	case 20: /* C-t */
	  /* FIXME */
	  break;
	  
	default:
	  if (grub_isprint (c))
	    {
	      char buf[2];

	      buf[0] = c;
	      buf[1] = '\0';
	      if (! insert_string (screen, buf, 1))
		goto fail;
	    }
	  break;
	}

      prev_c = c;
    }

 fail:
  destroy_screen (screen);
  
  grub_cls ();
  grub_print_error ();
  grub_errno = GRUB_ERR_NONE;
  grub_printf ("\nPress any key to continue...");
  (void) grub_getkey ();
}
