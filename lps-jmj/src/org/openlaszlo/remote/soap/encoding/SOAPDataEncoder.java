/* ****************************************************************************
 * SOAPDataEncoder.java
 *
 * Compile XML directly to SWF bytecodes.
 * ****************************************************************************/

/* J_LZ_COPYRIGHT_BEGIN *******************************************************
* Copyright 2001-2004 Laszlo Systems, Inc.  All Rights Reserved.              *
* Use is subject to license terms.                                            *
* J_LZ_COPYRIGHT_END *********************************************************/

package org.openlaszlo.remote.soap.encoding;

import java.io.*;
import java.util.*;
import java.util.Iterator;

import javax.xml.namespace.QName;
import javax.xml.soap.Name;
import javax.xml.soap.SOAPElement;

import org.openlaszlo.iv.flash.api.*;
import org.openlaszlo.iv.flash.api.action.*;
import org.openlaszlo.iv.flash.util.*;
import org.openlaszlo.remote.soap.LZSOAPUtils;
import org.openlaszlo.utils.ChainedException;
import org.openlaszlo.xml.internal.DataCommon;
import org.openlaszlo.xml.internal.DataContext;
import org.apache.axis.AxisFault;
import org.apache.log4j.Logger;
import org.xml.sax.*;
import org.xml.sax.helpers.AttributesImpl;

import org.apache.axis.message.MessageElement;
import org.apache.axis.message.SOAPHeader;
import org.apache.axis.message.Text;

/**
 */
public class SOAPDataEncoder implements ContentHandler {

    /* Logger */
    private static Logger mLogger  = Logger.getLogger(SOAPDataEncoder.class);

    /** Hint to allocate buffer size large enough to hold output. */
    private int initsize = 0;
    private static final int DEFAULT_BUFFER_SIZE = 4096;

    /** 
     * The SWF file
     */
    private FlashOutput mSWF = null;
    /**
     * Size of the SWF file.
     */
    private long mSize = -1;

    /**
     * Constructs an empty SOAPDataEncoder.
     */
    public SOAPDataEncoder () { }

    /**
     * Workaround variable for bug 4680.
     */
    boolean isProgram = false;

    /**
     * Constructs a SOAPDataEncoder with a buffer allocation size hint.
     * @param initsize hint to allocate buffer size large enough to hold output.
     */
    public SOAPDataEncoder (int initsize) {
        this.initsize = initsize;
    }

    public SOAPDataEncoder(Vector v, SOAPHeader h) {
        buildFromElements(v, h);
    }

    public SOAPDataEncoder(Program p, SOAPHeader h) {
        buildFromProgram(p, h);
    }

    public SOAPDataEncoder buildFromFault(AxisFault fault) {
        start();
        String actor = fault.getFaultActor(); // can be null
        QName  code = fault.getFaultCode(); // can be null
        String node = fault.getFaultNode(); // SOAP1.2
        String reason = fault.getFaultReason(); // SOAP1.2==SOAP1.1 faultstring
        String role = fault.getFaultRole(); // SOAP1.2==SOAP1.1 actor
        String faultstring = fault.getFaultString(); // never null
        QName[]  subcodes = fault.getFaultSubCodes(); // can be null
//         Element[] details  = fault.getFaultDetails(); // can be null
//         ArrayList headers = fault.getHeaders();

        int count = 0;
        if (subcodes != null) {
            for (int i=0; i < subcodes.length; i++) {
                LZSOAPUtils.pushQName(program, subcodes[i], dc);
            }
            program.push(subcodes.length);
            body.writeByte(Actions.InitArray);
            count++;
        }
        if (actor != null) {
            program.push("actor");
            program.push(actor);
            count++;
        }
        if (node != null) {
            program.push("node");
            program.push(node);
            count++;
        }
        if (reason != null) {
            program.push("reason");
            program.push(reason);
            count++;
        }
        if (role != null) {
            program.push("role");
            program.push(role);
            count++;
        }
        if (faultstring != null) {
            program.push("faultstring");
            program.push(faultstring);
            count++;
        }
        if (code != null) {
            program.push("code");
            LZSOAPUtils.pushQName(program, code, dc);
            count++;
        }

        program.push("errortype");
        program.push("fault");

        program.push(count+1);
        body.writeByte(Actions.InitObject);
        end();

        return this;
    }

    public SOAPDataEncoder buildFromException(Exception e) {
        start();
        {
            Throwable cause = e.getCause();
            String message = e.getMessage();
            StringWriter sw = new StringWriter();
            e.printStackTrace(new PrintWriter(sw));

            int count = 3;
            program.push("stacktrace");
            program.push(sw.toString());

            program.push("faultString");
            if (message != null) {
                program.push(message);
            } else { 
                program.push(sw.toString());
            }

            if (cause != null) {
                program.push("cause");
                program.push(cause.toString());
                count++;
            }

            program.push("errortype");
            program.push("exception");

            program.push(count);

            body.writeByte(Actions.InitObject);
        }
        end();
        return this;
    }

    //============================================================
    // SAX API
    //============================================================

    /**
     * Receive notification of character data.
     *
     * @param ch the characters from the XML document.
     * @param start the start position in the array.
     * @param length the number of characters to read from the array.
     *
     * @see #characters(String) characters(String)
     */
    public void characters(char[] ch, int start, int length) {
        String text = new String(ch, start, length);
        characters(text);
    }

    /**
     * Receive notification of string data.
     *
     * @param text the string from the XML document.
     *
     * @see #characters(char[], int, int) characters(char[], int, int)
     */
    public void characters(String text) {
        // makeTextNode = function (text, parent)
        // dup pointer to parent (who is at top of stack)
        // DUP
        body.writeByte(Actions.PushDuplicate);
        // Push text


        body.writeByte(Actions.PushData);
        // Leave a two byte space for the PUSH length
        // Mark where we are, so we can rewrite this with correct length later.
        int push_bufferpos = body.getPos();
        body.writeWord(0); // placeholder 16-bit length field 

        DataCommon.pushMergedStringData(text, body, dc);
        // Set up argsnum and function name
        // PUSH 2, _tdn
        body.writeByte(0x07); // INT type
        body.writeDWord(2);   // '2' integer constant ; number of args to function
        body.writeByte(0x08);     //  SHORT DICTIONARY LOOKUP
        body.writeByte(textnode_idx);  // push function name: index of "_t" string constant

        // Now go back and fix up the size arg to the PUSH instruction
        int total_size = body.getPos() - (push_bufferpos + 2);
        //System.out.println("pos="+body.getPos()+ " total_size = "+total_size+"  push_bufferpos="+push_bufferpos+" nattrs="+nattrs);
        body.writeWordAt(total_size, push_bufferpos);

        body.writeByte(Actions.CallFunction);
        // Pop the node, because there will be no end tag for it, and it has no children.
        body.writeByte(Actions.Pop); 
    }


    /**
     * Receive notification of the end of an element. This method is equivalent
     * to calling {@link #endElement() endElement()} -- the input parameters are
     * ignored.
     *
     * @param uri the Namespace URI, or the empty string if the element has no
     * Namespace URI or if Namespace processing is not being performed.
     * @param localName the local name (without prefix), or the empty string if
     * Namespace processing is not being performed.
     * @param qName the qualified XML 1.0 name (with prefix), or the empty
     * string if qualified names are not available.
     * @see #endElement() endElement()
     */
    public void endElement(String uri, String localName, String qName) {
        // Pop the node off the stack. 
        body.writeByte(Actions.Pop);
    }

    /**
     * Receive notification of the end of an element.
     *
     * @see #endElement(String,String,String) endElement(String,String,String)
     */
    public void endElement() {
        // Pop the node off the stack. 
        body.writeByte(Actions.Pop);
    }


    /**
     * End the scope of a prefix-URI mapping. This method is unimplemented.
     *
     * @param prefix the prefix that was being mapped.
     */
    public void endPrefixMapping(String prefix) {
    }

    /**
     * Receive notification of ignorable whitespace in element content. This
     * method is unimplemented.
     *
     * @param ch the characters from the XML document.
     * @param start the start position in the array.
     * @param length the number of characters to read from the array.
     */
    public void ignorableWhitespace(char[] ch, int start, int length) {
    }

    /**
     * Receive notification of a processing instruction. This method is
     * unimplemented.
     *
     * @param target the processing instruction target.
     * @param data the processing instruction data, or null if none was
     * supplied. The data does not include any whitespace separating it from the
     * target.
     */
    public void processingInstruction(String target, String data) {
    }

    /**
     * Receive an object for locating the origin of SAX document events. This
     * method is unimplemented.
     *
     * @param locator an object that can return the location of any SAX document
     * event.
     */
    public void setDocumentLocator(Locator locator) {
    }

    /**
     * Receive notification of a skipped entity. This method is unimplemented.
     *
     * @param name the name of the skipped entity. If it is a parameter entity,
     * the name will begin with '%', and if it is the external DTD subset, it
     * will be the string "[dtd]".
     */
    public void skippedEntity(String name) {
    }

    /** State vars */
    DataContext dc;
    // The node constructor function name. 
    private byte constructor_idx;
    private byte textnode_idx;

    FlashBuffer body;
    Program program;
    Program resultProgram;
    FlashBuffer out;

    public void start() {
        // Room at the end of the buffer for maybe some callback code to the runtime to say we're done.
        // Allocate enough room to hold the data nodes and strings ; it should be < input XML filesize
        body = new FlashBuffer(initsize == 0 ? DEFAULT_BUFFER_SIZE : initsize);
        program =  new Program(body);
        dc = new DataContext();
    }

    public void end() {
        FlashBuffer body = program.body();

        // Call into viewsystem
        program.push("this");
        program.getVar();
        //----------------------------------------------------------------------
        // FIXME [2004-10-21 pkang]: remove +(isProgram?1:0) conditional once
        // bug 4680 is fixed.
        //----------------------------------------------------------------------
        program.push(3 + (isProgram?1:0));        // this, data(2x?), header
        program.push("_parent"); 
        program.getVar();
        program.push("loader"); 
        body.writeByte(Actions.GetMember);
        program.push("returnData"); 
        program.callMethod();
        program.pop();

        // Collect the string dictionary data
        byte pooldata[] = DataCommon.makeStringPool(dc);
        // 'out' is the main FlashBuffer for composing the output file
        final int MISC = 64;
        out = new FlashBuffer(body.getSize() + pooldata.length + MISC);
        // Write out string constant pool
        out.writeByte( Actions.ConstantPool );
        out.writeWord( pooldata.length + 2 ); // number of bytes in pool data + int (# strings)
        out.writeWord( dc.cpool.size() ); // number of strings in pool
        out.writeArray( pooldata, 0, pooldata.length); // copy the data
        // Write out the code to build nodes
        out.writeArray(body.getBuf(), 0, body.getSize());
        resultProgram = new Program(out);
    }

    /**
     * Receive notification of the beginning of a document.
     *
     * @see #endDocument() endDocument()
     */
    public void startDocument() {
        start();
        constructor_idx   = (byte) (DataCommon.addStringConstant(DataCommon.NODE_INSTANTIATOR_FN, dc) & 0xFF);
        textnode_idx      = (byte) (DataCommon.addStringConstant(DataCommon.TEXT_INSTANTIATOR_FN, dc) & 0xFF);

        // Bind the node creation functions to some short local names:
        //  element nodes: _root._m => _m
        program.push(new Object[]{"_m", "_root"});
        program.getVar();
        program.push("_m");
        body.writeByte(Actions.GetMember);
        program.setVar();

        //  text nodes: _root._t => _t
        program.push(new Object[]{"_t", "_root"});
        program.getVar();
        program.push("_t");
        body.writeByte(Actions.GetMember);
        program.setVar();
    }

    /**
     * Receive notification of the end of a document.
     *
     * @see #startDocument() startDocument()
     */
    public void endDocument() {
        end();
    }

    /**
     * @return the Flash program to build the data set.
     */
    private Program getProgram() {
        return resultProgram;
    }


    /**
     * Make SWF
     *
     * @return FlashFile containing entire SWF with header
     */
    private FlashFile makeSWFFile() {
        // Create FlashFile object nd include action bytes
        FlashFile file = FlashFile.newFlashFile();
        Script s = new Script(1);
        file.setMainScript(s);
        file.setVersion(6);
        Frame frame = s.newFrame();
        frame.addFlashObject(new DoAction(resultProgram));
        return file;
    }


    /**
     * Get the compiled data swf program byte codes.  Only call this after you
     * have called {@link #endDocument endDocument()}.
     *
     * @return input stream containing the compiled SWF data program; only valid 
     * after {@link #endDocument endDocument()} has been called.  Must not be called
     * before {@link #endDocument endDocument()}.
     */
    public InputStream getInputStream() 
        throws IOException {

        generate();
        return mSWF.getInputStream();
    }

    /**
     * Return the size of the output object; only valid after endDocument
     * {@link #endDocument endDocument()} has been called.  Must not be called
     * before {@link #endDocument endDocument()}.
     *
     * @return long representing the size
     */
    public long getSize() 
        throws IOException {

        generate();
        return mSize;
    }

    /**
     * Generate the SWF file
     */
    private void generate() throws IOException {

        if (mSWF == null) {
            try {
                InputStream input;
                FlashFile file = makeSWFFile();
                mSWF = file.generate();
                mSize = mSWF.pos;
            } catch (IVException ex) {
                throw new ChainedException(ex);
            }
        }
    }

    /**
     * A lower level call than startElement(); attributes must be supplied by
     * individual calls to addAttribute(). This method is unimplemented.
     * @param localName the element name.
     */
    public void _startElement (String localName) {

    }

    /** 
     * A low level call to add an attribute, must be preceded by call to
     * _startElement() for a given element. This method is unimplemented.
     */
    public void addAttribute (String attrName, String attrVal) {

    }


    /**
     * Receive notification of the beginning of an element. This method is
     * equivalent to calling {@link #startElement(String, Attributes)
     * startElement(String, Attributes)} -- the uri and qName parameters are
     * ignored.
     *
     * @param uri the Namespace URI, or the empty string if the element has no
     * Namespace URI or if Namespace processing is not being performed.
     * @param localName the local name (without prefix), or the empty string if
     * Namespace processing is not being performed.
     * @param qName the qualified name (with prefix), or the empty string if
     * qualified names are not available.
     * @param atts the attributes attached to the element. If there are no
     * attributes, it shall be an empty Attributes object.
     *
     * @see #startElement(String, Attributes) startElement(String, Attributes)
     */
    public void startElement(String uri, String localName, String qName, Attributes atts) {
        startElement(localName, atts);
    }


    /**
     * Receive notification of the beginning of an element.
     *
     * @param localName the local name (without prefix), or the empty string if
     * Namespace processing is not being performed.
     * @param atts the attributes attached to the element. If there are no
     * attributes, it shall be an empty Attributes object.
     *
     * @see #startElement(String, String, String, Attributes)
     * startElement(String, String, String, Attributes)
     */
    public void startElement(String localName, Attributes atts) {
        int idx; // tmp var to hold a string pool index
        // makeNodeNoText = function (attrs, name, parent)
        // dup pointer to PARENT (who is at top of stack)
        // DUP
        body.writeByte(Actions.PushDuplicate);

        // We're really squeezing things down, so we are going to merge 
        // the PUSH of the element name with the PUSH of the attribute key/value
        // data and the attribute count. So the stack will look like
        // [eltname attrname1 attrval1 attrname2 attrval2 ... ... nattrs]
        // when we're done
        body.writeByte(Actions.PushData);
        // Leave a two byte space for the PUSH length
        // Mark where we are, so we can rewrite this with correct length later.
        int push_bufferpos = body.getPos();
        body.writeWord(0); // placeholder 16-bit length field 

        // Push element NAME
        String eltname = localName;
        DataCommon.pushMergedStringDataSymbol(eltname, body, dc);

        // Fold all the attribute key/value pairs into a single PUSH
        // Build ATTRIBUTE object
        int nattrs = atts.getLength();

        // PUSH := {0x96, lenlo, lenhi, 0x00, char, char, char, ...0x00, }
        for (int i = 0; i < nattrs; i++) {
            String attrname = atts.getLocalName(i);
            //System.out.print("Attr " + attrname);
            DataCommon.pushMergedStringDataSymbol(attrname, body, dc);

            String attrval = atts.getValue(i);
            //System.out.println("= " + attrval);
            DataCommon.pushMergedStringData(attrval, body, dc);
        }
        // create the attrs object; push the attr count
        body.writeByte(0x07); // INT type
        body.writeDWord(nattrs);
        // Now go back and fix up the size arg to the PUSH instruction
        int total_size = body.getPos() - (push_bufferpos + 2);
        //System.out.println("pos="+body.getPos()+ " total_size = "+total_size+"  push_bufferpos="+push_bufferpos+" nattrs="+nattrs);
        body.writeWordAt(total_size, push_bufferpos);
        body.writeByte(Actions.InitObject);

        // stack now has [parent, name, attrs]
        // Push # of args and node-instantiator-function name
        // PUSH 3, _mdn
        // [PUSHDATA, 7, 0,  0x07, 03 00 00 00 0x08, constructor_idx]
        body.writeByte(Actions.PushData);
        body.writeWord(7);
        body.writeByte(0x07); // INT type
        body.writeDWord(3);   // '3' integer constant , number of args to node constructor fn
        body.writeByte(0x08);     //  SHORT DICTIONARY LOOKUP type
        body.writeByte(constructor_idx);  // index of "_m" string constant
        body.writeByte(Actions.CallFunction);
        // We leave the new node on the stack, so we can reference it as the parent 
        // Stack => [parentnode newnode]
    }

    /**
     * Begin the scope of a prefix-URI Namespace mapping. This method is
     * unimplemented.
     *
     * @param prefix the Namespace prefix being declared.
     * @param uri the Namespace URI the prefix is mapped to.
     */
    public void startPrefixMapping(String prefix, String uri) {
    }


    //============================================================
    // End SAX ContentHandler Compatibility
    //============================================================

    private void buildHeaders(SOAPHeader h) {
        int hCount = 0;
        Iterator iter = h.getChildElements();
        while (iter.hasNext()) {
            hCount++;
            traverseDOM((SOAPElement)iter.next());
        }
        if (hCount != 0) {
            program.push(hCount);
            body.writeByte(Actions.InitArray);
        } else {
            LZSOAPUtils.pushNull(body);
        }
    }

    /**
     * Build from a vector of SOAPElement items.
     *
     * @param v vector of SOAPElement items.
     */
    public void buildFromElements(Vector v, SOAPHeader h) {
        startDocument();

        // soap headers
        buildHeaders(h);

        // array of documents
        for (int i = v.size()-1; i >= 0; --i) {
            traverseDOM((SOAPElement)v.get(i));
        }
        program.push(v.size());
        body.writeByte(Actions.InitArray);

        endDocument();
    }


    /**
     * Build from a vector of SOAPElement items.
     *
     * @param v vector of SOAPElement items.
     */
    public void buildFromProgram(Program p, SOAPHeader h) {
        // Workaround variable for bug 4680
        isProgram = true;

        startDocument();

        // soap headers
        buildHeaders(h);

        // program
        if ( p == null ) {
            LZSOAPUtils.pushNull(body);
        } else {
            body.writeFOB(p.body());
        }

        endDocument();
    }


    /**
     * Traverse DOM for single SOAPElement.
     */
    private void traverseDOM(SOAPElement el) {
        // Build a root node by calling the runtime's root node instantiator
        // 
        // The root node will have $n = 0, and whatever other initial conditions are needed.
        program.push(0); // Root node creator function takes no args.
        program.push("_root");
        program.getVar();
        program.push(DataCommon.ROOT_NODE_INSTANTIATOR_FN);
        program.callMethod();
        // The root node is now on the stack.
        // Build data. Invariant is that it leaves the stack the way it found it.

        AttributesImpl emptyAttr = new AttributesImpl();
//         startElement("resultset", emptyAttr);
//         startElement("body", emptyAttr);

        // we're now set up for a single document...let's traverse the element
        _traverseDOM(el);

//         endElement();
//         endElement();

        // The root node is sitting on the stack. Finalize; bind the variable
        // "root" to the node that the lfc expects.
        program.push(1); 
        program.push("_root");
        program.getVar();
        program.push(DataCommon.ROOT_NODE_FINAL_FN);
        program.callMethod();
    }


    /**
     * Help traverse the DOM. Start point is traverseDOM().
     */
    private void _traverseDOM(SOAPElement el) {
        AttributesImpl attrs = new AttributesImpl();
        Iterator iter = el.getAllAttributes();
        while (iter.hasNext()) {
            Name attrName = (Name)iter.next();
            String attrValue = el.getAttributeValue(attrName);
            attrs.addAttribute(attrName.getURI(), attrName.getLocalName(),
                               attrName.getQualifiedName(), "",
                               attrValue);
        }

        Name elName = el.getElementName();
        startElement(elName.getLocalName(), attrs);

        iter = el.getChildElements();
        while (iter.hasNext()) {
            Object o = iter.next();
            if (Text.class.isInstance(o)) {
                // add text node
                characters(((Text)o).getValue());
            } else {
                SOAPElement child = (SOAPElement)o;
                _traverseDOM(child);
            }
        }


        endElement();

    }


}
