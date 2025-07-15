/*
    Copyright (C) 1995-2025, The AROS Development Team. All rights reserved.
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

#include "env.h"
#include "misc.h"
#include "docommand.h"
#include "backend.h"
#include "ldscript.h"
#include "gensets.h"

#define EXTRA_ARG_CNT 2

#define EI_OSABI        7
#define EI_ABIVERSION   8

#define ELFOSABI_AROS   15


static char *ldscriptname, *tempoutput, *ld_name, *strip_name;
static FILE *ldscriptfile;

int have_gnunm = 0;

static void exitfunc(void)
{
    if (ldscriptfile != NULL)
        fclose(ldscriptfile);

    if (ldscriptname != NULL)
        remove(ldscriptname);

    if (tempoutput != NULL)
        remove(tempoutput);
}

static int set_os_and_abi(const char *file)
{
    int f;
    const unsigned char osabi = ELFOSABI_AROS;
    const unsigned char abiversion = 1;

    /* Modify OS and ABI fields */

    f = open(file, O_RDWR);
    if (f >= 0) {
        lseek(f, EI_OSABI, SEEK_SET);
        if (write(f, &osabi, 1) == 1) {
            lseek(f, EI_ABIVERSION, SEEK_SET);
            if (write(f, &abiversion, 1) == 1) {
                close(f);
                return 1;
            }
        }
    }

    perror(file);
    if (f >= 0)
            close(f);
    return 0;
}

struct libentry
{
    int idx;
    char *str;
};

int main(int argc, char *argv[])
{
    struct libentry llibs[argc];
    struct libentry rellibs[argc];
    char *output, **ldargs;
    char *do_verbose = NULL;
    /* incremental = 1 -> don't do final linking.
       incremental = 2 -> don't do final linking AND STILL produce symbol sets.  */
    int incremental = 0, ignore_undefined_symbols = 0;
    int strip_all   = 0;
    int cnt, i, libcnt = 0, relcnt = 0;
    char *linkAddress = NULL;

    setnode *setlist = NULL, *liblist = NULL, *extralist = NULL;

    program_name = argv[0];
    ld_name = LD_NAME;
    strip_name = STRIP_NAME;

    /* Do some stuff with the arguments */
    output = "a.out";
    for (cnt = 1; argv[cnt]; cnt++)
    {
        /* We've encountered an option */
        if (argv[cnt][0]=='-')
        {
            /* Get the output file name */
            if (argv[cnt][1]=='o')
                output = argv[cnt][2]?&argv[cnt][2]:argv[++cnt];
            else
            /* Incremental linking is requested */
            if ((argv[cnt][1]=='r' || argv[cnt][1]=='i') && argv[cnt][2]=='\0')
                incremental  = 1;
            else
            /* Incremental, but produce the symbol sets */
            if (strncmp(&argv[cnt][1], "Ur", 3) == 0)
            {
                incremental  = 2;
                
                argv[cnt][1] = 'r';  /* Just some non-harming option... */
                argv[cnt][2] = '\0';
            }
            else
            /* linklib */
            if (strncmp(&argv[cnt][1], "l", 1) == 0)
            {
                int lnlen = strlen(&argv[cnt][2]);
                if ((lnlen > 4) && (strcmp("_rel", &argv[cnt][lnlen - 2]) == 0))
                {
                    rellibs[relcnt].idx = cnt;
                    rellibs[relcnt++].str = &argv[cnt][2];
                }
                else
                {
                    llibs[libcnt].idx = cnt;
                    llibs[libcnt++].str = &argv[cnt][2];
                }
            }
            else
            /* Complete stripping is requested, but we do it our own way */
            /* Ignoring of missing symbols is requested */
            if (strncmp(&argv[cnt][1], "ius", 4) == 0)
            {
                ignore_undefined_symbols = 1;
                argv[cnt][1] = 'r';  /* Just some non-harming option... */
                argv[cnt][2] = '\0';
            }
            else
            /* Complete stripping is requested, but we do it our own way */
            if (argv[cnt][1]=='s' && argv[cnt][2]=='\0')
            {
                strip_all = 1;
                argv[cnt][1] = 'r'; /* Just some non-harming option... */
            }
            else
            /* Start address specified .. */
            if ((strncmp(&argv[cnt][1], "Ttext", 6) == 0) || ((strncmp(&argv[cnt][1], "Wl,-Ttext", 10) == 0)))
            {
                if (argv[cnt][1] =='T')
                    linkAddress = &argv[cnt][7];
                else
                    linkAddress = &argv[cnt][11];
            }
            else
            /* Use the specified linker.. */
            if ((strncmp(&argv[cnt][1], "-ld-path", 8) == 0) || ((strncmp(&argv[cnt][1], "Wl,--ld-path", 12) == 0)))
            {
                if (argv[cnt][1] =='-')
                    ld_name = &argv[cnt][10];
                else
                    ld_name = &argv[cnt][14];
                argv[cnt][1] = 'r';  /* Just some non-harming option... */
                argv[cnt][2] = '\0';
                break;
            }
            else
            /* The user just requested help info, don't do anything else */
            if (strncmp(&argv[cnt][1], "-help", 6) == 0)
            {
                /* I know, it's not incremental linking we're after, but the end result
                   is the same */
                incremental = 1;
                break;
            }
            else
            /* verbose output */
            if (strncmp(&argv[cnt][1], "-verbose", 9) == 0)
            {
                do_verbose = argv[cnt];
                break;
            }
        }
    }

    ldargs = xmalloc(sizeof(char *) * (argc + EXTRA_ARG_CNT
        + ((incremental == 1) ? 0 : 2) + 1));

    ldargs[0] = ld_name;
    ldargs[1] = OBJECT_FORMAT;
    ldargs[2] = "-r";

    for (i = 1; i < argc; i++)
        ldargs[i + EXTRA_ARG_CNT] = argv[i];
    cnt = argc + EXTRA_ARG_CNT;

    if (incremental != 1)
    {
        atexit(exitfunc);
        if
        (
            !(tempoutput   = make_temp_file(NULL))     ||
            !(ldscriptname = make_temp_file(NULL))     ||
            !(ldscriptfile = fopen(ldscriptname, "w"))
        )
        {
            fatal(ldscriptname ? ldscriptname : "make_temp_file()", strerror(errno));
        }

        ldargs[cnt++] = "-o";
        ldargs[cnt++] = tempoutput;
    }

    // Fixup relbase lib linking 
    if (relcnt > 0)
    {
        int relid, libid;
        for (relid = 0; relid < relcnt; relid++)
            for (libid = 0; libid < libcnt; libid++)
            {
                if (!strncmp(llibs[libid].str, rellibs[relid].str, strlen(llibs[libid].str)))
                {
                    ldargs[llibs[libid].idx + EXTRA_ARG_CNT] = ldargs[rellibs[relid].idx + EXTRA_ARG_CNT];
                }
            }
    }
      
    ldargs[cnt] = NULL;
              
    docommandvp(ld_name, ldargs);

    if (incremental == 1)
        return set_os_and_abi(output) ? EXIT_SUCCESS : EXIT_FAILURE;

    /*
       * initalize the backend and process the 
       * remaining work
       */
    backend_init(ldargs[0]);
    collect_sets(tempoutput, &setlist);
    collect_extra(tempoutput, &extralist);
    collect_libs(tempoutput, &liblist);

    if (linkAddress != NULL)
        fprintf(ldscriptfile, "ENTRY(_start)\n");

    if (setlist) {
        struct setnode *n;
        for (n = setlist; n; n = n->next) {
            if (strncmp(n->secname,".aros.set.",10) == 0) {
               fprintf(ldscriptfile, "EXTERN(__%s__symbol_set_handler_missing)\n", &n->secname[10]);
            }
        }
    }

    fwrite(LDSCRIPT_PART1, sizeof(LDSCRIPT_PART1) - 1, 1, ldscriptfile);
    if (linkAddress != NULL)
        fprintf(ldscriptfile, "  . = %s;\n", linkAddress);
#ifdef TARGET_FORMAT_EXE
    fprintf(ldscriptfile, "  .tag  :\n");
#else
    if (linkAddress != NULL)
        fprintf(ldscriptfile, "  .tag   :\n");
    else
        fprintf(ldscriptfile, "  .tag 0 :\n");
#endif
    fwrite(LDSCRIPT_PART2A, sizeof(LDSCRIPT_PART2A) - 1, 1, ldscriptfile);
    if (linkAddress != NULL)
        fprintf(ldscriptfile, "\n");
    else
        fprintf(ldscriptfile, "=0x90909090\n");
#ifdef TARGET_FORMAT_EXE
    fprintf(ldscriptfile, "  . = SEGMENT_START(\"text-segment\", 0x80000000);\n");
    fprintf(ldscriptfile, "  .text  :\n");
#else
    if (linkAddress != NULL)
        fprintf(ldscriptfile, "  .text   :\n");
    else
        fprintf(ldscriptfile, "  .text 0 :\n");
#endif
    fwrite(LDSCRIPT_PART2B, sizeof(LDSCRIPT_PART2B) - 1, 1, ldscriptfile);
    if (linkAddress != NULL)
        fprintf(ldscriptfile, "    KEEP(*(.text._start))\n");
    fwrite(LDSCRIPT_PART3A, sizeof(LDSCRIPT_PART3A) - 1, 1, ldscriptfile);
    if (linkAddress != NULL)
        fprintf(ldscriptfile, "\n\n");
    else
        fprintf(ldscriptfile, "=0x90909090\n\n");
#ifdef TARGET_FORMAT_EXE
    fprintf(ldscriptfile, "  . = SEGMENT_START(\"rodata-segment\", . );\n");
    fprintf(ldscriptfile, "  .rodata   :\n");
#else
    if (linkAddress != NULL)
        fprintf(ldscriptfile, "  .rodata    :\n");
    else
        fprintf(ldscriptfile, "  .rodata  0 :\n");
#endif
    fwrite(LDSCRIPT_PART3B, sizeof(LDSCRIPT_PART3B) - 1, 1, ldscriptfile);
    emit_sets(setlist, ldscriptfile);
    emit_libs(liblist, ldscriptfile);
    fprintf(ldscriptfile, "  }\n");
#ifdef TARGET_FORMAT_EXE
    fprintf(ldscriptfile, "  .rodata1  : { *(.rodata1) }\n");
#else
    if (linkAddress != NULL)
        fprintf(ldscriptfile, "  .rodata1   : { *(.rodata1) }\n");
    else
        fprintf(ldscriptfile, "  .rodata1 0 : { *(.rodata1) }\n");
#endif
    fwrite(LDSCRIPT_PART4A, sizeof(LDSCRIPT_PART4A) - 1, 1, ldscriptfile);
#ifdef TARGET_FORMAT_EXE
    fprintf(ldscriptfile, "  .sdata2   : { *(.sdata2) *(.sdata2.*) *(.gnu.linkonce.s2.*) }\n");
    fprintf(ldscriptfile, "  .sbss2    : { *(.sbss2)  *(.sbss2.*)  *(.gnu.linkonce.sb2.*) }\n\n");
    fprintf(ldscriptfile, "  .data   :\n");
#else
    if (linkAddress != NULL)
    {
        fprintf(ldscriptfile, "  .sdata2    : { *(.sdata2) *(.sdata2.*) *(.gnu.linkonce.s2.*) }\n");
        fprintf(ldscriptfile, "  .sbss2     : { *(.sbss2)  *(.sbss2.*)  *(.gnu.linkonce.sb2.*) }\n\n");
        fprintf(ldscriptfile, "  .data      :\n");
    }
    else
    {
        fprintf(ldscriptfile, "  .sdata2  0 : { *(.sdata2) *(.sdata2.*) *(.gnu.linkonce.s2.*) }\n");
        fprintf(ldscriptfile, "  .sbss2   0 : { *(.sbss2)  *(.sbss2.*)  *(.gnu.linkonce.sb2.*) }\n\n");
        fprintf(ldscriptfile, "  .data    0 :\n");
    }
#endif
    fwrite(LDSCRIPT_PART4B, sizeof(LDSCRIPT_PART4B) - 1, 1, ldscriptfile);
#ifdef TARGET_FORMAT_EXE
#else
    if (linkAddress != NULL)
        fprintf(ldscriptfile, "  .data1              : { *(.data1) }\n");
    else
        fprintf(ldscriptfile, "  .data1            0 : { *(.data1) }\n");
#endif
#if defined(TARGET_CPU_i386) || defined(TARGET_CPU_x86_64)
    fwrite(LDSCRIPT_PART4C, sizeof(LDSCRIPT_PART4C) - 1, 1, ldscriptfile);
#endif
    fwrite(LDSCRIPT_PART4D, sizeof(LDSCRIPT_PART4D) - 1, 1, ldscriptfile);
#ifdef TARGET_FORMAT_EXE
    fprintf(ldscriptfile, "  .eh_frame          :\n");
#else
    if (linkAddress != NULL)
        fprintf(ldscriptfile, "  .eh_frame           :\n");
    else
        fprintf(ldscriptfile, "  .eh_frame         0 :\n");
#endif
    fwrite(LDSCRIPT_PART4E, sizeof(LDSCRIPT_PART4E) - 1, 1, ldscriptfile);
    /* Append .eh_frame terminator only on final stage */
    if (incremental == 0)
        fputs("LONG(0)\n", ldscriptfile);
    fwrite(LDSCRIPT_PART5A, sizeof(LDSCRIPT_PART5A) - 1, 1, ldscriptfile);
#ifdef TARGET_FORMAT_EXE
    fprintf(ldscriptfile, "  .gcc_except_table  : { *(.gcc_except_table) }\n");
#else
    if (linkAddress != NULL)
        fprintf(ldscriptfile, "  .gcc_except_table   : { *(.gcc_except_table) }\n");
    else
        fprintf(ldscriptfile, "  .gcc_except_table 0 : { *(.gcc_except_table) }\n");
#endif
    fwrite(LDSCRIPT_PART5B, sizeof(LDSCRIPT_PART5B) - 1, 1, ldscriptfile);
#ifdef TARGET_FORMAT_EXE
    fprintf(ldscriptfile, "  .sdata    :\n");
#else
    if (linkAddress != NULL)
        fprintf(ldscriptfile, "  .sdata     :\n");
    else
        fprintf(ldscriptfile, "  .sdata   0 :\n");
#endif
    fwrite(LDSCRIPT_PART5C, sizeof(LDSCRIPT_PART5C) - 1, 1, ldscriptfile);
#ifdef TARGET_FORMAT_EXE
    fprintf(ldscriptfile, "  .sbss  :\n");
#else
    if (linkAddress != NULL)
        fprintf(ldscriptfile, "  .sbss   :\n");
    else
        fprintf(ldscriptfile, "  .sbss 0 :\n");
#endif
    fwrite(LDSCRIPT_PART5D, sizeof(LDSCRIPT_PART5D) - 1, 1, ldscriptfile);
#ifdef TARGET_FORMAT_EXE
    fprintf(ldscriptfile, "  .bss  :\n");
#else
    if (linkAddress != NULL)
        fprintf(ldscriptfile, "  .bss   :\n");
    else
        fprintf(ldscriptfile, "  .bss 0 :\n");
#endif
    fwrite(LDSCRIPT_PART5E, sizeof(LDSCRIPT_PART5E) - 1, 1, ldscriptfile);
    fwrite(LDSCRIPT_PART6, sizeof(LDSCRIPT_PART6) - 1, 1, ldscriptfile);

    fclose(ldscriptfile);
    ldscriptfile = NULL;
    
    char **cmdargv;
    int argallocs = 0;
    if (extralist) {
        struct setnode *n;
        for (n = extralist; n; n = n->next)
            argallocs += 1;
    }

#ifdef TARGET_FORMAT_EXE
    if (incremental == 0)
    {
#ifdef OBJECT_FORMAT_EXTRA_FINAL
        argallocs += 10;
        cmdargv = xmalloc(sizeof(char *) * (argallocs + 1));
        cmdargv[0] = ld_name;
        cmdargv[1] = OBJECT_FORMAT;
        cmdargv[2] = OBJECT_FORMAT_EXTRA_FINAL;
        cmdargv[3] = "-o";
        cmdargv[4] = output;
        cmdargv[5] = tempoutput;

        cnt = 0;
        if (extralist) {
        struct setnode *n;
        for (n = extralist; n; n = n->next)
            cmdargv[6 + cnt++] = n->secname;
        }
    
        cmdargv[6 + cnt] = "-T";
        cmdargv[7 + cnt] = ldscriptname;
        cmdargv[8 + cnt] = do_verbose;
        cmdargv[9 + cnt] = NULL;

        docommandvp(ld_name, cmdargv);
#else
        argallocs += 9;
        cmdargv = xmalloc(sizeof(char *) * (argallocs + 1));
        cmdargv[0] = ld_name;
        cmdargv[1] = OBJECT_FORMAT;
        cmdargv[2] = "-o";
        cmdargv[3] = output;
        cmdargv[4] = tempoutput;
        
        cnt = 0;
        if (extralist) {
        struct setnode *n;
        for (n = extralist; n; n = n->next)
            cmdargv[6 + cnt++] = n->secname;
        }
    
        cmdargv[5 + cnt] = "-T";
        cmdargv[6 + cnt] = ldscriptname;
        cmdargv[7 + cnt] = do_verbose;
        cmdargv[8 + cnt] = NULL;

        docommandvp(ld_name, cmdargv);
#endif
    }
    else
    {
        argallocs += 10;
        cmdargv = xmalloc(sizeof(char *) * (argallocs + 1));
        cmdargv[0] = ld_name;
        cmdargv[1] = OBJECT_FORMAT;
        cmdargv[2] = "-r";
        cmdargv[3] = "-o";
        cmdargv[4] = output;
        cmdargv[5] = tempoutput;

        cnt = 0;
        if (extralist) {
        struct setnode *n;
        for (n = extralist; n; n = n->next)
            cmdargv[6 + cnt++] = n->secname;
        }
    
        cmdargv[6 + cnt] = "-T";
        cmdargv[7 + cnt] = ldscriptname;
        cmdargv[8 + cnt] = do_verbose;
        cmdargv[9 + cnt] = NULL;

        docommandvp(ld_name, cmdargv);
    }
#else
#ifdef OBJECT_FORMAT_EXTRA_FINAL
    if (incremental == 0)
    {
        argallocs += 11;
        cmdargv = xmalloc(sizeof(char *) * (argallocs + 1));
        cmdargv[0] = ld_name;
        cmdargv[1] = OBJECT_FORMAT;
        cmdargv[2] = OBJECT_FORMAT_EXTRA_FINAL;
        cmdargv[3] = "-r";
        cmdargv[4] = "-o";
        cmdargv[5] = output;
        cmdargv[6] = tempoutput;
        
        cnt = 0;
        if (extralist) {
        struct setnode *n;
        for (n = extralist; n; n = n->next)
            cmdargv[6 + cnt++] = n->secname;
        }
    
        cmdargv[7 + cnt] = "-T";
        cmdargv[8 + cnt] = ldscriptname;
        cmdargv[9 + cnt] = do_verbose;
        cmdargv[10 + cnt] = NULL;

        docommandvp(ld_name, cmdargv);
    }
    else
#endif
    {
        argallocs += 10;
        cmdargv = xmalloc(sizeof(char *) * (argallocs + 1));
        cmdargv[0] = ld_name;
        cmdargv[1] = OBJECT_FORMAT;
        cmdargv[2] = "-r";
        cmdargv[3] = "-o";
        cmdargv[4] = output;
        cmdargv[5] = tempoutput;

        cnt = 0;
        if (extralist) {
        struct setnode *n;
        for (n = extralist; n; n = n->next)
            cmdargv[6 + cnt++] = n->secname;
        }
    
        cmdargv[6 + cnt] = "-T";
        cmdargv[7 + cnt] = ldscriptname;
        cmdargv[8 + cnt] = do_verbose;
        cmdargv[9 + cnt] = NULL;

        docommandvp(ld_name, cmdargv);
    }
#endif

    if (incremental != 0)
        return set_os_and_abi(output) ? EXIT_SUCCESS : EXIT_FAILURE;
        
    if (!ignore_undefined_symbols && check_and_print_undefined_symbols(output))
    {
        remove(output);
        return EXIT_FAILURE;
    }

    chmod(output, 0766);

    if (strip_all)
    {
        docommandlp(strip_name, strip_name, "--strip-unneeded", output, NULL);
    }

    if (!set_os_and_abi(output))
    {
        remove(output);
        return EXIT_FAILURE;
    }

    return 0;
}
