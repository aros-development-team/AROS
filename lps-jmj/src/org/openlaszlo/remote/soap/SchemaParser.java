/* *****************************************************************************
 * SchemaParser.java
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

import org.apache.log4j.Logger;

import org.apache.axis.utils.*;


public class SchemaParser
{
    public static Logger mLogger = Logger.getLogger(SchemaParser.class);

    static final int ARRAY = 0;
    static final int REGULAR_OBJECT = 1;
    static final int SEQUENCED_OBJECT = 2;

    /** don't support for now 
    static final int EXTENDED_OBJECT = 3; // object with <xsd:extention>
    **/

    Map mComplexTypeMap;

    // Values which are set when WSDLParser is passed in.
    String mNamespaceURI_SOAP_ENC;
    String mNamespaceURI_SCHEMA_XSD;
    String mNamespaceURI_WSDL;
    String mTargetNamespaceURI;

    QName mSOAPEncodingArray;
    QName mSOAPEncodingArrayType;

    Element mSchema;

    /**
     * @param wp parsed WSDLParser
     * @param schema element representing schema element
     */
    public SchemaParser(WSDLParser wp, Element schema) {

        mNamespaceURI_SCHEMA_XSD = wp.mNamespaceURI_SCHEMA_XSD;
        mNamespaceURI_SOAP_ENC = wp.mNamespaceURI_SOAP_ENC;
        mNamespaceURI_WSDL = wp.mNamespaceURI_WSDL;

        mSOAPEncodingArray = new QName(mNamespaceURI_SOAP_ENC, "Array");
        mSOAPEncodingArrayType = new QName(mNamespaceURI_SOAP_ENC, "arrayType");

        // set the target element here.
        mTargetNamespaceURI = schema.getAttribute("targetNamespace");

        mSchema = schema;

        if (mLogger.isDebugEnabled()) {
            mLogger.debug("mNamespaceURI_SCHEMA_XSD: " + mNamespaceURI_SCHEMA_XSD);
            mLogger.debug("mNamespaceURI_SOAP_ENC: " + mNamespaceURI_SOAP_ENC);
            mLogger.debug("mNamespaceURI_WSDL_SOAP: " + mNamespaceURI_WSDL);
            mLogger.debug("mSOAPEncodingArray: " + mSOAPEncodingArray);
            mLogger.debug("mSOAPEncodingArrayType: " + mSOAPEncodingArrayType);
            mLogger.debug("mTargetNamespaceURI: " + mTargetNamespaceURI);
        }
    }

    /**
     * @param complexTypeMap map where ComplexType objects will be stored.
     */
    public void parse(Map complexTypeMap) { 

        // save it so we can use it for checkExtension
        mComplexTypeMap = complexTypeMap;

        // just get the complex types
        NodeList list = mSchema.getElementsByTagNameNS(mNamespaceURI_SCHEMA_XSD, "complexType");

        for (int i=0; i < list.getLength(); i++) {
            try {
                ComplexType so = getComplexType((Element)list.item(i));
                if (so != null) {
                    complexTypeMap.put(so.getName(), so);
                } else {
                    mLogger.warn("skipping: " + list.item(i));
                }
            } catch (Exception e) {
                mLogger.error("skipping complexType: " + e.getMessage(), e);
            }
        }

    }

    ComplexType getComplexType(Element ct) throws Exception {
        String name = ct.getAttribute("name");

        NodeList list;
        list = ct.getElementsByTagNameNS(mNamespaceURI_SCHEMA_XSD, "complexContent");
        if (foundOne(list)) {
            // check for array in elements inside of <complexContent>
            return checkComplexContent(name, (Element)list.item(0));
        }

        list = ct.getElementsByTagNameNS(mNamespaceURI_SCHEMA_XSD, "all");
        if (foundOne(list)) {
            // get values inside <all> element
            return checkAllOrSequence(name, (Element)list.item(0));
        }

        list = ct.getElementsByTagNameNS(mNamespaceURI_SCHEMA_XSD, "sequence");
        if (foundOne(list)) {
            // get values inside <sequence> element
            return checkAllOrSequence(name, (Element)list.item(0));
        }

        mLogger.warn("no <complexContent>, <all>, or <sequence> nodes found under <complexType>");
        return null;
    }


    /**
     * Currently, just checks to see if complex content is an array. Anything
     * else will throw an exception.
     */
    ComplexType checkComplexContent(String name, Element cc) throws Exception {
        if (mLogger.isDebugEnabled()) {
            mLogger.debug("checkComplexContent: " + name + ", " + cc);
        }

        NodeList list;
        list = cc.getElementsByTagNameNS(mNamespaceURI_SCHEMA_XSD, "restriction");
        if (foundOne(list)) {
            return checkRestriction(name, (Element)list.item(0));             
        }

        list = cc.getElementsByTagNameNS(mNamespaceURI_SCHEMA_XSD, "extension");
        if (foundOne(list)) {
            return checkExtension(name, (Element)list.item(0));
        }

        // <restriction> and <extension> ony supported in <complexContent>
        mLogger.warn("No <restriction> or <extension> tags were found inside of <complexContent>");
        return null;
    }

    /**
     * 
     */
    ComplexType checkExtension(String name, Element extension) throws Exception {
        String base = extension.getAttribute("base");
        if (base == null || base.equals("")) {
            throw new Exception("no base attribute found in <extension>");
        }

        QName baseQName = XMLUtils.getQNameFromString(base, extension);
        ComplexType baseType = (ComplexType)mComplexTypeMap.get(baseQName);
        if (baseType == null) {
            throw new Exception("could not find <extension> base type: " + baseQName);
        }

        NodeList list;
        ComplexType ct = null;
        list = extension.getElementsByTagNameNS(mNamespaceURI_SCHEMA_XSD, "all");
        if (foundOne(list)) {
            // get values inside <all> element
            ct = checkAllOrSequence(name, (Element)list.item(0));
        }

        list = extension.getElementsByTagNameNS(mNamespaceURI_SCHEMA_XSD, "sequence");
        if (foundOne(list)) {
            // get values inside <sequence> element
            ct = checkAllOrSequence(name, (Element)list.item(0));
        }

        if (ct == null) {
            mLogger.warn("no <all> or <sequence> nodes found under <extension>");
            return null;
        }

        ct.setBase(baseType);

        return ct;
    }

    /**
     * Assume <restriction> in <complexContent> means array (for now).
     */
    ComplexType checkRestriction(String name, Element restriction) throws Exception {
        String base = restriction.getAttribute("base");
        if (base == null || base.equals("")) {
            throw new Exception("no base attribute found in <restriction>");
        }

        QName baseQName = XMLUtils.getQNameFromString(base, restriction);
        if (! mSOAPEncodingArray.equals(baseQName)) {
            throw new Exception("only arrays are supported in <restriction>, instead found "
                                + baseQName);
        }

        // now try to get type of array from <attribute> element in
        // <restriction>
        NodeList list = restriction.getElementsByTagNameNS(mNamespaceURI_SCHEMA_XSD, "attribute");
        if (! foundOne(list)) {
            // FIXME
            // XXX this is not necessarily true...I think we can assume anyType
            // array if attribute is missing. Fix this later.
            throw new Exception("expecting only one attribute inside <restriction base=\"soapenc:Array\">");
        }

        Element attributeElement = (Element)list.item(0);
        String ref = attributeElement.getAttribute("ref");

        String arrayType = attributeElement.getAttributeNS(mNamespaceURI_WSDL, "arrayType");

        if (ref.equals("")) {
            throw new Exception("empty ref attribute in <attribute> inside of <restriction> for <complexType> named " + name);
        }

        if (arrayType.equals("")) {
            throw new Exception("empty ref attribute in <attribute> inside of <restriction> for <complexType> named " + name);
        }

        QName refQName = XMLUtils.getQNameFromString(ref, attributeElement);
        if (! mSOAPEncodingArrayType.equals(refQName)) {
            throw new Exception("ref attribute in <attribute> inside of <restriction> does not refer to SOAP-ENC array");
        }

        String type = removeBrackets(arrayType);
        QName typeQName = XMLUtils.getQNameFromString(type, attributeElement);

        return 
            new ComplexType(new QName(mTargetNamespaceURI, name),
                            ComplexType.TYPE_ARRAY, typeQName);
    }


    /**
     * Get arrayType without brackets to just get type.
     */
    String removeBrackets(String arrayType) {
        int index = arrayType.indexOf('[');
        if (index == -1) {
            return arrayType;
        }
        return arrayType.substring(0, index);
    }


    /**
     * Check for elements inside <all> or sequence. All elements must have a type
     * attribute. We don't support anonymous types.
     */
    ComplexType checkAllOrSequence(String name, Element node) throws Exception {
        if (mLogger.isDebugEnabled()) {
            mLogger.debug("checkAllOrSequence: " + name + ", " + node);
        }

        String tag = node.getTagName();
        Map members = new HashMap();
        NodeList list = node.getElementsByTagNameNS(mNamespaceURI_SCHEMA_XSD, "element");
        for (int i=0; i < list.getLength(); i++) {
            Element element = (Element)list.item(i);
            String elName = element.getAttribute("name");
            String elType = element.getAttribute("type");
            if (elName.equals("")) {
                throw new Exception("name attribute missing in <element> inside <" + tag + ">: " + node);
            }
            if (elType.equals("")) {
                throw new Exception("type attribute missing in <element> inside <" + tag + ">: " + node
                                    + "(anonymous types not supported)");
            }
            QName elTypeQName = XMLUtils.getQNameFromString(elType, element);
            members.put(elName, elTypeQName);
        }

        return new ComplexType(new QName(mTargetNamespaceURI, name), ComplexType.TYPE_STRUCT, members);
    }


    /**
     * See if complexType element was found.
     */
    boolean foundOne(NodeList list) {
        return list.getLength() == 1;
    }
}
