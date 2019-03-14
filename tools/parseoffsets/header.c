/*
    Copyright © 2019, The AROS Development Team. All rights reserved.
*/

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdbool.h>

#include "parsendkoffsets.h"
#include "header.h"

typedef struct hnode {
    struct hnode * next;
    struct hnode * prev;
    char *id;
} hnode_t;

struct headerSet
{
    hnode_t hs_Node;
    hnode_t *headerStructs;
};

struct headerMatch
{
    char *headerStruct;
    bool headerSkip;
    char *headerfile;
};

hnode_t *headerSets = NULL;

struct headerMatch knownStructs[] =
{
    { "ExecBase",			false,		"exec/execbase.h"						},

    { "DateStamp",			false,		"dos/dos.h"							},
    { "FileInfoBlock",			false,		"dos/dos.h"							},
    { "InfoData",			false,		"dos/dos.h"							},

    { "DosLibrary",			false,		"dos/dosextens.h"						},
    { "RootNode",			false,		"dos/dosextens.h"						},
    { "CLIInfo",			false,		"dos/dosextens.h"						},
    { "DosInfo",			false,		"dos/dosextens.h"						},
    { "Process",			false,		"dos/dosextens.h"						},
    { "CommandLineInterface",		false,		"dos/dosextens.h"						},
    { "DevProcs",			false,		"dos/dosextens.h"						},
    { "FileHandle",			true,		"dos/dosextens.h"						},
    { "FileLock",			false,		"dos/dosextens.h"						},
    { "DosList",			false,		"dos/dosextens.h"						},
    { "DeviceList",			false,		"dos/dosextens.h"						},
    { "DevInfo",			false,		"dos/dosextens.h"						},
    { "AssignList",			false,		"dos/dosextens.h"						},
    { "DosPacket",			false,		"dos/dosextens.h"						},
    { "StandardPacket",			false,		"dos/dosextens.h"						},
    { "Segment",			false,		"dos/dosextens.h"						},
    { "ErrorString",			false,		"dos/dosextens.h"						},

    { "AChain",				false,		"dos/dosasl.h"							},
    { "AnchorPath",			false,		"dos/dosasl.h"							},

    { "AmigaGuideHost",			false,		"libraries/amigaguide.h"					},
    { "AmigaGuideMsg",			false,		"libraries/amigaguide.h"					},
    { "NewAmigaGuide",			false,		"libraries/amigaguide.h"					},
    { "XRef",				false,		"libraries/amigaguide.h"					},
    { "opFindHost",			false,		"libraries/amigaguide.h"					},
    { "opNodeIO",			false,		"libraries/amigaguide.h"					},
    { "opExpungeNode",			false,		"libraries/amigaguide.h"					},

    { "DateTime",			false,		"dos/datetime.h"						},

    {NULL,				true,		NULL							        }
};

void nodeaddtail(hnode_t *nodefirst, hnode_t *node)
{
    hnode_t * current = nodefirst;
    while (current->next != NULL) {
        current = current->next;
    }
    if (verbose)
    {
        printf ("Adding node @ %p after %p\n", node, current);
    }
    current->next = node;
    node->prev = current;
    node->next = NULL;
}

void noderem(hnode_t *node)
{
    hnode_t * prev, *next;
    prev = node->prev;
    next = node->next;

    next->prev = prev;
    prev->next = next;

    node->prev = NULL;
    node->next = NULL;
}

hnode_t *nodefind(hnode_t *nodefirst, char *id)
{
    hnode_t * current = nodefirst;
    while (current) {
        if (0 == strncmp(id, current->id, strlen(id)))
            return current;
        current = current->next;
    }
    return NULL;
}

void addSDKHeaderStruct(char *sdkheaderset, char *structname)
{
    struct headerSet *header_node = NULL;
    hnode_t *structnode = NULL;

    if (headerSets)
        header_node = (struct headerSet *)nodefind(headerSets, sdkheaderset);
    if (!header_node)
    {
        if (verbose)
        {
            printf ("registering new header-set %s\n", sdkheaderset);
        }
        header_node = malloc(sizeof(struct headerSet));
        header_node->hs_Node.id = malloc(strlen(sdkheaderset) + 1);
        memcpy(header_node->hs_Node.id, sdkheaderset, strlen(sdkheaderset));
        header_node->hs_Node.id[strlen(sdkheaderset)] = '\0';
        header_node->hs_Node.next = NULL;
        header_node->hs_Node.prev = NULL;
        header_node->headerStructs = NULL;

        if (!headerSets)
        {
            headerSets = &header_node->hs_Node;
            if (verbose)
            {
                printf ("first header-set registered (= %p)\n", headerSets);
            }
        }
        else
            nodeaddtail(headerSets, &header_node->hs_Node);
    }

    if (header_node->headerStructs)
        structnode = nodefind(header_node->headerStructs, structname);
    if (!structnode)
    {
        if (verbose)
        {
            printf ("registering new %s header-set struct %s\n", header_node->hs_Node.id, structname);
        }
        structnode = malloc(sizeof(hnode_t));
        structnode->id = malloc(strlen(structname) + 1);
        memcpy(structnode->id, structname, strlen(structname));
        structnode->id[strlen(structname)] = '\0';
        structnode->next = NULL;
        structnode->prev = NULL;

        if (!header_node->headerStructs)
        {
            header_node->headerStructs = structnode;
            if (verbose)
            {
                printf ("first struct registered (= %p)\n", structnode);
            }
        }
        else
            nodeaddtail(header_node->headerStructs, structnode);
    }
}

char *findStructHeader (char *sdkdir, char *structname)
{
    char *headerFilename = NULL;
    int i;

    if (sdkdir)
    {
        if (verbose)
        {
            printf ("Searching in '%s' for definition of '%s'\n", sdkdir, structname);
        }
    }
    for (i = 0; knownStructs[i].headerStruct != NULL; i++)
    {
        if (!strcmp(knownStructs[i].headerStruct, structname) && (!knownStructs[i].headerSkip))
            return knownStructs[i].headerfile;
    }
    return headerFilename;
}

void writeHeaderMakefile(char *gendir, char *bindir)
{
    char *Makefilename = NULL;
    FILE *Makefilefile = NULL;
    FILE *Settestfile = NULL;
    char *Settestfilename = NULL;

    makefullpath(gendir);
    if (strcmp(gendir, bindir))
        makefullpath(bindir);

    Makefilename = malloc(strlen(gendir) + strlen("Makefile") + 2);
    if (Makefilename)
    {
        sprintf(Makefilename, "%s/Makefile", gendir);

        Makefilefile = fopen(Makefilename, "w");
        if (!Makefilefile)
        {
            printf ("Failed to open '%s'\n", Makefilename);
            return;
        }
        printBanner(Makefilefile, "#");
        fprintf(Makefilefile, "include $(SRCDIR)/config/aros.cfg\n");

        if (headerSets)
        {
            struct headerSet *header_node = (struct headerSet *)headerSets;
            char *structFileName = malloc(LINELENGTH);

            /* Pass 1: generate recipes for the individual structure test targets */
            while (header_node) {
                if (header_node->headerStructs)
                {
                    hnode_t * current = header_node->headerStructs;
                    while (current) {
                        // Write rules for this struct's tests
                        sprintf(structFileName, "test-struct_%s", current->id);
                        fprintf(Makefilefile, " \n%s/%s.o : %s/%s.c\n\t@$(ECHO) \"Compiling $(notdir %s).c\"\n\t@$(strip $(TARGET_CC) $(TARGET_SYSROOT) $(CFLAGS) $(CPPFLAGS)) -c $< -o $@\n", gendir, structFileName, gendir, structFileName, structFileName);

                        fprintf(Makefilefile, " \n%s/%s : %s/%s.o\n\t@$(ECHO) \"Linking $(notdir %s)\"\n\t@$(strip $(TARGET_CC) $(TARGET_SYSROOT) $(TARGET_LDFLAGS)) $< -o $@\n", bindir, structFileName, gendir, structFileName, structFileName);

                        current = current->next;
                    }
                }
                header_node = (struct headerSet *)header_node->hs_Node.next;
            }

            /* Pass 2: generate header-set targets & scripts */
            header_node = (struct headerSet *)headerSets;		
            while (header_node) {
                if (header_node->headerStructs)
                {
                    hnode_t * current = header_node->headerStructs;

                    Settestfilename = malloc(strlen(bindir) + strlen(header_node->hs_Node.id) + 12);
                    fprintf(Makefilefile, "\ntest-sdk-%s-genfiles :", header_node->hs_Node.id);
                    if (Settestfilename)
                    {
                        sprintf(Settestfilename, "%s/run-%s-tests", bindir, header_node->hs_Node.id);
                        Settestfile = fopen(Settestfilename, "w");
                    }
                    while (current) {
                        // Write rules for this struct's tests
                        sprintf(structFileName, "%s/test-struct_%s", bindir, current->id);
                        fprintf(Makefilefile, " %s", structFileName);
                        if (Settestfile)
                            fprintf(Settestfile, "test-struct_%s\n", current->id);

                        current = current->next;
                    }
                    fprintf(Makefilefile, "\n");
                    if (Settestfile)
                        fclose(Settestfile);
                    if (Settestfilename)
                        free(Settestfilename);
                }
                header_node = (struct headerSet *)header_node->hs_Node.next;
            }

            /* Pass 3: generate main targets & scripts */
            Settestfilename = malloc(strlen(bindir) + 15);
            fprintf(Makefilefile, "\ntest-sdk-headers-genfiles :");
            header_node = (struct headerSet *)headerSets;
            if (Settestfilename)
            {
                sprintf(Settestfilename, "%s/run-sdk-tests", bindir);
                Settestfile = fopen(Settestfilename, "w");
            }
            while (header_node) {
                if (header_node->headerStructs)
                {
                    fprintf(Makefilefile, " test-sdk-%s-genfiles", header_node->hs_Node.id);
                    if (Settestfile)
                        fprintf(Settestfile, "c:execute run-%s-tests\n", header_node->hs_Node.id);
                }
                header_node = (struct headerSet *)header_node->hs_Node.next;
            }
            if (Settestfile)
                fclose(Settestfile);
            fprintf(Makefilefile, "\n");
            if (Settestfilename)
                free(Settestfilename);
            if (structFileName)
                free(structFileName);
        }
        fclose(Makefilefile);
        free(Makefilename);
    }
}

