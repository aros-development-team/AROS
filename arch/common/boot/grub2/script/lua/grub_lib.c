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

#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
#include "grub_lib.h"

#include <grub/env.h>
#include <grub/parser.h>
#include <grub/command.h>
#include <grub/normal.h>
#include <grub/file.h>
#include <grub/device.h>

static int
save_errno (lua_State *state)
{
  int saved_errno;

  saved_errno = grub_errno;
  grub_errno = 0;

  lua_pushinteger (state, saved_errno);
  lua_setfield (state, LUA_GLOBALSINDEX, "grub_errno");

  if (saved_errno)
    lua_pushstring (state, grub_errmsg);
  else
    lua_pushnil (state);

  lua_setfield (state, LUA_GLOBALSINDEX, "grub_errmsg");

  return saved_errno;
}

static int
push_result (lua_State *state)
{
  lua_pushinteger (state, save_errno (state));
  return 1;
}

static int
grub_lua_run (lua_State *state)
{
  int n;
  char **args;
  const char *s;

  s = luaL_checkstring (state, 1);
  if ((! grub_parser_split_cmdline (s, 0, &n, &args))
      && (n >= 0))
    {
      grub_command_t cmd;

      cmd = grub_command_find (args[0]);
      if (cmd)
	(cmd->func) (cmd, n, &args[1]);
      else
	grub_error (GRUB_ERR_FILE_NOT_FOUND, "command not found");

      grub_free (args[0]);
      grub_free (args);
    }

  return push_result (state);
}

static int
grub_lua_getenv (lua_State *state)
{
  int n, i;

  n = lua_gettop (state);
  for (i = 1; i <= n; i++)
    {
      const char *name, *value;

      name = luaL_checkstring (state, i);
      value = grub_env_get (name);
      if (value)
	lua_pushstring (state, value);
      else
	lua_pushnil (state);
    }

  return n;
}

static int
grub_lua_setenv (lua_State *state)
{
  const char *name, *value;

  name = luaL_checkstring (state, 1);
  value = luaL_checkstring (state, 2);

  if (name[0])
    grub_env_set (name, value);

  return 0;
}

static int
grub_lua_enum_device (lua_State *state)
{
  auto int enum_device (const char *name);
  int enum_device (const char *name)
  {
    int result;
    grub_device_t dev;

    result = 0;
    dev = grub_device_open (name);
    if (dev)
      {
	grub_fs_t fs;

	fs = grub_fs_probe (dev);
	if (fs)
	  {
	    lua_pushvalue (state, 1);
	    lua_pushstring (state, name);
	    lua_pushstring (state, fs->name);
	    if (! fs->uuid)
	      lua_pushnil (state);
	    else
	      {
		int err;
		char *uuid;

		err = fs->uuid (dev, &uuid);
		if (err)
		  {
		    grub_errno = 0;
		    lua_pushnil (state);
		  }
		else
		  {
		    lua_pushstring (state, uuid);
		    grub_free (uuid);
		  }
	      }

	    lua_call (state, 3, 1);
	    result = lua_tointeger (state, -1);
	    lua_pop (state, 1);
	  }
	else
	  grub_errno = 0;
	grub_device_close (dev);
      }
    else
      grub_errno = 0;

    return result;
  }

  luaL_checktype (state, 1, LUA_TFUNCTION);
  grub_device_iterate (enum_device);
  return push_result (state);
}

static int
grub_lua_enum_file (lua_State *state)
{
  char *device_name;
  const char *arg;
  grub_device_t dev;

  auto int enum_file (const char *name, const struct grub_dirhook_info *info);
  int enum_file (const char *name, const struct grub_dirhook_info *info)
  {
    int result;

    lua_pushvalue (state, 1);
    lua_pushstring (state, name);
    lua_pushinteger (state, info->dir != 0);
    lua_call (state, 2, 1);
    result = lua_tointeger (state, -1);
    lua_pop (state, 1);

    return result;
  }

  luaL_checktype (state, 1, LUA_TFUNCTION);
  arg = luaL_checkstring (state, 2);
  device_name = grub_file_get_device_name (arg);
  dev = grub_device_open (device_name);
  if (dev)
    {
      grub_fs_t fs;
      const char *path;

      fs = grub_fs_probe (dev);
      path = grub_strchr (arg, ')');
      if (! path)
	path = arg;
      else
	path++;

      if (fs)
	{
	  (fs->dir) (dev, path, enum_file);
	}

      grub_device_close (dev);
    }

  grub_free (device_name);

  return push_result (state);
}

static int
grub_lua_file_open (lua_State *state)
{
  grub_file_t file;
  const char *name;

  name = luaL_checkstring (state, 1);
  file = grub_file_open (name);
  save_errno (state);

  if (! file)
    return 0;

  lua_pushlightuserdata (state, file);
  return 1;
}

static int
grub_lua_file_close (lua_State *state)
{
  grub_file_t file;

  luaL_checktype (state, 1, LUA_TLIGHTUSERDATA);
  file = lua_touserdata (state, 1);
  grub_file_close (file);

  return push_result (state);
}

static int
grub_lua_file_seek (lua_State *state)
{
  grub_file_t file;
  grub_off_t offset;

  luaL_checktype (state, 1, LUA_TLIGHTUSERDATA);
  file = lua_touserdata (state, 1);
  offset = luaL_checkinteger (state, 2);

  offset = grub_file_seek (file, offset);
  save_errno (state);

  lua_pushinteger (state, offset);
  return 1;
}

static int
grub_lua_file_read (lua_State *state)
{
  grub_file_t file;
  luaL_Buffer b;
  int n;

  luaL_checktype (state, 1, LUA_TLIGHTUSERDATA);
  file = lua_touserdata (state, 1);
  n = luaL_checkinteger (state, 2);

  luaL_buffinit (state, &b);
  while (n)
    {
      char *p;
      int nr;

      nr = (n > LUAL_BUFFERSIZE) ? LUAL_BUFFERSIZE : n;
      p = luaL_prepbuffer (&b);

      nr = grub_file_read (file, p, nr);
      if (nr <= 0)
	break;

      luaL_addsize (&b, nr);
      n -= nr;
    }

  save_errno (state);
  luaL_pushresult (&b);
  return 1;
}

static int
grub_lua_file_getline (lua_State *state)
{
  grub_file_t file;
  char *line;

  luaL_checktype (state, 1, LUA_TLIGHTUSERDATA);
  file = lua_touserdata (state, 1);

  line = grub_file_getline (file);
  save_errno (state);

  if (! line)
    return 0;

  lua_pushstring (state, line);
  grub_free (line);
  return 1;
}

static int
grub_lua_file_getsize (lua_State *state)
{
  grub_file_t file;

  luaL_checktype (state, 1, LUA_TLIGHTUSERDATA);
  file = lua_touserdata (state, 1);

  lua_pushinteger (state, file->size);
  return 1;
}

static int
grub_lua_file_getpos (lua_State *state)
{
  grub_file_t file;

  luaL_checktype (state, 1, LUA_TLIGHTUSERDATA);
  file = lua_touserdata (state, 1);

  lua_pushinteger (state, file->offset);
  return 1;
}

static int
grub_lua_file_eof (lua_State *state)
{
  grub_file_t file;

  luaL_checktype (state, 1, LUA_TLIGHTUSERDATA);
  file = lua_touserdata (state, 1);

  lua_pushboolean (state, file->offset >= file->size);
  return 1;
}

static int
grub_lua_file_exist (lua_State *state)
{
  grub_file_t file;
  const char *name;
  int result;

  result = 0;
  name = luaL_checkstring (state, 1);
  file = grub_file_open (name);
  if (file)
    {
      result++;
      grub_file_close (file);
    }
  else
    grub_errno = 0;

  lua_pushboolean (state, result);
  return 1;
}

static int
grub_lua_add_menu (lua_State *state)
{
  int n;
  const char *source;

  source = luaL_checklstring (state, 1, 0);
  n = lua_gettop (state) - 1;
  if (n > 0)
    {
      const char *args[sizeof (char *) * n];
      char *p;
      int i;

      for (i = 0; i < n; i++)
	args[i] = luaL_checkstring (state, 2 + i);

      p = grub_strdup (source);
      if (! p)
	return push_result (state);

      grub_normal_add_menu_entry (n, args, p);
    }
  else
    {
      lua_pushstring (state, "not enough parameter");
      lua_error (state);
    }

  return push_result (state);
}

luaL_Reg grub_lua_lib[] =
  {
    {"run", grub_lua_run},
    {"getenv", grub_lua_getenv},
    {"setenv", grub_lua_setenv},
    {"enum_device", grub_lua_enum_device},
    {"enum_file", grub_lua_enum_file},
    {"file_open", grub_lua_file_open},
    {"file_close", grub_lua_file_close},
    {"file_seek", grub_lua_file_seek},
    {"file_read", grub_lua_file_read},
    {"file_getline", grub_lua_file_getline},
    {"file_getsize", grub_lua_file_getsize},
    {"file_getpos", grub_lua_file_getpos},
    {"file_eof", grub_lua_file_eof},
    {"file_exist", grub_lua_file_exist},
    {"add_menu", grub_lua_add_menu},
    {0, 0}
  };
