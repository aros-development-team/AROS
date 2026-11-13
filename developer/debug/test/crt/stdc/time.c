/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.
*/

#include <stdio.h>
#include <time.h>

int main(int argc, char **argv) {
    time_t now, mk;
    char *pbuf;

    now = time(NULL);
    printf("time: %d\n", (int)now);

    pbuf = ctime(&now);
    printf("ctime: %s", pbuf);

    mk = mktime(gmtime(&now));
    printf("gmtime: %d\n", (int)mk);

    mk = mktime(localtime(&now));
    printf("localtime: %d\n", (int)mk);

    pbuf = asctime(localtime(&now));
    printf("asctime: %s", pbuf);

    return 0;
}

