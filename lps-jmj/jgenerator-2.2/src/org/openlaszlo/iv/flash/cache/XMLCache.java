/*
 * $Id: XMLCache.java,v 1.3 2002/03/22 04:55:07 skavish Exp $
 *
 * ==========================================================================
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

package org.openlaszlo.iv.flash.cache;

import org.openlaszlo.iv.flash.api.*;
import org.openlaszlo.iv.flash.util.*;
import org.openlaszlo.iv.flash.url.*;
import java.util.*;
import java.io.*;

import org.w3c.dom.Document;

/**
 * Cache for parsed XML files
 */
public class XMLCache extends GenericCache {

    private static XMLCache instance = new XMLCache();

    private XMLCache() {
    }

    public static XMLCache getInstance() {
        return instance;
    }

    public static CacheSettings getSettings() {
        return instance.getMySettings();
    }

    /**
     * Get cached Document
     *
     * @param key cache key
     */
     public static Document getXMLDocument( IVUrl url ) {
        return getXMLDocument( url.getName() );
     }

    /**
     * Get cached Document
     *
     * @param key cache key
     */
     public static Document getXMLDocument( String key ) {
        CacheItem item = instance.getItem( key );
        if( item == null ) return null;
        return (Document) item.getObject();
     }

    /**
     * Add Document to cache
     *
     * @param url url of xml doc to cache
     * @param doc xml document
     */
    public static void addXMLDocument( IVUrl url, Document doc ) {
        // check whether we need to cache or not
        long lifespan = getSettings().getDefaultExpire();
        if( !getSettings().isForce() ) {
            String gxcStr = url.getParameter( "gxc" );
            if( gxcStr != null ) {
                if( !Util.toBool(gxcStr, false) ) return;
                String v = url.getParameter( "gxe" );
                if( v != null ) {
                    lifespan = Util.toLong( v, -1L )*1000L;
                }
            } else {
                return;
            }
        }
        long now = System.currentTimeMillis();
        if( lifespan <= 0 ) {
            lifespan = Long.MAX_VALUE-now;
        }
        long expire = now + lifespan;
        CacheItem item = new XMLCacheItem( url, doc, now, expire, getSettings().isCheckModifiedSince() );
        instance.addItem( item );
    }

    protected boolean isModified( CacheItem item ) {
        if( !getSettings().isCheckModifiedSince() ) return super.isModified( item );
        XMLCacheItem it = (XMLCacheItem) item;
        it.getUrl().refresh();
        return it.lastModified() != it.getUrl().lastModified();
    }

    public static class XMLCacheItem extends CacheItem {
        private long lastModified = 0;
        private IVUrl url;

        public XMLCacheItem( IVUrl url, Document doc, long cacheTime, long expireAfter, boolean isCheckModifiedSince ) {
            super( url.getName(), doc, 1, cacheTime, expireAfter );
            this.url = url;
            if( isCheckModifiedSince ) {
                this.lastModified = url.lastModified();
            }
        }

        public IVUrl getUrl() { return url; }
        public long lastModified() { return lastModified; }
    }

}
