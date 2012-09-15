/* main.c - the normal mode main routine */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2000,2001,2002,2003,2005,2006,2007,2008,2009  Free Software Foundation, Inc.
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

#include <grub/kernel.h>
#include <grub/normal.h>
#include <grub/dl.h>
#include <grub/misc.h>
#include <grub/file.h>
#include <grub/mm.h>
#include <grub/term.h>
#include <grub/env.h>
#include <grub/parser.h>
#include <grub/reader.h>
#include <grub/menu_viewer.h>
#include <grub/auth.h>
#include <grub/i18n.h>
#include <grub/charset.h>
#include <grub/script_sh.h>

GRUB_MOD_LICENSE ("GPLv3+");

#define GRUB_DEFAULT_HISTORY_SIZE	50

static int nested_level = 0;
int grub_normal_exit_level = 0;

/* Read a line from the file FILE.  */
char *
grub_file_getline (grub_file_t file)
{
  char c;
  grub_size_t pos = 0;
  char *cmdline;
  int have_newline = 0;
  grub_size_t max_len = 64;

  /* Initially locate some space.  */
  cmdline = grub_malloc (max_len);
  if (! cmdline)
    return 0;

  while (1)
    {
      if (grub_file_read (file, &c, 1) != 1)
	break;

      /* Skip all carriage returns.  */
      if (c == '\r')
	continue;


      if (pos + 1 >= max_len)
	{
	  char *old_cmdline = cmdline;
	  max_len = max_len * 2;
	  cmdline = grub_realloc (cmdline, max_len);
	  if (! cmdline)
	    {
	      grub_free (old_cmdline);
	      return 0;
	    }
	}

      if (c == '\n')
	{
	  have_newline = 1;
	  break;
	}

      cmdline[pos++] = c;
    }

  cmdline[pos] = '\0';

  /* If the buffer is empty, don't return anything at all.  */
  if (pos == 0 && !have_newline)
    {
      grub_free (cmdline);
      cmdline = 0;
    }

  return cmdline;
}

void
grub_normal_free_menu (grub_menu_t menu)
{
  grub_menu_entry_t entry = menu->entry_list;

  while (entry)
    {
      grub_menu_entry_t next_entry = entry->next;
      grub_size_t i;

      if (entry->classes)
	{
	  struct grub_menu_entry_class *class;
	  for (class = entry->classes; class; class = class->next)
	    grub_free (class->name);
	  grub_free (entry->classes);
	}

      if (entry->args)
	{
	  for (i = 0; entry->args[i]; i++)
	    grub_free (entry->args[i]);
	  grub_free (entry->args);
	}

      grub_free ((void *) entry->id);
      grub_free ((void *) entry->users);
      grub_free ((void *) entry->title);
      grub_free ((void *) entry->sourcecode);
      entry = next_entry;
    }

  grub_free (menu);
  grub_env_unset_menu ();
}

static grub_menu_t
read_config_file (const char *config)
{
  grub_file_t file;
  const char *old_file, *old_dir;
  char *config_dir, *ptr = 0;

  auto grub_err_t getline (char **line, int cont);
  grub_err_t getline (char **line, int cont __attribute__ ((unused)))
    {
      while (1)
	{
	  char *buf;

	  *line = buf = grub_file_getline (file);
	  if (! buf)
	    return grub_errno;

	  if (buf[0] == '#')
	    grub_free (*line);
	  else
	    break;
	}

      return GRUB_ERR_NONE;
    }

  grub_menu_t newmenu;

  newmenu = grub_env_get_menu ();
  if (! newmenu)
    {
      newmenu = grub_zalloc (sizeof (*newmenu));
      if (! newmenu)
	return 0;

      grub_env_set_menu (newmenu);
    }

  /* Try to open the config file.  */
  file = grub_file_open (config);
  if (! file)
    return 0;

  old_file = grub_env_get ("config_file");
  old_dir = grub_env_get ("config_directory");
  grub_env_set ("config_file", config);
  config_dir = grub_strdup (config);
  if (config_dir)
    ptr = grub_strrchr (config_dir, '/');
  if (ptr)
    *ptr = 0;
  grub_env_set ("config_directory", config_dir);

  grub_env_export ("config_file");
  grub_env_export ("config_directory");

  while (1)
    {
      char *line;

      /* Print an error, if any.  */
      grub_print_error ();
      grub_errno = GRUB_ERR_NONE;

      if ((getline (&line, 0)) || (! line))
	break;

      grub_normal_parse_line (line, getline);
      grub_free (line);
    }

  if (old_file)
    grub_env_set ("config_file", old_file);
  else
    grub_env_unset ("config_file");
  if (old_dir)
    grub_env_set ("config_directory", old_dir);
  else
    grub_env_unset ("config_directory");

  grub_file_close (file);

  return newmenu;
}

/* Initialize the screen.  */
void
grub_normal_init_page (struct grub_term_output *term)
{
  grub_ssize_t msg_len;
  int posx;
  const char *msg = _("GNU GRUB  version %s");
  char *msg_formatted;
  grub_uint32_t *unicode_msg;
  grub_uint32_t *last_position;
 
  grub_term_cls (term);

  msg_formatted = grub_xasprintf (msg, PACKAGE_VERSION);
  if (!msg_formatted)
    return;
 
  msg_len = grub_utf8_to_ucs4_alloc (msg_formatted,
  				     &unicode_msg, &last_position);
  grub_free (msg_formatted);
 
  if (msg_len < 0)
    {
      return;
    }

  posx = grub_getstringwidth (unicode_msg, last_position, term);
  posx = (grub_term_width (term) - posx) / 2;
  grub_term_gotoxy (term, posx, 1);

  grub_print_ucs4 (unicode_msg, last_position, 0, 0, term);
  grub_putcode ('\n', term);
  grub_putcode ('\n', term);
  grub_free (unicode_msg);
}

static void
read_lists (const char *val)
{
  if (! grub_no_autoload)
    {
      read_command_list (val);
      read_fs_list (val);
      read_crypto_list (val);
      read_terminal_list (val);
    }
  grub_gettext_reread_prefix (val);
}

static char *
read_lists_hook (struct grub_env_var *var __attribute__ ((unused)),
		 const char *val)
{
  read_lists (val);
  return val ? grub_strdup (val) : NULL;
}

/* Read the config file CONFIG and execute the menu interface or
   the command line interface if BATCH is false.  */
void
grub_normal_execute (const char *config, int nested, int batch)
{
  grub_menu_t menu = 0;
  const char *prefix;

  if (! nested)
    {
      prefix = grub_env_get ("prefix");
      read_lists (prefix);
      grub_register_variable_hook ("prefix", NULL, read_lists_hook);
    }

  if (config)
    {
      menu = read_config_file (config);

      /* Ignore any error.  */
      grub_errno = GRUB_ERR_NONE;
    }

  if (! batch)
    {
      if (menu && menu->size)
	{
	  grub_show_menu (menu, nested, 0);
	  if (nested)
	    grub_normal_free_menu (menu);
	}
    }
}

/* This starts the normal mode.  */
void
grub_enter_normal_mode (const char *config)
{
  nested_level++;
  grub_normal_execute (config, 0, 0);
  grub_cmdline_run (0);
  nested_level--;
  if (grub_normal_exit_level)
    grub_normal_exit_level--;
}

/* Enter normal mode from rescue mode.  */
static grub_err_t
grub_cmd_normal (struct grub_command *cmd __attribute__ ((unused)),
		 int argc, char *argv[])
{
  if (argc == 0)
    {
      /* Guess the config filename. It is necessary to make CONFIG static,
	 so that it won't get broken by longjmp.  */
      char *config;
      const char *prefix;

      prefix = grub_env_get ("prefix");
      if (prefix)
	{
	  config = grub_xasprintf ("%s/grub.cfg", prefix);
	  if (! config)
	    goto quit;

	  grub_enter_normal_mode (config);
	  grub_free (config);
	}
      else
	grub_enter_normal_mode (0);
    }
  else
    grub_enter_normal_mode (argv[0]);

quit:
  return 0;
}

/* Exit from normal mode to rescue mode.  */
static grub_err_t
grub_cmd_normal_exit (struct grub_command *cmd __attribute__ ((unused)),
		      int argc __attribute__ ((unused)),
		      char *argv[] __attribute__ ((unused)))
{
  if (nested_level <= grub_normal_exit_level)
    return grub_error (GRUB_ERR_BAD_ARGUMENT, "not in normal environment");
  grub_normal_exit_level++;
  return GRUB_ERR_NONE;
}

static grub_err_t
grub_normal_reader_init (int nested)
{
  struct grub_term_output *term;
  const char *msg = _("Minimal BASH-like line editing is supported. For "
		      "the first word, TAB lists possible command completions. Anywhere "
		      "else TAB lists possible device or file completions. %s");
  const char *msg_esc = _("ESC at any time exits.");
  char *msg_formatted;

  msg_formatted = grub_xasprintf (msg, nested ? msg_esc : "");
  if (!msg_formatted)
    return grub_errno;

  FOR_ACTIVE_TERM_OUTPUTS(term)
  {
    grub_normal_init_page (term);
    grub_term_setcursor (term, 1);

    grub_print_message_indented (msg_formatted, 3, STANDARD_MARGIN, term);
    grub_putcode ('\n', term);
    grub_putcode ('\n', term);
  }
  grub_free (msg_formatted);
 
  return 0;
}

static grub_err_t
grub_normal_read_line_real (char **line, int cont, int nested)
{
  const char *prompt;

  if (cont)
    /* TRANSLATORS: it's command line prompt.  */
    prompt = _(">");
  else
    /* TRANSLATORS: it's command line prompt.  */
    prompt = _("grub>");

  if (!prompt)
    return grub_errno;

  while (1)
    {
      *line = grub_cmdline_get (prompt);
      if (*line)
	return 0;

      if (cont || nested)
	{
	  grub_free (*line);
	  *line = 0;
	  return grub_errno;
	}
    }
 
}

static grub_err_t
grub_normal_read_line (char **line, int cont)
{
  return grub_normal_read_line_real (line, cont, 0);
}

void
grub_cmdline_run (int nested)
{
  grub_err_t err = GRUB_ERR_NONE;

  err = grub_auth_check_authentication (NULL);

  if (err)
    {
      grub_print_error ();
      grub_errno = GRUB_ERR_NONE;
      return;
    }

  grub_normal_reader_init (nested);

  while (1)
    {
      char *line;

      if (grub_normal_exit_level)
	break;

      /* Print an error, if any.  */
      grub_print_error ();
      grub_errno = GRUB_ERR_NONE;

      grub_normal_read_line_real (&line, 0, nested);
      if (! line)
	break;

      grub_normal_parse_line (line, grub_normal_read_line);
      grub_free (line);
    }
}

static char *
grub_env_write_pager (struct grub_env_var *var __attribute__ ((unused)),
		      const char *val)
{
  grub_set_more ((*val == '1'));
  return grub_strdup (val);
}

/* clear */
static grub_err_t
grub_mini_cmd_clear (struct grub_command *cmd __attribute__ ((unused)),
		   int argc __attribute__ ((unused)),
		   char *argv[] __attribute__ ((unused)))
{
  grub_cls ();
  return 0;
}

static grub_command_t cmd_clear;

static void (*grub_xputs_saved) (const char *str);
static const char *features[] = {
  "feature_chainloader_bpb", "feature_ntldr", "feature_platform_search_hint",
  "feature_default_font_path", "feature_all_video_module",
  "feature_menuentry_id", "feature_menuentry_options", "feature_200_final"
};

GRUB_MOD_INIT(normal)
{
  unsigned i;

  /* Previously many modules depended on gzio. Be nice to user and load it.  */
  grub_dl_load ("gzio");
  grub_errno = 0;

  grub_normal_auth_init ();
  grub_context_init ();
  grub_script_init ();
  grub_menu_init ();

  grub_xputs_saved = grub_xputs;
  grub_xputs = grub_xputs_normal;

  /* Normal mode shouldn't be unloaded.  */
  if (mod)
    grub_dl_ref (mod);

  cmd_clear =
    grub_register_command ("clear", grub_mini_cmd_clear,
			   0, N_("Clear the screen."));

  grub_set_history (GRUB_DEFAULT_HISTORY_SIZE);

  grub_register_variable_hook ("pager", 0, grub_env_write_pager);
  grub_env_export ("pager");

  /* Register a command "normal" for the rescue mode.  */
  grub_register_command ("normal", grub_cmd_normal,
			 0, N_("Enter normal mode."));
  grub_register_command ("normal_exit", grub_cmd_normal_exit,
			 0, N_("Exit from normal mode."));

  /* Reload terminal colors when these variables are written to.  */
  grub_register_variable_hook ("color_normal", NULL, grub_env_write_color_normal);
  grub_register_variable_hook ("color_highlight", NULL, grub_env_write_color_highlight);

  /* Preserve hooks after context changes.  */
  grub_env_export ("color_normal");
  grub_env_export ("color_highlight");

  /* Set default color names.  */
  grub_env_set ("color_normal", "white/black");
  grub_env_set ("color_highlight", "black/white");

  for (i = 0; i < ARRAY_SIZE (features); i++)
    {
      grub_env_set (features[i], "y");
      grub_env_export (features[i]);
    }
  grub_env_set ("grub_cpu", GRUB_TARGET_CPU);
  grub_env_export ("grub_cpu");
  grub_env_set ("grub_platform", GRUB_PLATFORM);
  grub_env_export ("grub_platform");
}

GRUB_MOD_FINI(normal)
{
  grub_context_fini ();
  grub_script_fini ();
  grub_menu_fini ();
  grub_normal_auth_fini ();

  grub_xputs = grub_xputs_saved;

  grub_set_history (0);
  grub_register_variable_hook ("pager", 0, 0);
  grub_fs_autoload_hook = 0;
  grub_unregister_command (cmd_clear);
}
