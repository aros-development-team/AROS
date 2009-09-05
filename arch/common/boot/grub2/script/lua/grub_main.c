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

#include <grub/dl.h>
#include <grub/parser.h>

static const char *
scan_str (const char *s1, const char *s2)
{
  while (*s1)
    {
      const char *p = s2;

      while (*p)
	{
	  if (*s1 == *p)
	    return s1;
	  p++;
	}

      s1++;
    }

  return s1;
}

int
strcspn (const char *s1, const char *s2)
{
  const char *r;

  r = scan_str (s1, s2);
  return r - s1;
}

char *
strpbrk (const char *s1, const char *s2)
{
  const char *r;

  r = scan_str (s1, s2);
  return (*r) ? (char *) r : 0;
}

void *
memchr (const void *s, int c, size_t n)
{
  const unsigned char *p = s;

  while (n)
    {
      if (*p == c)
	return (void *) p;

      n--;
      p++;
    }

  return 0;
}

static lua_State *state;

/* Call `grub_error' to report a Lua error.  The error message string must be
   on the top of the Lua stack (at index -1).  The error message is popped off
   the Lua stack before this function returns.  */
static void
handle_lua_error (const char *error_type)
{
  const char *error_msg;
  error_msg = lua_tostring (state, -1);
  if (error_msg == NULL)
    error_msg = "(error message not a string)";
  grub_error (GRUB_ERR_BAD_ARGUMENT, "%s: %s", error_type, error_msg);
  /* Pop the error message.  */
  lua_pop (state, 1);
}

static grub_err_t
grub_lua_parse_line (char *line, grub_reader_getline_t getline)
{
  int r;
  char *old_line = 0;

  lua_settop(state, 0);
  while (1)
    {
      r = luaL_loadbuffer (state, line, grub_strlen (line), "=grub");
      if (! r)
	{
	  /* No error: Execute the statement.  */
	  r = lua_pcall (state, 0, 0, 0);
	  if (r)
	    {
	      handle_lua_error ("Lua");
	      break;
	    }
	  else
	    {
	      grub_free (old_line);
	      return grub_errno;
	    }
	}

      if (r == LUA_ERRSYNTAX)
	{
	  /* Check whether the syntax error is a result of an incomplete
	     statement.  If it is, then try to complete the statement by
	     reading more lines.  */
	  size_t lmsg;
	  const char *msg = lua_tolstring (state, -1, &lmsg);
	  const char *tp = msg + lmsg - (sizeof (LUA_QL ("<eof>")) - 1);
	  if (grub_strstr (msg, LUA_QL ("<eof>")) == tp)
	    {
	      char *n, *t;
	      int len;

	      /* Discard the error message.  */
	      lua_pop (state, 1);
	      /* Try to read another line to complete the statement.  */
	      if ((getline (&n, 1)) || (! n))
		{
		  grub_error (GRUB_ERR_BAD_ARGUMENT, "incomplete command");
		  break;
		}

	      /* More input was available: Add it to the current statement
		 contents.  */
	      len = grub_strlen (line);
	      t = grub_malloc (len + grub_strlen (n) + 2);
	      if (! t)
		break;

	      grub_strcpy (t, line);
	      t[len] = '\n';
	      grub_strcpy (t + len + 1, n);
	      grub_free (old_line);
	      line = old_line = t;
	      /* Try again to execute the statement now that more input has
		 been appended.  */
	      continue;
	    }
	  /* The syntax error was not the result of an incomplete line.  */
	  handle_lua_error ("Lua syntax error");
	}
      else
	{
	  /* Handle errors other than syntax errors (out of memory, etc.).  */
	  handle_lua_error ("Lua parser failed");
	}

      break;
    }

  grub_free (old_line);
  lua_gc (state, LUA_GCCOLLECT, 0);

  return grub_errno;
}

static struct grub_parser grub_lua_parser =
  {
    .name = "lua",
    .parse_line = grub_lua_parse_line
  };

GRUB_MOD_INIT(lua)
{
  (void) mod;

  state = lua_open ();
  if (state)
    {
      lua_gc (state, LUA_GCSTOP, 0);
      luaL_openlibs (state);
      luaL_register (state, "grub", grub_lua_lib);
      lua_gc (state, LUA_GCRESTART, 0);
      grub_parser_register ("lua", &grub_lua_parser);
    }
}

GRUB_MOD_FINI(lua)
{
  if (state)
    {
      grub_parser_unregister (&grub_lua_parser);
      lua_close (state);
    }
}
