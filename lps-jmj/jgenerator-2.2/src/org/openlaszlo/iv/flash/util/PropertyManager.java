/*
 * $Id: PropertyManager.java,v 1.7 2002/07/15 02:15:03 skavish Exp $
 *
 * ===========================================================================
 *
 * The JGenerator Software License, Version 1.0
 *
 * Copyright (c) 2000 Dmitry Skavish (skavish@usa.net). All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * 3. The end-user documentation included with the redistribution, if
 *    any, must include the following acknowlegement:
 *    "This product includes software developed by Dmitry Skavish
 *     (skavish@usa.net, http://www.flashgap.com/)."
 *    Alternately, this acknowlegement may appear in the software itself,
 *    if and wherever such third-party acknowlegements normally appear.
 *
 * 4. The name "The JGenerator" must not be used to endorse or promote
 *    products derived from this software without prior written permission.
 *    For written permission, please contact skavish@usa.net.
 *
 * 5. Products derived from this software may not be called "The JGenerator"
 *    nor may "The JGenerator" appear in their names without prior written
 *    permission of Dmitry Skavish.
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESSED OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL DMITRY SKAVISH OR THE OTHER
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

package org.openlaszlo.iv.flash.util;

import org.openlaszlo.iv.flash.api.*;

import org.openlaszlo.iv.flash.cache.*;
import java.io.*;
import java.net.*;
import java.util.*;

/**
 * Property manager.
 * <P>
 * Reads and writes properties from iv.property file.
 * Caches most used properties as static variables
 *
 * @author Dmitry Skavish
 */
public class PropertyManager {

    /**
     * Property org.openlaszlo.iv.flash.varCaseSensitive
     */
    public static boolean varCaseSensitive = true;
    /**
     * Property org.openlaszlo.iv.flash.symCaseSensitive
     */
    public static boolean symCaseSensitive = false;
    /**
     * Property org.openlaszlo.iv.flash.showErrorsInline
     */
    public static boolean showErrorsInline = true;
    /**
     * Property org.openlaszlo.iv.flash.textBoundsMMStyle
     */
    public static boolean textMMStyle      = false;
    /**
     * Property org.openlaszlo.iv.flash.fontPath
     */
    public static String fontPath          = "fonts/";
    /**
     * Property org.openlaszlo.iv.flash.defaultEncoding
     */
    public static String defaultEncoding   = null;
    /**
     * Property org.openlaszlo.iv.flash.mxLibrarySymbolPrefix
     */
    public static String mxLibrarySymbolPrefix = "__";
    /**
     * Property org.openlaszlo.iv.flash.mxLibraryFontID
     */
    public static String mxLibraryFontID = "[jgen]";

    private static Properties myProperties;

    private static void loadCacheProperties( CacheSettings cs, String prefix ) {
        cs.setMaxSize( getIntProperty( prefix+"CacheMaxSize", 0 ) );
        cs.setDefaultExpire( (long) (getDoubleProperty( prefix+"CacheDefaultExpire", 0.0 )*1000) );
        cs.setForce( getBoolProperty( prefix+"CacheForce", false ) );
        cs.setRecycle( getBoolProperty( prefix+"CacheRecycle", false ) );
        cs.setCheckModifiedSince( getBoolProperty( prefix+"CacheCheckModifiedSince", false ) );
    }

    private static void saveCacheProperties( CacheSettings cs, String prefix ) {
        setProperty( prefix+"CacheMaxSize", cs.getMaxSize() );
        setProperty( prefix+"CacheDefaultExpire", cs.getDefaultExpire()/1000.0 );
        setProperty( prefix+"CacheForce", cs.isForce() );
        setProperty( prefix+"CacheRecycle", cs.isRecycle() );
        setProperty( prefix+"CacheCheckModifiedSince", cs.isCheckModifiedSince() );
    }

    /**
     * Initalizes manager.
     * <P>
     * Reads and caches all the properties
     *
     * @param propFileName iv.properties file name - can be absolute name
     */
    public static void init( String propFileName ) {
        if( propFileName == null ) propFileName = "iv.properties";
        load(propFileName);
        varCaseSensitive = getBoolProperty( "org.openlaszlo.iv.flash.varCaseSensitive", true );
        symCaseSensitive = getBoolProperty( "org.openlaszlo.iv.flash.symCaseSensitive", false );
        showErrorsInline = getBoolProperty( "org.openlaszlo.iv.flash.showErrorsInline", true );
        textMMStyle      = getBoolProperty( "org.openlaszlo.iv.flash.textBoundsMMStyle", false );
        fontPath         = getProperty( "org.openlaszlo.iv.flash.fontPath", "fonts/" );
        defaultEncoding  = getProperty( "org.openlaszlo.iv.flash.defaultEncoding" );
        mxLibrarySymbolPrefix = getProperty( "org.openlaszlo.iv.flash.mxLibrarySymbolPrefix", "__" );
        mxLibraryFontID = getProperty( "org.openlaszlo.iv.flash.mxLibraryFontID", "[jgen]" );
        if( defaultEncoding != null ) {
            defaultEncoding = defaultEncoding.trim();
            if( defaultEncoding.length() == 0 || Util.isDefault(defaultEncoding) ) {
                defaultEncoding = null;
            }
        }
    }

    /**
     * Initalizes manager.
     * <P>
     * Reads and caches all the properties
     */
    public static void init() {
        init("iv.properties");
    }

    /**
     * Loads iv.properties file and caches "cache" related properties.
     */
    public static void load( String propFileName ) {
        myProperties = new Properties();
        try {
            myProperties.load( new FileInputStream( Util.getSysFile( propFileName ) ) );
        } catch( Exception e ) {
            try {
                myProperties.load( Util.getResource("/iv.properties").openStream() );
            } catch( Exception e1 ) {
                Log.error(Resource.get(Resource.CANTLOADPROPERTIES), e1);
                myProperties = new Properties();
            }
        }
        loadCacheProperties( getRequestCacheSettings(), "org.openlaszlo.iv.flash.request" );
        loadCacheProperties( getFontCacheSettings(), "org.openlaszlo.iv.flash.font" );
        loadCacheProperties( getMediaCacheSettings(), "org.openlaszlo.iv.flash.media" );
        loadCacheProperties( getXMLCacheSettings(), "org.openlaszlo.iv.flash.xml" );
    }

    /**
     * Saves properties to iv.properties file.
     *
     * @exception IOException
     */
    public static void save( String propFileName ) throws IOException {
        saveCacheProperties( getRequestCacheSettings(), "org.openlaszlo.iv.flash.request" );
        saveCacheProperties( getFontCacheSettings(), "org.openlaszlo.iv.flash.font" );
        saveCacheProperties( getMediaCacheSettings(), "org.openlaszlo.iv.flash.media" );
        saveCacheProperties( getXMLCacheSettings(), "org.openlaszlo.iv.flash.xml" );
        setProperty( "org.openlaszlo.iv.flash.varCaseSensitive", varCaseSensitive );
        setProperty( "org.openlaszlo.iv.flash.symCaseSensitive", symCaseSensitive );
        setProperty( "org.openlaszlo.iv.flash.showErrorsInline", showErrorsInline );
        setProperty( "org.openlaszlo.iv.flash.textBoundsMMStyle", textMMStyle );
        setProperty( "org.openlaszlo.iv.flash.fontPath", fontPath );
        if( defaultEncoding != null ) setProperty( "org.openlaszlo.iv.flash.defaultEncoding", defaultEncoding );
        // has to be replaced with store
        myProperties.save( new FileOutputStream( Util.getSysFile(propFileName) ), "" );
    }

    /**
     * Sets property by name.
     *
     * @param name   name of the property
     * @param value  value of the property
     */
    public static void setProperty( String name, String value ) {
        myProperties.put(name, value);
    }

    /**
     * Sets integer property by name.
     *
     * @param name   name of the property
     * @param value  value of the property
     */
    public static void setProperty( String name, int value ) {
        myProperties.put(name, new Integer(value).toString());
    }

    /**
     * Sets boolean property by name.
     *
     * @param name   name of the property
     * @param value  value of the property
     */
    public static void setProperty( String name, boolean value ) {
        myProperties.put(name, new Boolean(value).toString());
    }

    /**
     * Sets double property by name.
     *
     * @param name   name of the property
     * @param value  value of the property
     */
    public static void setProperty( String name, double value ) {
        myProperties.put(name, new Double(value).toString());
    }

    /**
     * Gets property by name.
     *
     * @param name   name of the property
     * @param def    default value to be returned if there is no such property
     * @return value of the property or provided default value
     */
    public static String getProperty( String name, String def ) {
        String value = System.getProperty(name);
        if( value != null ) return value;
        return myProperties.getProperty(name, def);
    }

    /**
     * Gets property by name.
     *
     * @param name   name of the property
     * @return value of the property or null
     */
    public static String getProperty( String name ) {
        String value = System.getProperty(name);
        if( value != null ) return value;
        return myProperties.getProperty(name);
    }

    /**
     * Gets integer property by name.
     *
     * @param name   name of the property
     * @param def    default value to be returned if there is no such property
     * @return value of the property or provided default value
     */
    public static int getIntProperty( String property, int def ) {
        return Util.toInt( getProperty(property), def );
    }

    /**
     * Gets long property by name.
     *
     * @param name   name of the property
     * @param def    default value to be returned if there is no such property
     * @return value of the property or provided default value
     */
    public static long getLongProperty( String property, long def ) {
        return Util.toLong( getProperty(property), def );
    }

    /**
     * Gets boolean property by name.
     *
     * @param name   name of the property
     * @param def    default value to be returned if there is no such property
     * @return value of the property or provided default value
     */
    public static boolean getBoolProperty( String property, boolean def ) {
        return Util.toBool( getProperty(property), def );
    }

    /**
     * Gets double property by name.
     *
     * @param name   name of the property
     * @param def    default value to be returned if there is no such property
     * @return value of the property or provided default value
     */
    public static double getDoubleProperty( String property, double def ) {
        return Util.toDouble( getProperty(property), def );
    }

    public static Object getObjectProperty( Object key ) {
        return myProperties.get(key);
    }

    public static void setObjectProperty( Object key, Object value ) {
        myProperties.put(key, value);
    }

    /**
     * Returns request cache settings.
     *
     * @return request cache settings
     */
    public static CacheSettings getRequestCacheSettings() {
        return RequestCache.getSettings();
    }

    /**
     * Returns font cache settings.
     *
     * @return font cache settings
     */
    public static CacheSettings getFontCacheSettings() {
        return FontCache.getSettings();
    }

    /**
     * Returns media cache settings.
     *
     * @return media cache settings
     */
    public static CacheSettings getMediaCacheSettings() {
        return MediaCache.getSettings();
    }

    /**
     * Returns xml cache settings.
     *
     * @return xml cache settings
     */
    public static CacheSettings getXMLCacheSettings() {
        return XMLCache.getSettings();
    }

}
