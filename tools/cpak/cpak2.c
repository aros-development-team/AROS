#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>

#define IN_FUNC_FILE "lib.conf"
#define MAXBUF 100

struct inclist {
    struct inclist *next;
    char *text;
};

FILE * fd, * fdo;

#define _read(stream,buff,count)    fread(buff,1,count,stream)

int main(int argc,char **argv)
{
int count;
int i,j,filecount;
char fbuf[MAXBUF];
char comment[3]={0x2f,0x2a,0x00};

char ft[]="zyx";
struct inclist first = { NULL, ft}, *current, *search;

char bracket;
char incname[50];
char **infile = NULL;
char *name1,*name2,*dummy;
int numinfiles = 0;
int match;
char trail;

    /* Collect filenames from IN_FUNC_FILE */
    fd=fopen(IN_FUNC_FILE,"rb");
    if(!fd)
	fprintf(stderr,"\nCouldn't find %s!\n",IN_FUNC_FILE),exit(-1);
    do
    {
	/* Go to keyword "functions" */
	do
	{
	    i=0;
	    do
	    {
		count = _read(fd,&fbuf[i],1);
		i++;
	    } while(count!=0 && fbuf[i-1]!=' ' && fbuf[i-1]!='\n' && i<MAXBUF);
	    i--;
	    fbuf[i]=0;
	    match = strcmp(fbuf,"functions");
	    if(match!=0)
	    {
		do
		{
		    count = _read(fd,fbuf,1);
		} while(count!=0 && fbuf[0]!='\n');
	    }
	} while(count!=0 && match!=0);
	if(count!=0)
	{
	    /* Get filenames */
	    do
	    {
		i=0;
		/* Skip white space */
		do
		{
		    count = _read(fd,fbuf,1);
		} while(count!=0 && isspace(fbuf[0]));
		do
		{
		    i++;
		    count = _read(fd,&fbuf[i],1);
		} while(count!=0 && !isspace(fbuf[i]));
		trail = fbuf[i];
		if(!isspace(fbuf[0]))
		{
		    /* Save name */
		    numinfiles++;
		    infile = realloc(infile,numinfiles*sizeof(char *));
		    fbuf[i] = 0;
		    infile[numinfiles-1] = strdup(fbuf);
		}
	    } while(count!=0 && trail!='\n');
	}
    } while(count!=0);
    fclose(fd);

    /* Replace local file (infile[]) by specific remote file (argv[]) */
    /* Examples: "test.c" -> "/usr/src/AROS/config/linux/test.c"      */
    /*           "../test.c" -> "../../config/linux/test.c"           */
    /* -- Concatenate all unreplaceable files                         */
    for(i=1;i<argc;i++)
    {
	/* Get filename out of replacement file */
        match=0;
	name2=argv[i];
	dummy=name2;
	while(*dummy)
	{
	    if(*dummy=='/')
		name2=dummy+1;
	    dummy++;
	}
	for(j=0;j<numinfiles;j++)
	{
	    /* Get filename out of original file */
	    name1=infile[j];
	    dummy=name1;
	    while(*dummy)
	    {
		if(*dummy=='/')
		    name1=dummy+1;
		dummy++;
	    }
	    if(strcmp(name1,name2)==0)
	    {
		/* Replace file */
		free(infile[j]);
		/* printf("Replaced %s by %s\n",infile[j],argv[i]); */
		infile[j]=strdup(argv[i]);
		match++;
	    }
	}
	/* There is no file to replace */
	if(match==0)
	{
	    /* Save name */
	    numinfiles++;
	    infile = realloc(infile,numinfiles*sizeof(char *));
	    infile[numinfiles-1] = strdup(argv[i]);
	}
    }

    /* Collect files */
    fdo=fopen("functions.c","w");
    if(!fdo)
	fprintf(stderr, "Could not open functions.c out-file!\n"),exit(-1);
    fprintf(fdo,"#include \"functions.h\"\n");

    printf("Collecting functions...");
    for(filecount=0;filecount<numinfiles;filecount++)
    {
	fd=fopen(infile[filecount],"rb");
	if(!fd)
	    fprintf(stderr,"\n%s - No such file !\n",infile[filecount]),exit(-1);

	fprintf(fdo,"#line 1 \"%s\"\n", infile[filecount]);
	count=_read(fd,fbuf,1);
	while(count==1)
	{
	    switch (fbuf[0])
	    {
		case '/':
		    count=_read(fd,&fbuf[1],1);
		    if(fbuf[count]=='*')
		    {
			fprintf(fdo,comment);
			do
			{
			    do
			    {
				count=_read(fd,fbuf,1);
				putc (fbuf[0],fdo);
			    } while (count==1 && fbuf[0]!='*');
		    	    count=_read(fd,fbuf,1);
			    putc (fbuf[0],fdo);
			} while (count==1 && fbuf[0]!='/');
		    }
		    else
		    {
			if (fseek(fd,-count,SEEK_CUR))
			{
			    perror("fseek");
			    exit(-1);
			}
			putc (fbuf[0],fdo);
		    }
		    break;
		case '#':
		    count=_read(fd,&fbuf[1],8);
		    fbuf[count+1]=0;
		    if(strcmp(fbuf,"#include ")==0)
		    {
			_read(fd,incname,1);
		        if(incname[0]=='<'||incname[0]==0x22)
			{
			    if(incname[0]=='<')
				bracket='>';
			    else
				bracket=0x22;
			    i=0;
			    do {
				i++;
				count=_read(fd,&incname[i],1);
			    } while(incname[i]!=bracket&&count!=0&&incname[i]!='\n');
			    if(incname[i]=='\n'||count==0)
			    {
		 		fprintf(stderr,"\nUnclosed '#include %c' in file %s!\n",incname[0],infile[filecount]);
		 		exit(-1);
			    }
			    incname[i+1]=0;
			    search=&first;
			    while(search->next!=NULL&&strcmp(search->text,incname)!=0)
			        search=search->next;
			    if(search->next==NULL&&strcmp(search->text,incname)!=0)
			    {
				current=malloc(sizeof(struct inclist));
				current->next=NULL;
				current->text=strdup(incname);
				search->next=current;
			    }
			}
			else
			{
			    fprintf (fdo,"#include %s", &incname[0]);
			}
		    }
		    else
		    {
			if (fseek(fd,-count,SEEK_CUR))
			{
			    perror("fseek");
			    exit(-1);
			}
			putc (fbuf[0],fdo);
		    }
		    if(count>0)
			count=1;
		    break;
		default:
		    putc (fbuf[0],fdo);
		    break;
	    }
	    count=_read(fd,fbuf,1);
	}
	fclose(fd);
    }
    fclose(fdo);

    for(i=0;i<numinfiles;i++)
    {
      free(infile[i]);
    }
    free(infile);

    fdo=fopen("functions.h","w");
    if(!fdo)
	fprintf(stderr, "Could not open functions.h out-file!\n"),
	exit(-1);
    fprintf(fdo,"#define AROS_ALMOST_COMPATIBLE\n");
    search=first.next;
    while(search!=NULL)
    {
	current=search;
	fprintf(fdo,"\n#include %s", current->text);
	search=current->next;
	free(current->text);
	free(current);
    }
    fclose(fdo);

    printf("Done!\n");

    return(0);
}
