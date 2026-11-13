/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.

    Exercises the re-entrant (_r) time conversion functions, which live in
    posixc.library.
*/

#include <stdio.h>
#include <time.h>

int main(int argc, char **argv) {
    time_t now, mk;
    char buf[26], *pbuf;
    struct tm tm;

    now = time(NULL);
    printf("time: %d\n", (int)now);

    pbuf = ctime_r(&now, &buf[0]);
    printf("ctime_r: %s", pbuf);

    mk = mktime(gmtime_r(&now, &tm));
    printf("gmtime_r: %d\n", (int)mk);

    mk = mktime(localtime_r(&now, &tm));
    printf("localtime_r: %d\n", (int)mk);

    pbuf = asctime_r(&tm, &buf[0]);
    printf("asctime_r: %s", pbuf);

    return 0;
}
