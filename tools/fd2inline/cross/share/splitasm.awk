#! /bin/awk -f
#
# splitasm.awk
#
# Copyright (C) 2000 Emmanuel Lesueur <lesueur@club-internet.fr>
# Distributed under terms of GNU General Public License.
#
# This file is part of fd2inline package.
#
# It is used to create linker libraries with stubs for MorphOS.
# (And AmigaOS. And Amithlon. /Martin Blom)

BEGIN {
    dir=dest
}

/^[\t ]?.globl[\t ]/ {
	currfn=dir "/" $2 ".s"
	print "\t.text\n\t.balign\t4\n" >currfn
#	print "\t.section\t\".text\"\n\t.align\t2\n" >currfn
}

currfn!="" {
	print $0 >currfn
}

/^[\t ].size/ {
	close(currfn)
	currfn=""
}

/^[\t ]rts/ {
	close(currfn)
	currfn=""
}

