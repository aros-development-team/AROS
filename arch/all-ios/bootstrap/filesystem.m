/*
 *  filesystem.c
 *  AROS
 *
 *  Created by Pavel Fedin on 10/13/10.
 *  Copyright 2010 AROS Development Team. All rights reserved.
 *
 */

#include <Foundation/Foundation.h>
#include <unistd.h>

#include "filesystem.h"

int SetRootDirectory(void)
{
    int ret;

    NSString *home = NSHomeDirectory();
    
    ret = chdir([home UTF8String]);
    if (!ret)
	ret = chdir("Documents/AROS");
    
    return ret;
}
