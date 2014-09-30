/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <proto/dos.h>
#include <dos/dos.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mount.h> /* mnt_names is defined here */
#include "test.h"

int main() 
{
    TEST(strcmp(mnt_names[MOUNT_NONE], "none") == 0);
    TEST(strcmp(mnt_names[MOUNT_UFS], "ufs") == 0);
    TEST(strcmp(mnt_names[MOUNT_NFS], "nfs") == 0);
    TEST(strcmp(mnt_names[MOUNT_MFS], "mfs") == 0);
    TEST(strcmp(mnt_names[MOUNT_PC], "pc") == 0);
    TEST(strcmp(mnt_names[MOUNT_ADOS_OFS], "ofs") == 0);
    TEST(strcmp(mnt_names[MOUNT_ADOS_FFS], "ffs") == 0);
    TEST(strcmp(mnt_names[MOUNT_ADOS_IOFS], "iofs") == 0);
    TEST(strcmp(mnt_names[MOUNT_ADOS_IFFS], "iffs") == 0);
  
    return OK;
}

void cleanup() 
{
}
