/*
 * $Id: Parser.java,v 1.4 2002/03/17 03:51:34 skavish Exp $
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

package org.openlaszlo.iv.flash.parser;

import java.io.*;
import java.util.*;
import java.util.zip.*;

import org.openlaszlo.iv.flash.api.*;
import org.openlaszlo.iv.flash.util.*;


public final class Parser extends FlashBuffer {

    private int tagStartPos;
    private int tagDataPos;
    private int tagLength;
    private int tagEndPos;
    private int tagCode;

    private byte[] temp_bufb;
    private char[] temp_bufc;

    // file
    private FlashFile file;

    public Parser() {
    }

    public int getTag() {
        tagStartPos = getPos();
        int code = getUWord();
        int length = code & 0x3f;
        code = code >> 6;

        if( length == 0x3f ) {
            length = getUDWord();
        }

        tagDataPos = getPos();
        tagLength  = length;
        tagCode    = code;
        tagEndPos  = tagDataPos+length;
        return code;
    }

    public void addDef( FlashDef def ) {
        file.addDef(def);
    }

    public FlashDef getDef( int id ) {
        return file.getDef(id);
    }

    public void addDefToLibrary( String name, FlashDef def ) {
        file.addDefToLibrary( name, def );
    }

    public FlashDef getDefFromLibrary( String name ) {
        return file.getDefFromLibrary( name );
    }

    public int getTagStartPos() {
        return tagStartPos;
    }
    public int getTagDataPos() {
        return tagDataPos;
    }
    public int getTagLength() {
        return tagLength;
    }
    public int getTagEndPos() {
        return tagEndPos;
    }
    public int getTagCode() {
        return tagCode;
    }

    public byte[] getTempByteBuf( int size ) {
        if( temp_bufb == null || temp_bufb.length < size ) {
            temp_bufb = new byte[size];
        }
        return temp_bufb;
    }

    public char[] getTempCharBuf( int size ) {
        if( temp_bufc == null || temp_bufc.length < size ) {
            temp_bufc = new char[size];
        }
        return temp_bufc;
    }

    public FlashObject newUnknownTag() {
        return new UnparsedTag( tagCode, getBuf(), tagStartPos, tagEndPos );
    }

    public void skipLastTag() {
         setPos( tagEndPos );
    }

    public FlashFile getFile() {
        return file;
    }

    /**
     * Parse input stream
     */
    public void parseStream( InputStream is, FlashFile file ) throws IVException, IOException {

        byte[] fileHdr = new byte[8];

        if( is.read(fileHdr, 0, 8) != 8 ) {
            throw new IVException( Resource.CANTREADHEADER, new Object[] {file.getFullName()} );
        }

        if( fileHdr[1] != 'W' || fileHdr[2] != 'S' ) {
            throw new IVException( Resource.ILLEGALHEADER, new Object[] {file.getFullName()} );
        }

        boolean isCompressed = false;
        if( fileHdr[0] == 'C' ) {
            isCompressed = true;
        } else if( fileHdr[0] != 'F' ) {
            throw new IVException( Resource.ILLEGALHEADER, new Object[] {file.getFullName()} );
        }

        // get the file size.
        int fileSize = Util.getUDWord(fileHdr[4], fileHdr[5], fileHdr[6], fileHdr[7]);

        if( fileSize < 21 ) {
            throw new IVException( Resource.FILETOOSHORT, new Object[] {file.getFullName()} );
        }

        FlashBuffer fb;
        try {
            fb = new FlashBuffer(fileSize+8);
        } catch( OutOfMemoryError e ) {
            throw new IVException( Resource.FILETOOBIG, new Object[] {file.getFullName()} );
        }

        fb.writeArray(fileHdr, 0, 8);

        if( isCompressed ) {
            is = new InflaterInputStream(is);
        }

        fb.write(is);

        file.setCompressed(isCompressed);
        _parseBuffer(fb, file);
    }

    /**
     * Parse FlashBuffer
     */
    public void parseBuffer( FlashBuffer fob, FlashFile file ) throws IVException {
        boolean isCompressed = false;

        if( fob.getBuf()[0] == 'C' ) {
            isCompressed = true;
        } else if( fob.getBuf()[0] != 'F' ) {
            throw new IVException( Resource.ILLEGALHEADER, new Object[] {file.getFullName()} );
        }

        if( isCompressed ) {  // decompress buffer
            int fileSize = fob.getDWordAt(4);

            if( fileSize < 21 ) {
                throw new IVException( Resource.FILETOOSHORT, new Object[] {file.getFullName()} );
            }

            FlashBuffer fb;
            try {
                fb = new FlashBuffer(fileSize+8);
            } catch( OutOfMemoryError e ) {
                throw new IVException( Resource.FILETOOBIG, new Object[] {file.getFullName()} );
            }

            fb.writeArray(fob.getBuf(), 0, 8);

            try {
                fb.write( new InflaterInputStream(fob.getInputStream(8)) );
            } catch( IOException e ) {
                throw new IVException(e);
            }
            fob = fb;
        }

        file.setCompressed(isCompressed);
        _parseBuffer(fob, file);
    }

    /**
     */
    private void _parseBuffer( FlashBuffer fb, FlashFile file ) throws IVException {
        this.file = file;

        init( fb.getBuf(), 0, fb.getSize() );

        // get version
        file.setVersion( (int) getByteAt(3) );

        setPos(8);

        // get frame size
        file.setFrameSize( getRect() );

        // get frame rate
        file.setFrameRate( getUWord() );

        // parse main script
        file.setMainScript( Script.parse( this, true ) );
    }

}
