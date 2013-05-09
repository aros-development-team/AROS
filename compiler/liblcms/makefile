SHELL = /bin/sh

init: FORCED
	@$(MAKE) -s all

all install test::
	echo "little cms color management engine 1.09 is being made $@."
	echo
	echo "To install additional utilities, type 'make utils'"
	echo
	echo "please wait ..."
	cd src ; $(MAKE) $@ ; cd .. ; cd samples ; $(MAKE) $@ ; cd .. ; cd testbed ; $(MAKE) $@ ; cd .. 

clean: FORCED
	cd src ; $(MAKE) $@ ; cd .. ; cd samples ; $(MAKE) $@ ; cd .. ; cd testbed ; $(MAKE) $@ ; cd .. ; \
	cd tifficc; $(MAKE) $@ ; cd .. ; cd jpegicc; $(MAKE) $@ ; cd ..


utils: FORCED
	cd tifficc; $(MAKE) $@ ; cd .. ; 	cd jpegicc; $(MAKE) $@ ; cd ..

tifficc: FORCED
	cd tifficc; $(MAKE) $@ ; cd ..

jpegicc: FORCED
	cd jpegicc; $(MAKE) $@ ; cd ..


FORCED:


