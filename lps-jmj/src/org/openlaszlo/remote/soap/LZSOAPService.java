/* *****************************************************************************
 * LZSOAPService.java
 * ****************************************************************************/

/* J_LZ_COPYRIGHT_BEGIN *******************************************************
* Copyright 2001-2004 Laszlo Systems, Inc.  All Rights Reserved.              *
* Use is subject to license terms.                                            *
* J_LZ_COPYRIGHT_END *********************************************************/

package org.openlaszlo.remote.soap;

import org.openlaszlo.iv.flash.api.action.Program;
import org.openlaszlo.remote.soap.encoding.SWFArrayDeserializerFactory;
import org.openlaszlo.remote.soap.encoding.SWFSimpleDeserializerFactory;
import org.openlaszlo.remote.soap.encoding.SWFObjectDeserializerFactory;
import org.openlaszlo.remote.soap.encoding.LZObjectSerializerFactory;
import java.io.IOException;
import java.net.URLDecoder;
import java.rmi.RemoteException;
import java.util.Iterator;
import java.util.List;
import java.util.ArrayList;
import java.util.Map;
import javax.xml.namespace.QName;
import javax.xml.rpc.JAXRPCException;
import javax.xml.soap.SOAPHeader;
import org.apache.axis.client.Service;
import org.apache.axis.client.Call;
import javax.xml.rpc.ServiceException;
import javax.xml.rpc.encoding.DeserializerFactory;
import javax.xml.rpc.encoding.SerializerFactory;
import javax.xml.rpc.encoding.TypeMappingRegistry;
import javax.xml.rpc.encoding.TypeMapping;
import javax.xml.rpc.handler.HandlerInfo;
import javax.xml.rpc.handler.HandlerRegistry;
import javax.xml.soap.SOAPException;
import org.apache.axis.Constants;
import org.apache.axis.message.SOAPBodyElement;
import org.apache.axis.message.SOAPHeaderElement;
import org.apache.log4j.Logger;
import org.w3c.dom.DOMException;
import org.w3c.dom.Element;
import org.w3c.dom.Node;
import org.w3c.dom.NodeList;
import org.w3c.dom.Text;
import org.xml.sax.SAXException;


import java.util.ArrayList;

public class LZSOAPService
{
    private static Logger mLogger = Logger.getLogger(LZSOAPService.class);

    String mServiceName;
    String mPort;
    String mEndpointAddress;
    String mTransport;
    String mTargetNS;
    String mXMLSchemaNS;
    String mSOAPEncodingNS;
    Service mService;
    TypeMappingRegistry mTypeMappingRegistry;
    HandlerRegistry mHandlerRegistry;
    TypeMapping mDefaultTypeMapping;
    TypeMapping mDefaultSOAPEncodingTypeMapping;

    /** URL for wsdl. This gets set in SOAPDataSource. */
    String mWSDL;

    /** SWF of service */
    byte[] mClientSOAPService = null;

    /** Map of LZSOAPOperations. */
    Map mOperations = null;

    /** Map of schema complex types. */
    Map mSchemaComplexTypes = null;



    /** Keep one DeserializerFactory around. */
    DeserializerFactory mSWFObjectDeserializerFactory = 
        new SWFObjectDeserializerFactory();

    SerializerFactory mObjectSerializerFactory = new LZObjectSerializerFactory();

    public LZSOAPService(String wsdl, String serviceName, String port, 
                         String endpointAddress, String transport,
                         String targetNS, String xmlSchemaNS,
                         String soapEncodingNS)
        throws ServiceException {

        mWSDL = wsdl;
        mServiceName = serviceName;
        mPort = port;
        mEndpointAddress = endpointAddress;
        mTransport = transport;
        mTargetNS = targetNS;
        mXMLSchemaNS = xmlSchemaNS;
        mSOAPEncodingNS = soapEncodingNS;

        mService = new Service(new QName(mTargetNS, mServiceName));

        mTypeMappingRegistry = mService.getTypeMappingRegistry();
        mHandlerRegistry = mService.getHandlerRegistry();

        getHandlerChain().add(new HandlerInfo(LZSOAPHandler.class, null, null));

        // Register default type mapping and default soap encoding type mapping.
        mDefaultTypeMapping = LZDefaultTypeMapping.getSingleton();
        mDefaultSOAPEncodingTypeMapping = new LZDefaultSOAPEncodingTypeMapping();
        mTypeMappingRegistry.registerDefault(mDefaultTypeMapping);
        mTypeMappingRegistry.register(Constants.URI_SOAP11_ENC, mDefaultSOAPEncodingTypeMapping);
    }


    /**
     * This should be called after LZSOAPService is completely created and
     * initialized.
     */
    public void createClientService() throws ServiceException {
        if (mClientSOAPService != null)
            throw new ServiceException("client SOAP service already created");

        try {
            mClientSOAPService = ClientSOAPService.createObject(this);
        } catch (IOException e) {
            throw new ServiceException("could not create client SOAP service object"); 
        }
    }

    /**
     * Get client SWF representation of service.
     */
    public byte[] getClientSOAPService() {
        return mClientSOAPService;
    }

    /**
     * Invoke operation with parameters. Parameters are represented in XML like:
     *
     *     <params>
     *         <param>param1</param>
     *         <param>param2</param>
     *         <param>param3</param>
     *     <params>
     *
     * In document style, the string in the <param> element should be an
     * XML-escaped string. For example, suppose you were trying to send two
     * documents that looked like:
     *
     *     doc1: <a>1</a>
     *     doc2: <b>2</b>
     *
     * The XML parameter string should look as follows:
     *
     *    <params>
     *        <param>%3Ca%3E1%3C/a%3E</param>
     *        <param>%3Cb%3E2%3C/b%3E</param>
     *    </params>
     *
     * @param operation operation to invoke
     * @param xml XML from client that includes header and body. The format looks like:
     *
     *     <e><h>XML_HEADER</h><b>XML_BODY</b></e>
     *
     * where XML_BODY is
     *
     *     <params>
     *         <param>PARAM1</param>
     *         <param>PARAM2</param>
     *         <param>...</param>
     *     </params>
     *
     * @return object array where the first parameter is a string indicating the
     * style used to invoke the function (rpc|document) and the second is the
     * return value. For document styles, an array of SOAPBody message items are
     * returned.
     */
    public Object[] invoke(String operation, String xml) 
        throws ServiceException, RemoteException {

        mLogger.debug("invoke()");

        Call call = createCall(operation);

        LZSOAPOperation op = (LZSOAPOperation)mOperations.get(operation);
        List parts = op.getInputMessage().getParts();

        String style = op.getStyle();

        try {

            Element envelope = LZSOAPUtils.xmlStringToElement(xml);
            Element header = LZSOAPUtils.getFirstElementByTagName(envelope, "h");
            NodeList headerNodes = header.getChildNodes();
            for (int i=0; i < headerNodes.getLength(); i++) {
                Node headerNode = (Node)headerNodes.item(i);
                if (headerNode.getNodeType() != Node.ELEMENT_NODE) continue;
                call.addHeader(new SOAPHeaderElement((Element)headerNode));
            }

            Element body = LZSOAPUtils.getFirstElementByTagName(envelope, "b");
            Element paramsEl = LZSOAPUtils.getFirstElementByTagName(body, "params");
            Object[] params = extractParams(paramsEl, style, parts);

            // Pass back style and return value from call
            Object returnValue = call.invoke(params);

            // Return header, if any
            SOAPHeader responseHeader = 
                call.getResponseMessage().getSOAPEnvelope().getHeader();

            return new Object[]{ style, returnValue, responseHeader };

        } catch (IOException e) {
            mLogger.error("IOException");
            throw new ServiceException(e.getMessage());
        } catch (org.xml.sax.SAXException e) {
            mLogger.error("SAXException:", e);
            throw new ServiceException(e.getMessage());
        } catch (SOAPException e) {
            mLogger.error("SOAPException", e);
            throw new ServiceException(e.getMessage());
        }

    }

    /**
     * Extract parameters from XML.
     */
    Object[] extractParams(Element paramsEl, String style, List parts) 
        throws ServiceException {

        try {
            // FIXME: [2004-06-24 pkang] create XML format that we can pass
            // maps/objects, lists/arrays

            int pos = 0;        // parameter position
            List params = new ArrayList();
            NodeList list = paramsEl.getChildNodes();
            for (int i=0; i < list.getLength(); i++) {
                Node node = (Node)list.item(i);
                if (node.getNodeType() != Node.ELEMENT_NODE) continue;

                Element p = (Element)node;
                if (style.equals("rpc")) {
                    Node paramNode = getParamValue(p);
                    if (paramNode == null || paramNode.getNodeType() == Node.TEXT_NODE) {
                        Text text = (Text)paramNode;
                        String strValue = (text != null ? text.getData() : "");
                        params.add(((LZSOAPPart)parts.get(pos)).valueOf(strValue));

                        if (mLogger.isDebugEnabled()) {
                            mLogger.debug("primitive param: " + strValue);
                        }

                    } else if (paramNode.getNodeType() == Node.ELEMENT_NODE) {
                        // objects are added as elements
                        ObjectWrapper ow = new ObjectWrapper((Element)paramNode);
                        params.add(ow);
                        if (mLogger.isDebugEnabled()) {
                            mLogger.debug("object param: " + ow);
                        }
                    } else {
                        throw new ServiceException("bad paramater: " + paramNode);
                    }
                } else {
                    // get the XML string and convert to an element
                    Text text = (Text)p.getFirstChild();
                    String data = URLDecoder.decode(text.getData());
                    Element docElement = LZSOAPUtils.xmlStringToElement(data);
                    params.add(new SOAPBodyElement(docElement));
                }

                pos++;
            }

            if (params.size() != parts.size()) {
                throw new ServiceException("number of params does not match");
            }

            return params.toArray();
        } catch (IOException e) {
            mLogger.error("IOException", e);
            throw new ServiceException("IOException: " + e.getMessage());
        } catch (SAXException e) {
            mLogger.error("SAXException", e);
            throw new ServiceException("SAXException: " + e.getMessage());
        } catch (DOMException e) {
            mLogger.error("DOMException", e);
            throw new ServiceException("DOMException: " + e.getMessage());
        }
    }

    Node getParamValue(Element param) {
        NodeList list = param.getChildNodes();
        int len = list.getLength();
        if (len == 0) {
            return null;
        } 

        // if a subelement exists, the param must be an array or object.
        for (int i=0; i < list.getLength(); i++) {
            Node node = list.item(i);
            if (node.getNodeType() == Node.ELEMENT_NODE) {
                return param;
            }
        }

        return list.item(0);
    }


    /**
     * @param operation name of operation.
     */
    public Call createCall(String operation) throws ServiceException {
        LZSOAPOperation op = (LZSOAPOperation)mOperations.get(operation);
        if (op == null) {
            throw new ServiceException("could not find operation named " + 
                                       operation);
        }

        Call call = (org.apache.axis.client.Call)
            mService.createCall(new QName(mTargetNS, mPort));
        call.setOperationName(new QName(mTargetNS, op.getName()));
        call.setTargetEndpointAddress(mEndpointAddress);

        if (op.getStyle().equals("document")) {

            call.setProperty(Call.OPERATION_STYLE_PROPERTY, "document");
            call.setProperty(Call.SOAPACTION_USE_PROPERTY, new Boolean(true));
            call.setProperty(Call.SOAPACTION_URI_PROPERTY, op.getSoapAction());

        } else {
            /* rpc */

            // input message is not going to be null because we only support
            // request-response SOAP transactions
            List parts = op.getInputMessage().getParts();
            for (int i=0; i < parts.size(); i++) {
                LZSOAPPart part = (LZSOAPPart)parts.get(i);
                call.addParameter(part.getName(), part.getType(),
                                  part.getParameterMode());
            }

            parts = op.getOutputMessage().getParts();
            if (parts.size() == 1) {
                LZSOAPPart part = (LZSOAPPart)parts.get(0);
                QName type = part.getType();
                if (type != null) {
                    call.setReturnType(type);
                } else {
                    mLogger.warn("type of return unspecified"); 
                }

            } else {
                mLogger.warn("don't support more than one return value");
            }
        }

        return call;
    }

    public TypeMappingRegistry getTypeMappingRegistry() {
        return mTypeMappingRegistry;
    }

    public HandlerRegistry getHandlerRegistry() {
        return mHandlerRegistry;
    }

    public List getHandlerChain() {
        return mHandlerRegistry.getHandlerChain(new QName(mTargetNS, mPort));
    }

    public Service getService() {
        return mService;
    }

    /** Set map of LZSOAPOperations. */
    public void setOperations(Map operations) {
        mOperations = operations;
    }

    /** @return map of LZSOAPOperations. */
    public Map getOperations() {
        return mOperations;
    }

    public String getTargetNS() {
        return mTargetNS;
    }

    public String getSOAPEncodingNS() {
        return mSOAPEncodingNS;
    }

    public String getServiceName() {
        return mServiceName;
    }

    public String getPort() {
        return mPort;
    }

    public String getEndpointAddress() {
        return mEndpointAddress;
    }

    public String getTransport() {
        return mTransport;
    }

    /** WSDL URL for this SOAP service. */
    public String getWSDL() {
        return mWSDL;
    }

    /** Set WSDL URL for this SOAP service. */
    public void setWSDL(String wsdl) {
        mWSDL = wsdl;
    }

    public String toOperationString() {
        StringBuffer buf = new StringBuffer(); 
        Iterator iter = mOperations.keySet().iterator();
        while (iter.hasNext()) {
            String key = (String)iter.next();
            LZSOAPOperation op = (LZSOAPOperation)mOperations.get(key);
            buf.append(op).append("\n");
        }
        return buf.toString();
    }

    /**
     * This gets called by WSDLParser after WSDL schema is read. 
     */
    public void setSchemaComplexTypes(Map schemaComplexTypes) {
        mSchemaComplexTypes = schemaComplexTypes;

        // now register these complex types with type mapper.
        if (mSchemaComplexTypes != null) {
            Iterator iter = mSchemaComplexTypes.values().iterator();
            while (iter.hasNext()) {
                ComplexType value = (ComplexType)iter.next();
                if ( value.getType() == ComplexType.TYPE_STRUCT ) {
                    QName structQName = value.getName();
mLogger.info("AAAAAAAAA" + structQName);

                    // Just to be safe, registering in both default type mapping
                    // and SOAP type mapping.
                    mDefaultTypeMapping.register(ObjectWrapper.class, structQName,
                                                 mObjectSerializerFactory, 
                                                 mSWFObjectDeserializerFactory);
                    mDefaultSOAPEncodingTypeMapping.register(ObjectWrapper.class, structQName,
                                                             mObjectSerializerFactory, 
                                                             mSWFObjectDeserializerFactory);
                    if (mLogger.isDebugEnabled()) {
                        mLogger.debug("registered type mapping: " + structQName);
                    }
                }
            }
        }
    }

    /**
     * @return map of ComplexType.
     */
    public Map getSchemaComplexTypes() {
        return mSchemaComplexTypes;
    }

    String stringComplexTypes() {
        if (mSchemaComplexTypes == null) {
            return "";
        }

        StringBuffer buf = new StringBuffer();
        Iterator iter = mSchemaComplexTypes.entrySet().iterator();
        while (iter.hasNext()) {
            Map.Entry entry = (Map.Entry)iter.next();
            ComplexType ct =(ComplexType)entry.getValue();
            buf.append(ct).append("\n");
        }
        return buf.toString();
    }

    public String toString() {
        return "-- SOAPService ------------------------------\n"
            + "service="  + mServiceName + "\n"
            + "port="     + mPort + "\n"
            + "endpoint=" + mEndpointAddress + "\n"
            + "transport=" + mTransport + "\n"
            + "target namespace=" + mTargetNS + "\n\n"
            + "    ==== OPERATIONS ====\n\n"
            + toOperationString()
            + "    ==== SCHEMA COMPLEX TYPES ====\n\n"
            + stringComplexTypes()
            + "---------------------------------------------";
    }
}
