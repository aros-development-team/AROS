/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 1999,2000,2001,2002,2003,2004,2005  Free Software Foundation, Inc.
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
#include <grub/misc.h>
#include <grub/term.h>
#include <grub/err.h>
#include <grub/types.h>
#include <grub/mm.h>
#include <grub/partition.h>
#include <grub/disk.h>
#include <grub/file.h>
#include <grub/env.h>

static char *kill_buf;

static int hist_size;
static char **hist_lines = 0;
static int hist_pos = 0;
static int hist_end = 0;
static int hist_used = 0;

grub_err_t
grub_set_history (int newsize)
{
  char **old_hist_lines = hist_lines;
  hist_lines = grub_malloc (sizeof (char *) * newsize);

  /* Copy the old lines into the new buffer.  */
  if (old_hist_lines)
    {
      /* Remove the lines that don't fit in the new buffer.  */
      if (newsize < hist_used)
	{
	  int i;
	  int delsize = hist_used - newsize;
	  hist_used = newsize;

	  for (i = 1; i <= delsize; i++)
	    {
	      int pos = hist_end - i;
	      if (pos < 0)
		pos += hist_size;
	      grub_free (old_hist_lines[pos]);
	    }

	  hist_end -= delsize;
	  if (hist_end < 0)
	    hist_end += hist_size;
	}

      if (hist_pos < hist_end)
	grub_memmove (hist_lines, old_hist_lines + hist_pos,
		      (hist_end - hist_pos) * sizeof (char *));
      else if (hist_used)
	{
	  /* Copy the older part.  */
	  grub_memmove (hist_lines, old_hist_lines + hist_pos,
 			(hist_size - hist_pos) * sizeof (char *));
	  
	  /* Copy the newer part. */
	  grub_memmove (hist_lines + hist_size - hist_pos, old_hist_lines,
			hist_end * sizeof (char *));
	}
    }

  grub_free (old_hist_lines);

  hist_size = newsize;
  hist_pos = 0;
  hist_end = hist_used;
  return 0;
}

/* Get the entry POS from the history where `0' is the newest
   entry.  */
static char *
grub_history_get (int pos)
{
  pos = (hist_pos + pos) % hist_size;
  return hist_lines[pos];
}


/* Insert a new history line S on the top of the history.  */
static void
grub_history_add (char *s)
{
  /* Remove the oldest entry in the history to make room for a new
     entry.  */
  if (hist_used + 1 > hist_size)
    {
      hist_end--;
      if (hist_end < 0)
	hist_end = hist_size + hist_end;

      grub_free (hist_lines[hist_end]);
    }
  else
    hist_used++;

  /* Move to the next position.  */
  hist_pos--;
  if (hist_pos < 0)
    hist_pos = hist_size + hist_pos;

  /* Insert into history.  */
  hist_lines[hist_pos] = grub_strdup (s);
}

/* Replace the history entry on position POS with the string S.  */
static void
grub_history_replace (int pos, char *s)
{
  pos = (hist_pos + pos) % hist_size;
  grub_free (hist_lines[pos]);
  hist_lines[pos] = grub_strdup (s);
}

void
grub_cmdline_run (int nested)
{
  grub_normal_init_page ();
  grub_setcursor (1);
  
  grub_printf ("\
 [ Minimal BASH-like line editing is supported. For the first word, TAB\n\
   lists possible command completions. Anywhere else TAB lists possible\n\
   device/file completions.%s ]\n\n",
	       nested ? " ESC at any time exits." : "");
  
  while (1)
    {
      static char cmdline[GRUB_MAX_CMDLINE];

      grub_print_error ();
      grub_errno = GRUB_ERR_NONE;
      cmdline[0] = '\0';
      
      if (! grub_cmdline_get ("grub> ", cmdline, sizeof (cmdline), 0, 1)
	  && nested)
	return;

      if (! *cmdline)
	continue;

      grub_command_execute (cmdline, 1);
    }
}

/* A completion hook to print items.  */
static void
print_completion (const char *item, grub_completion_type_t type, int count)
{
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
	    
      grub_printf ("\nPossible %s are:\n", what);
    }

  if (type == GRUB_COMPLETION_TYPE_PARTITION)
    {
      grub_normal_print_device_info (item);
      grub_errno = GRUB_ERR_NONE;
    }
  else
    grub_printf (" %s", item);
}

/* Get a command-line. If ECHO_CHAR is not zero, echo it instead of input
   characters. If READLINE is non-zero, readline-like key bindings are
   available. If ESC is pushed, return zero, otherwise return non-zero.  */
/* FIXME: The dumb interface is not supported yet.  */
int
grub_cmdline_get (const char *prompt, char cmdline[], unsigned max_len,
		  int echo_char, int readline)
{
  unsigned xpos, ypos, ystart;
  grub_size_t lpos, llen;
  grub_size_t plen;
  char buf[max_len];
  int key;
  int histpos = 0;
  auto void cl_insert (const char *str);
  auto void cl_delete (unsigned len);
  auto void cl_print (int pos, int c);
  auto void cl_set_pos (void);

  void cl_set_pos (void)
    {
      xpos = (plen + lpos) % 79;
      ypos = ystart + (plen + lpos) / 79;
      grub_gotoxy (xpos, ypos);
    }
  
  void cl_print (int pos, int c)
    {
      char *p;

      for (p = buf + pos; *p; p++)
	{
	  if (xpos++ > 78)
	    {
	      grub_putchar ('\n');
	      
	      xpos = 1;
	      if (ypos == (unsigned) (grub_getxy () & 0xFF))
		ystart--;
	      else
		ypos++;
	    }

	  if (c)
	    grub_putchar (c);
	  else
	    grub_putchar (*p);
	}
    }
  
  void cl_insert (const char *str)
    {
      grub_size_t len = grub_strlen (str);

      if (len + llen < max_len)
	{
	  grub_memmove (buf + lpos + len, buf + lpos, llen - lpos + 1);
	  grub_memmove (buf + lpos, str, len);

	  llen += len;
	  lpos += len;
	  cl_print (lpos - len, echo_char);
	  cl_set_pos ();
	}
    }

  void cl_delete (unsigned len)
    {
      if (lpos + len <= llen)
	{
	  grub_size_t saved_lpos = lpos;

	  lpos = llen - len;
	  cl_set_pos ();
	  cl_print (lpos, ' ');
	  lpos = saved_lpos;
	  cl_set_pos ();
	  
	  grub_memmove (buf + lpos, buf + lpos + len, llen - lpos + 1);
	  llen -= len;
	  cl_print (lpos, echo_char);
	  cl_set_pos ();
	}
    }
  
  plen = grub_strlen (prompt);
  lpos = llen = 0;
  buf[0] = '\0';

  if ((grub_getxy () >> 8) != 0)
    grub_putchar ('\n');
  
  grub_printf (prompt);
  
  xpos = plen;
  ystart = ypos = (grub_getxy () & 0xFF);
  
  cl_insert (cmdline);

  if (hist_used == 0)
    grub_history_add (buf);

  while ((key = GRUB_TERM_ASCII_CHAR (grub_getkey ())) != '\n' && key != '\r')
    {
      if (readline)
	{
	  switch (key)
	    {
	    case 1:	/* Ctrl-a */
	      lpos = 0;
	      cl_set_pos ();
	      break;

	    case 2:	/* Ctrl-b */
	      if (lpos > 0)
		{
		  lpos--;
		  cl_set_pos ();
		}
	      break;

	    case 5:	/* Ctrl-e */
	      lpos = llen;
	      cl_set_pos ();
	      break;

	    case 6:	/* Ctrl-f */
	      if (lpos < llen)
		{
		  lpos++;
		  cl_set_pos ();
		}
	      break;

	    case 9:	/* Ctrl-i or TAB */
	      {
		char *insert;
		int restore;
		
		/* Backup the next character and make it 0 so it will
		   be easy to use string functions.  */
		char backup = buf[lpos];
		buf[lpos] = '\0';
		

		insert = grub_normal_do_completion (buf, &restore,
						    print_completion);
		/* Restore the original string.  */
		buf[lpos] = backup;
		
		if (restore)
		  {
		    /* Restore the prompt.  */
		    grub_printf ("\n%s%s", prompt, buf);
		    xpos = plen;
		    ystart = ypos = (grub_getxy () & 0xFF);
		  }

		if (insert)
		  {
		    cl_insert (insert);
		    grub_free (insert);
		  }
	      }
	      break;

	    case 11:	/* Ctrl-k */
	      if (lpos < llen)
		{
		  if (kill_buf)
		    grub_free (kill_buf);

		  kill_buf = grub_strdup (buf + lpos);
		  grub_errno = GRUB_ERR_NONE;

		  cl_delete (llen - lpos);
		}
	      break;

	    case 14:	/* Ctrl-n */
	      {
		char *hist;

		lpos = 0;

		if (histpos > 0)
		  {
		    grub_history_replace (histpos, buf);
		    histpos--;
		  }

		cl_delete (llen);
		hist = grub_history_get (histpos);
		cl_insert (hist);

		break;
	      }
	    case 16:	/* Ctrl-p */
	      {
		char *hist;

		lpos = 0;

		if (histpos < hist_used - 1)
		  {
		    grub_history_replace (histpos, buf);
		    histpos++;
		  }

		cl_delete (llen);
		hist = grub_history_get (histpos);

		cl_insert (hist);
	      }
	      break;

	    case 21:	/* Ctrl-u */
	      if (lpos > 0)
		{
		  grub_size_t n = lpos;
		  
		  if (kill_buf)
		    grub_free (kill_buf);

		  kill_buf = grub_malloc (n + 1);
		  grub_errno = GRUB_ERR_NONE;
		  if (kill_buf)
		    {
		      grub_memcpy (kill_buf, buf, n);
		      kill_buf[n] = '\0';
		    }

		  lpos = 0;
		  cl_set_pos ();
		  cl_delete (n);
		}
	      break;

	    case 25:	/* Ctrl-y */
	      if (kill_buf)
		cl_insert (kill_buf);
	      break;
	    }
	}

      switch (key)
	{
	case '\e':
	  return 0;

	case '\b':
	  if (lpos > 0)
	    {
	      lpos--;
	      cl_set_pos ();
	    }
          else
            break;
	  /* fall through */
	  
	case 4:	/* Ctrl-d */
	  if (lpos < llen)
	    cl_delete (1);
	  break;

	default:
	  if (grub_isprint (key))
	    {
	      char str[2];

	      str[0] = key;
	      str[1] = '\0';
	      cl_insert (str);
	    }
	  break;
	}
    }

  grub_putchar ('\n');
  grub_refresh ();

  /* If ECHO_CHAR is NUL, remove leading spaces.  */
  lpos = 0;
  if (! echo_char)
    while (buf[lpos] == ' ')
      lpos++;

  histpos = 0;
  if (grub_strlen (buf) > 0)
    {
      grub_history_replace (histpos, buf);
      grub_history_add ("");
    }
  
  grub_memcpy (cmdline, buf + lpos, llen - lpos + 1);

  return 1;
}
