/*
    preplink.c - description
    $Id$
*/

/*
    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your opinion) any later version.
*/

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>

volatile int bswap(int num)
{
    int ret =  (((num & 0x000000ff) << 24) |
    		((num & 0x0000ff00) << 8 ) |
      		((num & 0x00ff0000) >> 8 ) |
        	((num & 0xff000000) >> 24));
    return ret;
}

struct partition_entry {
    unsigned char 	boot_indicator;
    unsigned char	starting_head;
    unsigned char	starting_sector;
    unsigned char	starting_cylinder;

    unsigned char	system_indicator;
    unsigned char	ending_head;
    unsigned char	ending_sector;
    unsigned char	ending_cylinder;

    unsigned long	beginning_sector;
    unsigned long	number_of_sectors;
};

int main(int argc, char *argv[])
{
    unsigned char buff[512]={0,};
    struct partition_entry *pe = (struct partition_entry *)&buff[0x1be];
    unsigned long *entry = (unsigned long*)&buff[0];
    unsigned long *length = (unsigned long*)&buff[sizeof(long)];
    struct stat info;
    int n;
    
    int in,out;

    if (!strcmp(argv[1],"-"))
    {
    	in=0;
    }
    else if ((in=open(argv[1], 0)) < 0)
    {
	exit(-1);
    }

    if (!strcmp(argv[2],"-"))
    {
    	out=0;
    }
    else if ((out=creat(argv[2], 0755)) < 0)
    {
	exit(-1);
    }

    if (fstat(in, &info) < 0)
    {
	fprintf(stderr, "info failed\n");
	exit(-1);
    }
    
#ifdef __i386__
    *entry = 0x400;
    *length = info.st_size + 0x400;
    pe->number_of_sectors = 2*18*80;
#else
    *entry = bswap(0x400);
    *length = bswap(info.st_size + 0x400);
    pe->number_of_sectors = bswap(2*18*80);
#endif

    buff[510] = 0x55;
    buff[511] = 0xaa;

    pe->boot_indicator = 0x80;
    pe->system_indicator = 0x41;


    pe->starting_head		= 0;
    pe->starting_sector		= 2;
    pe->starting_cylinder	= 2;
    pe->ending_head		= 1;
    pe->ending_sector		= 18;
    pe->ending_cylinder		= 79;

    pe->beginning_sector	= 0;
    
    write(out, buff, sizeof(buff));
    write(out, entry, sizeof(*entry));
    write(out, length, sizeof(*length));
    lseek(out, 0x400, SEEK_SET);
    
    while ((n=read(in, buff, sizeof(buff))) > 0)
    	write(out, buff, n);
    
    return 0;
}
