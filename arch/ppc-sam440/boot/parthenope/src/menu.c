/* menu.c */

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

#include "menu.h"
#include "ext2.h"
#include "sfs.h"
#include "rdb.h"

static struct EntryObject *EntryObject_new(char *name, char *args,
					   char *partition)
{
	struct EntryObject *self;

	self = malloc(sizeof(struct EntryObject));
	self->name = strdup(name);
	self->args = NULL;
	if(args != NULL)
		self->args = strdup(args);
	self->partition = NULL;
	if(partition != NULL)
		self->partition = strdup(partition);

	return self;
}

static menu_t *entry_alloc(void)
{
	menu_t *entry;
	entry = malloc(sizeof(menu_t));
	entry->title = NULL;
	entry->kernel = NULL;
	entry->other = NULL;
	entry->append = NULL;
	entry->initrd = NULL;
	entry->modules_cnt = 0;
	entry->argc = 0;
	entry->device_type = DEFAULT_TYPE;
	entry->device_num = 0;
	entry->partition = 0;
	entry->server_ip = 0;
	entry->next = NULL;
	return entry;
}

static void entry_free(menu_t * entry)
{
	if (entry->title != NULL)
		free(entry->title);
	if (entry->kernel != NULL)
		free(entry->kernel);
	if (entry->other != NULL)
		free(entry->other);
	if (entry->append != NULL)
		free(entry->append);
	if (entry->initrd != NULL)
		free(entry->initrd);
	/* FIXME: free modules and args */
	free(entry);
}

enum states { S, DEF, DLY, TIT, IN, RT, K, O, RD, M, A, R, AQ, RQ };

char *getline(char *src, int src_len, int *src_offset)
{
	char *nlptr, *line;
	int len;

	if (*src_offset >= src_len)
		return NULL;

	nlptr = strchr(src + *src_offset, '\n');
	if (nlptr != NULL)
		len = nlptr - (src + *src_offset) + 1;
	else
		len = src_len - *src_offset;

	line = malloc(len + 1);
	strncpy(line, src + *src_offset, len);
	*src_offset += len;
	line[len] = 0;

	return line;
}

char *eatcomments(char *line)
{
	char *cptr;

	cptr = strchr(line, '#');

	if (cptr != NULL)
		line[cptr - line] = 0;

	return line;
}

char *getnextword(char *line)
{
	char *bptr, *word;
	int len;

	bptr = strchr(line, ' ');

	if (bptr == NULL)
		bptr = strchr(line, '\t');

	if (bptr != NULL)
		len = bptr - line;
	else
		len = strlen(line);

	word = malloc(len + 1);
	strncpy(word, line, len);
	word[len] = 0;

	return word;
}

char *strtrim(char *s)
{
	char *ptr_s, *tmp;
	int n;

	ptr_s = s;
	for (; isspace(*ptr_s); ptr_s++) ;

	for (n = strlen(ptr_s); n > 0 && isspace(*(ptr_s + n - 1)); n--) ;
	if (n < 0)
		n = 0;

	tmp = strdup(ptr_s);
	strcpy(s, tmp);
	*(s + n) = 0;
	free(tmp);

	return s;
}

char *kick_eatcomments(char *line)
{
	char *cptr;

	cptr = strchr(line, ';');

	if (cptr != NULL)
		line[cptr - line] = 0;

	return line;
}

static int kick_fsm(char *buffer, int buffer_len, menu_t * menu)
{
	int offset;
	char *line;
	char *word;
	char *lineptr = NULL;
	int status, n;
	menu_t /**menu,*/  * entry = NULL, *ptr = NULL;
	/*
	   menu = entry_alloc();
	   menu->default_os = 0;
	   menu->delay = 5;
	 */
	line = NULL;
	offset = 0;
	status = S;
	n = 0;
	int parse = 1, kernel = 0;
	while (parse) {
		switch (status) {
		case S:
			if ((line = getline(buffer, buffer_len,
					    &offset)) == NULL) {
				parse = 0;
				break;
			}
			kick_eatcomments(line);
			strtrim(line);
			if (strlen(line) == 0) {
				free(line);
				status = S;
				break;
			}
			word = getnextword(line);
			if (strcmp(word, "LABEL") == 0) {
				status = TIT;
				lineptr = line + strlen(word);
			} else
				status = S;
			free(word);
			break;
		case TIT:
			strtrim(lineptr);
			if (strlen(lineptr) > 0) {
				status = IN;
				kernel = 0;
				entry = entry_alloc();
				entry->title = strdup(lineptr);
			} else
				status = S;
			free(line);
			break;
		case IN:
			if ((line = getline(buffer, buffer_len,
					    &offset)) == NULL) {
				if (kernel)
					status = AQ;
				else
					status = RQ;
				break;
			}
			kick_eatcomments(line);
			strtrim(line);
			if (strlen(line) == 0) {
				free(line);
				status = IN;
				break;
			}
			word = getnextword(line);
			lineptr = line + strlen(word);
			if (strcmp(word, "EXEC") == 0) {
				status = K;
			} else if (strcmp(word, "MODULE") == 0) {
				status = M;
			} else if (strcmp(word, "LABEL") == 0) {
				offset -= strlen(line) + 1;
				free(line);
				if (kernel) {
					status = A;
				} else {
					status = R;
				}
			} else {
				free(line);
				status = R;
			}
			free(word);
			break;
		case K:
			strtrim(lineptr);
			word = getnextword(lineptr);
			if (!strlen(word))
				status = R;
			else {
				status = IN;
				kernel = 1;
				entry->kernel = strdup(word);
				lineptr = lineptr + strlen(word);
				strtrim(lineptr);
				if (strlen(lineptr) > 0) {
					entry->append = strdup(lineptr);
				}
			}
			free(line);
			free(word);
			break;
		case M:
			strtrim(lineptr);
			word = getnextword(lineptr);
			if (!strlen(word))
				status = R;
			else {
				status = IN;
				entry->modules[entry->modules_cnt++] =
					EntryObject_new(word, NULL, NULL);
			}
			free(line);
			free(word);
			break;
		case A:
			for (ptr = menu; ptr->next != NULL; ptr = ptr->next) ;
			ptr->next = entry;
			n++;
			status = S;
			break;
		case R:
			entry_free(entry);
			status = S;
			break;
		case AQ:
			for (ptr = menu; ptr->next != NULL; ptr = ptr->next) ;
			ptr->next = entry;
			return 1;
			break;
		case RQ:
		default:
			entry_free(entry);
			return 0;
			break;
		}
	}
	return n;
}

static int fsm(char *buffer, int buffer_len, menu_t * menu)
{
	int offset;
	char *line;
	char *word;
	char *lineptr = NULL;
	int status, n;
	menu_t /**menu,*/  * entry = NULL, *ptr = NULL;

	/*
	   menu = entry_alloc();
	   menu->default_os = 0;
	   menu->delay = 5;
	 */
	line = NULL;
	offset = 0;
	status = S;
	n = 0;
	int parse = 1, kernel = 0;
	while (parse) {
		switch (status) {
		case S:
			if ((line = getline(buffer, buffer_len,
					    &offset)) == NULL) {
				parse = 0;
				break;
			}
			eatcomments(line);
			strtrim(line);
			if (strlen(line) == 0) {
				free(line);
				status = S;
				break;
			}
			word = getnextword(line);
			if (strcmp(word, "title") == 0) {
				status = TIT;
				lineptr = line + strlen(word);
			} else if (strcmp(word, "default") == 0) {
				status = DEF;
				lineptr = line + strlen(word);
			} else if (strcmp(word, "delay") == 0) {
				status = DLY;
				lineptr = line + strlen(word);
			} else
				status = S;
			free(word);
			break;
		case DEF:
			status = S;
			if (strlen(lineptr) == 0)
				break;
			strtrim(lineptr);
			word = getnextword(lineptr);
			if (strlen(word) == 0)
				break;
			menu->default_os = strtol(word);
			free(word);
			break;
		case DLY:
			status = S;
			if (strlen(lineptr) == 0)
				break;
			strtrim(lineptr);
			word = getnextword(lineptr);
			if (strlen(word) == 0)
				break;
			menu->delay = strtol(word);
			free(word);
			break;
		case TIT:
			strtrim(lineptr);
			if (strlen(lineptr) > 0) {
				status = IN;
				kernel = 0;
				entry = entry_alloc();
				entry->title = strdup(lineptr);
			} else
				status = S;
			free(line);
			break;
		case IN:
			if ((line = getline(buffer, buffer_len,
					    &offset)) == NULL) {
				if (kernel)
					status = AQ;
				else
					status = RQ;
				break;
			}
			eatcomments(line);
			strtrim(line);
			if (strlen(line) == 0) {
				free(line);
				status = IN;
				break;
			}
			word = getnextword(line);
			lineptr = line + strlen(word);
			if (strcmp(word, "kernel") == 0) {
				status = K;
			} else if (strcmp(word, "other") == 0) {
				status = O;
			} else if (strcmp(word, "root") == 0) {
				status = RT;
			} else if (strcmp(word, "initrd") == 0) {
				status = RD;
			} else if (strcmp(word, "module") == 0) {
				status = M;
			} else if (strcmp(word, "title") == 0) {
				offset -= strlen(line) + 1;
				free(line);
				if (kernel) {
					status = A;
				} else {
					status = R;
				}
			} else {
				free(line);
				status = R;
			}
			free(word);
			break;
		case K:
			strtrim(lineptr);
			word = getnextword(lineptr);
			if (!strlen(word))
				status = R;
			else {
				status = IN;
				kernel = 1;
				entry->kernel = strdup(word);
				lineptr = lineptr + strlen(word);
				strtrim(lineptr);
				if (strlen(lineptr) > 0) {
					entry->append = strdup(lineptr);
				}
			}
			free(line);
			free(word);
			break;
		case RT:
			strtrim(lineptr);
			word = getnextword(lineptr);
			if (!strlen(word)) {
				status = R;
				free(word);
				free(line);
				break;
			}
			lineptr = lineptr + strlen(word);
			strtrim(lineptr);
			if (strcmp(word, "tftp") == 0) {
				entry->device_type = TFTP_TYPE;
				free(word);
				free(line);
				status = IN;
			} else if (strcmp(word, "cdrom") == 0) {
				entry->device_type = CD_TYPE;
				free(word);
				free(line);
				status = IN;
			} else if (strcmp(word, "ide") == 0) {
				char *sep = strchr(lineptr, ':');
				if (sep == NULL || *++sep == 0) {
					status = R;
					free(line);
					free(word);
					break;
				}
				char *dev = malloc(sep - lineptr);
				strncpy(dev, lineptr, sep - lineptr - 1);
				dev[sep - lineptr - 1] = 0;
				entry->device_type = IDE_TYPE;
				entry->device_num = strtol(dev);
				entry->partition = strtol(sep) - 1;
				free(word);
				free(line);
				free(dev);
				status = IN;
			} else {
				status = R;
				free(word);
				free(line);
			}
			break;
		case O:
			strtrim(lineptr);
			word = getnextword(lineptr);
			if (!strlen(word)) {
				status = R;
				free(word);
				free(line);
				break;
			}
			lineptr = lineptr + strlen(word);
			entry->other = strdup(word);
			kernel = 1;
			status = IN;
			break;
		case RD:
			strtrim(lineptr);
			word = getnextword(lineptr);
			if (!strlen(word))
				status = R;
			else {
				status = IN;
				entry->initrd = strdup(word);
			}
			free(line);
			free(word);
			break;
		case M:
			strtrim(lineptr);
			word = getnextword(lineptr);
			if (!strlen(word))
				status = R;
			else {
				status = IN;
				entry->modules[entry->modules_cnt++] =
					EntryObject_new(word, NULL, NULL);
			}
			free(line);
			free(word);
			break;
		case A:
			for (ptr = menu; ptr->next != NULL; ptr = ptr->next) ;
			ptr->next = entry;
			n++;
			status = S;
			break;
		case R:
			entry_free(entry);
			status = S;
			break;
		case AQ:
			for (ptr = menu; ptr->next != NULL; ptr = ptr->next) ;
			ptr->next = entry;
			return 1;
			break;
		case RQ:
		default:
			entry_free(entry);
			return 0;
			break;
		}
	}
	return 1;
}
/*
typedef struct menu {
	char *title;
	char *kernel;
	char *append;
	char *initrd;
	char *modules[MAX_MODULES];
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
*/

static int dev_load(menu_t *self, boot_dev_t *dev)
{
	int n = 0;
	char *buffer;
	int buffer_length;
	buffer = malloc(16 * 0x1000);
	if ((buffer_length = dev->load_file(dev, "menu.lst", buffer)) > 0)
		n += fsm(buffer, buffer_length, self);
	else if ((buffer_length = dev->load_file(dev, "boot/menu.lst", buffer)) > 0)
		n += fsm(buffer, buffer_length, self);
	else if ((buffer_length = dev->load_file(dev, "Kickstart/Kicklayout",
					    buffer)) > 0)
		n += kick_fsm(buffer, buffer_length, self);
	free(buffer);
	return n;
}

static int magic_load(menu_t *self)
{
	int i, n;
	boot_dev_t *dev;
	menu_t *ptr;
	struct RdbPartition *partition;

	n = 0;
	for(i = 0; i < RDB_LOCATION_LIMIT; i++) {
		partition = RdbPartitionTable_get(0, i);
		if(partition == NULL)
			break;
		if((dev = ext2_create(partition)) == NULL)
			dev = sfs_create(partition);
		if(dev == NULL)
			continue;
		n += dev_load(self, dev);
		for(ptr = self; ptr != NULL; ptr = ptr->next) {
			if(ptr->device_type != 0)
				continue;
			ptr->device_type = IDE_TYPE;
			ptr->device_num = 0;
			ptr->partition = i;
		}
		dev->destroy(dev);
	}
	return n;
}

menu_t *menu_load(boot_dev_t * boot)
{
	menu_t *self;
	int n;

	self = entry_alloc();
	self->default_os = 0;
	self->delay = 5;

	n = 0;

	if(boot != NULL)
		n = dev_load(self, boot);
	else
		n = magic_load(self);

	if (n == 0) {
		return NULL;
		free(self);
	}

	return self;
}

menu_t *display(menu_t * self, int selected)
{
	int i;
	menu_t *entry, *sentry = NULL;
	char buff[100];
	static int first = 0, last = 10;

	video_draw_box(1, 0, "Select boot option.", 1, 5, 5, 70, 15);

	if (selected < first)
		first--;

	if (selected > last)
		first++;

	for (i = 0; i < first; i++)
		self = self->next;

	for (i = 0, entry = self->next; entry != NULL && i < 11;
	     entry = entry->next, i++) {
		sprintf(buff, " %s", entry->title);
		video_draw_text(6, 8 + i, (i + first == selected ? 2 : 0), buff,
				68);
		if (i + first == selected)
			sentry = entry;
	}

	last = first + i - 1;

	return sentry;
}

menu_t *menu_display(menu_t * self)
{
	int key, max, selected, delay, old_delay;
	menu_t *entry;
	for (max = -2, entry = self; entry != NULL;
	     entry = entry->next, max++) ;

	selected = self->default_os <= max ? self->default_os : max;
	delay = self->delay * 100;
	if (delay <= 0)
		goto skip_delay;
	entry = display(self, selected);
	old_delay = 0;
	while (delay) {
		if (old_delay != delay / 100) {
			char tmp[30];
			sprintf(tmp, "(%d seconds left)", delay / 100);
			video_draw_text(54, 6, 0, tmp, 20);
			old_delay = delay / 100;
		}
		if (tstc())
			break;
		udelay(10000);
		delay--;
	}

	if (delay == 0)
		return entry;

skip_delay:
	while (1) {
		entry = display(self, selected);
		key = video_get_key();
		if (key == 4 && selected > 0)
			selected--;
		if (key == 3 && selected < max)
			selected++;
		if (key == 117)
			break;
		if (key == 1 || key == 6)
			return entry;
	}

	return NULL;
}

void menu_free(menu_t * self)
{
	menu_t *entry;
	while (self != NULL) {
		entry = self;
		self = self->next;
		entry_free(entry);
	}
}
