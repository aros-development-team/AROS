TOP=..

include $(TOP)/config/make.cfg

SUBDIRS=$(ARCH) $(KERNEL)

all:
	@for dir in $(SUBDIRS) ; do \
	    echo "Making all in $(CURDIR)/$$dir..." ; \
	    if ( cd $$dir ; \
		$(MAKE) $(MFLAGS) TOP="$(TOP)/.." CURDIR="$(CURDIR)/$$dir" \
		all ) ; \
	    then echo -n ; else exit 1 ; fi ; \
	done

config:
	@for dir in $(SUBDIRS) ; do \
	    echo "Configuring in $(CURDIR)/$$dir..." ; \
	    if ( cd $$dir ; \
		$(MAKE) $(MFLAGS) TOP="$(TOP)/.." CURDIR="$(CURDIR)/$$dir" \
		config ) ; \
	    then echo -n ; else exit 1 ; fi ; \
	done

clean:
	$(RM) ./machine
	@for dir in $(SUBDIRS) ; do \
	    echo "Making all in $(CURDIR)/$$dir..." ; \
	    $(MAKE) $(MFLAGS) TOP="$(TOP)/.." CURDIR="$(CURDIR)/$$dir" \
		clean ) ; \
	done

machine:
	$(CC) -I../include -o machine machine.c
	./machine


