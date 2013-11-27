# -*- coding: iso-8859-15 -*-

import errno, logging, os, shutil, sys

###################################################################################################

def makedir(path):
    try:
        os.makedirs(path)
    except OSError as exc:
        if exc.errno == errno.EEXIST and os.path.isdir(path):
            pass # path exists and is directory
        else:
            raise


def copy_tree(src, dst, ignore):
        names = os.listdir(src)
        makedir(dst)

        for name in names:
            srcname = os.path.join(src, name)
            dstname = os.path.join(dst, name)

            if os.path.isdir(srcname):
                if name not in ("CVS", ".svn"):
                    copy_tree(srcname, dstname, ignore)
            else:
                if name not in (".cvsignore", "mmakefile.src") and name not in ignore:
                    if not os.path.exists(dstname) or (os.path.getmtime(dstname) < os.path.getmtime(srcname)):
                        shutil.copy(srcname, dstname)


###################################################################################################

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
            makedir(path)
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
        makedir(dstdir)

        for file in self.files:
            file = self.buildenv.substitute(file)
            srcfile = os.path.join(srcdir, file)
            dstfile = os.path.join(dstdir, file)
            if not os.path.exists(dstfile) or (os.path.getmtime(dstfile) < os.path.getmtime(srcfile)):
                logging.info("[MMAKE] copying file '%s' to '%s'" % (srcfile, dstfile))
                shutil.copyfile(srcfile, dstfile)
        return True


class CopyDirRecursive(Function):
    def __init__(self, buildenv, src, dst, excludefiles):
        Function.__init__(self, buildenv)
        self.src = src
        self.dst = dst
        self.excludefiles = excludefiles


    def execute(self):
        dstdir = self.buildenv.substitute(self.dst)
        srcdir = self.buildenv.substitute(self.src)
        logging.info("[MMAKE] copying directory '%s' to '%s'" % (srcdir, dstdir))
        copy_tree(srcdir, dstdir, self.excludefiles)
        return True

