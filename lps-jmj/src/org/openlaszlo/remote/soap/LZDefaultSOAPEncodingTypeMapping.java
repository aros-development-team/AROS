/* *****************************************************************************
 * LZDefaultSOAPEncodingTypeMapping.java
 * ****************************************************************************/

/* J_LZ_COPYRIGHT_BEGIN *******************************************************
* Copyright 2001-2004 Laszlo Systems, Inc.  All Rights Reserved.              *
* Use is subject to license terms.                                            *
* J_LZ_COPYRIGHT_END *********************************************************/

/*
 * Copyright 2001-2004 The Apache Software Foundation.
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *      http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package org.openlaszlo.remote.soap;

import org.apache.axis.Constants;
import org.apache.axis.encoding.TypeMapping;
import org.apache.axis.encoding.ser.Base64SerializerFactory;
import org.apache.axis.encoding.ser.Base64DeserializerFactory;

/**
 * @author Rich Scheuerle (scheu@us.ibm.com)
 * 
 * This is the implementation of the axis Default JAX-RPC SOAP 1.2 TypeMapping
 * See DefaultTypeMapping for more information.
 * 
 */
public class LZDefaultSOAPEncodingTypeMapping extends LZDefaultTypeMapping {
    
    private static LZDefaultSOAPEncodingTypeMapping tm = null;
    /**
     * Construct TypeMapping
     */
    public static TypeMapping create() {
        if (tm == null) {
            tm = new LZDefaultSOAPEncodingTypeMapping();
        }
        return tm;
    }
    
    public static TypeMapping createWithDelegate() {
        TypeMapping ret = new LZDefaultSOAPEncodingTypeMapping();
        ret.setDelegate(LZDefaultTypeMapping.getSingleton());
        return ret;
    }

    protected LZDefaultSOAPEncodingTypeMapping() {
        registerSOAPTypes();        
    }

    /**
     * Register the SOAP encoding data types.  This is split out into a
     * method so it can happen either before or after the XSD mappings.
     */
    private void registerSOAPTypes() {
        // SOAP Encoded strings are treated as primitives.
        // Everything else is not.
        myRegisterSimple(Constants.SOAP_STRING, java.lang.String.class);
        myRegisterSimple(Constants.SOAP_BOOLEAN, java.lang.Boolean.class);
        myRegisterSimple(Constants.SOAP_DOUBLE, java.lang.Double.class);
        myRegisterSimple(Constants.SOAP_FLOAT, java.lang.Float.class);
        myRegisterSimple(Constants.SOAP_INT, java.lang.Integer.class);
        myRegisterSimple(Constants.SOAP_INTEGER, java.math.BigInteger.class);
        myRegisterSimple(Constants.SOAP_DECIMAL, java.math.BigDecimal.class);
        myRegisterSimple(Constants.SOAP_LONG, java.lang.Long.class);
        myRegisterSimple(Constants.SOAP_SHORT, java.lang.Short.class);
        myRegisterSimple(Constants.SOAP_BYTE, java.lang.Byte.class);
//         myRegister(Constants.SOAP_BASE64,     byte[].class,
//                    new Base64SerializerFactory(byte[].class,
//                                                Constants.SOAP_BASE64 ),
//                    new Base64DeserializerFactory(byte[].class,
//                                                  Constants.SOAP_BASE64)
//         );
//         myRegister(Constants.SOAP_BASE64BINARY,     
//                    byte[].class, 
//                    new Base64SerializerFactory(byte[].class, Constants.SOAP_BASE64 ), 
//                    new Base64DeserializerFactory(byte[].class, Constants.SOAP_BASE64) );
    }
}
