#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

struct inclist {
    struct inclist *next;
    char *text;
};

int fd,fdo;
char rbuf[1025];
int rbufc;
int pos=0;

void _backseek(int fh,int cnt)
{
    if(pos >= cnt)
	pos-=cnt;
    else
    {
	lseek(fh,(pos-cnt),SEEK_CUR);
	rbufc=read(fh,rbuf,1024);
	pos=0;
    }
}

int _read(int fh,char *buff,int cnt)
{
int curr=0;

    if( rbufc==0 || pos>(rbufc-1) )
    {
	pos=0;
	rbufc=read(fh,rbuf,1024);
    }
    if(rbufc!=0)
    {
	while( ( (pos+cnt-curr) > rbufc ) && (rbufc != 0) )
	{
	    strncpy(&buff[curr],&rbuf[pos],rbufc-pos);
	    curr+=rbufc-pos;
	    rbufc=read(fh,rbuf,1024);
	    pos=0;
	}
	if(rbufc != 0)
	{
	    strncpy(&buff[curr],&rbuf[pos],(cnt-curr));
	    pos+=(cnt-curr);
	    curr=cnt;
	}
	return(curr);
    }
    else
	return(0);
}

int main(int argc,char **argv)
{
int count;
int i,filecount;
char fbuf[50];

char ft[]="zyx";
struct inclist first = { NULL, ft}, *current, *search;

char bracket;
char incname[50];
char filename[50];

    fdo=open("functions.c",O_WRONLY|O_CREAT,0644);
    if(fdo==-1)
	printf("Could not open functions.c out-file!\n"),exit(-1);
    write(fdo,"#include \"functions.h\"\n",23);

    printf("Collecting functions...");
    for(filecount=1;filecount<argc;filecount++)
    {
/*	printf("Opening %s\n",argv[filecount]);*/
	strcpy(filename,argv[filecount]);
	strcat(filename,".c");
	fd=open(filename,O_RDONLY);
	if(fd==-1)
	    printf("\n%s - No such file !\n",argv[filecount]),exit(-1);

	rbufc=0;
	count=_read(fd,fbuf,1);
	while(count==1)
	{
	    if(fbuf[0]=='#')
	    {
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
			    _read(fd,&incname[i],1);
			} while(incname[i]!=bracket);
			incname[i+1]=0;
			search=&first;
			while(search->next!=NULL&&strcmp(search->text,incname)!=0)
			    search=search->next;
			if(search->next==NULL&&strcmp(search->text,incname)!=0)
			{
			    current=malloc(sizeof(struct inclist));
			    current->next=NULL;
			    current->text=malloc(sizeof(char)*(i+2));
			    strcpy(current->text,incname);
			    search->next=current;
			}
		    }
		    else
		    {
			write(fdo,"#include ",9);
			write(fdo,&incname[0],1);
		    }
		}
		else
		{
		    _backseek(fd,count);
		    write(fdo,fbuf,1);
		}
		if(count>0)
		    count=1;
	    }
	    else
		write(fdo,fbuf,1);
	    count=_read(fd,fbuf,1);
	}
	close(fd);
    }
    close(fdo);

    fdo=open("functions.h",O_WRONLY|O_CREAT,0644);
    if(fdo==-1)
	printf("Could not open functions.h out-file!\n"),
	exit(-1);
    write(fdo,"#define AROS_ALMOST_COMPATIBLE\n",31);
    search=first.next;
    while(search!=NULL)
    {
	current=search;
	write(fdo,"\n#include ",10);
	write(fdo,current->text,strlen(current->text));
	search=current->next;
	free(current->text);
	free(current);
    }
    close(fdo);

    printf("Done!\n");

    return(0);
}
