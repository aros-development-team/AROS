# File: Regenerator.py

# * P_LZ_COPYRIGHT_BEGIN ******************************************************
# * Copyright 2001-2004 Laszlo Systems, Inc.  All Rights Reserved.            *
# * Use is subject to license terms.                                          *
# * P_LZ_COPYRIGHT_END ********************************************************

import java.lang.Object
class Regenerator(java.lang.Object):
    def compile(self, props, args):
        "@sig public void compile(java.util.Properties props, java.lang.String[] args)"
        includeObjectHierarchy = 1
        if args and args[0] == '--mconly':
            includeObjectHierarchy = 0
            args = args[1:]
        from obj2as import optimize
        for fname in args:
            optimize(fname, props, includeObjectHierarchy)
