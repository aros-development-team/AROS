/* *****************************************************************************
 * LZDefaultTypeMapping.java
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
import org.apache.axis.attachments.OctetStream;
import org.apache.axis.encoding.TypeMapping;
import org.apache.axis.encoding.TypeMappingImpl;
import org.apache.axis.encoding.ser.ArraySerializerFactory;
import org.apache.axis.encoding.ser.Base64SerializerFactory;
import org.apache.axis.encoding.ser.BeanSerializerFactory;
import org.apache.axis.encoding.ser.DateSerializerFactory;
import org.apache.axis.encoding.ser.ElementSerializerFactory;
import org.apache.axis.encoding.ser.HexSerializerFactory;
import org.apache.axis.encoding.ser.JAFDataHandlerSerializerFactory;
import org.apache.axis.encoding.ser.MapSerializerFactory;
import org.apache.axis.encoding.ser.QNameSerializerFactory;
import org.apache.axis.encoding.ser.SimpleSerializerFactory;
import org.apache.axis.encoding.ser.VectorSerializerFactory;
import org.apache.axis.encoding.ser.DocumentSerializerFactory;
import org.apache.axis.schema.SchemaVersion;
import org.apache.axis.types.HexBinary;
import org.apache.axis.utils.JavaUtils;
import org.apache.axis.utils.Messages;

import javax.xml.namespace.QName;
import javax.xml.rpc.JAXRPCException;
import javax.xml.rpc.encoding.SerializerFactory;
import javax.xml.rpc.encoding.DeserializerFactory;

// X import org.apache.axis.encoding.ser.SimpleDeserializerFactory;
// X import org.apache.axis.encoding.ser.ArrayDeserializerFactory;
// import org.apache.axis.encoding.ser.Base64DeserializerFactory;
// import org.apache.axis.encoding.ser.BeanDeserializerFactory;
// import org.apache.axis.encoding.ser.DateDeserializerFactory;
// import org.apache.axis.encoding.ser.ElementDeserializerFactory;
// import org.apache.axis.encoding.ser.HexDeserializerFactory;
// import org.apache.axis.encoding.ser.JAFDataHandlerDeserializerFactory;
// import org.apache.axis.encoding.ser.MapDeserializerFactory;
// import org.apache.axis.encoding.ser.QNameDeserializerFactory;
// import org.apache.axis.encoding.ser.VectorDeserializerFactory;
// import org.apache.axis.encoding.ser.DocumentDeserializerFactory;

import org.openlaszlo.remote.soap.encoding.SWFArrayDeserializerFactory;
import org.openlaszlo.remote.soap.encoding.LZObjectSerializerFactory;
import org.openlaszlo.remote.soap.encoding.SWFObjectDeserializer;
import org.openlaszlo.remote.soap.encoding.SWFSimpleDeserializer;
import org.openlaszlo.remote.soap.encoding.SWFSimpleDeserializerFactory;


public class LZDefaultTypeMapping extends TypeMappingImpl {

    private static LZDefaultTypeMapping tm = null;
    private boolean doneInit = false;   // have we completed initalization

    /**
     * Obtain the singleton default typemapping.
     */
    public static synchronized TypeMapping getSingleton() {
        if (tm == null) {
            tm = new LZDefaultTypeMapping();
        }
        return tm;
    }

    protected LZDefaultTypeMapping() {
        super(null);
        delegate = null;

        // Notes:
        // 1) The registration statements are order dependent.  The last one
        //    wins.  So if two javaTypes of String are registered, the
        //    ser factory for the last one registered will be chosen.  Likewise
        //    if two javaTypes for XSD_DATE are registered, the deserializer
        //    factory for the last one registered will be chosen.
        //    Corollary:  Please be very careful with the order.  The
        //                runtime, Java2WSDL and WSDL2Java emitters all
        //                use this code to get type mapping information.
        // 2) Even if the SOAP 1.1 format is used over the wire, an
        //    attempt is made to receive SOAP 1.2 format from the wire.
        //    This is the reason why the soap encoded primitives are
        //    registered without serializers.


        // anySimpleType is mapped to java.lang.String according to JAX-RPC 1.1 spec.
        myRegisterSimple(Constants.XSD_ANYSIMPLETYPE, java.lang.String.class);
        
        // If SOAP 1.1 over the wire, map wrapper classes to XSD primitives.
        myRegisterSimple(Constants.XSD_STRING, java.lang.String.class);
        myRegisterSimple(Constants.XSD_BOOLEAN, java.lang.Boolean.class);
        myRegisterSimple(Constants.XSD_DOUBLE, java.lang.Double.class);
        myRegisterSimple(Constants.XSD_FLOAT, java.lang.Float.class);
        myRegisterSimple(Constants.XSD_INT, java.lang.Integer.class);
        myRegisterSimple(Constants.XSD_INTEGER, java.math.BigInteger.class
        );
        myRegisterSimple(Constants.XSD_DECIMAL, java.math.BigDecimal.class
        );
        myRegisterSimple(Constants.XSD_LONG, java.lang.Long.class);
        myRegisterSimple(Constants.XSD_SHORT, java.lang.Short.class);
        myRegisterSimple(Constants.XSD_BYTE, java.lang.Byte.class);

        // The XSD Primitives are mapped to java primitives.
        myRegisterSimple(Constants.XSD_BOOLEAN, boolean.class);
        myRegisterSimple(Constants.XSD_DOUBLE, double.class);
        myRegisterSimple(Constants.XSD_FLOAT, float.class);
        myRegisterSimple(Constants.XSD_INT, int.class);
        myRegisterSimple(Constants.XSD_LONG, long.class);
        myRegisterSimple(Constants.XSD_SHORT, short.class);
        myRegisterSimple(Constants.XSD_BYTE, byte.class);

//         // Map QNAME to the jax rpc QName class
//         myRegister(Constants.XSD_QNAME,
//               javax.xml.namespace.QName.class,
//               new QNameSerializerFactory(javax.xml.namespace.QName.class,
//                                         Constants.XSD_QNAME),
//               new QNameDeserializerFactory(javax.xml.namespace.QName.class,
//                                         Constants.XSD_QNAME)
//         );

//         // The closest match for anytype is Object
//         myRegister(Constants.XSD_ANYTYPE,    java.lang.Object.class,
//                    null, null);

//         // See the SchemaVersion classes for where the registration of
//         // dateTime (for 2001) and timeInstant (for 1999 & 2000) happen.
//         myRegister(Constants.XSD_DATE,       java.sql.Date.class,
//                    new DateSerializerFactory(java.sql.Date.class,
//                                              Constants.XSD_DATE),
//                    new DateDeserializerFactory(java.sql.Date.class,
//                                                Constants.XSD_DATE)
//         );

//         // See the SchemaVersion classes for where the registration of
//         // dateTime (for 2001) and timeInstant (for 1999 & 2000) happen.
//         myRegister(Constants.XSD_DATE,       java.util.Date.class,
//                    new DateSerializerFactory(java.util.Date.class,
//                                              Constants.XSD_DATE),
//                    new DateDeserializerFactory(java.util.Date.class,
//                                                Constants.XSD_DATE)
//         );

//         // Mapping for xsd:time.  Map to Axis type Time
//         myRegister(Constants.XSD_TIME,       org.apache.axis.types.Time.class,
//                    new SimpleSerializerFactory(org.apache.axis.types.Time.class,
//                                              Constants.XSD_TIME),
//                    new SimpleDeserializerFactory(org.apache.axis.types.Time.class,
//                                                Constants.XSD_TIME)
//         );
//         // These are the g* types (gYearMonth, etc) which map to Axis types
//         myRegister(Constants.XSD_YEARMONTH, org.apache.axis.types.YearMonth.class,
//                    new SimpleSerializerFactory(org.apache.axis.types.YearMonth.class,
//                                              Constants.XSD_YEARMONTH),
//                    new SimpleDeserializerFactory(org.apache.axis.types.YearMonth.class,
//                                              Constants.XSD_YEARMONTH)
//         );
//         myRegister(Constants.XSD_YEAR, org.apache.axis.types.Year.class,
//                    new SimpleSerializerFactory(org.apache.axis.types.Year.class,
//                                              Constants.XSD_YEAR),
//                    new SimpleDeserializerFactory(org.apache.axis.types.Year.class,
//                                              Constants.XSD_YEAR)
//         );
//         myRegister(Constants.XSD_MONTH, org.apache.axis.types.Month.class,
//                    new SimpleSerializerFactory(org.apache.axis.types.Month.class,
//                                              Constants.XSD_MONTH),
//                    new SimpleDeserializerFactory(org.apache.axis.types.Month.class,
//                                              Constants.XSD_MONTH)
//         );
//         myRegister(Constants.XSD_DAY, org.apache.axis.types.Day.class,
//                    new SimpleSerializerFactory(org.apache.axis.types.Day.class,
//                                              Constants.XSD_DAY),
//                    new SimpleDeserializerFactory(org.apache.axis.types.Day.class,
//                                              Constants.XSD_DAY)
//         );
//         myRegister(Constants.XSD_MONTHDAY, org.apache.axis.types.MonthDay.class,
//                    new SimpleSerializerFactory(org.apache.axis.types.MonthDay.class,
//                                              Constants.XSD_MONTHDAY),
//                    new SimpleDeserializerFactory(org.apache.axis.types.MonthDay.class,
//                                              Constants.XSD_MONTHDAY)
//         );

//         // Serialize all extensions of Map to SOAP_MAP
//         // The SOAP_MAP will be deserialized into a HashMap by default.
//         myRegister(Constants.SOAP_MAP,       java.util.HashMap.class,
//                    new MapSerializerFactory(java.util.Map.class,
//                                             Constants.SOAP_MAP),
//                    new MapDeserializerFactory(java.util.HashMap.class,
//                                               Constants.SOAP_MAP)
//         );
//         myRegister(Constants.SOAP_MAP,       java.util.Hashtable.class,
//                    new MapSerializerFactory(java.util.Hashtable.class,
//                                             Constants.SOAP_MAP),
//                    null  // Make sure not to override the deser mapping
//         );
//         myRegister(Constants.SOAP_MAP,       java.util.Map.class,
//                    new MapSerializerFactory(java.util.Map.class,
//                                             Constants.SOAP_MAP),
//                    null  // Make sure not to override the deser mapping
//         );

//         // Use the Element Serializeration for elements
//         myRegister(Constants.SOAP_ELEMENT,   org.w3c.dom.Element.class,
//                    new ElementSerializerFactory(),
//                    new ElementDeserializerFactory());

//         // Use the Document Serializeration for Document's
//         myRegister(Constants.SOAP_DOCUMENT,   org.w3c.dom.Document.class,
//                    new DocumentSerializerFactory(),
//                    new DocumentDeserializerFactory());

//         myRegister(Constants.SOAP_VECTOR,    java.util.Vector.class,
//                    new VectorSerializerFactory(java.util.Vector.class,
//                                                Constants.SOAP_VECTOR),
//                    new VectorDeserializerFactory(java.util.Vector.class,
//                                                  Constants.SOAP_VECTOR)
//         );

//         // xsd:token
//         myRegister(Constants.XSD_TOKEN, org.apache.axis.types.Token.class,
//                    new SimpleSerializerFactory(org.apache.axis.types.Token.class,
//                                              Constants.XSD_TOKEN),
//                    new SimpleDeserializerFactory(org.apache.axis.types.Token.class,
//                                              Constants.XSD_TOKEN)
//         );

//         // a xsd:normalizedString
//         myRegister(Constants.XSD_NORMALIZEDSTRING, org.apache.axis.types.NormalizedString.class,
//                    new SimpleSerializerFactory(org.apache.axis.types.NormalizedString.class,
//                                              Constants.XSD_NORMALIZEDSTRING),
//                    new SimpleDeserializerFactory(org.apache.axis.types.NormalizedString.class,
//                                              Constants.XSD_NORMALIZEDSTRING)
//         );

//         // a xsd:unsignedLong
//         myRegister(Constants.XSD_UNSIGNEDLONG, org.apache.axis.types.UnsignedLong.class,
//              new SimpleSerializerFactory(org.apache.axis.types.UnsignedLong.class,
//                                                   Constants.XSD_UNSIGNEDLONG),
//              new SimpleDeserializerFactory(org.apache.axis.types.UnsignedLong.class,
//                                            Constants.XSD_UNSIGNEDLONG)
//         );

//         // a xsd:unsignedInt
//         myRegister(Constants.XSD_UNSIGNEDINT, org.apache.axis.types.UnsignedInt.class,
//              new SimpleSerializerFactory(org.apache.axis.types.UnsignedInt.class,
//                                                   Constants.XSD_UNSIGNEDINT),
//              new SimpleDeserializerFactory(org.apache.axis.types.UnsignedInt.class,
//                                            Constants.XSD_UNSIGNEDINT)
//         );

//         // a xsd:unsignedShort
//         myRegister(Constants.XSD_UNSIGNEDSHORT, org.apache.axis.types.UnsignedShort.class,
//              new SimpleSerializerFactory(org.apache.axis.types.UnsignedShort.class,
//                                                   Constants.XSD_UNSIGNEDSHORT),
//              new SimpleDeserializerFactory(org.apache.axis.types.UnsignedShort.class,
//                                            Constants.XSD_UNSIGNEDSHORT)
//         );

//         // a xsd:unsignedByte
//         myRegister(Constants.XSD_UNSIGNEDBYTE, org.apache.axis.types.UnsignedByte.class,
//                    new SimpleSerializerFactory(org.apache.axis.types.UnsignedByte.class,
//                                              Constants.XSD_UNSIGNEDBYTE),
//                    new SimpleDeserializerFactory(org.apache.axis.types.UnsignedByte.class,
//                                              Constants.XSD_UNSIGNEDBYTE)
//         );

//         // a xsd:nonNegativeInteger
//         myRegister(Constants.XSD_NONNEGATIVEINTEGER, org.apache.axis.types.NonNegativeInteger.class,
//              new SimpleSerializerFactory(org.apache.axis.types.NonNegativeInteger.class,
//                                                   Constants.XSD_NONNEGATIVEINTEGER),
//              new SimpleDeserializerFactory(org.apache.axis.types.NonNegativeInteger.class,
//                                            Constants.XSD_NONNEGATIVEINTEGER)
//         );

//         // a xsd:negativeInteger
//         myRegister(Constants.XSD_NEGATIVEINTEGER, org.apache.axis.types.NegativeInteger.class,
//              new SimpleSerializerFactory(org.apache.axis.types.NegativeInteger.class,
//                                                   Constants.XSD_NEGATIVEINTEGER),
//              new SimpleDeserializerFactory(org.apache.axis.types.NegativeInteger.class,
//                                            Constants.XSD_NEGATIVEINTEGER)
//         );

//         // a xsd:positiveInteger
//         myRegister(Constants.XSD_POSITIVEINTEGER, org.apache.axis.types.PositiveInteger.class,
//              new SimpleSerializerFactory(org.apache.axis.types.PositiveInteger.class,
//                                                   Constants.XSD_POSITIVEINTEGER),
//              new SimpleDeserializerFactory(org.apache.axis.types.PositiveInteger.class,
//                                            Constants.XSD_POSITIVEINTEGER)
//         );

//         // a xsd:nonPositiveInteger
//         myRegister(Constants.XSD_NONPOSITIVEINTEGER, org.apache.axis.types.NonPositiveInteger.class,
//              new SimpleSerializerFactory(org.apache.axis.types.NonPositiveInteger.class,
//                                                   Constants.XSD_NONPOSITIVEINTEGER),
//              new SimpleDeserializerFactory(org.apache.axis.types.NonPositiveInteger.class,
//                                            Constants.XSD_NONPOSITIVEINTEGER)
//         );

//         // a xsd:Name
//         myRegister(Constants.XSD_NAME, org.apache.axis.types.Name.class,
//                    new SimpleSerializerFactory(org.apache.axis.types.Name.class,
//                                              Constants.XSD_NAME),
//                    new SimpleDeserializerFactory(org.apache.axis.types.Name.class,
//                                              Constants.XSD_NAME)
//         );

//         // a xsd:NCName
//         myRegister(Constants.XSD_NCNAME, org.apache.axis.types.NCName.class,
//                    new SimpleSerializerFactory(org.apache.axis.types.NCName.class,
//                                              Constants.XSD_NCNAME),
//                    new SimpleDeserializerFactory(org.apache.axis.types.NCName.class,
//                                              Constants.XSD_NCNAME)
//         );

//          // a xsd:ID
//         myRegister(Constants.XSD_ID, org.apache.axis.types.Id.class,
//                    new SimpleSerializerFactory(org.apache.axis.types.Id.class,
//                                              Constants.XSD_ID),
//                    new SimpleDeserializerFactory(org.apache.axis.types.Id.class,
//                                              Constants.XSD_ID)
//         );

//         // a xsd:language
//         myRegister(Constants.XSD_LANGUAGE, org.apache.axis.types.Language.class,
//                    new SimpleSerializerFactory(org.apache.axis.types.Language.class,
//                                              Constants.XSD_LANGUAGE),
//                    new SimpleDeserializerFactory(org.apache.axis.types.Language.class,
//                                              Constants.XSD_LANGUAGE)
//         );

//         // a xml:lang
//         myRegister(Constants.XML_LANG, org.apache.axis.types.Language.class,
//                    new SimpleSerializerFactory(org.apache.axis.types.Language.class,
//                                              Constants.XML_LANG),
//                    new SimpleDeserializerFactory(org.apache.axis.types.Language.class,
//                                              Constants.XML_LANG)
//         );
        
//         // a xsd:NmToken
//         myRegister(Constants.XSD_NMTOKEN, org.apache.axis.types.NMToken.class,
//                    new SimpleSerializerFactory(org.apache.axis.types.NMToken.class,
//                                              Constants.XSD_NMTOKEN),
//                    new SimpleDeserializerFactory(org.apache.axis.types.NMToken.class,
//                                              Constants.XSD_NMTOKEN)
//         );

//         // a xsd:NmTokens
//         myRegister(Constants.XSD_NMTOKENS, org.apache.axis.types.NMTokens.class,
//                    new SimpleSerializerFactory(org.apache.axis.types.NMTokens.class,
//                                              Constants.XSD_NMTOKENS),
//                    new SimpleDeserializerFactory(org.apache.axis.types.NMTokens.class,
//                                              Constants.XSD_NMTOKENS)
//         );

//         // a xsd:NOTATION
//         myRegister(Constants.XSD_NOTATION, org.apache.axis.types.Notation.class,
//                    new BeanSerializerFactory(org.apache.axis.types.Notation.class,
//                                              Constants.XSD_NOTATION),
//                    new BeanDeserializerFactory(org.apache.axis.types.Notation.class,
//                                              Constants.XSD_NOTATION)
//         );

//         // a xsd:XSD_ENTITY
//         myRegister(Constants.XSD_ENTITY, org.apache.axis.types.Entity.class,
//                    new SimpleSerializerFactory(org.apache.axis.types.Entity.class,
//                                              Constants.XSD_ENTITY),
//                    new SimpleDeserializerFactory(org.apache.axis.types.Entity.class,
//                                              Constants.XSD_ENTITY)
//         );

//         // a xsd:XSD_ENTITIES
//         myRegister(Constants.XSD_ENTITIES, org.apache.axis.types.Entities.class,
//                    new SimpleSerializerFactory(org.apache.axis.types.Entities.class,
//                                              Constants.XSD_ENTITIES),
//                    new SimpleDeserializerFactory(org.apache.axis.types.Entities.class,
//                                              Constants.XSD_ENTITIES)
//         );

//         // a xsd:XSD_IDREF
//         myRegister(Constants.XSD_IDREF, org.apache.axis.types.IDRef.class,
//                    new SimpleSerializerFactory(org.apache.axis.types.IDRef.class,
//                                              Constants.XSD_IDREF),
//                    new SimpleDeserializerFactory(org.apache.axis.types.IDRef.class,
//                                              Constants.XSD_IDREF)
//         );

//         // a xsd:XSD_XSD_IDREFS
//         myRegister(Constants.XSD_IDREFS, org.apache.axis.types.IDRefs.class,
//                    new SimpleSerializerFactory(org.apache.axis.types.IDRefs.class,
//                                              Constants.XSD_IDREFS),
//                    new SimpleDeserializerFactory(org.apache.axis.types.IDRefs.class,
//                                              Constants.XSD_IDREFS)
//         );
        
//         // a xsd:Duration
//         myRegister(Constants.XSD_DURATION, org.apache.axis.types.Duration.class,
//                    new SimpleSerializerFactory(org.apache.axis.types.Duration.class,
//                                              Constants.XSD_DURATION),
//                    new SimpleDeserializerFactory(org.apache.axis.types.Duration.class,
//                                              Constants.XSD_DURATION)
//         );
        
//         // a xsd:anyURI
//         myRegister(Constants.XSD_ANYURI, org.apache.axis.types.URI.class,
//                    new SimpleSerializerFactory(org.apache.axis.types.URI.class,
//                                              Constants.XSD_ANYURI),
//                    new SimpleDeserializerFactory(org.apache.axis.types.URI.class,
//                                              Constants.XSD_ANYURI)
//         );

//         // a xsd:schema
//         myRegister(Constants.XSD_SCHEMA, org.apache.axis.types.Schema.class,
//                    new BeanSerializerFactory(org.apache.axis.types.Schema.class,
//                                              Constants.XSD_SCHEMA),
//                    new BeanDeserializerFactory(org.apache.axis.types.Schema.class,
//                                              Constants.XSD_SCHEMA)
//         );
        
        // TODO: move it to the DefaultSOAPEncodingTypeMappingImpl class
        myRegister(Constants.SOAP_ARRAY12,     java.util.Collection.class,
                   new ArraySerializerFactory(),
                   new SWFArrayDeserializerFactory()
        );
        // TODO: move it to the DefaultSOAPEncodingTypeMappingImpl class
        myRegister(Constants.SOAP_ARRAY12,     java.util.ArrayList.class,
                   new ArraySerializerFactory(),
                   new SWFArrayDeserializerFactory()
        );

        myRegister(Constants.SOAP_ARRAY12,     Object[].class,
                   new ArraySerializerFactory(),
                   new SWFArrayDeserializerFactory()
        );

        myRegister(Constants.SOAP_ARRAY,     java.util.ArrayList.class,
                   new ArraySerializerFactory(),
                   new SWFArrayDeserializerFactory()
        );
        
        // All array objects automatically get associated with the SOAP_ARRAY.
        // There is no way to do this with a hash table,
        // so it is done directly in getTypeQName.
        // Internally the runtime uses ArrayList objects to hold arrays...
        // which is the reason that ArrayList is associated with SOAP_ARRAY.
        // In addition, handle all objects that implement the List interface
        // as a SOAP_ARRAY
        myRegister(Constants.SOAP_ARRAY,     java.util.Collection.class,
                   new ArraySerializerFactory(),
                   new SWFArrayDeserializerFactory()
        );

        myRegister(Constants.SOAP_ARRAY,     Object[].class,
                   new ArraySerializerFactory(),
                   new SWFArrayDeserializerFactory()
        );

        //
        // Now register the schema specific types
        //
        SchemaVersion.SCHEMA_1999.registerSchemaSpecificTypes(this);
        SchemaVersion.SCHEMA_2000.registerSchemaSpecificTypes(this);
        SchemaVersion.SCHEMA_2001.registerSchemaSpecificTypes(this);

        SerializerFactory sf = new SimpleSerializerFactory(java.util.Date.class, Constants.XSD_DATETIME);
        DeserializerFactory df = new SWFSimpleDeserializerFactory(java.util.Date.class, Constants.XSD_DATETIME);
        myRegister(Constants.XSD_DATETIME, java.util.Date.class, sf, df);
        
        doneInit = true;
    }

    protected void myRegisterSimple(QName xmlType, Class javaType) {
        SerializerFactory sf = new SimpleSerializerFactory(javaType, xmlType);
        DeserializerFactory df = null;
        if (javaType != java.lang.Object.class) {
            df = new SWFSimpleDeserializerFactory(javaType, xmlType);
        }
        myRegister(xmlType, javaType, sf, df);
    }

    /**
     * Construct TypeMapping for all the [xmlType, javaType] for all of the
     * known xmlType namespaces.  This is the shotgun approach, which works
     * in 99% of the cases.  The other cases that are Schema version specific
     * (i.e. timeInstant vs. dateTime) are handled by the SchemaVersion
     * Interface registerSchemaSpecificTypes().
     *
     * @param xmlType is the QName type
     * @param javaType is the java type
     * @param sf is the ser factory (if null, the simple factory is used)
     * @param df is the deser factory (if null, the simple factory is used)
     */
    protected void myRegister(QName xmlType, Class javaType,
                              SerializerFactory sf, DeserializerFactory df) {
        // Register all known flavors of the namespace.
        try {
            if (xmlType.getNamespaceURI().equals(
                    Constants.URI_DEFAULT_SCHEMA_XSD)) {
                for (int i=0; i < Constants.URIS_SCHEMA_XSD.length; i++) {
                    QName qName = new QName(Constants.URIS_SCHEMA_XSD[i],
                                            xmlType.getLocalPart());
                    super.internalRegister(javaType, qName, sf, df);
                }
            }
            else if (xmlType.getNamespaceURI().equals(
                    Constants.URI_DEFAULT_SOAP_ENC)) {
                for (int i=0; i < Constants.URIS_SOAP_ENC.length; i++) {
                    QName qName = new QName(Constants.URIS_SOAP_ENC[i],
                                            xmlType.getLocalPart());
                    super.internalRegister(javaType, qName, sf, df);
                }
            } else {
                // Register with the specified xmlType.
                // This is the prefered mapping and the last registed one wins
                super.internalRegister(javaType, xmlType, sf, df);
            }
        } catch (JAXRPCException e) { }
    }

    // Don't allow anyone to muck with the default type mapping because
    // it is a singleton used for the whole system.
    public void register(Class javaType, QName xmlType,
                         javax.xml.rpc.encoding.SerializerFactory sf,
                         javax.xml.rpc.encoding.DeserializerFactory dsf)
        throws JAXRPCException {
        // Don't allow anyone but init to modify us.
        super.register(javaType, xmlType, sf, dsf);
    }

    public void removeSerializer(Class javaType, QName xmlType)
        throws JAXRPCException {
        throw new JAXRPCException(Messages.getMessage("fixedTypeMapping"));
    }
    public void removeDeserializer(Class javaType, QName xmlType)
        throws JAXRPCException {
        throw new JAXRPCException(Messages.getMessage("fixedTypeMapping"));
    }
    public void setSupportedEncodings(String[] namespaceURIs) {
    }
}
