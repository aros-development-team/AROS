/* char_io.c - basic console input and output */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 1996  Erich Boleyn  <erich@uruk.org>
 *  Copyright (C) 1999, 2000  Free Software Foundation, Inc.
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

#include <shared.h>

#ifdef SUPPORT_SERIAL
# include <serial.h>
#endif

#ifndef STAGE1_5
int auto_fill = 1;
#endif

void
print_error (void)
{
  if (errnum > ERR_NONE && errnum < MAX_ERR_NUM)
#ifndef STAGE1_5
    /* printf("\7\n %s\n", err_list[errnum]); */
    printf ("\nError %u: %s\n", errnum, err_list[errnum]);
#else /* STAGE1_5 */
    printf ("Error %u\n", errnum);
#endif /* STAGE1_5 */
}

char *
convert_to_ascii (char *buf, int c,...)
{
  unsigned long num = *((&c) + 1), mult = 10;
  char *ptr = buf;

#ifndef STAGE1_5
  if (c == 'x' || c == 'X' || c == 'b')
    mult = 16;

  if ((num & 0x80000000uL) && c == 'd')
    {
      num = (~num) + 1;
      *(ptr++) = '-';
      buf++;
    }
#endif

  do
    {
      int dig = num % mult;
      *(ptr++) = ((dig > 9) ? dig + 'a' - 10 : '0' + dig);
    }
  while (num /= mult);

  /* reorder to correct direction!! */
  {
    char *ptr1 = ptr - 1;
    char *ptr2 = buf;
    while (ptr1 > ptr2)
      {
	int tmp = *ptr1;
	*ptr1 = *ptr2;
	*ptr2 = tmp;
	ptr1--;
	ptr2++;
      }
  }

  return ptr;
}

void
grub_printf (const char *format,...)
{
  int *dataptr = (int *) &format;
  char c, *ptr, str[16];
  unsigned long mask = 0xFFFFFFFF;
  
  dataptr++;

  while ((c = *(format++)) != 0)
    {
      if (c != '%')
	putchar (c);
      else
	switch (c = *(format++))
	  {
#ifndef STAGE1_5
	  case 'b':
	    mask = 0xFF;
	    /* Fall down intentionally!  */
	  case 'd':
	  case 'x':
	  case 'X':
#endif
	  case 'u':
	    *convert_to_ascii (str, c, *((unsigned long *) dataptr++) & mask)
	      = 0;

	    ptr = str;

	    while (*ptr)
	      putchar (*(ptr++));
	    break;

#ifndef STAGE1_5
	  case 'c':
	    putchar ((*(dataptr++)) & 0xff);
	    break;

	  case 's':
	    ptr = (char *) (*(dataptr++));

	    while ((c = *(ptr++)) != 0)
	      putchar (c);
	    break;
#endif
	  }
    }
}

#ifndef STAGE1_5
int
grub_sprintf (char *buffer, const char *format, ...)
{
  /* XXX hohmuth
     ugly hack -- should unify with printf() */
  int *dataptr = (int *) &format;
  char c, *ptr, str[16];
  char *bp = buffer;

  dataptr++;

  while ((c = *format++) != 0)
    {
      if (c != '%')
	*bp++ = c; /* putchar(c); */
      else
	switch (c = *(format++))
	  {
	  case 'd': case 'u': case 'x':
	    *convert_to_ascii (str, c, *((unsigned long *) dataptr++)) = 0;

	    ptr = str;

	    while (*ptr)
	      *bp++ = *(ptr++); /* putchar(*(ptr++)); */
	    break;

	  case 'c': *bp++ = (*(dataptr++))&0xff;
	    /* putchar((*(dataptr++))&0xff); */
	    break;

	  case 's':
	    ptr = (char *) (*(dataptr++));

	    while ((c = *ptr++) != 0)
	      *bp++ = c; /* putchar(c); */
	    break;
	  }
    }

  *bp = 0;
  return bp - buffer;
}


void
init_page (void)
{
  cls ();

  printf ("\n    GRUB  version %s  (%dK lower / %dK upper memory)\n\n",
	  version_string, mbi.mem_lower, mbi.mem_upper);
}

/* The number of the history entries.  */
static int num_history = 0;

/* Get the NOth history. If NO is less than zero or greater than or
   equal to NUM_HISTORY, return NULL. Otherwise return a valid string.  */
static char *
get_history (int no)
{
  if (no < 0 || no >= num_history)
    return 0;

  return (char *) HISTORY_BUF + MAX_CMDLINE * no;
}

/* Add CMDLINE to the history buffer.  */
static void
add_history (const char *cmdline, int no)
{
  grub_memmove ((char *) HISTORY_BUF + MAX_CMDLINE * (no + 1),
		(char *) HISTORY_BUF + MAX_CMDLINE * no,
		MAX_CMDLINE * (num_history - no));
  grub_strcpy ((char *) HISTORY_BUF + MAX_CMDLINE * no, cmdline);
  if (num_history < HISTORY_SIZE)
    num_history++;
}

/* Don't use this with a MAXLEN greater than 1600 or so!  The problem
   is that GET_CMDLINE depends on the everything fitting on the screen
   at once.  So, the whole screen is about 2000 characters, minus the
   PROMPT, and space for error and status lines, etc.  MAXLEN must be
   at least 1, and PROMPT and CMDLINE must be valid strings (not NULL
   or zero-length).

   If ECHO_CHAR is nonzero, echo it instead of the typed character. */
int
get_cmdline (char *prompt, char *cmdline, int maxlen,
	     int echo_char, int readline)
{
  /* This is a rather complicated function. So explain the concept.
     
     A command-line consists of ``section''s. A section is a part of the
     line which may be displayed on the screen, but a section is never
     displayed with another section simultaneously.

     Each section is basically 77 or less characters, but the exception
     is the first section, which is 78 or less characters, because the
     starting point is special. See below.

     The first section contains a prompt and a command-line (or the
     first part of a command-line when it is too long to be fit in the
     screen). So, in the first section, the number of command-line
     characters displayed is 78 minus the length of the prompt (or
     less). If the command-line has more characters, `>' is put at the
     position 78 (zero-origin), to inform the user of the hidden
     characters.

     Other sections always have `<' at the first position, since there
     is absolutely a section before each section. If there is a section
     after another section, this section consists of 77 characters and
     `>' at the last position. The last section has 77 or less
     characters and doesn't have `>'.

     Each section other than the last shares some characters with the
     previous section. This region is called ``margin''. If the cursor
     is put at the magin which is shared by the first section and the
     second, the first section is displayed. Otherwise, a displayed
     section is switched to another section, only if the cursor is put
     outside that section.  */

  /* XXX: These should be defined in shared.h, but I leave these here,
     until this code is freezed.  */
#define CMDLINE_WIDTH	78
#define CMDLINE_MARGIN	10
  
  int xpos, lpos, c, section;
  /* The length of PROMPT.  */
  int plen;
  /* The length of the command-line.  */
  int llen;
  /* The index for the history.  */
  int history = -1;
  /* The working buffer for the command-line.  */
  char *buf = (char *) CMDLINE_BUF;
  /* The kill buffer.  */
  char *kill_buf = (char *) KILL_BUF;
  /* The original state of AUTO_FILL.  */
  int saved_auto_fill = auto_fill;
  
  /* Nested function definitions for code simplicity.  */

  /* The forward declarations of nested functions are prefixed
     with `auto'.  */
  auto void cl_refresh (int full, int len);
  auto void cl_backward (int count);
  auto void cl_forward (int count);
  auto void cl_insert (const char *str);
  auto void cl_delete (int count);
  auto void cl_init (void);
  
  /* Move the cursor backward.  */
  void cl_backward (int count)
    {
      lpos -= count;
      
      /* If the cursor is in the first section, display the first section
	 instead of the second.  */
      if (section == 1 && plen + lpos < CMDLINE_WIDTH)
	cl_refresh (1, 0);
      else if (xpos - count < 1)
	cl_refresh (1, 0);
      else
	{
	  xpos -= count;

	  if (terminal & TERMINAL_CONSOLE)
	    {
	      int y = getxy () & 0xFF;
	      
	      gotoxy (xpos, y);
	    }
# ifdef SUPPORT_SERIAL
	  else if (! (terminal & TERMINAL_DUMB) && (count > 4))
	    {
	      grub_printf ("\e[%dD", count);
	    }
	  else
	    {
	      int i;
	      
	      for (i = 0; i < count; i++)
		grub_putchar ('\b');
	    }
# endif /* SUPPORT_SERIAL */
	}
    }

  /* Move the cursor forward.  */
  void cl_forward (int count)
    {
      lpos += count;

      /* If the cursor goes outside, scroll the screen to the right.  */
      if (xpos + count >= CMDLINE_WIDTH)
	cl_refresh (1, 0);
      else
	{
	  xpos += count;

	  if (terminal & TERMINAL_CONSOLE)
	    {
	      int y = getxy () & 0xFF;
	      
	      gotoxy (xpos, y);
	    }
# ifdef SUPPORT_SERIAL
	  else if (! (terminal & TERMINAL_DUMB) && (count > 4))
	    {
	      grub_printf ("\e[%dC", count);
	    }
	  else
	    {
	      int i;
	      
	      for (i = lpos - count; i < lpos; i++)
		{
		  if (! echo_char)
		    grub_putchar (buf[i]);
		  else
		    grub_putchar (echo_char);
		}
	    }
# endif /* SUPPORT_SERIAL */
	}
    }

  /* Refresh the screen. If FULL is true, redraw the full line, otherwise,
     only LEN characters from LPOS.  */
  void cl_refresh (int full, int len)
    {
      int i;
      int start;
      int pos = xpos;
      
      if (full)
	{
	  /* Recompute the section number.  */
	  if (lpos + plen < CMDLINE_WIDTH)
	    section = 0;
	  else
	    section = ((lpos + plen - CMDLINE_WIDTH)
		       / (CMDLINE_WIDTH - 1 - CMDLINE_MARGIN) + 1);

	  /* From the start to the end.  */
	  len = CMDLINE_WIDTH;
	  pos = 0;
	  grub_putchar ('\r');

	  /* If SECTION is the first section, print the prompt, otherwise,
	     print `<'.  */
	  if (section == 0)
	    {
	      grub_printf ("%s", prompt);
	      len -= plen;
	      pos += plen;
	    }
	  else
	    {
	      grub_putchar ('<');
	      len--;
	      pos++;
	    }
	}

      /* Compute the index to start writing BUF and the resulting position
	 on the screen.  */
      if (section == 0)
	{
	  int offset = 0;
	  
	  if (! full)
	    offset = xpos - plen;
	  
	  start = 0;
	  xpos = lpos + plen;
	  start += offset;
	}
      else
	{
	  int offset = 0;
	  
	  if (! full)
	    offset = xpos - 1;
	  
	  start = ((section - 1) * (CMDLINE_WIDTH - 1 - CMDLINE_MARGIN)
		   + CMDLINE_WIDTH - plen - CMDLINE_MARGIN);
	  xpos = lpos + 1 - start;
	  start += offset;
	}

      /* Print BUF. If ECHO_CHAR is not zero, put it instead.  */
      for (i = start; i < start + len && i < llen; i++)
	{
	  if (! echo_char)
	    grub_putchar (buf[i]);
	  else
	    grub_putchar (echo_char);

	  pos++;
	}

      /* Fill up the rest of the line with spaces.  */
      for (; i < start + len; i++)
	{
	  grub_putchar (' ');
	  pos++;
	}

      /* If the cursor is at the last position, put `>' or a space,
	 depending on if there are more characters in BUF.  */
      if (pos == CMDLINE_WIDTH)
	{
	  if (start + len < llen)
	    grub_putchar ('>');
	  else
	    grub_putchar (' ');
	  
	  pos++;
	}

      /* Back to XPOS.  */
      if (terminal & TERMINAL_CONSOLE)
	{
	  int y = getxy () & 0xFF;
	  
	  gotoxy (xpos, y);
	}
# ifdef SUPPORT_SERIAL      
      else if (! (terminal & TERMINAL_SERIAL) && (pos - xpos > 4))
	{
	  grub_printf ("\e[%dD", pos - xpos);
	}
      else
	{
	  for (i = 0; i < pos - xpos; i++)
	    grub_putchar ('\b');
	}
# endif /* SUPPORT_SERIAL */
    }

  /* Initialize the command-line.  */
  void cl_init (void)
    {
      /* Distinguish us from other lines and error messages!  */
      grub_putchar ('\n');

      /* Print full line and set position here.  */
      cl_refresh (1, 0);
    }

  /* Insert STR to BUF.  */
  void cl_insert (const char *str)
    {
      int l = grub_strlen (str);

      if (llen + l < maxlen)
	{
	  if (lpos == llen)
	    grub_memmove (buf + lpos, str, l + 1);
	  else
	    {
	      grub_memmove (buf + lpos + l, buf + lpos, llen - lpos + 1);
	      grub_memmove (buf + lpos, str, l);
	    }
	  
	  llen += l;
	  lpos += l;
	  if (xpos + l >= CMDLINE_WIDTH)
	    cl_refresh (1, 0);
	  else if (xpos + l + llen - lpos > CMDLINE_WIDTH)
	    cl_refresh (0, CMDLINE_WIDTH - xpos);
	  else
	    cl_refresh (0, l + llen - lpos);
	}
    }

  /* Delete COUNT characters in BUF.  */
  void cl_delete (int count)
    {
      grub_memmove (buf + lpos, buf + lpos + count, llen - count + 1);
      llen -= count;
      
      if (xpos + llen + count - lpos > CMDLINE_WIDTH)
	cl_refresh (0, CMDLINE_WIDTH - xpos);
      else
	cl_refresh (0, llen + count - lpos);
    }

  plen = grub_strlen (prompt);
  llen = grub_strlen (cmdline);

  if (maxlen > MAX_CMDLINE)
    {
      maxlen = MAX_CMDLINE;
      if (llen >= MAX_CMDLINE)
	{
	  llen = MAX_CMDLINE - 1;
	  cmdline[MAX_CMDLINE] = 0;
	}
    }
  lpos = llen;
  grub_strcpy (buf, cmdline);

  /* Disable the auto fill mode.  */
  auto_fill = 0;
  
  cl_init ();

  while (ASCII_CHAR (c = getkey ()) != '\n' && ASCII_CHAR (c) != '\r')
    {
      c = translate_keycode (c);
      
      /* If READLINE is non-zero, handle readline-like key bindings.  */
      if (readline)
	{
	  switch (c)
	    {
	    case 9:		/* TAB lists completions */
	      {
		int i;
		/* POS points to the first space after a command.  */
		int pos = 0;
		int ret;
		char *completion_buffer = (char *) COMPLETION_BUF;
		int equal_pos = -1;
		int is_filename;

		/* Find the first word.  */
		while (buf[pos] == ' ')
		  pos++;
		while (buf[pos] && buf[pos] != '=' && buf[pos] != ' ')
		  pos++;

		is_filename = (lpos > pos);

		/* Find the position of the equal character after a
		   command, and replace it with a space.  */
		for (i = pos; buf[i] && buf[i] != ' '; i++)
		  if (buf[i] == '=')
		    {
		      equal_pos = i;
		      buf[i] = ' ';
		      break;
		    }

		/* Find the position of the first character in this
		   word.  */
		for (i = lpos; i > 0 && buf[i - 1] != ' '; i--)
		  ;

		/* Invalidate the cache, because the user may exchange
		   removable disks.  */
		buf_drive = -1;

		/* Copy this word to COMPLETION_BUFFER and do the
		   completion.  */
		grub_memmove (completion_buffer, buf + i, lpos - i);
		completion_buffer[lpos - i] = 0;
		/* Enable the auto fill mode temporarily.  */
		auto_fill = 1;
		ret = print_completions (is_filename, 1);
		/* Disable the auto fill mode again.  */
		auto_fill = 0;
		errnum = ERR_NONE;

		if (ret >= 0)
		  {
		    /* Found, so insert COMPLETION_BUFFER.  */
		    cl_insert (completion_buffer + lpos - i);

		    if (ret > 0)
		      {
			/* There are more than one candidates, so print
			   the list.  */

			grub_putchar ('\n');
			/* Enable the auto fill mode temporarily.  */
			auto_fill = 1;
			print_completions (is_filename, 0);
			/* Disable the auto fill mode again.  */
			auto_fill = 0;
			errnum = ERR_NONE;
		      }
		  }

		/* Restore the command-line.  */
		if (equal_pos >= 0)
		  buf[equal_pos] = '=';

		if (ret)
		  cl_init ();
	      }
	      break;
	    case 1:		/* C-a go to beginning of line */
	      cl_backward (lpos);
	      break;
	    case 5:		/* C-e go to end of line */
	      cl_forward (llen - lpos);
	      break;
	    case 6:		/* C-f forward one character */
	      if (lpos < llen)
		cl_forward (1);
	      break;
	    case 2:		/* C-b backward one character */
	      if (lpos > 0)
		cl_backward (1);
	      break;
	    case 21:		/* C-u kill to beginning of line */
	      if (lpos == 0)
		break;
	      /* Copy the string being deleted to KILL_BUF.  */
	      grub_memmove (kill_buf, buf, lpos);
	      kill_buf[lpos] = 0;
	      {
		/* XXX: Not very clever.  */
		
		int count = lpos;
		
		cl_backward (lpos);
		cl_delete (count);
	      }
	      break;
	    case 11:		/* C-k kill to end of line */
	      if (lpos == llen)
		break;
	      /* Copy the string being deleted to KILL_BUF.  */
	      grub_memmove (kill_buf, buf + lpos, llen - lpos + 1);
	      cl_delete (llen - lpos);
	      break;
	    case 25:		/* C-y yank the kill buffer */
	      cl_insert (kill_buf);
	      break;
	    case 16:		/* C-p fetch the previous command */
	      {
		char *p;

		if (history < 0)
		  /* Save the working buffer.  */
		  grub_strcpy (cmdline, buf);
		else if (grub_strcmp (get_history (history), buf) != 0)
		  /* If BUF is modified, add it into the history list.  */
		  add_history (buf, history);

		history++;
		p = get_history (history);
		if (! p)
		  {
		    history--;
		    break;
		  }

		grub_strcpy (buf, p);
		llen = grub_strlen (buf);
		lpos = llen;
		cl_refresh (1, 0);
	      }
	      break;
	    case 14:		/* C-n fetch the next command */
	      {
		char *p;

		if (history < 0)
		  {
		    break;
		  }
		else if (grub_strcmp (get_history (history), buf) != 0)
		  /* If BUF is modified, add it into the history list.  */
		  add_history (buf, history);

		history--;
		p = get_history (history);
		if (! p)
		  p = cmdline;

		grub_strcpy (buf, p);
		llen = grub_strlen (buf);
		lpos = llen;
		cl_refresh (1, 0);
	      }
	      break;
	    }
	}

      /* ESC, C-d and C-h are always handled. Actually C-d is not
	 functional if READLINE is zero, as the cursor cannot go
	 backward, but that's ok.  */
      switch (c)
	{
	case 27:	/* ESC immediately return 1 */
	  return 1;
	case 4:		/* C-d delete character under cursor */
	  if (lpos == llen)
	    break;
	  cl_delete (1);
	  break;
	case 8:		/* C-h backspace */
# ifdef GRUB_UTIL
	case 127:	/* also backspace */
# endif
	  if (lpos > 0)
	    {
	      cl_backward (1);
	      cl_delete (1);
	    }
	  break;
	default:		/* insert printable character into line */
	  if (c >= ' ' && c <= '~')
	    {
	      char str[2];

	      str[0] = c;
	      str[1] = 0;
	      cl_insert (str);
	    }
	}
    }

  grub_putchar ('\n');

  /* If ECHO_CHAR is NUL, remove the leading spaces.  */
  lpos = 0;
  if (! echo_char)
    while (buf[lpos] == ' ')
      lpos++;

  /* Copy the working buffer to CMDLINE.  */
  grub_memmove (cmdline, buf + lpos, llen - lpos + 1);

  /* If the readline-like feature is turned on and CMDLINE is not
     empty, add it into the history list.  */
  if (readline && lpos < llen)
    add_history (cmdline, 0);

  /* Restore the auto fill mode.  */
  auto_fill = saved_auto_fill;
  
  return 0;
}

/* Translate a special key to a common ascii code.  */
int
translate_keycode (int c)
{
# ifdef SUPPORT_SERIAL
  if (terminal & TERMINAL_SERIAL)
    {
      /* In a serial terminal, things are complicated, because several
	 key codes start from the character ESC, while we want to accept
	 ESC itself.  */
      if (c == '\e')
	{
	  int start;

	  /* Get current time.  */
	  start = currticks ();

	  while (checkkey () == -1)
	    {
	      /* Wait for a next character, at least for 0.1 sec
		 (18.2 ticks/sec).  */
	      int now;
	      
	      now = currticks ();
	      if (now - start >= 2)
		return c;
	    }

	  c = getkey ();
	  if (c == '[')
	    {
	      int c1, c2;

	      /* To filter illegal states.  */
	      c = 0;
	      c1 = getkey ();
	      switch (c1)
		{
		case 'A':	/* KEY_UP */
		  c = 16;
		  break;
		case 'B':	/* KEY_DOWN */
		  c = 14;
		  break;
		case 'C':	/* KEY_RIGHT */
		  c = 6;
		  break;
		case 'D':	/* KEY_LEFT */
		  c = 2;
		  break;
		case 'F':	/* End */
		  c = 5;
		  break;
		case 'H':	/* Home */
		  c = 1;
		  break;
		case '1':
		  c2 = getkey ();
		  if (c2 == '~')
		    {
		      /* One of control keys (pos1,....).  */
		      c = 1;
		    }
		  break;
		case '3':
		  c2 = getkey ();
		  if (c2 == '~')
		    {
		      /* One of control keys (del,....).  */
		      c = 4;
		    }
		  break;
		case '4':	/* Del */
		  c = 4;
		  break;
		}
	    }
	}
    }
  else
# endif /* SUPPORT_SERIAL */
    {
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
	case KEY_HOME:
	  c = 1;
	  break;
	case KEY_END:
	  c = 5;
	  break;
	case KEY_DC:
	  c = 4;
	  break;
	case KEY_BACKSPACE:
	  c = 8;
	  break;
	}
    }
  
  return ASCII_CHAR (c);
}
#endif /* STAGE1_5 */

int
safe_parse_maxint (char **str_ptr, int *myint_ptr)
{
  char *ptr = *str_ptr;
  int myint = 0;
  int mult = 10, found = 0;

  /*
   *  Is this a hex number?
   */
  if (*ptr == '0' && tolower (*(ptr + 1)) == 'x')
    {
      ptr += 2;
      mult = 16;
    }

  while (1)
    {
      /* A bit tricky. This below makes use of the equivalence:
	 (A >= B && A <= C) <=> ((A - B) <= (C - B))
	 when C > B and A is unsigned.  */
      unsigned int digit;

      digit = tolower (*ptr) - '0';
      if (digit > 9)
	{
	  digit -= 'a' - '0';
	  if (mult == 10 || digit > 5)
	    break;
	  digit += 10;
	}

      found = 1;
      if (myint > ((MAXINT - digit) / mult))
	{
	  errnum = ERR_NUMBER_PARSING;
	  return 0;
	}
      myint = (myint * mult) + digit;
      ptr++;
    }

  if (!found)
    {
      errnum = ERR_NUMBER_PARSING;
      return 0;
    }

  *str_ptr = ptr;
  *myint_ptr = myint;

  return 1;
}

int
grub_tolower (int c)
{
  if (c >= 'A' && c <= 'Z')
    return (c + ('a' - 'A'));

  return c;
}

int
grub_isspace (int c)
{
  if (c == ' ' || c == '\t' || c == '\r' || c == '\n')
    return 1;

  return 0;
}

#ifndef STAGE1_5
int
grub_memcmp (const char *s1, const char *s2, int n)
{
  while (n)
    {
      if (*s1 < *s2)
	return -1;
      else if (*s1 > *s2)
	return 1;
      s1++;
      s2++;
      n--;
    }

  return 0;
}

int
grub_strncat (char *s1, const char *s2, int n)
{
  int i = -1;

  while (++i < n && s1[i] != 0);

  while (i < n && (s1[i++] = *(s2++)) != 0);

  s1[n - 1] = 0;

  if (i >= n)
    return 0;

  s1[i] = 0;

  return 1;
}

int
grub_strcmp (const char *s1, const char *s2)
{
  while (*s1 || *s2)
    {
      if (*s1 < *s2)
	return -1;
      else if (*s1 > *s2)
	return 1;
      s1 ++;
      s2 ++;
    }

  return 0;
}

/* Wait for a keypress and return its code.  */
int
getkey (void)
{
  int c = -1;
  
  if (terminal & TERMINAL_CONSOLE)
    c = console_getkey ();
#ifdef SUPPORT_SERIAL
  else if (terminal & TERMINAL_SERIAL)
    c = serial_getkey ();
#endif /* SUPPORT_SERIAL */

  return c;
}

/* Check if a key code is available.  */
int
checkkey (void)
{
  int c = -1;

  if (terminal & TERMINAL_CONSOLE)
    c = console_checkkey ();

#ifdef SUPPORT_SERIAL
  if (terminal & TERMINAL_SERIAL)
    c = serial_checkkey ();
#endif /* SUPPORT_SERIAL */

  return c;
}

#endif /* ! STAGE1_5 */

/* Display an ASCII character.  */
void
grub_putchar (int c)
{
#ifndef STAGE1_5
  static int col = 0;
#endif
  
  if (c == '\n')
    grub_putchar ('\r');
  
#ifdef STAGE1_5
  
  /* In Stage 1.5, only the normal console is supported.  */
  console_putchar (c);
  
#else /* ! STAGE1_5 */

  /* Track the cursor by software here.  */
  /* XXX: This doesn't handle horizontal or vertical tabs.  */
  if (c == '\r')
    col = 0;
  else if (c == '\b')
    {
      if (col > 0)
	col--;
    }
  else if (c >= ' ' && c <= '~')
    {
      /* Fold a line only if AUTO_FILL is true.  */
      if (auto_fill && col >= 79)
	grub_putchar ('\n');

      col++;
    }

  if (terminal & TERMINAL_CONSOLE)
    console_putchar (c);

# ifdef SUPPORT_SERIAL
  if (terminal & TERMINAL_SERIAL)
    serial_putchar (c);
# endif /* SUPPORT_SERIAL */
  
#endif /* ! STAGE1_5 */
}

#ifndef STAGE1_5
void
gotoxy (int x, int y)
{
  if (terminal & TERMINAL_CONSOLE)
    console_gotoxy (x, y);
#ifdef SUPPORT_SERIAL
  else if (terminal & TERMINAL_SERIAL)
    serial_gotoxy (x, y);
#endif
}

#ifdef SUPPORT_SERIAL
/* The serial part of gotoxy.  */
void
serial_gotoxy (int x, int y)
{
  grub_printf ("\e[%d;%dH", y + 1, x + 1);
}
#endif /* SUPPORT_SERIAL */

int
getxy (void)
{
  int ret = 0;
  
  if (terminal & TERMINAL_CONSOLE)
    ret = console_getxy ();
#ifdef SUPPORT_SERIAL
  else if (terminal & TERMINAL_SERIAL)
    ret = serial_getxy ();
#endif
  
  return ret;
}

#ifdef SUPPORT_SERIAL
/* The serial part of getxy.  */
int
serial_getxy (void)
{
  int x, y;
  int start, now;
  char buf[32];	/* XXX */
  int i;
  int c;
  char *p;
  
  /* Drain the input buffer.  */
  while (serial_checkkey () != -1)
    serial_getkey ();

  /* CPR.  */
  grub_printf ("\e[6n");

  /* Get current time.  */
  while ((start = getrtsecs ()) == 0xFF)
    ;

 again:
  i = 0;
  c = 0;
  do
    {
      if (serial_checkkey () != -1)
	{
	  c = serial_getkey ();
	  if (i == 1 && c != '[')
	    i = 0;
	  
	  if (i == 0 && c != '\e')
	    continue;

	  if (i != 0 && c == '\e')
	    i = 0;

	  buf[i] = c;
	  i++;
	}
      else
	{
	  /* Get current time.  */
	  while ((now = getrtsecs ()) == 0xFF)
	    ;

	  /* FIXME: Remove this magic number.  */
	  if (now - start > 10)
	    {
	      /* Something is quite wrong.  */
	      return 0;
	    }
	}
    }
  while (c != 'R' && i < sizeof (buf));
  
  if (c != 'R')
    goto again;

  p = buf + 2;
  if (! safe_parse_maxint (&p, &y))
    {
      errnum = 0;
      goto again;
    }
  
  if (*p != ';')
    goto again;

  p++;
  if (! safe_parse_maxint (&p, &x))
    {
      errnum = 0;
      goto again;
    }

  if (*p != 'R')
    goto again;

  return ((x - 1) << 8) | (y - 1);
}
#endif /* SUPPORT_SERIAL */

void
cls (void)
{
  if (terminal & TERMINAL_CONSOLE)
    console_cls ();
#ifdef SUPPORT_SERIAL
  else if (terminal & TERMINAL_SERIAL)
    serial_cls ();
#endif
}

#ifdef SUPPORT_SERIAL
/* The serial part of cls.  */
void
serial_cls (void)
{
  /* If the terminal is dumb, there is no way to clean the terminal.  */
  if (terminal & TERMINAL_DUMB)
    grub_putchar ('\n');
  else
    grub_printf ("\e[H\e[J");
}
#endif /* SUPPORT_SERIAL */
#endif /* ! STAGE1_5 */

int
substring (char *s1, char *s2)
{
  while (*s1 == *s2)
    {
      /* The strings match exactly. */
      if (! *(s1++))
	return 0;
      s2 ++;
    }

  /* S1 is a substring of S2. */
  if (*s1 == 0)
    return -1;

  /* S1 isn't a substring. */
  return 1;
}

#ifndef STAGE1_5
/* Terminate the string STR with NUL.  */
int
nul_terminate (char *str)
{
  int ch;
  
  while (*str && ! grub_isspace (*str))
    str++;

  ch = *str;
  *str = 0;
  return ch;
}

char *
grub_strstr (const char *s1, const char *s2)
{
  const char *ptr, *tmp;

  while (*s1)
    {
      ptr = s1;
      tmp = s2;

      while (*s1 && *s1++ == *tmp++);

      if (tmp > s2 && !*(tmp - 1))
	return (char *) ptr;
    }

  return 0;
}

int
grub_strlen (const char *str)
{
  int len = 0;

  while (*str++)
    len++;

  return len;
}
#endif /* ! STAGE1_5 */

int
memcheck (int addr, int len)
{
#ifdef GRUB_UTIL
  static int start_addr (void)
    {
      int ret;
# if defined(HAVE_START_SYMBOL)
      asm volatile ("movl	$start, %0" : "=a" (ret));
# elif defined(HAVE_USCORE_START_SYMBOL)
      asm volatile ("movl	$_start, %0" : "=a" (ret));
# endif
      return ret;
    }

  static int end_addr (void)
    {
      int ret;
# if defined(HAVE_END_SYMBOL)
      asm volatile ("movl	$end, %0" : "=a" (ret));
# elif defined(HAVE_USCORE_END_SYMBOL)
      asm volatile ("movl	$_end, %0" : "=a" (ret));
# endif
      return ret;
    }

  if (start_addr () <= addr && end_addr () > addr + len)
    return ! errnum;
#endif /* GRUB_UTIL */

  if ((addr < RAW_ADDR (0x1000))
      || (addr < RAW_ADDR (0x100000)
	  && RAW_ADDR (mbi.mem_lower * 1024) < (addr + len))
      || (addr >= RAW_ADDR (0x100000)
	  && RAW_ADDR (mbi.mem_upper * 1024) < ((addr - 0x100000) + len)))
    errnum = ERR_WONT_FIT;

  return ! errnum;
}

void *
grub_memmove (void *to, const void *from, int len)
{
   if (memcheck ((int) to, len))
     {
       /* This assembly code is stolen from
	  linux-2.2.2/include/asm-i386/string.h. This is not very fast
	  but compact.  */
       int d0, d1, d2;

       if (to < from)
	 {
	   asm volatile ("cld\n\t"
			 "rep\n\t"
			 "movsb"
			 : "=&c" (d0), "=&S" (d1), "=&D" (d2)
			 : "0" (len),"1" (from),"2" (to)
			 : "memory");
	 }
       else
	 {
	   asm volatile ("std\n\t"
			 "rep\n\t"
			 "movsb\n\t"
			 "cld"
			 : "=&c" (d0), "=&S" (d1), "=&D" (d2)
			 : "0" (len),
			 "1" (len - 1 + (const char *) from),
			 "2" (len - 1 + (char *) to)
			 : "memory");
	 }
     }

   return errnum ? NULL : to;
}

#ifndef STAGE1_5
void *
grub_memset (void *start, int c, int len)
{
  char *p = start;

  if (memcheck ((int) start, len))
    {
      while (len -- > 0)
	*p ++ = c;
    }

  return errnum ? NULL : start;
}

char *
grub_strcpy (char *dest, const char *src)
{
  grub_memmove (dest, src, grub_strlen (src) + 1);
  return dest;
}
#endif /* ! STAGE1_5 */
