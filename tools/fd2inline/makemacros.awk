#! /bin/awk -f
#
# makemacros.awk
#
# Copyright (C) 1997 Kamil Iskra <iskra@student.uci.agh.edu.pl>
# Distributed under terms of GNU General Public License.
#
# This file is part of fd2inline package.
#
# It is used to produce <pInline/macros.h> file.

BEGIN {
	print "/* Automatically generated header! Do not edit! */"
	print
	print "#ifndef __INC_POS_PINLINE_MACROS_H"
	print "#define __INC_POS_PINLINE_MACROS_H"
	print

	for (i=0; i<8; i++)
		print "#define __INLINE_REG_A" i " \"a" i "\""
	for (i=0; i<8; i++)
		print "#define __INLINE_REG_D" i " \"d" i "\""
	print

	for (i=0; i<=10; i++)
	{
		printf "#define __INLINE_FUN_%d(__base, __lib, __offs, __type, __name", i
		for (j=1; j<=i; j++)
			printf ", __type%d, __name%d, __reg%d", j, j, j
		printf ") \\\n"

		printf "((__type (*)(void*"
		for (j=1; j<=i; j++)
			printf ", __type%d", j
		printf ")) \\\n"

		printf "*(ULONG*)(((char*)__lib)-__offs))"

		printf "(__base"
		for (j=1; j<=i; j++)
			printf ", __name%d", j
		printf ")\n\n"
	}

	print "#endif /* !__INC_POS_PINLINE_MACROS_H */"
	exit
}
