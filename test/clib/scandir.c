/*
    Copyright © 1995-2017, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int filter(const struct dirent *de)
{
    return !strcmp(de->d_name, "T");
}

void testscandir(const char *dir,
    int (*select)(const struct dirent *),
    int (*compar)(const struct dirent **, const struct dirent **))
{
    int i;
    struct dirent **namelist;

    printf("\nscandir dir %s filter %p sort %p\n", dir, select, compar);

    int res = scandir(dir, &namelist, select, compar);
    printf("result %d\n", res);
    if (res < 0)
    {
        perror("scandir");
    }
    else
    {
        for (i=0 ; i < res ; i++)
        {
            printf("%d %s\n", i, namelist[i]->d_name);
            free(namelist[i]);
        }
        free(namelist);
    }
}

int main(void)
{
    testscandir("ram:", NULL, NULL);
    testscandir("ram:", NULL, alphasort);
    testscandir("ram:", filter, NULL);
    return 0;
}
