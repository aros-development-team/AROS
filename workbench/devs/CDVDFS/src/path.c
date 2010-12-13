/* path.c:
 *
 * Path lists.
 *
 * ----------------------------------------------------------------------
 * This code is (C) Copyright 1993,1994 by Frank Munkert.
 * All rights reserved.
 * This software may be freely distributed and redistributed for
 * non-commercial purposes, provided this notice is included.
 * ----------------------------------------------------------------------
 * History:
 *
 * 07-Jul-02 sheutlin  various changes when porting to AROS
 *                     - global variables are now in a struct Globals *global
 *                     - moved structure to path.h
 */

#include <proto/exec.h>
#include <exec/memory.h>
#include <stdlib.h>
#include <string.h>

#include "generic.h"
#include "path.h"
#include "globals.h"

#include "clib_stuff.h"

extern struct Globals *global;

#ifdef SysBase
#	undef SysBase
#endif
#define SysBase global->SysBase

/* Append p_name to path list: */

t_path_list Append_Path_List(t_path_list p_list, char *p_name) {
t_path_node *node;

	if (!(node = AllocMem (sizeof (*node), MEMF_PUBLIC)))
		return NULL;

	node->references = 1;
	if (!(node->name = AllocMem (StrLen (p_name) + 1, MEMF_PUBLIC)))
	{
		FreeMem (node, sizeof (*node));
		return NULL;
	}

	StrCpy (node->name, p_name);
	node->next = Copy_Path_List (p_list, FALSE);
  
	return node;
}

t_path_list Copy_Path_List (t_path_list p_src, int p_strip)
{
  t_path_node *node, *start;

  if (!p_src)
    return NULL;

  start = p_strip ? p_src->next : p_src;

  for (node = start; node; node = node->next) {
    node->references++;
  }
  
  return start;
}

void Free_Path_List(t_path_list p_list) {
t_path_node *node, *next;

	if (!p_list)
		return;

	for (node = p_list; node; node = next)
	{
		next = node->next;
		if (--node->references == 0)
		{
			FreeMem (node->name, StrLen (node->name) + 1);
			FreeMem (node, sizeof (*node));
		}
	}
}

t_bool Path_Name_From_Path_List
	(t_path_list p_list, char *p_buffer, int p_buffer_length)
{
int len;
t_path_node *node;

	if (!p_list)
	{
		StrCpy (p_buffer, ":");
		return TRUE;
	}
  
	/* calculate length: */
	for (len=1, node=p_list; node; node = node->next)
		len += StrLen (node->name) + 1;

	if (len > p_buffer_length)
		return FALSE;

	for (node=p_list; node; node = node->next)
	{
		CopyMem
			(
            node->name,
				p_buffer + len - StrLen (node->name) - 1,
				StrLen (node->name)
			);
		p_buffer[len-1] = (node == p_list) ? 0 : '/';
		len -= StrLen (node->name) + 1;
	}
	p_buffer[0] = ':';
	return TRUE;
}
