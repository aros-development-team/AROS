/* *****************************************************************************
 * SWFObjectDeserializer.java
 * ****************************************************************************/

/* J_LZ_COPYRIGHT_BEGIN *******************************************************
* Copyright 2001-2004 Laszlo Systems, Inc.  All Rights Reserved.              *
* Use is subject to license terms.                                            *
* J_LZ_COPYRIGHT_END *********************************************************/

package org.openlaszlo.remote.soap.encoding;

import org.openlaszlo.iv.flash.util.FlashBuffer;
import org.openlaszlo.iv.flash.api.action.Actions;
import org.openlaszlo.iv.flash.api.action.Program;
import org.apache.axis.Constants;
import org.apache.axis.components.logger.LogFactory;
import org.apache.axis.encoding.DeserializationContext;
import org.apache.axis.encoding.Deserializer;
import org.apache.axis.encoding.DeserializerImpl;
import org.apache.axis.encoding.DeserializerTarget;
import org.apache.axis.message.SOAPHandler;
import org.apache.axis.utils.ClassUtils;
import org.apache.axis.utils.JavaUtils;
import org.apache.axis.utils.Messages;
import org.apache.axis.wsdl.symbolTable.SchemaUtils;
import org.apache.commons.logging.Log;
import org.apache.axis.soap.SOAPConstants;
import org.apache.axis.MessageContext;

import org.apache.axis.utils.DOM2Writer;

import org.xml.sax.Attributes;
import org.xml.sax.SAXException;

import javax.xml.namespace.QName;
import java.io.StringWriter;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.StringTokenizer;

import org.apache.axis.message.MessageElement;
import org.apache.log4j.Logger;
import org.openlaszlo.iv.flash.util.FlashBuffer;

public class SWFObjectDeserializer extends SWFDeserializer
{
    public static Logger mLogger =
        Logger.getLogger(SWFObjectDeserializer.class);

    int mCounter = 0;
    String mClassName = "";
    String mClassNameSpace = "";

    // TODO: [2005-02-24 pkang]: pushNull is duplicated in other places. Should
    // put move this to a utility file.
    public void pushNull() {
        mProgram.body().writeByte(Actions.PushData);
        mProgram.body().writeWord(0+1);
        mProgram.body().writeByte(2);
    }

    public void onStartElement(String namespace, String localName,
                               String prefix, Attributes attributes,
                               DeserializationContext context)
        throws SAXException {

        if (mLogger.isDebugEnabled()) {
            mLogger.debug("Enter: SWFObjectDeserializer::onStartChild"
                          + "( namespace: " + namespace
                          + ", localname: " + localName
                          + ", prefix: "    + prefix                           
                          + ")");
        }

        // Use the xsi:type setting on the attribute if it exists.
        QName itemType = 
            context.getTypeFromAttributes(namespace, localName, attributes);

        if (itemType == null) {
            // FIXME: [2004-07-11 pkang] what do we do in this case? ideally
            // treat the rest this as an element.
            mLogger.debug("itemType is null"); 
        } else {
            mClassName = itemType.getLocalPart();
            mClassNameSpace = itemType.getNamespaceURI();
        }
    }


    public SOAPHandler onStartChild(String namespace, String localName,
                                    String prefix, Attributes attributes,
                                    DeserializationContext context)
        throws SAXException
    {
        if (mLogger.isDebugEnabled()) {
            mLogger.debug("Enter: SWFObjectDeserializer::onStartChild"
                          + "( namespace: " + namespace
                          + ", localname: " + localName
                          + ", prefix: "    + prefix                           
                          + ")");
        }

        // Use the xsi:type setting on the attribute if it exists.
        QName itemType = 
            context.getTypeFromAttributes(namespace, localName, attributes);

        // Get the deserializer for the type. 
        Deserializer dSer = null;
        if (itemType != null && (context.getCurElement().getHref() == null)) {
            dSer = context.getDeserializerForType(itemType);
        }


        if (dSer == null) {
            dSer = new SWFObjectDeserializer();
        }

        // increment counter
        mCounter++;

        // Register the callback value target, and keep track of this index so
        // we know when it has been set.
        dSer.registerValueTarget(new DeserializerTarget(this, localName));
        
        // The framework handles knowing when the value is complete, as long as
        // we tell it about each child we're waiting on.
        addChildDeserializer(dSer);

        // push the name of the property
        mProgram.push(localName);

        return (SOAPHandler)dSer;
    }


    public void valueComplete() throws SAXException { 

        if (mLogger.isDebugEnabled()) {
            mLogger.debug("Enter: SWFObjectDeserializer::valueComplete()");
        }

        if (componentsReady()) {

            if (! ("".equals(mClassName) && "".equals(mClassNameSpace))) {

                mProgram.push("__LZclassnamespace");
                mProgram.push(mClassNameSpace);
                mProgram.push("__LZclassname");
                mProgram.push(mClassName);
                mProgram.push(mCounter + 2);
                mProgram.body().writeByte(Actions.InitObject);

                // Call _root.LzSOAPService.__LZnormObj(). This function will set the
                // object's prototype to one that exists in the namespace and will
                // return the object so it stays in the stack
                mProgram.push(1);
                mProgram.push("_root");
                mProgram.getVar();
                mProgram.push("LzSOAPService");
                mProgram.body().writeByte(Actions.GetMember);
                mProgram.push("__LZnormObj");
                mProgram.callMethod();
            } else if (isNil) {
                pushNull();
            }

            value = mProgram;
        }     
        
        super.valueComplete();

    }
}
