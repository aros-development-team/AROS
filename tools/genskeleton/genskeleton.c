/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Tool to create the skeleton files for a library,
    	  device, gadget or datatype.
	      
    Lang: 
    
    TODO: Generate special functions of devices (BeginIO)
          and datatypes (ObtainEngine)
	  
*/

#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>

/* Fool the CVS server not to replace empty cvs version tags in printfs */
#define TAG	" $"
#define VERSION	"$Id:"

/****************************************************************************************/

#define TYPE_LIBRARY 	0
#define TYPE_DEVICE	1
#define TYPE_GADGET	2
#define TYPE_DATATYPE	3

#define TYPE_MIN	TYPE_LIBRARY
#define TYPE_MAX	TYPE_DATATYPE

#ifdef __MINGW32__
#    define mkdir(name, mode) ((mkdir) (name))
#endif
/****************************************************************************************/




struct TypeInfo
{
    char *typestring;
    char *lowertypestring;
    char *defbasetype;
    char *destdir;
    char *metatarget;
}
typeinfo [] =
{
    {"LIBRARY"	, "library"  , "Library" , "$(SLIBDIR)"	     , "workbench-libs-"	     },
    {"DEVICE"	, "device"   , "Device"	 , "$(DEVSDIR)"	     , "workbench-devs-"	     },
    {"GADGET" 	, "gadget"   , "Library" , "$(GADGETDIR)"    , "workbench-classes-gadgets-"  },
    {"DATATYPE"	, "datatype" , "Library" , "$(DATATYPESDIR)" , "workbench-classes-datatypes-"}
};

/****************************************************************************************/

static char s[300];
static char destdir[300];
static char name[300];
static char lowername[300];
static char uppername[300];
static char shortname[300];
static char basetype[300];
static char basename[300];
static char includesdir[300];
static int  includes, type, i;
static long l;

/****************************************************************************************/

static void getstring(char *dest, char *def)
{
    strcpy(dest, def);
    fgets(dest, 300, stdin);
    l = strlen(dest);
    if (l) if (dest[l - 1] = '\n') dest[l - 1] = '\0';
    if (!dest[0]) strcpy(dest, def);
}

/****************************************************************************************/

static int getinteger(void)
{
    int ret = 0;

    getstring(s, "0");
    ret = strtol(s, 0, 10);

    return ret;
}

/****************************************************************************************/

static void getarguments(void)
{
    int i2;

    printf("Type (1 = Library, 2 = Device, 3 = Gadget, 4 = DataType): ");
    type = getinteger() - 1;
    if ((type < TYPE_MIN) || (type > TYPE_MAX)) type = TYPE_LIBRARY;

    printf("Name (e.g. Intuition, GadTools, Asl)                    : ");
    getstring(name, "Dummy");

    for(i = 0, i2 = 0; i <= strlen(name); i++)
    {
        lowername[i] = tolower(name[i]);
        uppername[i] = toupper(name[i]);
	if (toupper(name[i]) == name[i]) shortname[i2++] = name[i];
    }
    strcat(shortname, "B");

    printf("Lib Base Type (e.g. IntuitionBase, Library, Device)     : ");
    getstring(basetype, typeinfo[type].defbasetype);

    printf("Lib Base Name (e.g. Intuition, GadTools, Asl)           : ");
    getstring(basename, name);

    printf("Destination directory                                   : ");
    getstring(destdir, "");
    if (destdir[0])
    {
        l = strlen(destdir);
	if (destdir[l - 1] != '/') strcat(destdir, "/");
    }

    printf("Includes (0 = None, 1 = in libraries/, 2 = $(LIBNAME)/, 3 = other : ");
    includes = getinteger();
    switch (includes)
    {
	case 0:
	    break;

	case 1:
	    sprintf( includesdir, "libraries");
	    break;

	case 2:
	    sprintf( includesdir, "$(LIBNAME)");
	    break;

	default:
	    printf("Includes Destination directory                          : ");
	    getstring(includesdir, "");
	    break;
    }

}

/****************************************************************************************/

static void showarguments(void)
{
    puts("");
    printf("Type                 : %s\n", typeinfo[type].typestring);
    printf("Name                 : %s\n", name);
    printf("Lib Base Type        : %s\n", basetype);
    printf("Lib Base Name        : %s\n", basename);
    printf("Destination directory: %s\n", destdir);
    if (includes)
    {
	printf("Includes directory   : %s\n", includesdir);
    }
    else
    {
	printf("No includes.\n");
    }
    puts("");
    printf("--> struct %s *%sBase. Library Filename = %s.%s\n", basetype,
    	    	    	    	    	    	    	    	basename,
								lowername,
								typeinfo[type].lowertypestring);
    puts("");
    printf("Are you sure you want to continue (yes/no)? ");
    getstring(s, "");

    if ((s[0] != 'y') && s[0] != 'Y')
    {
        fprintf(stderr, "\n*** Aborted\n");
	exit(1);
    }
}

/****************************************************************************************/

static void preparedir(void)
{
    struct stat st;
    int err;

    err = stat(destdir, &st);
    if (!err)
    {
	printf("Warning: Directory \"%s\" already exists. Proceed anyway (yes/no)? ", destdir);
	getstring(s, "");
	if ((s[0] != 'y') && (s[0] != 'Y'))
	{
	    fprintf(stderr, "\n*** Aborted\n");
	    exit(1);
	}
    }
    else {
	err = mkdir(destdir, 0766);
	if (err)
	{
	    fprintf(stderr, "Error: could not create directory \"%s\"! Error = %i\n", destdir, err);
	    exit(1);
	}
	if (includes)
	{
	    char incdir[300];

	    sprintf(incdir, "%s/include", destdir);
	    err = mkdir(incdir, 0766);
	    if (err)
	    {
		fprintf(stderr, "Error: could not create directory \"%s\"! Error = %i\n", incdir, err);
		exit(1);
	    }
	}
    }
    puts("");

}

/****************************************************************************************/

static void make_cvsignore(void)
{
    FILE *f;

    printf("Creating %s.cvsignore\n", destdir);

    strcpy(s, destdir);
    strcat(s, ".cvsignore");

    f = fopen(s, "w");
    if (!f)
    {
        fprintf(stderr, "Error: could not create \"%s\"!\n", s);
	exit(1); 
    }

    fprintf(f, "*_end.c\n"
    	       "*_endtag.c\n"
	       "libdefs.h\n"
	       "mmakefile\n");
    fclose(f);
}

/****************************************************************************************/

static void make_libconf(void)
{
    FILE *f;

    printf("Creating %slib.conf\n", destdir);

    strcpy(s, destdir);
    strcat(s, "lib.conf");

    f = fopen(s, "w");
    if (!f)
    {
        fprintf(stderr, "Error: could not create \"%s\"!\n", s);
	exit(1); 
    }

    fprintf(f, "name %s\n", lowername);    
    fprintf(f, "basename %s\n", basename);
    fprintf(f, "libbase %sBase\n", basename);
    fprintf(f, "libbasetype struct %s\n", basetype);
    fprintf(f, "version 41.0\n");
    fprintf(f, "type %s\n", typeinfo[type].lowertypestring);

    fclose(f);
}

/****************************************************************************************/

static void make_mmakefilesrc(void)
{
    FILE *f;

    printf("Creating %smmakefile.src\n", destdir);

    strcpy(s, destdir);
    strcat(s, "mmakefile.src");

    f = fopen(s, "w");
    if (!f)
    {
        fprintf(stderr, "Error: could not create \"%s\"!\n", s);
	exit(1); 
    }

    fprintf(f, "# "VERSION TAG"\n"
    	        "include $(TOP)/config/make.cfg\n"
		"\n"
		"USER_INCLUDES := -I.\n"
		"\n");

    fprintf(f, "LIBNAME   := %s\n", lowername);
    fprintf(f, "ULIBNAME  := %s\n", name);
    fprintf(f, "BASENAME  := %s\n", basename);
    fprintf(f, "LIBPOST   := %s\n", typeinfo[type].lowertypestring);
    fprintf(f, "DESTDIR   := %s\n", typeinfo[type].destdir);
    fprintf(f, "\n");

    fprintf(f, "FILES     := %s_init\n\n", lowername);

    fprintf(f, "FUNCTIONS := dummyfunction\n\n");

    if (includes)
    {
	fprintf(f, "INC_FILES = $(wildcard include/*.h)\n");
	fprintf(f, "DEST_INC = $(foreach f,$(INC_FILES), $(INCDIR)/%s/$(notdir $f))\n",includesdir);
	fprintf(f, "GEN_INC = $(foreach f,$(INC_FILES), $(GENINCDIR)/%s/$(notdir $f))\n\n",includesdir);
	fprintf(f, "#MM\n");
	fprintf(f, "setup-includes :\n");
	fprintf(f, "%%mkdirs_q $(GENINCDIR)/%s $(INCDIR)/%s\n\n",includesdir, includesdir);
	fprintf(f, "#MM\n");
	fprintf(f, "includes-copy : $(DEST_INC) $(GEN_INC)\n");
	fprintf(f, "\t@$(NOP)\n\n");
	fprintf(f, "$(INCDIR)/%s/%%.h : include/%%.h\n",includesdir);
	fprintf(f, "\t@$(CP) include/$(notdir $<) $@\n\n");
	fprintf(f, "$(GENINCDIR)/%s/%%.h : include/%%.h\n",includesdir);
	fprintf(f, "\t@$(CP) include/$(notdir $<) $@\n\n");
    }

    fprintf(f, "%%define_libs prelibs=-l$(LIBNAME)\n\n");

    fprintf(f, "%%genlib_cpak\n\n");

    fprintf(f, "#MM %s%s : setup includes\n", typeinfo[type].metatarget, lowername);
    fprintf(f, "%s%s : $(SLIB)\n", typeinfo[type].metatarget, lowername);
    fprintf(f, "\t@$(NOP)\n\n");

    fprintf(f, "#MM %s%s-quick :\n", typeinfo[type].metatarget, lowername);
    fprintf(f, "%s%s-quick : $(SLIB)\n", typeinfo[type].metatarget, lowername);
    fprintf(f, "\t@$(NOP)\n\n");

    fprintf(f, "#MM\n"
    	       "setup :\n"
	       "\t%%mkdirs_q $(OBJDIR) $(LIBDIR) %s\n\n", typeinfo[type].destdir);

    fprintf(f, "#MM\n"
    	       "clean ::\n"
	       "\t$(RM) $(OBJDIR) *.err libdefs.h mmakefile $(END_FILE).c $(LIB) $(SLIB)\n\n");

    fprintf(f, "%%common\n");

    fclose(f);
}

/****************************************************************************************/

static void make_internh(void)
{
    FILE *f;

    printf("Creating %s%s_intern.h\n", destdir, lowername);

    sprintf(s, "%s%s_intern.h", destdir, lowername);

    f = fopen(s, "w");
    if (!f)
    {
        fprintf(stderr, "Error: could not create \"%s\"!\n", s);
	exit(1); 
    }

    fprintf(f, "#ifndef %s_INTERN_H\n", uppername);
    fprintf(f, "#define %s_INTERN_H\n", uppername);
    fprintf(f, "\n");
    fprintf(f, "/*\n");
    fprintf(f, "    (C) 2001 AROS - The Amiga Research OS\n");
    fprintf(f, "    "VERSION TAG"\n");
    fprintf(f, "\n");
    fprintf(f, "    Desc: Internal definitions for %s.%s\n", lowername, typeinfo[type].lowertypestring);
    fprintf(f, "    Lang: English\n");
    fprintf(f, "*/\n");
    fprintf(f, "\n");
    fprintf(f, "#ifndef EXEC_TYPES_H\n");
    fprintf(f, "#   include <exec/types.h>\n");
    fprintf(f, "#endif\n");
    fprintf(f, "#ifndef EXEC_LIBRARIES_H\n");
    fprintf(f, "#   include <exec/libraries.h>\n");
    fprintf(f, "#endif\n");
    fprintf(f, "#ifndef AROS_LIBCALL_H\n");
    fprintf(f, "#   include <aros/libcall.h>\n");
    fprintf(f, "#endif\n");
    fprintf(f, "#ifndef DOS_DOS_H\n");
    fprintf(f, "#   include <dos/dos.h>\n");
    fprintf(f, "#endif\n");
    fprintf(f, "#ifndef UTILITY_UTILITY_H\n");
    fprintf(f, "#   include <utility/utility.h>\n");
    fprintf(f, "#endif\n");
    fprintf(f, "\n");
    fprintf(f, "/****************************************************************************************/\n");
    fprintf(f, "\n");
    fprintf(f, "struct %sBase_intern\n", basename);
    fprintf(f, "{\n");
    fprintf(f, "    struct %s\t\tlibrary;\n", basetype);
    fprintf(f, "    struct ExecBase\t\t*sysbase;\n");
    fprintf(f, "    BPTR\t\t\tseglist;\n");
    fprintf(f, "\n");
    fprintf(f, "    struct UtilityBase\t\t*utilitybase;\n");
    fprintf(f, "};\n");
    fprintf(f, "\n");
    fprintf(f, "/****************************************************************************************/\n");
    fprintf(f, "\n");
    fprintf(f, "#undef %s\n", shortname);
    fprintf(f, "#define %s(b)\t\t((struct %sBase_intern *)b)\n", shortname, basename);
    fprintf(f, "\n");
    fprintf(f, "#undef SysBase\n");
    fprintf(f, "#define SysBase\t\t(%s(%sBase)->sysbase)\n", shortname, basename);
    fprintf(f, "\n");
    fprintf(f, "#undef UtilityBase\n");
    fprintf(f, "#define UtilityBase\t(%s(%sBase)->utilitybase)\n", shortname, basename);
    fprintf(f, "\n");
    fprintf(f, "/****************************************************************************************/\n");
    fprintf(f, "\n");
    fprintf(f, "#endif /* %s_INTERN_H */\n", uppername);

    fclose(f);
}

/****************************************************************************************/

static void make_initc()
{
    FILE *f;

    printf("Creating %s%s_init.c\n", destdir, lowername);

    sprintf(s, "%s%s_init.c", destdir, lowername);

    f = fopen(s, "w");
    if (!f)
    {
        fprintf(stderr, "Error: could not create \"%s\"!\n", s);
	exit(1); 
    }

    fprintf(f, "/*\n");
    fprintf(f, "    (C) 2001 AROS - The Amiga Research OS\n");
    fprintf(f, "    "VERSION TAG"\n");
    fprintf(f, "\n");
    fprintf(f, "    Desc: %s initialization code.\n", name);
    fprintf(f, "    Lang: English\n");
    fprintf(f, "*/\n");
    fprintf(f, "\n");
    fprintf(f, "#include <exec/types.h>\n");
    fprintf(f, "#include <exec/libraries.h>\n");
    fprintf(f, "#include <aros/libcall.h>\n");
    fprintf(f, "\n");
    fprintf(f, "#include <proto/exec.h>\n");
    fprintf(f, "\n");
    fprintf(f, "#include \"%s_intern.h\"\n", lowername);
    fprintf(f, "#include \"libdefs.h\"\n");
    fprintf(f, "\n");
    fprintf(f, "/****************************************************************************************/\n");
    fprintf(f, "\n");
    fprintf(f, "#undef SysBase\n");
    fprintf(f, "\n");
    fprintf(f, "/* Customize libheader.c */\n");
    fprintf(f, "#define LC_SYSBASE_FIELD(lib)   (%s(lib)->sysbase)\n", shortname);
    fprintf(f, "#define LC_SEGLIST_FIELD(lib)   (%s(lib)->seglist)\n", shortname);
    fprintf(f, "#define LC_LIBBASESIZE		sizeof(struct %sBase_intern)\n", basename);
    fprintf(f, "#define LC_LIBHEADERTYPEPTR	LIBBASETYPEPTR\n");
    fprintf(f, "#define LC_LIB_FIELD(lib)	(%s(lib)->library)\n", shortname);
    fprintf(f, "\n");
    fprintf(f, "/* #define LC_NO_INITLIB    */\n");
    fprintf(f, "/* #define LC_NO_OPENLIB    */\n");
    fprintf(f, "/* #define LC_NO_CLOSELIB   */\n");
    fprintf(f, "/* #define LC_NO_EXPUNGELIB */\n");
    fprintf(f, "\n");
    fprintf(f, "#include <libcore/libheader.c>\n");
    fprintf(f, "\n");
    fprintf(f, "#undef DEBUG\n");
    fprintf(f, "#define DEBUG 1\n");
    fprintf(f, "#include <aros/debug.h>\n");
    fprintf(f, "\n");
    fprintf(f, "#define SysBase\t\t\t(LC_SYSBASE_FIELD(%sBase))\n", basename);
    fprintf(f, "\n");
    fprintf(f, "/****************************************************************************************/\n");
    fprintf(f, "\n");
    fprintf(f, "ULONG SAVEDS STDARGS LC_BUILDNAME(L_InitLib) (LC_LIBHEADERTYPEPTR %sBase)\n", basename);
    fprintf(f, "{\n");
    fprintf(f, "    D(bug(\"Inside Init func of %s.%s\\n\"));\n", lowername, typeinfo[type].lowertypestring);
    fprintf(f, "\n");
    fprintf(f, "    if (!UtilityBase)\n");
    fprintf(f, "        (struct Library *)UtilityBase = OpenLibrary(\"utility.library\", 37);\n");
    fprintf(f, "    if (!UtilityBase)\n");
    fprintf(f, "        return FALSE;\n");
    fprintf(f, "\n");
    fprintf(f, "    return TRUE;\n");
    fprintf(f, "}\n");
    fprintf(f, "\n");
    fprintf(f, "/****************************************************************************************/\n");
    fprintf(f, "\n");
    fprintf(f, "ULONG SAVEDS STDARGS LC_BUILDNAME(L_OpenLib) (LC_LIBHEADERTYPEPTR %sBase)\n", basename);
    fprintf(f, "{\n");
    fprintf(f, "    D(bug(\"Inside Open func of %s.%s\\n\"));\n", lowername, typeinfo[type].lowertypestring);
    fprintf(f, "\n");
    fprintf(f, "    return TRUE;\n");
    fprintf(f, "}\n");
    fprintf(f, "\n");
    fprintf(f, "/****************************************************************************************/\n");
    fprintf(f, "\n");
    fprintf(f, "void  SAVEDS STDARGS LC_BUILDNAME(L_CloseLib) (LC_LIBHEADERTYPEPTR %sBase)\n", basename);
    fprintf(f, "{\n");
    fprintf(f, "    D(bug(\"Inside Close func of %s.%s\\n\"));\n", lowername, typeinfo[type].lowertypestring);
    fprintf(f, "}\n");
    fprintf(f, "\n");
    fprintf(f, "/****************************************************************************************/\n");
    fprintf(f, "\n");
    fprintf(f, "void  SAVEDS STDARGS LC_BUILDNAME(L_ExpungeLib) (LC_LIBHEADERTYPEPTR %sBase)\n", basename);
    fprintf(f, "{\n");
    fprintf(f, "    D(bug(\"Inside Expunge func of %s.%s\\n\"));\n", lowername, typeinfo[type].lowertypestring);
    fprintf(f, "\n");
    fprintf(f, "    /* CloseLibrary() checks for NULL-pointers */\n");
    fprintf(f, "\n");
    fprintf(f, "    CloseLibrary((struct Library *)UtilityBase);\n");
    fprintf(f, "    UtilityBase = NULL;\n");
    fprintf(f, "}\n");
    fprintf(f, "\n");
    fprintf(f, "/****************************************************************************************/\n");

    fclose(f);
}

/****************************************************************************************/

static void make_dummyfunction(void)
{
    FILE *f;

    printf("Creating %sdummyfunction.c\n", destdir);

    sprintf(s, "%sdummyfunction.c", destdir);

    f = fopen(s, "w");
    if (!f)
    {
        fprintf(stderr, "Error: could not create \"%s\"!\n", s);
	exit(1); 
    }

    fprintf(f, "/*\n");
    fprintf(f, "    (C) 2001 AROS - The Amiga Research OS\n");
    fprintf(f, "    "VERSION TAG"\n");
    fprintf(f, "\n");
    fprintf(f, "    Desc: %s function DummyFunction()\n", name);
    fprintf(f, "    Lang: English\n");
    fprintf(f, "*/\n");
    fprintf(f, "\n");
    fprintf(f, "#include \"%s_intern.h\"\n", lowername);
    fprintf(f, "\n");
    fprintf(f, "/*****************************************************************************\n");
    fprintf(f, "\n");
    fprintf(f, "    NAME */\n");
    fprintf(f, "\n");
    fprintf(f, "\tAROS_LH2(ULONG, DummyFunction,\n");
    fprintf(f, "\n");
    fprintf(f, "/*  SYNOPSIS */\n");
    fprintf(f, "\tAROS_LHA(ULONG, one, D0),\n"),
    fprintf(f, "\tAROS_LHA(ULONG, two, D1),\n"),
    fprintf(f, "\n");
    fprintf(f, "/*  LOCATION */\n");
    fprintf(f, "\tstruct %s *, %sBase, 1000, %s)\n", basetype, basename, basename);
    fprintf(f, "\n");
    fprintf(f, "/*  FUNCTION\n");
    fprintf(f, "\tReturns the sum of one + two.\n");
    fprintf(f, "\n");
    fprintf(f, "    INPUTS\n");
    fprintf(f, "\tone - First value.\n");
    fprintf(f, "\ttwo - Second value.\n");
    fprintf(f, "\n");
    fprintf(f, "    RESULT\n");
    fprintf(f, "\tThe sum of one + two.\n");
    fprintf(f, "\n");
    fprintf(f, "    NOTES\n");
    fprintf(f, "\n");
    fprintf(f, "    EXAMPLE\n");
    fprintf(f, "\n");
    fprintf(f, "    BUGS\n");
    fprintf(f, "\tThe function itself is a bug ;-) Remove it!\n");
    fprintf(f, "\n");
    fprintf(f, "    SEE ALSO\n"),
    fprintf(f, "\n");
    fprintf(f, "    INTERNALS\n");
    fprintf(f, "\n");
    fprintf(f, "    HISTORY\n");
    fprintf(f, "\n");
    fprintf(f, "*****************************************************************************/\n");
    fprintf(f, "{\n");
    fprintf(f, "    AROS_LIBFUNC_INIT\n");
    fprintf(f, "    AROS_LIBBASE_EXT_DECL(struct %s *,%sBase)\n", basetype, basename);
    fprintf(f, "\n");
    fprintf(f, "    return one + two;\n");
    fprintf(f, "\n");
    fprintf(f, "    AROS_LIBFUNC_EXIT\n");
    fprintf(f, "\n");
    fprintf(f, "} /* DummyFunction */\n");

    fclose(f);
}

/****************************************************************************************/

int main(void)
{
    getarguments();
    showarguments();
    preparedir();
    make_cvsignore();
    make_libconf();
    make_mmakefilesrc();
    make_internh();
    make_initc();
    make_dummyfunction();

    return 0;
}

/****************************************************************************************/
