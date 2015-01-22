/**
***  SetRev.c   Set revision number due to the Amiga guidelines
***
***  This is a small BumpRev replacement (yes, yet another :-).
***  Unlike BumpRev, UpRev and others this doesn't need to use
***  a special revision file. Instead it scans a source file for
***  certain patterns which are replaced.
***
***
***  Author:    Jochen Wiedmann
***             Am Eisteich 9
***             72555 Metzingen
***             Germany
***
***             Phone: (49)7123 / 14881
***             Internet: jochen.wiedmann@zdv.uni-tuebingen.de
***
***
***  This program is in the public domain; use it as you want,
***  but WITHOUT ANY WARRANTY!
**/


/**
***  These are the patterns that will be replaced.
**/
#define VERSION        1
#define REVISION       4
#define DATE    "11.05.2011"
#define VERS    "SetRev 1.4"
#define VSTRING "SetRev 1.4 (11.05.2011)"
#define VERSTAG "\0$VER: SetRev 1.4 (11.05.2011)"
const char *const VersionString=VERSTAG;




/**
***  Include files
**/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <errno.h>

#if defined(__GNUC__)
#define stricmp strcasecmp
#define strnicmp strncasecmp
#endif


#ifndef FALSE
#define FALSE 0
#endif
#ifndef TRUE
#define TRUE (!FALSE)
#endif





/**
***  SetRev cannot scan files with lines longer than MAXLINE character.
**/
#define MAXLINE 4096




/**
***  Global variables
**/
int CurrentVersion = 1;
int CurrentRevision = 1;
char *CurrentName;
char CurrentDate[20];

char line[MAXLINE];

int Created = FALSE;



/**
***  This function determines the current name, version
***  and revision.
**/
void FirstScan(char *file)

  {
    FILE *fh;
    int linenum = 1;

    if (!(fh = fopen(file, "r")))
      {
        if (errno == ENOENT)
          {
            /**
            ***  File doesn't exist, create it.
            **/
            if (!(fh = fopen(file, "w")))
              {
                perror("SetRev");
                exit(10);
              }

            if (fprintf(fh,
                       "#define VERSION     %d\n",
                       CurrentVersion)  <  0    ||
                fprintf(fh,
                       "#define REVISION    %d\n",
                       CurrentRevision)  <  0   ||
                fprintf(fh,
                       "#define DATE        \"%s\"\n",
                       CurrentDate)  <  0       ||
                fprintf(fh,
                       "#define VERS        \"%s %d.%d\"\n",
                       CurrentName, CurrentVersion,
                       CurrentRevision)  <  0   ||
                fprintf(fh,
                       "#define VSTRING     \"%s %d.%d (%s)\\r\\n\"\n",
                       CurrentName, CurrentVersion,
                       CurrentRevision, CurrentDate)  <  0      ||
                fprintf(fh,
                       "#define VERSTAG     \"\\0%sVER: %s %d.%d (%s)\"\n",
                       "$", /*  Prevent "version" command from using the    */
                            /*  above string.                               */
                       CurrentName, CurrentVersion,
                       CurrentRevision, CurrentDate)  <  0      ||
                fclose(fh))
              {
                perror("SetRev");
                exit(10);
              }

            Created = TRUE;

            return;
          }
        perror("SetRev");
        exit(10);
      }

    errno = 0;
    while(fgets(line, MAXLINE, fh)  &&  !errno)
      {
        int len = strlen(line);

        if (len+1 == MAXLINE  &&  line[MAXLINE-2] != '\n')
          {
            fprintf(stderr,
                    "SetRev warning: Line %d exceeds %d characters\n",
                    linenum, MAXLINE);
          }

        if (strncmp(line, "#define", 7) == 0)
          {
            char *ptr = line+7;

            while (*ptr == ' '  ||  *ptr == '\t')
              {
                ++ptr;
              }

            if (strncmp(ptr, "VERSION", 7) == 0  &&
                (ptr[7] == ' ' || ptr[7] == '\t'))
              {
                CurrentVersion = atoi(&ptr[7]);
              }
            else if (strncmp(ptr, "REVISION", 8) == 0  &&
                     (ptr[8] == ' '  ||  ptr[8] == '\t'))
              {
                CurrentRevision = atoi(&ptr[8]);
              }
          }

        if (line[len-1] == '\n')
          {
            ++linenum;
          }
      }
    fclose(fh);
  }





/**
***  This function inserts the new version, revision,
***  projectname and other stuff in a second scan.
**/
void SecondScan(char *file)

  {
    char *bakfile;
    FILE *fh, *bfh;
    int linenum = 1;

    if (!(bakfile = malloc(strlen(file)+5)))
      {
        perror("SetRev");
        exit(10);
      }
    strcpy(bakfile, file);
    strcat(bakfile, ".bak");

    if (!(fh = fopen(file, "r"))  ||  !(bfh = fopen(bakfile, "w")))
      {
        perror("SetRev");
        exit(10);
      }

    errno = 0;
    while(fgets(line, MAXLINE, fh)  &&  !errno)
      {
        int len = strlen(line);

        if (len+1 == MAXLINE  &&  line[MAXLINE-2] != '\n')
          {
            fprintf(stderr,
                    "SetRev warning: Line %d exceeds %d characters\n",
                    linenum, MAXLINE);
          }

        if (strncmp(line, "#define", 7) == 0)
          {
            char *ptr = line+7;

            while (*ptr == ' '  ||  *ptr == '\t')
              {
                ++ptr;
              }

            if (strncmp(ptr, "VERSION", 7) == 0  &&
                (ptr[7] == ' ' || ptr[7] == '\t'))
              {
                ptr += 7;
                while (*ptr == ' '  ||  *ptr == '\t')
                  {
                    ++ptr;
                  }
                sprintf(ptr, "%d\n", CurrentVersion);
              }
            else if (strncmp(ptr, "REVISION", 8) == 0  &&
                     (ptr[8] == ' '  ||  ptr[8] == '\t'))
              {
                ptr += 8;
                while (*ptr == ' '  ||  *ptr == '\t')
                  {
                    ++ptr;
                  }
                sprintf(ptr, "%d\n", CurrentRevision);
              }
            else if (strncmp(ptr, "DATE", 4) == 0  &&
                     (ptr[4] == ' '  ||  ptr[4] == '\t'))
              {
                ptr += 4;
                while (*ptr == ' '  ||  *ptr == '\t')
                  {
                    ++ptr;
                  }
                sprintf(ptr, "\"%s\"\n", CurrentDate);
              }
            else if (strncmp(ptr, "VERS", 4) == 0  &&
                     (ptr[4] == ' '  ||  ptr[4] == '\t'))
              {
                ptr += 4;
                while (*ptr == ' '  ||  *ptr == '\t')
                  {
                    ++ptr;
                  }
                sprintf(ptr, "\"%s %d.%d\"\n", CurrentName,
                        CurrentVersion, CurrentRevision);
              }
            else if (strncmp(ptr, "VSTRING", 7) == 0  &&
                     (ptr[7] == ' '  ||  ptr[7] == '\t'))
              {
                ptr += 7;
                while (*ptr == ' '  ||  *ptr == '\t')
                  {
                    ++ptr;
                  }
                sprintf(ptr, "\"%s %d.%d (%s)\"\n",
                        CurrentName, CurrentVersion,
                        CurrentRevision, CurrentDate);
              }
            else if (strncmp(ptr, "VERSTAG", 7) == 0  &&
                     (ptr[7] == ' '  ||  ptr[7] == '\t'))
              {
                ptr += 7;
                while (*ptr == ' '  ||  *ptr == '\t')
                  {
                    ++ptr;
                  }
                sprintf(ptr, "\"\\0%sVER: %s %d.%d (%s)\"\n",
                        "$",
                        CurrentName, CurrentVersion,
                        CurrentRevision, CurrentDate);
              }
          }
        fputs(line, bfh);
      }
    fclose(fh);
    fclose(bfh);

    remove(file);
    rename(bakfile, file);
  }





/**
***  This prints out the Usage: message.
**/
void Usage(void)

  {
    printf("Usage: SetRev PROJECT/A,VERSION/N,FILE/K\n\n");
    printf("The given FILE (default: PROJECT_rev.h) will be searched for\n");
    printf("version and revision definitions and bumped to the next\n");
    printf("revision number.\n\n");
    printf("%s %c 1994  by  Jochen Wiedmann\n\n", VSTRING, 0xa9);
    printf("This program is in the public domain, use it as you want, but\n");
    printf("WITHOUT ANY WARRANTY.\n");
    exit(5);
  }





/**
***  Finally main()
**/
int main(int argc, char *argv[])

  {
    int i;
    char *file;
    int versionseen, version;
    time_t currenttime = time(NULL);
    struct tm *localtm = localtime(&currenttime);

    /**
    *** Get local time
    **/
    currenttime = time(NULL);
    localtm = localtime(&currenttime);
    strftime(CurrentDate, sizeof(CurrentDate), "%d.%m.%Y", localtm);

    if (argc < 2  ||
        strcmp(argv[1], "?") == 0  ||
        strcmp(argv[1], "-h") == 0  ||
        strcmp(argv[1], "--help") == 0)
      {
        Usage();
      }

    CurrentName = NULL;
    versionseen = FALSE;
    file = NULL;
    for (i = 1;  i < argc;  i++)
      {
        if (stricmp(argv[i], "file") == 0  &&  i+1 < argc)
          {
            if (file)
              {
                Usage();
              }
            file = argv[++i];
          }
        else if (strnicmp(argv[i], "file=", 5) == 0)
          {
            if (file)
              {
                Usage();
              }
            file = &argv[i][5];
          }
        else if (!CurrentName)
          {
            CurrentName = argv[i];
          }
        else if (!versionseen)
          {
            if ((version = atoi(argv[i])) == 0)
              {
                Usage();
              }
            versionseen = TRUE;
          }
        else
          {
            Usage();
          }
      }
    if (!CurrentName)
      {
        Usage();
      }

    if (!file)
      {
        if (!(file = malloc(strlen(CurrentName) + 7)))
          {
            errno = ENOMEM;
            perror("SetRev");
            exit(20);
          }
        strcpy(file, CurrentName);
        strcat(file, "_rev.h");
      }

    if (versionseen)
      {
        CurrentVersion = version;
      }

    FirstScan(file);

    if (!Created)
      {
        if (versionseen  &&  version != CurrentVersion)
          {
            CurrentRevision = 1;
          }
        else
          {
            ++CurrentRevision;
          }

        if (versionseen)
          {
            CurrentVersion = version;
          }

        SecondScan(file);
      }
    exit(0);
  }
