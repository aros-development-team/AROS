/* *****************************************************************************
 * SWFDeserializer.java
 * ****************************************************************************/

/* J_LZ_COPYRIGHT_BEGIN *******************************************************
* Copyright 2001-2004 Laszlo Systems, Inc.  All Rights Reserved.              *
* Use is subject to license terms.                                            *
* J_LZ_COPYRIGHT_END *********************************************************/

package org.openlaszlo.remote.soap.encoding;

import org.openlaszlo.iv.flash.util.FlashBuffer;
import org.openlaszlo.iv.flash.api.action.Program;
import org.xml.sax.Attributes;
import org.xml.sax.SAXException;
import org.apache.axis.MessageContext;
import org.apache.axis.encoding.DeserializationContext;
import org.apache.axis.encoding.DeserializerImpl;
import org.apache.log4j.Logger;

public class SWFDeserializer extends DeserializerImpl
{
    public static Logger mLogger = Logger.getLogger(SWFDeserializer.class);

    // set by the first element we entered from
    boolean mIsTopLevel = false;

    Program mProgram = null;
    static int BUFSIZE = 8192;

    public void startElement(String namespace, String localName,
                             String prefix, Attributes attributes,
                             DeserializationContext context)
        throws SAXException
    {
        MessageContext mesgContext = context.getMessageContext();
        if (mesgContext == null) {
            throw new RuntimeException("message context is null");
        }

        mProgram = (Program)mesgContext.getProperty("program");
        if ( mProgram == null ) {
            mProgram = new Program( new FlashBuffer(BUFSIZE) );
            mesgContext.setProperty("program", mProgram);
            mLogger.debug("created Program");
            mIsTopLevel = true;
        }

        super.startElement(namespace, localName, prefix, attributes, context);
    }
}
