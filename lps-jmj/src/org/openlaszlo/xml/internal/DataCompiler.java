/*

TODO

+ wrap resultset as sepearate compiled XML, splice together
<resultset>
 <body/>
 <headers/>
</resultset>
   
+ rename "root"

+ add whitespace preserve option flag

        // Write the header
        xmlResponse.append("<resultset>");
        
            
        // Get body
        xmlResponse.append("<body>\n");
        String body = null;

        </body>
                    xmlResponse.append("<headers>\n");
            data.appendResponseHeadersAsXML(xmlResponse);
            xmlResponse.append("</headers>");
        }

        // End the resultset tag
        xmlResponse.append("</resultset>");
*/



/* ****************************************************************************
 * DataCompiler.java
 *
 * Compile XML directly to SWF bytecodes.
 * ****************************************************************************/

/* J_LZ_COPYRIGHT_BEGIN *******************************************************
* Copyright 2001-2004 Laszlo Systems, Inc.  All Rights Reserved.              *
* Use is subject to license terms.                                            *
* J_LZ_COPYRIGHT_END *********************************************************/

package org.openlaszlo.xml.internal;

import java.io.*;
import java.util.*;

import org.jdom.Attribute;
import org.jdom.Comment;
import org.jdom.Document;
import org.jdom.Element;
import org.jdom.JDOMException;
import org.jdom.input.SAXBuilder;
import org.jdom.output.*;


import org.openlaszlo.iv.flash.util.*;
import org.openlaszlo.iv.flash.api.action.*;
import org.openlaszlo.iv.flash.api.*;
import org.openlaszlo.compiler.CompilationError;
import org.openlaszlo.utils.ChainedException;
import org.openlaszlo.utils.FileUtils;
import org.openlaszlo.utils.HashIntTable;

import org.xmlpull.v1.XmlPullParser;
import org.xmlpull.v1.XmlPullParserException;
import org.xmlpull.v1.XmlPullParserFactory;

import org.apache.log4j.*;


/**
 * Takes XML in various forms, and serializes it to isomorphic actionscript
 * bytecode.
 *
 * @author Max Carlson, Henry Minsky
 * @version 1.1
 */
public class DataCompiler extends DataCommon {
    /* Logger */
    private static Logger mLogger  = Logger.getLogger(DataCompiler.class);
    private static XmlPullParserFactory factory = null;

    /** global var to hold the image of the stack when splitting frames */
    static int MISC_BUFSIZ = 4096;
    static String STRINGBUF_VAR = "__dcstrbuf__";


    private static XmlPullParserFactory getXPPFactory () {
        if (factory == null) {
            // Set up the XML Parser factory
            try {
                String sys = null; 
                try {
                    sys = System.getProperty(XmlPullParserFactory.PROPERTY_NAME);
                } catch (SecurityException se) {
                }
                factory = XmlPullParserFactory.newInstance(sys, null);
                factory.setNamespaceAware(true);
            } catch (XmlPullParserException e) {
                throw new RuntimeException(e.getMessage());
            } 
        }
        return factory;
    }

    public DataCompiler () { }


    // compress = false, for back compatibility
    public static InputStream compile(String x, String headers,
                                      int flashVersion,
                                      boolean addwrapper,
                                      boolean trimWhitespace
                                      ) throws IOException, DataCompilerException {
        return compile(x, headers, flashVersion, addwrapper, trimWhitespace, false);
    }


    /**
     * Compile XML to SWF
     *
     * @param String x XML string to compile
     * @return input stream
     *
     * The size of the input XML will always be greater than the
     * output .swf (possibly plus some factor, to be measured), so we can allocate a FlashBuffer
     * with that max size and be done.
     */

    // Change input from String to Reader, for higher efficiency
    public static InputStream compile(String x, String headers,
                                      int flashVersion,
                                      boolean addwrapper,
                                      boolean trimWhitespace,
                                      boolean compress
                                      ) throws IOException, DataCompilerException {
        try {
            // XML Parser for the data body
            XmlPullParser xppbody = getXPPFactory().newPullParser();
            xppbody.setInput( new StringReader(x) );

            // XML Parser for the headers
            XmlPullParser xppheaders = getXPPFactory().newPullParser();
            if (addwrapper) {
                xppheaders.setInput( new StringReader(headers) );
            }
            mLogger.debug("compile x="+x+"\nheaders="+headers);
            return getSWF(xppbody, xppheaders, x.length(), flashVersion, addwrapper, trimWhitespace, compress);
        } catch (XmlPullParserException ex) {
            throw new DataCompilerException("Parsing XML: " + ex.getMessage());
        }
    }

    public static InputStream compile(String x, int flashVersion) throws IOException, DataCompilerException {
        try {
            // XML Parser for the data body
            XmlPullParser xppbody = getXPPFactory().newPullParser();
            xppbody.setInput( new StringReader(x) );

            return getSWF(xppbody, null, x.length(), flashVersion, false);
        } catch (XmlPullParserException ex) {
            throw new DataCompilerException("Parsing XML: " + ex.getMessage());
        }
    }

    /** Create a new string pool buffer */
    private static DataContext makeDataContext(int flashVersion) {
        DataContext dc = new DataContext(flashVersion);
        if (flashVersion == 5) {
            dc.setEncoding("Cp1252");
        } else {
            dc.setEncoding("UTF-8");
        }
        return dc;
    }

    /** Split string into PUSH chunks of less than 64k, and emit string concat
        instructions to re-assemble them. Leaves string value on stack.
        
    */
    static void splitPushString (String text, Program prog) {
        // break up into 64K chunks, append to local variable.
        // To account for multibyte chars, use 10000 to be safe.
        int index = 0;
        int len = 0;
        int nchunks = 0;
        while (index < text.length()) {
            if ((text.length() - index) > 10000) {
                len = 10000;
            } else {
                len = text.length() - index;
            }
            String chunk = text.substring(index, index+len);
            index += len;

            prog.push(chunk);
            nchunks++;
        }

        // concatenate all the strings
        while (nchunks > 1) {
            prog.addString();
            nchunks--;
        }
    }



    static int MAXFRAME_SIZE = 20000;

    // Initial buffer size for program, with space for string constant pool (max 64k)
    static int FB_INIT_SIZE = MAXFRAME_SIZE * 2 + 128000;

    /* These two hardcoded constants are always added first to string pool of a frame, in this order. */
    private static byte constructor_idx = 0;
    private static byte textnode_idx    = 1;

    /**
     * Main loop which pulls XPP events and writes swf bytecodes to build the node structure.
     *
     * @param FlashBuffer body flashbuffer to write to
     * @param xpp XML parser pointing to data
     * @param programs a list of Flash Programs. Add to it as we create new frames. 
     * enter assuming that root node is on the stack
     *
     */


    /*

    >>> c("stackarray=['top','middle', 'root']",printInstructions=1)
      constants 'stackarray' 'middle' 'root' 'top'
      push 'stackarray' 'root' 'middle' 'top' '3'
      initArray
      setVariable
    >>>


    >>> c('while(a.length > 0) a.pop()' , printInstructions=1)
      constants 'a' 'length' 'pop'
    L1:
      push '0.0' 'a'
      getVariable
      push 'length'
      getMember
      lessThan
      not
      branchIfTrue 'L0'
      push '0' 'a'
      getVariable
      push 'pop'
      callMethod
      pop
      branch 'L1'
    L0:
    >>>

    */

    private static final DataContext writeXMLData(XmlPullParser xpp, int flashVersion,
                                                  boolean splitframes, Vector programs,
                                                  DataContext dc, 
                                                  boolean trimWhitespace, int stackdepth)
          throws IOException, XmlPullParserException {

        // Use the program we were passed in, it has the root node on top of stack.
        Program bodyp = (Program) programs.lastElement();
        FlashBuffer body = bodyp.body();

        int eventType = xpp.getEventType();
        while (eventType != xpp.END_DOCUMENT) {
            if(eventType == xpp.START_DOCUMENT) {
            } else if(eventType == xpp.START_TAG) {
                // makeNodeNoText = function (attrs, name, parent)
                // dup pointer to PARENT (who is at top of stack)
                // DUP
                body._writeByte(Actions.PushDuplicate);


                // Fold all the attribute key/value pairs into a single PUSH
                // Build ATTRIBUTE object
                int nattrs = xpp.getAttributeCount();
                String eltname = xpp.getName();

                // Check if combined attr+data is > 64k, if so, use individual
                // pushes with string breaking/merging, rather than compressed merging.
                int datasize = 0;
                for (int i = 0; i < nattrs; i++) {
                        String attrname = xpp.getAttributeName(i);
                        datasize += getByteLength(attrname, dc.encoding) + 2;
                        String attrval = xpp.getAttributeValue(i);
                        datasize += getByteLength(attrval, dc.encoding) + 2;
                }

                // If if data is too large, use individual split pushses
                if (datasize > 65500) {
                    bodyp.push(eltname);

                    for (int i = 0; i < nattrs; i++) {
                        String attrname = xpp.getAttributeName(i);
                        //System.out.print("Attr " + attrname);
                        bodyp.push(attrname);

                        String attrval = xpp.getAttributeValue(i);
                        splitPushString(attrval, bodyp);
                    }

                    bodyp.push(nattrs);
                    body._writeByte(Actions.InitObject);
                } else {

                // We're really squeezing things down, so we are going to merge 
                // the PUSH of the element name with the PUSH of the attribute key/value
                // data and the attribute count. So the stack will look like
                // [eltname attrname1 attrval1 attrname2 attrval2 ... ... nattrs]
                // when we're done
                body._writeByte(Actions.PushData);
                // Leave a two byte space for the PUSH length
                // Mark where we are, so we can rewrite this with correct length later.
                int push_bufferpos = body.getPos();
                body._writeWord(0); // placeholder 16-bit length field 

                // Push element NAME
                _pushMergedStringDataSymbol(eltname, body, dc);

                // PUSH := {0x96, lenlo, lenhi, 0x00, char, char, char, ...0x00, }
                for (int i = 0; i < nattrs; i++) {
                    String attrname = xpp.getAttributeName(i);
                    //System.out.print("Attr " + attrname);
                    _pushMergedStringDataSymbol(attrname, body, dc);

                    String attrval = xpp.getAttributeValue(i);
                    //System.out.println("= " + attrval);
                    _pushMergedStringData(attrval, body, dc);
                }
                // create the attrs object; push the attr count
                body._writeByte(0x07); // INT type
                body._writeDWord(nattrs);

                // Now go back and fix up the size arg to the PUSH instruction
                int total_size = body.getPos() - (push_bufferpos + 2);
                //System.out.println("pos="+body.getPos()+ " total_size = "+total_size+"  push_bufferpos="+push_bufferpos+" nattrs="+nattrs);
                body.writeWordAt(total_size, push_bufferpos);

                body._writeByte(Actions.InitObject);
                }

                // stack now has [parent, name, attrs]
                // Push # of args and node-instantiator-function name
                // PUSH 3, _mdn
                // [PUSHDATA, 9, 0,  0x07, 03 00 00 00 0x08, "_m"]
                body._writeByte(Actions.PushData);
                body._writeWord(7);
                body._writeByte(0x07); // INT type
                body._writeDWord(3);   // '3' integer constant , number of args to node constructor fn
                body._writeByte(0x08);     //  SHORT DICTIONARY LOOKUP type
                body._writeByte(constructor_idx);  // index of "_m" string constant
                body._writeByte(Actions.CallFunction);


                // We push new node on the stack, so we can reference it as the parent 
                // Stack => [parentnode newnode]
                // increment the node stack depth
                stackdepth++;
            } else if (eventType == xpp.END_TAG) {
                // Pop the node off the stack. 
                body._writeByte(Actions.Pop);
                stackdepth--;

                // Good place to check if this frame has gotten large enough to split.

                // IMPORTANT: recalculate buffer size to what it
                // really is (since we've been using the fast
                // _writeXXX functions which dont' update size
                // automatically);
                body.setSize(body.getPos());

                // When we end a frame, we call initArray (stackdepth) to unload to "__dcns".
                // Then when we start the next frame, we loop to transfer array contents back to stack
                if (splitframes && body.getSize() > MAXFRAME_SIZE) {
                    //mLogger.debug("splitting frame "+body.getSize());

                    // dump the stack to a global variable
                    // XXXXX NEED TO COMPENSATE FOR HEADER WRAPPERS!!!!!!!
                    bodyp.push(stackdepth);
                    body.writeByte(Actions.InitArray);
                    bodyp.push("__dcns");
                    body.writeByte(Actions.StackSwap);
                    bodyp.setVar();


                    // prepend the string pool to the program,and add it to the frame list
                    Program withpool = appendStringPool(dc, body);
                    // replace the last program with one that has a string pool.
                    programs.setElementAt(withpool, programs.size() -1);

                    body = new FlashBuffer(FB_INIT_SIZE);
                    bodyp =  new Program(body);
                    programs.add(bodyp);

                    dc = makeDataContext(flashVersion);
                    // Intern the node constructor function names in
                    // the string pool as first entries. Always use
                    // this ordering.
                    addStringConstant(NODE_INSTANTIATOR_FN, dc);
                    addStringConstant(TEXT_INSTANTIATOR_FN, dc);

                    // The code below is basically this:
                    // while(__dcns.length > 0) { __dcns.pop() }
                    // But each pop() leaves the popped data on stack.
                    //L1:
                    //push '0.0' 'a'
                    // == 34
                    bodyp.push(0); // 8
                    bodyp.push("__dcns"); // 4 + 6 +1= 11
                    //getVariable
                    bodyp.getVar(); // 1
                    //push 'length'
                    bodyp.push("length"); // 4 + "length" +1 = 11
                    //getMember
                    bodyp.getMember(); // 1
                    //lessThan
                    bodyp.lessThan(); //1 
                    //not
                    bodyp.logicalNot(); // 1 
                    // total = (+ 8 11 1 11 1 1 1 ) = 34
                    //branchIfTrue 'L0'

                    // == 5
                    bodyp.jumpIfTrue(34); // 5 bytes   //jump L0:

                    // == 34
                    //push '0' 'a'
                    bodyp.push(0); // 4 + data length = 8
                    bodyp.push("__dcns"); // 4 + "__dcns" + 1 = 11
                    //getVariable
                    bodyp.getVar(); // 1
                    //push 'pop'
                    bodyp.push("pop"); // 4 + 1 + 'pop' = 8
                    //callMethod
                    bodyp.callMethod(); // 1
                    //branch 'L1'
                    bodyp.jump(- (34 + 34 + 5)); //5 bytes  // jump L1:
                    //L0:
                }

            } else if(eventType == xpp.TEXT) {
                if (trimWhitespace && xpp.isWhitespace()) {
                    // If we're supposed to trim whitespace, and text is all whitespace,
                    // then don't create a node.
                } else {
                    String text = xpp.getText();
                    if (trimWhitespace) {
                        text = text.trim();
                    }

                    // dup pointer to parent (who is at top of stack)
                    // DUP
                    body._writeByte(Actions.PushDuplicate);

                    // Push text

                    if (getByteLength(text, dc.encoding) > 64000) {
                        splitPushString (text, bodyp);

                        bodyp.push(2);
                        bodyp.push(TEXT_INSTANTIATOR_FN);
                        bodyp.callFunction();
                        // Pop the node, because there will be no end tag for it, and it has no children.
                        body._writeByte(Actions.Pop); 

                    } else {
                        body._writeByte(Actions.PushData);
                        // Leave a two byte space for the PUSH length
                        // Mark where we are, so we can rewrite this with correct length later.
                        int push_bufferpos = body.getPos();
                        body._writeWord(0); // placeholder 16-bit length field 

                        _pushMergedStringData(text, body, dc);
                        // Set up argsnum and function name
                        // PUSH 2, _tdn
                        body._writeByte(0x07); // INT type
                        body._writeDWord(2);   // '2' integer constant ; number of args to function
                        body._writeByte(0x08);     //  SHORT DICTIONARY LOOKUP
                        body._writeByte(textnode_idx);  // push function name: index of "_t" string constant


                        // Now go back and fix up the size arg to the PUSH instruction
                        int total_size = body.getPos() - (push_bufferpos + 2);
                        body.writeWordAt(total_size, push_bufferpos);
                        body._writeByte(Actions.CallFunction);
                        // Pop the node, because there will be no end tag for it, and it has no children.
                        body._writeByte(Actions.Pop); 
                    }

                }
            }
            eventType = xpp.next();
            // invariant is that when we start a frame, stack contains parent node list
        }

        // return the datacontext, the caller will use it to write the last string pool
        return dc;
    }

    /** Takes the string hash table in DataContext dc, and prepends it to the
        program in FlashBuffer body. Returns a new program with the stringpool
        at the start.
    **/
    private static Program appendStringPool (DataContext dc, FlashBuffer body) {
        // Collect the string dictionary data
        byte pooldata[] = makeStringPool(dc);

        // sync up size with buffer
        body.setSize(body.getPos());

        // 'out' is the main FlashBuffer for composing the output file
        FlashBuffer out = new FlashBuffer(body.getSize() + pooldata.length + MISC_BUFSIZ);
        // Write out string constant pool
        out._writeByte( Actions.ConstantPool );
        out._writeWord( pooldata.length + 2 ); // number of bytes in pool data + int (# strings)
        out._writeWord( dc.cpool.size() ); // number of strings in pool
        out.writeArray( pooldata, 0, pooldata.length); // copy the data

        // Write out the code to build nodes
        out.writeArray(body.getBuf(), 0, body.getSize());
        // This is a program that contains the constant pool plus code
        Program prog = new Program(out);
        return prog;
    }

    /** default trimWhitespace=true */
    public static Program makeProgram(Element data, int flashVersion) throws CompilationError {
        return makeProgram(data, flashVersion, true, false);
    }

    /**
       Called for compile-time data, don't do frame splitting, and don't add any resultset wrapper
    */
    public static Program makeProgram(Element data, int flashVersion, boolean trimWhitespace, boolean localdata) throws CompilationError {
        XMLOutputter outputter = new XMLOutputter();
        StringWriter sout = new StringWriter();
        try {
            outputter.output(data, sout);
        }
        catch (IOException ex) {
            throw new RuntimeException("DataCompiler makeProgram parsing XML: " + ex.getMessage());
        }
        String x = sout.toString();
        // [TODO 02-10-2003 hqm -- Do we need to catch character conversion errors -> ASCII like we did with JDOM?]
        try {
            XmlPullParser xpp = getXPPFactory().newPullParser();
            xpp.setInput( new StringReader(x) );
            Vector progs = makeProgram(xpp, null, x.length(), flashVersion, false, false, trimWhitespace, localdata);
            return ((Program) progs.firstElement());
        } catch (XmlPullParserException ex) {
            throw new CompilationError("DataCompiler makeProgram parsing XML: " + ex.getMessage());
        } catch (IOException ex) {
            throw new RuntimeException("DataCompiler makeProgram parsing XML: " + ex.getMessage());
        }
    }



    /**
     * Produces a JGenerator Flash Program containing
     * executable SWF codes to build an XML datasource structure which
     * represents this XML stream.
     *
     * Splits execution across frames when program buffer becomes too large.
     *
     *
     * @param xpp XML XPP parser which is reading from the data content string
     * @param splitframes if true, split program across multiple frames. In that case, a final stop will be inserted at the last frame. If false, a single frame program will be returned, with no stop or end of frame.
     * @return Vector of one or more (if frame splitting) Flash Programs */
    public static Vector makeProgram(XmlPullParser xpp, XmlPullParser xpheaders,
                                     int xmlsize, int flashVersion,
                                     boolean splitframes,
                                     boolean addwrapper,
                                     boolean trimWhitespace,
                                     boolean localdata)
      throws IOException, XmlPullParserException {
        Vector programs = new Vector();

        // Allocate at least enough room to hold the data nodes and strings, ok to allocate too much;
        // If we are not splitting frames, allocate enough to hold the whole XML file.
        FlashBuffer body = new FlashBuffer(splitframes ? FB_INIT_SIZE : (xmlsize * 3) + 128000);
        Program bodyp =  new Program(body);
        programs.add(bodyp);

        // Bind the node creation functions to some short local names:
        //  element nodes: _level0._m => _m
        bodyp.push(new Object[]{"_m", "_level0"});
        bodyp.getVar();
        bodyp.push("_m");
        bodyp.getMember();
        bodyp.setVar();

        //  text nodes: _level0._t => _t
        bodyp.push(new Object[]{"_t", "_level0"});
        bodyp.getVar();
        bodyp.push("_t");
        bodyp.getMember();
        bodyp.setVar();

        // Build a root node by calling the runtime's root node instantiator
        // The root node will have $n = 0, and whatever other initial conditions are needed.
        bodyp.push(0); // Root node creator function takes no args.
        bodyp.push("_level0");
        bodyp.getVar();
        bodyp.push(ROOT_NODE_INSTANTIATOR_FN);
        bodyp.callMethod();
        // The root node is now on the stack.
        
        if (addwrapper) {
            // dup root node
            body.writeByte(Actions.PushDuplicate);
            // Create and push <resultset> node on stack
            bodyp.push("resultset");
            bodyp.push(0);
            body._writeByte(Actions.InitObject);
            bodyp.push(3); // 3 args : makeElementNode(attrs, name, parent)
            bodyp.push(NODE_INSTANTIATOR_FN);
            bodyp.callFunction();            
            // stack = <root> <resultset>


            // Push <body> element
            body.writeByte(Actions.PushDuplicate);
            bodyp.push("body");
            bodyp.push(0);
            body._writeByte(Actions.InitObject); // {}
            bodyp.push(3); // 3 args : makeElementNode(attrs, name, parent)
            bodyp.push(NODE_INSTANTIATOR_FN);
            bodyp.callFunction(); // stack = <root> <resultset> <body>
        }

        // Build data, which will get stuck under <body> node
        DataContext dc = makeDataContext(flashVersion);
        // Always insert the node constructor function names as the
        // first entries in string pool in this order.
        addStringConstant(NODE_INSTANTIATOR_FN, dc);
        addStringConstant(TEXT_INSTANTIATOR_FN, dc);
        // If it's a big multiframe dataset, a new dc in a new frame may be returned.
        
        // If we are adding a wrapper, the stack has two extra items on it already, <resultset> and <body>.
        int stackdepth = (addwrapper ? 3 : 1);
        dc = writeXMLData(xpp, flashVersion, splitframes, programs, dc, trimWhitespace, stackdepth);
        // Multiple frames may have been created, so append
        // finalization code to the last program in the list.
        bodyp = (Program) programs.lastElement();
        body = bodyp.body();

        // Finalization Phase
        if (addwrapper) {
            // pop the <body> node
            bodyp.pop();
            // stack ==  <root> <resultset> 
            // Add in <headers>, they will get the <resultset> node as parent,
            // since it's on top of stack
            dc = writeXMLData(xpheaders, flashVersion, false, programs, dc, trimWhitespace, stackdepth);
            bodyp = (Program) programs.lastElement();
            body = bodyp.body();
            bodyp.pop(); // stack == <root>
        }

        // Call the node finalize function; takes one arg: the scaffolding root node
        bodyp.push(1); 
        bodyp.push("_level0");
        bodyp.getVar();
        bodyp.push(ROOT_NODE_FINAL_FN);
        bodyp.callMethod();

        // The <resultset> node is on the stack now

        // This is used by the data compiler to compile inline
        // datasets into an app swf file.
        body.writeByte(Actions.PushDuplicate);
        bodyp.push("__lzdataroot"); 
        body.writeByte(Actions.StackSwap);
        bodyp.setVar();


        // If we're runtime-loaded (non-local) data, we need to call
        // back to our parent movieclip data loader.
        if (!localdata) {
            // Call to _parent.loader.returnData(this, data) 
            bodyp.push("this");
            bodyp.getVar();
            bodyp.push(2); 
            bodyp.push("_parent"); 
            bodyp.getVar();
            bodyp.push("loader"); 
            bodyp.getMember();
            bodyp.push("returnData"); 
            bodyp.callMethod();
            bodyp.pop();

            if (splitframes) {
                // add a STOP for multiframe programs, otherwise it loops
                bodyp.stop();
            }
        }

        // prepend the string pool to the program,and add it to the frame list
        Program withpool = appendStringPool(dc, body);
        // add to the list of frame/programs
        programs.setElementAt(withpool, programs.size()-1);

        return programs;
    }


    /**
     * Make SWF
     *
     * @param xpp parser which is pointing at XML data 
     * @return FlashFile containing entire SWF with header
     */
    private static FlashFile makeSWF(XmlPullParser xpp, XmlPullParser xpheaders, int xmlsize,
                                     int flashVersion, boolean addwrapper, boolean trimWhitespace)
      throws IOException, XmlPullParserException {
        mLogger.debug("makeSWF: addwrapper="+addwrapper+" xpp = "+xpp+" xpheaders="+xpheaders+" xmlsize="+xmlsize);
        // Create FlashFile object nd include action bytes
        FlashFile file = FlashFile.newFlashFile();
        Script s = new Script(1);
        file.setMainScript(s);
        file.setVersion(flashVersion);
        Vector progs = makeProgram(xpp, xpheaders, xmlsize, flashVersion, true, addwrapper, trimWhitespace, false);

        // iterate making a frame for each program
        while (progs.size() > 0) {
            Program prog = (Program) progs.firstElement();
            progs.removeElementAt(0); // pop
            Frame frame = s.newFrame();
            frame.addFlashObject(new DoAction(prog));
        }

        return file;
    }

    /** default trimWhitespace to true, for back compatibility */
    public static InputStream getSWF(XmlPullParser xpp, XmlPullParser xpheaders, int xmlsize,
                                     int flashVersion, boolean addwrapper)
      throws IOException, XmlPullParserException {
        return getSWF(xpp, xpheaders, xmlsize, flashVersion, addwrapper, true, false);
    }


    /**
     * Get XML to output stream SWF
     *
     * @param xpp an XPP XML parser which points to the XML data
     * @param xppheaders an XPP XML parser which points to HTTP or other header XML metadata
     * @param flashVersion 5 or greater
     * @param addwrapper Set to true if you pass in a separate HTTP headers string
     * @param trimWhitespace controls whether whitespace is trimmed in text content
     * @return swf input stream
     */
    public static InputStream getSWF(XmlPullParser xpp, XmlPullParser xpheaders, int xmlsize,
                                     int flashVersion, boolean addwrapper, boolean trimWhitespace,
                                     boolean compress)
      throws IOException, XmlPullParserException {
        // Get inputstream and write to outputstream
        InputStream input;
        int i = 0;
        try {
            FlashFile file = makeSWF(xpp, xpheaders, xmlsize, flashVersion, addwrapper, trimWhitespace);
            if (flashVersion > 5) {
                if (compress) {
                    file.setCompressed(true);
                }
            }
            return file.generate().getInputStream();
        } catch (IVException ex) {
            throw new ChainedException(ex);
        } catch (IOException e) {
            mLogger.error("io error getting SWF: " + e.getMessage());
            throw e;
        } 
    }

}
 
