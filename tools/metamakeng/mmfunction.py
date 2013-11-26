# -*- coding: iso-8859-15 -*-

import errno, logging, os, sys

class Function:
    def __init__(self, buildenv):
        self.buildenv = buildenv
        self.directory = buildenv.get_variable("CURDIR")


class Output(Function):
    def __init__(self, buildenv, text):
        Function.__init__(self, buildenv)
        self.text = text


    def execute(self):
        text = self.buildenv.substitute(self.text)
        print text
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
