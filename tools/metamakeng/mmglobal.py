# -*- coding: iso-8859-15 -*-


# global variables
verbose = False
quiet = False
debug = False

def debugout(str):
    if debug:
        print "[MMAKE Debug] %s" % (str)
