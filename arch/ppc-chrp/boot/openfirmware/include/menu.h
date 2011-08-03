#ifndef MENU_H_
#define MENU_H_

/*
 * $Id$
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#define MAX_ARGC           20
#define MAX_MODULES       100
#define MAX_MENU_ENTRIES    9

typedef struct menu_entry {
    char *      m_title;
    char *      m_kernel;
    char *      m_cmdline;
    char *      m_modules[MAX_MODULES];
    int         m_modules_cnt;
} menu_entry_t;

#if 0
extern int default_option;
extern int timeout_option;
extern menu_entry_t *menu_entries[9];
extern int menu_entries_cnt;
#endif

int load_menu(uint8_t *load_base);
void parse_menu();
menu_entry_t *execute_menu();
void free_menu();

#endif /*MENU_H_*/
