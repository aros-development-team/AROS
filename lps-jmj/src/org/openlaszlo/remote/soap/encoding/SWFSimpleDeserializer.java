/* *****************************************************************************
 * SWFSimpleDeserializer.java
 * ****************************************************************************/

/* J_LZ_COPYRIGHT_BEGIN *******************************************************
* Copyright 2001-2004 Laszlo Systems, Inc.  All Rights Reserved.              *
* Use is subject to license terms.                                            *
* J_LZ_COPYRIGHT_END *********************************************************/

/*
 * The Apache Software License, Version 1.1
 *
 *
 * Copyright (c) 2001-2003 The Apache Software Foundation.  All rights
 * reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * 3. The end-user documentation included with the redistribution,
 *    if any, must include the following acknowledgment:
 *       "This product includes software developed by the
 *        Apache Software Foundation (http://www.apache.org/)."
 *    Alternately, this acknowledgment may appear in the software itself,
 *    if and wherever such third-party acknowledgments normally appear.
 *
 * 4. The names "Axis" and "Apache Software Foundation" must
 *    not be used to endorse or promote products derived from this
 *    software without prior written permission. For written
 *    permission, please contact apache@apache.org.
 *
 * 5. Products derived from this software may not be called "Apache",
 *    nor may "Apache" appear in their name, without prior written
 *    permission of the Apache Software Foundation.
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESSED OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE APACHE SOFTWARE FOUNDATION OR
 * ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 * ====================================================================
 *
 * This software consists of voluntary contributions made by many
 * individuals on behalf of the Apache Software Foundation.  For more
 * information on the Apache Software Foundation, please see
 * <http://www.apache.org/>.
 */

package org.openlaszlo.remote.soap.encoding;

import org.openlaszlo.iv.flash.api.action.Actions;
import org.openlaszlo.iv.flash.util.FlashBuffer;
import java.io.CharArrayWriter;
import javax.xml.namespace.QName;
import org.apache.axis.message.MessageElement;
import org.apache.axis.encoding.DeserializationContext;
import org.apache.axis.message.SOAPHandler;
import org.apache.axis.utils.Messages;
import org.apache.log4j.Logger;
import org.xml.sax.Attributes;
import org.xml.sax.SAXException;
import java.math.BigInteger;
import java.math.BigDecimal;

import org.apache.axis.Message;
import org.apache.axis.MessageContext;
import javax.xml.soap.SOAPMessage;


// Lifted from SimpleDeserializer
public class SWFSimpleDeserializer extends SWFDeserializer
{
    public static Logger mLogger =
        Logger.getLogger(SWFSimpleDeserializer.class);

    private final CharArrayWriter val = new CharArrayWriter();

    public QName xmlType;
    public Class javaType;

    public SWFSimpleDeserializer(Class javaType, QName xmlType) {
        this.xmlType = xmlType;
        this.javaType = javaType;
    }

    public void characters(char [] chars, int start, int end)
        throws SAXException {
        val.write(chars,start,end);
    }


    public void pushNull() {
        mProgram.body().writeByte(Actions.PushData);
        mProgram.body().writeWord(0+1);
        mProgram.body().writeByte(2);
    }


    void pushBoolean(boolean b) {
        mProgram.body().writeByte(Actions.PushData);
        mProgram.body().writeWord(1+1);
        mProgram.body().writeByte(5);
        mProgram.body().writeByte(b?1:0);
    }

    void pushDouble(double d) {
        mProgram.body().writeByte(Actions.PushData);
        mProgram.body().writeWord(8+1);
        mProgram.body().writeByte(6);
        long dbits = Double.doubleToLongBits(d);
        mProgram.body().writeDWord((int)(dbits>>>32));
        mProgram.body().writeDWord((int)(dbits&0xffffffffL));
    }

    public void onEndElement(String namespace, String localName,
                             DeserializationContext context) 
        throws SAXException {

        //----------------------------------------------------------------------
        // FIXME: [2004-07-13 pkang] does this handle SOAP 1.2 fault format?
        // If we're deserializing fault, just pass back the string value. 
        // SOAP 1.1: <faultstring> 
        //----------------------------------------------------------------------
        if ("".equals(namespace)) {
            if ( "faultstring".equals(localName) || 
                 "faultactor".equals(localName)  ) {
                value = val.toString();
                return;
            }
        }

        //if (isNil || val == null) {  -- FIX http://nagoya.apache.org/bugzilla/show_bug.cgi?id=11945
        if (isNil) {
            pushNull();
            value = mProgram;
            return;
        }

        String source = val.toString();

mLogger.info("JJJJJ " + source + ", " + namespace +", " + localName);
        try {
            if ( javaType == int.class || javaType == Integer.class) {
                mProgram.push(Integer.parseInt(source));
            } else if (javaType == long.class || javaType == Long.class) {
                // push as int
                int n = Long.valueOf(source).intValue();
                mProgram.push(n);
            } else if (javaType == short.class || javaType == Short.class) {
                // push as int
                int n = Short.valueOf(source).intValue();
                mProgram.push(n);
            } else if (javaType == byte.class || javaType == Byte.class) {
                // push as int
                int n = Byte.valueOf(source).intValue();
                mProgram.push(n);
            } else if (javaType == BigInteger.class) {
                // push as int
                int n = BigInteger.valueOf(Long.parseLong(source)).intValue();
                mProgram.push(n);
            } else if (javaType == BigDecimal.class) {
                // push as int
                int n = BigDecimal.valueOf(Long.parseLong(source)).intValue();
                mProgram.push(n);
            } else if (javaType == boolean.class || javaType == Boolean.class) {
                switch (source.charAt(0)) {
                case '0': case 'f': case 'F':
                    pushBoolean(false);
                    break;
                case '1': case 't': case 'T': 
                    pushBoolean(true);
                    break;
                default:
                    pushBoolean(true);
                    break;
                }
            } else if (javaType == float.class || javaType == Float.class) {
                if (source.equals("NaN")) {
                    mProgram.push(Float.NaN);
                } else if (source.equals("INF")) {
                    mProgram.push(Float.POSITIVE_INFINITY);
                } else if (source.equals("-INF")) {
                    mProgram.push(Float.NEGATIVE_INFINITY);
                } else {
                    mProgram.push(Float.parseFloat(source));
                }
            } else if (javaType == double.class || javaType == Double.class) {
                if (source.equals("NaN")) {
                    pushDouble(Double.NaN);
                } else if (source.equals("INF")) {
                    pushDouble(Double.POSITIVE_INFINITY);
                } else if (source.equals("-INF")) {
                    pushDouble(Double.NEGATIVE_INFINITY);
                } else {
                    pushDouble(Double.parseDouble(source));
                }
            } else if (javaType == String.class) {
                // treat as a string by default
                mProgram.push(source);
            } else {
                // catch all
                mLogger.warn("treating " + javaType + " like string: " + source);
                mProgram.push(source);
            }
        } catch (Exception e) {
            mLogger.error("Exception", e);
            throw new SAXException(e.getMessage());
        }

        value = mProgram;
    }

    /**
     * There should not be nested elements.
     */
    public SOAPHandler onStartChild(String namespace, String localName, 
                                    String prefix, Attributes attributes,
                                    DeserializationContext context)
        throws SAXException {
        throw new SAXException(Messages.getMessage("cantHandle00",
                                                   "SimpleDeserializer"));
    }
}
