/*
 * $Id: FlashOutput.java,v 1.3 2002/05/16 05:08:41 skavish Exp $
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
import org.openlaszlo.iv.flash.api.text.*;

/**
 * This class is basically a {@link FlashBuffer} which
 * tracks flash definitions written into the buffer and
 * provides unique IDs for them.
 * <P>
 * The class also supports hierarchy of flash buffers
 *
 * @author Dmitry Skavish
 * @see FlashBuffer
 */
public class FlashOutput extends FlashBuffer {

    // definitions caches
    private FlashItem jpegTables;
    private Hashtable hashTable = new Hashtable();
    private FlashOutput main;
    private int currentID = 1;
    private Object userData;
    private FlashFile file;

    public FlashOutput( int size ) {
        super(size);
    }

    public FlashOutput( FlashFile file, int size ) {
        super(size);
        this.file = file;
    }

    public FlashOutput( FlashOutput main, int size ) {
        super(size);
        this.main = main;
    }

    public FlashFile getFlashFile() {
        if( main != null ) return main.getFlashFile();
        return file;
    }

    /**
     * Gets ID of flash definition.
     *
     * @param def    flash definition ID of which is requisted
     * @return unique ID
     */
    public int getDefID( FlashDef def ) {
        return getObjID( def );
    }

    private synchronized int getObjID( Object key ) {
        if( main != null ) return main.getObjID( key );
        Integer id = (Integer) hashTable.get( key );
        if( id != null ) return id.intValue();
        hashTable.put( key, new Integer(currentID) );
        /*if( key instanceof FlashDef ) {
            FlashDef def = (FlashDef) key;
            System.out.println( "FlashOutput.getDefID: oldID="+def.getID()+", newID="+currentID );
        }*/
        return currentID++;
    }

    public synchronized boolean defined( Object key ) {
        if (main != null) return main.defined(key);
        return (hashTable.get( key ) != null );
    }

    /**
     * Writes ID of flash definiton into the buffer.
     *
     * @param def    flash definiton, ID of which will be written
     */
    public void writeDefID( FlashDef def ) {
        writeWord( getObjID( def ) );
    }

    /**
     * Writes ID of font into the buffer.
     *
     * @param font    font, ID of which will be written
     */
    public void writeFontID( Font font ) {
        writeWord( getObjID( font ) );
    }

    public void writeJPegTables( FlashItem jt ) {
        if( main != null ) {
            main.writeJPegTables( jt );
        } else {
            if( jpegTables != null ) return;
            jpegTables = jt;
            jt.write( this );
        }
    }

    public void setUserData( Object data ) {
        userData = data;
    }

    public Object getUserData() {
        return userData;
    }
}
