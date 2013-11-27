# -*- coding: iso-8859-15 -*-

import errno, logging, os, shutil, sys

class Function:
    def __init__(self, buildenv):
        self.buildenv = buildenv
        # set the directory so that the builder can be called
        # in the right directory when executed.
        self.directory = buildenv.get_variable("CURDIR")


class Output(Function):
    def __init__(self, buildenv, text):
        Function.__init__(self, buildenv)
        # remember the argument(s)
        self.text = text


    def execute(self):
        # do substitution on the argument. We must do it during execution
        # because some variables like CURDIR are constantly changed.
        text = self.buildenv.substitute(self.text)
        # do the action
        print text
        # return True if it worked well
        return True


class MkDirs(Function):
    def __init__(self, buildenv, dirs):
        Function.__init__(self, buildenv)
        self.dirs = dirs


    def execute(self):
        for path in self.dirs:
            path = self.buildenv.substitute(path)
            logging.info("[MMAKE] creating directory '%s'" % (path))
            try:
                os.makedirs(path)
            except OSError as exc:
                if exc.errno == errno.EEXIST and os.path.isdir(path):
                    pass # path exists and is directory
                else:
                    raise
        return True


class CopyFiles(Function):
    def __init__(self, buildenv, files, src, dst):
        Function.__init__(self, buildenv)
        self.files = files
        self.src = src
        self.dst = dst


    def execute(self):
        dstdir = self.buildenv.substitute(self.dst)
        srcdir = self.buildenv.substitute(self.src)
        try:
            os.makedirs(dstdir)
        except OSError as exc:
            if exc.errno == errno.EEXIST and os.path.isdir(dstdir):
                pass # path exists and is directory
            else:
                raise

        for file in self.files:
            file = self.buildenv.substitute(file)
            srcfile = os.path.join(srcdir, file)
            dstfile = os.path.join(dstdir, file)
            if not os.path.exists(dstfile) or (os.path.getmtime(dstfile) < os.path.getmtime(srcfile)):
                logging.info("[MMAKE] copying file '%s' to '%s'" % (srcfile, dstfile))
                shutil.copyfile(srcfile, dstfile)
        return True
