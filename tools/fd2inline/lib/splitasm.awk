#! /bin/awk -f
#
# splitasm.awk
#
# Copyright (C) 1995, 96 Kamil Iskra <iskra@student.uci.agh.edu.pl>
# Distributed under terms of GNU General Public License.
#
# This file is part of fd2inline package.
#
# It is used to create linker libraries with stubs for Amiga shared libraries'
# functions.

/^.globl/ {
	currfn=substr($2, 2) ".s"
	print ".text\n\t.even" >currfn
}

currfn!="" {
	print $0 >currfn
}

/^\trts$/ {
	close(currfn)
	currfn=""
}
