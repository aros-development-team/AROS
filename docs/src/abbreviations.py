'''Python module to handle abbreviations used in AROS.
Usage:

    import sys, os.path
    sys.path.append (os.path.join (AROSDIR, 'docs', 'src'))

    from abbreviations import abbreviations

You can use the object abbreviations just like any other dictionary.
It will return the lookup value for all unknown abbreviations.
'''

import os, sys, string, fileinput

class MyDict:
    def __init__ (self):
	self.data = {}

    def __setitem__ (self, key, value):
	raise 'Immutable type'

    def set (self, key, value):
	self.data[key] = value

    def __getitem__ (self, key, default=''):
	return self.data.get (key, default)

    def has_key (self, key):
	return self.data.has_key (key)

abbreviations = MyDict ()

# Find the file
path = os.path.dirname (__file__)
#print path
filename = os.path.join (path, 'abbreviations.dat')
#print filename

# Read contents
fh = open (filename, 'r')
for line in fileinput.input (filename):
    key, value = string.split (string.rstrip (line), ';')
    
    abbreviations.set (key, value)

