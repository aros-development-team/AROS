
DEST=../html/
FILES = background devinfo devinf_inside devinf_outside \
	history links

SRCS = $(foreach f,$(FILES),$(f).src)
HTML = $(DEST)index.html $(foreach f,$(FILES),$(DEST)$(f).html)

all : $(HTML) $(DEST)filesystems.doc

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
