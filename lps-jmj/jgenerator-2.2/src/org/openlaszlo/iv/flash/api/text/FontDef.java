/*
 * $Id: FontDef.java,v 1.3 2002/02/24 02:10:19 skavish Exp $
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

import org.openlaszlo.iv.flash.parser.Parser;
import org.openlaszlo.iv.flash.util.*;
import org.openlaszlo.iv.flash.cache.*;

import org.openlaszlo.iv.flash.api.*;
import java.io.*;
import java.awt.geom.Rectangle2D;

/**
 * Instance of Font in particular flash file.
 * <p>
 * There may be several instances of one font in one file.
 *
 * @author Dmitry Skavish
 */
public final class FontDef extends FlashDef {

    private boolean isWriteLayout = false;          // if true - write font outlines as well
    private boolean isWriteAllChars = false;        // if true - write full set of characters, otherwise only used ones
    private Font font;                              // font
    private IVVector textBlocks = new IVVector();   // vector of TextBlock's using this font

    /**
     * Creates empty font definition
     */
    public FontDef() {
    }

    /**
     * Creates font definition of specified font and ID
     *
     * @param font   specified font
     * @param id     ID of font definition to be created
     */
    public FontDef( Font font, int id ) {
        this.font = font;
        setID( id );
    }

    public int getTag() {
        return Tag.DEFINEFONT2;
    }

    /**
     * Returns font name
     *
     * @return font name
     */
    public String getFontName() {
        return font.fontName;
    }

    /**
     * Returns font
     *
     * @return font
     */
    public Font getFont() {
        return font;
    }

    public void setFont( Font font ) {
        this.font = font;
    }

    /**
     * Adds specified text block to this fontdef
     *
     * @param tblock specified text block
     */
    public void addTextBlock( TextBlock tblock ) {
        if( tblock == null ) return;
        textBlocks.addElement(tblock);
    }

    /**
     * Adds vector of text blocks to this fontdef
     *
     * @param tblocks vector of text blocks
     */
    public void addTextBlocks( IVVector tblocks ) {
        if( tblocks == null ) return;
        for( int i=0; i<tblocks.size(); i++ ) {
            addTextBlock( (TextBlock) tblocks.elementAt(i) );
        }
    }

    /**
     * Returns all text blocks added to this fontdef
     *
     * @return all text blocks added to this fontdef
     */
    public IVVector getTextBlocks() {
        return textBlocks;
    }

    /**
     * Specifies whether to write layout information when generating this font or not
     *
     * @param v      true - write layout
     */
    public void setWriteLayout( boolean v ) {
        isWriteLayout = v;
    }

    /**
     * Returns true if layout information is going to generated
     *
     * @return true if layout information is going to generated
     */
    public boolean isWriteLayout() {
        return isWriteLayout;
    }

    /**
     * Specifies whether to write all font's character or only used (in text blocks) ones.
     *
     * @param v      true - write all characters
     */
    public void setWriteAllChars( boolean v ) {
        isWriteAllChars = v;
    }

    /**
     * Returns true if all font's character are going to be generated
     *
     * @return true if all font's character are going to be generated
     */
    public boolean isWriteAllChars() {
        return isWriteAllChars;
    }

    /**
     * Parses External font tag
     */
    public static FontDef parseExternalFontTag( Parser p ) {
        int id = p.getUWord();
        String fontFileName = p.getString();
        Font font = load( fontFileName, p.getFile() );
        if( font != null ) return new FontDef( font, id );
        return null;
    }

    /**
     * Loads external font to the specified file
     */
    public static Font load( String fontFileName, FlashFile file ) {
        FlashFile fontFile;
        try {
            fontFile = file.addExternalFile( fontFileName, true );
        } catch( IVException e ) {
            Log.log(e);
            // load default symbol file and pick first font from it
            fontFile = file.getDefaultSymbolFile();
        }
        if( fontFile != null ) {
            IVVector v = fontFile.getLocalFonts();
            if( v.size() == 0 ) {
                Log.logRB( Resource.INVLEXTERNALFONT, new Object[] {fontFileName} );
                return null;
            }
            return (Font) v.elementAt(0);
        }
        return null;
    }

    /**
     * Loads font from file (FFT)
     *
     * @param fontFileName font file name
     * @return loaded font
     */
    public static Font load( String fontFileName ) {
        FlashFile fontFile;

        // Find the font
        fontFileName = Util.translatePath( fontFileName );
        File f = new File( fontFileName );
        if( !f.exists() ) {
            fontFileName = Util.concatFileNames( PropertyManager.fontPath, fontFileName );
            f = new File( Util.getInstallDir(), fontFileName );
            if ( !f.exists() ) {
                Log.logRB( Resource.FILENOTFOUND, new Object[] {fontFileName} );
                return null;
            }
        }
        fontFileName = f.getAbsolutePath();

        // Load the font
        try {
            fontFile = FlashFile.parse( fontFileName );
            IVVector v = fontFile.getLocalFonts();
            if ( v.size() == 0 ) {
                Log.logRB( Resource.INVLEXTERNALFONT, new Object[] {fontFileName} );
                return null;
            }
            return (Font) v.elementAt(0);
        } catch( FileNotFoundException e ) {
            Log.logRB(Resource.FILENOTFOUND, new Object[] {fontFileName});
            return null;
        } catch( IVException e ) {
            Log.log(e);
            return null;
        }
    }

    /**
     * Parses DefineFont tag.
     * <P>
     * We expect that after DefineFont tag, DefineFontInfo tag will follow
     * which will define codetable for this font.<br>
     * This is not always true of course, but in majority of cases it is.
     *
     * @param p      parser
     * @return font def
     */
    public static FontDef parse( Parser p ) {
        // get id
        int id = p.getUWord();
        Font font = new Font();
        font.flags = Font.ANSI;
        font.fileBuffer = p.getBuf();

        // get offset table and shape table
        int tableOffset = p.getPos();
        int offset = p.getUWord();
        int nGlyph = offset/2;

        int[] glyphOffsets = new int[nGlyph+1]; // +1 because we need last offset
        glyphOffsets[0] = tableOffset+offset;
        for( int i=1; i<nGlyph; i++ ) glyphOffsets[i] = tableOffset+p.getUWord();

        p.skipLastTag();  // skip all shapes
        glyphOffsets[nGlyph] = p.getPos();

        font.glyphOffsets = glyphOffsets;

        // there is no need to cache font, fontinfo will take care of it
        return new FontDef( font, id );
    }

    /**
     * Parse  DefineFontInfo tag
     */
    public static void parseFontInfoTag( Parser p, boolean MX ) {
        // get id
        int fontId = p.getUWord();
        FontDef fontDef = (FontDef) p.getDef(fontId);
        String fontName = p.getString( p.getUByte() );
        int flags = (MX)? p.getUWord() : p.getUByte();
        String ext = ((flags&0x04)!=0?"%i":"") + ((flags&0x02)!=0?"%b":"");
        String fontKey = fontName+ext;

        // do not put this font into cache
        // later on we will try to merge it into some probably existing in cache font

        Font font = fontDef.font;
        font.fontName = fontName;
        font.fontKey  = fontKey;
        font.flags =  ((flags&0x04)!=0?Font.ITALIC:0) | ((flags&0x02)!=0?Font.BOLD:0) |
                      ((flags&0x20)!=0?Font.UNICODE:0) | ((flags&0x10)!=0?Font.SHIFT_JIS:0) |
                      ((flags&0x08)!=0?Font.ANSI:0) | ((flags&0x01)!=0?Font.WIDE_CODES:0);
        //Log.logRB( Resource.STR, "parseInfo: font="+fontName+", font2="+font2+", flags="+Util.w2h(font.flags) );
        // get code table
        int nGlyph = font.getNumGlyph();
        int[] codeTable = new int[nGlyph];
        if( (font.flags&Font.WIDE_CODES) != 0 ) {
            for( int i=0; i<nGlyph; i++ ) codeTable[i] = p.getUWord();
        } else {
            for( int i=0; i<nGlyph; i++ ) codeTable[i] = p.getUByte();
        }
        font.codeTable = codeTable;

        // find 'blank'
        for( int i=0; i<nGlyph; i++ ) {
            if( codeTable[i] == ' ' ) {
                font.blankPos = i;
                break;
            }
        }
    }

    /**
     * Parse  DefineFontInfo tag
     */
    public static void parseFontInfoTag( Parser p ) {
        parseFontInfoTag( p, false );
    }

    /**
     * Parse  DefineFontInfo2 tag
     */
    public static void parseFontInfoTag2( Parser p ) {
        parseFontInfoTag( p, true );
    }

    /**
     * Parse  DefineFont2 tag
     */
    public static FontDef parse2( Parser p ) {
        // get id
        int id = p.getUWord();
        // get flags
        int flags = p.getUWord();
        // get name of the font
        String fontName = p.getString( p.getUByte() );

        // check if this font is in cache
        String ext = ((flags&Font.ITALIC)!=0?"%i":"")+((flags&Font.BOLD)!=0?"%b":"");
        String fontKey = fontName+ext;
        /* NOTE LASZLO: [2003-09-22 bloch] disable font cache (see bug 2109) */
        /*
        Font font2 = FontCache.getFont( fontKey );
        */
        Font font2 = null;

        // get offset table and shape table
        int nGlyph = p.getUWord();

        // if these fonts have the same number of glyphs and the same flags then
        // we guess that this is the same font and don't parse it further
        // (we may be wrong, but it's very unlikely)
        if( font2 != null ) {
            if( nGlyph == font2.getNumGlyph() && flags == font2.flags ) {
                return new FontDef( font2, id );
            }
        }

        Font font = new Font();
        font.flags = flags;
        font.fontName = fontName;
        font.fontKey  = fontKey;
        font.fileBuffer = p.getBuf();
        int tableOffset = p.getPos();
        int[] glyphOffsets = new int[nGlyph+1]; // +1 because we need last offset
        int codeOff;
        if( (flags&Font.WIDE_OFFSETS) != 0 ) {
            for( int i=0; i<nGlyph; i++ ) glyphOffsets[i] = tableOffset+p.getUDWord();
            codeOff = p.getUDWord();
        } else {
            for( int i=0; i<nGlyph; i++ ) glyphOffsets[i] = tableOffset+p.getUWord();
            codeOff = p.getUWord();
        }
        font.glyphOffsets = glyphOffsets;

        // do not parse shapes, delay until we really need them
        p.setPos( tableOffset+codeOff );    // skip shapes
        glyphOffsets[nGlyph] = p.getPos();

        // get code table
        int[] codeTable = new int[nGlyph];
        if( (flags&Font.WIDE_CODES) != 0 ) {
            for( int i=0; i<nGlyph; i++ ) codeTable[i] = p.getUWord();
        } else {
            for( int i=0; i<nGlyph; i++ ) codeTable[i] = p.getUByte();
        }
        font.codeTable = codeTable;

        // find 'blank'
        for( int i=0; i<nGlyph; i++ ) {
            if( codeTable[i] == ' ' ) {
                font.blankPos = i;
                break;
            }
        }

        if( (flags&Font.HAS_LAYOUT) != 0 ) {
            font.ascent = p.getWord();
            font.descent = p.getWord();
            font.leading = p.getWord();

            // get advance table
            int[] advanceTable = new int[nGlyph];
            for( int i=0; i<nGlyph; i++ ) advanceTable[i] = p.getWord();
            font.advanceTable = advanceTable;

            // skip bounds table, delay until we need them
            font.boundsBuffer = p.getBuf();
            font.boundsOffset = p.getPos();
            for( int i=0; i<nGlyph; i++ ) p.skipRect();
            font.boundsLength = p.getPos()-font.boundsOffset;

            // get kerning table
            int nKern = p.getUWord();
            font.kernLeftCodes  = new int[nKern];
            font.kernRightCodes = new int[nKern];
            font.kernAdjustment = new int[nKern];
            if( (flags&Font.WIDE_CODES) != 0 ) {
                for( int i=0; i<nKern; i++ ) {
                    font.kernLeftCodes[i]  = p.getUWord();
                    font.kernRightCodes[i] = p.getUWord();
                    font.kernAdjustment[i] = p.getWord();
                }
            } else {
                for( int i=0; i<nKern; i++ ) {
                    font.kernLeftCodes[i]  = p.getUByte();
                    font.kernRightCodes[i] = p.getUByte();
                    font.kernAdjustment[i] = p.getWord();
                }
            }
        }

        // cache only if there is no already such font in cache, font has a layout and number of
        // glyphs >= 200
        /* NOTE LASZLO: [2003-09-22 bloch] disable font cache (see bug 2109) */
        /*
        if( font2 == null && (flags&Font.HAS_LAYOUT) != 0 && nGlyph >= 200 ) {
            FontCache.addFont( fontKey, font );
        }
        */

        return new FontDef( font, id );
    }

    /**
     * Builds and generates this font into buffer
     *
     * @param fob    flash buffer
     */
    public void write( FlashOutput fob ) {

        if (fob.defined( font )) {
            return;
        }

        int pos = fob.getPos();
        fob.skip(6);

        fob.writeFontID( font );
        if( isWriteLayout ) {
            fob.writeWord( font.flags );
        } else {
            fob.writeWord( font.flags&~Font.HAS_LAYOUT );
        }
        fob.writeStringL( getFontName() );

        // create index table
        int[] indexTable = new int[font.codeTable.length];

        // create codetable from registered text blocks
        int[] codeTable;
        int nGlyph;
        if( isWriteAllChars ) {
            codeTable = font.codeTable;
            nGlyph = codeTable.length;
            for( int i=0; i<indexTable.length; i++ ) indexTable[i] = i;
        } else {
            codeTable = new int[font.codeTable.length];
            nGlyph = 0;
        }

        for( int i=0; i<textBlocks.size(); i++ ) {
            TextBlock tblock = (TextBlock) textBlocks.elementAt(i);
            tblock.layout();
            IVVector trs = tblock.getTextRecords(font);
            if( trs == null || trs.size() == 0 ) continue;
            for( int t=0; t<trs.size(); t++ ) {
                TextRecord tr = (TextRecord) trs.elementAt(t);
                L1:
                for( int c=0; c<tr.getSize(); c++ ) {
                    char ch = tr.getChar(c);
                    if( ch == '\r' || ch == '\n' ) {    // how come we see this chars here ?
                        tr.setIndex(c, 0);      // ???
                        continue;
                    }
                    // find this char in our codetable
                    for( int j=0; j<nGlyph; j++ ) {
                        if( codeTable[j] == ch ) {
                            tr.setIndex(c, j);
                            continue L1;
                        }
                    }
                    // char has not been not found, add it
                    if( !isWriteAllChars ) {    // should always be true here!
                        int idx = font.getIndex(ch);    // get original index
                        if( idx == -1 ) {
                            idx = font.getIndex(' ');
                        }
                        indexTable[nGlyph] = idx;
                        tr.setIndex(c, nGlyph);
                        codeTable[nGlyph] = ch;
                        ++nGlyph;
                    }
                }
            }
        }

        fob.writeWord(nGlyph);
        int offsetTable = fob.getPos();
        int inc = (font.flags&Font.WIDE_OFFSETS)!=0? 4: 2;
        fob.skip( nGlyph*inc );
        int codeOffset = fob.getPos();
        fob.skip( inc );
        for( int i=0, curOff=offsetTable; i<nGlyph; i++, curOff+=inc ) {
            int offset = fob.getPos()-offsetTable;
            int idx = indexTable[i];
            int start = font.glyphOffsets[idx];
            int end = font.glyphOffsets[idx+1];
            fob.writeArray( font.fileBuffer, start, end-start );
            if( inc == 2 )
                fob.writeWordAt( offset, curOff );
            else
                fob.writeDWordAt( offset, curOff );
        }
        if( inc == 2 )
            fob.writeWordAt(fob.getPos()-offsetTable, codeOffset);
        else
            fob.writeDWordAt(fob.getPos()-offsetTable, codeOffset);

        for( int i=0; i<nGlyph; i++ ) {
            if( (font.flags&Font.WIDE_CODES) != 0 ) {
                fob.writeWord( codeTable[i] );
            } else {
                fob.writeByte( codeTable[i] );
            }
        }

        // write layout
        if( isWriteLayout && ((font.flags&Font.HAS_LAYOUT) != 0) ) {
            fob.writeWord( font.ascent );
            fob.writeWord( font.descent );
            fob.writeWord( font.leading );

            // write advance table
            for( int i=0; i<nGlyph; i++ ) fob.writeWord( font.advanceTable[indexTable[i]] );

            // copy bounds table
            if( isWriteAllChars ) {
                // just for the sake of perfomance, because font.getGlyphBounds() causes parsing of all bounds
                fob.writeArray( font.boundsBuffer, font.boundsOffset, font.boundsLength );
            } else {
                Rectangle2D[] bounds = font.getGlyphBounds();
                for( int i=0; i<nGlyph; i++ ) {
                    fob.write( bounds[indexTable[i]] );
                }
            }

            // write kerning tables
            // probably we need to restrict writing only to codes from codetable
            int nKern = font.kernLeftCodes.length;
            fob.writeWord( nKern );
            if( (font.flags&Font.WIDE_CODES) != 0 ) {
                for( int i=0; i<nKern; i++ ) {
                    fob.writeWord( font.kernLeftCodes[i] );
                    fob.writeWord( font.kernRightCodes[i] );
                    fob.writeWord( font.kernAdjustment[i] );
                }
            } else {
                for( int i=0; i<nKern; i++ ) {
                    fob.writeByte( font.kernLeftCodes[i] );
                    fob.writeByte( font.kernRightCodes[i] );
                    fob.writeWord( font.kernAdjustment[i] );
                }
            }
        }

        fob.writeLongTagAt(Tag.DEFINEFONT2, fob.getPos()-pos-6, pos);
    }

    /**
     * Merge two fonts
     *
     * @param f1     font to merge
     * @param f2     font to merge
     * @return new merged font
     */
    public static Font mergeFonts( Font f1, Font f2 ) {
        //System.out.println( "Merging fonts: "+f2.getFontName() );
        int flags = f2.flags | f1.flags;
        //System.out.println( "f2_flags: "+Util.w2h(f2.flags)+", f1_flags: "+Util.w2h(f2.flags) );

        boolean f1_hasLayout = (f1.flags&Font.HAS_LAYOUT) != 0;
        boolean f2_hasLayout = (f2.flags&Font.HAS_LAYOUT) != 0;
        if( f1_hasLayout && !f2_hasLayout ) {
            Font f = f1; f1 = f2; f2 = f;
            boolean fb = f1_hasLayout; f1_hasLayout = f2_hasLayout; f2_hasLayout = fb;
        }

        // merge code tables
        int[] codeTable1 = f1.codeTable;
        int[] codeTable2 = f2.codeTable;
        int n = 0;
        int[] codeTable_tmp = new int[codeTable1.length];
        int[] indexes = new int[codeTable1.length];
        for( int i=0; i<codeTable1.length; i++ ) {
            int code = codeTable1[i];
            int j=0;
            for( ; j<codeTable2.length; j++ ) {
                if( codeTable2[j] == code ) break;
            }
            if( j == codeTable2.length ) {
                codeTable_tmp[n] = code;
                indexes[n] = i;
                n++;
            }
        }

        if( n == 0 ) {  // no new characters in f1
            f2.copyTo(f1);  // no need!!!
            return f2;
        }
        //System.out.println( "continue merge" );

        int nGlyph = codeTable2.length+n;
        int[] codeTable = new int[nGlyph];
        System.arraycopy( codeTable2, 0, codeTable, 0, codeTable2.length );
        System.arraycopy( codeTable_tmp, 0, codeTable, codeTable2.length, n );

        // merge glyphs
        int f1_start = f1.glyphOffsets[0];
        int f1_end   = f1.glyphOffsets[codeTable1.length];
        int f2_start = f2.glyphOffsets[0];
        int f2_end   = f2.glyphOffsets[codeTable2.length];
        int totalSize = f1_end-f1_start + f2_end-f2_start;
        byte[] fileBuffer = new byte[ totalSize ];
        int[] glyphOffsets = new int[ nGlyph + 1 ];
        int offset = f2_end-f2_start;
        // copy glyphoffset and shapes from f2
        System.arraycopy( f2.fileBuffer, f2_start, fileBuffer, 0, offset );
        int gln = 0;
        for( ; gln<codeTable2.length; gln++ ) {
            int off = f2.glyphOffsets[gln] - f2_start;
            glyphOffsets[gln] = off;
        }
        // copy glyphoffsets and shapes from f1
        for( int i=0; i<n; i++, gln++ ) {
            int idx = indexes[i];
            int start = f1.glyphOffsets[idx];
            int end   = f1.glyphOffsets[idx+1];
            int size  = end-start;
            System.arraycopy( f1.fileBuffer, start,
                              fileBuffer, offset, size );
            glyphOffsets[gln] = offset;
            offset += size;
        }
        glyphOffsets[gln] = offset;

        int[] advanceTable = null;
        int[] kernLeftCodes = f2.kernLeftCodes;
        int[] kernRightCodes = f2.kernRightCodes;
        int[] kernAdjustment = f2.kernAdjustment;
        // 2 cases:
        //  - no fonts have layouts
        //  - at least f2 has layout
        if( f1_hasLayout || f2_hasLayout ) {
            if( f1_hasLayout ) {
                // merge two layouts
                // extend advancetable and boundstable
                advanceTable = new int[nGlyph];
                int off = codeTable2.length;
                System.arraycopy( f2.advanceTable, 0, advanceTable, 0, off );
                for( int i=0; i<n; i++, off++ ) {
                    int idx = indexes[i];
                    int adv = f1.advanceTable[idx];
                    advanceTable[off] = adv;
                }
            } else {
                // use f2 layout
                // extend advancetable and boundstable
                advanceTable = new int[nGlyph];
                System.arraycopy( f2.advanceTable, 0, advanceTable, 0, codeTable2.length );
                for( int i=codeTable2.length; i<nGlyph; i++ ) {
                    advanceTable[i] = 500;
                }
            }
        }

        synchronized(f2) {
            // store new data to f2 font
            for( int i=0; i<nGlyph; i++ ) {
                if( codeTable[i] == ' ' ) {
                    f2.blankPos = i;
                    break;
                }
            }
            f2.kernAdjustment = kernAdjustment;
            f2.kernLeftCodes = kernLeftCodes;
            f2.kernRightCodes = kernRightCodes;
            f2.advanceTable = advanceTable;
            f2.flags = flags;
            f2.codeTable = codeTable;
            f2.glyphOffsets = glyphOffsets;
            f2.fileBuffer = fileBuffer;

            // need to copy bounds buffer too !!!
            // .....
            // ....

            f2.copyTo(f1);  // no need !!!
        }

        return f2;
    }

    public void printContent( PrintStream out, String indent ) {
        font.printContent(out, indent, getID());
    }

    public boolean isConstant() {
        return true;
    }

    protected FlashItem copyInto( FlashItem item, ScriptCopier copier ) {
        super.copyInto( item, copier );
        ((FontDef)item).font = font;
        ((FontDef)item).isWriteAllChars = isWriteAllChars;
        ((FontDef)item).isWriteLayout = isWriteLayout;
        // nothing else
        return item;
    }

    public FlashItem getCopy( ScriptCopier copier ) {
        return copyInto( new FontDef(), copier );
    }

    /**
     * Change font in all specified records to new one
     *
     * @param records  vector containing TextRecord and TextStyleChangeRecord
     * @param new_font new font
     */
    public static void changeRecordsFont( IVVector records, Font old_font, Font new_font ) {
        //System.out.println( "changeRecordsFont:" );
        Font last_font = null;
        for( int i=0; i<records.size(); i++ ) {
            Object o = records.elementAt(i);
            if( o instanceof TextRecord ) {
                if( last_font == old_font ) {
                    ((TextRecord)o).updateIndexes(new_font);
                }
            } else {
                TextStyleChangeRecord trs = (TextStyleChangeRecord) o;
                Font font = trs.getFont();
                if( font != null ) {
                    last_font = font;
                    if( last_font == old_font ) {
                        trs.setFont(new_font);
                    }
                }
            }
        }
    }
}
