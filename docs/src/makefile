# $Id$
TOP=../..

include $(TOP)/config/make.cfg

DEST=../html/
FILES = background faq devinfo-1 devinfo-2 devinfo-3 devinfo-4 devinfo-5 \
	devinf_inside devinf_outside \
	history links

AUTODOC_SRCS = $(wildcard $(TOP)/config/m68k-native/*.s $(TOP)/rom/exec/*.c \
	$(TOP)/rom/dos/*.c $(TOP)/rom/intuition/*.c $(TOP)/rom/graphics/*.c \
	$(TOP)/rom/utility/*.c $(TOP)/apps/compiler/alib/*.c \
	$(TOP)/apps/compiler/aros/*.c \
	$(TOP)/rom/devs/*.c $(TOP)/apps/compiler/clib/*.c \
	$(TOP)/workbench/libs/*/*.c)

INCLUDE_SRCS = $(wildcard $(TOP)/include/*.h $(TOP)/include/*/*.h)

SRCS = $(foreach f,$(FILES),$(f).src)
HTML = $(foreach f,$(FILES),$(DEST)$(f).html) $(DEST)index.html

all : $(DEST)srcs autodocs $(HTML) $(DEST)filesystems.doc \
	jobserver cgi-scripte \
	access-rights

cgi-scripte : \
	$(DEST)cgi-bin/search.cgi \
	$(DEST)cgi-bin/counter.cgi \
	$(DEST)cgi-bin/jobserv-free.cgi \
	$(DEST)cgi-bin/jobserv-work.cgi \
	$(DEST)cgi-bin/jobserv-done.cgi

access-rights:
	cd $(DEST) ; \
	    chmod -R ug=rwX,o=rwX . ; \
	    chmod o+w cgi-bin/counter.txt

index :
	cd $(DEST) ; glimpseindex -H . -o .

$(DEST)srcs:
	ln -s $(TOP) $(DEST)srcs

$(DEST)cgi-bin/%.cgi : %.cgi
	cp $< $@
	chmod 755 $@

$(DEST)filesystems.doc: filesystems.doc
	cp filesystems.doc $(DEST)

clean :
	rm -f $(HTML)

$(DEST)index.html : $(DEST)toc.html doc_header.html doc_footer.html
	cat doc_header.html $(DEST)toc.html doc_footer.html | \
	    sed "s/\\\\today/`date "+%d %b %Y"`/" \
	    > $@
	chmod a+r $@

$(DEST)toc.html : $(SRCS) tochtml.gawk
	@echo "Generating TOC"
	@gawk -f tochtml.gawk $(SRCS) > $@
	@echo "<UL>" >> $@
	@echo "<LI><FONT SIZE=\"+3\"><A HREF=\"adoc_index.html\">Appendix A. AutoDocs</A> (`getfiledate ../html/adoc_index.html`)</FONT>" >> $@
	@echo "</UL>" >> $@
	@echo "" >> $@
	chmod a+r $@

$(DEST)links.html: links.src $(wildcard $(TOP)/dist/*) src2html.gawk
	touch links.src
	gawk -f src2html.gawk --assign TOP="$(TOP)" $< > $@
	chmod a+r $@

$(DEST)%.html : %.src src2html.gawk page_header.html page_footer.html
	gawk -f src2html.gawk --assign TOP="$(TOP)" $< > $@
	chmod a+r $@

$(DEST)devinfo-4.html : devinfo-4.src src2html.gawk page_header.html \
	    page_footer.html collectcontents.sh contents2html.gawk \
	    makefile2html.gawk src2html.gawk
	touch devinfo-4.src
	gawk -f src2html.gawk --assign TOP="$(TOP)" $< > $@
	chmod a+r $@

$(DEST)background.html: background.src stathtml.gawk src2html.gawk
	touch background.src
	gawk -f src2html.gawk --assign TOP="$(TOP)" $< > $@
	chmod a+r $@

jobserver : $(DEST)jobserv-free.html \
	    $(DEST)jobserv-work.html \
	    $(DEST)jobserv-done.html

$(DEST)jobserv-free.html: page_header.html jobserv-free.body \
	    page_footer_nolinks.html
	cat $^ | \
	    sed "s/\\\\thischapter/Free jobs/" \
	    sed "s/\\\\today/`date "+%d %b %Y"`/" \
	    > $@

$(DEST)jobserv-work.html: page_header.html jobserv-work.body \
	    page_footer_nolinks.html
	cat $^ | \
	    sed "s/\\\\thischapter/Free in work/" \
	    sed "s/\\\\today/`date "+%d %b %Y"`/" \
	    > $@

$(DEST)jobserv-done.html: page_header.html jobserv-done.body \
	    page_footer_nolinks.html
	cat $^ | \
	    sed "s/\\\\thischapter/Finished jobs/" \
	    sed "s/\\\\today/`date "+%d %b %Y"`/" \
	    > $@

autodocs: $(DEST)adoc_index.html gen/htmlpages

gen/htmlpages: $(AUTODOC_SRCS)
	@echo "Regenerating the HTML AutoDocs"
	@makeadocs $(AUTODOC_SRCS)
	@touch gen/htmlpages

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
	@echo "Generating HTML AutoDoc Index"
	@gawk -f adochtmlindex.gawk $(AUTODOC_SRCS) > $@
	chmod -R a+r $(DEST)autodocs

$(DEST)inc_index.html: $(INCLUDE_SRCS)
	echo gawk -f inc2html.gawk $(AUTODOC_SRCS) > $@
	chmod a+r $@

