all : real-all

include $(TOP)/config/make.cfg

real-all: $(GENMF)

$(GENMF) : genmf.c
	gcc -Wall -g -O genmf.c -o $(GENMF)

clean :
	rm -f $(GENMF)

check :
	$(GENMF) tmpl.test < in.test
	$(GENMF) --curdir=`pwd` tmpl.test < in.test

test : check
