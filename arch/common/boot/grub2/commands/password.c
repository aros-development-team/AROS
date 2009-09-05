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

#include <grub/auth.h>
#include <grub/list.h>
#include <grub/mm.h>
#include <grub/misc.h>
#include <grub/env.h>
#include <grub/normal.h>
#include <grub/dl.h>

static grub_dl_t my_mod;

static grub_err_t
check_password (const char *user,
		void *password)
{
  char entered[1024];

  grub_memset (entered, 0, sizeof (entered));

  if (!GRUB_GET_PASSWORD (entered, sizeof (entered) - 1))
    return GRUB_ACCESS_DENIED;

  if (grub_auth_strcmp (entered, password) != 0)
    return GRUB_ACCESS_DENIED;

  grub_auth_authenticate (user);

  return GRUB_ERR_NONE;
}

static grub_err_t
grub_cmd_password (grub_command_t cmd __attribute__ ((unused)),
		   int argc, char **args)
{
  grub_err_t err;
  char *pass;

  if (argc != 2)
    return grub_error (GRUB_ERR_BAD_ARGUMENT, "Two arguments expected.");

  pass = grub_strdup (args[1]);
  if (!pass)
    return grub_errno;

  err = grub_auth_register_authentication (args[0], check_password, pass);
  if (err)
    {
      grub_free (pass);
      return err;
    }
  grub_dl_ref (my_mod);
  return GRUB_ERR_NONE;
}

static grub_command_t cmd;

GRUB_MOD_INIT(password)
{
  my_mod = mod;
  cmd = grub_register_command ("password", grub_cmd_password,
			       "password USER PASSWORD",
			       "Set user password (plaintext). "
			       "Unrecommended and insecure.");
}

GRUB_MOD_FINI(password)
{
  grub_unregister_command (cmd);
}
