/*
    Copyright (C) 2000 AROS - The Amiga Research OS
    $Id$

    Desc:
*/

/* Function prototypes */

/* module.c */
void writeModDefs(char *file, struct ModuleConfig *mc);

/* includes.c */
void genInclClib(char *file, struct ModuleData *md, char *hdrTmpl);
void genInclProto(char *file, struct ModuleData *md, char *hdrTmpl);
void genInclInline(char *file, struct ModuleData *md, char *hdrTmpl);
void genInclDefine(char *file, struct ModuleData *md, char *hdrTmpl);
void genInclPragma(char *file, struct ModuleData *md, char *hdrTmpl);
