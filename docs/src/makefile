TOP=../..

include $(TOP)/config/make.cfg

DEST=../html/
FILES = background devinfo devinf_inside devinf_outside \
	history links

AUTODOC_SRCS = $(wildcard $(TOP)/config/m68k-native/*.s $(TOP)/exec/*.c \
	$(TOP)/dos/*.c $(TOP)/intuition/*.c $(TOP)/graphics/*.c \
	$(TOP)/utility/*.c $(TOP)/alib/*.c $(TOP)/aros/*.c)

INCLUDE_SRCS = $(wildcard $(TOP)/include/*.h $(TOP)/include/*/*.h)

SRCS = $(foreach f,$(FILES),$(f).src)
HTML = $(DEST)index.html $(foreach f,$(FILES),$(DEST)$(f).html)

all : autodocs $(HTML) $(DEST)filesystems.doc

$(DEST)filesystems.doc: filesystems.doc
	cp filesystems.doc $(DEST)

clean :
	rm -f $(HTML)

$(DEST)index.html : $(DEST)toc.html doc_header.html doc_footer.html
	cat doc_header.html $(DEST)toc.html \
	    doc_footer.html | sed "s/\\\\today/`date "+%d %b %Y"`/" \
	    > $(DEST)index.html

$(DEST)toc.html : $(SRCS) tochtml.gawk
	gawk -f tochtml.gawk $(SRCS) > $@

$(DEST)%.html : %.src src2html.gawk page_header.html page_footer.html
	gawk -f src2html.gawk $< > $@

autodocs: $(DEST)adoc_index.html

$(DEST)adoc_index.html: $(AUTODOC_SRCS) adoc2html.gawk adocpost.gawk
	gawk -f adoc2html.gawk $(AUTODOC_SRCS) | sort | \
	gawk -f adocpost.gawk > $@

$(DEST)inc_index.html: $(INCLUDE_SRCS)
	echo gawk -f inc2html.gawk $(AUTODOC_SRCS) > $@

