/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <stdio.h>
#include <time.h>

int main(int argc, char **argv) {
    time_t now, mk;
    char buf[26], *pbuf;
    struct tm tm;

    now = time(NULL);
    printf("time: %d\n", (int)now);

    pbuf = ctime(&now);
    printf("ctime: %s", pbuf);
    pbuf = ctime_r(&now, &buf[0]);
    printf("ctime_r: %s", buf);

    mk = mktime(gmtime(&now));
    printf("gmtime: %d\n", (int)mk);
    mk = mktime(gmtime_r(&now, &tm));
    printf("gmtime_r: %d\n", (int)mk);

    mk = mktime(localtime(&now));
    printf("localtime: %d\n", (int)mk);
    mk = mktime(localtime_r(&now, &tm));
    printf("localtime_r: %d\n", (int)mk);

    pbuf = asctime(&tm);
    printf("asctime: %s", pbuf);
    pbuf = asctime_r(&tm, &buf[0]);
    printf("asctime_r: %s", buf);

    return 0;
}
