/* memrw.c - command to read / write physical memory  */
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
#include <grub/command.h>

static grub_command_t cmd_read_byte, cmd_read_word, cmd_read_dword;
static grub_command_t cmd_write_byte, cmd_write_word, cmd_write_dword;

static grub_err_t
grub_cmd_read (grub_command_t cmd, int argc, char **argv)
{
  grub_target_addr_t addr;
  grub_uint32_t value;

  if (argc != 1)
    return grub_error (GRUB_ERR_BAD_ARGUMENT, "Invalid number of arguments");

  addr = grub_strtoul (argv[0], 0, 0);
  if (cmd->name[5] == 'd')
    value = *((grub_uint32_t *) addr);
  else if (cmd->name[5] == 'w')
    value = *((grub_uint16_t *) addr);
  else
    value = *((grub_uint8_t *) addr);

  grub_printf ("0x%x\n", value);

  return 0;
}

static grub_err_t
grub_cmd_write (grub_command_t cmd, int argc, char **argv)
{
  grub_target_addr_t addr;
  grub_uint32_t value;

  if (argc != 2)
    return grub_error (GRUB_ERR_BAD_ARGUMENT, "Invalid number of arguments");

  addr = grub_strtoul (argv[0], 0, 0);
  value = grub_strtoul (argv[1], 0, 0);
  if (cmd->name[6] == 'd')
    *((grub_uint32_t *) addr) = value;
  else if (cmd->name[6] == 'w')
    *((grub_uint16_t *) addr) = (grub_uint16_t) value;
  else
    *((grub_uint8_t *) addr) = (grub_uint8_t) value;

  return 0;
}

GRUB_MOD_INIT(memrw)
{
  cmd_read_byte =
    grub_register_command ("read_byte", grub_cmd_read,
			   "read_byte ADDR", "read byte.");
  cmd_read_word =
    grub_register_command ("read_word", grub_cmd_read,
			   "read_word ADDR", "read word.");
  cmd_read_dword =
    grub_register_command ("read_dword", grub_cmd_read,
			   "read_dword ADDR", "read dword.");
  cmd_write_byte =
    grub_register_command ("write_byte", grub_cmd_write,
			   "write_byte ADDR VALUE", "write byte.");
  cmd_write_word =
    grub_register_command ("write_word", grub_cmd_write,
			   "write_word ADDR VALUE", "write word.");
  cmd_write_dword =
    grub_register_command ("write_dword", grub_cmd_write,
			   "write_dword ADDR VALUE", "write dword.");
}

GRUB_MOD_FINI(memrw)
{
  grub_unregister_command (cmd_read_byte);
  grub_unregister_command (cmd_read_word);
  grub_unregister_command (cmd_read_dword);
  grub_unregister_command (cmd_write_byte);
  grub_unregister_command (cmd_write_word);
  grub_unregister_command (cmd_write_dword);
}
