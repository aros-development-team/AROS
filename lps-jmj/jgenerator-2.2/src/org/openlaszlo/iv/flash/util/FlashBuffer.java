/*
 * $Id: FlashBuffer.java,v 1.4 2002/07/15 02:15:03 skavish Exp $
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

import java.awt.geom.AffineTransform;
import java.awt.geom.Rectangle2D;
import java.io.*;
import org.openlaszlo.iv.flash.api.*;

/**
 * Wrapper of array of bytes.
 * <p>
 * Provides reading and writing of flash related data types.
 *
 * @author Dmitry Skavish
 * @see FlashOutput
 */
public class FlashBuffer {

    // bit buffer and position
    private int bitBuf;
    private int bitPos;

    // byte array
    private byte buf[];

    // current position in the buffer for reading and writing
    public int pos;

    // size of the buffer
    private int size;

    public FlashBuffer() {
    }

    /**
     * Allocates buffer of given capacity.<p>
     * Sets current position and size to zero.
     *
     * @param capacity capacity of allocated buffer in bytes
     */
    public FlashBuffer( int capacity ) {
        this( new byte[capacity], 0, 0 );
    }

    /**
     * Creates buffer from given one.<p>
     * Sets current position to zero and size to the size of the buffer.
     *
     * @param buf    buffer to init from
     */
    public FlashBuffer( byte[] buf ) {
        this( buf, 0, buf.length );
    }

    /**
     * Creates buffer from given one of specified size.<p>
     * Sets current position to zero and size to given value.
     *
     * @param buf    buffer to init from
     * @param size   size of buffer
     */
    public FlashBuffer( byte[] buf, int size ) {
        this( buf, 0, size );
    }

    /**
     * Creates buffer from given one of specified size and position.
     *
     * @param buf    buffer to init from
     * @param pos    current position
     * @param size   size of filled buffer (writer pos)
     */
    public FlashBuffer( byte[] buf, int pos, int size ) {
        init( buf, pos, size );
    }

    /**
     * Creates FlashBuffer from input stream.<p>
     * Reads InputStream into the buffer, sets current position
     * to zero and size to the size of data read.
     *
     * @author Andrew Wason
     * @author Dmitry Skavish
     * @param is     InputStream to read from
     * @exception IOException
     */
    public FlashBuffer( InputStream is ) throws IOException {
        size = 0;
        try {
            int count = is.available();
            if( count <= 0 ) count = 4096;
            buf = new byte[count+8];    // make it a little bit bigger to avoid reallocation
            for(;;) {
                count = is.read(buf, size, buf.length - size);
                if( count == -1 ) break;
                size += count;
                if( size == buf.length )
                    ensureCapacity(buf.length + 4096 * 4);
            }
        } finally {
            is.close();
        }
        pos = 0;
        bitBuf = 0;
        bitPos = 0;
    }

    public void init( byte[] buf, int pos, int size ) {
        this.buf = buf;
        this.pos = pos;
        this.size = size;
        this.bitBuf = 0;
        this.bitPos = 0;
    }

    /**
     * Ensures that the buffer is as big as specified number of bytes
     *
     * @param cap required size of buffer
     */
    public final void ensureCapacity( int cap ) {
        if( cap > buf.length ) {
            int max = buf.length*2;
            if( cap > max ) max = cap+16;
            if( max < 4096 ) max = 4096;
            byte[] newBuf = new byte[max];
            System.arraycopy(buf, 0, newBuf, 0, buf.length);
            buf = newBuf;
        }
    }

    /**
     * Creates copy of the buffer.
     *
     * @return copy of the buffer
     */
    public FlashBuffer getCopy() {
        byte[] myBuf = new byte[buf.length];
        System.arraycopy(buf, 0, myBuf, 0, buf.length);
        return new FlashBuffer( myBuf, pos, size );
    }

    /**
     * Current read/write position.
     *
     * @return current read/write position
     */
    public final int getPos() {
        return pos;
    }

    /**
     * Returns size of the buffer.
     *
     * @return size of the buffer
     */
    public final int getSize() {
        return size;
    }

    /**
     * Sets current read/write position.<P>
     * Does not increase the buffer if new position is larger
     * than current capacity.
     *
     * @param pos    new position
     * @see #ensureCapacity
     */
    public final void setPos( int pos ) {
        this.pos = pos;
        if( pos > size ) size = pos;
    }

    /**
     * Increment current position.<p>
     * Does not increase the buffer if new position is larger
     * than current capacity.
     *
     * @see #ensureCapacity
     * @see #setPos
     */
    public final void incPos() {
        if( ++pos > size ) size = pos;
    }

    /**
     * Sets new size of the buffer.<p>
     * Does not increase the buffer if new position is larger
     * than current capacity.
     *
     * @param size   new size of the buffer
     */
    public final void setSize( int size ) {
        this.size = size;
    }

    /**
     * Skips bytes (changes current position).<P>
     * Does not increase the buffer if new position is larger
     * than current capacity.
     *
     * @param inc    advance value
     */
    public final void skip( int inc ) {
        setPos( pos + inc );
    }

    /**
     * Returns the whole buffer.
     *
     * @return buffer
     */
    public final byte[] getBuf() {
        return buf;
    }

    /*-----------------------------------------------------------------------
     *                       W R I T E R
     *-----------------------------------------------------------------------*/

    /**
     * Writes byte at specified position.<p>
     * Does not change current position.
     *
     * @param b      byte to write
     * @param pos    position to write
     */
    public final void writeByteAt( int b, int pos ) {
        buf[pos] = (byte) b;
    }

    /**
     * Writes word at specified position.<p>
     * Does not change current position.
     *
     * @param b      word to write
     * @param pos    position to write
     */
    public final void writeWordAt( int b, int pos ) {
        buf[pos] = (byte) b;
        buf[pos+1] = (byte) (b>>8);
    }

    /**
     * Writes dword at specified position.<p>
     * Does not change current position.
     *
     * @param b      dword to write
     * @param pos    position to write
     */
    public final void writeDWordAt( int b, int pos ) {
        buf[pos] = (byte) b;
        buf[pos+1] = (byte) (b>>8);
        buf[pos+2] = (byte) (b>>16);
        buf[pos+3] = (byte) (b>>24);
    }

    /**
     * Writes byte and advance current position.
     *
     * @param b      byte to write
     */
    public final void writeByte( int b ) {
        ensureCapacity( pos+1 );
        buf[pos] = (byte) b;
        incPos();
    }

    /**
     * Writes word and advance current position.
     *
     * @param b      word to write
     */
    public final void writeWord( int b ) {
        ensureCapacity( pos+2 );
        buf[pos] = (byte) b;
        buf[pos+1] = (byte) (b>>8);
        setPos( pos+2 );
    }

    /**
     * Writes dword and advance current position.
     *
     * @param b      dword to write
     */
    public final void writeDWord( int b ) {
        ensureCapacity( pos+4 );
        buf[pos] = (byte) b;
        buf[pos+1] = (byte) (b>>8);
        buf[pos+2] = (byte) (b>>16);
        buf[pos+3] = (byte) (b>>24);
        setPos( pos+4 );
    }


    /****************************************************************
     * Fast operations do not call ensureCapacity
     */

    /**
     * Writes byte and advance current position.
     *
     * @param b      byte to write
     */
    public final void _writeByte( int b ) {
        buf[pos++] = (byte) b;
    }

    /**
     * Writes word and advance current position.
     *
     * @param b      word to write
     */
    public final void _writeWord( int b ) {
        buf[pos++] = (byte) b;
        buf[pos++] = (byte) (b>>8);
    }

    /**
     * Writes dword and advance current position.
     *
     * @param b      dword to write
     */
    public final void _writeDWord( int b ) {
        buf[pos++] = (byte) b;
        buf[pos++] = (byte) (b>>8);
        buf[pos++] = (byte) (b>>16);
        buf[pos++] = (byte) (b>>24);
    }


    /**
     * Writes zero-ending string into the buffer and advance current position.
     * (Flash5 back compatibility, uses Cp1252 encoding)
     * @param s      string to write
     */
    public final byte[]  _writeStringZ( String s ) {
        return _writeStringZ(s, "Cp1252");
    }


    /**
     * Writes zero-ending string into the buffer and advance current position.
     *
     * @param s      string to write
     * @return the string converted to a byte array in Flash's favorite encoding
     */
    public final byte[]  _writeStringZ( String s, String encoding ) {
        byte chars[];
        try {
            chars = s.getBytes(encoding);
        } catch (UnsupportedEncodingException e) {
            throw new RuntimeException("could not convert string to "+encoding);
        }
        System.arraycopy(chars, 0, buf, pos, chars.length);
        setPos( pos+chars.length );
        buf[pos++] = 0; // null-terminate the string
        return chars;
    }



    /**
     * Writes array into this one and advance current position.
     *
     * @param b      intput buffer
     * @param off    offset in the input buffer
     * @param len    number of bytes in input buffer
     */
    public final void _writeArray( byte b[], int off, int len ) {
        System.arraycopy(b, off, buf, pos, len);
        setPos( pos+len );
    }


    /****************************************************************/



    /**
     * Writes FlashBuffer into this one and advance current position.
     *
     * @param fob    buffer to write
     */
    public final void writeFOB( FlashBuffer fob ) {
        int len = fob.getSize();
        ensureCapacity( pos+len );
        System.arraycopy(fob.getBuf(), 0, buf, pos, len);
        setPos( pos+len );
    }

    /**
     * Writes array into this one and advance current position.
     *
     * @param b      intput buffer
     * @param off    offset in the input buffer
     * @param len    number of bytes in input buffer
     */
    public final void writeArray( byte b[], int off, int len ) {
        ensureCapacity( pos+len );
        System.arraycopy(b, off, buf, pos, len);
        setPos( pos+len );
    }

    /**
     * Writes zero-ending string into the buffer and advance current position.
     *
     * +++ Back compatible with Jgen-1.4/Flash5
     * Uses default encoding of JGen 1.4, "Cp1252".
     * @param s      string to write
     *
     */

    public final  byte[] writeStringZ( String s ) {
        return writeStringZ(s, "Cp1252");
    }


    /**
     * Writes zero-ending string into the buffer and advance current position.
     *
     * @param s      string to write
     * @param encoding      charset encoding to use
     *
     */

    public final  byte[] writeStringZ( String s, String encoding ) {
        byte chars[];
        try {
            // encode utf-8 for flash 6 
            chars = s.getBytes(encoding);
        } catch (UnsupportedEncodingException e) {
            throw new RuntimeException("could not convert string to "+encoding);
        }
        ensureCapacity( pos+chars.length+1 );
        for( int i=0; i<chars.length; i++ ) {
            buf[pos++] = chars[i];
        }
        buf[pos] = 0;
        incPos();
        return chars;
    }

    /**
     * Writes length-prefixed string into the buffer and advance current position.
     *
     * @param s      string to write
     */
    public final void writeStringL( String s ) {
        writeStringL(s, "Cp1252");
    }

    /**
     * Writes length-prefixed string into the buffer and advance current position.
     *
     * @param s      string to write
     * @param encoding      charset encoding to use
     */
    public final void writeStringL( String s, String encoding ) {

        byte chars[];
        try {
            chars = s.getBytes(encoding);
        } catch (UnsupportedEncodingException e) {
            throw new RuntimeException("could not convert string to "+encoding);
        }
        ensureCapacity( pos+chars.length+1 );
        buf[pos++] = (byte) chars.length;
        for( int i=0; i<chars.length; i++ ) {
            buf[pos++] = chars[i];
        }
        setPos( pos );  // to update size
    }

    /**
     * Writes flash tag into the buffer and advance current position.<P>
     * Depending on tag length writes short or long tag
     *
     * @param tagCode tag code
     * @param tagSize tag size
     * @see Tag
     * @see #writeLongTag
     * @see #writeLongTagAt
     * @see #writeShortTagAt
     */
    public final void writeTag( int tagCode, int tagSize ) {
        if( tagSize >= 0x3f ) {
            writeLongTag( tagCode, tagSize );
        } else {
            writeWord( (tagCode<<6) | tagSize );
        }
    }

    /**
     * Writes long flash tag into the buffer and advance current position.<P>
     *
     * @param tagCode tag code
     * @param tagSize tag size
     * @see Tag
     * @see #writeTag
     * @see #writeLongTagAt
     * @see #writeShortTagAt
     */
    public final void writeLongTag( int tagCode, int tagSize ) {
        writeWord( (tagCode<<6) | 0x3f );
        writeDWord( tagSize );
    }

    /**
     * Writes short flash tag into the buffer at specified position.<P>
     * Does not advance position
     *
     * @param tagCode tag code
     * @param tagSize tag size
     * @see Tag
     * @see #writeLongTag
     * @see #writeLongTagAt
     * @see #writeTag
     */
    public final void writeShortTagAt( int tagCode, int tagSize, int pos ) {
        writeWordAt( (tagCode<<6) | tagSize, pos );
    }

    /**
     * Writes long flash tag into the buffer at specified position.<P>
     * Does not advance position
     *
     * @param tagCode tag code
     * @param tagSize tag size
     * @see Tag
     * @see #writeLongTag
     * @see #writeShortTagAt
     * @see #writeTag
     */
    public final void writeLongTagAt( int tagCode, int tagSize, int pos ) {
        writeWordAt( (tagCode<<6) | 0x3f, pos );
        writeDWordAt( tagSize, pos+2 );
    }

    /**
     * Writes lower bit to bit buffer.
     *
     * @param b      bit to write
     * @see #initBits
     * @see #flushBits
     * @see #writeBits
     */
    public final void writeBit( int b ) {
        writeBits( b, 1 );
    }

    /**
     * Writes boolean as a bit to bit buffer.
     *
     * @param b      boolean to write
     * @see #initBits
     * @see #flushBits
     */
    public final void writeBool( boolean b ) {
        writeBits( b?1:0, 1 );
    }

    /**
     * Writes bits into the buffer.<P>
     * Before starting writing bits you have to init or flush bits
     * buffer using methods {@link #initBits} or {@link #flushBits}
     *
     * @param v      bits to write packed in integer
     * @param len    number of bits to write
     * @see #initBits
     * @see #flushBits
     */
    public final void writeBits( int v, int len ) {
        ensureCapacity( pos+4 );
        for(;;) {
            v = v & ((1<<len)-1);
            int l = 8-bitPos;
            int s = l-len;
            if( s >= 0 ) {
                bitBuf = (bitBuf<<len) | v;
                bitPos += len;
                return;
            } else {
                s = -s;
                int bb = (bitBuf<<l) | (v>>>s);
                buf[pos] = (byte)bb;
                incPos();
                len = s;
                bitBuf = 0;
                bitPos = 0;
            }
        }
    }

    /**
     * Flushes bits buffer into flash buffer.<P>
     * Has to be called after you finished writing series of bits
     *
     * @see #writeBits
     * @see #initBits
     */
    public final void flushBits() {
        if( bitPos != 0 ) {
            int bb = bitBuf << (8-bitPos);
            writeByte( bb );
        }
        bitBuf = 0;
        bitPos = 0;
    }

    /**
     * Inits bits buffer.<P>
     * Has to be called before writing series of bits or
     * before reading bits
     *
     * @see #writeBits
     * @see #flushBits
     * @see #skipBits
     * @see #getBits
     */
    public final void initBits() {
        bitBuf = 0;
        bitPos = 0;
    }

    /**
     * Writes specified inputstream to this buffer
     *
     * @param is     input stream
     */
    public final void write( InputStream is ) throws IOException {
        try {
            int count = is.available();
            if( count <= 0 ) count = 4096;
            ensureCapacity(pos+count+8);
            for(;;) {
                count = is.read(buf, pos, buf.length - pos);
                if( count == -1 ) break;
                pos += count;
                if( pos == buf.length )
                    ensureCapacity(buf.length + 4096 * 4);
            }
            setPos(pos);
        } finally {
            is.close();
        }
    }

    /*-----------------------------------------------------------------------
     *                       R E A D E R
     *-----------------------------------------------------------------------*/

    /**
     * Skips bits.
     *
     * @param n      number of bits to skip
     * @see #initBits
     */
    public final void skipBits( int n ) {
        for (;;) {
            int s = n - bitPos;
            if( s > 0 ) {
                n -= bitPos;
                // get the next buffer
                bitBuf = getUByte();
                bitPos = 8;
            } else {
                // Consume a portion of the buffer
                s = -s;
                bitPos = s;
                bitBuf &= (1<<s)-1;   // mask off the consumed bits
                break;
            }
        }
    }

    /**
     * Reads <b>unsigned</b> bits from the buffer.<P>
     *
     * According to profiler this is probably the most
     * time consuming operation. Below there is a new version,
     * but I did not test it much, it's about 30% percent faster.
     *
     * @param n      number of bits to read
     * @return read bits
     * @see #initBits
     * @see #getSBits
     */
    public final int getBits( int n ) {
        // get n bits from the stream.
        int v = 0;

        for (;;) {
            int s = n - bitPos;
            if( s > 0 ) {
                // Consume the entire buffer
                v |= bitBuf << s;
                n -= bitPos;

                // get the next buffer
                bitBuf = getUByte();
                bitPos = 8;
            } else {
                // Consume a portion of the buffer
                s = -s;
                v |= bitBuf >> s;
                bitPos = s;
                bitBuf &= (1<<s)-1;   // mask off the consumed bits
                return v;
            }
        }
    }

    // new version of getBits
    public final int new_getBits( int n ) {
        // get n bits from the stream.

        int s = n-bitPos;
        if( s > 0 ) {
            // Consume the entire buffer
            int v = bitBuf << s;
            n -= bitPos;

            // get the next buffer
            if( n <= 8 ) {
                bitBuf = getUByte();
                bitPos = 8;
            } else if( n <= 16 ) {
                bitBuf = (getUByte()<<8) | getUByte();
                bitPos = 16;
            } else if( n <= 24 ) {
                bitBuf = (getUByte()<<16) | (getUByte()<<8) | getUByte();
                bitPos = 24;
            } else {
                bitBuf = (getUByte()<<24) | (getUByte()<<16) | (getUByte()<<8) | getUByte();
                bitPos = 32;
            }
            bitPos -= n;
            v |= bitBuf >> bitPos;
            bitBuf &= (1<<bitPos)-1;
            return v;
        }

        // Consume a portion of the buffer
        s = -s;
        int v = bitBuf >> s;
        bitPos = s;
        bitBuf &= (1<<s)-1;   // mask off the consumed bits
        return v;
    }

    /**
     * Reads <b>signed</b> bits from the buffer.<P>
     *
     * @param n      number of bits to read
     * @return read bits extended with sign
     * @see #initBits
     * @see #getBits
     */
    public final int getSBits( int n ) {
        // get n bits from the string with sign extension.
        // get the number as an unsigned value.
        int v = getBits(n);

        // Is the number negative?
        if( (v & (1<<(n-1))) != 0 ) {
            // Yes. Extend the sign.
            v |= -1 << n;
        }

        return v;
    }

    /**
     * Reads one bit and returns it as boolean
     *
     * @return true - if bit is 1, false - if bit is 0
     */
    public boolean getBool() {
        return getBits(1) == 1;
    }

    /**
     * Reads bytes into given FlashBuffer.
     *
     * @param fob    flash buffer where to read bytes
     * @param length number of bytes to read
     */
    public final void getTo( FlashBuffer fob, int length ) {
        fob.writeArray(buf, pos, length);
        pos += length;
    }

    /**
     * Reads zero-ending string.
     *
     * @return read string
     */
    public final String getString() {
        int sp = pos;
        while( buf[pos++] != 0 );
        return new String( buf, sp, pos-sp-1 );
    }

    /**
     * Read string by its length.
     *
     * @param length string length
     * @return read string
     */
    public final String getString( int length ) {
        int sp = pos;
        pos += length;
        return new String(buf, sp, length);
    }

    /**
     * Reads bytes into array of bytes.
     *
     * @param length number of bytes to read
     * @return created array of bytes with data
     */
    public final byte[] getBytes( int length ) {
        byte[] ba = new byte[length];
        System.arraycopy(buf, pos, ba, 0, length);
        pos += length;
        return ba;
    }

    /**
     * Reads one signed byte.
     *
     * @return signed byte
     */
    public final int getByte() {
        return buf[pos++];
    }

    /**
     * Reads one unsigned byte.
     *
     * @return unsigned byte
     */
    public final int getUByte() {
        return buf[pos++] & 0xff;
    }

    public final int getByteAt( int p ) {
        return buf[p];
    }

    public final int getUByteAt( int p ) {
        return buf[p] & 0xff;
    }

    /**
     * Reads one signed word.
     *
     * @return signed word
     */
    public final int getWord() {
        int r = Util.getWord(buf[pos], buf[pos+1]);
        pos += 2;
        return r;
    }

    public final int getWordAt( int p ) {
        return Util.getWord(buf[p], buf[p+1]);
    }

    /**
     * Reads one unsigned word.
     *
     * @return unsigned word
     */
    public final int getUWord() {
        int r = Util.getUWord(buf[pos], buf[pos+1]);
        pos += 2;
        return r;
    }

    public final int getUWordAt( int p ) {
        return Util.getUWord(buf[p], buf[p+1]);
    }

    /**
     * Reads one signed dword.
     *
     * @return signed dword
     */
    public int getDWord() {
        int r = Util.getDWord(buf[pos], buf[pos+1], buf[pos+2], buf[pos+3]);
        pos += 4;
        return r;
    }

    public int getDWordAt( int p ) {
        return Util.getDWord(buf[p], buf[p+1], buf[p+2], buf[p+3]);
    }

    /**
     * Reads one unsigned dword.
     *
     * @return unsigned dword
     */
    public int getUDWord() {
        int r = Util.getUDWord(buf[pos], buf[pos+1], buf[pos+2], buf[pos+3]);
        pos += 4;
        return r;
    }

    public int getUDWordAt( int p ) {
        return Util.getUDWord(buf[p], buf[p+1], buf[p+2], buf[p+3]);
    }

    /*-----------------------------------------------------------------------
     *                       InputStream
     *-----------------------------------------------------------------------*/

    /**
     * Creates input stream which can be used to read data from this buffer.
     *
     * @return input stream
     * @see #getOutputStream
     */
    public InputStream getInputStream() {
        return new FlashBufferInputStream();
    }

    /**
     * Creates input stream which can be used to read data from this buffer.
     *
     * @param pos    first input position, position to start reading from
     * @return input stream
     * @see #getOutputStream
     */
    public InputStream getInputStream( int pos ) {
        return new FlashBufferInputStream(pos);
    }

    public class FlashBufferInputStream extends InputStream {

        private int curPos = 0;

        public FlashBufferInputStream() {
        }

        public FlashBufferInputStream( int curPos ) {
            this.curPos = curPos;
        }

        public int read() throws IOException {
            if( curPos >= size ) return -1;
            return buf[curPos++] & 0xff;
        }

        public int read( byte b[], int off, int len ) throws IOException {
            if( len == 0 ) return 0;
            int sz = Math.min(len, size-curPos);
            if( sz <= 0 ) return -1;
            System.arraycopy(buf, curPos, b, off, sz);
            curPos += sz;
            return sz;
        }

        public int available() throws IOException {
            return size-curPos;
        }
    }

    /*-----------------------------------------------------------------------
     *                       OutputStream
     *-----------------------------------------------------------------------*/

    /**
     * Creates output stream which can be used to write data to this buffer.
     *
     * @return output stream
     * @see #getInputStream
     */
    public OutputStream getOutputStream() {
        return new FlashBufferOutputStream();
    }

    public class FlashBufferOutputStream extends OutputStream {

        public FlashBufferOutputStream() {
        }

        public void write(int b) {
            writeByte(b);
        }

        public void write(byte b[], int off, int len) {
            writeArray(b,off,len);
        }
    }

    /*-----------------------------------------------------------------------
     *                       AffineTransform
     *-----------------------------------------------------------------------*/

    public AffineTransform getMatrix() {
        initBits();

        double m00;   // scale x
        double m10;   // skew0 (y shear)
        double m01;   // skew1 (x shear)
        double m11;   // scale y
        double m02;   // translate x
        double m12;   // trasnlate y

        // Scale terms
        if( getBool() ) {
            int nBits = getBits(5);
            m00 = Util.fixed2double(getSBits(nBits));
            m11 = Util.fixed2double(getSBits(nBits));
        } else {
            m00 = 1.0;
            m11 = 1.0;
        }

        // Rotate/skew terms
        if( getBool() ) {
            int nBits = getBits(5);
            m10 = Util.fixed2double(getSBits(nBits));
            m01 = Util.fixed2double(getSBits(nBits));
        } else {
            m10 = 0.0;
            m01 = 0.0;
        }

        // Translate terms
        int nBits = getBits(5);
        m02 = getSBits(nBits);
        m12 = getSBits(nBits);

        AffineTransform m = new AffineTransform( m00, m10, m01, m11, m02, m12 );
        return m;
    }

    /**
     * Skips MATRIX tag without creating Matrix object
     */
    public void skipMatrix() {
        initBits();
        // Scale terms
        if( getBool() ) {
            int nBits = getBits(5);
            skipBits(nBits+nBits);
        }
        // Rotate/skew terms
        if( getBool() ) {
            int nBits = getBits(5);
            skipBits(nBits+nBits);
        }
        // Translate terms
        int nBits = getBits(5);
        skipBits(nBits+nBits);
    }

    public void write( AffineTransform m ) {
        initBits();

        double m00 = m.getScaleX();
        double m10 = m.getShearY();
        double m01 = m.getShearX();
        double m11 = m.getScaleY();
        double m02 = m.getTranslateX();
        double m12 = m.getTranslateY();

        if( m00 != 1.0 || m11 != 1.0 ) {
            writeBit(1);
            int i_scaleX = Util.double2fixed(m00);
            int i_scaleY = Util.double2fixed(m11);
            int nBits = Util.getMinBitsS( Util.getMax(i_scaleX,i_scaleY) );
            writeBits(nBits, 5);
            writeBits(i_scaleX, nBits);
            writeBits(i_scaleY, nBits);
        } else {
            writeBit(0);
        }

        if( m10 != 0.0 || m01 != 0.0 ) {
            writeBit(1);
            int i_rotateSkew0 = Util.double2fixed(m10);
            int i_rotateSkew1 = Util.double2fixed(m01);
            int nBits = Util.getMinBitsS( Util.getMax(i_rotateSkew0,i_rotateSkew1) );
            writeBits(nBits, 5);
            writeBits(i_rotateSkew0, nBits);
            writeBits(i_rotateSkew1, nBits);
        } else {
            writeBit(0);
        }

        int i_translateX = (int) m02;
        int i_translateY = (int) m12;
        int nBits = Util.getMinBitsS( Util.getMax(i_translateX,i_translateY) );
        writeBits(nBits, 5);
        writeBits(i_translateX, nBits);
        writeBits(i_translateY, nBits);
        flushBits();
    }

    /*-----------------------------------------------------------------------
     *                       Rectangle2D
     *-----------------------------------------------------------------------*/

    public Rectangle2D getRect() {
        initBits();
        int nBits = getBits(5);
        int xmin = getSBits(nBits);
        int xmax = getSBits(nBits);
        int ymin = getSBits(nBits);
        int ymax = getSBits(nBits);

        Rectangle2D r = GeomHelper.newRectangle( xmin, ymin, xmax-xmin, ymax-ymin );
        return r;
    }

    /**
     * Skips rectangle
     */
    public void skipRect() {
        initBits();
        int nBits = getBits(5);
        skip( ((5+(nBits*4))+7)/8 - 1  );
    }

    public void write( Rectangle2D r ) {
        initBits();

        int xmin = (int) r.getMinX();
        int xmax = (int) r.getMaxX();
        int ymin = (int) r.getMinY();
        int ymax = (int) r.getMaxY();

        int nBits = Util.getMinBitsS( Util.getMax(xmin,xmax,ymin,ymax) );
        writeBits( nBits, 5 );
        writeBits( xmin, nBits );
        writeBits( xmax, nBits );
        writeBits( ymin, nBits );
        writeBits( ymax, nBits );
        flushBits();
    }

    public String toString() {
        return new String(buf, 0, pos);
    }

    public String toString( String encoding ) throws java.io.UnsupportedEncodingException {
        return new String(buf, 0, pos, encoding);
    }
}
