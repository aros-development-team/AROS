# * P_LZ_COPYRIGHT_BEGIN ******************************************************
# * Copyright 2001-2004 Laszlo Systems, Inc.  All Rights Reserved.            *
# * Use is subject to license terms.                                          *
# * P_LZ_COPYRIGHT_END ********************************************************
import java.lang.Object
class LFCCompiler(java.lang.Object):
    def compile(self, args):
        "@sig public void compile(java.lang.String[] args)"
        from lzsc import main
        main(args)
