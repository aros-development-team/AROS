/*
 * $Id: GenericCache.java,v 1.5 2002/06/12 23:54:14 skavish Exp $
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

import java.util.*;

public abstract class GenericCache {

    protected Hashtable cache = new Hashtable();
    protected int cacheSize = 0;
    protected CacheSettings settings = new CacheSettings();

    public CacheSettings getMySettings() {
        return settings;
    }

    public int getSize() {
        return cacheSize;
    }

    public synchronized void clear() {
        cache.clear();
        cacheSize = 0;
    }

    protected synchronized boolean addItem( CacheItem item ) {
        //Log.debug("adding item to cache: "+getClass().getName()+", cachesize="+cacheSize+", key='"+item.getKey()+"', expireAfter="+(new Date(item.getExpireAfter()))+", size="+item.getSize() );
        if( !checkCacheSize( item.getSize() ) ) {
            //Log.debug("  ... failed. cachesize="+cacheSize);
            return false;
        }
        cache.put( item.getKey(), item );
        cacheSize += item.getSize();
        //Log.debug("  ... succeed. cachesize="+cacheSize);
        return true;
    }

    protected synchronized CacheItem getItem( Object key ) {
        CacheItem item = (CacheItem) cache.get( key );
        if( item == null ) return null;
        if( !isModified( item ) &&
            System.currentTimeMillis() <= item.getExpireAfter()
          ) return item;

        // modified or expired - remove from the cache
        removeItem( item );
        return null;
    }

    protected boolean isModified( CacheItem item ) {
        return false;
    }

    /**
     * Returns true if there is enough place
     */
    protected boolean checkCacheSize( int size ) {
        if( cacheSize + size <= settings.getMaxSize() ) return true;
        if( !settings.isRecycle() ) return false;
        recycle( size );
        return cacheSize + size <= settings.getMaxSize();
    }

    /**
     * Try to recycle at least 'size' bytes
     */
    protected synchronized void recycle( int size ) {
        if( cache.size() == 0 ) return;
        //Log.log( Resource.STR, "Recycling cache("+cacheSize+")" );

        // sort list in ascending order, so older elements come first
        ArrayList l = new ArrayList( cache.values() );
        Collections.sort( l, new Comparator() {
                public int compare( Object o1, Object o2 ) {
                    return (int) (((CacheItem)o1).getCacheTime() - ((CacheItem)o2).getCacheTime());
                }
            }
        );

        // recycle cache and remove stale objects at the same time
        long now = System.currentTimeMillis();
        for( int i=0; i<l.size(); i++ ) {
            CacheItem item = (CacheItem) l.get(i);
            if( size > 0 || now > item.getExpireAfter() ) {
                size -= item.getSize();
                removeItem( item );
            }
        }
        //Log.log( Resource.STR, "Recycling cache("+cacheSize+") DONE" );
    }

    protected synchronized void removeItem( CacheItem item ) {
        cacheSize -= item.getSize();
        cache.remove( item.getKey() );
    }

}
