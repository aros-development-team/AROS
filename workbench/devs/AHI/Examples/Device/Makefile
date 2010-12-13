
MAKE	= SC:c/smake
MFLAGS	= 
SUBDIRS	= AHI-Handler PlayTest RecordTest

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
