#include <stdio.h>
#include <string.h>

int main(int argc, char **argv)
{
    char name[256];
    FILE *in, *out;
    unsigned int start;
    char haseol;
    
    if (argc != 3)
    {
	fprintf(stderr, "Usage: %s libname gendir\n", argv[0]);
	exit(20);
    }

    if (strlen(argv[2])>200)
    {
	fprintf(stderr, "Ridiculously long path for gendir\n");
	exit(20);
    }

    if (argv[2][strlen(argv[2])-1]=='/') argv[2][strlen(argv[2])-1]='\0';
    
    snprintf(name, 255, "%s_funclist", argv[1]);
    in = fopen(name, "r");
    if (in == NULL)
    {
	fprintf(stderr, "Could not open %s\n", name);
	exit(20);
    }

    snprintf(name, 255, "%s/%s_functable.c", argv[2], argv[1]); 
    out = fopen(name, "w");
    if (out == NULL)
    {
	fprintf(stderr, "Could not open %s\n", name);
	fclose(in);
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
    
    while(fgets(name, 254, in))
    {
	if (name[0]!='#')
	{
	    haseol = name[strlen(name)-1]=='\n';
	    if (haseol) name[strlen(name)-1]='\0';
	    fprintf(out, ",\n     *%s", name);
	    
	    while(!haseol && fgets(name, 254, in))
	    {
		haseol = name[strlen(name)-1]=='\n';
		if (haseol) name[strlen(name)-1]='\0';
		fputs(name, out);
	    }
	}
	else
	{
	    haseol = name[strlen(name)-1]=='\n';
	    while(!haseol && fgets(name, 254, in))
		haseol = name[strlen(name)-1]=='\n';
	}
    }

    fprintf(out,
	    ";\n"
	    "\n"
	    "void **const %s_functable[]=\n"
	    "{\n"
	    "    &AROS_SLIB_ENTRY(LC_BUILDNAME(OpenLib),LibHeader),\n"
	    "    &AROS_SLIB_ENTRY(LC_BUILDNAME(CloseLib),LibHeader),\n"
	    "    &AROS_SLIB_ENTRY(LC_BUILDNAME(ExpungeLib),LibHeader),\n"
	    "    &AROS_SLIB_ENTRY(LC_BUILDNAME(ExtFuncLib),LibHeader),\n",
	    argv[1]);

    fseek(in, 0, SEEK_SET);
    while (fgets(name, 254, in))
    {
	if (name[0]!='#')
	{
	    haseol = name[strlen(name)-1]=='\n';
	    if (haseol) name[strlen(name)-1]='\0';
	    fprintf(out, "    &%s", name);
	    
	    while(!haseol && fgets(name, 254, in))
	    {
		haseol = name[strlen(name)-1]=='\n';
		if (haseol) name[strlen(name)-1]='\0';
		fputs(name, out);
	    }
	
	    fputs(",\n", out);
	}
	else
	{
	    haseol = name[strlen(name)-1]=='\n';
	    while(!haseol && fgets(name, 254, in))
		haseol = name[strlen(name)-1]=='\n';
	}
    }

    fprintf(out, "    (void *)-1\n};\n");
    
    fclose(in);
    fclose(out);
    
    return 0;
}
