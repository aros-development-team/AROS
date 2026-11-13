#! /bin/awk -f
#
# makepragmas.awk
#
# Copyright (C) 1996 Kamil Iskra <iskra@student.uci.agh.edu.pl>
# Distributed under terms of GNU General Public License.
#
# This file is part of fd2inline package.
#
# It is used to produce SAS/C compatible "pragmas" files.
#
# Input variables:
# PRAGMAS - basename of "pragma" file to create.

BEGIN {
	print "/* Automatically generated header! Do not edit! */"
	print
	print "#ifndef _INLINE_" toupper(PRAGMAS) "_H"
	print "#include <inline/" PRAGMAS ".h>"
	print "#endif /* !_INLINE_" toupper(PRAGMAS) "_H */"
	exit
}
