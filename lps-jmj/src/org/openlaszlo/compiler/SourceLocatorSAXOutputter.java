/* *****************************************************************************
 * Parser.java
* ****************************************************************************/

/* J_LZ_COPYRIGHT_BEGIN *******************************************************
* Copyright 2001-2004 Laszlo Systems, Inc.  All Rights Reserved.              *
* Use is subject to license terms.                                            *
* J_LZ_COPYRIGHT_END *********************************************************/

package org.openlaszlo.compiler;

import java.io.*;
import java.lang.reflect.Method;
import java.lang.reflect.InvocationTargetException;
import java.util.*;

import org.xml.sax.*;
import org.xml.sax.Locator;
import org.xml.sax.ext.DeclHandler;
import org.xml.sax.ext.LexicalHandler;
import org.xml.sax.helpers.LocatorImpl;
import org.xml.sax.helpers.AttributesImpl;
import org.xml.sax.helpers.XMLFilterImpl;

import org.jdom.*;
import org.jdom.Element;


/**

  Generate SAX events from a Laszlo Parser JDOM tree, with filename
  and linenumber info updated from the ElementWithLocationInfo
  elements.

  <p>
  This does the bare minimum needed to feed the RELAXNG
  validator. It is not namespace aware.

 */
public class SourceLocatorSAXOutputter extends XMLFilterImpl{
    public static final String SOURCEINFO_ATTRIBUTE_NAME = "_lzc_meta_sourceLocation";
    
    private LocatorImpl locator = new LocatorImpl();

    /** registered <code>ContentHandler</code> */
    private ContentHandler contentHandler;
   
    /** registered <code>ErrorHandler</code> */
    private ErrorHandler errorHandler;
   
    private boolean writeMetaData = false;

    /**
     * <p>
     * This will create a <code>SAXOutputter</code> without any
     * registered handler.  The application is then responsible for
     * registering them using the <code>setXxxHandler()</code> methods.
     * </p>
     */
    public SourceLocatorSAXOutputter() {
    }

    /**
     * <p>
     * This will create a <code>SAXOutputter</code> with the
     * specified <code>ContentHandler</code>.
     * </p>
     *
     * @param contentHandler contains <code>ContentHandler</code> 
     * callback methods
     */
    public SourceLocatorSAXOutputter(ContentHandler contentHandler) {
        setContentHandler(contentHandler);
    }

    /**
     * <p>
     * This will set the <code>ContentHandler</code>.
     * </p>
     *
     * @param contentHandler contains <code>ContentHandler</code> 
     * callback methods.
     */
    public void setContentHandler(ContentHandler contentHandler) {
        this.contentHandler = contentHandler;
    }
   
    /**
     * <p>
     * Returns the registered <code>ContentHandler</code>.
     * </p>
     *
     * @return the current <code>ContentHandler</code> or
     * <code>null</code> if none was registered.
     */
    public ContentHandler getContentHandler() {
        return this.contentHandler;
    }

    /**
     * <p>
     * This will set the <code>ErrorHandler</code>.
     * </p>
     *
     * @param errorHandler contains <code>ErrorHandler</code> callback methods.
     */
    public void setErrorHandler(ErrorHandler errorHandler) {
        this.errorHandler = errorHandler;
    }

    /**
     * <p>
     * Return the registered <code>ErrorHandler</code>.
     * </p>
     *
     * @return the current <code>ErrorHandler</code> or
     * <code>null</code> if none was registered.
     */
    public ErrorHandler getErrorHandler() {
        return this.errorHandler;
    }


    /**
     * <p>
     * This will set the state of a SAX feature. We don't support any options right now.
     */
    public void setFeature(String name, boolean value)
                throws SAXNotRecognizedException, SAXNotSupportedException {
        // No options supported
    }

    /**
     * This will look up the value of a SAX feature.
     */
    public boolean getFeature(String name)
                throws SAXNotRecognizedException, SAXNotSupportedException {
        // we don't support any options
        return false;
    }

    /**
     */
    public void setProperty(String name, Object value)
                throws SAXNotRecognizedException, SAXNotSupportedException {
        // nothing to see here
    }

    /**
     * <p>
     * This will look up the value of a SAX property.
     * </p>
     *
     */
    public Object getProperty(String name)
                throws SAXNotRecognizedException, SAXNotSupportedException {
        // nothing to see here
        return null;
    }

    public void setWriteMetaData(boolean writeMetaData) {
        this.writeMetaData = writeMetaData;
    }
    
    /**
     * <p>
     * This will output the <code>JDOM Document</code>, firing off the
     * SAX events that have been registered.
     * </p>
     *
     * @param document <code>JDOM Document</code> to output.
     */
    public void output(Document document) throws JDOMException {
        if (document == null) {
            return;
        }

        // contentHandler.setDocumentLocator()
        documentLocator(document);

        // contentHandler.startDocument()
        _startDocument();

        // Fire DTD events
        //dtdEvents(document);

        // Handle root element, as well as any root level
        // processing instructions and CDATA sections
        Iterator i = document.getContent().iterator();
        while (i.hasNext()) {
            Object obj = i.next();
            if (obj instanceof Element) {
                // process root element and its content
                element(document.getRootElement());
            }
            else if (obj instanceof ProcessingInstruction) {
                // contentHandler.processingInstruction()
                processingInstruction((ProcessingInstruction) obj);
            }
            else if (obj instanceof CDATA) {
                // contentHandler.characters()
                characters(((CDATA) obj).getText());
            }
        }
       
        // contentHandler.endDocument()
        _endDocument();
    }
   
    /**
     *  Copy source linenumber information from the Elements to
     *  make them available to the SAX API.
     * 
     *
     * @param document JDOM <code>Document</code>.  */
    private void documentLocator(Document document) {
        String publicID = null;
        String systemID = null;
        DocType docType = document.getDocType();
        if (docType != null) {
            publicID = docType.getPublicID();
            systemID = docType.getSystemID();
        }
      
        locator.setPublicId(publicID);
        locator.setSystemId(systemID);
        locator.setLineNumber(1);
        locator.setColumnNumber(1);
      
        contentHandler.setDocumentLocator((Locator) locator);
    }
   
    /**
     * <p>
     * This method is always the second method of all callbacks in
     * all handlers to be invoked (setDocumentLocator is always first).
     * </p>
     */
    private void _startDocument() throws JDOMException {
        try {
            contentHandler.startDocument();
            //contentHandler.startPrefixMapping("ps",
            //"http://www.psol.com/2001/08/dw/tip");
            //attributes.addAttribute("","ps","xmlns:ps","CDATA",
            //"http://www.psol.com/2001/08/dw/tip");        }
        } catch (SAXException se) {
            throw new JDOMException("Exception in startDocument", se);
        }
    }
   
    /**
     * <p>
     * Always the last method of all callbacks in all handlers
     * to be invoked.
     * </p>
     */
    private void _endDocument() throws JDOMException {
        try {
            contentHandler.endDocument();
        }
        catch (SAXException se) {
            throw new JDOMException("Exception in endDocument", se);
        }
    }
   
    /**
     * <p>
     * This will invoke the <code>ContentHandler.processingInstruction</code>
     * callback when a processing instruction is encountered.
     * </p>
     *
     * @param pi <code>ProcessingInstruction</code> containing target and data.
     */
    private void processingInstruction(ProcessingInstruction pi) 
                           throws JDOMException {
        if (pi != null) {
            String target = pi.getTarget();
            String data = pi.getData();
            try {
                contentHandler.processingInstruction(target, data);
            }
            catch (SAXException se) {
                throw new JDOMException(
                    "Exception in processingInstruction", se);
            }
        }
    }
   
    /**
     * <p>
     * This will recursively invoke all of the callbacks for a particular 
     * element.
     * </p>
     *
     * @param element <code>Element</code> used in callbacks.
     */
    private void element(Element element) 
                           throws JDOMException {

        // Update the document locator
        Integer lineNumber = Parser.getSourceLocation((ElementWithLocationInfo) element, Parser.LINENO);
        Integer colNumber = Parser.getSourceLocation((ElementWithLocationInfo) element, Parser.COLNO);

        locator.setSystemId(Parser.getSourcePathname(element));
        locator.setPublicId(Parser.getSourceMessagePathname(element));
        locator.setLineNumber(lineNumber.intValue());
        locator.setColumnNumber(colNumber.intValue());
        contentHandler.setDocumentLocator((Locator) locator);

        // +++ Check if we need to send a new documentLocator event if
        // we changed these values?

        // contentHandler.startElement()
        startElement(element);

        // handle content in the element
        elementContent(element);

        // contentHandler.endElement()
        endElement(element);

    }

    /**
     * <p>
     * This will invoke the <code>startElement</code> callback
     * in the <code>ContentHandler</code>.
     * </p>
     *
     * @param element <code>Element</code> used in callbacks.
     * @param eltNamespaces <code>List</code> of namespaces to declare with
     * the element or <code>null</code>.
     */
    private void startElement(Element element) 
                      throws JDOMException {
        String namespaceURI = element.getNamespaceURI();
        String localName = element.getName();
        String rawName = element.getQualifiedName();

        AttributesImpl atts = new AttributesImpl();
        List attributes = element.getAttributes();
        Iterator i = attributes.iterator();
        while (i.hasNext()) {
            Attribute a = (Attribute) i.next();
            atts.addAttribute(a.getNamespaceURI(),
                              a.getName(),
                              a.getQualifiedName(),
                              getAttributeTypeName(a.getAttributeType()),
                              a.getValue());
        }
        if (this.writeMetaData) {
            // TODO [2003-4-30 ows]: Use a namespace for LZX
            // metasource information, instead of an obfuscated name.
            atts.addAttribute("",
                              SOURCEINFO_ATTRIBUTE_NAME,
                              SOURCEINFO_ATTRIBUTE_NAME,
                              "CDATA",
                              ((ElementWithLocationInfo) element).getSourceLocator().toString()
                              );
        }
        
        try {
            contentHandler.startElement(namespaceURI, localName, rawName, atts);
        }
        catch (SAXException se) {
            throw new JDOMException("Exception in startElement", se);
        }
    }
   
    /**
     * <p>
     * This will invoke the <code>endElement</code> callback
     * in the <code>ContentHandler</code>.
     * </p>
     *
     * @param element <code>Element</code> used in callbacks.
     */
    private void endElement(Element element) throws JDOMException {
        String namespaceURI = element.getNamespaceURI();
        String localName = element.getName();
        String rawName = element.getQualifiedName();
        
        try {
            contentHandler.endElement(namespaceURI, localName, rawName);
        }
        catch (SAXException se) {
            throw new JDOMException("Exception in endElement", se);
        }
    }
   
    /**
     * <p>
     * This will invoke the callbacks for the content of an element.
     * </p>
     *
     * @param element <code>Element</code> used in callbacks.
     */
    private void elementContent(Element element) 
                      throws JDOMException {
        List eltContent = element.getContent();
      
        boolean empty = eltContent.size() == 0;
        boolean stringOnly =
            !empty &&
            eltContent.size() == 1 &&
            eltContent.get(0) instanceof Text;
          
        if (stringOnly) {
            // contentHandler.characters()
            characters(element.getText());
        }
        else {
            Object content = null;
            for (int i = 0, size = eltContent.size(); i < size; i++) {
                content = eltContent.get(i);
                if (content instanceof Element) {
                    element((Element) content);
                }
                else if (content instanceof Text) {
                    // contentHandler.characters()
                    characters(((Text) content).getText());
                }
                else if (content instanceof CDATA) {
                    // contentHandler.characters()
                    characters(((CDATA) content).getText());
                }
                else if (content instanceof ProcessingInstruction) {
                    // contentHandler.processingInstruction()
                    processingInstruction((ProcessingInstruction) content);
                }
            }
        }
    }
    
    /**
     * <p>
     * This will be called for each chunk of character data encountered.
     * </p>
     *
     * @param elementText all text in an element, including whitespace.
     */
    private void characters(String elementText) throws JDOMException {
        char[] c = elementText.toCharArray();
        try {
            contentHandler.characters(c, 0, c.length);
        }
        catch (SAXException se) {
            throw new JDOMException("Exception in characters", se);
        }
    }


    /**
     * Array to map JDOM attribute type (as entry index) to SAX
     * attribute type names.
     */
    private static final String[] attrTypeToNameMap = new String[] {
        "CDATA",        // Attribute.UNDEFINED_ATTRIBUTE, as per SAX 2.0 spec.
        "CDATA",        // Attribute.CDATA_ATTRIBUTE
        "ID",           // Attribute.ID_ATTRIBUTE
        "IDREF",        // Attribute.IDREF_ATTRIBUTE
        "IDREFS",       // Attribute.IDREFS_ATTRIBUTE
        "ENTITY",       // Attribute.ENTITY_ATTRIBUTE
        "ENTITIES",     // Attribute.ENTITIES_ATTRIBUTE
        "NMTOKEN",      // Attribute.NMTOKEN_ATTRIBUTE
        "NMTOKENS",     // Attribute.NMTOKENS_ATTRIBUTE
        "NOTATION",     // Attribute.NOTATION_ATTRIBUTE
        "NMTOKEN",      // Attribute.ENUMERATED_ATTRIBUTE, as per SAX 2.0 spec.
    };

    /**
     * <p>
     * Returns the SAX 2.0 attribute type string from the type of
     * a JDOM Attribute.
     * </p>
     *
     * @param type <code>int</code> the type of the JDOM attribute.
     *
     * @return <code>String</code> the SAX 2.0 attribute type string.
     *
     * @see org.jdom.Attribute#getAttributeType
     * @see org.xml.sax.Attributes#getType
     */
    private String getAttributeTypeName(int type) {
        if ((type < 0) || (type >= attrTypeToNameMap.length)) {
            type = Attribute.UNDECLARED_ATTRIBUTE;
        }
        return attrTypeToNameMap[type];
    }

}
