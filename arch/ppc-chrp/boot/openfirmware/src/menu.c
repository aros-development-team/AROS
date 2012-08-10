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

#define DEBUG 0

#include <inttypes.h>
#include <menu.h>
#include <support.h>
#include <debug.h>
#include <of1275.h>

static char *menu_file;
static int menu_file_size;
static char *menu_lines[1024];
static int menu_lines_cnt;

static menu_entry_t *menu_entries[9];
static int menu_entries_cnt;

static int default_option = 0;
static int timeout_option = 10;

extern char *bootpath;

menu_entry_t *execute_menu()
{
    menu_entry_t *menu = NULL;

    if (menu_entries_cnt == 1)
    {
        menu = menu_entries[0];
    }
    else if (menu_entries_cnt > 1)
    {
        int i;
        int selected = default_option;
        char chr;

        printf("\r\nAvailable boot configurations:\r\n");

        for (i=0; i < menu_entries_cnt; i++)
        {
        	printf("%c. %s\r\n", (i+1) < 10 ? i+'1':i+'A', menu_entries[i]->m_title);
        }

        printf("Boot? >");
        while (ofw_read(stdin, &chr, 1) < 0) {
        	ofw_interpret("10 ms");
        }

        if (chr >= '1' && chr <= '9')
        	chr-='0';
        else if (chr >= 'A' && chr <= 'Z')
        	chr = chr - 'A' + 10;

        printf("\r\n");

        if (1)
        {
			if (chr >= 1 && chr <= menu_entries_cnt) {
				D(bug("[BOOT] correct key\r\n"));
				selected = chr-1;
			}
			else return NULL;
        }

        menu = menu_entries[selected];
        D(bug("[BOOT] Selected configuration: \"%s\"\r\n", menu->m_title));
    }

    return menu;
}

int load_menu(uint8_t *load_base)
{
    int err = 0;
    uint32_t *ptr;
    int i;
    char tmp[1024];

    D(bug("[BOOT] Trying to load menu.lst file\r\n"));

	/* Clear memory before loading a text file */
	ptr = (uint32_t *)load_base;
	for (i=0; i < LOAD_SIZE / 4; i++)
		ptr[i] = 0;

	sprintf(tmp, "load %s boot/menu.lst", bootpath);
	if (ofw_interpret(tmp))
	{
		sprintf(tmp, "load %s menu.lst", bootpath);
		if (ofw_interpret(tmp))
			printf("Failed to load menu file\r\n");
	}

	menu_file_size = 0;

	while(load_base[menu_file_size])
		menu_file_size++;

	D(bug("[BOOT] Size of menu file: %d bytes\r\n", menu_file_size));

    /* 64 kilobytes of memory should be enough for simple menu ;) */
    menu_file = ofw_claim(NULL, menu_file_size, 4);

    if (menu_file == (char*)-1)
    {
        err = -1;
    }
    else
    {
    	memcpy(menu_file, load_base, menu_file_size);
    }

    return err;
}

void free_menu()
{
    int i;
    ofw_release(menu_file, menu_file_size);

    for (i=0; i < menu_entries_cnt; i++)
        ofw_release(menu_entries[i], sizeof(menu_entry_t));
}

char *eat_whitespace(char *c)
{
    while (*c && isblank(*c))
        c++;

    return c;
}

char *next_word(char *c)
{
    while (*c && !isblank(*c))
        c++;
    while (*c && isblank(*c))
        c++;

    return c;
}

void parse_menu(unsigned long def_virt)
{
    int i;
    menu_lines_cnt = 0;
    int new_line = 1;
    menu_entry_t *menu = NULL;

    for (i=0; i < menu_file_size; i++)
    {
        if (new_line) {
            if (menu_file[i]) {
                menu_lines[menu_lines_cnt++] = &menu_file[i];
                new_line = 0;
            }
            else continue;
        }

        if (menu_file[i] == '\r') {
            menu_file[i] = 0;
        }
        else if (menu_file[i] == '\n') {
            menu_file[i] = 0;
            new_line = 1;
        }
    }

    for (i=0; i < menu_lines_cnt; i++)
    {
        char *str = eat_whitespace(menu_lines[i]);

        D(bug("[BOOT] Line: "));
        D(bug(str));
        D(bug("\r\n"));

        if (*str == '#')
	{
	    /* Skip comments */
            continue;
        }
        else if (!strncasecmp(str, "timeout", 7) && isspace(str[7]))
	{
            str = next_word(str);
            timeout_option = atoi(str);

            D(bug("[BOOT] Found new timeout = %d\r\n", timeout_option));

        }
        else if (!strncasecmp(str, "default", 7) && isspace(str[7]))
	{
            str = next_word(str);
            default_option = atoi(str);

            D(bug("[BOOT] Found new default = %d\r\n", default_option));
        }
        else if (!strncasecmp(str, "title", 5) && isspace(str[5]))
        {
            if (menu_entries_cnt < MAX_MENU_ENTRIES)
            {
                int j;

                menu = menu_entries[menu_entries_cnt++] = ofw_claim(NULL, sizeof(menu_entry_t), 4);

                menu->m_title       = next_word(str);
		menu->m_kernel      = NULL;
                menu->m_modules_cnt = 0;
		menu->m_virtual     = def_virt;

                j = strlen(menu->m_title);
                while (isspace(menu->m_title[--j]))
                    menu->m_title[j] = 0;

                D(bug("[BOOT] Found new title '%s'\r\n", menu->m_title));
            }
            else return;
        }
	else if (!strncasecmp(str, "virtual_addr", 12) && isspace(str[12]))
	{
	    if (menu)
	    {
		char *c = next_word(str);

		menu->m_virtual = atoi(c);
	    }
	}
        else if (!strncasecmp(str, "kernel", 6) && isspace(str[6]))
        {
            D(bug("[BOOT] Found new kernel\r\n"));

            if (menu)
	    {
                char *c = next_word(str);
                menu->m_kernel = c;
                menu->m_cmdline = NULL;
                while(*c && !isblank(*c))
                	c++;

                while (*c) {
                    if (isblank(*c)) {
                        *c++ = 0;
                    }
                    else {
                        menu->m_cmdline = c;
                        break;
                    }
                }
                if (menu->m_cmdline && strlen(menu->m_cmdline))
                {
                	char *c = menu->m_cmdline + strlen(menu->m_cmdline);
                	while (--c > menu->m_cmdline)
                	{
                		if (isspace(*c))
                			*c = 0;
                		else
                			break;
                	}

                }
            }
        }
        else if (!strncasecmp(str, "module", 6) && isspace(str[6]))
        {
            D(bug("[BOOT] Found new module\r\n"));
            if (menu && menu->m_modules_cnt < MAX_MODULES)
            {
                menu->m_modules[menu->m_modules_cnt++] = next_word(str);
            }
        }
    }
}
