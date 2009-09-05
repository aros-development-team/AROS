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

struct grub_auth_user
{
  struct grub_auth_user *next;
  char *name;
  grub_auth_callback_t callback;
  void *arg;
  int authenticated;
};

struct grub_auth_user *users = NULL;

int
grub_auth_strcmp (const char *user_input, const char *template)
{
  int ok = 1;
  const char *ptr1, *ptr2;
  for (ptr1 = user_input, ptr2 = template; *ptr1; ptr1++)
    if (*ptr1 == (ptr2 ? *ptr2 : ptr1[1]) && ok && ptr2 != NULL)
      ptr2++;
    else
      ok = 0;

  return !ok;
}

static int
grub_iswordseparator (int c)
{
  return (grub_isspace (c) || c == ',' || c == ';' || c == '|' || c == '&');
}

int
grub_auth_strword (const char *haystack, const char *needle)
{
  const char *n_pos = needle;
  int found = 0;

  while (grub_iswordseparator (*haystack))
    haystack++;

  while (*haystack)
    {
      int ok = 1;
      /* Crawl both the needle and the haystack word we're on.  */
      while(*haystack && !grub_iswordseparator (*haystack))
        {
	  if (*haystack == *n_pos && ok)
	    n_pos++;
	  else
	    ok = 0;

          haystack++;
        }

      if (ok)
	found = 1;
    }

  return found;
}

grub_err_t
grub_auth_register_authentication (const char *user,
				   grub_auth_callback_t callback,
				   void *arg)
{
  struct grub_auth_user *cur;

  cur = grub_named_list_find (GRUB_AS_NAMED_LIST (users), user);
  if (!cur)
    cur = grub_zalloc (sizeof (*cur));
  if (!cur)
    return grub_errno;
  cur->callback = callback;
  cur->arg = arg;
  if (! cur->name)
    {
      cur->name = grub_strdup (user);
      if (!cur->name)
	{
	  grub_free (cur);
	  return grub_errno;
	}
      grub_list_push (GRUB_AS_LIST_P (&users), GRUB_AS_LIST (cur));
    }
  return GRUB_ERR_NONE;
}

grub_err_t
grub_auth_unregister_authentication (const char *user)
{
  struct grub_auth_user *cur;
  cur = grub_named_list_find (GRUB_AS_NAMED_LIST (users), user);
  if (!cur)
    return grub_error (GRUB_ERR_BAD_ARGUMENT, "user '%s' not found", user);
  if (!cur->authenticated)
    {
      grub_free (cur->name);
      grub_list_remove (GRUB_AS_LIST_P (&users), GRUB_AS_LIST (cur));
      grub_free (cur);
    }
  else
    {
      cur->callback = NULL;
      cur->arg = NULL;
    }
  return GRUB_ERR_NONE;
}

grub_err_t
grub_auth_authenticate (const char *user)
{
  struct grub_auth_user *cur;

  cur = grub_named_list_find (GRUB_AS_NAMED_LIST (users), user);
  if (!cur)
    cur = grub_zalloc (sizeof (*cur));
  if (!cur)
    return grub_errno;

  cur->authenticated = 1;

  if (! cur->name)
    {
      cur->name = grub_strdup (user);
      if (!cur->name)
	{
	  grub_free (cur);
	  return grub_errno;
	}
      grub_list_push (GRUB_AS_LIST_P (&users), GRUB_AS_LIST (cur));
    }

  return GRUB_ERR_NONE;
}

grub_err_t
grub_auth_deauthenticate (const char *user)
{
  struct grub_auth_user *cur;
  cur = grub_named_list_find (GRUB_AS_NAMED_LIST (users), user);
  if (!cur)
    return grub_error (GRUB_ERR_BAD_ARGUMENT, "user '%s' not found", user);
  if (!cur->callback)
    {
      grub_free (cur->name);
      grub_list_remove (GRUB_AS_LIST_P (&users), GRUB_AS_LIST (cur));
      grub_free (cur);
    }
  else
    cur->authenticated = 0;
  return GRUB_ERR_NONE;
}

static int
is_authenticated (const char *userlist)
{
  const char *superusers;

  auto int hook (grub_list_t item);
  int hook (grub_list_t item)
  {
    const char *name;
    if (!((struct grub_auth_user *) item)->authenticated)
      return 0;
    name = ((struct grub_auth_user *) item)->name;

    return (userlist && grub_auth_strword (userlist, name))
      || grub_auth_strword (superusers, name);
  }

  superusers = grub_env_get ("superusers");

  if (!superusers)
    return 1;

  return grub_list_iterate (GRUB_AS_LIST (users), hook);
}

grub_err_t
grub_auth_check_authentication (const char *userlist)
{
  char login[1024];
  struct grub_auth_user *cur = NULL;
  grub_err_t err;

  auto int hook (grub_list_t item);
  int hook (grub_list_t item)
  {
    if (grub_auth_strcmp (login, ((struct grub_auth_user *) item)->name) == 0)
      cur = (struct grub_auth_user *) item;
    return 0;
  }

  auto int hook_any (grub_list_t item);
  int hook_any (grub_list_t item)
  {
    if (((struct grub_auth_user *) item)->callback)
      cur = (struct grub_auth_user *) item;
    return 0;
  }

  grub_memset (login, 0, sizeof (login));

  if (is_authenticated (userlist))
    return GRUB_ERR_NONE;

  if (!grub_cmdline_get ("Enter username: ", login, sizeof (login) - 1,
			 0, 0, 0))
    return GRUB_ACCESS_DENIED;

  grub_list_iterate (GRUB_AS_LIST (users), hook);

  if (!cur || ! cur->callback)
    {
      grub_list_iterate (GRUB_AS_LIST (users), hook_any);

      /* No users present at all.  */
      if (!cur)
	return GRUB_ACCESS_DENIED;

      /* Display any of available authentication schemes.  */
      err = cur->callback (login, 0);

      return GRUB_ACCESS_DENIED;
    }
  err = cur->callback (login, cur->arg);
  if (is_authenticated (userlist))
    return GRUB_ERR_NONE;
  return GRUB_ACCESS_DENIED;
}
