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

#include "mkamikeymap.h"
#include "debug.h"

static unsigned int     slen = 0; /* The allocation length pointed to be line */
static char             *line = NULL; /* The current read file */
static unsigned int     lineno = 0; /* The line number, will be increased by one everytime a line is read */

#define TYPE_STRINGDESC (1 << 0)
#define TYPE_DEADDESC   (1 << 1)

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

void GetEncodedChar(char *s, UBYTE *dchar, ULONG *len)
{
    *len = 0;
    if (s[0] == '\\')
    {
        char c = s[1];
        switch (c)
        {
        case 'b':
            *len = 2;
            *dchar = 0x8;
            break;

        case 'f':
            *len = 2;
            *dchar = 0xC;
            break;

        case 'n':
            *len = 2;
            *dchar = 0xA;
            break;

        case 'r':
            *len = 2;
            *dchar = 0xD;
            break;

        case 't':
            *len = 2;
            *dchar = 0x9;
            break;

        case 'v':
            *len = 2;
            *dchar = 0xB;
            break;

        case '\\':
            *len = 2;
            *dchar = 0x5c;
            break;

        case '\'':
            *len = 2;
            *dchar = 0x27;
            break;

        case '\"':
            *len = 2;
            *dchar = 0x22;
            break;

        case '\?':
            *len = 2;
            *dchar = 0x3F;
            break;

        default:
            fprintf(stdout, "support for char sequence needs implemented\n");
            break;
        }
    }
    else
    {
        *len = 1;
        *dchar = s[0];
    }
}

void GetEncodedBytes(char *s, UBYTE *strbuffer, ULONG *count)
{
    BOOL done = FALSE;
    char *ptr = s, *nxt, c;
    UBYTE *out = strbuffer;
    UBYTE tmp;

    while (!done)
    {
        c = *ptr;
        switch (c)
        {
        case '\0':
            done = 1;
            break;

        /* skip spaces and commas ..*/
        case ' ':
        case '\t':
        case ',':
            break;

        case '\'':
            {
                ULONG clen;
                char cbyte;

                GetEncodedChar(&ptr[1], &cbyte, &clen);
                D(fprintf(stdout, "decoded %u bytes into char '%c'\n", clen, cbyte);)
                *out = cbyte;
                *count = *count + 1;
                out++;
                ptr = (char *)((IPTR)ptr + 1 + clen);
            }
            break;

        default:
            tmp = (UBYTE)strtoul(ptr, &nxt, 0);
            if (nxt != ptr)
            {
                *out = tmp;
                *count = *count + 1;
                out++;
                ptr = (char *)((IPTR)nxt - 1);
            }
            break;
        }
        ptr++;
    }
}

BOOL processSectString(struct config *cfg, FILE *descf)
{
    UBYTE strbuffer[256], *buffptr;
    ULONG bcount, count = 0;
    char *s, *id = NULL;
    BOOL retval = TRUE, done = FALSE;

    D(fprintf(stdout, "processing 'string' section\n");)
    memset(strbuffer, 0, sizeof(strbuffer));
    buffptr = strbuffer;
    while (!done && (readline(descf)!=NULL))
    {
        DLINE(fprintf(stdout, "line = '%s'\n", line);)
        if (strncmp(line, "##", 2)==0)
        {
            s = line+2;
            while (isspace(*s)) s++;

            if (strncmp(s, "end", 3)!=0)
            {
                retval = FALSE;
            }
            done = TRUE;
        }
        else if (strncmp(line, "#", 1)!=0)
        {
            if (strncmp(line, "id:", 3)==0)
            {
                s = line+3;
                while (isspace(*s)) s++;

                id = malloc (strlen (s) + 1);
                strcpy (id, s);
                D(fprintf(stdout, "string ID: %s\n", id);)
            }
            else
            {
                bcount = 0;
                GetEncodedBytes(line, buffptr, &bcount);
                buffptr += bcount;
                count += bcount;
            }
        }
    }
    D(fprintf(stdout, "string section contained %u bytes\n", count);)
    if (id && count)
    {
        struct Node *strNode = malloc(sizeof(struct Node) + count + strlen(id) + 1);
        char *tmp;
        D(
            int i;
            for (i = 0; i < count; i ++)
                fprintf(stdout, " %u", strbuffer[i]);
            fprintf(stdout, "\n");
        )
        tmp = (char *)((IPTR)strNode + sizeof(struct Node));
        strNode->ln_Name = tmp;
        sprintf(strNode->ln_Name, "%s", id);
        tmp = (char *)((IPTR)strNode->ln_Name + strlen(strNode->ln_Name) + 1);
        memcpy(tmp, strbuffer, count);
        strNode->ln_Type = TYPE_STRINGDESC;
        strNode->ln_Pri = count;
        // Add the node ...
        ADDTAIL(&cfg->KeyDesc, strNode);
    }
    free(id);
    return retval;
}

BOOL processSectDeadkey(struct config *cfg, FILE *descf)
{
    UBYTE dkbuffer[256], *buffptr;
    ULONG bcount, count = 0;
    char *s, *id = NULL;
    BOOL retval = TRUE, done = FALSE;

    D(fprintf(stdout, "processing 'deadkey' section\n");)
    memset(dkbuffer, 0, sizeof(dkbuffer));
    buffptr = dkbuffer;
    while (!done && (readline(descf)!=NULL))
    {
        DLINE(fprintf(stdout, "line = '%s'\n", line);)
        if (strncmp(line, "##", 2)==0)
        {
            s = line+2;
            while (isspace(*s)) s++;

            if (strncmp(s, "end", 3)!=0)
            {
                retval = FALSE;
            }
            done = TRUE;
        }
        else if (strncmp(line, "#", 1)!=0)
        {
            if (strncmp(line, "id:", 3)==0)
            {
                s = line+3;
                while (isspace(*s)) s++;

                id = malloc (strlen (s) + 1);
                strcpy (id, s);
                D(fprintf(stdout, "deadkey ID: %s\n", id);)
            }
            else
            {
                bcount = 0;
                GetEncodedBytes(line, buffptr, &bcount);
                buffptr += bcount;
                count += bcount;
            }
        }
    }
    D(fprintf(stdout, "deadkey section contained %u bytes\n", count);)
    if (id && count)
    {
        struct Node *strNode = malloc(sizeof(struct Node) + count + strlen(id) + 1);
        char *tmp;
        D(
            int i;
            for (i = 0; i < count; i ++)
                fprintf(stdout, " %u", dkbuffer[i]);
            fprintf(stdout, "\n");
        )
        tmp = (char *)((IPTR)strNode + sizeof(struct Node));
        strNode->ln_Name = tmp;
        sprintf(strNode->ln_Name, "%s", id);
        tmp = (char *)((IPTR)strNode->ln_Name + strlen(strNode->ln_Name) + 1);
        memcpy(tmp, dkbuffer, count);
        strNode->ln_Type = TYPE_DEADDESC;
        strNode->ln_Pri = count;
        // Add the node ...
        ADDTAIL(&cfg->KeyDesc, strNode);
    }
    free(id);
    return retval;
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
        typesptr[keyno] = keytypes;
        D(fprintf(stdout, "key #%02d types = 0x%02x\n", keyno, keytypes);)
        keyno++;
    }
    return FALSE;
}

IPTR FindListEntry(struct config *cfg, char *id)
{
    struct Node *entry;
    ForeachNode(&cfg->KeyDesc, entry)
    {
        if (!strcmp(entry->ln_Name, id))
        {
            return (IPTR)entry;
        }
    }
    return 0;
}

BOOL processSectMap(struct config *cfg, FILE *descf, IPTR *map, UBYTE *typesptr, UBYTE cnt)
{
    int i = 0;
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
        if ((typesptr[i] & (KCF_DEAD|KCF_STRING)) != 0)
        {
            if (strncmp(line, "id:", 3)!=0)
            {
                fprintf(stdout, "ERROR: string/deadkey ID expected!\n");
                return FALSE;
            }
            s = line+3;
            while (isspace(*s)) s++;

            D(fprintf(stdout, "string/deadkey ID '%s'\n", s);)
            if ((map[i] = FindListEntry(cfg, s)) == 0)
            {
                fprintf(stdout, "ERROR: string/deadkey descriptor '%s' missing!\n", s);
                return FALSE;
            }
            else if ((typesptr[i] & KCF_DEAD) && (((struct Node *)map[i])->ln_Type == TYPE_STRINGDESC))
            {
                fprintf(stdout, "ERROR: deadkey descriptor type mismatch!\n");
                return FALSE;
            }
            else if ((typesptr[i] & KCF_STRING) && (((struct Node *)map[i])->ln_Type == TYPE_DEADDESC))
            {
                fprintf(stdout, "ERROR: string descriptor type mismatch!\n");
                return FALSE;
            }
        }
        else
        {
            UBYTE tmpbuffer[32];
            ULONG bcount = 0;
            GetEncodedBytes(line, tmpbuffer, &bcount);
            if (bcount != 4)
            {
                D(fprintf(stdout, "ERROR: incorrect number of entries in map!\n");)
                return FALSE;
            }
            map[i] = (tmpbuffer[0] << 24 | tmpbuffer[1] << 16 | tmpbuffer[2] << 8 | tmpbuffer[3]);
            D(fprintf(stdout, "%08x\n", (ULONG)map[i]);)
        }
        i++;
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
        flags[rowstart >> 3] = caps;
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
                if (!processSectTypes(cfg, descf, cfg->LoKeyMapTypes, 0x40))
                {
                    fprintf(stderr, "error processing lokey map-types section\n");
                    exit(20);
                }
                break;

            case 3: /* lokeymap */
                if (!processSectMap(cfg, descf, cfg->LoKeyMap, cfg->LoKeyMapTypes, 0x40))
                {
                    fprintf(stderr, "error processing lokey map section\n");
                    exit(20);
                }
                break;

            case 4: /* locapsable */
                if (!processSectCapsRep(cfg, descf, cfg->LoCapsable, 0x8))
                {
                    fprintf(stderr, "error processing lokey capsable section\n");
                    exit(20);
                }
                break;

            case 5: /* lorepeatable */
                if (!processSectCapsRep(cfg, descf, cfg->LoRepeatable, 0x8))
                {
                    fprintf(stderr, "error processing lokey repeatable section\n");
                    exit(20);
                }
                break;

            case 6: /* hikeymaptypes */
                if (!processSectTypes(cfg, descf, cfg->HiKeyMapTypes, 0x38))
                {
                    fprintf(stderr, "error processing hikey map-types section\n");
                    exit(20);
                }
                break;

            case 7: /* hikeymap */
                if (!processSectMap(cfg, descf, cfg->HiKeyMap, cfg->HiKeyMapTypes, 0x38))
                {
                    fprintf(stderr, "error processing hikey map section\n");
                    exit(20);
                }
                break;

            case 8: /* hicapsable */
                if (!processSectCapsRep(cfg, descf, cfg->HiCapsable, 0x7))
                {
                    fprintf(stderr, "error processing hikey capsable section\n");
                    exit(20);
                }
                break;

            case 9: /* hirepeatable */
                if (!processSectCapsRep(cfg, descf, cfg->HiRepeatable, 0x7))
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
        NEWLIST(&cfg->KeyDesc);
        retval = processDescriptor(cfg, descf);
        fclose(descf);
    }
    return retval;
}
