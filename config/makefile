TOP=..

include $(TOP)/config/make.cfg

SUBDIRS=$(KERNEL) $(ARCH)

all: $(TOP)/apps/compiler/include/aros/config.h
	@for dir in $(SUBDIRS) ; do \
	    echo "Making all in $(CURDIR)/$$dir..." ; \
	    if ( cd $$dir ; \
		$(MAKE) $(MFLAGS) TOP="$(TOP)/.." CURDIR="$(CURDIR)/$$dir" \
		all ) ; \
	    then echo -n ; else exit 1 ; fi ; \
	done

config: $(TOP)/apps/compiler/include/aros/config.h
	@for dir in $(SUBDIRS) ; do \
	    echo "Configuring in $(CURDIR)/$$dir..." ; \
	    if ( cd $$dir ; \
		$(MAKE) $(MFLAGS) TOP="$(TOP)/.." CURDIR="$(CURDIR)/$$dir" \
		config ) ; \
	    then echo -n ; else exit 1 ; fi ; \
	done

$(TOP)/apps/compiler/include/aros/config.h: $(TOP)/configure
	sh $(TOP)/configure -h $(TOP)/apps/compiler/include/aros/config.h

clean:
	$(RM) ./machine
	@for dir in $(SUBDIRS) ; do \
	    echo "Making all in $(CURDIR)/$$dir..." ; \
	    $(MAKE) $(MFLAGS) TOP="$(TOP)/.." CURDIR="$(CURDIR)/$$dir" \
		clean ) ; \
	done
	$(RM) host.cfg config.h

machine:
	$(CC) -I../apps/compiler/include -o machine machine.c
	./machine


