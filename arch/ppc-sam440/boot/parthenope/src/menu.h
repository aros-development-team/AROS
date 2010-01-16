/* menu.h */

/* <project_name> -- <project_description>
 *
 * Copyright (C) 2006 - 2007
 *     Giuseppe Coviello <cjg@cruxppc.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef _MENU_H
#define _MENU_H

#include "context.h"
#include "device.h"

#define DEFAULT_TYPE  0
#define IDE_TYPE      1
#define TFTP_TYPE     2
#define CD_TYPE       3

#define MAX_MODULES 100

struct EntryObject {
	char *name;
	char *args;
	char *partition;
};

typedef struct menu {
	char *title;
	char *kernel;
	char *other;
	char *append;
	char *initrd;
	struct EntryObject *modules[MAX_MODULES];
/* 	char *modules[MAX_MODULES]; */
	int modules_cnt;
	char *argv[MAX_MODULES];
	int argc;
	int device_type;
	int device_num;
	int partition;
	int default_os;
	int delay;
	char *server_ip;
	struct menu *next;
} menu_t;

menu_t *menu_load(boot_dev_t * boot);
menu_t *menu_display(menu_t * self);
void menu_free(menu_t * self);

#endif /* _MENU_H */
