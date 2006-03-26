/*
    Copyright © 1995-2005, The AROS Development Team. All rights reserved.
    $Id$
    
    Print the library magic and init code in the file modname_start.c.
    This code is partly based on code in CLib37x.lha from Andreas R. Kleinert
*/
#include "genmodule.h"
#include "muisupport.h"
#include "dtsupport.h"
#include "boopsisupport.h"

static void writedecl(FILE *, struct config *);
static void writedeclsets(FILE *, struct config *);
static void writeresident(FILE *, struct config *);
static void writeinitlib(FILE *, struct config *);
static void writeopenlib(FILE *, struct config *);
static void writecloselib(FILE *, struct config *);
static void writeexpungelib(FILE *, struct config *);
static void writeextfunclib(FILE *, struct config *);
static void writefunctable(FILE *, struct config *);
static void writesets(FILE *, struct config *);

void writestart(struct config *cfg)
{
    FILE *out;
    char line[256];
    struct classinfo *cl;
    
    snprintf(line, 255, "%s/%s_start.c", cfg->gendir, cfg->modulename);
    out = fopen(line, "w");

    if (out == NULL)
    {
        perror(line);
        exit(20);
    }

    fprintf(out, getBanner(cfg));

    writedecl(out, cfg);
    if (!(cfg->options & OPTION_NORESIDENT))
    {
	writeresident(out, cfg);
	writedeclsets(out, cfg);
	writeinitlib(out, cfg);
	if (cfg->modtype != RESOURCE)
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
	    writemccinit(out, cl != cfg->classlist || cfg->modtype != cl->classtype, cl);
	    break;
	case GADGET:
	case DATATYPE:
	case CLASS:
	case IMAGE:
	    writeclassinit(out, cl);
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
    struct stringlist *linelistit;
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
	    "#include <dos/dos.h>\n"
	    "\n"
	    "#include \"%s_libdefs.h\"\n"
	    "\n"
	    "#ifdef SysBase\n"
	    "#undef SysBase\n"
	    "#endif\n"
	    "\n"
	    "#include <proto/exec.h>\n"
	    "\n",
	    cfg->modulename
    );

    /* Write out declaration section provided in the config file */
    for (linelistit = cfg->cdeflines; linelistit != NULL; linelistit = linelistit->next)
    {
        fprintf(out, "%s\n", linelistit->s);
    }

    /* Write out the prototypes for the functions of the function table */
    writefuncprotos(out, cfg, cfg->funclist);
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
    fprintf(out,
	    "THIS_PROGRAM_HANDLES_SYMBOLSETS\n"
	    "DECLARESET(INIT)\n"
	    "DECLARESET(EXIT)\n"
	    "DECLARESET(CTORS)\n"
	    "DECLARESET(DTORS)\n"
	    "DECLARESET(INITLIB)\n"
	    "DECLARESET(EXPUNGELIB)\n"
	    "DECLARESET(OPENLIB)\n"
	    "DECLARESET(CLOSELIB)\n"
	    "DECLARESET(OPENDEV)\n"
	    "DECLARESET(CLOSEDEV)\n"
    );
    if (cfg->classlist != NULL)
	fprintf(out,
		"DECLARESET(CLASSESINIT)\n"
		"DECLARESET(CLASSESEXPUNGE)\n"
		"#define ADD2INITCLASSES(symbol, pri) ADD2SET(symbol, classesinit, pri)\n"
		"#define ADD2EXPUNGECLASSES(symbol, pri) ADD2SET(symbol, classesexpunge, pri)\n"
	);
    fprintf(out, "\n");
}


static void writeresident(FILE *out, struct config *cfg)
{
    fprintf(out,
	    "extern const int GM_UNIQUENAME(End);\n"
	    "extern const APTR GM_UNIQUENAME(FuncTable)[];\n"
    );
    if (cfg->modtype != RESOURCE)
	fprintf(out, "static const struct InitTable GM_UNIQUENAME(InitTable);\n");
    fprintf(out,
	    "\n"
	    "extern const char GM_UNIQUENAME(LibName)[];\n"
	    "extern const char GM_UNIQUENAME(LibID)[];\n"
	    "extern const char GM_UNIQUENAME(Copyright)[];\n"
	    "\n"
    );

    if (cfg->modtype != RESOURCE)
    {
	fprintf(out,
		"#define __freebase(lh)\\\n"
		"do {\\\n"
		"    UWORD negsize, possize;\\\n"
		"    UBYTE *negptr = (UBYTE *)lh;\\\n"
		"    negsize = ((struct Library *)lh)->lib_NegSize;\\\n"
		"    negptr -= negsize;\\\n"
		"    possize = ((struct Library *)lh)->lib_PosSize;\\\n"
		"    FreeMem (negptr, negsize+possize);\\\n"
		"} while(0)\n"
		"\n"
       	);
    }
	
    fprintf(out,
	    "AROS_UFP3 (LIBBASETYPEPTR, GM_UNIQUENAME(InitLib),\n"
	    "    AROS_UFPA(LIBBASETYPEPTR, lh, D0),\n"
	    "    AROS_UFPA(BPTR, segList, A0),\n"
	    "    AROS_UFPA(struct ExecBase *, sysBase, A6)\n"
	    ");\n"
    );
    if (cfg->modtype != RESOURCE)
    {
	fprintf(out,
		"AROS_LP1(BPTR, GM_UNIQUENAME(ExpungeLib),\n"
		"    AROS_LPA(LIBBASETYPEPTR, lh, D0),\n"
		"    struct ExecBase *, sysBase, 3, %s\n"
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
	    "    (APTR)&GM_UNIQUENAME(End),\n"
	    "    RESIDENTFLAGS,\n"
	    "    VERSION_NUMBER,\n"
    );
    switch (cfg->modtype)
    {
    case LIBRARY:
    case MUI:
    case MCC:
    case MCP:
    case GADGET:
    case DATATYPE:
    case HIDD:
	fprintf(out, "    NT_LIBRARY,\n");
	break;
    case DEVICE:
	fprintf(out, "    NT_DEVICE,\n");
	break;
    case RESOURCE:
	fprintf(out, "    NT_RESOURCE,\n");
	break;
    default:
	fprintf(stderr, "Internal error: unsupported modtype for NT_...\n");
	exit(20);
	break;
    }
    fprintf(out,
	    "    RESIDENTPRI,\n"
	    "    (char *)&GM_UNIQUENAME(LibName)[0],\n"
	    "    (char *)&GM_UNIQUENAME(LibID)[6],\n"
    );
    if (cfg->modtype != RESOURCE)
    {
	fprintf(out,
		"    (APTR)&GM_UNIQUENAME(InitTable)\n"
		"};\n"
		"\n"
		"static struct InitTable\n"
		"{\n"
		"    IPTR                   Size;\n"
		"    const APTR             *FuncTable;\n"
		"    const struct DataTable *DataTable;\n"
		"    APTR                    InitLibTable;\n"
		"}\n"
		"const GM_UNIQUENAME(InitTable) =\n"
		"{\n"
		"    sizeof(LIBBASETYPE),\n"
		"    &GM_UNIQUENAME(FuncTable)[0],\n"
		"    NULL,\n"
		"    (APTR)GM_UNIQUENAME(InitLib)\n"
		"};\n"
	);
    }
    else /* RESOURCE */
	fprintf(out, "    (APTR)GM_UNIQUENAME(InitLib)\n};\n");
    
    fprintf(out,
	    "\n"
	    "const char GM_UNIQUENAME(LibName)[] = MOD_NAME_STRING;\n"
	    "const char GM_UNIQUENAME(LibID)[] = VERSION_STRING;\n"
	    "const char GM_UNIQUENAME(Copyright)[] = COPYRIGHT_STRING;\n"
	    "\n"
    );
}

static void writeinitlib(FILE *out, struct config *cfg)
{
    fprintf(out,
	    "AROS_UFH3 (LIBBASETYPEPTR, GM_UNIQUENAME(InitLib),\n"
	    "    AROS_UFHA(LIBBASETYPEPTR, lh, D0),\n"
	    "    AROS_UFHA(BPTR, segList, A0),\n"
	    "    AROS_UFHA(struct ExecBase *, sysBase, A6)\n"
	    ")\n"
	    "{\n"
	    "    AROS_USERFUNC_INIT\n"
	    "\n"
	    "    int ok;\n"
	    "\n"
    );
    if (cfg->modtype == RESOURCE)
    {
	unsigned int funccount;
	struct functionhead *funclistit = cfg->funclist;
	if (funclistit == NULL)
	    funccount = cfg->firstlvo-1;
	else
	{
	    while (funclistit->next != NULL)
		funclistit = funclistit->next;
	    
	    funccount = funclistit->lvo;
	}
	fprintf(out,
		"    int vecsize;\n"
		"    struct Node *n;\n"
		"    char *mem;\n"
		"\n"
		"    vecsize = %u*LIB_VECTSIZE;\n"
		"    if (vecsize > 0)\n"
		"        vecsize = ((vecsize-1)/sizeof(IPTR) + 1)*sizeof(IPTR);\n"
		"    mem = AllocMem(vecsize+sizeof(LIBBASETYPE), MEMF_PUBLIC);\n"
		"    if (mem == NULL)\n"
		"         return NULL;\n"
		"    lh = (LIBBASETYPEPTR)(mem + vecsize);\n"
		"    n = (struct Node *)lh;\n"
		"    n->ln_Type = NT_RESOURCE;\n"
		"    n->ln_Pri = RESIDENTPRI;\n"
		"    n->ln_Name = (char *)GM_UNIQUENAME(LibName);\n"
		"    MakeFunctions(lh, (APTR)GM_UNIQUENAME(FuncTable), NULL);\n",
		funccount
	);
	
	fprintf(out,
	    	"#ifdef GM_SYSBASE_FIELD\n"
		"    GM_SYSBASE_FIELD(lh) = sysBase;\n"
		"#endif\n");
    }
    else
    {
	fprintf(out,
		"    GM_SYSBASE_FIELD(lh) = sysBase;\n"
		"    ((struct Library *)lh)->lib_Revision = REVISION_NUMBER;\n"
	);
    }
    
    if (!(cfg->options & OPTION_NOEXPUNGE) && cfg->modtype!=RESOURCE)
	fprintf(out, "    GM_SEGLIST_FIELD(lh) = segList;\n");
    if (cfg->options & OPTION_DUPBASE)
	fprintf(out, "    GM_ROOTBASE_FIELD(lh) = (LIBBASETYPEPTR)lh;\n");
    fprintf(out, "    if (");
    if (!(cfg->options & OPTION_NOAUTOLIB))
	fprintf(out, "set_open_libraries() && ");
    if (cfg->classlist != NULL)
	fprintf(out, "set_call_libfuncs(SETNAME(CLASSESINIT), 1, lh) && ");
    fprintf(out,
	    "set_call_funcs(SETNAME(INIT), 1, 1) )\n"
	    "    {\n"
	    "        set_call_funcs(SETNAME(CTORS), -1, 0);\n"
	    "\n"
	    "        ok = set_call_libfuncs(SETNAME(INITLIB),1,lh);\n"
	    "    }\n"
	    "    else\n"
	    "        ok = 0;\n"
	    "\n"
	    "    if (!ok)\n"
	    "    {\n"
	    "        set_call_libfuncs(SETNAME(EXPUNGELIB),-1,lh);\n"
	    "        set_call_funcs(SETNAME(DTORS), 1, 0);\n"
	    "        set_call_funcs(SETNAME(EXIT), -1, 0);\n"
    );
    if (cfg->classlist != NULL)
	fprintf(out, "        set_call_libfuncs(SETNAME(CLASSESEXPUNGE),-1,lh);\n");
    if (!(cfg->options & OPTION_NOAUTOLIB))
	fprintf(out, "        set_close_libraries();\n");
    if (cfg->modtype != RESOURCE)
    {
	fprintf(out,
		"\n"
		"        __freebase(lh);\n"
	);
    }
    else
    {
	fprintf(out,
		"\n"
		"        FreeMem(mem, vecsize+sizeof(LIBBASETYPE));\n"
	);
    }
    fprintf(out,
	    "        return NULL;\n"
	    "    }\n"
	    "    else\n"
	    "        return  lh;\n"
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
    case DEVICE:
	fprintf(out,
		"AROS_LH3 (void, GM_UNIQUENAME(OpenLib),\n"
		"    AROS_LHA(struct IORequest *, ioreq, A1),\n"
		"    AROS_LHA(ULONG, unitnum, D0),\n"
		"    AROS_LHA(ULONG, flags, D1),\n"
		"    LIBBASETYPEPTR, lh, 1, %s\n"
		")\n",
		cfg->basename
	);
	fprintf(out,
		"{\n"
		"    AROS_LIBFUNC_INIT\n"
		"\n"
		"    if ( set_call_libfuncs(SETNAME(OPENLIB), 1, lh)\n"
		"         && set_call_devfuncs(SETNAME(OPENDEV), 1, ioreq, unitnum, flags, lh)\n"
		"    )\n"
		"    {\n"
		"        ((struct Library *)lh)->lib_OpenCnt++;\n"
		"        ((struct Library *)lh)->lib_Flags &= ~LIBF_DELEXP;\n"
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
	break;
    default:
	fprintf(out,
		"AROS_LH1 (LIBBASETYPEPTR, GM_UNIQUENAME(OpenLib),\n"
		"    AROS_LHA (ULONG, version, D0),\n"
		"    LIBBASETYPEPTR, lh, 1, %s\n"
		")\n",
		cfg->basename
	);
	fprintf(out,
		"{\n"
		"    AROS_LIBFUNC_INIT\n"
		"\n"
	);
	fprintf(out,
		"    if ( set_call_libfuncs(SETNAME(OPENLIB), 1, lh) )\n"
		"    {\n"
	);
	if (!(cfg->options & OPTION_DUPBASE))
	{
	    fprintf(out,
		    "        ((struct Library *)lh)->lib_OpenCnt++;\n"
		    "        ((struct Library *)lh)->lib_Flags &= ~LIBF_DELEXP;\n"
	    );
	}
	else
	{
	    fprintf(out,
		    "        struct Library *newlib;\n"
		    "        UWORD possize = ((struct Library *)lh)->lib_PosSize;\n"
		    "\n"
		    "        ((struct Library *)lh)->lib_OpenCnt++;\n"
		    "        ((struct Library *)lh)->lib_Flags &= ~LIBF_DELEXP;\n"
		    "\n"
		    "        newlib = MakeLibrary(GM_UNIQUENAME(InitTable).FuncTable,\n"
		    "                             GM_UNIQUENAME(InitTable).DataTable,\n"
		    "                             NULL,\n"
		    "                             GM_UNIQUENAME(InitTable).Size,\n"
		    "                             (BPTR)NULL\n"
		    "        );\n"
		    "        if (newlib == NULL)\n"
		    "            return 0;\n"
		    "\n"
		    "        CopyMem(lh, newlib, possize);\n"
		    "        lh = (LIBBASETYPEPTR)newlib;\n"
	    );
	}
	fprintf(out,
		"\n"
		"        return lh;\n"
		"    }\n"
		"\n"
		"    return NULL;\n"
		"\n"
		"    AROS_LIBFUNC_EXIT\n"
		"}\n"
		"\n"
	);
    }
}


static void writecloselib(FILE *out, struct config *cfg)
{
    if (cfg->modtype != DEVICE)
	fprintf(out,
		"AROS_LH0 (BPTR, GM_UNIQUENAME(CloseLib),\n"
		"    LIBBASETYPEPTR, lh, 2, %s\n"
		")\n",
		cfg->basename
	);
    else
	fprintf(out,
		"AROS_LH1(BPTR, GM_UNIQUENAME(CloseLib),\n"
		"    AROS_LHA(struct IORequest *, ioreq, A1),\n"
		"    LIBBASETYPEPTR, lh, 2, %s\n"
		")\n",
		cfg->basename
	);
	
    fprintf(out,
	    "{\n"
	    "    AROS_LIBFUNC_INIT\n"
	    "\n"
    );
    if (!(cfg->options & OPTION_DUPBASE))
    {
	fprintf(out,
		"    ((struct Library *)lh)->lib_OpenCnt--;\n"
		"    set_call_libfuncs(SETNAME(CLOSELIB),-1,lh);\n"
	);
    }
    else
    {
	fprintf(out,
		"    if (lh != GM_ROOTBASE_FIELD(lh))\n"
		"    {\n"
		"        LIBBASETYPEPTR rootbase = GM_ROOTBASE_FIELD(lh);\n"
		"        __freebase(lh);\n"
		"        set_call_libfuncs(SETNAME(CLOSELIB),-1,lh);\n"
		"        lh = rootbase;\n"
		"    }\n"
		"    ((struct Library *)lh)->lib_OpenCnt--;\n"
		"\n"
	);
    }
    if (cfg->modtype == DEVICE)
	fprintf(out,
		"    set_call_devfuncs(SETNAME(CLOSEDEV),-1,ioreq,0,0,lh);\n"
	);
    if (!(cfg->options & OPTION_NOEXPUNGE))
	fprintf(out,
		"    if\n"
		"    (\n"
		"        (((struct Library *)lh)->lib_OpenCnt == 0)\n"
		"        && (((struct Library *)lh)->lib_Flags & LIBF_DELEXP)\n"
		"    )\n"
		"    {\n"
		"        return AROS_LC1(BPTR, GM_UNIQUENAME(ExpungeLib),\n"
		"                   AROS_LCA(LIBBASETYPEPTR, lh, D0),\n"
		"                   struct ExecBase *, SysBase, 3, %s\n"
		"        );\n"
		"    }\n",
		cfg->basename
	);
    fprintf(out,
	    "\n"
	    "    return NULL;\n"
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
	    "    AROS_LHA(LIBBASETYPEPTR, lh, D0),\n"
	    "    struct ExecBase *, sysBase, 3, %s\n"
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
	fprintf(out,
		"\n"
		"    if ( ((struct Library *)lh)->lib_OpenCnt == 0 )\n"
		"    {\n"
		"        BPTR seglist = GM_SEGLIST_FIELD(lh);\n"
		"\n"
		"        Remove((struct Node *)lh);\n"
		"\n"
		"        set_call_libfuncs(SETNAME(EXPUNGELIB),-1,lh);\n"
		"        set_call_funcs(SETNAME(DTORS), 1, 0);\n"
		"        set_call_funcs(SETNAME(EXIT),-1,0);\n"
	);
	if (cfg->classlist != NULL)
	    fprintf(out, "        set_call_libfuncs(SETNAME(CLASSESEXPUNGE),-1,lh);\n");
	if (!(cfg->options & OPTION_NOAUTOLIB))
	    fprintf(out, "        set_close_libraries();\n");
	fprintf(out,
		"\n"
		"        __freebase(lh);\n"
		"\n"
		"        return seglist;\n"
		"    }\n"
		"\n"
		"    ((struct Library *)lh)->lib_Flags |= LIBF_DELEXP;\n"
	);
    }
    fprintf(out,
	    "\n"
	    "    return NULL;\n"
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
	    "    LIBBASETYPEPTR, lh, 4, %s\n"
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
    struct functionhead *funclistit;
    struct functionarg *arglistit;
    unsigned int lvo;
    int i;
    char *name, *type;
    
    /* lvo contains the number of functions already printed in the functable */
    lvo = 0;
    
    if (!(cfg->options & OPTION_NORESIDENT))
    {
	fprintf(out,
		"\n"
		"const APTR GM_UNIQUENAME(FuncTable)[]=\n"
		"{\n"
	);
	if (cfg->modtype != RESOURCE)
	{
	    fprintf(out,
		    "    &AROS_SLIB_ENTRY(GM_UNIQUENAME(OpenLib),%s),\n"
		    "    &AROS_SLIB_ENTRY(GM_UNIQUENAME(CloseLib),%s),\n"
		    "    &AROS_SLIB_ENTRY(GM_UNIQUENAME(ExpungeLib),%s),\n"
		    "    &AROS_SLIB_ENTRY(GM_UNIQUENAME(ExtFuncLib),%s),\n",
		    cfg->basename, cfg->basename, cfg->basename, cfg->basename
	    );
	    lvo += 4;
	}
        if (cfg->modtype == MCC || cfg->modtype == MUI || cfg->modtype == MCP)
	{
	    fprintf(out,
		    "    &AROS_SLIB_ENTRY(MCC_Query,%s),\n",
		    cfg->basename
	    );
	    lvo++;
	}
        else if (cfg->modtype == DATATYPE)
	{
	    fprintf(out,
		    "    &AROS_SLIB_ENTRY(ObtainEngine,%s),\n",
		    cfg->basename
	    );
	    lvo++;
	}
	funclistit = cfg->funclist;
    }
    else /* NORESIDENT */
    {
	if (cfg->modtype != RESOURCE)
	{
	    int neednull = 0;
	    struct functionhead *funclistit2;
	
	    funclistit = cfg->funclist;
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
			"AROS_UFH1(static int, %s_null,\n"
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
		    "    &AROS_SLIB_ENTRY(%s,%s),\n"
		    "    &AROS_SLIB_ENTRY(%s,%s),\n",
		    funclistit->name, cfg->basename,
		    funclistit2->name, cfg->basename
	    );
	    lvo += 2;
	    funclistit = funclistit2->next;

	    if (funclistit->lvo == 3)
	    {
		fprintf(out, "    &AROS_SLIB_ENTRY(%s,%s),\n",
			funclistit->name, cfg->basename
		);
		funclistit = funclistit->next;
	    }
	    else
		fprintf(out, "    &%s_null,\n", cfg->modulename);
	    lvo++;
	    
	    if (funclistit->lvo == 4)
	    {
		fprintf(out, "    &AROS_SLIB_ENTRY(%s,%s),\n",
			funclistit->name, cfg->basename
		);
		funclistit = funclistit->next;
	    }
	    else
		fprintf(out, "    &%s_null,\n", cfg->modulename);
	    lvo++;
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
	    fprintf(out, "    &%s,\n", funclistit->name);
	    break;
	    
	case REGISTER:
	case REGISTERMACRO:
	    fprintf(out, "    &AROS_SLIB_ENTRY(%s,%s),\n", funclistit->name, cfg->basename);
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
	    "DEFINESET(CTORS)\n"
	    "DEFINESET(DTORS)\n"
	    "DEFINESET(INITLIB)\n"
	    "DEFINESET(EXPUNGELIB)\n"
	    "DEFINESET(OPENLIB)\n"
	    "DEFINESET(CLOSELIB)\n"
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
