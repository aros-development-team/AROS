'''A contents-file has this format:

<contents>
<description>Some text.</description>
<item>
    <dir>...</dir>
    <file>...</file>
    Some text
</item>
... more items ...
</contents>

The <description> element is optional. There can be any number of items.
An item can contain any number of <dir> and <file> elements as long
as they come before the text.'''

import xmlsupport, sys, os, os.path, string

def processContents (p, xmlfile, item):
    xmlfile.processRecursive (p, item.content)

def processDescription (p, xmlfile, item):
    # Strip whilespace before the first element
    list = item.content[:]
    while list:
	if isinstance (list[0], xmlsupport.Text):
	    if string.strip (list[0].text) != '':
		break

	    del list[0]
	else:
	    break

    i = len (list)-1
    # Strip whilespace after the last element
    while list:
	if isinstance (list[i], xmlsupport.Text):
	    if string.strip (list[i].text) != '':
		break

	    del list[i]
	    i = i - 1
	else:
	    break

    p.contents.description = list

class Item:
    def __init__ (self):
	self.files = []
	self.dirs = []
	self.description = []

    def dump (self, level):
	if self.files:
	    sys.stdout.write ('    '*level)
	    sys.stdout.write (self.files[0])
	    for name in self.files[1:]:
		sys.stdout.write (', ' + name)
	    print
	
	print '    '*level, self.description
	if self.dirs:
	    for dir in self.dirs:
		if type (dir) == type (''):
		    sys.stdout.write ('    '*level)
		    sys.stdout.write (dir+'/\n')
		else:
		    sys.stdout.write ('    '*level)
		    sys.stdout.write (dir[0]+'/\n')
		    dir[1].dump (level+1)


def processItem (p, xmlfile, item):
    # Strip whilespace before the first text element
    list = item.content[:]
    #print '1',list
    i = 0
    while i < len (list):
	if isinstance (list[i], xmlsupport.Text):
	    if string.strip (list[i].text) != '':
		break

	    del list[i]
	else:
	    if isinstance (list[i], xmlsupport.Tag):
		if not list[i].name in ('dir', 'file'):
		    break
	    i = i + 1

    #print '1b',list
    result = Item ()
    dirs = []

    while list:
	if isinstance (list[0], xmlsupport.Tag):
	    if list[0].name == 'file':
		result.files.append (list[0].content[0].text)
	    elif list[0].name == 'dir':
		dirs.append (list[0].content[0].text)
	    else:
		break
	else:
	    break
	
	del list[0]
	
    #print '2',list
    result.description = list
    p.contents.items.append (result)

    for dir in dirs:
	# Read contents files recursively
	filename = os.path.join (p.contents.basedir, dir, 'contents.xml')
	if os.path.exists (filename):
	    result.dirs.append ((dir, Contents (filename)))
	else:
	    print '%s missing' % filename
	    result.dirs.append (dir)

class Contents:
    def __init__ (self, filename):
	sys.stderr.write ('Processing %s...\n' % filename)
	self.filename = filename
	xmlfile = xmlsupport.XmlFile ()
	xmlfile.parse (self.filename)

	self.basedir = os.path.dirname (self.filename)

	p = xmlsupport.Processor (
	    contents=processContents,
	    description=processDescription,
	    item=processItem,
	)
	p.contents = self

	self.description = []
	self.items = []

	xmlfile.process (p)

    def dump (self, level=0):
	if self.description:
	    print `self.description`
	
	for item in self.items:
	    item.dump (level)

if __name__ == '__main__':
    c = Contents (sys.argv[1])
    c.dump ()
