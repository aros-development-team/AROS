
Creating the HTML version of the documentation
==============================================

To create the HTML docs, you must call mmake .docs or mmake .all-docs.
The latter will query the mSQL database at the AROS site and update 
the status pages (if it doesn't, then that's what it should do). 

This is a multi-step process. First the makefile generates a list of all
source files. These are then scanned for function and library names (to
get a list of functions sorted by name and library) and converted into
an index. Then CVS is called for every source file (to get the HISTORY)
and the html page for each function is generated from the source and the
CVS log. The last step is to create one big HTML file with lots of
tables which contain a nicely formatted version of the function index.

After that, the .src files are converted into HTML, the resulting files
are copied into the HTML directory and the permissions are fixed.


What is mmake?
==============

"mmake" ist the preprocessor, ".docs" is the meta-make rule.
Just run "AROS/setup", "AROS/configure" and "mmake .docs"
But I suppose you will have some trouble, since the makefile tries to
get the history of sources via cvs (so you have to be on the net, or
disable this in the mmakefile)

mmake is a separate tool that's part of AROS. Just do

./configure
make

in the main dir and an executable mmake should be generated there.
Alternatively, change to tools/MetaMake and see there for further
instructions.



What else do I need?
====================

The files in:
-AROS/
And everything in: 
-AROS/config/
-AROS/docs/
-AROS/rom/
-AROS/tools/
-AROS/workbench/

AROS/ contains the setup files to generate some tools you may need. 
AROS/tools/ contains the tools you need to 'build' the docs.
AROS/workbench/, AROS/rom/ and AROS/config/ contain the sources 
with the docs for the AROS functions. 
AROS/docs/ contains some other docs (how to use CVS, for example)
and the scripts to build the HTML docs from the sources. You can 
update the parts you need with

    cvs update -dP config docs rom workbench tools


(This README file is largely based on e-mail from Aaron Digulla, 
Henning Kiel and Sebastian Rittau in response to questions by me, 
Branko Collin.)