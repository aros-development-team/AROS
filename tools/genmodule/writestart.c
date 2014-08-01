/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    $Id$

    Print the library magic and init code in the file modname_start.c.
    This code is partly based on code in CLib37x.lha from Andreas R. Kleinert
*/
#include "genmodule.h"
#include "oopsupport.h"
#include "muisupport.h"
#include "dtsupport.h"
#include "boopsisupport.h"

static void writedecl(FILE *, struct config *);
static void writedeclsets(FILE *, struct config *);
static void writeresident(FILE *, struct config *);
static void writeinitlib(FILE *, struct config *);
static void writehandler(FILE *, struct config *);
static void writeopenlib(FILE *, struct config *);
static void writecloselib(FILE *, struct config *);
static void writeexpungelib(FILE *, struct config *);
static void writeextfunclib(FILE *, struct config *);
static void writefunctable(FILE *, struct config *);
static void writesets(FILE *, struct config *);

static inline const char *upname(const char *s)
{
    static char name[512];
    int i = 0;

    while (s && i < (sizeof(name)-1))
        name[i++] = toupper(*(s++));
    name[i] = 0;

    return &name[0];
}


/* Some general info on the function of the generated code and how they work
   for .library modules (TODO: add docs for other module types):

   The code in this file will write out the inittable, the function table,
   the InitLib, OpenLib, CloseLib and ExpungeLib function.
   InitLib will call the functions in the ADD2INIT symbolset.
   OpenLib will call the functions in the ADD2OPENLIB symbolset.
   CloseLib will call the functions in the ADD2CLOSELIB symbolset.
   ExpungeLib will call the fucntions in the ADD2EXIT symbolset.

   Currently 3 types of libraries are supported: 1 libbase, a per-opener
   libbase and a per-task libbase.
   * For 1 libbase the libbase is initialized once in InitLib and
     then returned for each call to OpenLib. OpenLib and CloseLib keep track
     of the times the libraries is still open; if it is 0 the library may be
     expunged.
   * The per-opener libbase will create a new libbase for each call to OpenLib
     After InitLib has called the ADD2INIT functions the fields in the root
     libbase may have been initialized to a certain value. These value are
     copied in the newly generated libbase and then the ADD2OPENLIB functions
     are called with the new libbase.
     A per-opener libbase is indicated by the OPTION_DUPBASE flag in cfg->options.
     Currently, a task memory slot is used to store the libbase when a function
     of the library is entered. With proper support from the compiler this could
     be changed to use a reserved register - probably increasing speed. This
     could be implemented later on without breaking backwards compatibility.
     Special provision has been taken to restore the state of the memory slot
     after CloseLib to what it was when OpenLib is called. This is to handle the
     case properly where the library is used both in the parent and child
     of a Task run with RunCommand().
   * The per-task libbase will create a new libbase for a call to OpenLib when
     there does not exist a libbase yet for this task. This is handled by reserving
     an additional task memory slot. In OpenLib the contents of this memory slot
     is checked and only a new libbase will be generated when the libbase stored
     there is not for the current Task. Also on then the ADD2OPENLIB
     A separate counter is added to count the number of times the libbase is opened
     in the same task. The libbase will also only be closed and removed if this
     counter reaches 0 and only then the ADD2CLOSELIB functions are called.
     A per-task libbase is indicated by both flags OPTION_DUPBASE and
     OPTION_PERTASKBASE in cfg->options.
     It was decided to use an extra slot for storing the per-task libbase to check
     in OpenLib if a new libbase has to created. This is to increase flexibility
     during calling of the library functions. If the library supports this the
     library can still be called with another libbase then the libbase stored in the
     per-task slot, or the library function can be called from another task then the
     one that opened the libbase.
     Also here special provision has been taken to restore the state of the memory
     slot after CloseLib to what it was when OpenLib is called.
*/
void writestart(struct config *cfg)
{
    FILE *out;
    char line[256], *banner;
    struct classinfo *cl;

    snprintf(line, 255, "%s/%s_start.c", cfg->gendir, cfg->modulename);
    out = fopen(line, "w");

    if (out == NULL)
    {
        perror(line);
        exit(20);
    }

    banner = getBanner(cfg);
    fprintf(out, "%s", banner);
    freeBanner(banner);

    fprintf(out,
            "/* For comments and explanation of generated code look in writestart.c source code\n"
            "   of the genmodule program */\n"
    );

    writedecl(out, cfg);
    if (!(cfg->options & OPTION_NORESIDENT))
    {
        writeresident(out, cfg);
        writedeclsets(out, cfg);
        writeinitlib(out, cfg);
        if (cfg->modtype != RESOURCE && cfg->modtype != HANDLER)
        {
            writeopenlib(out, cfg);
            writecloselib(out, cfg);
            writeexpungelib(out, cfg);
            writeextfunclib(out, cfg);
            if (cfg->modtype == MCC || cfg->modtype == MUI || cfg->modtype == MCP)
                writemccquery(out, cfg);
            else if (cfg->modtype == DATATYPE)
                writeobtainengine(out, cfg);
        }
        writesets(out, cfg);
    }

    if (cfg->modtype != HANDLER)
        writefunctable(out, cfg);

    for (cl = cfg->classlist; cl != NULL; cl = cl->next)
    {
        switch (cl->classtype)
        {
        case MCC:
        case MUI:
        case MCP:
            /* Second argument to next call: the class is not the main class if it is not
             * the first class or the modtype is not a MUI class
             */
            writemccinit(cfg, out, cl != cfg->classlist || cfg->modtype != cl->classtype, cl);
            break;
        case GADGET:
        case DATATYPE:
        case CLASS:
        case IMAGE:
            writeclassinit(cfg, out, cl);
            break;
        case HIDD:
            writeoopinit(out, cl);
            break;
        default:
            fprintf(stdout, "Internal error: unsupported classtype in writestart\n");
            exit(20);
        }
    }

    fclose(out);
}


static void writedecl(FILE *out, struct config *cfg)
{
    struct stringlist *linelistit, *s;
    int boopsiinc=0, muiinc=0, oopinc=0;
    struct functionhead *funclistit;
    struct functionarg *arglistit;
    struct classinfo *classlistit;
    char *type, *name;

    if (cfg->modtype == DEVICE)
    {
        fprintf(out,
                "#include <exec/io.h>\n"
                "#include <exec/errors.h>\n"
        );
    }
    fprintf(out,
            "#include <exec/types.h>\n"
            "#include <exec/libraries.h>\n"
            "#include <exec/resident.h>\n"
            "#include <aros/libcall.h>\n"
            "#include <aros/asmcall.h>\n"
            "#include <aros/symbolsets.h>\n"
            "#include <aros/genmodule.h>\n"
            "#include <dos/dos.h>\n"
            "\n"
            "#include \"%s_libdefs.h\"\n"
            "\n"
            , cfg->modulename
    );
    for (s = cfg->rellibs; s; s = s->next)
        fprintf(out,
                "#include <proto/%s.h>\n"
                , s->s
        );
    fprintf(out,
            "\n"
            "#undef SysBase\n"
            "#undef OOPBase\n"
            "#undef UtilityBase\n"
            "\n"
            "#include <proto/exec.h>\n"
            "#include <proto/alib.h>\n"
            "\n"
    );

    /* Write out declaration section provided in the config file */
    for (linelistit = cfg->cdeflines; linelistit != NULL; linelistit = linelistit->next)
    {
        fprintf(out, "%s\n", linelistit->s);
    }

    /* Is there a variable for storing the segList ? */
    if (!(cfg->options & OPTION_NOEXPUNGE) && cfg->modtype!=RESOURCE && cfg->modtype != HANDLER)
    {
        fprintf(out,
                "#ifndef GM_SEGLIST_FIELD\n"
                "static BPTR __attribute__((unused)) GM_UNIQUENAME(seglist);\n"
                "#define GM_SEGLIST_FIELD(LIBBASE) (GM_UNIQUENAME(seglist))\n"
                "#endif\n"
        );
    }
    if (cfg->options & OPTION_DUPBASE)
        fprintf(out,
                "/* Required for TaskStorage manipulation */\n"
                "extern struct ExecBase *SysBase;\n"
        );
    if (cfg->options & OPTION_DUPBASE)
    {
        fprintf(out,
                "#ifndef GM_ROOTBASE_FIELD\n"
                "static LIBBASETYPEPTR GM_UNIQUENAME(rootbase);\n"
                "#define GM_ROOTBASE_FIELD(LIBBASE) (GM_UNIQUENAME(rootbase))\n"
                "#endif\n"
                "struct __GM_DupBase {\n"
                "    LIBBASETYPE base;\n"
                "    LIBBASETYPEPTR oldbase;\n"
        );
        if (cfg->options & OPTION_PERTASKBASE)
        {
            fprintf(out,
                    "    ULONG taskopencount;\n"
                    "    struct Task *task;\n"
                    "    APTR retaddr;\n"
                    "    LIBBASETYPEPTR oldpertaskbase;\n"
            );
        }
        fprintf(out,
                "};\n"
        );
        if (cfg->options & OPTION_PERTASKBASE)
        {
            fprintf(out,
                    "static LONG __pertaskslot;\n"
                    "LIBBASETYPEPTR __GM_GetBaseParent(LIBBASETYPEPTR base)\n"
                    "{\n"
                    "    return ((struct __GM_DupBase *)base)->oldpertaskbase;\n"
                    "}\n"
                    "static inline LIBBASETYPEPTR __GM_GetPerTaskBase(void)\n"
                    "{\n"
                    "    return (LIBBASETYPEPTR)GetTaskStorageSlot(__pertaskslot);\n"
                    "}\n"
                    "static inline LIBBASETYPEPTR __GM_GetParentPerTaskBase(void)\n"
                    "{\n"
                    "    return (LIBBASETYPEPTR)GetParentTaskStorageSlot(__pertaskslot);\n"
                    "}\n"
                    "static inline void __GM_SetPerTaskBase(LIBBASETYPEPTR base)\n"
                    "{\n"
                    "    SetTaskStorageSlot(__pertaskslot, (IPTR)base);\n"
                    "}\n"
            );
        }
    }

    /* Set the size of the library base to accomodate the relbases */
    if (cfg->options & OPTION_DUPBASE)
    {
        fprintf(out,
                 "#define LIBBASESIZE (sizeof(struct __GM_DupBase) + sizeof(struct Library *)*%d)\n"
                 , slist_length(cfg->rellibs)

        );
    } else {
        fprintf(out,
                "#define LIBBASESIZE (sizeof(LIBBASETYPE) + sizeof(struct Library *)*%d)\n"
                , slist_length(cfg->rellibs)
        );
    }

    if (cfg->rellibs) {
        struct stringlist *sl;
        int i, n = slist_length(cfg->rellibs);

        for (sl=cfg->rellibs; sl; sl=sl->next, n--) {
            fprintf(out,
                    "#ifndef AROS_RELLIB_OFFSET_%s\n"
                    "#  error '%s' is not a relative library\n"
                    "#endif\n"
                    "const IPTR AROS_RELLIB_OFFSET_%s = LIBBASESIZE-sizeof(struct Library *)*%d;\n"
                    , upname(sl->s)
                    , sl->s
                    , upname(sl->s)
                    , n
            );
        }
    }

    if (cfg->options & OPTION_DUPBASE) {
        fprintf(
            out,
            "static LONG __GM_BaseSlot;\n"
            "char *__aros_getoffsettable(void)\n"
            "{\n"
            "    return (char *)GetTaskStorageSlot(__GM_BaseSlot);\n"
            "}\n"
            "void __aros_setoffsettable(void *base)\n"
            "{\n"
            "    SetTaskStorageSlot(__GM_BaseSlot, (IPTR)base);\n"
            "}\n"
            "%s__aros_getbase_%s(void)\n"
            "{\n"
            "    return (%s)__aros_getoffsettable();\n"
            "}\n",
            cfg->libbasetypeptrextern, cfg->libbase,
            cfg->libbasetypeptrextern
        );
    } else if (cfg->rellibs || (cfg->options & OPTION_STACKCALL)) {
        fprintf(out,
            "#ifdef __aros_getoffsettable\n"
        );
        if ((cfg->options & OPTION_STACKCALL))
            fprintf(out,
             "#error Redefining __aros_setoffsettable is not permitted with stackcall APIs\n"
            );
        else
            fprintf(out,
                    "#define __aros_getbase_%s() __aros_getoffsettable()\n",
                    cfg->libbase
            );
        fprintf(out,
            "#else /* !__aros_getoffsettable */\n"
            "static char *__GM_OffsetTable;\n"
            "char *__aros_getoffsettable(void)\n"
            "{\n"
            "    return __GM_OffsetTable;\n"
            "}\n"
            "BOOL __aros_setoffsettable(char *base)\n"
            "{\n"
            "    __GM_OffsetTable = base;\n"
            "    return TRUE;\n"
            "}\n"
            "%s__aros_getbase_%s(void)\n"
            "{\n"
            "    return (%s)__aros_getoffsettable();\n"
            "}\n"
            "#endif /* __aros_getoffsettable */\n"
            "\n",
            cfg->libbasetypeptrextern, cfg->libbase,
            cfg->libbasetypeptrextern
        );
    }

    if (cfg->rellibs)
    {
        struct stringlist *sl;

        for (sl = cfg->rellibs; sl; sl = sl->next)
        {
            fprintf(out,
                    "AROS_IMPORT_ASM_SYM(void *,__GM_rellib_base_%s,AROS_RELLIB_BASE_%s);\n"
                    , sl->s
                    , upname(sl->s)
            );
        }
    }

    for (classlistit = cfg->classlist; classlistit != NULL; classlistit = classlistit->next)
    {
        /* For the main class basename is the same as the module basename */
        if (strcmp(classlistit->basename, cfg->basename) == 0)
        {
            if (classlistit->classptr_var == NULL)
            {
                fprintf(out,
                        "#if !defined(GM_CLASSPTR_FIELD) && !defined(%s_CLASSPTR_FIELD)\n"
                        "static APTR GM_UNIQUENAME(%sClass);\n"
                        "#define GM_CLASSPTR_FIELD(LIBBASE) (GM_UNIQUENAME(%sClass))\n"
                        "#define %s_CLASSPTR_FIELD(LIBBASE) (GM_UNIQUENAME(%sClass))\n"
                        "#define %s_STORE_CLASSPTR 1\n"
                        "#elif defined(GM_CLASSPTR_FIELD) && !defined(%s_CLASSPTR_FIELD)\n"
                        "#define %s_CLASSPTR_FIELD(LIBBASE) (GM_CLASSPTR_FIELD(LIBBASE))\n"
                        "#elif !defined(GM_CLASSPTR_FIELD) && defined(%s_CLASSPTR_FIELD)\n"
                        "#define GM_CLASSPTR_FIELD(LIBBASE) (%s_CLASSPTR_FIELD(LIBBASE))\n"
                        "#endif\n",
                        classlistit->basename,
                        classlistit->basename,
                        classlistit->basename,
                        classlistit->basename, classlistit->basename,
                        classlistit->basename,
                        classlistit->basename,
                        classlistit->basename,
                        classlistit->basename,
                        classlistit->basename
                );
            }
            else
            {
                fprintf(out,
                        "#define GM_CLASSPTR_FIELD(LIBBASE) (%s)\n"
                        "#define %s_CLASSPTR_FIELD(LIBBASE) (%s)\n"
                        "#define %s_STORE_CLASSPTR 1\n",
                        classlistit->classptr_var,
                        classlistit->basename, classlistit->classptr_var,
                        classlistit->basename
                );
            }
        }
        else
        {
            if (classlistit->classptr_var == NULL)
            {
                fprintf(out,
                        "#if !defined(%s_CLASSPTR_FIELD)\n"
                        "static APTR GM_UNIQUENAME(%sClass);\n"
                        "#define %s_CLASSPTR_FIELD(LIBBASE) (GM_UNIQUENAME(%sClass))\n"
                        "#define %s_STORE_CLASSPTR 1\n"
                        "#endif\n",
                        classlistit->basename,
                        classlistit->basename,
                        classlistit->basename, classlistit->basename,
                        classlistit->basename
                );
            }
            else
            {
                fprintf(out,
                        "#define %s_CLASSPTR_FIELD(LIBBASE) (%s)\n"
                        "#define %s_STORE_CLASSPTR 1\n",
                        classlistit->basename, classlistit->classptr_var,
                        classlistit->basename
                );
            }
        }
    }

    /* Write out the defines for the functions of the function table */
    writefuncdefs(out, cfg, cfg->funclist);
    /* Write internal stubs */
    writefuncinternalstubs(out, cfg, cfg->funclist);
    fprintf(out, "\n");


    /* Write out the includes needed for the classes */
    if (cfg->classlist != NULL)
        writeboopsiincludes(out);

    for (classlistit = cfg->classlist; classlistit != NULL; classlistit = classlistit->next)
    {
        switch (classlistit->classtype)
        {
        case MUI:
        case MCC:
        case MCP:
            if (!muiinc)
            {
                writemuiincludes(out);
                muiinc = 1;
            }
            /* Fall through: also write boopsi includes */
        case GADGET:
        case DATATYPE:
        case CLASS:
        case IMAGE:
            if (!boopsiinc)
            {
                writeboopsiincludes(out);
                boopsiinc = 1;
            }
            break;
        case HIDD:
            if (!oopinc)
            {
                writeoopincludes(out);
                oopinc = 1;
            }
            break;
        default:
            fprintf(stderr, "Internal error: unhandled classtype in writedecl\n");
            exit(20);
        }
    }
}


static void writedeclsets(FILE *out, struct config *cfg)
{
    /* PROGRAM_ENTRIES is marked as handled but are just ignored in modules */
    fprintf(out,
            "THIS_PROGRAM_HANDLES_SYMBOLSET(INIT)\n"
            "THIS_PROGRAM_HANDLES_SYMBOLSET(EXIT)\n"
            "DECLARESET(INIT)\n"
            "DECLARESET(EXIT)\n"
            "THIS_PROGRAM_HANDLES_SYMBOLSET(PROGRAM_ENTRIES)\n"
            "DECLARESET(PROGRAM_ENTRIES)\n"
    );
    if (cfg->modtype != HANDLER)
        fprintf(out,
            "THIS_PROGRAM_HANDLES_SYMBOLSET(CTORS)\n"
            "THIS_PROGRAM_HANDLES_SYMBOLSET(DTORS)\n"
            "DECLARESET(CTORS)\n"
            "DECLARESET(DTORS)\n"
            "THIS_PROGRAM_HANDLES_SYMBOLSET(INIT_ARRAY)\n"
            "THIS_PROGRAM_HANDLES_SYMBOLSET(FINI_ARRAY)\n"
            "DECLARESET(INIT_ARRAY)\n"
            "DECLARESET(FINI_ARRAY)\n"
            "THIS_PROGRAM_HANDLES_SYMBOLSET(INITLIB)\n"
            "THIS_PROGRAM_HANDLES_SYMBOLSET(EXPUNGELIB)\n"
            "DECLARESET(INITLIB)\n"
            "DECLARESET(EXPUNGELIB)\n"
        );
    if (!(cfg->options & OPTION_NOAUTOLIB))
    {
        fprintf(out,
            "THIS_PROGRAM_HANDLES_SYMBOLSET(LIBS)\n"
            "DECLARESET(LIBS)\n"
        );
        if (cfg->rellibs)
            fprintf(out,
            "THIS_PROGRAM_HANDLES_SYMBOLSET(RELLIBS)\n"
            "DECLARESET(RELLIBS)\n"
            );
    }
    if (!(cfg->options & OPTION_NOOPENCLOSE))
        fprintf(out,
            "THIS_PROGRAM_HANDLES_SYMBOLSET(OPENLIB)\n"
            "THIS_PROGRAM_HANDLES_SYMBOLSET(CLOSELIB)\n"
            "DECLARESET(OPENLIB)\n"
            "DECLARESET(CLOSELIB)\n"
        );
    if (cfg->modtype == DEVICE)
        fprintf(out,
            "THIS_PROGRAM_HANDLES_SYMBOLSET(OPENDEV)\n"
            "THIS_PROGRAM_HANDLES_SYMBOLSET(CLOSEDEV)\n"
            "DECLARESET(OPENDEV)\n"
            "DECLARESET(CLOSEDEV)\n"
        );
    if (cfg->classlist != NULL)
        fprintf(out,
                "THIS_PROGRAM_HANDLES_SYMBOLSET(CLASSESINIT)\n"
                "THIS_PROGRAM_HANDLES_SYMBOLSET(CLASSESEXPUNGE)\n"
                "DECLARESET(CLASSESINIT)\n"
                "DECLARESET(CLASSESEXPUNGE)\n"
                "#define ADD2INITCLASSES(symbol, pri) ADD2SET(symbol, CLASSESINIT, pri)\n"
                "#define ADD2EXPUNGECLASSES(symbol, pri) ADD2SET(symbol, CLASSESEXPUNGE, pri)\n"
        );
    fprintf(out, "\n");
}


static void writeresident(FILE *out, struct config *cfg)
{
    char *rt_skip = cfg->addromtag;

    if (rt_skip)
        fprintf(out, "extern const struct Resident %s;\n", rt_skip);
    else
    {
        rt_skip = "GM_UNIQUENAME(End)";
        fprintf(out, "extern const int %s;\n", rt_skip);
    }
    fprintf(out,
            "extern const APTR GM_UNIQUENAME(FuncTable)[];\n"
    );
    if (cfg->options & OPTION_RESAUTOINIT)
        fprintf(out, "static const struct InitTable GM_UNIQUENAME(InitTable);\n");
    fprintf(out,
            "\n"
            "extern const char GM_UNIQUENAME(LibName)[];\n"
            "extern const char GM_UNIQUENAME(LibID)[];\n"
            "extern const char GM_UNIQUENAME(Copyright)[];\n"
            "\n"
    );

    if (cfg->options & OPTION_RESAUTOINIT)
    {
         fprintf(out,
            "#define __freebase(LIBBASE)\\\n"
            "do {\\\n"
            "    UWORD negsize, possize;\\\n"
            "    UBYTE *negptr = (UBYTE *)LIBBASE;\\\n"
            "    negsize = ((struct Library *)LIBBASE)->lib_NegSize;\\\n"
            "    negptr -= negsize;\\\n"
            "    possize = ((struct Library *)LIBBASE)->lib_PosSize;\\\n"
            "    FreeMem (negptr, negsize+possize);\\\n"
            "} while(0)\n"
            "\n");
    }

    fprintf(out,
            "AROS_UFP3 (LIBBASETYPEPTR, GM_UNIQUENAME(InitLib),\n"
            "    AROS_UFPA(LIBBASETYPEPTR, LIBBASE, D0),\n"
            "    AROS_UFPA(BPTR, segList, A0),\n"
            "    AROS_UFPA(struct ExecBase *, sysBase, A6)\n"
            ");\n"
    );
    if (cfg->modtype != RESOURCE && cfg->modtype != HANDLER)
    {
        fprintf(out,
                "AROS_LD1(BPTR, GM_UNIQUENAME(ExpungeLib),\n"
                "    AROS_LDA(LIBBASETYPEPTR, extralh, D0),\n"
                "    LIBBASETYPEPTR, LIBBASE, 3, %s\n"
                ");\n"
                "\n",
                cfg->basename
        );
    }
    fprintf(out,
            "struct Resident const GM_UNIQUENAME(ROMTag) =\n"
            "{\n"
            "    RTC_MATCHWORD,\n"
            "    (struct Resident *)&GM_UNIQUENAME(ROMTag),\n"
            "    (APTR)&%s,\n"
            "    RESIDENTFLAGS,\n"
            "    VERSION_NUMBER,\n",
            rt_skip
    );
    switch (cfg->modtype)
    {
    case LIBRARY:
    case MUI:
    case MCC:
    case MCP:
    case GADGET:
    case DATATYPE:
    case USBCLASS:
    case HIDD:
        fprintf(out, "    NT_LIBRARY,\n");
        break;
    case DEVICE:
        fprintf(out, "    NT_DEVICE,\n");
        break;
    case RESOURCE:
    case HANDLER:
        fprintf(out, "    NT_RESOURCE,\n");
        break;
    default:
        fprintf(stderr, "Internal error: unsupported modtype for NT_...\n");
        exit(20);
        break;
    }
    fprintf(out,
            "    RESIDENTPRI,\n"
            "    (CONST_STRPTR)&GM_UNIQUENAME(LibName)[0],\n"
            "    (CONST_STRPTR)&GM_UNIQUENAME(LibID)[6],\n"
    );
    if (cfg->options & OPTION_RESAUTOINIT)
    {
        fprintf(out,
                "    (APTR)&GM_UNIQUENAME(InitTable)\n"
                "};\n"
                "\n"
                "static struct InitTable\n"
                "{\n"
                "    IPTR              Size;\n"
                "    const APTR       *FuncTable;\n"
                "    struct DataTable *DataTable;\n"
                "    APTR              InitLibTable;\n"
                "}\n"
                "const GM_UNIQUENAME(InitTable) =\n"
                "{\n"
                "    LIBBASESIZE,\n"
                "    &GM_UNIQUENAME(FuncTable)[0],\n"
                "    NULL,\n"
                "    (APTR)GM_UNIQUENAME(InitLib)\n"
                "};\n"
        );
    }
    else
        fprintf(out, "    (APTR)GM_UNIQUENAME(InitLib)\n};\n");

    fprintf(out,
            "\n"
            "const char GM_UNIQUENAME(LibName)[] = MOD_NAME_STRING;\n"
            "const char GM_UNIQUENAME(LibID)[] = VERSION_STRING;\n"
            "const char GM_UNIQUENAME(Copyright)[] = COPYRIGHT_STRING;\n"
            "\n"
    );
}

static void writehandler(FILE *out, struct config *cfg)
{
    int i, handlers=0;
    struct handlerinfo *hl;
    int need_fse = 0, need_dos = 0;

    fprintf(out,
               "\n"
               "#include <resources/filesysres.h>\n"
               "#include <aros/system.h>\n"
               "#include <proto/arossupport.h>\n"
               "#include <proto/expansion.h>\n"
               "\n"
               );

    for (hl = cfg->handlerlist; hl != NULL; hl = hl->next) {
        if (hl->type == HANDLER_DOSNODE)
            need_dos = 1;
        else
            need_fse = 1;
        handlers++;
    }

    fprintf(out,
           "\n"
           "LONG %s(struct ExecBase *sysBase);\n"
           "extern const LONG const __aros_libreq_SysBase __attribute__((weak));\n"
           "\n"
           "__startup AROS_PROCH(GM_UNIQUENAME(Handler), argptr, argsize, SysBase)\n"
           "{\n"
           "    AROS_PROCFUNC_INIT\n"
           "\n"
           "    LONG ret = RETURN_FAIL;\n"
           "\n"
           "    if (!SysBase || SysBase->LibNode.lib_Version < __aros_libreq_SysBase)\n"
           "        return ERROR_INVALID_RESIDENT_LIBRARY;\n"
           "    if (set_call_funcs(SETNAME(INIT), 1, 1)) {\n"
           "        ret = %s(SysBase);\n"
           "        set_call_funcs(SETNAME(EXIT), -1, 0);\n"
           "    }\n"
           "\n"
           "    return ret;\n"
           "\n"
           "    AROS_PROCFUNC_EXIT\n"
           "}\n"
           , cfg->handlerfunc
           , cfg->handlerfunc
    );

    fprintf(out,
            "\n"
            "static inline BOOL GM_UNIQUENAME(InitHandler)(struct ExecBase *SysBase)\n"
            "{\n"
           );

    if (!need_dos && !need_fse) {
        fprintf(out,
            "    return TRUE;\n"
            "}\n"
        );
        return;
    }

    fprintf(out,
            "    BPTR seg;\n"
           );

    if (need_dos) {
        fprintf(out,
            "    struct Library *ExpansionBase;\n"
        );
    }
    if (need_fse) {
        fprintf(out,
            "    struct FileSysResource *fsr;\n"
            "    struct FileSysEntry *fse;\n"
        );
    }
    if (need_fse) {
        fprintf(out,
            "    fsr = (struct FileSysResource *)OpenResource(\"FileSystem.resource\");\n"
            "    if (fsr == NULL)\n"
            "        return FALSE;\n"
        );
    }
    if (need_dos) {
        fprintf(out,
            "    ExpansionBase = OpenLibrary(\"expansion.library\", 36);\n"
            "    if (ExpansionBase == NULL)\n"
            "        return FALSE;\n"
        );
    }
    fprintf(out,
            "    seg = CreateSegList(GM_UNIQUENAME(Handler));\n"
            "    if (seg != BNULL) {\n"
    );
    for (hl = cfg->handlerlist; hl != NULL; hl = hl->next)
    {
        switch (hl->type)
        {
        case HANDLER_DOSNODE:
            fprintf(out,
            "\n"
            "        {\n"
            "            struct DeviceNode *node;\n"
            "            IPTR pp[] = { \n"
            "                (IPTR)\"%s\",\n"
            "                (IPTR)NULL,\n"
            "                (IPTR)0,\n"
            "                (IPTR)0,\n"
            "                (IPTR)0\n"
            "            };\n"
            "            node = MakeDosNode((APTR)pp);\n"
            "            if (node) {\n"
            "                node->dn_StackSize = %u;\n"
            "                node->dn_SegList = seg;\n"
            "                node->dn_Startup = (BPTR)%d;\n"
            "                node->dn_Priority = %d;\n"
            "                node->dn_GlobalVec = (BPTR)(SIPTR)-1;\n"
            "                AddBootNode(%d, 0, node, NULL);\n"
            "            }\n"
            "        }\n"
            "\n"
            , hl->name
            , hl->stacksize
            , hl->startup
            , hl->priority
            , hl->bootpri
            );
            break;
        case HANDLER_RESIDENT:
        case HANDLER_DOSTYPE:
            fprintf(out,
                   "\n"
                   "        /* Check to see if we can allocate the memory for the fse */\n"
                   "        fse = AllocMem(sizeof(*fse), MEMF_CLEAR);\n"
                   "        if (fse) {\n"
                   "            fse->fse_Node.ln_Name = VERSION_STRING;\n"
                   "            fse->fse_Node.ln_Pri  = %d;\n"
                   "            fse->fse_DosType = 0x%08x;\n"
                   "            fse->fse_Version = (MAJOR_VERSION << 16) | MINOR_VERSION;\n"
                   "            fse->fse_PatchFlags = FSEF_SEGLIST | FSEF_GLOBALVEC | FSEF_PRIORITY;\n"
                   , hl->autodetect
                   , hl->id
               );
            if (hl->stacksize)
            {
                fprintf(out,
                   "            fse->fse_PatchFlags |= FSEF_STACKSIZE;\n"
                   "            fse->fse_StackSize = %d;\n"
                   , hl->stacksize
                   );
            }
            if (hl->name)
                fprintf(out,
                   "            fse->fse_PatchFlags |= FSEF_HANDLER;\n"
                   "            fse->fse_Handler = AROS_CONST_BSTR(\"%s\");\n"
                   , hl->name);
            fprintf(out,
                   "            fse->fse_Priority = %d;\n"
                   "            fse->fse_SegList = seg;\n"
                   "            fse->fse_GlobalVec = (BPTR)(SIPTR)-1;\n"
                   "            fse->fse_Startup   = (BPTR)%d;\n"
                   "\n"
                   "            /* Add to the list. I know forbid and permit are\n"
                   "             * a little unnecessary for the pre-multitasking state\n"
                   "             * we should be in at this point, but you never know\n"
                   "             * who's going to blindly copy this code as an example.\n"
                   "             */\n"
                   "            Forbid();\n"
                   "            Enqueue(&fsr->fsr_FileSysEntries, (struct Node *)fse);\n"
                   "            Permit();\n"
                   "        }\n"
                   , hl->priority
                   , hl->startup
                  );
            break;
        }
    }
    fprintf(out,
            "    }\n"
            );
    if (need_dos) {
        fprintf(out,
            "    CloseLibrary(ExpansionBase);\n"
        );
    }
    fprintf(out,
            "    return TRUE;\n"
            "}\n"
            "\n"
    );
}

static void writeinitlib(FILE *out, struct config *cfg)
{
    if (cfg->handlerlist)
        writehandler(out, cfg);

    fprintf(out,
            "extern const LONG const __aros_libreq_SysBase __attribute__((weak));\n"
            "\n"
            "AROS_UFH3 (LIBBASETYPEPTR, GM_UNIQUENAME(InitLib),\n"
            "    AROS_UFHA(LIBBASETYPEPTR, LIBBASE, D0),\n"
            "    AROS_UFHA(BPTR, segList, A0),\n"
            "    AROS_UFHA(struct ExecBase *, sysBase, A6)\n"
            ")\n"
            "{\n"
            "    AROS_USERFUNC_INIT\n"
    );

    if (cfg->modtype == HANDLER) {
        fprintf(out,
             "\n"
             "    GM_UNIQUENAME(InitHandler)(sysBase);\n"
             "    return LIBBASE;\n"
             "\n"
             "    AROS_USERFUNC_EXIT\n"
             "}\n"
             "\n"
        );
        return;
    }

    fprintf(out,
            "\n"
            "    int ok;\n"
            "    int initcalled = 0;\n"
    );
    /* Set the global SysBase, needed for __aros_setoffsettable()/__aros_getoffsettable() */
    if (cfg->options & OPTION_DUPBASE)
        fprintf(out,
            "    SysBase = sysBase;\n"
        );
    else
        fprintf(out,
            "    struct ExecBase *SysBase = sysBase;\n"
        );
    fprintf(out,
            "\n"
            "#ifdef GM_SYSBASE_FIELD\n"
            "    GM_SYSBASE_FIELD(LIBBASE) = (APTR)SysBase;\n"
            "#endif\n"
            "    if (!SysBase || SysBase->LibNode.lib_Version < __aros_libreq_SysBase)\n"
            "        return NULL;\n"
            "\n"
    );

    if (cfg->options & OPTION_RESAUTOINIT) {
        fprintf(out,
                "#ifdef GM_OOPBASE_FIELD\n"
                "    GM_OOPBASE_FIELD(LIBBASE) = OpenLibrary(\"oop.library\",0);\n"
                "    if (GM_OOPBASE_FIELD(LIBBASE) == NULL)\n"
                "        return NULL;\n"
                "#endif\n"
        );
    }

    if (!(cfg->options & OPTION_RESAUTOINIT))
    {
        fprintf(out,
            "    int vecsize;\n"
            "    struct Node *n;\n"
            "    char *mem;\n"
            "\n"
            "    vecsize = FUNCTIONS_COUNT * LIB_VECTSIZE;\n"
            "    if (vecsize > 0)\n"
            "        vecsize = ((vecsize-1)/sizeof(IPTR) + 1)*sizeof(IPTR);\n"
            "    mem = AllocMem(vecsize+sizeof(LIBBASETYPE), MEMF_PUBLIC|MEMF_CLEAR);\n"
            "    if (mem == NULL)\n"
            "         return NULL;\n"
            "    LIBBASE = (LIBBASETYPEPTR)(mem + vecsize);\n"
            "    n = (struct Node *)LIBBASE;\n"
            "    n->ln_Type = NT_RESOURCE;\n"
            "    n->ln_Pri = RESIDENTPRI;\n"
            "    n->ln_Name = (char *)GM_UNIQUENAME(LibName);\n"
            "    MakeFunctions(LIBBASE, (APTR)GM_UNIQUENAME(FuncTable), NULL);\n"
        );
        if ((cfg->modtype != RESOURCE) && (cfg->options & OPTION_SELFINIT))
        {
            fprintf(out,
                    "    ((struct Library*)LIBBASE)->lib_NegSize = vecsize;\n"
                    "    ((struct Library*)LIBBASE)->lib_PosSize = sizeof(LIBBASETYPE);\n"
            );

        }
    }
    else
    {
        fprintf(out,
            "    ((struct Library *)LIBBASE)->lib_Revision = REVISION_NUMBER;\n"
        );
    }

    if (cfg->options & OPTION_DUPBASE)
        fprintf(out,
                "    __GM_BaseSlot = AllocTaskStorageSlot();\n"
                "    if (!SetTaskStorageSlot(__GM_BaseSlot, (IPTR)LIBBASE)) {\n"
                "        FreeTaskStorageSlot(__GM_BaseSlot);\n"
                "        return NULL;\n"
                "    }\n"
        );
    else if (cfg->rellibs || (cfg->options & OPTION_STACKCALL))
        fprintf(out,
                "    __aros_setoffsettable((char *)LIBBASE);\n"
        );
    if (cfg->options & OPTION_PERTASKBASE)
        fprintf(out,
                "    __pertaskslot = AllocTaskStorageSlot();\n"
        );

    if (!(cfg->options & OPTION_NOEXPUNGE) && cfg->modtype!=RESOURCE)
        fprintf(out, "    GM_SEGLIST_FIELD(LIBBASE) = segList;\n");
    if (cfg->options & OPTION_DUPBASE)
        fprintf(out, "    GM_ROOTBASE_FIELD(LIBBASE) = (LIBBASETYPEPTR)LIBBASE;\n");
    fprintf(out, "    if (");
    if (!(cfg->options & OPTION_NOAUTOLIB))
        fprintf(out, "set_open_libraries() && ");
    if (cfg->rellibs)
        fprintf(out, "set_open_rellibraries(LIBBASE) && ");
    fprintf(out,
            "set_call_funcs(SETNAME(INIT), 1, 1) &&"
    );
    if (cfg->classlist != NULL)
        fprintf(out, "set_call_libfuncs(SETNAME(CLASSESINIT), 1, 1, LIBBASE) && ");
    fprintf(out,
            "1)\n"
            "    {\n"
            "        set_call_funcs(SETNAME(CTORS), -1, 0);\n"
            "        set_call_funcs(SETNAME(INIT_ARRAY), 1, 0);\n"
            "\n"
    );

    fprintf(out,
            "        initcalled = 1;\n"
            "        ok = set_call_libfuncs(SETNAME(INITLIB), 1, 1, LIBBASE);\n"
            "    }\n"
            "    else\n"
            "        ok = 0;\n"
            "\n"
            "    if (!ok)\n"
            "    {\n"
            "        if (initcalled)\n"
            "            set_call_libfuncs(SETNAME(EXPUNGELIB), -1, 0, LIBBASE);\n"
            "        set_call_funcs(SETNAME(FINI_ARRAY), -1, 0);\n"
            "        set_call_funcs(SETNAME(DTORS), 1, 0);\n"
            "        set_call_funcs(SETNAME(EXIT), -1, 0);\n"
    );
    if (cfg->classlist != NULL)
        fprintf(out, "        set_call_libfuncs(SETNAME(CLASSESEXPUNGE), -1, 0, LIBBASE);\n");
    if (cfg->rellibs)
        fprintf(out, "        set_close_rellibraries(LIBBASE);\n");
    if (!(cfg->options & OPTION_NOAUTOLIB))
        fprintf(out, "        set_close_libraries();\n");

    if (cfg->options & OPTION_RESAUTOINIT)
    {
        fprintf(out,
            "\n"
            "        __freebase(LIBBASE);\n"
        );
    }
    else
    {
        fprintf(out,
            "\n"
            "        FreeMem(mem, vecsize+LIBBASESIZE);\n"
        );
    }
    fprintf(out,
            "        return NULL;\n"
            "    }\n"
            "    else\n"
            "    {\n"
    );

    if (!(cfg->options & OPTION_RESAUTOINIT) && !(cfg->options & OPTION_SELFINIT))
    {
        switch (cfg->modtype)
        {
        case RESOURCE:
            fprintf(out, "        AddResource(LIBBASE);\n");
            break;

        case DEVICE:
            fprintf(out, "        AddDevice(LIBBASE);\n");

        default:
            /* Everything else is library */
            fprintf(out, "        AddLibrary(LIBBASE);\n");
            break;
        }
    }

    fprintf(out,
            "        return  LIBBASE;\n"
            "    }\n"
            "\n"
            "    AROS_USERFUNC_EXIT\n"
            "}\n"
            "\n"
    );
}

static void writeopenlib(FILE *out, struct config *cfg)
{
    switch (cfg->modtype)
    {
    case RESOURCE:
        fprintf(stderr, "Internal error: writeopenlib called for a resource\n");
        break;
    case HANDLER:
        fprintf(stderr, "Internal error: writeopenlib called for a handler\n");
        break;
    case DEVICE:
        if (cfg->options & OPTION_NOOPENCLOSE)
            fprintf(out,
                "AROS_LD3 (void, GM_UNIQUENAME(OpenLib),\n"
                "    AROS_LDA(struct IORequest *, ioreq, A1),\n"
                "    AROS_LDA(ULONG, unitnum, D0),\n"
                "    AROS_LDA(ULONG, flags, D1),\n"
                "    LIBBASETYPEPTR, LIBBASE, 1, %s\n"
                ");\n",
                cfg->basename
            );
        else
        {
            fprintf(out,
                "AROS_LH3 (void, GM_UNIQUENAME(OpenLib),\n"
                "    AROS_LHA(struct IORequest *, ioreq, A1),\n"
                "    AROS_LHA(IPTR, unitnum, D0),\n"
                "    AROS_LHA(ULONG, flags, D1),\n"
                "    LIBBASETYPEPTR, LIBBASE, 1, %s\n"
                ")\n",
                cfg->basename
            );
            fprintf(out,
                "{\n"
                "    AROS_LIBFUNC_INIT\n"
                "\n"
                "    if ( set_call_libfuncs(SETNAME(OPENLIB), 1, 1, LIBBASE)\n"
                "         && set_call_devfuncs(SETNAME(OPENDEV), 1, 1, LIBBASE, ioreq, unitnum, flags)\n"
                "    )\n"
                "    {\n"
                "        ((struct Library *)LIBBASE)->lib_OpenCnt++;\n"
                "        ((struct Library *)LIBBASE)->lib_Flags &= ~LIBF_DELEXP;\n"
                "\n"
                "        ioreq->io_Message.mn_Node.ln_Type = NT_REPLYMSG;\n"
                "    }\n"
                "    else\n"
                "    {\n"
                "        if (ioreq->io_Error >= 0)\n"
                "            ioreq->io_Error = IOERR_OPENFAIL;\n"
                "    }\n"
                "\n"
                "    return;\n"
                "\n"
                "    AROS_LIBFUNC_EXIT\n"
                "}\n"
                "\n"
            );
        }
        break;
    default:
        if (cfg->options & OPTION_NOOPENCLOSE)
        {
            fprintf(out,
                "AROS_LD1 (LIBBASETYPEPTR, GM_UNIQUENAME(OpenLib),\n"
                "    AROS_LDA (ULONG, version, D0),\n"
                "    LIBBASETYPEPTR, LIBBASE, 1, %s\n"
                ");\n",
                cfg->basename
            );
            return;
        }
        fprintf(out,
                "AROS_LH1 (LIBBASETYPEPTR, GM_UNIQUENAME(OpenLib),\n"
                "    AROS_LHA (ULONG, version, D0),\n"
                "    LIBBASETYPEPTR, LIBBASE, 1, %s\n"
                ")\n"
                "{\n"
                "    AROS_LIBFUNC_INIT\n"
                "\n",
                cfg->basename
        );
        if (!(cfg->options & OPTION_DUPBASE))
        {
            fprintf(out,
                    "    if ( set_call_libfuncs(SETNAME(OPENLIB), 1, 1, LIBBASE) )\n"
                    "    {\n"
                    "        ((struct Library *)LIBBASE)->lib_OpenCnt++;\n"
                    "        ((struct Library *)LIBBASE)->lib_Flags &= ~LIBF_DELEXP;\n"
                    "        return LIBBASE;\n"
                    "    }\n"
                    "\n"
                    "    return NULL;\n"
                    "\n"
                    "    AROS_LIBFUNC_EXIT\n"
                    "}\n"
                    "\n"
            );
        }
        else /* OPTION_DUPBASE */
        {
            fprintf(out,
                    "    struct Library *newlib = NULL;\n"
                    "    UWORD possize = ((struct Library *)LIBBASE)->lib_PosSize;\n"
                    "    LIBBASETYPEPTR oldbase = (LIBBASETYPEPTR)__aros_getbase_%s();\n",
                    cfg->libbase
            );
            if (cfg->options & OPTION_PERTASKBASE)
                fprintf(out,
                        "    struct Task *thistask = FindTask(NULL);\n"
                        "    LIBBASETYPEPTR oldpertaskbase = __GM_GetPerTaskBase();\n"
                        "    if (!oldpertaskbase)\n"
                        "       oldpertaskbase = __GM_GetParentPerTaskBase();\n"
                        "    newlib = (struct Library *)oldpertaskbase;\n"
                        "    if (newlib)\n"
                        "    {\n"
                        "        struct __GM_DupBase *dupbase = (struct __GM_DupBase *)newlib;\n"
                        "        if (dupbase->task != thistask)\n"
                        "            newlib = NULL;\n"
                        "        else if (thistask->tc_Node.ln_Type == NT_PROCESS\n"
                        "                 && dupbase->retaddr != ((struct Process *)thistask)->pr_ReturnAddr\n"
                        "        )\n"
                        "            newlib = NULL;\n"
                        "        else\n"
                        "            dupbase->taskopencount++;\n"
                        "    }\n"
                );

            fprintf(out,
                    "\n"
                    "    if (newlib == NULL)\n"
                    "    {\n"
                    "        newlib = MakeLibrary(GM_UNIQUENAME(InitTable).FuncTable,\n"
                    "                             GM_UNIQUENAME(InitTable).DataTable,\n"
                    "                             NULL,\n"
                    "                             GM_UNIQUENAME(InitTable).Size,\n"
                    "                             (BPTR)NULL\n"
                    "        );\n"
                    "        if (newlib == NULL)\n"
                    "            return NULL;\n"
                    "\n"
                    "        CopyMem(LIBBASE, newlib, possize);\n"
                    "        struct __GM_DupBase *dupbase = (struct __GM_DupBase *)newlib;\n"
                    "        dupbase->oldbase = oldbase;\n"
                    "        __aros_setoffsettable((char *)newlib);\n"
            );
            if (cfg->options & OPTION_PERTASKBASE)
                fprintf(out,
                        "        dupbase->task = thistask;\n"
                        "        if (thistask->tc_Node.ln_Type == NT_PROCESS)\n"
                        "             dupbase->retaddr = ((struct Process *)thistask)->pr_ReturnAddr;\n"
                        "        dupbase->oldpertaskbase = oldpertaskbase;\n"
                        "        dupbase->taskopencount = 1;\n"
                        "        __GM_SetPerTaskBase((LIBBASETYPEPTR)newlib);\n"
                );
            fprintf(out,
                    "\n"
                    "        if (!(set_open_rellibraries(newlib)\n"
                    "              && set_call_libfuncs(SETNAME(OPENLIB), 1, 1, newlib)\n"
                    "             )\n"
                    "        )\n"
                    "        {\n");
            if (cfg->options & OPTION_PERTASKBASE)
                fprintf(out,
                    "            __GM_SetPerTaskBase(oldpertaskbase);\n");
            fprintf(out,
                    "            __freebase(newlib);\n"
                    "            return NULL;\n"
                    "        }\n"
                    "\n"
                    "        ((struct Library *)LIBBASE)->lib_OpenCnt++;\n"
                    "        ((struct Library *)LIBBASE)->lib_Flags &= ~LIBF_DELEXP;\n"
                    "    }\n"
                    "\n"
                    "    return (LIBBASETYPEPTR)newlib;\n"
                    "\n"
                    "    AROS_LIBFUNC_EXIT\n"
                    "}\n"
                    "\n"
            );
        }
    }
}


static void writecloselib(FILE *out, struct config *cfg)
{
    if (cfg->options & OPTION_NOOPENCLOSE)
    {
        if (cfg->modtype != DEVICE)
            fprintf(out,
                "AROS_LD0 (BPTR, GM_UNIQUENAME(CloseLib),\n"
                "    LIBBASETYPEPTR, LIBBASE, 2, %s\n"
                ");\n",
                cfg->basename
            );
        else
            fprintf(out,
                "AROS_LD1(BPTR, GM_UNIQUENAME(CloseLib),\n"
                "    AROS_LDA(struct IORequest *, ioreq, A1),\n"
                "    LIBBASETYPEPTR, LIBBASE, 2, %s\n"
                ");\n",
                cfg->basename
            );
        return;
    }
    if (cfg->modtype != DEVICE)
        fprintf(out,
                "AROS_LH0 (BPTR, GM_UNIQUENAME(CloseLib),\n"
                "    LIBBASETYPEPTR, LIBBASE, 2, %s\n"
                ")\n",
                cfg->basename
        );
    else
        fprintf(out,
                "AROS_LH1(BPTR, GM_UNIQUENAME(CloseLib),\n"
                "    AROS_LHA(struct IORequest *, ioreq, A1),\n"
                "    LIBBASETYPEPTR, LIBBASE, 2, %s\n"
                ")\n",
                cfg->basename
        );

    fprintf(out,
            "{\n"
            "    AROS_LIBFUNC_INIT\n"
            "\n"
    );
    if (cfg->modtype == DEVICE)
        fprintf(out,
                "    if (!set_call_devfuncs(SETNAME(CLOSEDEV), -1, 1, LIBBASE, ioreq, 0, 0))\n"
                "    {\n"
                "        return BNULL;\n"
                "    }\n"
        );
    if (!(cfg->options & OPTION_DUPBASE))
    {
        fprintf(out,
                "    ((struct Library *)LIBBASE)->lib_OpenCnt--;\n"
                "    set_call_libfuncs(SETNAME(CLOSELIB), -1, 0, LIBBASE);\n"
        );
    }
    else /* OPTION_DUPBASE */
    {
        fprintf(out,
                "    LIBBASETYPEPTR rootbase = GM_ROOTBASE_FIELD(LIBBASE);\n"
                "    struct __GM_DupBase *dupbase = (struct __GM_DupBase *)LIBBASE;\n"
                "    __aros_setoffsettable(LIBBASE);\n"
        );
        if (cfg->options & OPTION_PERTASKBASE)
            fprintf(out,
                    "    dupbase->taskopencount--;\n"
                    "    if (dupbase->taskopencount != 0)\n"
                    "        return BNULL;\n"
            );
        fprintf(out,
                "\n"
                "    set_call_libfuncs(SETNAME(CLOSELIB), -1, 0, LIBBASE);\n"
                "    set_close_rellibraries(LIBBASE);\n"
                "    __aros_setoffsettable((char *)dupbase->oldbase);\n"
        );
        if (cfg->options & OPTION_PERTASKBASE)
            fprintf(out,
                    "    __GM_SetPerTaskBase(((struct __GM_DupBase *)LIBBASE)->oldpertaskbase);\n"
            );
        fprintf(out,
                "    __freebase(LIBBASE);\n"
                "    LIBBASE = rootbase;\n"
                "    ((struct Library *)LIBBASE)->lib_OpenCnt--;\n"
                "\n"
        );
    }
    if (!(cfg->options & OPTION_NOEXPUNGE))
        fprintf(out,
                "    if\n"
                "    (\n"
                "        (((struct Library *)LIBBASE)->lib_OpenCnt == 0)\n"
                "        && (((struct Library *)LIBBASE)->lib_Flags & LIBF_DELEXP)\n"
                "    )\n"
                "    {\n"
                "        return AROS_LC1(BPTR, GM_UNIQUENAME(ExpungeLib),\n"
                "                   AROS_LCA(LIBBASETYPEPTR, LIBBASE, D0),\n"
                "                   LIBBASETYPEPTR, LIBBASE, 3, %s\n"
                "        );\n"
                "    }\n",
                cfg->basename
        );
    fprintf(out,
            "\n"
            "    return BNULL;\n"
            "\n"
            "    AROS_LIBFUNC_EXIT\n"
            "}\n"
            "\n"
    );
}


static void writeexpungelib(FILE *out, struct config *cfg)
{
    fprintf(out,
            "AROS_LH1 (BPTR, GM_UNIQUENAME(ExpungeLib),\n"
            "    AROS_LHA(LIBBASETYPEPTR, extralh, D0),\n"
            "    LIBBASETYPEPTR, LIBBASE, 3, %s\n"
            ")\n",
            cfg->basename
    );
    fprintf(out,
            "{\n"
            "    AROS_LIBFUNC_INIT\n"
            "\n"
    );
    if (!(cfg->options & OPTION_NOEXPUNGE))
    {
        if (cfg->options & OPTION_RESAUTOINIT) {
            fprintf(out,
                    "#ifdef GM_SYSBASE_FIELD\n"
                    "    struct ExecBase *SysBase = (struct ExecBase *)GM_SYSBASE_FIELD(LIBBASE);\n"
                    "#endif\n"
            );
        }
        if (cfg->options & OPTION_DUPBASE)
            fprintf(out, "    __aros_setoffsettable(LIBBASE);\n");
        fprintf(out,
                "\n"
                "    if ( ((struct Library *)LIBBASE)->lib_OpenCnt == 0 )\n"
                "    {\n"
                "        BPTR seglist = GM_SEGLIST_FIELD(LIBBASE);\n"
                "\n"
                "        if(!set_call_libfuncs(SETNAME(EXPUNGELIB), -1, 1, LIBBASE))\n"
                "        {\n"
                "            ((struct Library *)LIBBASE)->lib_Flags |= LIBF_DELEXP;\n"
                "            return BNULL;\n"
                "        }\n"
                "\n"
                "        Remove((struct Node *)LIBBASE);\n"
                "\n"
                "        set_call_funcs(SETNAME(FINI_ARRAY), -1, 0);\n"
                "        set_call_funcs(SETNAME(DTORS), 1, 0);\n"
                "        set_call_funcs(SETNAME(EXIT), -1, 0);\n"
        );
        if (cfg->classlist != NULL)
            fprintf(out, "        set_call_libfuncs(SETNAME(CLASSESEXPUNGE), -1, 0, LIBBASE);\n");
        if (cfg->rellibs)
            fprintf(out, "        set_close_rellibraries(LIBBASE);\n");
        if (!(cfg->options & OPTION_NOAUTOLIB))
            fprintf(out, "        set_close_libraries();\n"
                         "#ifdef GM_OOPBASE_FIELD\n"
                         "        CloseLibrary((struct Library *)GM_OOPBASE_FIELD(LIBBASE));\n"
                         "#endif\n"
                    );
        if (cfg->options & OPTION_PERTASKBASE)
            fprintf(out,
                    "        FreeTaskStorageSlot(__pertaskslot);\n"
                    "        __pertaskslot = 0;\n"
            );
        fprintf(out,
                "\n"
                "        __freebase(LIBBASE);\n"
                "\n"
                "        return seglist;\n"
                "    }\n"
                "\n"
                "    ((struct Library *)LIBBASE)->lib_Flags |= LIBF_DELEXP;\n"
        );
    }
    fprintf(out,
            "\n"
            "    return BNULL;\n"
            "\n"
            "    AROS_LIBFUNC_EXIT\n"
            "}\n"
            "\n"
    );
}


static void writeextfunclib(FILE *out, struct config *cfg)
{
    fprintf(out,
            "AROS_LH0 (LIBBASETYPEPTR, GM_UNIQUENAME(ExtFuncLib),\n"
            "    LIBBASETYPEPTR, LIBBASE, 4, %s\n"
            ")\n"
            "{\n"
            "    AROS_LIBFUNC_INIT\n"
            "    return NULL;\n"
            "    AROS_LIBFUNC_EXIT\n"
            "}\n"
            "\n",
            cfg->basename
    );
}


static void
writefunctable(FILE *out,
               struct config *cfg
)
{
    struct functionhead *funclistit = cfg->funclist;
    struct functionarg *arglistit;
    unsigned int lvo;
    int i;
    char *name, *type;
    int lastversion = 0;

    /* lvo contains the number of functions already printed in the functable */
    lvo = 0;

    if (!(cfg->options & OPTION_NORESIDENT))
    {
        fprintf(out,
                "\n"
                "const APTR GM_UNIQUENAME(FuncTable)[]=\n"
                "{\n"
        );
        if (cfg->modtype != RESOURCE && cfg->modtype != HANDLER)
        {
            fprintf(out,
                    "    &AROS_SLIB_ENTRY(GM_UNIQUENAME(OpenLib),%s,1),\n"
                    "    &AROS_SLIB_ENTRY(GM_UNIQUENAME(CloseLib),%s,2),\n"
                    "    &AROS_SLIB_ENTRY(GM_UNIQUENAME(ExpungeLib),%s,3),\n"
                    "    &AROS_SLIB_ENTRY(GM_UNIQUENAME(ExtFuncLib),%s,4),\n",
                    cfg->basename, cfg->basename, cfg->basename, cfg->basename
            );
            lvo += 4;
        }
        if (cfg->modtype == MCC || cfg->modtype == MUI || cfg->modtype == MCP)
        {
            lvo++;
            fprintf(out,
                    "    &AROS_SLIB_ENTRY(MCC_Query,%s,%d),\n",
                    cfg->basename, lvo
            );
        }
        else if (cfg->modtype == DATATYPE)
        {
            lvo++;
            fprintf(out,
                    "    &AROS_SLIB_ENTRY(ObtainEngine,%s,%d),\n",
                   cfg->basename, lvo
            );
        }
    }
    else /* NORESIDENT */
    {
        if (cfg->modtype != RESOURCE && cfg->modtype != HANDLER)
        {
            int neednull = 0;
            struct functionhead *funclistit2;

            if (funclistit->lvo != 1)
            {
                fprintf(stderr, "Module without a generated resident structure has to provide the Open function (LVO==1)\n");
                exit(20);
            }
            else
                funclistit = funclistit->next;

            if (funclistit->lvo != 2)
            {
                fprintf(stderr, "Module without a generated resident structure has to provide the Close function (LVO==2)\n");
                exit(20);
            }
            else
                funclistit = funclistit->next;

            if (funclistit->lvo == 3)
                funclistit = funclistit->next;
            else
                neednull = 1;

            if (funclistit->lvo == 4)
                funclistit = funclistit->next;
            else
                neednull = 1;

            if (neednull)
                fprintf(out,
                        "\n"
                        "AROS_UFH1S(int, %s_null,\n"
                        "          AROS_UFHA(struct Library *, libbase, A6)\n"
                        ")\n"
                        "{\n"
                        "    AROS_USERFUNC_INIT\n"
                        "    return 0;\n"
                        "    AROS_USERFUNC_EXIT\n"
                        "}\n",
                        cfg->modulename
                );

            funclistit = cfg->funclist;
            funclistit2 = funclistit->next;
            fprintf(out,
                    "\n"
                    "const APTR GM_UNIQUENAME(FuncTable)[]=\n"
                    "{\n"
                    "    &AROS_SLIB_ENTRY(%s,%s,%d),\n"
                    "    &AROS_SLIB_ENTRY(%s,%s,%d),\n",
                    funclistit->internalname, cfg->basename, lvo+1,
                    funclistit2->internalname, cfg->basename, lvo+2
            );
            lvo += 2;
            funclistit = funclistit2->next;

            if (funclistit->lvo == 3)
            {
                fprintf(out, "    &AROS_SLIB_ENTRY(%s,%s,%d),\n",
                        funclistit->internalname, cfg->basename, lvo+1
                );
                funclistit = funclistit->next;
            }
            else
                fprintf(out, "    &%s_null,\n", cfg->modulename);
            lvo++;

            if (funclistit->lvo == 4)
            {
                fprintf(out, "    &AROS_SLIB_ENTRY(%s,%s,%d),\n",
                        funclistit->internalname, cfg->basename, lvo+1
                );
                funclistit = funclistit->next;
            }
            else
                fprintf(out, "    &%s_null,\n", cfg->modulename);
            lvo++;
        }
        else
        {
            fprintf(out,
                    "\n"
                    "const APTR GM_UNIQUENAME(FuncTable)[]=\n"
                    "{\n");
        }
    }

    while (funclistit != NULL)
    {
        for (i = lvo+1; i<funclistit->lvo; i++)
            fprintf(out, "    NULL,\n");
        lvo = funclistit->lvo;

        switch (funclistit->libcall)
        {
        case STACK:
        case REGISTER:
        case REGISTERMACRO:
            if (funclistit->version != lastversion) {
                lastversion = funclistit->version;
                fprintf(out, "    /* Version %d */\n", lastversion);
            }
            fprintf(out, "    &AROS_SLIB_ENTRY(%s,%s,%d),\n", funclistit->internalname, cfg->basename, lvo);
            break;

        default:
            fprintf(stderr, "Internal error: unhandled libcall type in writestart\n");
            exit(20);
            break;
        }

        funclistit = funclistit->next;
    }

    fprintf(out, "    (void *)-1\n};\n");
}


static void writesets(FILE *out, struct config *cfg)
{
    fprintf(out,
            "DEFINESET(INIT)\n"
            "DEFINESET(EXIT)\n"
    );
    if (cfg->modtype != HANDLER)
        fprintf(out,
            "DEFINESET(CTORS)\n"
            "DEFINESET(DTORS)\n"
            "DEFINESET(INIT_ARRAY)\n"
            "DEFINESET(FINI_ARRAY)\n"
            "DEFINESET(INITLIB)\n"
            "DEFINESET(EXPUNGELIB)\n"
        );
    if (!(cfg->options & OPTION_NOOPENCLOSE))
        fprintf(out,
            "DEFINESET(OPENLIB)\n"
            "DEFINESET(CLOSELIB)\n"
        );
    if (cfg->modtype == DEVICE)
        fprintf(out,
            "DEFINESET(OPENDEV)\n"
            "DEFINESET(CLOSEDEV)\n"
        );
    if (cfg->classlist != NULL)
        fprintf(out,
                "DEFINESET(CLASSESINIT)\n"
                "DEFINESET(CLASSESEXPUNGE)\n"
        );
    fprintf(out, "\n");
}

