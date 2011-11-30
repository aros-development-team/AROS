/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$
*/
/*
 * This program generates the libcall.h macroset for gcc 
 */

#include <stdio.h>
#include <string.h>

#define GENCALL_MAX	(13 + 1)	/* Max number of arguments */

void aros_lc(int id, const char *suffix)
{
    int i, isvoid = strcmp(suffix, "NR") == 0;
    
    printf("#define AROS_LC%d%s(t,n,", id, suffix);
    for (i = 0; i < id; i++)
        printf("a%d,", i + 1);
    printf("bt,bn,o,s) \\\n"
           "({ \\\n"
           "    register void *__fixedreg asm(__AROS_LCGCCREG_REGNAME); \\\n"
           "    t (*__stub)("
    );
    if (id == 0)
        printf("void");
    else
        for (i = 0; i < id; i++)
        {
            if (i > 0)
                printf(", ");
            printf("__AROS_LDA(a%d)", i+1);
        }
    printf("); \\\n");
    if (!isvoid)
        printf("    t _ret; \\\n");
    printf("    bt _bn = (bt)bn; \\\n"
           "    __stub = (t (*)("
    );
    if (id == 0)
        printf("void");
    else
        for (i = 0; i < id; i++)
        {
            if (i > 0)
                printf(", ");
            printf("__AROS_LDA(a%d)", i+1);
        }
    printf("))__AROS_GETVECADDR(_bn,o); \\\n"
           "    __asm__ __volatile__( \\\n"
           "        __AROS_LCGCCREG_ASMSET \\\n"
           "        : \"=r\"(__fixedreg) : __AROS_LCGCCREG_ASMOPERAND(_bn) \\\n"
           "    ); \\\n"
           "    %s__stub(",
           isvoid ? "" : "_ret = "
    );
    for (i = 0; i < id; i++)
    {
        if (i > 0)
            printf(", ");
        printf("__AROS_LCA(a%d)", i+1);
    }
    printf("); \\\n"
           "    __asm__ __volatile(\" \" : : \"r\"(__fixedreg)); \\\n"
    );
    if (!isvoid)
        printf("    _ret; \\\n");
    printf("})\n");
}

void aros_call(int id, const char *suffix)
{
    int i, isvoid = strcmp(suffix, "NR") == 0;
    
    printf("#define AROS_CALL%d%s(t,n,", id, suffix);
    for (i = 0; i < id; i++)
        printf("a%d,", i + 1);
    printf("bt,bn) \\\n"
           "({ \\\n"
           "    register void *__fixedreg asm(__AROS_LCGCCREG_REGNAME); \\\n"
           "    t (*__stub)("
    );
    if (id == 0)
        printf("void");
    else
        for (i = 0; i < id; i++)
        {
            if (i > 0)
                printf(", ");
            printf("__AROS_LDA(a%d)", i+1);
        }
    printf("); \\\n");
    if (!isvoid)
        printf("    t _ret; \\\n");
    printf("    bt _bn = bn; \\\n"
           "    __stub = (void *)n; \\\n"
           "    __asm__ __volatile__( \\\n"
           "        __AROS_LCGCCREG_ASMSET \\\n"
           "        : \"=r\"(__fixedreg) : __AROS_LCGCCREG_ASMOPERAND(_bn) \\\n"
           "    ); \\\n"
           "    %s__stub(",
           isvoid ? "" : "_ret = "
    );
    for (i = 0; i < id; i++)
    {
        if (i > 0)
            printf(", ");
        printf("__AROS_LCA(a%d)", i+1);
    }
    printf("); \\\n"
           "    __asm__ __volatile(\" \" : : \"r\"(__fixedreg)); \\\n"
    );
    if (!isvoid)
        printf("    _ret; \\\n");
    printf("})\n");
}

void aros_lh(int id, int is_ignored)
{
    int i;
    
    printf("#define AROS_LH%d%s(t,n,", id, is_ignored ? "I" : "");
    for (i = 0; i < id; i++)
        printf("a%d,", i + 1);
    printf("bt,bn,o,s) \\\n");
    printf("    t AROS_SLIB_ENTRY(n,s,o) (");
    if (i == 0)
        printf("void");
    for (i = 0; i < id; i++)
    {
        if (i > 0)
            printf(", ");
        printf("__AROS_LHA(a%d)", i + 1);
    }
    printf(") \\\n");
    if (!is_ignored)
        printf(
            "    { \\\n"
            "        register bt __bn asm(__AROS_LCGCCREG_REGNAME); \\\n"
            "        bt bn = __bn; (void)bn;\n"
            "\n"
        );
    else
        printf("    {\n\n");
}

static void aros_ld(int id, int is_ignored)
{
    int i;
    
    printf("#define AROS_LD%d%s(t,n,", id, is_ignored ? "I" : "");
    for (i = 0; i < id; i++)
        printf("a%d,", i + 1);
    printf("bt,bn,o,s) \\\n");
    printf("    t AROS_SLIB_ENTRY(n,s,o) (");
    if (i == 0)
        printf("void");
    for (i = 0; i < id; i++)
    {
        if (i > 0)
            printf(", ");
        printf("__AROS_LHA(a%d)", i + 1);
    }
    printf(");\n");
}

const static char extra[] =
"\n"
"#define __AROS_QUADt(type,name,reg1,reg2) type\n"
"#define __AROS_QUADn(type,name,reg1,reg2) name\n"
"#define __AROS_QUADr(type,name,reg1,reg2) reg1##reg2\n"
"\n"
"#define AROS_LHQUAD1(t,n,a1,bt,bn,o,s) \\\n"
"    AROS_LH1(t,n, \\\n"
"             AROS_LHA(__AROS_QUADt(a1), __AROS_QUADn(a1), __AROS_QUADr(a1)), \\\n"
"             bt, bn, o, s \\\n"
"    )\n"
"#define AROS_LHQUAD2(t,n,a1,a2,bt,bn,o,s) \\\n"
"    AROS_LH2(t,n, \\\n"
"             AROS_LHA(__AROS_QUADt(a1), __AROS_QUADn(a1), __AROS_QUADr(a1)), \\\n"
"             AROS_LHA(__AROS_QUADt(a2), __AROS_QUADn(a2), __AROS_QUADr(a2)), \\\n"
"             bt, bn, o, s \\\n"
"    )\n"
"#define AROS_LH1QUAD1(t,n,a1,a2,bt,bn,o,s) \\\n"
"    AROS_LH2(t,n, \\\n"
"             AROS_LHA(a1), \\\n"
"             AROS_LHA(__AROS_QUADt(a2), __AROS_QUADn(a2), __AROS_QUADr(a2)), \\\n"
"             bt, bn, o, s \\\n"
"    )\n"
"\n"
"\n"
"#define AROS_LCQUAD1(t,n,a1,bt,bn,o,s) \\\n"
"    AROS_LC1(t,n, \\\n"
"             AROS_LCA(__AROS_QUADt(a1), __AROS_QUADn(a1), __AROS_QUADr(a1)), \\\n"
"             bt, bn, o, s \\\n"
"    )\n"
"#define AROS_LCQUAD2(t,n,a1,a2,bt,bn,o,s) \\\n"
"    AROS_LC2(t,n, \\\n"
"             AROS_LCA(__AROS_QUADt(a1), __AROS_QUADn(a1), __AROS_QUADr(a1)), \\\n"
"             AROS_LCA(__AROS_QUADt(a2), __AROS_QUADn(a2), __AROS_QUADr(a2)), \\\n"
"             bt, bn, o, s \\\n"
"    )\n"
"#define AROS_LC1QUAD1(t,n,a1,a2,bt,bn,o,s) \\\n"
"    AROS_LC2(t,n, \\\n"
"             AROS_LCA(a1), \\\n"
"             AROS_LCA(__AROS_QUADt(a2), __AROS_QUADn(a2), __AROS_QUADr(a2)), \\\n"
"             bt, bn, o, s \\\n"
"    )\n"
"\n"
"#define AROS_LDQUAD1(t,n,a1,bt,bn,o,s) \\\n"
"     AROS_LD1(t,n, \\\n"
"         AROS_LDA(__AROS_QUADt(a1), __AROS_QUADn(a1), __AROS_QUADr(a1)), \\\n"
"         bt, bn, o, s \\\n"
"     )\n"
"#define AROS_LDQUAD2(t,n,a1,a2,bt,bn,o,s) \\\n"
"     AROS_LD2(t,n, \\\n"
"         AROS_LDA(__AROS_QUADt(a1), __AROS_QUADn(a1), __AROS_QUADr(a1)), \\\n"
"         AROS_LDA(__AROS_QUADt(a2), __AROS_QUADn(a2), __AROS_QUADr(a2)), \\\n"
"         bt, bn, o, s \\\n"
"     )\n"
"\n";

int main(int argc, char **argv)
{
    int i;
    
    printf("/* AUTOGENERATED by compiler/arossupport/gencall_gccreg.c */\n"
           "\n"
           "#ifndef AROS_LIBCALL_GCCREG_H\n"
           "#define AROS_LIBCALL_GCCREG_H\n"
           "\n"
           "/*\n"
           "    For shared libraries functions with specified m68k regs most\n"
           "    archs use normal C based argument passing except for the libbase\n"
           "    which is passed in a register\n"
           "*/\n"
    );

    printf("#define __AROS_CPU_SPECIFIC_LH\n\n");
	
    for (i = 0; i < GENCALL_MAX; i++)
        aros_lh(i, 0);

    for (i = 0; i < GENCALL_MAX; i++)
        aros_lh(i, 1);

    printf("\n");
    printf("#define __AROS_CPU_SPECIFIC_LC\n\n");
	
    for (i = 0; i < GENCALL_MAX; i++)
        aros_lc(i, "");

    for (i = 0; i < GENCALL_MAX; i++)
        aros_lc(i, "NR");

    for (i = 0; i < GENCALL_MAX; i++)
        aros_call(i, "");

    for (i = 0; i < GENCALL_MAX; i++)
        aros_call(i, "NR");

    printf("\n");
    printf("#define __AROS_CPU_SPECIFIC_LD\n\n");
	
    for (i = 0; i < GENCALL_MAX; i++)
        aros_ld(i, 0);

    for (i = 0; i < GENCALL_MAX; i++)
        aros_ld(i, 1);

    printf("%s\n", extra);

    printf("#endif /* AROS_LIBCALL_GCCREG_H */\n");
    return 0;
}
