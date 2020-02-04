/*
    Copyright © 2020, The AROS Development Team. All rights reserved.
    $Id$

    Code to parse the command line options and the module config file for
    the genmodule program
*/

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#define __USE_XOPEN
#include <time.h>
#include <unistd.h>
#include <limits.h>
#include <ctype.h>

#include "mkkeymap.h"

#define D(x)
#define DLINE(x)

static unsigned int     slen = 0; /* The allocation length pointed to be line */
static char             *line = NULL; /* The current read file */
static unsigned int     lineno = 0; /* The line number, will be increased by one everytime a line is read */

char *readline(FILE *descf)
{
    char haseol;

    if (descf==NULL || feof(descf))
        return NULL;

    if (slen==0)
    {
        slen = 256;
        line = malloc(slen);
    }
    if (fgets(line, slen, descf))
    {
        size_t len = strlen(line);
        haseol = line[len-1]=='\n';
        if (haseol) line[len-1]='\0';

        while (!(haseol || feof(descf)))
        {
            slen += 256;
            line = (char *)realloc(line, slen);
            if (fgets(line+len, slen, descf))
            {
                len = strlen(line);
                haseol = line[len-1]=='\n';
                if (haseol) line[len-1]='\0';
            }
            else if (ferror(descf))
            {
                perror("descriptor error");
                free(line);
                return NULL;
            }
        }
    }
    else
        line[0]='\0';
    lineno++;

    return line;
}

BOOL processSectConfig(struct config *cfg, FILE *descf)
{
    char *s;

    D(fprintf(stdout, "processing 'config' section\n");)
    while ((readline(descf))!=NULL)
    {
        DLINE(fprintf(stdout, "line = '%s'\n", line);)
        if (strncmp(line, "##", 2)==0)
        {
            s = line+2;
            while (isspace(*s)) s++;

            if (strncmp(s, "end", 3)!=0)
                return FALSE;
            return TRUE;
        }
        if (strncmp(line, "keymap:", 7)==0)
        {
            if (!cfg->keymap)
            {
                s = line + 7;
                while (isspace(*s)) s++;
                cfg->keymap = malloc (strlen (s) + 1);
                strcpy (cfg->keymap, s);
                D(fprintf(stdout, "using keymap name '%s'\n", cfg->keymap);)
            }
        }
    }
    return FALSE;
}

BOOL processSectString(struct config *cfg, FILE *descf)
{
    char *s;

    D(fprintf(stdout, "processing 'string' section\n");)
    while ((readline(descf))!=NULL)
    {
        DLINE(fprintf(stdout, "line = '%s'\n", line);)
        if (strncmp(line, "##", 2)==0)
        {
            s = line+2;
            while (isspace(*s)) s++;

            if (strncmp(s, "end", 3)!=0)
                return FALSE;
            return TRUE;
        }
    }
    return FALSE;
}

BOOL processSectDeadkey(struct config *cfg, FILE *descf)
{
    char *s;

    D(fprintf(stdout, "processing 'deadkey' section\n");)
    while ((readline(descf))!=NULL)
    {
        DLINE(fprintf(stdout, "line = '%s'\n", line);)
        if (strncmp(line, "##", 2)==0)
        {
            s = line+2;
            while (isspace(*s)) s++;

            if (strncmp(s, "end", 3)!=0)
                return FALSE;
            return TRUE;
        }
    }
    return FALSE;
}

BOOL processSectTypes(struct config *cfg, FILE *descf, UBYTE *typesptr, UBYTE cnt)
{
    UBYTE keytypes, keyno = 0;
    char *s;

    D(fprintf(stdout, "processing 'types' section\n");)
    while ((readline(descf))!=NULL)
    {
        DLINE(fprintf(stdout, "line = '%s'\n", line);)
        keytypes = 0;
        if (strncmp(line, "##", 2)==0)
        {
            s = line+2;
            while (isspace(*s)) s++;

            if (strncmp(s, "end", 3)!=0)
                return FALSE;
            return TRUE;
        }

        static char *types[] =
        {
            "KC_NOQUAL", "KCF_SHIFT", "KCF_ALT", "KCF_CONTROL", "KCF_DEAD", "KC_VANILLA", "KCF_STRING", "KCF_NOP"
        };
        const unsigned int nums = sizeof(types)/sizeof(char *);
        unsigned int type;
        char *atend;
        int i;

        s = line;
        atend = line + strlen(line);

        while (isspace(*s)) s++;

        while (s < atend)
        {
            if (*s == '#')
                break;

            for (i = 0, type = 0; type==0 && i<nums; i++)
            {
                if (strncmp(s, types[i], strlen(types[i]))==0)
                {
                    type = i+1;
                    s += strlen(types[i]);
                    while (isspace(*s)) s++;
                    break;
                }
            }

            if (type != 0)
            {
                switch (type)
                {
                    case 1:
                        keytypes |= KC_NOQUAL;
                        break;
                    case 2:
                        keytypes |= KCF_SHIFT;
                        break;
                    case 3:
                        keytypes |= KCF_ALT;
                        break;
                    case 4:
                        keytypes |= KCF_CONTROL;
                        break;
                    case 5:
                        keytypes |= KCF_DEAD;
                        break;
                    case 6:
                        keytypes |= KC_VANILLA;
                        break;
                    case 7:
                        keytypes |= KCF_STRING;
                        break;
                    case 8:
                        keytypes |= KCF_NOP;
                        break;
                }
            }
            else s++;
        }
        if (keyno >= cnt)
        {
            fprintf(stderr, "error, too many key types defined (%d >= %d)\n", keyno, cnt);
            exit(20);
        }
        D(fprintf(stdout, "key #%02d types = 0x%02x\n", keyno, keytypes);)
        keyno++;
    }
    return FALSE;
}

BOOL processSectMap(struct config *cfg, FILE *descf, IPTR *map, UBYTE cnt)
{
    char *s;

    D(fprintf(stdout, "processing 'map' section\n");)
    while ((readline(descf))!=NULL)
    {
        DLINE(fprintf(stdout, "line = '%s'\n", line);)
        if (strncmp(line, "##", 2)==0)
        {
            s = line+2;
            while (isspace(*s)) s++;

            if (strncmp(s, "end", 3)!=0)
                return FALSE;
            return TRUE;
        }
    }
    return FALSE;
}

BOOL processSectCapsRep(struct config *cfg, FILE *descf, UBYTE *flags, UBYTE cnt)
{
    UBYTE caps, keyno = 0, rowstart;
    char *s;

    D(fprintf(stdout, "processing 'capsable/repeatable' section\n");)
    while ((readline(descf))!=NULL)
    {
        DLINE(fprintf(stdout, "line = '%s'\n", line);)
        if (strncmp(line, "##", 2)==0)
        {
            s = line+2;
            while (isspace(*s)) s++;

            if (strncmp(s, "end", 3)!=0)
                return FALSE;
            return TRUE;
        }

        char *atend;
        int i;

        caps = 0;
        rowstart = keyno;

        s = line;
        atend = line + strlen(line);

        while (isspace(*s)) s++;

        while (s < atend)
        {
            BOOL shift = FALSE;
            if (*s == '#')
                break;

            if (*s == '1')
            {
                caps |= 1;
                shift = TRUE;
            }
            else if (*s == '0')
                shift = TRUE;
            s++;
            if (shift)
            {
                keyno++;
                if ((keyno - rowstart) >= 7)
                    break;
                if ((keyno & 8) != 0)
                    caps <<= 1;
            }
        }
        if ((keyno >> 3) >= cnt)
        {
            fprintf(stderr, "error, too many rows of keys defined (%d >= %d)\n", (keyno >> 3), cnt);
            exit(20);
        }
        keyno = rowstart + 8;
        D(fprintf(stdout, "key #%02d - %02d caps = 0x%02x\n", keyno - 8, keyno - 1, caps);)
    }
    return FALSE;
}

/* Parse a keymap descriptor and populate the KeyMap structure.
 */
BOOL processDescriptor(struct config *cfg, FILE *descf)
{
    char *s, *s2;

    while ((readline(descf))!=NULL)
    {
        if (strncmp(line, "##", 2)==0)
        {
            static char *parts[] =
            {
                "config", "lokeymaptypes", "lokeymap", "locapsable", "lorepeatable", "hikeymaptypes", "hikeymap", "hicapsable", "hirepeatable", "string", "deadkey"
            };
            const unsigned int nums = sizeof(parts)/sizeof(char *);
            unsigned int partnum;
            int i, atend = 0;

            s = line+2;
            while (isspace(*s)) s++;

            if (strncmp(s, "begin", 5)!=0)
                return FALSE;

            s += 5;
            if (!isspace(*s))
            {
                fprintf(stderr, "expected space after 'begin'\n");
                exit(20);
            }
            while (isspace(*s)) s++;

            for (i = 0, partnum = 0; partnum==0 && i<nums; i++)
            {
                if (strncmp(s, parts[i], strlen(parts[i]))==0)
                {
                    partnum = i+1;
                    s += strlen(parts[i]);
                    while (isspace(*s)) s++;
                    if (*s!='\0')
                    {
                        fprintf(stderr, "unexpected character at position %d\n", (int)(s-line));
                        exit(20);
                    }
                }
            }
            if (partnum==0)
            {
                fprintf(stderr, "unknown section start\n");
                exit(20);
            }
            switch (partnum)
            {
            case 1: /* config */
                if (!processSectConfig(cfg, descf))
                {
                    fprintf(stderr, "error processing config section\n");
                    exit(20);
                }
                break;

            case 2: /* lokeymaptypes */
                if (!processSectTypes(cfg, descf, NULL, 0x40))
                {
                    fprintf(stderr, "error processing lokey map-types section\n");
                    exit(20);
                }
                break;

            case 3: /* lokeymap */
                if (!processSectMap(cfg, descf, NULL, 0x40))
                {
                    fprintf(stderr, "error processing lokey map section\n");
                    exit(20);
                }
                break;

            case 4: /* locapsable */
                if (!processSectCapsRep(cfg, descf, NULL, 0x8))
                {
                    fprintf(stderr, "error processing lokey capsable section\n");
                    exit(20);
                }
                break;

            case 5: /* lorepeatable */
                if (!processSectCapsRep(cfg, descf, NULL, 0x8))
                {
                    fprintf(stderr, "error processing lokey repeatable section\n");
                    exit(20);
                }
                break;

            case 6: /* hikeymaptypes */
                if (!processSectTypes(cfg, descf, NULL, 0x38))
                {
                    fprintf(stderr, "error processing hikey map-types section\n");
                    exit(20);
                }
                break;

            case 7: /* hikeymap */
                if (!processSectMap(cfg, descf, NULL, 0x38))
                {
                    fprintf(stderr, "error processing hikey map section\n");
                    exit(20);
                }
                break;

            case 8: /* hicapsable */
                if (!processSectCapsRep(cfg, descf, NULL, 0x7))
                {
                    fprintf(stderr, "error processing hikey capsable section\n");
                    exit(20);
                }
                break;

            case 9: /* hirepeatable */
                if (!processSectCapsRep(cfg, descf, NULL, 0x7))
                {
                    fprintf(stderr, "error processing hikey repeatable section\n");
                    exit(20);
                }
                break;

            case 10: /* string */
                if (!processSectString(cfg, descf))
                {
                    fprintf(stderr, "error processing string section\n");
                    exit(20);
                }
                break;

            case 11: /* deadkey */
                if (!processSectDeadkey(cfg, descf))
                {
                    fprintf(stderr, "error processing deadkey section\n");
                    exit(20);
                }
                break;
            }
        }
    }
    return TRUE;
}

BOOL parseKeyDescriptor(struct config *cfg)
{
    FILE *descf = NULL;
    BOOL retval = FALSE;

    descf = fopen(cfg->descriptor, "r");
    if (descf != NULL)
    {
        retval = processDescriptor(cfg, descf);
        fclose(descf);
    }
    return retval;
}
