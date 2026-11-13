$Id$

HTML Datatype
=============

This concept splits the task of displaying HTML into three parts, the
parser, the layouter and the renderer. In between there are well
defined small interfaces. This way, other parsers can use the same
layout and rendering engine, too, for example:

amigaguide.datatype - for a nostalgic experience
rtf.datatype - M$ rich text format
man.datatype - UN*X man pages
and there are a lot of other hypertext and enriched text formats, too.

Initially it was intended to put only the parser into a subdatatype
and have the layouter and the renderer in a datatypes superclass. As
an interim solution text.datatype is used as superclass and both
parser and layouter are found in html.datatype. Clicking on links
doesn't work yet.

The design goals were speed, low memory and readable code. I'm not
sure if the latter is met. :-) For the parser I tried to move part
of the code into data structures to keep the code simple and clean.

There are several open source HTML engines available. However, I
decided to write this from scratch instead of porting, because
porting means to deal with dependencies like POSIX, gtk+ and the
browser environment. This way I could focus on the core issues
which promises more fun and it brings up something unique.

Parser
======

The parser is called only once during loading the HTML file.

I have kept an eye on the fact, that many Web pages are HTML crap, but
usual Web browsers display them nevertheless. So the parser tries to
form something that pleases the eye even from a faulty HTML page. It
supports XHTML, too, actually the syntax checker and the selection of
supported tags/attributes are XHTML biased.

Supported tags:
---------------
<html> - working
<head> - working
<title> - working
<body> - working
<!-- comment -->
<br> - working
<div> - noop
<h1> - working
<h2> - working
<p> - working
<pre> - working
<span> - noop
<b> - working
<big> - working
<hr> - working
<i> - working
<small> - working
<tt> - working
<a> - partly working, no link info
<img> - partly working, no image info

Supported attributes:
---------------------
(none yet :-)

Supported character entities (escaped characters, for examle &amp; ):
----------------------------
all known, which includes latin-1 character set
numeric chars are possible in decimal ( &#64; ) or hex ( &#x40; )

If above listed tags/attributes are listed "working", it doesn't mean
that the layout engine or the renderer supports them, it just means
that the associated info is passed to the layout engine.

Known bugs:
* style stack is limited in size
* <pre> sections have an initial linefeed
* in <pre> sections tabs are replaced by 8 spcaes


Layouter
========

The layouter is called for initial layout and the every time the
window is resized.

The interface to the parser is a list of text segments interleaved
with style and formatting information. A text segment is a word or
are multiple words separated by space that have the same style and
belong to the same paragraph.

Interface description:
----------------------
The text segment list (= seglist) are seg_struct structures linearly
in memory. It is a union structure to assure the same size for every
seglist element. Listed below are the possible seglist elements
together with their relevant structure elements.

SEG_CMD_Text: text segment

struct seg_struct
{
	u_char		cmd;
	u_short		textlen;
	string		textseg;
};

SEG_CMD_Blockstart: start of paragraph: <p>, <h1> etc.
SEG_CMD_Blockend: end of paragraph: </p>, </h1> etc.
SEG_CMD_Linebreak: insert linefeed: <br>
SEG_CMD_Ruler: insert horizontal separator: <hr>

struct seg_struct
{
	u_char		cmd;
};

SEG_CMD_Image: insert image: <img ...>

struct seg_struct
{
	u_char		cmd;
	image_struct	*image;
};

SEG_CMD_Parastyle: paragraph layout

struct seg_struct
{
	para_flags	paramask;
	para_flags	paraflags;
};

SEG_CMD_Softstyle,

struct seg_struct
{
	style_flags	stylemask;
	style_flags	styleflags;
};


SEG_CMD_Next: branch to new text segment list
SEG_CMD_Sublist: use sub list for macros

struct seg_struct
{
	u_char		cmd;
	seg_struct	*next or *sublist;
};

Known bugs:
-----------
* word wrapping can occur at style changes, too, even if they
  appear inside a word


Renderer
========

The renderer is called every time after the layouter has been called
and every time the window contents have to be scrolled or the window
has to be refreshed otherwise.

For now, text.datatype is used as renderer. The interface to the
layouter is the line list as defined in <datatypes/textclass.h> .

Limitations
-----------
* only one font type and size for the whole page
* all lines have to have the same height with no
  spaces between the lines


      Martin Gierich <gierich AT gmx.net>
