
SUBDIRS	= Device Low-level Extras

dist:
	@for i in $(SUBDIRS) ;\
	do \
	echo "Making +$$i..."; \
	(cd $$i && $(MAKE) $(MFLAGS) $@); \
	done

clean:
	@for i in $(SUBDIRS) ;\
	do \
	echo "Cleaning +$$i..."; \
	(cd $$i && $(MAKE) $(MFLAGS) $@); \
	done

allclean:
	@for i in $(SUBDIRS) ;\
	do \
	echo "Cleaning +$$i... (all)"; \
	(cd $$i && $(MAKE) $(MFLAGS) $@); \
	done
