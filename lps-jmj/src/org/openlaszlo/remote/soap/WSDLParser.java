/* *****************************************************************************
 * WSDLParser.java
 * ****************************************************************************/

/* J_LZ_COPYRIGHT_BEGIN *******************************************************
* Copyright 2001-2004 Laszlo Systems, Inc.  All Rights Reserved.              *
* Use is subject to license terms.                                            *
* J_LZ_COPYRIGHT_END *********************************************************/

package org.openlaszlo.remote.soap;

import java.util.*;
import java.io.*;
import javax.xml.rpc.*;
import javax.xml.parsers.*;
import javax.xml.namespace.*;
import org.w3c.dom.*;
import org.xml.sax.*;
import org.apache.axis.Constants;
import org.apache.axis.utils.*;
import org.apache.log4j.Logger;


/**
 * WSDL parser to obtain a SOAP service object.
 */
public class WSDLParser
{
    private static Logger mLogger = Logger.getLogger(WSDLParser.class);

    public String mTargetNamespace;
    public String mNamespaceURI_WSDL;
    public String mNamespaceURI_WSDL_SOAP;
    public String mNamespaceURI_SOAP_ENC;
    public String mNamespaceURI_SCHEMA_XSD;

    public QName mQNameOperationInput;
    public QName mQNameOperationOutput;

    public static final String[] URIS_SOAP_HTTP = {
        Constants.URI_SOAP11_HTTP,
        Constants.URI_SOAP12_HTTP
    };

    /** Static document builder. */
    static DocumentBuilder mBuilder = null;

    /** WSDL root element. */
    Element mDefinitions = null;

    //------------------------------------------------------------
    // create namespace aware builder
    //------------------------------------------------------------
    {
        DocumentBuilderFactory factory = DocumentBuilderFactory.newInstance();
        factory.setNamespaceAware(true);
        try {
            mBuilder = factory.newDocumentBuilder();
        } catch (ParserConfigurationException e) {
            mLogger.error("Can't create DocumentBuilder");
        }
    }


    /**
     * Protected constructor. Entry point is static parse() method.
     */
    private WSDLParser() { }


    /**
     * Entry point method.
     *
     * @param wsdl WSDL URL
     * @param is
     * @param serviceName can be null.
     * @param servicePort can be null.
     * @return LZSOAPService object.
     * @exception WSDLException if there was a problem with the WSDL.
     * @exception ServiceException if there was a problem creating LZSOAPService.
     */
    static public LZSOAPService parse(String wsdl, InputSource is, String serviceName, 
                                      String servicePort) 
        throws WSDLException, ServiceException {
        WSDLParser parser = new WSDLParser();
        return parser.parseWSDL(wsdl, is, serviceName, servicePort);
    }

    /**
     * Entry point method. Same as calling parse(is, null, null). 
     *
     * @param wsdl WSDL URL
     * @param is
     * @return LZSOAPService object.
     * @exception WSDLException if there was a problem with the WSDL.
     * @exception ServiceException if there was a problem creating LZSOAPService.
     */
    static public LZSOAPService parse(String wsdl, InputSource is) 
        throws WSDLException, ServiceException {
        return parse(wsdl, is, null, null);
    }


    /**
     * Set namespaces:
     *
     * mNamespaceURI_WSDL, mNamespaceURI_WSDL_SOAP, mNamespaceURI_SCHEMA_XSD,
     * mQNameOperationInput, mQNameOperationOutput
     */
    void setNamespaces() {
        mTargetNamespace = mDefinitions.getAttribute("targetNamespace");

        NamedNodeMap attrs = mDefinitions.getAttributes();
        for (int i=0; i < attrs.getLength(); i++) {
            String attr = ((Attr)attrs.item(i)).getValue();
            if (Constants.isWSDL(attr)) {
                mNamespaceURI_WSDL = attr;
            } else if (Constants.isSchemaXSD(attr)) {
                mNamespaceURI_SCHEMA_XSD = attr;
            } else if (Constants.isWSDLSOAP(attr)) {
                mNamespaceURI_WSDL_SOAP = attr;
            } else if (Constants.isSOAP_ENC(attr)) {
                mNamespaceURI_SOAP_ENC = attr;
            }
        }

        mQNameOperationInput  = new QName(mNamespaceURI_WSDL, "input");
        mQNameOperationOutput = new QName(mNamespaceURI_WSDL, "output");

        if (mLogger.isDebugEnabled()) {
            mLogger.debug("WSDL: URI_WSDL " + mNamespaceURI_WSDL);
            mLogger.debug("WSDL: URI_SCHEMA_XSD " + mNamespaceURI_SCHEMA_XSD);
            mLogger.debug("WSDL: URI_WSDL_SOAP " + mNamespaceURI_WSDL_SOAP);
            mLogger.debug("WSDL: URI_SOAP_ENC " + mNamespaceURI_SOAP_ENC);
            mLogger.debug("WSDL: targetnamespace " + mTargetNamespace);
        }
    }


    /**
     * Parse WDSL file
     *
     * @param wsdl WSDL URL
     * @param is WSDL input source.
     * @param serviceName name of service. If null, use first service element encountered.
     * @param servicePort name of port. if null, use first SOAP port element
     * encountered..
     * @return LZSOAPService object.
     * @exception WSDLException if there was a problem with the WSDL.
     * @exception ServiceException if there was a problem creating LZSOAPService.
     */
    LZSOAPService parseWSDL(String wsdl, InputSource is, String serviceName, 
                            String servicePort) 
        throws WSDLException, ServiceException {

        LZSOAPService soapService = null;

        try {
            mDefinitions = mBuilder.parse(is).getDocumentElement();
        } catch (IOException e) {
            throw new WSDLException("IOException: " + e.getMessage());
        } catch (SAXException e) {
            throw new WSDLException("SAXException: " + e.getMessage());
        }


        setNamespaces();

        Element service;
        if (serviceName == null)
            service = 
                LZSOAPUtils.getFirstElementByTagNameNS(mNamespaceURI_WSDL, 
                                                       mDefinitions, "service");
        else
            service = findServiceElement(serviceName);

        if (service == null) {
            if (serviceName != null)
                throw new WSDLException("no service named " + serviceName +
                                        " was found.");
            else
                throw new WSDLException("no service was found");
        }


        serviceName = service.getAttribute("name");
        if (serviceName.equals("")) {
            throw new WSDLException("service has no name");
        }


        NodeList portList = service.getElementsByTagNameNS(mNamespaceURI_WSDL, "port");
        int len = portList.getLength();
        if (len == 0) 
            throw new WSDLException("No SOAP ports found for service " +
                                    serviceName);

        for (int i=0; i < len; i++) {
            Element port = (Element)portList.item(i);

            String portName = port.getAttribute("name");
            if ( portName == null ) {
                mLogger.warn("encountered port with no name");
                continue;
            }

            if (servicePort != null && ! servicePort.equals(portName)) {
                continue;
            }

            if (doesPortSupportSOAP(port)) {

                Element binding = getBindingElement(port);
                mLogger.debug("binding: " +  binding);

                Element soapBinding = 
                    LZSOAPUtils.getFirstElementByTagNameNS
                    (mNamespaceURI_WSDL_SOAP, binding, "binding");

                if (soapBinding == null) {
                    throw new WSDLException("no SOAP binding for port " +
                                            portName);
                }

                // we only support SOAP over HTTP
                String transport = soapBinding.getAttribute("transport");
                if ( ! isSOAPHTTPTransport(transport) ) {
                    if ( servicePort != null && servicePort.equals(portName)) {
                        throw new WSDLException("port " + servicePort +
                                                " does not have a valid SOAP transport");
                    } else {
                        continue;
                    }
                }

                soapService = new LZSOAPService(wsdl, service.getAttribute("name"),
                                                portName, getEndpointAddress(port),
                                                transport, mTargetNamespace,
                                                mNamespaceURI_SCHEMA_XSD, 
                                                mNamespaceURI_SOAP_ENC);

                String defaultStyle = soapBinding.getAttribute("style");
                if ( "".equals(defaultStyle) ) defaultStyle = "document";

                parseOperations(soapService, binding, defaultStyle);
                break;
            } else if (servicePort != null && servicePort.equals(portName)) {
                throw new WSDLException("port " + servicePort +
                                        " does not support SOAP");
            }
        }

        if (soapService == null) {
            throw new WSDLException("could not find requested SOAP service "
                                    + "(service: " + serviceName
                                    + ", port: " + servicePort
                                    + ")");
        }

        SchemaParser sp = null;
        Map complexTypeMap = new HashMap();
        try {
            NodeList schemalist = getSchema();
            if (schemalist != null) {
                for (int i=0; i < schemalist.getLength(); i++) {
                    Element schema = (Element)schemalist.item(i);
                    sp = new SchemaParser(this, schema);
                    sp.parse(complexTypeMap);
                }
            }
        } catch (Exception e) {
            mLogger.warn("SchemaParser", e);
        }

        soapService.setSchemaComplexTypes(complexTypeMap);
        soapService.createClientService();

//         if (mLogger.isDebugEnabled()) {
//             mLogger.debug("\n========== SOAP SERVICE ==========\n\n" +
//                           soapService);
//         }

        return soapService;
    }


    /**
     * Get the namespace for the schema.
     *
     * @return target namespace for schema.
     */
    NodeList getSchema() {
        return mDefinitions.getElementsByTagNameNS(mNamespaceURI_SCHEMA_XSD, 
                                                  "schema");
    }

    /**
     * Parse SOAP operations and add them to service object.
     *
     * @param service SOAP service object to add operations.
     * @param binding WSDL SOAP binding element.
     */
    void parseOperations(LZSOAPService service, Element binding, 
                         String defaultStyle) throws WSDLException {

        String bindingType = binding.getAttribute("type");
        if ("".equals(bindingType)) {
            throw new WSDLException("binding does not have a type attribute");
        }
        QName bindingTypeQName = XMLUtils.getQNameFromString(bindingType, binding);

        // binding maps to portType
        Element portType = findPortTypeElement(bindingTypeQName.getLocalPart());
        if (portType == null) {
            throw new WSDLException("could not find portType named " +
                                    bindingTypeQName);
        }

        // list of operations for portType
        NodeList portTypeOpList = 
            portType.getElementsByTagNameNS(mNamespaceURI_WSDL, "operation");
        if (portTypeOpList.getLength() == 0) {
            throw new WSDLException("portType named " + 
                                    bindingTypeQName.getLocalPart() +
                                    " has no operations"); 
        }

        Map operationMap = new HashMap();
        service.setOperations(operationMap);

        NodeList list = binding.getElementsByTagNameNS(mNamespaceURI_WSDL, "operation");
        for (int i=0; i < list.getLength(); i++) {
            Element operation = (Element)list.item(i);
            String opName = operation.getAttribute("name");
            if ( "".equals(opName) ) {
                mLogger.warn("name not found for an operation element");
                continue;
            }

            // From WSDL spec: For the HTTP protocol binding of SOAP,
            // [soapAction] is value required (it has no default value).
            //
            // <operation>
            //     <soap:operation soapAction="..." style="[rpc|document]"? />
            //     ...
            // </operation>
            Element soapOperation = 
                LZSOAPUtils.getFirstElementByTagNameNS(mNamespaceURI_WSDL_SOAP,
                                                       operation, "operation");

            String soapAction = null;
            if (isSOAPHTTPTransport(service.getTransport())) {
                Attr soapActionAttr = soapOperation.getAttributeNode("soapAction");
                if ( soapActionAttr == null ) {
                    mLogger.warn("required soapAction attribute not found for <soap:operation>");
                    continue;
                }
                soapAction = soapActionAttr.getValue();
            }

            String style = soapOperation.getAttribute("style");
            if ( "".equals(style) )
                style = defaultStyle;

            Element portTypeOp = findPortTypeOperationElement(portTypeOpList, opName);
            if (portTypeOp == null) {
                mLogger.warn("could not find portType operation named " + opName);
                continue;
            }

            // We only support request-response type operations
            if (! isRequestResponseOperation(portTypeOp)) {
                mLogger.warn("WARNING: portType operation named " + opName + 
                             " is not a supported operation." +
                             " Only request-response operations are supported.");
                continue;
            }

            LZSOAPMessage inputBody;
            LZSOAPMessage outputBody;
            try { 
                inputBody  = getMessage(operation, portTypeOp, "input", "Request");
                outputBody = getMessage(operation, portTypeOp, "output", "Response");
            } catch (WSDLException e) {
                mLogger.warn(e.getMessage());
                continue;
            }

            LZSOAPOperation op = new LZSOAPOperation(opName);
            op.setSoapAction(soapAction);
            op.setStyle(style);
            op.setInputMessage(inputBody);
            op.setOutputMessage(outputBody);
            op.setMangledName(opName + "_" + inputBody.getName() + "_" + outputBody.getName()); 
            op.setIsDocumentLiteralWrapped(isDocumentLiteralWrapped(op));

            if (operationMap.containsKey(op.getMangledName())) {
                mLogger.warn("operation named " + op.getMangledName() + " is not unique");
                continue;
            }

            // if already exists, we have overloaded method
            if (operationMap.containsKey(op.getName())) {

                // make sure previous operation wouldn't mangle to the same
                // mangled name as the current one
                LZSOAPOperation prevOp = (LZSOAPOperation)operationMap.get(op.getName());
                if ( prevOp.getMangledName().equals(op.getMangledName()) ) {
                    mLogger.warn("operation named" + op.getMangledName() +
                                 " is not unique");
                    continue;
                }

                operationMap.put(op.getMangledName(), op);

            } else {
                operationMap.put(op.getName(), op);
            }

        }
    }


    /**
     * Check to see if operation is a document/literal wrapped operation.
     *
     * @param op check to see if operation is document/literal wrapped.
     * @return true if it's a document/literal wrapped operation, else false.
     */
    boolean isDocumentLiteralWrapped(LZSOAPOperation op) {

        // make sure it's document literal
        if (! op.getStyle().equals("document")) {
            return false;
        }

        LZSOAPMessage inputMesg = op.getInputMessage();
        if (! inputMesg.getUse().equals("literal")) {
            return false;
        }
            
        // only one part can exist 
        List parts = inputMesg.getParts();
        if (parts == null || parts.size() != 1) {
            return false;
        }

        // element has to exist
        LZSOAPPart part = (LZSOAPPart)parts.get(0);
        String eName = part.getElement();
        if ( eName == null ) {
            return false;
        }

        // maybe I should be getting qname from schema element?
        QName eQName = XMLUtils.getQNameFromString(eName, mDefinitions);
        Element element = findSchemaElement(eQName.getLocalPart());
        if (element == null) {
            return false;
        }

        // name of element has to match name of operation
        String name = element.getAttribute("name");
        if (! name.equals(op.getName())) {
            return false;
        }

        // there can't be attributes on this element
        NodeList attrList = 
            element.getElementsByTagNameNS(mNamespaceURI_SCHEMA_XSD,
                                           "attribute");
        if (attrList.getLength() != 0) {
            return false;
        }

        return true;
    }

    /**
     * Find service element.
     *
     * @param name name of service element to find.
     * @return found service element, else null.
     */
    Element findServiceElement(String name) {
        return findElementByName(mNamespaceURI_WSDL, mDefinitions,
                                 "service", name);
    }


    /**
     * Find schema element.
     *
     * @param name name of schema element to find.
     * @return found schema element, else null.
     */
    Element findSchemaElement(String name) {
        return findElementByName(mNamespaceURI_SCHEMA_XSD, mDefinitions,
                                 "element", name);
    }

    /**
     * Find message element.
     *
     * @param name name of message to find.
     * @return found message element, else null.
     */
    Element findMessageElement(String name) {
        return findElementByName(mNamespaceURI_WSDL, mDefinitions, "message", name);
    }

    /**
     * Find portType element.
     *
     * @param name name of portType element to find.
     * @return matched portType element or null, if none found. 
     */
    Element findPortTypeElement(String name) {
        return findElementByName(mNamespaceURI_WSDL, mDefinitions, "portType", name);
    }

    /**
     * Find a particular portType operation element based on name.
     *
     * @param portTypeOpList node list of portType operations.
     * @param name operation name to find.
     * @return portType operation element if found, else null.
     */
    Element findPortTypeOperationElement(NodeList portTypeOpList, String name) {
        return findElementByName(portTypeOpList, name);
    }

    /**
     * Find element by name attribute.
     *
     * @param ns namespace.
     * @param owner owner element.
     * @param tag tag to search on.
     * @param name name to match.
     * @return found element.
     */
    Element findElementByName(String ns, Element owner, String tag, String name) {
        NodeList list = owner.getElementsByTagNameNS(ns, tag);
        return findElementByName(list, name);
    }

    /**
     * Find element by name attribute.
     *
     * @param list node list.
     * @param name name value to match.
     * @return found element.
     */
    Element findElementByName(NodeList list, String name) {
        for (int i=0; i < list.getLength(); i++) {
            Element e = (Element)list.item(i);
            if ( name.equals(e.getAttribute("name")) )
                return e;
        }
        return null;
    }

    /**
     * Parse message for input, output from portType operation.
     *
     * @param bindOp operation element in binding
     * @param portTypeOp operation element in portType.
     * @param tag one of input, output.
     * @param appendName string to append to create implicit operation type name
     * if element has not explicitly declared it.
     * @return a soap message object.
     */
    LZSOAPMessage getMessage(Element bindOp, Element portTypeOp, 
                             String tag, String appendName) 
        throws WSDLException {

        Element opType = 
            LZSOAPUtils.getFirstElementByTagNameNS(mNamespaceURI_WSDL,
                                                   portTypeOp, tag);
        String opName = portTypeOp.getAttribute("name");

        String name = opType.getAttribute("name");
        if (name.equals("")) {
            name = opName + appendName;
        }

        LZSOAPMessage message = new LZSOAPMessage(name, tag);

        Element bindTag = 
            LZSOAPUtils.getFirstElementByTagNameNS(mNamespaceURI_WSDL, 
                                                   bindOp, tag);
        if (bindTag == null) {
            throw new WSDLException("could not find " + tag +
                                    " element for bind operation " + opName);
        }

        // only parse <soap:body> element for now
        Element soapBody = 
            LZSOAPUtils.getFirstElementByTagNameNS(mNamespaceURI_WSDL_SOAP,
                                                   bindTag, "body");

        String use = soapBody.getAttribute("use");
        if (use.equals("")) {
            throw new WSDLException("Attribute use is not defined for " + tag +
                                    " <soap:body> in operation " + opName);
        }
        message.setUse(use);

        String parts = soapBody.getAttribute("parts");
        if (! parts.equals("")) {
            message.setPartNames(getPartSet(parts));
        }

        String mesgName = opType.getAttribute("message");
        QName mesgQName = XMLUtils.getQNameFromString(mesgName, opType);
        Element mesgElement = findMessageElement(mesgQName.getLocalPart());
        if (mesgElement == null) {
            throw new WSDLException("could not find message element named " +
                                    mesgQName.getLocalPart());
        }

        
        bindMessage(message, mesgElement, tag);

        return message;
    }

    
    /**
     * @param soapMessage 
     * @param mesgElement
     * @param tag one of input or output
     */
    void bindMessage(LZSOAPMessage soapMessage, Element mesgElement, String tag) 
        throws WSDLException {

        List parts = new Vector();
        Set partNames = soapMessage.getPartNames();

        // go through mesgElement and add part information to soapMessage
        NodeList list = mesgElement.getElementsByTagNameNS(mNamespaceURI_WSDL, "part");
        for (int i=0; i < list.getLength(); i++) {
            Element part = (Element)list.item(i);
            Attr nameAttr = part.getAttributeNode("name");
            if (nameAttr == null) {
                throw new WSDLException("part for message named " + 
                                        mesgElement.getAttribute("name") +
                                        " does not have required name attribute");
            }
            String name = nameAttr.getValue();

            // only set part if message should contain it
            if (partNames != null && ! partNames.contains(name)) continue;

            LZSOAPPart p = new LZSOAPPart(name);

            ParameterMode mode;
            if (tag.equals("output")) {
                mode = ParameterMode.OUT;
            } else {
                mode = ParameterMode.IN;
            }
            p.setParameterMode(mode);

            Attr elementAttr = part.getAttributeNode("element");
            Attr typeAttr = part.getAttributeNode("type");

            if (elementAttr != null) {
                p.setElement(elementAttr.getValue());
            }

            if (typeAttr != null) {
                p.setType(XMLUtils.getQNameFromString(typeAttr.getValue(), 
                                                      part));
            }

            parts.add(p);
        }

        soapMessage.setParts(parts);
    }


    /**
     * Get list of message parts.
     *
     * @param nmtokens name tokens.
     * @return set part names.
     */
    Set getPartSet(String nmtokens) {
        Set set = null;
        StringTokenizer st = new StringTokenizer(nmtokens);
        while (st.hasMoreTokens()) {
            if (set == null) set = new HashSet();
            set.add(st.nextToken());
        }
        return set;
    }


    /**
     * Check to see if portType operation is request-response operation, since
     * we currently have no way of supporting one-way, solicit-response,
     * notification operations.
     * 
     * @param op port Type operation element.
     */
    boolean isRequestResponseOperation(Element op) {
        Node n = op.getFirstChild();
        while (n != null && 
               ( n.getNodeType() != Node.ELEMENT_NODE ||
                 n.getNodeName().equals("documentation")) ) {
            n = n.getNextSibling();
        }

        // make sure we're comparing w/o prefix but with the right namespace
        QName input = XMLUtils.getQNameFromString(n.getNodeName(), op);
        input = new QName(n.getNamespaceURI(), input.getLocalPart());
        if (n == null || ! input.equals(mQNameOperationInput))
            return false;

        n = n.getNextSibling();
        while (n != null && 
               n.getNodeType() != Node.ELEMENT_NODE) {
            n = n.getNextSibling();
        }
        QName output = XMLUtils.getQNameFromString(n.getNodeName(), op);
        output = new QName(n.getNamespaceURI(), output.getLocalPart());
        if (n == null || ! output.equals(mQNameOperationOutput))
            return false;

        return true;
    }


    /**
     * Check to see if SOAP transport for binding is supported. Support only
     * HTTP for now.
     *
     * @param transport transport string.
     * @return true if SOAP transport is supported, else false.
     */
    boolean isSOAPHTTPTransport(String transport) {
        if (transport == null) 
            return false;

        for (int i=0; i<URIS_SOAP_HTTP.length; i++) {
            if (URIS_SOAP_HTTP[i].equals(transport)) {
                return true;
            }
        }
        return false;
    } 

    /**
     * Get the &lt;soap:address .../&gt; location attribute.
     *
     * @param port port element to get endpoint address from.
     * @return endpoint address string.
     */
    String getEndpointAddress(Element port) {
        Element soapAddress = 
            LZSOAPUtils.getFirstElementByTagNameNS(mNamespaceURI_WSDL_SOAP, 
                                                   mDefinitions, "address");
        String name = port.getAttribute("name");
        return soapAddress.getAttribute("location");
    }

    /**
     * Get binding element specified by the port's binding attribute.
     *
     * @param port port element to get binding.
     * @return binding element.
     */
    Element getBindingElement(Element port) {
        QName bindingQName = 
            XMLUtils.getQNameFromString(port.getAttribute("binding"), port);
        return getElementByAttributeNS(mNamespaceURI_WSDL, "binding", "name",
                                       bindingQName);
    }


    /**
     * Get an element based on its attribute name.
     *
     * @param ns namespace.
     * @param elementName name of element to search.
     * @param attrName name of attribute to match.
     * @param attrValue value of attribute to match.
     * @return element or null, if none found with elementName.
     */
    Element getElementByAttributeNS(String ns, String elementName, 
                                    String attrName, QName attrValue) {
        NodeList list = mDefinitions.getElementsByTagNameNS(ns, elementName);

        int length = list.getLength();
        if (length == 0) return null;

        for (int i=0; i < length; i++) {
            Element element = (Element)list.item(i);
            String searchAttr = element.getAttribute(attrName);
            if (searchAttr.equals(attrValue.getLocalPart())) {
                return element;
            }
        }

        return null;
    }


    /**
     * Check to see if port supports SOAP.
     *
     * @param port port to check SOAP support.
     * @return true if port supports SOAP, else false.
     */
    boolean doesPortSupportSOAP(Element port) {
        return LZSOAPUtils.getFirstElementByTagNameNS(mNamespaceURI_WSDL_SOAP, 
                                                      port, "address") != null;
    }

    static public void main(String[] args) {

        if (1 < args.length && args.length > 3) {
            System.err.println("Usage: WSDLParser <wsdl> [<service> <port>]");
            return;
        }

        String service = null; 
        String port = null;

        if (args.length == 2 || args.length == 3) service = args[1];
        if (args.length == 3) port = args[2];

        try {
            InputSource is = new InputSource(new FileReader(args[0]));
            System.out.println(WSDLParser.parse(args[0], is, service, port));
        } catch (IOException e) {
            e.printStackTrace();
        } catch (ServiceException e) {
            System.err.println("ERROR: " + e.getMessage());
        } catch (WSDLException e) {
            System.err.println("ERROR: " + e.getMessage());
        }
    }
}
