/* *****************************************************************************
 * SWFArrayDeserializer.java
 * ****************************************************************************/

/* J_LZ_COPYRIGHT_BEGIN *******************************************************
* Copyright 2001-2004 Laszlo Systems, Inc.  All Rights Reserved.              *
* Use is subject to license terms.                                            *
* J_LZ_COPYRIGHT_END *********************************************************/

package org.openlaszlo.remote.soap.encoding;

import org.openlaszlo.iv.flash.util.FlashBuffer;
import org.openlaszlo.iv.flash.api.action.Actions;
import org.openlaszlo.iv.flash.api.action.Program;
import java.io.StringWriter;
import java.util.ArrayList;
import java.util.StringTokenizer;
import javax.xml.namespace.QName;
import org.apache.axis.Constants;
import org.apache.axis.encoding.DeserializationContext;
import org.apache.axis.encoding.Deserializer;
import org.apache.axis.encoding.DeserializerImpl;
import org.apache.axis.encoding.DeserializerTarget;
import org.apache.axis.message.SOAPHandler;
import org.apache.axis.utils.DOM2Writer;
import org.apache.axis.utils.Messages;
import org.apache.axis.soap.SOAPConstants;
import org.apache.axis.MessageContext;
import org.apache.log4j.Logger;
import org.xml.sax.Attributes;
import org.xml.sax.SAXException;

public class SWFArrayDeserializer extends SWFDeserializer
{
    public static Logger mLogger =
        Logger.getLogger(SWFArrayDeserializer.class);

    public SWFArrayDeserializer() {
        mLogger.debug("new SWFArrayDeserializer");
    }

    public QName arrayType = null;
    public int curIndex = 0;
    QName defaultItemType;
    int length;
    ArrayList mDimLength = null;  // If set, array of multi-dim lengths 
    ArrayList mDimFactor = null;  // If set, array of factors for multi-dim []
    SOAPConstants soapConstants = SOAPConstants.SOAP11_CONSTANTS;

    // array item tag to use (in LFC, see valueToElement() in
    // data/LzDataElement.as)
    String mItemTag = null;

    /**
     * This method is invoked after startElement when the element requires
     * deserialization (i.e. the element is not an href & the value is not nil)
     * DeserializerImpl provides default behavior, which simply
     * involves obtaining a correct Deserializer and plugging its handler.
     * @param namespace is the namespace of the element
     * @param localName is the name of the element
     * @param prefix is the prefix of the element
     * @param attributes are the attrs on the element...used to get the type
     * @param context is the DeserializationContext
     */
    public void onStartElement(String namespace, String localName,
                               String prefix, Attributes attributes,
                               DeserializationContext context)
        throws SAXException
    {
        // Deserializing the xml array requires processing the xsi:type=
        // attribute, the soapenc:arrayType attribute, and the xsi:type
        // attributes of the individual elements.
        //
        // The xsi:type=<qName> attribute is used to determine the java type of
        // the array to instantiate. Axis expects it to be set to the generic
        // "soapenc:Array" or to a specific qName. If the generic
        // "soapenc:Array" specification is used, Axis determines the array type
        // by examining the soapenc:arrayType attribute.
        //
        // The soapenc:arrayType=<qname><dims> is used to determine
        //
        // i) the number of dimensions, 
        // ii) the length of each dimension,
        // iii) the default xsi:type of each of the elements.
        //
        // If the arrayType attribute is missing, Axis assumes a single
        // dimension array with length equal to the number of nested elements.
        // In such cases, the default xsi:type of the elements is determined
        // using the array xsi:type.
        //
        // The xsi:type attributes of the individual elements of the array are
        // used to determine the java type of the element.  If the xsi:type
        // attribute is missing for an element, the default xsi:type value is
        // used.

        if (mLogger.isDebugEnabled()) {
            mLogger.debug("Enter: SWFArrayDeserializer::onStartElement"
                          + "( namespace: " + namespace
                          + ", localname: " + localName
                          + ", prefix: "    + prefix
                          + ", context: "   + context
                          + ")");

            try {
                StringWriter writer = new StringWriter();
                DOM2Writer.serializeAsXML(context.getCurElement().getAsDOM(), writer, 
                                          true, true);
                mLogger.debug("onStartElement CURRENT ELEMENT:\n" + writer.toString());
            } catch (Exception e) {
            }
        }

        MessageContext msgContext = context.getMessageContext();
        if (msgContext != null) {
            soapConstants = msgContext.getSOAPConstants();
        }

        // Get QName for the array type. Set to null if generic type is used,
        // e.g., soapenc:Array
        QName typeQName = 
            context.getTypeFromAttributes(namespace, localName, attributes);

        if (typeQName == null) {
            typeQName = getDefaultType();
        }

        // if our typeQName is generic, set typeQName to null
        if (typeQName != null &&
            Constants.equals(Constants.SOAP_ARRAY, typeQName)) {
            typeQName = null;
        }

        // Now get the arrayType value. 
        //
        //    Constants.getValue(Attributes, String[] search, String localPart)
        //
        // search - an array of namespace URI strings to search
        // 
        // Basically, get "soapenc:arrayType" 
        String prefixArrayTypeValue = 
            Constants.getValue(attributes, Constants.URIS_SOAP_ENC,
                               soapConstants.getAttrItemType());
        QName arrayTypeValue = context.getQNameFromString(prefixArrayTypeValue);

        // The first part of the arrayType expression is the default item type
        // qname.  The second part is the dimension information
        String dimString = null;
        QName innerQName = null; // for multidimensional arrays
        String innerDimString = "";  // for multidimensional arrays
        if (arrayTypeValue != null) {

            if (soapConstants != SOAPConstants.SOAP12_CONSTANTS) {
                // Handle SOAP 1.1 Array
                mLogger.debug("Parse SOAP 1.1 array");

                String arrayTypeValueNamespaceURI = 
                    arrayTypeValue.getNamespaceURI();
                String arrayTypeValueLocalPart = 
                    arrayTypeValue.getLocalPart();

                int leftBracketIndex = 
                    arrayTypeValueLocalPart.lastIndexOf('[');
                int rightBracketIndex = 
                    arrayTypeValueLocalPart.lastIndexOf(']');

                if (leftBracketIndex == -1 || 
                    rightBracketIndex == -1 || 
                    rightBracketIndex < leftBracketIndex) {
                    throw new IllegalArgumentException
                        (Messages.getMessage("badArrayType00", "" + 
                                             arrayTypeValue));
                }
                
                dimString = 
                    arrayTypeValueLocalPart.substring(leftBracketIndex + 1,
                                                      rightBracketIndex);

                arrayTypeValueLocalPart = 
                    arrayTypeValueLocalPart.substring(0, leftBracketIndex);
                

                // If multi-dim array set to soapenc:Array
                if (arrayTypeValueLocalPart.endsWith("]")) {
                    defaultItemType = Constants.SOAP_ARRAY;
                    innerQName = new QName(arrayTypeValueNamespaceURI,
                                           arrayTypeValueLocalPart.substring
                                           (0,arrayTypeValueLocalPart.indexOf("["))); 
                    innerDimString = 
                        arrayTypeValueLocalPart.substring
                        (arrayTypeValueLocalPart.indexOf("["));

                    // TODO: [2004-06-30 pkang] support multidimensional arrays 
                    throw new RuntimeException("multidimensional array not supported");

                } else {
                    defaultItemType = new QName(arrayTypeValueNamespaceURI,
                                                arrayTypeValueLocalPart);
                }

            } else {

                // Handle SOAP 1.2 Array
                mLogger.debug("Parse SOAP 1.2 array");

                // --------------------------------------------------------------
                // From http://www.w3.org/TR/soap12-part2/#arraySizeattr:
                //
                //  The arraySize attribute information item has the following
                //  Infoset properties:
                //
                //     * A [local name] of arraySize .
                //     * A [namespace name] of 
                //       "http://www.w3.org/2003/05/soap-encoding".
                //
                // The type of the arraySize attribute information item is
                // enc:arraySize. The value of the arraySize attribute
                // information item MUST conform to the following EBNF grammar
                //
                // [1] arraySizeValue	::= ("*" | concreteSize) nextConcreteSize*
                // [2] nextConcreteSize	::= whitespace concreteSize
                // [3] concreteSize	::= [0-9]+
                // [4] white space	::= (#x20 | #x9 | #xD | #xA)+
                //
                // The array's dimensions are represented by each item in the
                // list of sizes (unspecified size in case of the asterisk). The
                // number of items in the list represents the number of
                // dimensions in the array. The asterisk, if present, MUST only
                // appear in the first position in the list. The default value
                // of the arraySize attribute information item is "*", that is
                // by default arrays are considered to have a single dimension
                // of unspecified size.
                // --------------------------------------------------------------

                String arraySizeValue = attributes.getValue(soapConstants.getEncodingURI(), Constants.ATTR_ARRAY_SIZE);
                int leftStarIndex = arraySizeValue.lastIndexOf('*');

                // Skip to num if any
                if (leftStarIndex != -1) {
                    // "*" => ""
                    if (leftStarIndex == 0 && arraySizeValue.length() == 1) {
                        // "* *" => ""
                    } else if (leftStarIndex == (arraySizeValue.length() - 1)) {
                        throw new IllegalArgumentException(
                                                           Messages.getMessage("badArraySize00",
                                                                               "" + arraySizeValue));
                        // "* N" => "N"
                    } else {
                        dimString = arraySizeValue.substring(leftStarIndex + 2);
                        innerQName = arrayTypeValue;
                        innerDimString = arraySizeValue.substring(0, leftStarIndex + 1);
                    }
                } else {
                    dimString = arraySizeValue;
                }

                if (innerDimString == null || innerDimString.length() == 0) {
                    defaultItemType = arrayTypeValue;
                } else {
                    defaultItemType = Constants.SOAP_ARRAY12;

                    // TODO: [2004-06-30 pkang] support multidimensional arrays 
                    throw new RuntimeException("multidimensional array not supported");
                }
            }
        }

        // If no type QName and no defaultItemType qname, use xsd:anyType
        if (defaultItemType == null && typeQName == null) {
            defaultItemType = Constants.XSD_ANYTYPE;
        }
        
        // -------------------------------------------------------------------
        // If soapenc:offset specified, set the current index accordingly to
        // support SOAP 1.1 sparse arrays. Sparse arrays are not available in
        // SOAP 1.2.
        // -------------------------------------------------------------------
        String offset = Constants.getValue(attributes,
                                           Constants.URIS_SOAP_ENC,
                                           Constants.ATTR_OFFSET);
        if (offset != null) {
            // let's just no support sparse arrays for now
            if (soapConstants == SOAPConstants.SOAP12_CONSTANTS) {
                throw new SAXException(Messages.getMessage("noSparseArray"));
            }
        }
        
        if (mLogger.isDebugEnabled()) {
            mLogger.debug("Exit: SWFArrayDeserializer::onStartElement()");
        }
    }




    /**
     * onStartChild is called on each child element.
     * @param namespace is the namespace of the child element
     * @param localName is the local name of the child element
     * @param prefix is the prefix used on the name of the child element
     * @param attributes are the attributes of the child element
     * @param context is the deserialization context.
     * @return is a Deserializer to use to deserialize a child (must be
     * a derived class of SOAPHandler) or null if no deserialization should
     * be performed.
     */
    public SOAPHandler onStartChild(String namespace,
                                    String localName,
                                    String prefix,
                                    Attributes attributes,
                                    DeserializationContext context)
        throws SAXException
    {
        if (mLogger.isDebugEnabled()) {

            mLogger.debug("Enter: SWFArrayDeserializer::onStartChild"
                          + "( namespace: " + namespace
                          + ", localname: " + localName
                          + ", prefix: "    + prefix                           
                          + ")");
            try {
                StringWriter writer = new StringWriter();
                DOM2Writer.serializeAsXML(context.getCurElement().getAsDOM(), writer, 
                                          true, true);
                mLogger.debug("onStartChild CURRENT ELEMENT:\n" + writer.toString());
            } catch (Exception e) { }

        }

        if (mItemTag == null) mItemTag = localName;

        // Use the xsi:type setting on the attribute if it exists.
        QName itemType = 
            context.getTypeFromAttributes(namespace, localName, attributes);

        if (mLogger.isDebugEnabled()) {
            mLogger.info("array itemType: " + itemType);
        }

        // Get the deserializer for the type. 
        Deserializer dSer = null;
        if (itemType != null && (context.getCurElement().getHref() == null)) {
            dSer = context.getDeserializerForType(itemType);
        }

        if (dSer == null) {
            dSer = new SWFObjectDeserializer();
        }

        // Register the callback value target, and
        // keep track of this index so we know when it has been set.
        dSer.registerValueTarget(new DeserializerTarget(this, null));
        
        // The framework handles knowing when the value is complete, as
        // long as we tell it about each child we're waiting on...
        addChildDeserializer(dSer);

        curIndex++;
        
        if (log.isDebugEnabled()) {
            mLogger.debug("itemType: " + itemType + ", dSer: " + dSer );
            mLogger.debug("Exit: SWFArrayDeserializer.onStartChild()");
        }
        
        return (SOAPHandler)dSer;
    }



    public void characters(char[] chars, int i, int i1) throws SAXException {
        for (int idx = i; i < i1; i++) {
            if (!Character.isWhitespace(chars[idx]))
                throw new SAXException(Messages.getMessage("charsInArray"));            
        }
    }


    /**
     * When valueComplete() is invoked on the array, 
     * first convert the array value into the expected array.
     * Then call super.valueComplete() to inform referents
     * that the array value is ready.
     **/
    public void valueComplete() throws SAXException
    { 
        if (mLogger.isDebugEnabled()) {
            mLogger.debug("Enter: SWFArrayDeserializer::valueComplete()");
        }

        if (componentsReady()) {
            if (mLogger.isDebugEnabled()) {
                mLogger.debug("SWFArrayDeserializer::valueComplete() - componentsReady");
            }
            try {
               mProgram.push(curIndex);
               mProgram.body().writeByte(Actions.InitArray);

               // because the array is reversed, we need to call reverse()
               mProgram.push(0);
               mProgram.body().writeByte(Actions.StackSwap);
               mProgram.push("reverse");
               mProgram.callMethod();
               
               if (mItemTag != null) {
                   mProgram.body().writeByte(Actions.PushDuplicate);
                   mProgram.push("__LZtag"); // __LZtag (array item tag)
                   mProgram.push(mItemTag);
                   mProgram.body().writeByte(Actions.SetMember);
               }

               value = mProgram;

           } catch (RuntimeException e) {
               // We must ignore exceptions from convert for Arrays with null - why?
               mLogger.warn("RuntimeException", e);
           }
        }     
        
        super.valueComplete();
    }
}
