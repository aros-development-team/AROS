/* test.c -- The test command..  */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2005,2007  Free Software Foundation, Inc.
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
#include <grub/dl.h>
#include <grub/arg.h>
#include <grub/misc.h>
#include <grub/mm.h>
#include <grub/env.h>

static grub_err_t
grub_cmd_test (struct grub_arg_list *state __attribute__ ((unused)), int argc,
	       char **args)

{
  char *eq;
  char *eqis;

  /* XXX: No fancy expression evaluation yet.  */
  
  if (argc == 0)
    return 0;
  
  eq = grub_strdup (args[0]);
  eqis = grub_strchr (eq, '=');
  if (! eqis)
    return 0;

  *eqis = '\0';
  eqis++;
  /* Check an expression in the form `A=B'.  */
  if (grub_strcmp (eq, eqis))
    grub_error (GRUB_ERR_TEST_FAILURE, "false");
  grub_free (eq);

  return grub_errno;
}



GRUB_MOD_INIT(test)
{
  (void)mod;			/* To stop warning. */
  grub_register_command ("[", grub_cmd_test, GRUB_COMMAND_FLAG_CMDLINE,
			 "[ EXPRESSION ]", "Evaluate an expression", 0);
  grub_register_command ("test", grub_cmd_test, GRUB_COMMAND_FLAG_CMDLINE,
			 "test EXPRESSION", "Evaluate an expression", 0);
}

GRUB_MOD_FINI(test)
{
  grub_unregister_command ("[");
  grub_unregister_command ("test");
}
