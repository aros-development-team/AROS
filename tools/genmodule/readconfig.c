/*
    Copyright © 1995-2002, The AROS Development Team. All rights reserved.

    Desc: function to read in the module config file. Part of genmodule.
*/
#include "genmodule.h"
#include "fileread.h"

static void readsectionconfig(void);
static void readsectionproto(void);
static void readsectioncdef(void);
static void readsectioncdefprivate(void);
static void readsectionfunctionlist(void);

void readconfig(void)
{
    char *line, *s, *s2;

    if (!fileopen(conffile))
    {
	fprintf(stderr, "Could not open %s\n", conffile);
	exit(20);
    }

    while ((line=readline())!=NULL)
    {
	if (strncmp(line, "##", 2)==0)
	{
	    static char *parts[] = {"config", "proto", "cdefprivate", "cdef", "functionlist"};
	    const unsigned int nums = sizeof(parts)/sizeof(char *);
	    unsigned int partnum;
	    int i, atend = 0;
	    
	    s = line+2;
	    while (isspace(*s)) s++;
	    if (strncmp(s, "begin", 5)!=0)
		exitfileerror(20, "\"##begin\" expected\n");

	    s += 5;
	    if (!isspace(*s))
		exitfileerror(20, "space after begin expected\n");
	    while (isspace(*s)) s++;
	    
	    for (i = 0, partnum = 0; partnum==0 && i<nums; i++)
	    {
		if (strncmp(s, parts[i], strlen(parts[i]))==0)
		{
		    partnum = i+1;
		    s += strlen(parts[i]);
		    while (isspace(*s)) s++;
		    if (*s!='\0')
			exitfileerror(20, "unexpected character on position %d\n", s-line);
		}
	    }
	    if (partnum==0)
		exitfileerror(20, "unknown start of section\n");
	    switch (partnum)
	    {
	    case 1: /* config */
		readsectionconfig();
		break;
		
	    case 2: /* proto */
		readsectionproto();
		break;
		
	    case 3: /* cdefprivate */
		readsectioncdefprivate();
		break;
		
	    case 4: /* cdef */
		readsectioncdef();
		break;

	    case 5: /* functionlist */
		readsectionfunctionlist();
		break;
	    }
	}
	else if (strlen(line)!=0)
	    filewarning("warning line outside section ignored\n");
    }
    fileclose();
    
    if (modtype == MCC || modtype == MUI || modtype == MCP)
    {
        struct functionlist *function;
        
        function = malloc(sizeof(struct functionlist));
        function->next = funclist;
        function->name = "MCC_Query";
        function->type = NULL;
        function->lvo = firstlvo - 1;
	function->novararg = 0;
        function->argcount = 1;
        function->arguments = malloc(sizeof(struct arglist));
        function->arguments->next = NULL;
        function->arguments->type = NULL;
        function->arguments->name = NULL;
        function->arguments->reg  = "D0";
        
        funclist = function;
    }
}


static void readsectionconfig(void)
{
    int atend = 0, i;
    char *line, *s, *s2;
    struct forcelist **forcelistptr = &forcelist;
    
    while (!atend)
    {
	line = readline();
	if (line==NULL)
	    exitfileerror(20, "unexpected end of file in section config\n");

	if (strncmp(line, "##", 2)!=0)
	{
	    const char *names[] = 
            {
                "basename", "libbase", "libbasetype", "libbasetypeextern", 
                "version", "date", "libcall", "forcebase", "superclass",
		"residentpri", "options"
            };
	    const unsigned int namenums = sizeof(names)/sizeof(char *);
	    unsigned int namenum;
	    
	    for (i = 0, namenum = 0; namenum==0 && i<namenums; i++)
	    {
		if 
                (
                       strncmp(line, names[i], strlen(names[i]))==0 
                    && isspace(*(line+strlen(names[i])))
                )
		    namenum = i+1;
	    }
	    if (namenum==0)
		exitfileerror(20, "unrecognized configuration option\n");

	    s = line + strlen(names[namenum-1]);
	    if (!isspace(*s))
		exitfileerror(20, "space character expected after \"%s\"\n", names[namenum-1]);

	    while (isspace(*s)) s++;
	    if (*s=='\0')
		exitfileerror(20, "unexpected end of line\n");
	    
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
		    exitfileerror(20, "wrong version string \"%s\"\n", s);
		break;
		
	    case 6: /* date */
		if (!(strlen(s)==10 && isdigit(s[0]) && isdigit(s[1]) &&
		      s[2]=='.' && isdigit(s[3]) && isdigit(s[4]) &&
		      s[5]=='.' && isdigit(s[6]) && isdigit(s[7]) &&
		      isdigit(s[8]) && isdigit(s[9])))
		{
		    exitfileerror(20, "date string has have dd.mm.yyyy format\n");
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
		    exitfileerror(20, "mixed libcall not supported yet\n");
		}
		else if (strcmp(s, "registermacro")==0)
		{
		    libcall = REGISTERMACRO;
		}
		else if (strcmp(s, "autoregister")==0)
		{
		    libcall = AUTOREGISTER;
		    exitfileerror(20, "autoregister libcall not supported yet\n");
		}
		else
		    exitfileerror(20, "unknown libcall type\n");
		break;
		
	    case 8: /* forcebase */
		*forcelistptr = malloc(sizeof(struct forcelist));
		(*forcelistptr)->next = NULL;
		(*forcelistptr)->basename = strdup(s);
		forcelistptr = &(*forcelistptr)->next;
		break;
                
            case 9: /* superclass */
                superclass = strdup(s);
                break;
		
	    case 10: /* residentpri */
		{
		    int count;
		    char dummy;
		    
		    count = sscanf(s, "%d%c", &residentpri, &dummy);
		    if (count != 1 ||
			residentpri < -128 || residentpri > 127
		    )
		    {
			exitfileerror(20, "residentpri number format error\n");
		    }
		}
		break;
	    case 11: /* options */
		do {
		    static const char *optionnames[] =
		    {
			"noautolib", "noexpunge", "noresident"
		    };
		    const unsigned int optionnums = sizeof(optionnames)/sizeof(char *);
		    int optionnum;

		    for (i = 0, optionnum = 0; optionnum==0 && i<optionnums; i++)
		    {
			if (strncmp(s, optionnames[i], strlen(optionnames[i]))==0)
			{
			    optionnum = i + 1;
			    s += strlen(optionnames[i]);
			    while (isspace(*s)) s++;
			    if (*s == ',')
				s++;
			    else if (*s != '\0')
				exitfileerror(20, "Unrecognized option\n");
			}
		    }
		    if (optionnum == 0)
			exitfileerror(20, "Unrecognized option\n");
		    switch (optionnum)
		    {
		    case 1: /* noautolib */
			options |= OPTION_NOAUTOLIB;
			break;
		    case 2: /* noexpunge */
			options |= OPTION_NOEXPUNGE;
			break;
		    case 3: /* noresident */
			options |= OPTION_NORESIDENT;
			break;
		    }
		    while (isspace(*s)) s++;
		} while(*s !='\0');
		break;
	    }
	}
	else /* Line starts with ## */
	{
	    s = line+2;
	    while (isspace(*s)) s++;
	    if (strncmp(s, "end", 3)!=0)
		exitfileerror(20, "\"##end config\" expected\n");

	    s += 3;
	    if (!isspace(*s))	
		exitfileerror(20, "\"##end config\" expected\n");

	    while (isspace(*s)) s++;
	    if (strncmp(s, "config", 6)!=0)
		exitfileerror(20, "\"##end config\" expected\n");

	    s += 6;
	    while (isspace(*s)) s++;
	    if (*s!='\0')
		exitfileerror(20, "\"##end config\" expected\n");

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
	libbasetype = "struct LibHeader";
    if (libbasetypeextern==NULL)
	libbasetypeextern = "struct Library";
}

static void readsectionproto(void)
{
    int atend = 0;
    struct linelist **linelistptr = &protolines;
    char *line, *s;
    
    while (!atend)
    {
	line = readline();
	if (line==NULL)
	    exitfileerror(20, "unexptected end of file in section proto\n");

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
		exitfileerror(20, "\"##end proto\" expected\n");

	    s += 3;
	    while (isspace(*s)) s++;
	    if (strncmp(s, "proto", 5)!=0)
		exitfileerror(20, "\"##end proto\" expected\n");

	    s += 5;
	    while (isspace(*s)) s++;
	    if (*s!='\0')
		exitfileerror(20, "unexpected character at position %d\n");

	    atend = 1;
	}
    }
}

static void readsectioncdef(void)
{
    int atend = 0;
    char *line, *s;
    struct linelist **linelistptr = &cdeflines;
    
    while (!atend)
    {
	line = readline();
	if (line==NULL)
	    exitfileerror(20, "unexptected end of file in section cdef\n");

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
		exitfileerror(20, "\"##end cdef\" expected\n");

	    s += 3;
	    while (isspace(*s)) s++;
	    if (strncmp(s, "cdef", 4)!=0)
		exitfileerror(20, "\"##end cdef\" expected\n");

	    s += 5;
	    while (isspace(*s)) s++;
	    if (*s!='\0')
		exitfileerror(20, "unexpected character at position %d\n");

	    atend = 1;
	}
    }
}

static void readsectioncdefprivate(void)
{
    int atend = 0;
    char *line, *s;
    struct linelist **linelistptr = &cdefprivatelines;
    
    while (!atend)
    {
	line = readline();
	if (line==NULL)
	    exitfileerror(20, "unexptected end of file in section cdef\n");

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
		exitfileerror(20, "\"##end cdefprivate\" expected\n");

	    s += 3;
	    while (isspace(*s)) s++;
	    if (strncmp(s, "cdefprivate", 11)!=0)
		exitfileerror(20, "\"##end cdefprivate\" expected\n");

	    s += 11;
	    while (isspace(*s)) s++;
	    if (*s!='\0')
		exitfileerror(20, "unexpected character at position %d\n");

	    atend = 1;
	}
    }
}

static void readsectionfunctionlist(void)
{
    int atend = 0;
    char *line, *s, *s2;
    unsigned int lvo = firstlvo;
    struct functionlist **funclistptr = &funclist;
    struct arglist **arglistptr;
    
    if (basename==NULL)
	exitfileerror(20, "section functionlist has to come after section config\n");

    if (libcall==REGISTERMACRO)
	exitfileerror(20, "No functionlist allowed for registermacro libcall");
    
    while (!atend)
    {
	line = readline();
	if (line==NULL)
	    exitfileerror(20, "unexptected EOF in functionlist section\n");
	if (strlen(line)==0)
	    lvo++;
	else if (isspace(*line))
	{
	    s = line;
	    while (isspace(*s)) s++;
	    if (*s=='\0')
		lvo++;
	    else
		exitfileerror(20, "no space allowed before or after functionname\n");
	}
	else if (strncmp(line, "##", 2)==0)
	{
	    s = line+2;
	    while (isspace(*s)) s++;
	    if (strncmp(s, "end", 3)!=0)
		exitfileerror(20, "\"##end functionlist\" expected\n");

	    s += 3;
	    while (isspace(*s)) s++;
	    if (strncmp(s, "functionlist", 12)!=0)
		exitfileerror(20, "\"##end functionlist\" expected\n");

	    s += 12;
	    while (isspace(*s)) s++;
	    if (*s!='\0')
		exitfileerror(20, "unexpected character on position %d\n", s-line);

	    atend = 1;
	}
	else if (*line=='.')
	{
	    s = line+1;
	    if (strncmp(s, "skip", 4)==0)
	    {
		int n;
		
		s += 4;
		if (!isspace(*s))
		    exitfileerror(20, "in syntax\n");
		
		n=strtol(s, &s2, 10);
		if (s2==NULL)
		    exitfileerror(20, "positive number expected\n");
		
		while (isspace(*s2)) s2++;
		if (*s2!='\0')
		    exitfileerror(20, "in syntax\n");
		lvo += n;
	    }
	}
	else if (*line!='#')
	{
	    int hasbracket;
	    
	    if (isspace(line[strlen(line)-1]))
		exitfileerror(20, "no space allowed before or after functionname\n");

	    hasbracket = ((s=strchr(line, '('))!=NULL);
	    
	    *funclistptr = malloc(sizeof(struct functionlist));
	    (*funclistptr)->next = NULL;
	    (*funclistptr)->type = NULL;
	    (*funclistptr)->argcount = 0;
	    (*funclistptr)->arguments = NULL;
	    (*funclistptr)->lvo = lvo;
	    (*funclistptr)->novararg = 0;
	    lvo++;
	    switch (libcall)
	    {
	    case STACK:
		if (hasbracket)
		    exitfileerror(20, "registers given for stack based call\n");

		(*funclistptr)->name = strdup(line);
		break;
		
	    case REGISTER:
		if (!hasbracket)
		    exitfileerror(20, "no register specified for register based call\n");

		s2 = s;
		
		while (isspace(*(s-1))) s--;
		*s='\0';

		(*funclistptr)->name = strdup(line);
		arglistptr = &(*funclistptr)->arguments;
		
		s2++;
		while (isspace(*s2)) s2++;
		if (*s2!=')')
		{
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
			    exitfileerror(20, "wrong register \"%s\" for argument %u\n", s2, (*funclistptr)->argcount+1);

			s2 = s3;
		    }
		}
		break;
			
	    default:
		exitfileerror(20, "Internal; unsupported libcall type\n");
		break;
	    }
	    funclistptr = &((*funclistptr)->next);
	}
    }
}
    
