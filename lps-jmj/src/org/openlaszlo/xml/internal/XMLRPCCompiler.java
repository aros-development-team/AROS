/* ****************************************************************************
 * XMLRPCCompiler.java
 * ****************************************************************************/

/* J_LZ_COPYRIGHT_BEGIN *******************************************************
* Copyright 2001-2004 Laszlo Systems, Inc.  All Rights Reserved.              *
* Use is subject to license terms.                                            *
* J_LZ_COPYRIGHT_END *********************************************************/

package org.openlaszlo.xml.internal;

import java.io.*;
import java.util.*;
import org.w3c.dom.*;
import org.xml.sax.*;
import javax.xml.parsers.*;
import org.openlaszlo.iv.flash.util.*;
import org.openlaszlo.iv.flash.api.action.*;
import org.openlaszlo.iv.flash.api.*;
import org.openlaszlo.utils.ChainedException;
import org.openlaszlo.utils.FileUtils;
import org.apache.log4j.*;

/**
 * Use XMLRPCCompiler.compile().
 */
public class XMLRPCCompiler
{
    public static Logger mLogger = Logger.getLogger(XMLRPCCompiler.class);

    private static DocumentBuilderFactory factory = null;

    private FlashBuffer body;
    private Program program;

    /**
     * Get document builder factory.
     */
    static DocumentBuilderFactory getDocumentBuilderFactory() {
        if (factory == null) {
            try {
                factory = DocumentBuilderFactory.newInstance();
            } catch (FactoryConfigurationError e) {
                throw new RuntimeException(e.getMessage());
            }
        }
        return factory;
    }


    /**
     * Get the list of first-level children based on tag name.
     * @param parent element to retrieve first-level children based on tag name.
     * @param tag tag based to search children.
     */
    static List getElementsByTagName(Element parent, String tag)
    {
        List list = new Vector();
        NodeList children = parent.getChildNodes();

        for (int i=0; i < children.getLength(); i++) {
            Node node = children.item(i);
            if (node.getNodeType() == Node.ELEMENT_NODE) {
                list.add((Element)node);
            }
        }
        return list;
    }

    /**
     * Skips through comments and spaces.
     * @param parent an element.
     * @param n the nth child element to get from parent (starting from 0).
     */
    static Element getChildElement(Element parent, int n) {
        NodeList children = parent.getChildNodes();
        int count = 0;
        for (int i=0; i < children.getLength(); i++) {
            Node node = children.item(i);
            if (node.getNodeType() == Node.ELEMENT_NODE) {
                if (n == count++) {
                    mLogger.debug("found " + node.getNodeName());
                    return (Element)node;
                }
            }
        }
        return null;
    }

    /**
     * Get text string of an element.
     * @param parent element to get text from.
     */
    static String getFirstChildTextString(Element parent) {
        Node text = parent.getFirstChild();
        return text != null && text.getNodeType() == Node.TEXT_NODE
            ? ( (Text)text ).getData() : "";
    }

    /**
     * Write XMLRPC string response to SWF.
     * @param string XMLRPC string to push.
     */
    void writeString(String string)
        throws IOException {
        mLogger.debug("writeString");
        program.push(string);
    }

    /**
     * Write XMLRPC integer response to SWF.
     * @param intval XMLRPC integer string response to parse and push.
     */
    void writeInteger(String intval)
        throws IOException {
        mLogger.debug("writeInteger");
        try {
            program.push(Integer.parseInt(intval));
        } catch (NumberFormatException e) {
            throw new IOException(e.getMessage());
        }
    }

    /**
     * Write XMLRPC double response to SWF.
     * @param doubleval XMLRPC 'double' string response to parse to float and
     * push.
     */
    void writeDouble(String doubleval)
        throws IOException {
        mLogger.debug("writeDouble");
        try {
            // Program doesn't have a push(double).
            // This code taken from Program.push( Object data) in jgenerator-2.2.
            body.writeByte(Actions.PushData);
            body.writeWord(8+1);
            body.writeByte(6);
            long dbits = Double.doubleToLongBits(Double.parseDouble(doubleval));
            body.writeDWord((int)(dbits>>>32));
            body.writeDWord((int)(dbits&0xffffffffL));
        } catch (NumberFormatException e) {
            throw new IOException(e.getMessage());
        }
    }

    /**
     * Snarfed from Program.push(Object o) in jgenerator-2.2.
     */
    void pushBoolean(boolean b)
    {
        body.writeByte(Actions.PushData);
        body.writeWord(1+1);
        body.writeByte(5);
        body.writeByte(b?1:0);
    }

    /**
     * Write XMLRPC boolean response to SWF.
     * @param boleanval XMLRPC boolean string response to parse and push.
     */
    void writeBoolean(String booleanval)
        throws IOException {
        mLogger.debug("writeBoolean");
        try {
            pushBoolean(Integer.parseInt(booleanval) != 0);
        } catch (NumberFormatException e) {
            if (booleanval.equals("false"))
                pushBoolean(false);
            else if (booleanval.equals("true"))
                pushBoolean(true);
            else
                throw new IOException("not a boolean value");
        }
    }

    /**
     * Write XMLRPC array response to SWF.
     * @param array XMLRPC array element to parse and push.
     */
    void writeArray(Element array)
        throws IOException {
        mLogger.debug("writeArray");
        List data = getElementsByTagName(array, "data");
        if (data.size() != 1)
            throw new IOException("Invalid number of data elements in array");

        List values = getElementsByTagName((Element)data.get(0), "value");
        for (int i = values.size()-1; i >= 0; --i) {
            writeValue((Element)values.get(i));
        }
        // Now push the array length and initialize it
        program.push(values.size());
        body.writeByte(Actions.InitArray);
    }

    /**
     * Write XMLRPC struct response to SWF.
     * @param struct XMLRPC struct element to parse and push.
     */
    void writeStruct(Element struct)
        throws IOException {
        mLogger.debug("writeStruct");
        List members = getElementsByTagName(struct, "member");
        // Push member values in reverse order
        for (int i = members.size()-1; i >= 0; --i) {
            writeMember((Element)members.get(i));
        }
        // Now push length of hash values and initialize it
        program.push(members.size());
        body.writeByte(Actions.InitObject);
    }

    /**
     * Write XMLRPC struct member to SWF. Helper for writeStruct.
     * @param member XMLRPC struct member element to parse and push.
     */
    void writeMember(Element member)
        throws IOException {
        mLogger.debug("writeMember");
        Element name = getChildElement(member, 0);
        Element value = getChildElement(member, 1);
        if (name == null || value == null)
            throw new IOException("Name or value appears to be null.");
        if (! name.getNodeName().equals("name"))
            throw new IOException("Name does not appear to be the first argument in member");
        if (! value.getNodeName().equals("value"))
            throw new IOException("Value does not appear to be the second argument in member");

        // when initing hash, value are popped before names
        program.push(getFirstChildTextString(name));
        writeValue(value);
    }

    /**
     * Unimplemented.
     */
    void writeDateTime(Element datetimeval)
        throws RuntimeException {
        mLogger.debug("writeDateTime");
        throw new RuntimeException("datetime.iso8601 unimplemented");
    }

    /**
     * Unimplemented.
     */
    void writeBase64(Element base64val) {
        mLogger.debug("writeBase64");
        throw new RuntimeException("base64 unimplemented");
    }


    /**
     * Write XMLRPC response to SWF.
     * @param value
     */
    void writeValue(Element value)
        throws IOException {
        mLogger.debug("writeValue");

        Element type = getChildElement(value, 0);
        if (type == null) {
            writeString(getFirstChildTextString(value));
            return;
        }

        String t = type.getTagName();
        if (t.equals("string")) {
            writeString(getFirstChildTextString(type));
        } else if (t.equals("int") || t.equals("i4")) {
            writeInteger(getFirstChildTextString(type));
        } else if (t.equals("double")) {
            writeDouble(getFirstChildTextString(type));
        } else if (t.equals("boolean")) {
            writeBoolean(getFirstChildTextString(type));
        } else if (t.equals("struct")) {
            writeStruct(type);
        } else if (t.equals("array")) {
            writeArray(type);
        } else if (t.equals("dateTime.iso8601")) {
            writeDateTime(type);
        } else if (t.equals("base64")) {
            writeBase64(type);
        }
    }

    /**
     *
     */
    void writeParams(Element params)
        throws IOException {
        mLogger.debug("writeParams");

        Element param = getChildElement(params, 0);
        if (! param.getTagName().equals("param"))
            throw new IOException("Invalid params body");

        Element value = getChildElement(param, 0);
        if (! value.getTagName().equals("value"))
            throw new IOException("Invalid param body");

        writeValue(value);
    }

    /**
     * Write XMLRPC fault response struct to SWF.
     * @param faul the fault element.
     */
    void writeFault(Element fault)
        throws IOException {
        mLogger.debug("writeFault");

        Element value = getChildElement(fault, 0);
        if (! value.getTagName().equals("value"))
            throw new IOException("Invalid param body");

        writeValue(value);
    }

    /**
     *
     */
    void writeXMLRPCData(Element element)
        throws IOException {
        mLogger.debug("writeXMLRPCData");

        if (! element.getTagName().equals("methodResponse"))
            throw new IOException("Invalid XMLRPC response");

        Element child = getChildElement(element, 0);
        if (child != null) {
            if (child.getTagName().equals("params")) {
                writeParams(child);
                return;
            } else if (child.getTagName().equals("fault")) {
                writeFault(child);
                return;
            }
        }
        // TODO: [2004-02-20 pkang] this should return actionscript for xmlrpc
        // client lib
        throw new IOException("bad XMLRPC message body");
    }


    /**
     * Produces a JGenerator Flash Program containing executable SWF codes to
     * build an XML datasource structure which represents this XML stream.
     *
     * @param element xml response document element
     * @param xmlsize size of xml used to create initial flash buffer.
     * @return Flash Program containing entire SWF with header
     */
    public Program makeProgram(Element element, int xmlsize)
        throws IOException {
        mLogger.debug("makeProgram");

        // Room at the end of the buffer for maybe some callback code to the
        // runtime to say we're done.
        final int MISC = 4096;
        // Allocate enough room to hold the data nodes and strings ; it should
        // be < (input XML filesize * scalar)
        body = new FlashBuffer((int) (Math.floor(xmlsize * 3) + MISC));
        program = new Program(body);

        writeXMLRPCData(element);

        // call into the viewsystem
        program.push("this");
        program.getVar();
        program.push(2);
        program.push("_parent");
        program.getVar();
        program.push("loader");
        body.writeByte(Actions.GetMember);
        program.push("returnData");
        program.callMethod();
        program.pop();

        return program;
    }


    /**
     * Make SWF
     *
     * @param xpp parser which is pointing at XML data
     * @return FlashFile containing entire SWF with header
     */
    public FlashFile makeSWF(Element element, int xmlsize, int swfversion)
        throws IOException {
        mLogger.debug("makeSWF");

        // Create FlashFile object nd include action bytes
        FlashFile file = FlashFile.newFlashFile();
        Script s = new Script(1);
        file.setMainScript(s);
        file.setVersion(swfversion);
        Frame frame = s.newFrame();
        Program program = makeProgram(element, xmlsize);
        frame.addFlashObject(new DoAction(program));
        return file;
    }


    /**
     * Get XML to output stream SWF
     *
     * @param xpp an XPP XML parser which points to the XML data
     * @return swf input stream
     */
    public byte[] getSWF(Element element, int xmlsize, int swfversion)
        throws IOException {
        mLogger.debug("getSWF");
        int i = 0;
        try {
            FlashFile file = makeSWF(element, xmlsize, swfversion);
            FlashOutput fob = file.generate();
            byte[] buf = new byte[fob.getSize()];
            System.arraycopy(fob.getBuf(), 0, buf, 0, fob.getSize());
            return buf;
        } catch (IVException e) {
            throw new ChainedException(e);
        } catch (IOException e) {
            mLogger.error("io error getting SWF: " + e.getMessage());
            throw e;
        }
    }

    /**
     * @see compile(Reader, int)
     */
    public static byte[] compile(String xmlrpc, int swfversion)
        throws IOException {
        return compile(new StringReader(xmlrpc), xmlrpc.length(), swfversion);
    }

    /**
     * Compile the XMLRPC response to SWF bytecode.
     * @param in XMLRPC input.
     * @return SWF bytecode for flash client.
     */
    public static byte[] compile(Reader reader, int xmlsize, int swfversion)
        throws IOException {
        mLogger.debug("compile(reader,xmlsize,swfversion)");
        try {
            // TODO: [2004-02-20 pkang] do we worry about character encoding?
            DocumentBuilder builder = getDocumentBuilderFactory().newDocumentBuilder();
            Document document = builder.parse(new InputSource(reader));
            return new XMLRPCCompiler().getSWF(document.getDocumentElement(), xmlsize, swfversion);
        } catch (Exception e) {
            mLogger.error("Caught exception at compile: " + e);
            StringWriter trace = new StringWriter();
            e.printStackTrace(new PrintWriter(trace));
            return compileFault(trace.toString(), swfversion);
        }
    }


    public static String xmlFaultResponse(int code, String message) {
        return new StringBuffer("<?xml version=\"1.0\"?>")
            .append("<methodResponse>")
            .append("<fault>")
            .append("<value>")
            .append("<struct>")
            .append("<member>")
            .append("<name>faultCode</name>")
            .append("<value><int>").append(code).append("</int></value>")
            .append("</member>")
            .append("<member>")
            .append("<name>faultString</name>")
            .append("<value><string>")
            .append(message)
            .append("</string></value>")
            .append("</member>")
            .append("</struct>")
            .append("</value>")
            .append("</fault>")
            .append("</methodResponse>")
            .toString();
    }

    public static byte[] compileResponse(int code, String message, int swfversion)
        throws IOException {
        String fault = xmlFaultResponse(code, message);
        try {
            DocumentBuilder builder = getDocumentBuilderFactory().newDocumentBuilder();
            Document document = builder.parse
                (new InputSource(new StringReader(fault)));
            return new XMLRPCCompiler().getSWF(document.getDocumentElement(), fault.length(), swfversion);
        } catch (Exception e) {
            mLogger.error("Caught exception at compileFault: " + message, e);
            throw new IOException(e.getMessage());
        }
    }

    /**
     * Used by compiler to send back exception messages.
     */
    public static byte[] compileFault(String message, int swfversion) throws IOException {
        return compileResponse(-1, message, swfversion);
    }


    /**
     * Main.
     */
    public static void main(String[] args) {
        System.out.println("args: " + args.length);
        if (args.length != 1) {
            System.err.println("Usage: XMLRPCCompiler xmlrpcfile");
            return;
        }
        try {
            File file = new File(args[0]);
            InputStream in = new ByteArrayInputStream(compile(new FileReader(file), 
                                                              (int)file.length(), 6));
            OutputStream out = new FileOutputStream("xmlrpc.swf");
            FileUtils.send(in, out, 4096);
        } catch (Exception e) {
            e.printStackTrace();
        }
    }
}
