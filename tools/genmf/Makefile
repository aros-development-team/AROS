
genmf : genmf.c
	gcc -Wall -g -O genmf.c -o genmf

clean :
	rm -f genmf

check :
	genmf tmpl.test < in.test
	genmf --curdir=`pwd` tmpl.test < in.test

test : check
