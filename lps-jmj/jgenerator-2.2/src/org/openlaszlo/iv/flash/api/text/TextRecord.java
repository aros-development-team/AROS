/*
 * $Id: TextRecord.java,v 1.3 2002/04/15 00:49:45 skavish Exp $
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

package org.openlaszlo.iv.flash.api.text;

import org.openlaszlo.iv.flash.parser.*;
import org.openlaszlo.iv.flash.util.*;
import org.openlaszlo.iv.flash.api.*;
import java.io.*;
import java.util.*;

/**
 * TextRecord.
 * <p>
 * Defines a text of one style.
 *
 * @author Dmitry Skavish
 * @see TextStyleChangeRecord
 */
public final class TextRecord extends FlashItem {

    private int size;           // size of the text (number of characters)
    private char[] text;        // the text itself ('size' characters)
    private int[] indexes;      // indexes of the text in font's codetable
    private int[] advances;     // advance values of the text in TWIXELS!

    public TextRecord() {}

    /**
     * Creates text record of zero size and of specified capacity
     *
     * @param maxSize capacity of created text record
     */
    public TextRecord( int maxSize ) {
        text = new char[ maxSize ];
        indexes = new int[ maxSize ];
        advances = new int[ maxSize ];
        size = 0;
    }

//    public void setText( char[] text ) { this.text = text; }
//    public void setIndexes( int[] indexes ) { this.indexes = indexes; }
//    public void setAdvances( int[] advances ) { this.advances = advances; }

    public char[] getText() { return text; }
    public int[] getIndexes() { return indexes; }
    public int[] getAdvances() { return advances; }

    /**
     * Returns character at specified index
     *
     * @param i      index of the character to be returned
     * @return character at specified index
     */
    public char getChar( int i ) {
        return text[i];
    }

    /**
     * Returns character's index at specified index
     *
     * @param i      index of the character's index to be returned
     * @return character's index at specified index
     */
    public int getIndex( int i ) {
        return indexes[i];
    }

    /**
     * Returns advance value at specified index
     *
     * @param i      index of the advance value to be returned
     * @return advance value at specified index
     */
    public int getAdvance( int i ) {
        return advances[i];
    }

    /**
     * Sets new character at specified index
     *
     * @param i      index of the character to be set
     * @param ch     character to be set
     */
    public void setChar( int i, char ch ) {
        text[i] = ch;
    }

    /**
     * Sets new character's index at specified index
     *
     * @param i      index of the character's index to be set
     * @param index  character's index to be set
     */
    public void setIndex( int i, int index ) {
        indexes[i] = index;
    }

    /**
     * Sets new advance value at specified index
     *
     * @param i      index of the advance value to be set
     * @param ch     advance value to be set
     */
    public void setAdvance( int i, int advance ) {
        advances[i] = advance;
    }

    /**
     * Returns size of the text
     *
     * @return size of the text
     */
    public int getSize() {
        return size;
    }

    /**
     * Sets new size of the text
     *
     * @param size   new size of the text
     */
    public void setSize( int size ) {
        this.size = size;
    }

    /**
     * Returns width of the text in twixels
     *
     * @return width of the text in twixels
     */
    public int getWidth() {
        int width = 0;
        for( int i=0; i<size; i++ ) {
            width += advances[i];
        }
        return width;
    }

    /**
     * Updates indexes of this record from specified font
     *
     * @param font   font to update from
     */
    public void updateIndexes( Font font ) {
        for( int i=0; i<size; i++ ) {
            char ch = text[i];
            //System.out.print( "text["+i+"]='"+ch+"'" );
            int idx = font.getIndex(ch);
            //System.out.println( ", new index="+idx );
            if( idx == -1 ) idx = 0;    // just for safety
            indexes[i] = idx;
        }
    }

    /**
     * Adds new index and advance value.
     * <P>
     * Does not check for overflow.
     *
     * @param index   new index to be added
     * @param advance new advance value to be added
     */
    public void add( int index, int advance ) {
        indexes[size] = index;
        advances[size] = advance;
        size++;
    }

    /**
     * Adds new character, index and advance value.
     * <P>
     * Does not check for overflow.
     *
     * @param ch      new character to be added
     * @param index   new index to be added
     * @param advance new advance value to be added
     */
    public void add( char ch, int index, int advance ) {
        text[size] = ch;
        add( index, advance );
    }

    /**
     * Trims this record from the end
     * <P>
     * Removes all spaces from the end
     *
     * @return width in twixels of all removed spaces
     */
    public int trimEnd() {
        int i;
        int w = 0;
        for( i=size; --i>=0; ) {
            if( !Character.isWhitespace(text[i]) ) break;
            w += advances[i];
        }
        size = i+1;
        return w;
    }

    /**
     * Trims this record from the start
     * <P>
     * Removes all spaces from the start
     *
     * @return width in twixels of all removed spaces
     */
    public int trimStart() {
        int i;
        int w = 0;
        for( i=0; i<size; i++ ) {
            if( !Character.isWhitespace(text[i]) ) break;
            w += advances[i];
        }
        if( i != 0 ) {
            size -= i;
            System.arraycopy( text,     i, text,     0, size );
            System.arraycopy( advances, i, advances, 0, size );
            System.arraycopy( indexes,  i, indexes,  0, size );
        }
        return w;
    }

    /**
     * Returns maximum index
     *
     * @return maximum index
     */
    public int getMaxIndex() {
        return Util.getMax(indexes, size);
    }

    /**
     * Returns maximum advance value
     *
     * @return maximum advance value
     */
    public int getMaxAdvance() {
        return Util.getMax(advances, size);
    }

    /**
     * Writes this text record to flash buffer.
     * <P>
     * Expects two-element integer array in userdata of flashbuffer.
     * First element is number of glyph bits and second - number of
     * advance value bits.
     *
     * @param fob    flashbuffer to write to
     */
    public void write( FlashOutput fob ) {
        int[] nbits = (int[]) fob.getUserData();
        int nGlyphBits   = nbits[0];
        int nAdvanceBits = nbits[1];
        fob.writeByte(size);
        for( int k=0; k<size; k++ ) {
            fob.writeBits(indexes[k], nGlyphBits);
            fob.writeBits(advances[k], nAdvanceBits);
        }
        fob.flushBits();
    }

    public void printContent( PrintStream out, String indent ) {
        out.println( indent+"TextRecord: nGlyphs="+size );
        for( int k=0; k<size; k++ ) {
            out.println( indent+"    char["+k+"]='"+text[k]+"', advance["+k+"]="+advances[k] );
        }
    }

    protected FlashItem copyInto( FlashItem item, ScriptCopier copier ) {
        char[] txt = new char[text.length];
        System.arraycopy(text, 0, txt, 0, text.length);
        int[] ind = new int[indexes.length];
        System.arraycopy(indexes, 0, ind, 0, indexes.length);
        int[] adv = new int[advances.length];
        System.arraycopy(advances, 0, adv, 0, advances.length);
        ((TextRecord)item).size = size;
        ((TextRecord)item).text = txt;
        ((TextRecord)item).indexes = ind;
        ((TextRecord)item).advances = adv;
        return item;
    }

    public FlashItem getCopy( ScriptCopier copier ) {
        return copyInto( new TextRecord(), copier );
    }
}

