#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

struct arglist {
    struct arglist *next;
    char *type;
    char *name;
};

struct functionlist {
    struct functionlist *next;
    char *name;
    char *type;
    unsigned int argcount;
    struct arglist *arguments;
};

struct linelist {
    struct linelist *next;
    char *line;
};

/* In funclist the information of all the functions of the module will be stored.
 * The list has to be sorted on the lvonum field
 */
struct functionlist *funclist = NULL;

/* global variables that store filename and paths derived from argv */
char *conffile, *gendir, *genincdir, *reffile;

/* global variables that store the configuration of the module */
enum modtype { UNSPECIFIED, LIBRARY };
enum modtype modtype = UNSPECIFIED;

char *modulename = NULL, *basename = NULL, *modulenameupper = NULL, *basetype = NULL, *datestring = "00.00.0000";
unsigned int majorversion = 0, minorversion = 0, firstlvo = 0;
struct linelist *cliblines = NULL, *protolines = NULL;

/* global variables for reading lines from files */
char *line; /* The current read file */
unsigned int slen; /* The allocation length pointed to be line */
unsigned int lineno; /* The line number, will be increased by one everytime a line is read */

void readline(FILE *f)
{
    char haseol;

    if (fgets(line, slen, f))
    {
	haseol = line[strlen(line)-1]=='\n';
	if (haseol) line[strlen(line)-1]='\0';
	
	while (!(haseol || feof(f)))
	{
	    slen += 256;
	    line = realloc(line, slen);
	    fgets(line+strlen(line), slen, f);
	    haseol = line[strlen(line)-1]=='\n';
	    if (haseol) line[strlen(line)-1]='\0';
	}
    }
    else
	line[0]='\0';
    lineno++;
}

static void readconfig(void);
static void readref(void);
static void writeincproto(void);
static void writeincclib(void);
static void writeincdefines(void);
static void writefunctable(void);

int main(int argc, char **argv)
{
    if (argc != 5)
    {
	fprintf(stderr, "Usage: %s conffile gendir genincdir reffile\n", argv[0]);
	exit(20);
    }

    conffile = argv[1];

    if (strlen(argv[2])>200)
    {
	fprintf(stderr, "Ridiculously long path for gendir\n");
	exit(20);
    }
    if (argv[2][strlen(argv[2])-1]=='/') argv[2][strlen(argv[2])-1]='\0';
    gendir = argv[2];
    
    if (strlen(argv[3])>200)
    {
	fprintf(stderr, "Ridiculously long path for genincdir\n");
	exit(20);
    }
    if (argv[3][strlen(argv[3])-1]=='/') argv[3][strlen(argv[3])-1]='\0';
    genincdir = argv[3];

    reffile = argv[4];

    line = malloc(256);
    slen = 256;

    readconfig();
    readref();
    writeincproto();
    writeincclib();
    writeincdefines();
    writefunctable();
    
    return 0;
}

static void readconfig(void)
{
    FILE *in;
    char *s, *s2;
    int atend;
    
    in = fopen(conffile, "r");
    printf("Opening: %s\n", line);
    if (in==NULL)
    {
	fprintf(stderr, "Could not open %s\n", line);
	exit(20);
    }
    lineno = 0;
    
    while (!feof(in))
    {
	readline(in);
	if (strncmp(line, "##", 2)==0)
	{
	    static char *parts[] = {"config", "proto", "clib", "functionlist"};
	    const unsigned int nums = sizeof(parts)/sizeof(char *);
	    unsigned int partnum;
	    int i, atend = 0;
	    
	    s = line+2;
	    while (isspace(*s)) s++;
	    if (strncmp(s, "begin", 5)!=0)
	    {
		fprintf(stderr, "%s:%d:error \"##begin\" expected\n", conffile, lineno);
		exit(20);
	    }
	    s += 5;
	    if (!isspace(*s))
	    {
		fprintf(stderr, "%s:%d:error space after begin expected\n", conffile, lineno);
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
			fprintf(stderr, "%s:%d:error unexpected character on position %d\n",
				conffile, lineno, s-line);
			exit(20);
		    }
		}
	    }
	    if (partnum==0)
	    {
		fprintf(stderr, "%s:%d:error unknown start of section\n", conffile, lineno);
		exit(20);
	    }
	    switch (partnum)
	    {
	    case 1: /* config */
		while (!atend)
		{
		    readline(in);
		    if (feof(in))
		    {
			fprintf(stderr, "%s:%d:error unexpected end of file in section config\n",
				conffile, lineno);
			exit(20);
		    }
		    if (strncmp(line, "##", 2)!=0)
		    {
			const char *names[] = {"name", "type", "basename", "basetype", "version", "date"};
			const unsigned int namenums = sizeof(names)/sizeof(char *);
			unsigned int namenum;
			
			for (i = 0, namenum = 0; namenum==0 && i<namenums; i++)
			{
			    if (strncmp(line, names[i], strlen(names[i]))==0)
				namenum = i+1;
			}
			if (namenum==0)
			{
			    fprintf(stderr, "%s:%d:unrecognized configuration option\n",
				    conffile, lineno);
			    exit(20);
			}
			s = line + strlen(names[namenum-1]);
			if (!isspace(*s))
			{
			    fprintf(stderr, "%s:%d:space character expected after \"%s\"\n",
				    conffile, lineno, s-line);
			    exit(20);
			}
			while (isspace(*s)) s++;
			if (*s=='\0')
			{
			    fprintf(stderr, "%s:%d:unexpected end of line\n",
				    conffile, lineno);
			    exit(20);
			}
			
			s2 = s + strlen(s);
			while (isspace(*(s2-1))) s2--;
			*s2 = '\0';
			
			switch (namenum)
			{
			case 1: /* name */
			    modulename = strdup(s);
			    break;
			    
			case 2: /* type */
			    if (strcmp(s, "library")==0)
			    {
				modtype = LIBRARY;
				firstlvo = 5;
			    }
			    else
			    {
				fprintf(stderr, "%s:%d:error unrecognized type \"%s\"\n",
					conffile, lineno, s);
				exit(20);
			    }
			    break;
			    
			case 3: /* basename */
			    basename = strdup(s);
			    break;
			    
			case 4: /* basetype */
			    basetype = strdup(s);
			    break;
			    
			case 5: /* version */
			    if (sscanf(s, "%u.%u", &majorversion, &minorversion)!=2)
			    {
				fprintf(stderr, "%s:%d:error wrong version string \"%s\"\n",
					conffile, lineno, s);
				exit(20);
			    }
			    break;
			    
			case 6: /* date */
			    if (!(strlen(s)==10 && isdigit(s[0]) && isdigit(s[1]) &&
				  s[2]=='.' && isdigit(s[3]) && isdigit(s[4]) &&
				  s[5]=='.' && isdigit(s[6]) && isdigit(s[7]) &&
				  isdigit(s[8]) && isdigit(s[9])))
			    {
				fprintf(stderr, "%s:%d:error date string has have dd.mm.yyyy format\n",
					conffile, lineno);
				exit(20);
			    }
			    datestring = strdup(s);
			    break;
			}
		    }
		    else /* Line starts with ## */
		    {
			s = line+2;
			while (isspace(*s)) s++;
			if (strncmp(s, "end", 3)!=0)
			{
			    fprintf(stderr, "%s:%d:error \"##end config\" expected\n",
				    conffile, lineno);
			    exit(20);
			}
			s += 3;
			if (!isspace(*s))	
			{
			    fprintf(stderr, "%s:%d:error \"##end config\" expected\n",
				    conffile, lineno);
			    exit(20);
			}
			while (isspace(*s)) s++;
			if (strncmp(s, "config", 6)!=0)
			{
			    fprintf(stderr, "%s:%d:error \"##end config\" expected\n",
				    conffile, lineno);
			    exit(20);
			}
			s += 6;
			while (isspace(*s)) s++;
			if (*s!='\0')
			{
			    fprintf(stderr, "%s:%d:error \"##end config\" expected\n",
				    conffile, lineno);
			    exit(20);
			}
			atend = 1;
		    }
		}
		if (modulename==NULL)
		{
		    fprintf(stderr, "%s:%d:error no name given for module\n", conffile, lineno);
		    exit(20);
		}
		modulenameupper = strdup(modulename);
		for (s=modulenameupper; *s!='\0'; *s = toupper(*s), s++)
		    ;
		if (modtype==UNSPECIFIED)
		{
		    modtype = LIBRARY;
		    firstlvo = 5;
		}

		if (basename==NULL)
		{
		    basename = strdup(modulename);
		    *basename = toupper(*basename);
		}
		if (basetype==NULL)
		    basetype = strdup(basename);
		
		break;
		
	    case 2: /* proto */
		while (!atend)
		{
		    struct linelist **linelistptr = &protolines;
		    
		    while (!atend)
		    {
			readline(in);
			if (feof(in))
			{
			    fprintf(stderr, "%s:%d:error unexptected end of file in section proto\n",
				    conffile, lineno);
			    exit(20);
			}
			if (strncmp(line, "##", 2)!=0)
			{
			    *linelistptr = malloc(sizeof(struct linelist));
			    (*linelistptr)->line = strdup(line);
			    (*linelistptr)->next = NULL;
			    linelistptr = &(*linelistptr)->next;
			}
			else
			{
			    s = line+2;
			    while (isspace(*s)) s++;
			    if (strncmp(s, "end", 3)!=0)
			    {
				fprintf(stderr, "%s:%d:error \"##end proto\" expected\n", conffile, lineno);
				exit(20);
			    }
			    s += 3;
			    while (isspace(*s)) s++;
			    if (strncmp(s, "proto", 5)!=0)
			    {
				fprintf(stderr, "%s:%d:error \"##end proto\" expected\n", conffile, lineno);
				exit(20);
			    }
			    s += 5;
			    while (isspace(*s)) s++;
			    if (*s!='\0')
			    {
				fprintf(stderr, "%s:%d:error unexpected character at position %d\n",
					conffile, lineno, s-line);
				exit(20);
			    }
			    atend = 1;
			}
		    }
		}
		break;
		
	    case 3: /* clib */
		while (!atend)
		{
		    struct linelist **linelistptr = &cliblines;
		    readline(in);
		    if (feof(in))
		    {
			fprintf(stderr, "%s:%d:error unexptected end of file in section clib\n", conffile, lineno);
			exit(20);
		    }
		    if (strncmp(line, "##", 2)!=0)
		    {
			*linelistptr = malloc(sizeof(struct linelist));
			(*linelistptr)->line = strdup(line);
			(*linelistptr)->next = NULL;
			linelistptr = &(*linelistptr)->next;
		    }
		    else
		    {
			s = line+2;
			while (isspace(*s)) s++;
			if (strncmp(s, "end", 3)!=0)
			{
			    fprintf(stderr, "%s:%d:error \"##end clib\" expected\n", conffile, lineno);
			    exit(20);
			}
			s += 3;
			while (isspace(*s)) s++;
			if (strncmp(s, "clib", 4)!=0)
			{
			    fprintf(stderr, "%s:%d:error \"##end clib\" expected\n", conffile, lineno);
			    exit(20);
			}
			s += 5;
			while (isspace(*s)) s++;
			if (*s!='\0')
			{
			    fprintf(stderr, "%s:%d:error unexpected character at position %d\n",
				    conffile, lineno, s-line);
			    exit(20);
			}
			atend = 1;
		    }
		}
		break;
		
	    case 4: /* functionlist */
		while (!atend)
		{
		    unsigned int lvo = firstlvo;
		    struct functionlist **funclistptr = &funclist;
		    
		    if (basename==NULL)
		    {
			fprintf(stderr, "%s:%d:error section functionlist has to come after section config\n",
				conffile, lineno);
			exit(20);
		    }
		    while (!atend)
		    {
			readline(in);
			if (feof(in))
			{
			    fprintf(stderr, "%s:%d:error unexptected EOF in functionlist section\n", conffile, lineno);
			    exit(20);
			}
			if (strlen(line)==0)
			    lvo++;
			else if (isspace(*line))
			{
			    s = line;
			    while (isspace(*s)) s++;
			    if (*s=='\0')
				lvo++;
			    else
			    {
				fprintf(stderr, "%s:%d:error no space allowed before or after functionname\n",
					conffile, lineno);
				exit(20);
			    }
			}
			else if (strncmp(line, "##", 2)==0)
			{
			    s = line+2;
			    while (isspace(*s)) s++;
			    if (strncmp(s, "end", 3)!=0)
			    {
				fprintf(stderr, "%s:%d:error \"##end functionlist\" expected\n", conffile, lineno);
				exit(20);
			    }
			    s += 3;
			    while (isspace(*s)) s++;
			    if (strncmp(s, "functionlist", 12)!=0)
			    {
				fprintf(stderr, "%s:%d:error \"##end functionlist\" expected\n", conffile, lineno);
				exit(20);
			    }
			    s += 12;
			    while (isspace(*s)) s++;
			    if (*s!='\0')
			    {
				fprintf(stderr, "%s:%d:error unexpected character on position %d\n",
					conffile, lineno, s-line);
				exit(20);
			    }
			    atend = 1;
			}
			else if (*line!='#')
			{
			    if (isspace(line[strlen(line)-1]))
			    {
				fprintf(stderr, "%s:%d:error no space allowed before or after functionname\n",
					conffile, lineno);
				exit(20);
			    }
			    *funclistptr = malloc(sizeof(struct functionlist));
			    (*funclistptr)->next = NULL;
			    (*funclistptr)->name = strdup(line);
			    (*funclistptr)->type = NULL;
			    (*funclistptr)->argcount = 0;
			    (*funclistptr)->arguments = NULL;
			    funclistptr = &((*funclistptr)->next);
			}
		    }
		}
		break;
	    }
	}
	else if (!feof(in))
	    fprintf(stderr, "%s:%d:warning line outside section ignored\n", conffile, lineno);
    }
    fclose(in);
}


static void readref(void)
{
    FILE *in;
    struct functionlist *funclistit;
    char *begin, *end;
    
    in = fopen(reffile, "r");
    if (in==NULL)
    {
	fprintf(stderr, "Could not open %s\n", reffile);
	exit(20);
    }
    do
    {
	static char infunction=0;
	static struct arglist **arglistptr;
	
	readline(in);
	if (strlen(line)>0)
	{
	    if (infunction &&
		(strncmp(line, "FILE", 4)==0 ||
		 strncmp(line, "INCLUDES", 8)==0 ||
		 strncmp(line, "DEFINES", 7)==0 ||
		 strncmp(line, "VARIABLE", 8)==0
		)
	       )
		infunction = 0;

	    if (strncmp(line, "FUNCTION", 8)==0)
	    {
		begin = strchr(line,':');
		if (begin==NULL)
		{
		    fprintf(stderr, "Syntax error in reffile %s\n", reffile);
		    exit(20);
		}
		begin++;
		while (isspace(*begin)) begin++;
		end = strchr(begin, '[');
		if (end==NULL)
		{
		    fprintf(stderr, "Syntax error in reffile %s\n", reffile);
		    exit(20);
		}
		while (isspace(*(end-1))) end--;
		*end = '\0';

		for (funclistit = funclist;
		     funclistit!=NULL && strcmp(funclistit->name, begin)!=0;
		     funclistit = funclistit->next)
		    ;

		if (funclistit==NULL)
		    infunction = 0;
		else
		{
		    infunction = 1;
		    arglistptr = &(funclistit->arguments);
		}
	    }
	    else if (infunction)
	    {
		if (strncmp(line, "Arguments", 9)==0)
		{
		    (*arglistptr) = malloc(sizeof(struct arglist));
		    begin = strchr(line, ':');
		    if (begin==NULL)
		    {
			fprintf(stderr, "Syntax error in reffile %s\n", reffile);
			exit(20);
		    }
		    begin++;
		    while (isspace(*begin)) begin++;
		    end = begin+strlen(begin);
		    while (isspace(*(end-1))) end--;
		    while (!isspace(*(end-1)) && *(end-1)!='*') end--;
		    (*arglistptr)->name = strdup(end);
		    while (isspace(*(end-1))) end--;
		    *end='\0';
		    (*arglistptr)->type = strdup(begin);
		    (*arglistptr)->next = NULL;
		    arglistptr = &((*arglistptr)->next);
		    funclistit->argcount++;
		}
		else if (strncmp(line, "Type", 4)==0)
		{
		    begin = strchr(line, ':');
		    if (begin==NULL)
		    {
			fprintf(stderr, "Syntax error in reffile %s\n", reffile);
			exit(20);
		    }
		    begin++;
		    while (isspace(*begin)) begin++;
		    end = begin+strlen(begin)-strlen(funclistit->name);
		    assert(strcmp(end,funclistit->name)==0);
		    while (isspace(*(end-1))) end--;
		    *end = '\0';
		    funclistit->type = strdup(begin);
		}
	    }
	}
    } while (!feof(in));
    fclose(in);

    /* Checking to see if every function has a prototype */
    for (funclistit = funclist;
	 funclistit!=NULL;
	 funclistit = funclistit->next)
    {
	if (funclistit->type==NULL)
	{
	    fprintf(stderr, "Did not find function %s in reffile %s\n", funclistit->name, reffile);
	    exit(20);
	}
    }
}

static void writeincproto(void)
{
    FILE *out;
    struct linelist *linelistit;
    
    snprintf(line, slen-1, "%s/proto/%s.h", genincdir, modulename);
    out = fopen(line, "w");
    if (out==NULL)
    {
	fprintf(stderr, "Could not write %s\n", line);
	exit(20);
    }
    fprintf(out,
	    "#ifndef PROTO_%s_H\n"
	    "#define PROTO_%s_H\n"
	    "\n"
	    "/*\n"
	    "    *** Automatically generated file. Do not edit ***\n"
	    "    Copyright © 1995-2002, The AROS Development Team. All rights reserved.\n"
	    "*/\n"
	    "\n"
	    "#include <aros/system.h>\n",
	    modulenameupper, modulenameupper);
    for (linelistit = protolines; linelistit!=NULL; linelistit = linelistit->next)
	fprintf(out, "%s\n", linelistit->line);
    fprintf(out,
	    "#include <clib/%s_protos.h>\n"
	    "#include <defined/%s.h>\n"
	    "\n"
	    "#endif /* PROTO_%s_H */\n",
	    modulename, modulename, modulenameupper);
    fclose(out);
}

static void writeincclib(void)
{
    FILE *out;
    struct functionlist *funclistit;
    struct arglist *arglistit;
    struct linelist *linelistit;
    unsigned int start;
    
    snprintf(line, slen-1, "%s/clib/%s.h", genincdir, modulename);
    out = fopen(line, "w");
    if (out==NULL)
    {
	fprintf(stderr, "Could write file %s\n", line);
	exit(20);
    }
    fprintf(out,
	    "#ifndef CLIB_%s_PROTOS_H\n"
	    "#define CLIB_%s_PROTOS_H\n"
	    "\n"
	    "/*\n"
	    "    *** Automatically generated file. Please do not edit ***\n"
	    "    Copyright © 1995-2002, The AROS Development Team. All rights reserved.\n"
	    "*/\n"
	    "\n"
	    "#include <aros/libcall.h>\n",
	    modulenameupper, modulenameupper);
    for (linelistit = cliblines; linelistit!=NULL; linelistit = linelistit->next)
	fprintf(out, "%s\n", linelistit->line);
    for (funclistit = funclist, start = 5;
	 funclistit!=NULL;
	 funclistit = funclistit->next, start++)
    {
	fprintf(out, "\nAROS_LP%d(%s, %s,\n", funclistit->argcount, funclistit->type, funclistit->name);

	for (arglistit = funclistit->arguments;
	     arglistit!=NULL;
	     arglistit = arglistit->next)
	    fprintf(out, "        AROS_LPA(%s, %s,),\n", arglistit->type, arglistit->name);

	fprintf(out, "        struct Library *, %sBase, %d, %s)\n",
		basename, start, basename);
    }
    fprintf(out, "\n#endif /* CLIB_%s_PROTOS_H */\n", modulenameupper);
}

static void writeincdefines(void)
{
    FILE *out;
    struct functionlist *funclistit;
    struct arglist *arglistit;
    unsigned int start;
    
    snprintf(line, slen-1, "%s/defines/%s.h", genincdir, modulename);
    out = fopen(line, "w");
    if (out==NULL)
    {
	fprintf(stderr, "Could not write %s\n", line);
	exit(20);
    }
    fprintf(out,
	    "#ifndef DEFINES_%s_PROTOS_H\n"
	    "#define DEFINES_%s_PROTOS_H\n"
	    "\n"
	    "/*\n"
	    "    *** Automatically generated file. Do not edit ***\n"
	    "    Copyright © 1995-2002, The AROS Development Team. All rights reserved.\n"
	    "\n"
	    "    Desc: Prototype for %s\n"
	    "*/\n"
	    "\n"
	    "#include <aros/libcall.h>\n"
	    "#include <exec/types.h>\n"
	    "\n",
	    modulenameupper, modulenameupper, modulename);
    for (funclistit = funclist, start=5;
	 funclistit!=NULL;
	 funclistit = funclistit->next, start++)
    {
	fprintf(out,
		"\n"
		"#define %s(",
		funclistit->name);
	for (arglistit = funclistit->arguments;
	     arglistit!=NULL;
	     arglistit = arglistit->next)
	{
	    if (arglistit != funclistit->arguments)
		fprintf(out, ", ");
	    fprintf(out, "%s", arglistit->name);
	}
	fprintf(out,
		") \\\n"
		"        AROS_LC%d(%s, %s, \\\n",
		funclistit->argcount, funclistit->type, funclistit->name);
	for (arglistit = funclistit->arguments;
	     arglistit!=NULL;
	     arglistit = arglistit->next)
	    fprintf(out, "        AROS_LCA(%s,%s,), \\\n", arglistit->type, arglistit->name);
	fprintf(out, "        struct Library *, %sBase, %d, %s)\n", basename, start, basename);
    }
    fprintf(out,
	    "\n"
	    "#endif /* DEFINES_%s_PROTOS_H*/\n",
	    modulenameupper);
    fclose(out);
}

static void writefunctable(void)
{
    FILE *out;
    struct functionlist *funclistit;
    
    snprintf(line, 255, "%s/%s_functable.c", gendir, modulename); 
    out = fopen(line, "w");
    if (out==NULL)
    {
	fprintf(stderr, "Could not open %s\n", line);
	exit(20);
    }
    fprintf(out,
	    "#include <aros/libcall.h>\n"
	    "#include \"libdefs.h\"\n"
	    "\n"
	    "extern\n"
	    "void * AROS_SLIB_ENTRY(LC_BUILDNAME(OpenLib),LibHeader),\n"
	    "     * AROS_SLIB_ENTRY(LC_BUILDNAME(CloseLib),LibHeader),\n"
	    "     * AROS_SLIB_ENTRY(LC_BUILDNAME(ExpungeLib),LibHeader),\n"
	    "     * AROS_SLIB_ENTRY(LC_BUILDNAME(ExtFuncLib),LibHeader)");
    
    for (funclistit = funclist;
	 funclistit != NULL;
	 funclistit = funclistit->next)
	fprintf(out, ",\n      %s", funclist->name);

    fprintf(out,
	    ";\n"
	    "\n"
	    "void **const %s_functable[]=\n"
	    "{\n"
	    "    &AROS_SLIB_ENTRY(LC_BUILDNAME(OpenLib),LibHeader),\n"
	    "    &AROS_SLIB_ENTRY(LC_BUILDNAME(CloseLib),LibHeader),\n"
	    "    &AROS_SLIB_ENTRY(LC_BUILDNAME(ExpungeLib),LibHeader),\n"
	    "    &AROS_SLIB_ENTRY(LC_BUILDNAME(ExtFuncLib),LibHeader),\n",
	    modulename);

    for (funclistit = funclist;
	 funclistit != NULL;
	 funclistit = funclistit->next)
	fprintf(out, "    &%s,\n", funclist->name);

    fprintf(out, "    (void *)-1\n};\n");
    fclose(out);
}
