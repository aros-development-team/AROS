/*
    Copyright © 1995-2002, The AROS Development Team. All rights reserved.

    Desc: function to read in the module config file. Part of genmodule.
*/
#include "genmodule.h"

static void readsectionconfig(FILE *);
static void readsectionproto(FILE *);
static void readsectionclib(FILE *);
static void readsectionfunctionlist(FILE *);

void readconfig(void)
{
    FILE *in;
    char *s, *s2;
    
    in = fopen(conffile, "r");
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
		readsectionconfig(in);
		break;
		
	    case 2: /* proto */
		readsectionproto(in);
		break;
		
	    case 3: /* clib */
		readsectionclib(in);
		break;
		
	    case 4: /* functionlist */
		readsectionfunctionlist(in);
		break;
	    }
	}
	else if (!feof(in))
	    fprintf(stderr, "%s:%d:warning line outside section ignored\n", conffile, lineno);
    }
    fclose(in);
}


static void readsectionconfig(FILE *in)
{
    int atend = 0, i;
    char *s, *s2;
    
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
	    const char *names[] = {"basename", "libbase", "libbasetype", "libbasetypeextern", "version", "date", "libcall" };
	    const unsigned int namenums = sizeof(names)/sizeof(char *);
	    unsigned int namenum;
	    
	    for (i = 0, namenum = 0; namenum==0 && i<namenums; i++)
	    {
		if (strncmp(line, names[i], strlen(names[i]))==0 && isspace(*(line+strlen(names[i]))))
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
			conffile, lineno, names[namenum-1]);
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
	    case 1: /* basename */
		basename = strdup(s);
		break;
		
	    case 2: /* libbase */
		libbase = strdup(s);
		break;
		
	    case 3: /* libbasetype */
		libbasetype = strdup(s);
		break;
		
	    case 4: /* libbasetypeextern */
		libbasetypeextern = strdup(s);
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
		
	    case 7: /* libcall */
		if (strcmp(s, "stack")==0)
		    libcall = STACK;
		else if (strcmp(s, "register")==0)
		    libcall = REGISTER;
		else if (strcmp(s, "mixed")==0)
		{
		    libcall = MIXED;
		    fprintf(stderr, "%s:%d:Mixed libcall not supported yet\n",
			    conffile, lineno);
		    exit(20);
		}
		else if (strcmp(s, "registermacro")==0)
		{
		    libcall = REGISTERMACRO;
		    fprintf(stderr, "%s:%d:Registermacro libcall not supported yet\n",
			    conffile, lineno);
		    exit(20);
		}
		else if (strcmp(s, "autoregister")==0)
		{
		    libcall = AUTOREGISTER;
		    fprintf(stderr, "%s:%d:Autoregister libcall not supported yet\n",
			    conffile, lineno);
		    exit(20);
		}
		else
		{
		    fprintf(stderr, "%s:%d:Unknown libcall type\n",
			    conffile, lineno);
		    exit(20);
		}
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
    
    if (basename==NULL)
    {
	basename = strdup(modulename);
	*basename = toupper(*basename);
    }
    if (libbase==NULL)
    {
	unsigned int len = strlen(basename)+5;
	libbase = malloc(len);
	snprintf(libbase, len, "%sBase", basename);
    }
    if (libbasetype==NULL)
	libbasetype = "struct Library";
    if (libbasetypeextern==NULL)
	libbasetypeextern = strdup(libbasetype);
}

static void readsectionproto(FILE *in)
{
    int atend = 0;
    struct linelist **linelistptr = &protolines;
    char *s;
    
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

static void readsectionclib(FILE *in)
{
    int atend = 0;
    char *s;
    struct linelist **linelistptr = &cliblines;
    
    while (!atend)
    {
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
}

static void readsectionfunctionlist(FILE *in)
{
    int atend = 0;
    char *s, *s2;
    unsigned int lvo = firstlvo;
    struct functionlist **funclistptr = &funclist;
    struct arglist **arglistptr;
    
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
	    int hasbracket;
	    
	    if (isspace(line[strlen(line)-1]))
	    {
		fprintf(stderr, "%s:%d:error no space allowed before or after functionname\n",
			conffile, lineno);
		exit(20);
	    }
	    hasbracket = ((s=strchr(line, '('))!=NULL);
	    
	    switch (libcall)
	    {
	    case STACK:
		if (hasbracket)
		{
		    fprintf(stderr, "%s:%d:error registers given for stack based call\n",
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
		break;
		
	    case REGISTER:
		if (!hasbracket)
		{
		    fprintf(stderr, "%s:%d:error no register specified for register based call\n",
			    conffile, lineno);
		    exit(20);
		}
		s2 = s;
		
		while (isspace(*(s-1))) s--;
		*s='\0';

		*funclistptr = malloc(sizeof(struct functionlist));
		(*funclistptr)->next = NULL;
		(*funclistptr)->name = strdup(line);
		(*funclistptr)->type = NULL;
		(*funclistptr)->argcount = 0;
		(*funclistptr)->arguments = NULL;
		arglistptr = &(*funclistptr)->arguments;
		
		s2++;
		
		while ((s = strpbrk(s2,",)"))!=NULL)
		{
		    char *s3=s+1;

		    while (isspace(*s2)) s2++;
		    while (isspace(*(s-1))) s--;
		    *s='\0';
		    
		    *s2 = toupper(*s2);
		    if (strlen(s2)==2 && memchr("AD",s2[0],2)!=NULL && memchr("01234567",s2[1],8)!=NULL)
		    {
			(*funclistptr)->argcount++;
			(*arglistptr) = malloc(sizeof(struct arglist));
			(*arglistptr)->reg = strdup(s2);
			(*arglistptr)->next = NULL;
			(*arglistptr)->type = NULL;
			(*arglistptr)->name = NULL;
			arglistptr = &(*arglistptr)->next;
		    }
		    else
		    {
			fprintf(stderr, "%s:%d:error wrong register \"%s\" for argument %u\n",
				conffile, lineno, s2, (*funclistptr)->argcount+1);
			exit(20);
		    }
		    s2 = s3;
		}
		funclistptr = &((*funclistptr)->next);
		break;
		
	    default:
		fprintf(stderr, "Internal error: unsupported libcall type\n");
		exit(20);
		break;
	    }
	}
    }
}
    
