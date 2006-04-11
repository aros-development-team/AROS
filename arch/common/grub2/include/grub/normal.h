/* normal.h - prototypes for the normal mode */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2002,2003,2005  Free Software Foundation, Inc.
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

#ifndef GRUB_NORMAL_HEADER
#define GRUB_NORMAL_HEADER	1

#include <grub/setjmp.h>
#include <grub/symbol.h>
#include <grub/err.h>
#include <grub/arg.h>

/* The maximum size of a command-line.  */
#define GRUB_MAX_CMDLINE	1600

/* Can be run in the command-line.  */
#define GRUB_COMMAND_FLAG_CMDLINE	0x1
/* Can be run in the menu.  */
#define GRUB_COMMAND_FLAG_MENU		0x2
/* Can be run in both interfaces.  */
#define GRUB_COMMAND_FLAG_BOTH		0x3
/* Only for the command title.  */
#define GRUB_COMMAND_FLAG_TITLE		0x4
/* Don't print the command on booting.  */
#define GRUB_COMMAND_FLAG_NO_ECHO	0x8
/* Don't print the command on booting.  */
#define GRUB_COMMAND_FLAG_NO_ARG_PARSE	0x10
/* Not loaded yet. Used for auto-loading.  */
#define GRUB_COMMAND_FLAG_NOT_LOADED	0x20

/* The type of a completion item.  */
enum grub_completion_type
  {
    GRUB_COMPLETION_TYPE_COMMAND,
    GRUB_COMPLETION_TYPE_DEVICE,
    GRUB_COMPLETION_TYPE_PARTITION,
    GRUB_COMPLETION_TYPE_FILE,
    GRUB_COMPLETION_TYPE_ARGUMENT
  };
typedef enum grub_completion_type grub_completion_type_t;

/* The command description.  */
struct grub_command
{
  /* The name.  */
  char *name;

  /* The callback function.  */
  grub_err_t (*func) (struct grub_arg_list *state, int argc, char **args);

  /* The flags.  */
  unsigned flags;

  /* The summary of the command usage.  */
  const char *summary;

  /* The description of the command.  */
  const char *description;

  /* The argument parser optionlist.  */
  const struct grub_arg_option *options;

  /* The name of a module. Used for auto-loading.  */
  char *module_name;
  
  /* The next element.  */
  struct grub_command *next;
};
typedef struct grub_command *grub_command_t;

/* The command list.  */
struct grub_command_list
{
  /* The string of a command.  */
  char *command;

  /* The next element.  */
  struct grub_command_list *next;
};
typedef struct grub_command_list *grub_command_list_t;

/* The menu entry.  */
struct grub_menu_entry
{
  /* The title name.  */
  const char *title;

  /* The number of commands.  */
  int num;

  /* The list of commands.  */
  grub_command_list_t command_list;

  /* The next element.  */
  struct grub_menu_entry *next;
};
typedef struct grub_menu_entry *grub_menu_entry_t;

/* The menu.  */
struct grub_menu
{
  /* The default entry number.  */
  int default_entry;

  /* The fallback entry number.  */
  int fallback_entry;

  /* The timeout to boot the default entry automatically.  */
  int timeout;
  
  /* The size of a menu.  */
  int size;

  /* The list of menu entries.  */
  grub_menu_entry_t entry_list;
};
typedef struct grub_menu *grub_menu_t;

/* A list of menus.  */
struct grub_menu_list
{
  grub_menu_t menu;
  struct grub_menu_list *next;
};
typedef struct grub_menu_list *grub_menu_list_t;

/* The context.  A context holds some global information.  */
struct grub_context
{
  /* The menu list.  */
  grub_menu_list_t menu_list;
};
typedef struct grub_context *grub_context_t;

/* This is used to store the names of filesystem modules for auto-loading.  */
struct grub_fs_module_list
{
  char *name;
  struct grub_fs_module_list *next;
};
typedef struct grub_fs_module_list *grub_fs_module_list_t;

/* To exit from the normal mode.  */
extern grub_jmp_buf grub_exit_env;

void grub_enter_normal_mode (const char *config);
void grub_normal_execute (const char *config, int nested);
void grub_menu_run (grub_menu_t menu, int nested);
void grub_menu_entry_run (grub_menu_entry_t entry);
void grub_cmdline_run (int nested);
int grub_cmdline_get (const char *prompt, char cmdline[], unsigned max_len,
		      int echo_char, int readline);
grub_command_t grub_register_command (const char *name,
				      grub_err_t (*func) (struct grub_arg_list *state,
							  int argc,
							  char **args),
				      unsigned flags,
				      const char *summary,
				      const char *description,
				      const struct grub_arg_option *parser);
void grub_unregister_command (const char *name);
grub_command_t grub_command_find (char *cmdline);
grub_err_t grub_set_history (int newsize);
int grub_iterate_commands (int (*iterate) (grub_command_t));
int grub_command_execute (char *cmdline, int interactive);
void grub_command_init (void);
void grub_normal_init_page (void);
void grub_menu_init_page (int nested, int edit);
int grub_arg_parse (grub_command_t parser, int argc, char **argv,
		    struct grub_arg_list *usr, char ***args, int *argnum);
void grub_arg_show_help (grub_command_t cmd);
grub_context_t grub_context_get (void);
grub_menu_t grub_context_get_current_menu (void);
grub_menu_t grub_context_push_menu (grub_menu_t menu);
void grub_context_pop_menu (void);
char *grub_normal_do_completion (char *buf, int *restore,
				 void (*hook) (const char *item, grub_completion_type_t type, int count));
grub_err_t grub_normal_print_device_info (const char *name);

#ifdef GRUB_UTIL
void grub_normal_init (void);
void grub_normal_fini (void);
void grub_hello_init (void);
void grub_hello_fini (void);
void grub_ls_init (void);
void grub_ls_fini (void);
void grub_cat_init (void);
void grub_cat_fini (void);
void grub_boot_init (void);
void grub_boot_fini (void);
void grub_cmp_init (void);
void grub_cmp_fini (void);
void grub_terminal_init (void);
void grub_terminal_fini (void);
void grub_loop_init (void);
void grub_loop_fini (void);
void grub_help_init (void);
void grub_help_fini (void);
void grub_halt_init (void);
void grub_halt_fini (void);
void grub_reboot_init (void);
void grub_reboot_fini (void);
void grub_default_init (void);
void grub_default_fini (void);
void grub_timeout_init (void);
void grub_timeout_fini (void);
void grub_configfile_init (void);
void grub_configfile_fini (void);
void grub_search_init (void);
void grub_search_fini (void);
#endif

#endif /* ! GRUB_NORMAL_HEADER */
