/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 1996  Erich Boleyn  <erich@uruk.org>
 *  Copyright (C) 2000  Free Software Foundation, Inc.
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

#include "shared.h"

grub_jmp_buf restart_env;

static char *
get_entry (char *list, int num, int nested)
{
  int i;

  for (i = 0; i < num; i++)
    {
      do
	{
	  while (*(list++));
	}
      while (nested && *(list++));
    }

  return list;
}


static void
print_entries (int y, int size, int first, char *menu_entries)
{
  int i;
  int disp_up = DISP_UP;
  int disp_down = DISP_DOWN;

#ifdef SUPPORT_SERIAL
  if (terminal & TERMINAL_SERIAL)
    {
      disp_up = ACS_UARROW;
      disp_down = ACS_DARROW;
    }
#endif /* SUPPORT_SERIAL */
  
  gotoxy (77, y + 1);

  if (first)
    grub_putchar (disp_up);
  else
    grub_putchar (' ');

  menu_entries = get_entry (menu_entries, first, 0);

  for (i = 1; i <= size; i++)
    {
      int j = 0;

      gotoxy (3, y + i);

      while (*menu_entries)
	{
	  if (j < 71)
	    {
	      grub_putchar (*menu_entries);
	      j++;
	    }

	  menu_entries++;
	}

      if (*(menu_entries - 1))
	menu_entries++;

      for (; j < 71; j++)
	grub_putchar (' ');
    }

  gotoxy (77, y + size);

  if (*menu_entries)
    grub_putchar (disp_down);
  else
    grub_putchar (' ');
}


static void
print_border (int y, int size)
{
  int i;
  int disp_ul = DISP_UL;
  int disp_ur = DISP_UR;
  int disp_ll = DISP_LL;
  int disp_lr = DISP_LR;
  int disp_horiz = DISP_HORIZ;
  int disp_vert = DISP_VERT;

#ifdef SUPPORT_SERIAL
  if (terminal & TERMINAL_SERIAL)
    {
      disp_ul = ACS_ULCORNER;
      disp_ur = ACS_URCORNER;
      disp_ll = ACS_LLCORNER;
      disp_lr = ACS_LRCORNER;
      disp_horiz = ACS_HLINE;
      disp_vert = ACS_VLINE;
    }
#endif /* SUPPORT_SERIAL */
  
#ifndef GRUB_UTIL
  /* Color the menu. The menu is 75 * 14 characters.  */
# ifdef SUPPORT_SERIAL
  if (terminal & TERMINAL_CONSOLE)
# endif
    {
      for (i = 0; i < 14; i++)
	{
	  int j;
	  for (j = 0; j < 75; j++)
	    {
	      gotoxy (j + 1, i + y);
	      set_attrib (normal_color);
	    }
	}
    }
#endif

  gotoxy (1, y);

  grub_putchar (disp_ul);
  for (i = 0; i < 73; i++)
    grub_putchar (disp_horiz);
  grub_putchar (disp_ur);

  i = 1;

  while (1)
    {
      gotoxy (1, y + i);

      if (i > size)
	break;

      grub_putchar (disp_vert);
      gotoxy (75, y + i);
      grub_putchar (disp_vert);

      i++;
    }

  grub_putchar (disp_ll);
  for (i = 0; i < 73; i++)
    grub_putchar (disp_horiz);
  grub_putchar (disp_lr);
}

static void
set_line (int y, int entryno, int attr, char *menu_entries)
{
  int x;

#ifdef SUPPORT_SERIAL
  if (terminal & TERMINAL_SERIAL)
    {
      menu_entries = get_entry (menu_entries, entryno, 0);
      gotoxy (2, y);
      grub_putchar (' ');
      for (x = 3; x < 75; x++)
	{
	  if (*menu_entries && x < 71)
	    grub_putchar (*menu_entries++);
	  else
	    grub_putchar (' ');
	}
    }
  else
#endif /* SUPPORT_SERIAL */
    {
      for (x = 2; x < 75; x++)
	{
	  gotoxy (x, y);
	  set_attrib (attr);
	}
    }

  gotoxy (74, y);
}

/* Set the attribute of the line Y to normal state.  */
static void
set_line_normal (int y, int entryno, char *menu_entries)
{
#ifdef GRUB_UTIL
  set_line (y, entryno, A_NORMAL, menu_entries);
#else
  set_line (y, entryno, normal_color, menu_entries);
#endif
}

/* Set the attribute of the line Y to highlight state.  */
static void
set_line_highlight (int y, int entryno, char *menu_entries)
{
#ifdef SUPPORT_SERIAL
  if (terminal & TERMINAL_SERIAL)
    grub_printf ("\e[7m");
#endif /* SUPPORT_SERIAL */
  
#ifdef GRUB_UTIL
  set_line (y, entryno, A_REVERSE, menu_entries);
#else
  set_line (y, entryno, highlight_color, menu_entries);
#endif
  
#ifdef SUPPORT_SERIAL
  if (terminal & TERMINAL_SERIAL)
    grub_printf ("\e[0m");
#endif /* SUPPORT_SERIAL */
}

static void
run_menu (char *menu_entries, char *config_entries, int num_entries,
	  char *heap, int entryno)
{
  int c, time1, time2 = -1, first_entry = 0;
  char *cur_entry = 0;
  int disp_up = DISP_UP;
  int disp_down = DISP_DOWN;

  /*
   *  Main loop for menu UI.
   */

restart:
  while (entryno > 11)
    {
      first_entry++;
      entryno--;
    }

  /* If the timeout was expired or wasn't set, force to show the menu
     interface. If the terminal is dumb and the timeout is set, hide
     the menu to boot the default entry automatically when the timeout
     is expired.  */
  if (grub_timeout < 0)
    show_menu = 1;
  else if (terminal & TERMINAL_DUMB)
    show_menu = 0;
  
  /* If SHOW_MENU is false, don't display the menu until ESC is pressed.  */
  if (! show_menu)
    {
      /* Get current time.  */
      while ((time1 = getrtsecs ()) == 0xFF)
	;

      while (1)
	{
	  /* Check if ESC is pressed.  */
	  if (checkkey () != -1 && ASCII_CHAR (getkey ()) == '\e')
	    {
	      grub_timeout = -1;
	      show_menu = 1;
	      break;
	    }

	  /* If GRUB_TIMEOUT is expired, boot the default entry.  */
	  if (grub_timeout >=0
	      && (time1 = getrtsecs ()) != time2
	      && time1 != 0xFF)
	    {
	      if (grub_timeout <= 0)
		{
		  grub_timeout = -1;
		  goto boot_entry;
		}
	      
	      time2 = time1;
	      grub_timeout--;
	      
	      /* Print a message.  */
	      if (terminal & TERMINAL_DUMB)
		grub_printf ("\rPress `ESC' to enter the command-line... %d   ",
			     grub_timeout);
	      else
		grub_printf ("\rPress `ESC' to enter the menu... %d   ",
			     grub_timeout);
	    }
	}
    }

  /* If the terminal is dumb, enter the command-line interface instead.  */
  if (show_menu && (terminal & TERMINAL_DUMB))
    {
      if (! auth && password)
	{
	  /* The user must enter a correct password.  */
	  char entered[32];

	  /* Make sure that PASSWORD is NUL-terminated.  */
	  nul_terminate (password);
	  
	  do
	    {
	      grub_memset (entered, 0, sizeof (entered));
	      get_cmdline ("Password: ", entered, 31, '*', 0);
	    }
	  while (grub_strcmp (password, entered) != 0);
	}
	  
      enter_cmdline (heap, 1);
    }
  
  /* Only display the menu if the user wants to see it. */
  if (show_menu)
    {
      /* Disable the auto fill mode.  */
      auto_fill = 0;
      
      init_page ();
#ifndef GRUB_UTIL
# ifdef SUPPORT_SERIAL
      if (terminal & TERMINAL_CONSOLE)
# endif /* SUPPORT_SERIAL */
	nocursor ();
#endif /* ! GRUB_UTIL */

      print_border (3, 12);

#ifdef GRUB_UTIL
      /* In the grub shell, always use ACS_*.  */
      disp_up = ACS_UARROW;
      disp_down = ACS_DARROW;
#else /* ! GRUB_UTIL */
# ifdef SUPPORT_SERIAL
      if (terminal & TERMINAL_CONSOLE)
	{
	  disp_up = DISP_UP;
	  disp_down = DISP_DOWN;
	}
      else
	{
	  disp_up = ACS_UARROW;
	  disp_down = ACS_DARROW;
	}
# endif /* SUPPORT_SERIAL */
#endif /* ! GRUB_UTIL */
      
      grub_printf ("\n\
      Use the %c and %c keys to select which entry is highlighted.\n",
		   disp_up, disp_down);
      
      if (! auth && password)
	{
	  printf ("\
      Press enter to boot the selected OS or \'p\' to enter a\n\
      password to unlock the next set of features.");
	}
      else
	{
	  if (config_entries)
	    printf ("\
      Press enter to boot the selected OS, \'e\' to edit the\n\
      commands before booting, or \'c\' for a command-line.");
	  else
	    printf ("\
      Press \'b\' to boot, \'e\' to edit the selected command in the\n\
      boot sequence, \'c\' for a command-line, \'o\' to open a new line\n\
      after (\'O\' for before) the selected line, \'d\' to remove the\n\
      selected line, or escape to go back to the main menu.");
	}

      print_entries (3, 12, first_entry, menu_entries);

      /* highlight initial line */
      set_line_highlight (4 + entryno, first_entry + entryno, menu_entries);
    }

  /* XX using RT clock now, need to initialize value */
  while ((time1 = getrtsecs()) == 0xFF);

  while (1)
    {
      /* Initialize to NULL just in case...  */
      cur_entry = NULL;

      if (grub_timeout >= 0 && (time1 = getrtsecs()) != time2 && time1 != 0xFF)
	{
	  if (grub_timeout <= 0)
	    {
	      grub_timeout = -1;
	      break;
	    }

	  /* else not booting yet! */
	  time2  = time1;
	  gotoxy (3, 22);
	  printf ("The highlighted entry will be booted automatically in %d seconds.    ", grub_timeout);
	  gotoxy (74, 4 + entryno);
	  grub_timeout--;
	}

      /* Check for a keypress, however if TIMEOUT has been expired
	 (GRUB_TIMEOUT == -1) relax in GETKEY even if no key has been
	 pressed.  
	 This avoids polling (relevant in the grub-shell and later on
	 in grub if interrupt driven I/O is done).  */
      if ((checkkey () != -1) || (grub_timeout == -1)) 
	{
	  c = translate_keycode (getkey ());

	  if (grub_timeout >= 0)
	    {
	      gotoxy (3, 22);
	      printf ("                                                                    ");
	      grub_timeout = -1;
	      fallback_entry = -1;
	      gotoxy (74, 4 + entryno);
	    }

	  /* We told them above (at least in SUPPORT_SERIAL) to use
	     '^' or 'v' so accept these keys.  */
	  if (c == 16 || c == '^')
	    {
	      if (entryno > 0)
		{
		  set_line_normal (4 + entryno, first_entry + entryno,
				   menu_entries);
		  entryno--;
		  set_line_highlight (4 + entryno, first_entry + entryno,
				      menu_entries);
		}
	      else if (first_entry > 0)
		{
		  first_entry--;
		  print_entries (3, 12, first_entry, menu_entries);
		  set_line_highlight (4, first_entry + entryno, menu_entries);
		}
	    }
	  if ((c == 14 || c == 'v') && first_entry + entryno + 1 < num_entries)
	    {
	      if (entryno < 11)
		{
		  set_line_normal (4 + entryno, first_entry + entryno,
				   menu_entries);
		  entryno++;
		  set_line_highlight (4 + entryno, first_entry + entryno,
				      menu_entries);
		}
	      else if (num_entries > 12 + first_entry)
		{
		  first_entry++;
		  print_entries (3, 12, first_entry, menu_entries);
		  set_line_highlight (15, first_entry + entryno, menu_entries);
		}
	    }

	  if (config_entries)
	    {
	      if ((c == '\n') || (c == '\r'))
		break;
	    }
	  else
	    {
	      if ((c == 'd') || (c == 'o') || (c == 'O'))
		{
		  set_line_normal (4 + entryno, first_entry + entryno,
				   menu_entries);

		  /* insert after is almost exactly like insert before */
		  if (c == 'o')
		    {
		      /* But `o' differs from `O', since it may causes
			 the menu screen to scroll up.  */
		      if (entryno < 11)
			entryno++;
		      else
			first_entry++;
		      
		      c = 'O';
		    }

		  cur_entry = get_entry (menu_entries,
					 first_entry + entryno,
					 0);

		  if (c == 'O')
		    {
		      memmove (cur_entry + 2, cur_entry,
			       ((int) heap) - ((int) cur_entry));

		      cur_entry[0] = ' ';
		      cur_entry[1] = 0;

		      heap += 2;

		      num_entries++;
		    }
		  else if (num_entries > 0)
		    {
		      char *ptr = get_entry(menu_entries,
					    first_entry + entryno + 1,
					    0);

		      memmove (cur_entry, ptr, ((int) heap) - ((int) ptr));
		      heap -= (((int) ptr) - ((int) cur_entry));

		      num_entries--;

		      if (entryno >= num_entries)
			entryno--;
		      if (first_entry && num_entries < 12 + first_entry)
			first_entry--;
		    }

		  print_entries (3, 12, first_entry, menu_entries);
		  set_line_highlight (4 + entryno, first_entry + entryno,
				      menu_entries);
		}

	      cur_entry = menu_entries;
	      if (c == 27)
		return;
	      if (c == 'b')
		break;
	    }

	  if (! auth && password)
	    {
	      if (c == 'p')
		{
		  /* Do password check here! */
		  char entered[32];
		  char *pptr = password;

		  gotoxy (1, 21);

		  /* Wipe out the previously entered password */
		  memset (entered, 0, sizeof (entered));
		  get_cmdline (" Password: ", entered, 31, '*', 0);

		  while (! isspace (*pptr) && *pptr)
		    pptr++;

		  /* Make sure that PASSWORD is NUL-terminated.  */
		  *pptr++ = 0;

		  if (! strcmp (password, entered))
		    {
		      char *new_file = config_file;
		      while (isspace (*pptr))
			pptr++;

		      /* If *PPTR is NUL, then allow the user to use
			 privileged instructions, otherwise, load
			 another configuration file.  */
		      if (*pptr != 0)
			{
			  while ((*(new_file++) = *(pptr++)) != 0)
			    ;

			  /* Make sure that the user will not have
			     authority in the next configuration.  */
			  auth = 0;
			  return;
			}
		      else
			{
			  /* Now the user is superhuman.  */
			  auth = 1;
			  goto restart;
			}
		    }
		  else
		    {
		      printf ("Failed!\n      Press any key to continue...");
		      getkey ();
		      goto restart;
		    }
		}
	    }
	  else
	    {
	      if (c == 'e')
		{
		  int new_num_entries = 0, i = 0;
		  char *new_heap;

		  if (config_entries)
		    {
		      new_heap = heap;
		      cur_entry = get_entry (config_entries,
					     first_entry + entryno,
					     1);
		    }
		  else
		    {
		      /* safe area! */
		      new_heap = heap + NEW_HEAPSIZE + 1;
		      cur_entry = get_entry (menu_entries,
					     first_entry + entryno,
					     0);
		    }

		  do
		    {
		      while ((*(new_heap++) = cur_entry[i++]) != 0);
		      new_num_entries++;
		    }
		  while (config_entries && cur_entry[i]);

		  /* this only needs to be done if config_entries is non-NULL,
		     but it doesn't hurt to do it always */
		  *(new_heap++) = 0;

		  if (config_entries)
		    run_menu (heap, NULL, new_num_entries, new_heap, 0);
		  else
		    {
		      cls ();
		      print_cmdline_message (0);

		      new_heap = heap + NEW_HEAPSIZE + 1;

		      saved_drive = boot_drive;
		      saved_partition = install_partition;
		      current_drive = 0xFF;

		      if (! get_cmdline (PACKAGE " edit> ", new_heap,
					 NEW_HEAPSIZE + 1, 0, 1))
			{
			  int j = 0;

			  /* get length of new command */
			  while (new_heap[j++])
			    ;

			  if (j < 2)
			    {
			      j = 2;
			      new_heap[0] = ' ';
			      new_heap[1] = 0;
			    }

			  /* align rest of commands properly */
			  memmove (cur_entry + j, cur_entry + i,
				   ((int) heap) - (((int) cur_entry) + i));

			  /* copy command to correct area */
			  memmove (cur_entry, new_heap, j);

			  heap += (j - i);
			}
		    }

		  goto restart;
		}
	      if (c == 'c')
		{
		  enter_cmdline (heap, 0);
		  goto restart;
		}
#ifdef GRUB_UTIL
	      if (c == 'q')
		{
		  /* The same as ``quit''.  */
		  stop ();
		}
#endif
	    }
	}
    }

  /* Attempt to boot an entry.  */
  
 boot_entry:
  /* Enable the auto fill mode.  */
  auto_fill = 1;
  
  while (1)
    {
      cls ();

      if (config_entries)
	printf ("  Booting \'%s\'\n\n",
		get_entry (menu_entries, first_entry + entryno, 0));
      else
	printf ("  Booting command-list\n\n");

      if (! cur_entry)
	cur_entry = get_entry (config_entries, first_entry + entryno, 1);

      /* Set CURRENT_ENTRYNO for the command "savedefault".  */
      current_entryno = first_entry + entryno;
      
      if (run_script (cur_entry, heap))
	{
	  if (fallback_entry < 0)
	    break;
	  else
	    {
	      cur_entry = NULL;
	      first_entry = 0;
	      entryno = fallback_entry;
	      fallback_entry = -1;
	    }
	}
      else
	break;
    }

  show_menu = 1;
  goto restart;
}


static int
get_line_from_config (char *cmdline, int maxlen)
{
  int pos = 0, literal = 0, comment = 0;
  char c;  /* since we're loading it a byte at a time! */

  while (grub_read (&c, 1))
    {
      /* translate characters first! */
      if (c == '\\' && ! literal)
	{
	  literal = 1;
	  continue;
	}
      if (c == '\r')
	continue;
      if ((c == '\t') || (literal && (c == '\n')))
	c = ' ';

      literal = 0;

      if (comment)
	{
	  if (c == '\n')
	    comment = 0;
	}
      else if (! pos)
	{
	  if (c == '#')
	    comment = 1;
	  else if ((c != ' ') && (c != '\n'))
	    cmdline[pos++] = c;
	}
      else
	{
	  if (c == '\n')
	    break;

	  if (pos < maxlen)
	    cmdline[pos++] = c;
	}
    }

  cmdline[pos] = 0;

  return pos;
}


/* This is the starting function in C.  */
void
cmain (void)
{
  int config_len, menu_len, num_entries;
  char *config_entries, *menu_entries;
  char *kill_buf = (char *) KILL_BUF;

  /* Initialize the environment for restarting Stage 2.  */
  grub_setjmp (restart_env);
  
  /* Initialize the kill buffer.  */
  *kill_buf = 0;

  /* Never return.  */
  for (;;)
    {
      auto_fill = 1;
      config_len = 0;
      menu_len = 0;
      num_entries = 0;
      config_entries = (char *) (mbi.mmap_addr + mbi.mmap_length);
      menu_entries = (char *) MENU_BUF;
      init_config ();

      /* Here load the configuration file.  */

#ifdef GRUB_UTIL
      if (use_config_file && grub_open (config_file))
#else
      if (grub_open (config_file))
#endif
	{
	  /* STATE 0:  Before any title command.
	     STATE 1:  In a title command.
	     STATE >1: In a entry after a title command.  */
	  int state = 0, prev_config_len = 0, prev_menu_len = 0;
	  char *cmdline;

	  cmdline = (char *) CMDLINE_BUF;
	  while (get_line_from_config (cmdline, NEW_HEAPSIZE))
	    {
	      struct builtin *builtin;

	      /* Get the pointer to the builtin structure.  */
	      builtin = find_command (cmdline);
	      errnum = 0;
	      if (! builtin)
		/* Unknown command. Just skip now.  */
		continue;

	      if (builtin->flags & BUILTIN_TITLE)
		{
		  char *ptr;

		  /* the command "title" is specially treated.  */
		  if (state > 1)
		    {
		      /* The next title is found.  */
		      num_entries++;
		      config_entries[config_len++] = 0;
		      prev_menu_len = menu_len;
		      prev_config_len = config_len;
		    }
		  else
		    {
		      /* The first title is found.  */
		      menu_len = prev_menu_len;
		      config_len = prev_config_len;
		    }

		  /* Reset the state.  */
		  state = 1;

		  /* Copy title into menu area.  */
		  ptr = skip_to (1, cmdline);
		  while ((menu_entries[menu_len++] = *(ptr++)) != 0)
		    ;
		}
	      else if (! state)
		{
		  /* Run a command found is possible.  */
		  if (builtin->flags & BUILTIN_MENU)
		    {
		      char *arg = skip_to (1, cmdline);
		      (builtin->func) (arg, BUILTIN_MENU);
		      errnum = 0;
		    }
		  else
		    /* Ignored.  */
		    continue;
		}
	      else
		{
		  char *ptr = cmdline;

		  state++;
		  /* Copy config file data to config area.  */
		  while ((config_entries[config_len++] = *ptr++) != 0)
		    ;
		}
	    }

	  if (state > 1)
	    {
	      /* Finish the last entry.  */
	      num_entries++;
	      config_entries[config_len++] = 0;
	    }
	  else
	    {
	      menu_len = prev_menu_len;
	      config_len = prev_config_len;
	    }

	  menu_entries[menu_len++] = 0;
	  config_entries[config_len++] = 0;
	  grub_memmove (config_entries + config_len, menu_entries, menu_len);
	  menu_entries = config_entries + config_len;

	  grub_close ();
	}

      if (! num_entries)
	{
	  /* If no acceptable config file, goto command-line, starting
	     heap from where the config entries would have been stored
	     if there were any.  */
	  enter_cmdline (config_entries, 1);
	}
      else
	{
	  /* Run menu interface.  */
	  run_menu (menu_entries, config_entries, num_entries,
		    menu_entries + menu_len, default_entry);
	}
    }
}
