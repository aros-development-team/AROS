TOP=../..

include $(TOP)/config/make.cfg

DEST=../html/
FILES = background devinfo devinf_inside devinf_outside \
	history links

AUTODOC_SRCS = $(wildcard $(TOP)/config/m68k-native/*.s $(TOP)/exec/*.c \
	$(TOP)/dos/*.c $(TOP)/intuition/*.c $(TOP)/graphics/*.c \
	$(TOP)/utility/*.c $(TOP)/alib/*.c $(TOP)/aros/*.c \
	$(TOP)/devs/*.c)

INCLUDE_SRCS = $(wildcard $(TOP)/include/*.h $(TOP)/include/*/*.h)

SRCS = $(foreach f,$(FILES),$(f).src)
HTML = $(DEST)index.html $(foreach f,$(FILES),$(DEST)$(f).html)

all : $(DEST)srcs autodocs $(HTML) $(DEST)filesystems.doc \
	$(DEST)cgi-bin/search.cgi \
	$(DEST)cgi-bin/counter.cgi

index :
	cd $(DEST) ; glimpseindex -H . -o .

$(DEST)srcs:
	ln -s $(TOP) $(DEST)srcs

$(DEST)cgi-bin/search.cgi : search.cgi
	cp search.cgi $@
	chmod 755 $@

$(DEST)cgi-bin/counter.cgi : counter.cgi
	cp counter.cgi $@
	chmod 755 $@

$(DEST)filesystems.doc: filesystems.doc
	cp filesystems.doc $(DEST)

clean :
	rm -f $(HTML)

$(DEST)index.html : $(DEST)toc.html doc_header.html doc_footer.html
	cat doc_header.html $(DEST)toc.html \
	    doc_footer.html | sed "s/\\\\today/`date "+%d %b %Y"`/" \
	    > $@
	chmod a+r $@

$(DEST)toc.html : $(SRCS) tochtml.gawk
	gawk -f tochtml.gawk $(SRCS) > $@
	chmod a+r $@

$(DEST)%.html : %.src src2html.gawk page_header.html page_footer.html
	gawk -f src2html.gawk $< > $@
	chmod a+r $@

autodocs: $(DEST)adoc_index.html

$(DEST)adoc_index.html: $(DEST)adoc_index.tmp adocpost.gawk adoc_header.html \
	    adoc_footer.html
	cat adoc_header.html > $@
	gawk -F: -f adocpost.gawk --assign mode=pre_bylib $(DEST)adoc_index.tmp | \
	sort -f | \
	gawk -F: -f adocpost.gawk --assign mode=post_bylib >> $@
	gawk -F: -f adocpost.gawk --assign mode=pre_byname $(DEST)adoc_index.tmp | \
	sort -f | \
	gawk -F: -f adocpost.gawk --assign mode=post_byname >> $@
	cat adoc_footer.html | sed "s/\\\\today/`date "+%d %b %Y"`/" >> $@
	chmod a+r $@

$(DEST)adoc_index.tmp: $(AUTODOC_SRCS) adoc2html.gawk
	gawk -f adoc2html.gawk $(AUTODOC_SRCS) > $@
	chmod -R a+r $(DEST)autodocs

$(DEST)inc_index.html: $(INCLUDE_SRCS)
	echo gawk -f inc2html.gawk $(AUTODOC_SRCS) > $@
	chmod a+r $@

