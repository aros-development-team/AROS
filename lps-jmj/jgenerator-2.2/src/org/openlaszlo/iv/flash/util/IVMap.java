/*
 * $Id: IVMap.java,v 1.2 2002/02/15 23:44:28 skavish Exp $
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

import java.io.*;
import java.util.*;

import org.openlaszlo.iv.flash.api.*;

/**
 * Simple unsynchronized hashtable with integer keys.
 *
 * @author Dmitry Skavish
 */
public final class IVMap {

    private static final int DEFAULT_SIZE = 257;
    private static final float DEFAULT_RATIO = 0.75f;

    private MyNode[] buckets;
    private int size;
    private int limit;
    private int length;

    /**
     * Create empty hashtable.
     */
    public IVMap() {
        clear();
    }

    /**
     * Add flash definition to the hashtable by it's ID.
     *
     * @param def    flash definition to add
     */
    public void add( FlashDef def ) {
        put( def.getID(), def );
    }

    /**
     * Add flash definition to the hashtable by integer key.
     *
     * @param key    object's key
     * @param def    flash definition to add
     */
    public void put( int key, FlashDef def ) {
        int probe = key % length;
        MyNode newNode = new MyNode( key, def );
        newNode.next = buckets[probe];
        buckets[probe] = newNode;
        if( size++ > limit ) expand();
    }

    /**
     * Retrieve flash definition by integer key.
     *
     * @param key    defintion's key
     * @return found flash defintion or null
     */
    public FlashDef get( int key ) {
        for( MyNode node = buckets[key%length]; node != null; node = node.next ) {
            if( key == node.key ) return node.value;
        }
        return null;
    }

    /**
     * Number of objects in the hashtable.
     *
     * @return number of objects
     */
    public int size() {
        return size;
    }

    /**
     * Enumeration of all the values (FlashDef's) of the hashtable.
     *
     * @return values of the hashtable
     */
    public Enumeration values() {
        return new Enumeration() {
            int cur = 0;
            MyNode node = null;
            public boolean hasMoreElements() {
                while( node == null ) {
                    if( cur >= length ) return false;
                    node = buckets[cur++];
                 }
                 return true;
             }
             public Object nextElement() {
                 if( !hasMoreElements() ) return null;
                 Object value = node.value;
                 node = node.next;
                 return value;
             }
        };
    }

    /**
     * Clear hashtable.
     */
    public void clear() {
        length = DEFAULT_SIZE;
        buckets = new MyNode[length];
        size = 0;
        limit = (int) (length*DEFAULT_RATIO);
    }

    protected void expand() {
        length = length*2+1;
        MyNode[] new_bucket = new MyNode[length];
        for( int i=buckets.length; --i>=0; ) {
            MyNode node = buckets[i];
            while( node != null ) {
                MyNode cnode = node;
                node = node.next;
                int probe = cnode.key % length;
                cnode.next = new_bucket[probe];
                new_bucket[probe] = cnode;
            }
        }
        limit = (int) (length*DEFAULT_RATIO);
        buckets = new_bucket;
    }

    static class MyNode {
        public int key;
        public FlashDef value;
        public MyNode next;

        public MyNode( int key, FlashDef value ) {
            this.key = key;;
            this.value = value;
        }
    }

}
