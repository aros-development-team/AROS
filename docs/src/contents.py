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

import xmlsupport, sys, os, os.path, string, fileinput

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

def filterMakefile (p, item, tag):
    filename = os.path.join (p.contents.basedir, item.files[0])

    words = string.split (tag.content[0].text, ',')
    parts = []
    for word in words:
	parts.append (string.strip (word))

    list = []

    mode = 'search'
    for line in fileinput.input (filename):
	if mode == 'search':
	    if line[:12] == '# BEGIN_DESC':
		pos = string.index (line, '{')
		pos2 = string.index (line, '}', pos)
		part = line[pos+1:pos2]
		#print part
		if part in parts:
		    mode = 'found'
		    text = ''
	elif mode == 'found':
	    if line[:10] == '# END_DESC':
		list.append ((part, text))
		mode = 'search'
	    else:
		text = text + string.rstrip (line[2:]) + '\n'

    result = xmlsupport.Tag ('p')

    #print 'list:',list
    dl = None

    for part, text in list:
	paras = string.split (string.rstrip (text), '\n\n')

	for para in paras:
	    print `para`
	    if para[:5] == '\\item':
		pos = string.index (para, '{')
		pos2 = string.index (para, '}', pos)
		name = para[pos+1:pos2]
		para = string.lstrip (para[pos2+1:])
		print 'item:',name,`para`
	
		liElement = xmlsupport.Tag ('li')
		itemElement = xmlsupport.Tag ('item')
		textElement = xmlsupport.Text (para)
		itemElement.append (xmlsupport.Text (name))
		liElement.append (itemElement, textElement)
		
		if dl:
		    dl.append (liElement)
		else:
		    dl = xmlsupport.Tag ('description')
		    result.append (dl)
	    else:
		dl = None
		pElement = xmlsupport.Tag ('p')
		textElement = xmlsupport.Text (para)
		pElement.append (textElement)
		
		result.append (pElement)

    result.dump (0)
    return result
    
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
    i = 0
    for child in list:
	if isinstance (child, xmlsupport.Tag) and child.name == 'filtermakefile':
	    del list[i]
	    list.insert (i, filterMakefile (p, result, child))
	i = i + 1
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
