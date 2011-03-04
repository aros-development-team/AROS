/* search.c - search devices based on a file or a filesystem label */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2005,2007,2008,2009  Free Software Foundation, Inc.
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

#include <grub/types.h>
#include <grub/misc.h>
#include <grub/mm.h>
#include <grub/err.h>
#include <grub/dl.h>
#include <grub/env.h>
#include <grub/extcmd.h>
#include <grub/search.h>
#include <grub/i18n.h>

static const struct grub_arg_option options[] =
  {
    {"file",		'f', 0, N_("Search devices by a file."), 0, 0},
    {"label",		'l', 0, N_("Search devices by a filesystem label."),
     0, 0},
    {"fs-uuid",		'u', 0, N_("Search devices by a filesystem UUID."),
     0, 0},
    {"set",		's', GRUB_ARG_OPTION_OPTIONAL,
     N_("Set a variable to the first device found."), "VAR", ARG_TYPE_STRING},
    {"no-floppy",	'n', 0, N_("Do not probe any floppy drive."), 0, 0},
    {"hint",	        'h', GRUB_ARG_OPTION_REPEATABLE,
     N_("First try the device HINT. If HINT ends in comma, "
	"also try subpartitions"), N_("HINT"), ARG_TYPE_STRING},
    {0, 0, 0, 0, 0, 0}
  };

enum options
  {
    SEARCH_FILE,
    SEARCH_LABEL,
    SEARCH_FS_UUID,
    SEARCH_SET,
    SEARCH_NO_FLOPPY,
    SEARCH_HINT
 };

static grub_err_t
grub_cmd_search (grub_extcmd_context_t ctxt, int argc, char **args)
{
  struct grub_arg_list *state = ctxt->state;
  const char *var = 0;
  int nhints = 0;

  if (state[SEARCH_HINT].set)
    while (state[SEARCH_HINT].args[nhints])
      nhints++;

  if (argc == 0)
    return grub_error (GRUB_ERR_BAD_ARGUMENT, "no argument specified");

  if (state[SEARCH_SET].set)
    var = state[SEARCH_SET].arg ? state[SEARCH_SET].arg : "root";

  if (state[SEARCH_LABEL].set)
    grub_search_label (args[0], var, state[SEARCH_NO_FLOPPY].set, 
		       state[SEARCH_HINT].args, nhints);
  else if (state[SEARCH_FS_UUID].set)
    grub_search_fs_uuid (args[0], var, state[SEARCH_NO_FLOPPY].set,
			 state[SEARCH_HINT].args, nhints);
  else if (state[SEARCH_FILE].set)
    grub_search_fs_file (args[0], var, state[SEARCH_NO_FLOPPY].set, 
			 state[SEARCH_HINT].args, nhints);
  else
    return grub_error (GRUB_ERR_INVALID_COMMAND, "unspecified search type");

  return grub_errno;
}

static grub_extcmd_t cmd;

GRUB_MOD_INIT(search)
{
  cmd =
    grub_register_extcmd ("search", grub_cmd_search, GRUB_COMMAND_FLAG_EXTRACTOR,
			  N_("[-f|-l|-u|-s|-n] [--hint HINT [--hint HINT] ...]"
			     " NAME"),
			  N_("Search devices by file, filesystem label"
			     " or filesystem UUID."
			     " If --set is specified, the first device found is"
			     " set to a variable. If no variable name is"
			     " specified, \"root\" is used."),
			  options);
}

GRUB_MOD_FINI(search)
{
  grub_unregister_extcmd (cmd);
}
