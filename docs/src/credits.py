#!/usr/local/bin/python
#
# NAME:
#
# credits.py
#
# FUNCTION:
#
# Parses AROS/docs/src/credits into a list 'credits' of work areas and
# lists of their associated contributors. Ie, something like this:
#
# [ [ 'Area1', [ 'Name1', 'Name2' ]], [ 'Area2', [ 'Name3' ]] ]
#
# See below for an example on how to use it.
#
# EXAMPLE:
#
# # The following program will print the credits in the same
# # format as the input file is in.
#
# from docs.src.credits import credits
#
# for area, names in credits:
#     print '\n' + area + ':'
#
#     for name in names:
#         print '    ' + name
#

import os, sys, string

# Input file, change this if need be.

path = os.path.dirname( __file__ )
filename = os.path.join( path, 'credits' )

# Read the file in.

file = open( filename, 'r' )

lines = file.readlines()

file.close()

# Parse it.

credits = []
names   = []

for line in lines:
    line = string.strip( line )

    if ':' in line:
	if len( names ) > 0:
	    credits.append( [ area, names ] )

	area = line[:-1]

	names = []

    elif line != '':
	names.append( line )

if len( names ) > 0:
    credits.append( [ area, names ] )
