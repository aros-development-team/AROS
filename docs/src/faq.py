'''Convert FAQ files into a python object or XML.'''

import os.path, xmlsupport

class Item:
    def __init__ (self, filename):
	self.filename = filename
	fh = open (self.filename, 'r')
	self.text = fh.read ()
	fh.close ()

	self.mtime = os.path.getmtime (self.filename)

class FAQ:
    def __init__ (self, *dirs):
	self.items = []

	apply (self.processDirs, dirs)

    def processDirs (self, *dirs):
	for dir in dirs:
	    os.path.walk (dir, self.processDir, None)

    def processDir (self, dummy, dirname, items):
	print 'dirname:',dirname
	try:
	    # Skip CVS dirs
	    pos = items.index ('CVS')
	    del items[pos]
	except:
	    pass

	items.sort ()
	for item in items:
	    print item
	    path = os.path.join (dirname, item)
	    if os.path.isfile (path):
		self.items.append (Item (path))

    def toXml (self):
	result = xmlsupport.Tag ('chapter title="FAQ - Frequently Asked Questions"')

	for item in self.items:
	    xf = xmlsupport.AROSXmlFile ()
	    xf.parseString (item.text)

	    for child in xf.tree:
		if isinstance (child, xmlsupport.Tag) and child.name == 'section':
		    child.attr['mtime'] = `item.mtime`
		    result.append (child)

	return result

if __name__ == '__main__':
    import sys
    faq = apply (FAQ, sys.argv[1:])
    print faq.items
