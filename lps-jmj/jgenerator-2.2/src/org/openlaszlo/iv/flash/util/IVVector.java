/*
 * $Id: IVVector.java,v 1.2 2002/02/15 23:44:28 skavish Exp $
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

import java.util.Enumeration;
import java.io.*;
import org.openlaszlo.iv.flash.api.FlashItem;

/**
 * Simple unsynchronized vector.
 *
 * @author Dmitry Skavish
 */
public class IVVector implements Cloneable {

    protected Object[] objects;
    protected int top;
    protected static final int INIT_CAPACITY = 20;

    /**
     * Creates emtpy vector with default capacity.
     */
    public IVVector() {
        this( INIT_CAPACITY );
    }

    /**
     * Creates empty vector with specified capacity.
     *
     * @param capacity initial capacity
     */
    public IVVector( int capacity ) {
        init( capacity );
    }

    /**
     * Creates vector from existing one.
     * <P>
     * Creates vector with capacity equal to the size of given vector
     * and copies all data from given vector to this one
     *
     * @param data   vector to copy from
     */
    public IVVector( IVVector data ) {
        init( data.size()+1, data.size() );
        System.arraycopy(data.objects, 0, objects, 0, top);
    }

    /**
     * Ensure capacity of this vector.<p>
     * Increases the capacity of this vector, if necessary, to ensure
     * that it can hold at least the number of objects specified by
     * the argument.
     *
     * @param cap    new capacity
     */
    public final void ensureCapacity( int cap ) {
        if( cap >= objects.length ) {
            Object[] newObjs = new Object[cap*2];
            System.arraycopy(objects, 0, newObjs, 0, objects.length);
            objects = newObjs;
        }
    }

    /**
     * Adds the specified object to the end of this vector,
     * increasing its size by one. The capacity of this vector is
     * increased if its size becomes greater than its capacity.
     *
     * @param o      object to be added
     */
    public final void addElement( Object o ) {
        ensureCapacity( top );
        objects[top++] = o;
    }

    /**
     * Sets the object at the specified <code>index</code> of this
     * vector to be the specified object. The previous object at that
     * position is discarded.
     *
     * @param o      new object to be set at the index
     * @param index  the specified index
     */
    public final void setElementAt( Object o, int index ) {
        ensureCapacity( index );
        objects[index] = o;
        if( index >= top ) top = index+1;
    }

    /**
     * Returns the object at the specified index.
     *
     * @param index  the specified index
     * @return object at specified index
     */
    public final Object elementAt( int index ) {
        if( index >= top ) {
            throw new ArrayIndexOutOfBoundsException( index + " >= " + top );
        }
        return objects[index];
    }

    /**
     * Removes the object at the specified index.
     *
     * @param index  the specified index
     * @return removed object at specified index
     */
    public final Object removeElementAt( int index ) {
        if( index >= top ) {
            throw new ArrayIndexOutOfBoundsException( index + " >= " + top );
        }
        Object o = objects[index];
        --top;
        if( index < top ) {
            System.arraycopy( objects, index+1, objects, index, top-index );
        }
        objects[top] = null;
        return o;
    }

    /**
     * Removes the object from the vector.
     * <p>
     * Finds the specified object in the vector and removes it.
     *
     * @param o      the specified object
     */
    public final void removeElement( Object o ) {
        removeElementAt( find(o) );
    }

    /**
     * Removes a number of objects.
     *
     * @param from   first object to be removed from the vector
     * @param to     last object to be removed from the vector
     */
    public final void removeRange( int from, int to ) {
        for( int i=from; i<to; i++ ) {
            removeElementAt( i );
        }
    }

    /**
     * Inserts specified number of null objects beginning from specified index.
     *
     * @param from   inserts nulls beginning from this index (including)
     * @param num    number of null objects to be inserted
     */
    public final void insertObjects( int from, int num ) {
        if( from > top ) { // has to be '>'
            throw new ArrayIndexOutOfBoundsException( from + " > " + top );
        }
        ensureCapacity( top+num );
        if( from < top ) {
            System.arraycopy( objects, from, objects, from+num, top-from );
        }
        top += num;
    }

    /**
     * Returns size of the vector.
     *
     * @return size of the vector (number of objects in it)
     */
    public final int size() {
        return top;
    }

    /**
     * Resets vector.
     * <P>
     * Removes all objects from the vector, but does not change the capacity
     */
    public final void reset() {
        top = 0;
    }

    /**
     * Clears vector.
     * <P>
     * Removes all objects from the vector and fills it with nulls,
     * but does not change the capacity
     */
    public final void clear() {
        for( int i=0; i<objects.length; i++ ) {
            objects[i] = null;
        }
        top = 0;
    }

    /**
     * Copies objects of the vector into specified array.
     *
     * @param cobjects array of objects to be copied to
     */
    public final void copyInto( Object[] cobjects ) {
        System.arraycopy(objects, 0, cobjects, 0, top);
    }

    /**
     * Returns enumeration of all the objects of the vector.
     *
     * @return enumeration of all the objects
     */
    public final Enumeration elements() {
        return new Enumeration() {
            private int cur = 0;
            public boolean hasMoreElements() {
                return cur < top;
            }
            public Object nextElement() {
                return objects[cur++];
            }
        };
    }

    protected final int find( Object o ) {
        for( int i=top; --i>=0; ) {
            if( objects[i] == o ) return i;
        }
        return -1;
    }

    protected final void init( int capacity ) {
        init( capacity, 0 );
    }

    protected final void init( int capacity, int top ) {
        this.top = top;
        if( capacity <= 0 ) capacity = 1;
        objects = new Object[capacity];
    }

    /**
     * Clones this vector.
     *
     * @return new vector - clone of this one
     */
    public Object clone() {
        IVVector v = new IVVector( top );
        for( int i=0; i<top; i++ ) {
            v.setElementAt( objects[i], i );
        }
        return v;
    }

    /**
     * Creates copy of this vector in jgenerator sense.
     * <P>
     * Assuming that this vector contains only {@link org.openlaszlo.iv.flash.api.FlashItem}s,
     * creates copy of this vector which contains copies of all the FlashItems
     * from this vector.
     *
     * @param copier copier to be used when copying FlashItems
     * @return jgenerator copy of this vector
     * @see org.openlaszlo.iv.flash.api.FlashItem#getCopy
     */
    public IVVector getCopy( ScriptCopier copier ) {
        IVVector v = new IVVector( top );
        for( int i=0; i<top; i++ ) {
            v.setElementAt( ((FlashItem)objects[i]).getCopy(copier), i );
        }
        return v;
    }

    /**
     * Prints content of this vector.
     * <P>
     * For each {@link org.openlaszlo.iv.flash.api.FlashItem} in this vector call it's
     * {@link org.openlaszlo.iv.flash.api.FlashItem#printContent} method
     *
     * @param out    printstream to print to
     * @param indent indentation
     * @see org.openlaszlo.iv.flash.api.FlashItem#printContent
     */
    public void printContent( PrintStream out, String indent ) {
        for( int i=0; i<top; i++ ) {
            ((FlashItem)objects[i]).printContent( out, indent );
        }
    }

    /**
     * Writes content of this vector to flash buffer.
     * <P>
     * For each {@link org.openlaszlo.iv.flash.api.FlashItem} in this vector call it's
     * {@link org.openlaszlo.iv.flash.api.FlashItem#write} method
     *
     * @param fob    flash buffer to write
     * @see org.openlaszlo.iv.flash.api.FlashItem#write
     */
    public void write( FlashOutput fob ) {
        for( int i=0; i<top; i++ ) {
            ((FlashItem)objects[i]).write( fob );
        }
    }
}

