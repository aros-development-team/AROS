# file: attachMCs.py

# * P_LZ_COPYRIGHT_BEGIN ******************************************************
# * Copyright 2001-2004 Laszlo Systems, Inc.  All Rights Reserved.            *
# * Use is subject to license terms.                                          *
# * P_LZ_COPYRIGHT_END ********************************************************

import sys

# Mapping of serialized values to 'native' slots
# playing and currentframe handled specially
attrdict = { \
    "xscale" : "_xscale", \
    "yscale" : "_yscale", \
    "y" : "_y", \
    "x" : "_x", \
    "opacity" : "_alpha", \
    "visible" : "_visible", \
    "rotation" : "_rotation", \
    };

def getAttribute(aname, xpp):
    return xpp.getAttributeValue(None, aname)

import java.io.IOException as IOException
import java.io.StringReader as StringReader

import org.xmlpull.v1.XmlPullParser as XmlPullParser
import org.xmlpull.v1.XmlPullParserException as XmlPullParserException
import org.xmlpull.v1.XmlPullParserFactory as XmlPullParserFactory

import java.io.FileInputStream as FileInputStream
import java.io.InputStreamReader as InputStreamReader
import java.io.BufferedReader as BufferedReader
import org.xml.sax.InputSource as InputSource

class Compiler:

    def __init__(self, out=None):
        self.out = out

    def compile(self, fname):
        
        # initialize a dictionary to hold requested fixed-size text resource names
        self.fxtextnames = {}
        # this holds a list of actual text resource names which exist in the swf file
        self.realtextnames = {}

        fis = FileInputStream(fname)
        r = BufferedReader(InputStreamReader(fis, "UTF-8"))
        factory = XmlPullParserFactory.newInstance()
        factory.setNamespaceAware(0)
        xpp = factory.newPullParser()
        xpp.setInput(r)
        eventType = xpp.getEventType()
        # advance to first tag
        while (eventType != xpp.END_DOCUMENT):
            if (eventType == xpp.START_TAG):
                assert xpp.getName() == "_top", "This does not appear to be an lzx object heirarchy"
                break
            eventType = xpp.next()

        print >> self.out,  "push '_root'"
        print >> self.out,  "getVariable"
        print >> self.out,  "setRegister r:0"
        print >> self.out,  "pop"
        self.writeNodes(xpp)
	fis.close()

    def writeNodes(self, xpp):
        eventType = xpp.next()
        while (eventType != xpp.END_DOCUMENT):
            if (eventType == xpp.START_TAG):
                name = xpp.getName()
                if (name != "m"):
                    return
                #assumes r:0 is currently set to parent node
                # save r:0
                print >> self.out,  "push r:0"

                # collect fixed text name resource, if any
                fx = getAttribute("fxlink", xpp)
                link = getAttribute( "link" , xpp )

                self.realtextnames[link] = link
                if fx != None:
                    self.fxtextnames[fx] = fx
                    # Fixed-size inputtext optimization (swf5):
                    # This overrides the normal "link" resource name, it represents
                    # a text resource name containing the actual
                    # runtime dimensions of the inputtext view. Just be sure to
                    # add in these desired flash text resources when generating the lzo file!
                    resourceName = fx
                else:
                    resourceName = link

                l = getAttribute ( "depth" , xpp )
                if  l == "contained":
                    #this is in the definition of the parent
                    print >> self.out,  "push r:0,'" + getAttribute( "n", xpp ) + "'"
                    print >> self.out,  "getMember"
                else:
                    print >> self.out,  "push r:0, '" + getAttribute( "n" , xpp ) + "', ",
                    print >> self.out,  getAttribute ( 'depth' , xpp ) + ", ",
                    print >> self.out,  "'"+ getAttribute( "n" , xpp ) +"' " ,
                    print >> self.out,  ", '" + resourceName +"' , ",
                    print >> self.out,  "3 , r:0 , 'attachMovie'"
                    print >> self.out,  "callMethod"
                    print >> self.out,  "pop"
                    print >> self.out,  "getMember"
                #set register to current node
                print >> self.out,  "setRegister r:0"
                print >> self.out,  "pop"

                # If playing was set need to go (or stop) there, NOTE:
                # if we don't know the state of playing we can't set
                # the frame; most likely this occurs in a resource
                # that loops and was just at some arbitrary frame when
                # serialization happens
                if getAttribute('playing', xpp):
                    frame = getAttribute('currentframe', xpp) or "1"
                    if getAttribute('playing', xpp) == "true":
                        print >> self.out, "push " + frame + ", 1, r:0, 'gotoAndPlay'"
                    else:
                        print >> self.out, "push " + frame + ", 1, r:0, 'gotoAndStop'"
                    print >> self.out, "callMethod"
                    print >> self.out, "pop"

                for idx in range(xpp.getAttributeCount()):
                    pname = xpp.getAttributeName(idx)
                    pvalue = xpp.getAttributeValue(idx)
                    if not attrdict.has_key(pname):
                        continue
                    value = pvalue
                    if value == 'Infinity':
                        value = `value`
                    print >> self.out,  "push r:0, '" + attrdict[ pname ] + "', " + value
                    if value == 'Infinity':
                        print >> self.out, 'getVariable'
                    print >> self.out,  "setMember"

            # end tag, pop the stack
            elif (eventType == xpp.END_TAG):                
                name = xpp.getName()
                if not (name == "m"):
                    return
                # restore r:0 to the saved parent
                print >> self.out,  "setRegister r:0"
                print >> self.out,  "pop"
            eventType = xpp.next()

# returns a pair of (<instruction list>, <hashtable of fixed-text resource names>)
def mc2flasm(fname):
    from cStringIO import StringIO
    str = StringIO()
    compiler = Compiler(str);
    compiler.compile(fname)
    from parseinstructions import parseInstructions
    return (parseInstructions(str.getvalue()), compiler.fxtextnames)

if __name__ == '__main__' :
    Compiler().compile(sys.argv[1])
