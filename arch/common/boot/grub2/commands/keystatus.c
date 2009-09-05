/* keystatus.c - Command to check key modifier status.  */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2009  Free Software Foundation, Inc.
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

#include <grub/dl.h>
#include <grub/misc.h>
#include <grub/extcmd.h>
#include <grub/term.h>

static const struct grub_arg_option options[] =
  {
    {"shift", 's', 0, "check Shift key", 0, 0},
    {"ctrl", 'c', 0, "check Control key", 0, 0},
    {"alt", 'a', 0, "check Alt key", 0, 0},
    {0, 0, 0, 0, 0, 0}
  };

#define grub_cur_term_input	grub_term_get_current_input ()

static grub_err_t
grub_cmd_keystatus (grub_extcmd_t cmd,
		    int argc __attribute__ ((unused)),
		    char **args __attribute__ ((unused)))
{
  struct grub_arg_list *state = cmd->state;
  int expect_mods = 0;
  int mods;

  if (state[0].set)
    expect_mods |= GRUB_TERM_STATUS_SHIFT;
  if (state[1].set)
    expect_mods |= GRUB_TERM_STATUS_CTRL;
  if (state[2].set)
    expect_mods |= GRUB_TERM_STATUS_ALT;

  /* Without arguments, just check whether getkeystatus is supported at
     all.  */
  if (!grub_cur_term_input->getkeystatus)
    return grub_error (GRUB_ERR_TEST_FAILURE, "false");
  grub_dprintf ("keystatus", "expect_mods: %d\n", expect_mods);
  if (!expect_mods)
    return 0;

  mods = grub_getkeystatus ();
  grub_dprintf ("keystatus", "mods: %d\n", mods);
  if (mods >= 0 && (mods & expect_mods) != 0)
    return 0;
  else
    return grub_error (GRUB_ERR_TEST_FAILURE, "false");
}

static grub_extcmd_t cmd;

GRUB_MOD_INIT(keystatus)
{
  cmd = grub_register_extcmd ("keystatus", grub_cmd_keystatus,
			      GRUB_COMMAND_FLAG_BOTH,
			      "keystatus [--shift] [--ctrl] [--alt]",
			      "Check key modifier status",
			      options);
}

GRUB_MOD_FINI(keystatus)
{
  grub_unregister_extcmd (cmd);
}
