/*
 *  filesystem.c
 *  AROS
 *
 *  Created by Pavel Fedin on 10/13/10.
 *  Copyright 2010 AROS Development Team. All rights reserved.
 *
 */

#import <Foundation/Foundation.h>

#include "filesystem.h"

int SetRootDirectory(void)
{
    BOOL ret;

    NSString *home = NSHomeDirectory();
    NSString *dir = [home stringByAppendingPathComponent:@"Documents/AROS"];
    NSFileManager *fm = [NSFileManager defaultManager];

    ret = [fm changeCurrentDirectoryPath:dir];
    [fm release];

    return !ret;
}
