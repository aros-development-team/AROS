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

int main(int argc,char **argv)
{
int count;
char fbuf[33];
int i,filecount;

char ft[]="zyx";
struct inclist first = { NULL, ft}, *current = &first, *search;

char bracket;
char incname[50];
char filename[50];

    fdo=open("functions.c",O_WRONLY|O_CREAT,0644);
    if(fdo==-1)
        printf("Could not open functions.c out-file!\n"),exit(-1);
    write(fdo,"#include \"functions.h\"\n",23);

    for(filecount=1;filecount<argc;filecount++)
    {
        printf("Opening %s\n",argv[filecount]);
	strcpy(filename,argv[filecount]);
	strcat(filename,".c");
        fd=open(filename,O_RDONLY);
        if(fd==-1)
            printf("No such file !\n"),exit(-1);

        count=1;
        while(count==1)
        {
            count=read(fd,fbuf,1);
            if(fbuf[0]=='#' && count==1)
            {
                count=read(fd,&fbuf[1],8);
                fbuf[count+1]=0;
                if(strcmp(fbuf,"#include ")==0)
                {
                    current=malloc(sizeof(struct inclist));
                    current->next=NULL;
                    read(fd,incname,1);
                    if(incname[0]=='<'||incname[0]==0x22)
                    {
                        if(incname[0]=='<')
                            bracket='>';
                        else
                            bracket=0x22;
                        i=0;
                        do {
                            i++;
                            read(fd,&incname[i],1);
                        } while(incname[i]!=bracket);
                        incname[i+1]=0;
                        current->text=malloc(sizeof(char)*(i+2));
                        strcpy(current->text,incname);
                        search=&first;
                        while(search->next!=NULL&&strcmp(search->text,current->text)!=0)
                            search=search->next;
                        if(search->next==NULL&&strcmp(search->text,current->text)!=0)
                            search->next=current;
                        else
                        {
                            free(current->text);
                            free(current);
                        }
                    }
                    else
                        write(fdo,&incname[0],1);
                }
                else
                    write(fdo,fbuf,count+1);
                if(count>0)
		    count=1;
            }
            else
                write(fdo,fbuf,1);
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
    
    while(search->next!=NULL)
    {
        current=search;
        search=search->next;
        write(fdo,"\n#include ",10);
        write(fdo,current->text,strlen(current->text));
        free(current->text);
        free(current);
    }
    write(fdo,"\n#include ",10);
    write(fdo,search->text,strlen(search->text));
    free(search->text);
    free(search);
    close(fdo);

return(0);
}
