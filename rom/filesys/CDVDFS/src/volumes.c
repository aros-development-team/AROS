/* volumes.c:
 *
 * Volume management.
 *
 * ----------------------------------------------------------------------
 * This code is (C) Copyright 1993,1994 by Frank Munkert.
 *              (C) Copyright 2002-2010 The AROS Development Team
 * All rights reserved.
 * This software may be freely distributed and redistributed for
 * non-commercial purposes, provided this notice is included.
 * ----------------------------------------------------------------------
 * History:
 * 
 * 11-Aug-10 sonic     - Fixed for 64-bit compatibility
 * 27-Aug-07 sonic   - Register_Volume_Node() now takes separate pointer to a volume name.
 * 19-Sep-94   fmu   Fixed bug in Reinstall_Locks()
 * 22-May-94   fmu   Performance improvement for lock+filehandle processing.
 * 09-Jul-02 sheutlin  various changes when porting to AROS
 *                     - global variables are now in a struct Globals *global
 */

#include <proto/exec.h>
#include <proto/utility.h>
#include <stdlib.h>
#include <string.h>

#include "volumes.h"
#include "cdrom.h"
#include "device.h"
#include "devsupp.h"
#include "generic.h"
#include "debug.h"
#include "path.h"
#include "globals.h"

#include "aros_stuff.h"
#include "clib_stuff.h"

/*  Associate p_lock with the pathname of the locked object on the current
 *  volume.
 */

void Register_Lock (LOCK *p_lock)
{
  CDROM_OBJ *obj_p = (CDROM_OBJ *)p_lock->fl_Link;
  struct CDVDBase *global = obj_p->global;
  t_lock_node *new;
  BUG(char pathname[300];)

  BUG
  (
    if (!Full_Path_Name (obj_p, pathname, sizeof (pathname))) {
      dbprintf (global, "[Cannot install lock / cannot determine pathname]");
      return;
    }
  );

  new = (t_lock_node*) AllocMem (sizeof (t_lock_node), MEMF_PUBLIC);
  if (!new) {
    BUG(dbprintf (global, "[Cannot install lock on '%s']", pathname);)
    return;
  }

  new->pathlist = Copy_Path_List (obj_p->pathlist, FALSE);

  new->vol_name = (char*) AllocMem (strlen (global->g_vol_name+1) + 1,
  				    MEMF_PUBLIC);
  if (!new->vol_name) {
    BUG(dbprintf ("[Cannot install lock on '%s']", pathname);)
    FreeMem (new, sizeof (t_lock_node));
    return;
  }
  strcpy (new->vol_name, global->g_vol_name+1);

  new->lock = p_lock;
  new->next = global->g_lock_list;
  global->g_lock_list = new;
  
  BUG(dbprintf ("[CDVDFS]\tInstalling lock on '%s'", pathname);)
}

/*  Remove the entry for p_lock in the list g_lock_list.
 */

void Unregister_Lock (LOCK *p_lock)
{
  CDROM_OBJ *obj_p = (CDROM_OBJ *)p_lock->fl_Link;
  struct CDVDBase *global = obj_p->global;
  t_lock_node *ptr, *old;
  BUG(char pathname[300];)

  for (ptr=global->g_lock_list, old = NULL; ptr; old = ptr, ptr = ptr->next)
    if (ptr->lock == p_lock) {
      BUG
      (
        if (!Path_Name_From_Path_List (ptr->pathlist, pathname,
                                       sizeof (pathname))) {
          dbprintf (global, "[cannot determine pathname]");
          return;
        }
        dbprintf (global, "[Removing lock from '%s']", pathname);
      );
      if (old)
        old->next = ptr->next;
      else
        global->g_lock_list = ptr->next;
      Free_Path_List (ptr->pathlist);
      FreeMem (ptr->vol_name, strlen (ptr->vol_name) + 1);
      FreeMem (ptr, sizeof (t_lock_node));
      return;
    }
  BUG(dbprintf (global, "[Lock cannot be removed %p]", p_lock);)
}

/*  Update the fl_Link values for all locks that have been
 *  registered for a volume with the name g_vol_name.
 *  The fl_Link value is a pointer to a CDROM_OBJ object which
 *  respresents the locked file or directory.
 *
 *  Returns the number of locks on the volume.
 */

int Reinstall_Locks (struct CDVDBase *global)
{
  t_lock_node *ptr;
  CDROM_OBJ* obj;
  int result = 0;
  char pathname[300];

  for (ptr=global->g_lock_list; ptr; ptr = ptr->next) {
    if (strcmp (global->g_vol_name+1, ptr->vol_name) == 0) {
      result++;
      if (!Path_Name_From_Path_List (ptr->pathlist, pathname, sizeof (pathname))) {
        BUG(dbprintf (global, "[cannot determine pathname]");)
        break;
      }
      BUG(dbprintf (global, "[Reinstalling lock on '%s'", pathname);)
      obj = Open_Object (global->g_top_level_obj, pathname);
      if (obj) {
        BUG(dbprintf (global, "]\n");)
      } else {
        BUG(dbprintf (global, "(FAILED) ]\n");)
	continue;
      }
      ptr->lock->fl_Link = (BPTR)obj;
    }
  }
  return result;
}

/*  Associate p_obj with the pathname of the associated disk object on the current
 *  volume.
 */

void Register_File_Handle(CDROM_OBJ *p_obj)
{
struct CDVDBase *global = p_obj->global;
t_fh_node *new;

	new = (t_fh_node*) AllocMem (sizeof (t_fh_node), MEMF_PUBLIC);
	if (!new)
	{
		return;
	}

	new->vol_name = (char*) AllocMem (StrLen (global->g_vol_name+1) + 1, MEMF_PUBLIC);
	if (!new->vol_name)
	{
		FreeMem (new, sizeof (t_fh_node));
		return;
	}
	StrCpy (new->vol_name, global->g_vol_name+1);

	new->obj = p_obj;
	new->devlist = global->DevList;
	new->next = global->g_fh_list;
	global->g_fh_list = new;

}

/*  Remove the entry for the file handle p_obj in the list g_fh_list.
 */

void Unregister_File_Handle(CDROM_OBJ *p_obj) {
t_fh_node *ptr, *old;
struct CDVDBase *global = p_obj->global;
  
	for (ptr=global->g_fh_list, old = NULL; ptr; old = ptr, ptr = ptr->next)
		if (ptr->obj == p_obj && StrCmp (global->g_vol_name+1, ptr->vol_name) == 0)
		{
			if (old)
				old->next = ptr->next;
			else
				global->g_fh_list = ptr->next;
			FreeMem (ptr->vol_name, StrLen (ptr->vol_name) + 1);
			FreeMem (ptr, sizeof (t_fh_node));
			return;
		}
}

/*  Update the volume pointer for all CDROM_OBJs which are used as
 *  filehandles for the current volume.
 *
 *  Returns the number of file handles on the volume.
 */

int Reinstall_File_Handles (struct CDVDBase *global)
{
  t_fh_node *ptr;
  int result = 0;

  for (ptr=global->g_fh_list; ptr; ptr = ptr->next) {
    if (StrCmp (global->g_vol_name+1, ptr->vol_name) == 0) {
      result++;
      ptr->obj->volume = global->g_volume;
    }
  }
  return result;
}

struct DeviceList *Find_Dev_List (CDROM_OBJ *p_obj) {
struct CDVDBase *global = p_obj->global;
t_fh_node *ptr;

	for (ptr=global->g_fh_list; ptr; ptr = ptr->next)
	{
		if (ptr->obj == p_obj)
		{
			return ptr->devlist;
		}
	}
	return NULL;
}

/*  Register a volume node as owned by this handler.
 */

void Register_Volume_Node(struct CDVDBase *global, struct DeviceList *p_volume, char *Name) {
t_vol_reg_node *new;
int len;
  
	new = (t_vol_reg_node*) AllocMem (sizeof (t_vol_reg_node), MEMF_PUBLIC);
	if (!new)
		return;

	new->volume = p_volume;
	len = strlen(Name) + 1;
	new->name = (char*) AllocMem (len, MEMF_PUBLIC);
	if (!new)
	{
		FreeMem (new, sizeof (t_vol_reg_node));
		return;
	}
	CopyMem(Name, new->name, len);
	new->next = global->g_volume_list;
	global->g_volume_list = new;
}

/*  Remove the registration for the volume node.
 */

void Unregister_Volume_Node(struct CDVDBase *global, struct DeviceList *p_volume) {
t_vol_reg_node *ptr, *old;

	for (ptr=global->g_volume_list, old=NULL; ptr; old=ptr, ptr=ptr->next)
	{
		if (p_volume == ptr->volume)
		{
			if (old)
				old->next = ptr->next;
			else
				global->g_volume_list = ptr->next;
			FreeMem (ptr->name, StrLen (ptr->name) + 1);
			FreeMem (ptr, sizeof (t_vol_reg_node));
			return;
		}
	}
}

/*  Find a volume node with a matching name.
 */

struct DeviceList *Find_Volume_Node(struct CDVDBase *global, char *p_name) {
t_vol_reg_node *ptr;

	for (ptr=global->g_volume_list; ptr; ptr=ptr->next)
	{
		if (Stricmp (ptr->name, p_name) == 0)
			return ptr->volume;
	}
	return NULL;
}
