/*
 * $Id: Font.java,v 1.3 2002/07/12 07:46:51 skavish Exp $
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
import org.openlaszlo.iv.flash.api.shape.*;
import java.io.*;
import java.awt.geom.Rectangle2D;

/**
 * This class defines flash font.
 * <P>
 * Flash text has been designed to be completely device independent.
 * Text is guaranteed to look exactly the same on every device, regardless
 * of which fonts are installed on the client machine.
 * The SWF format achieves this by including the exact shape of every letter,
 * number (or other text character) used in the movie.  These character shape
 * definitions are called glyphs.
 * <P>
 * Defining each and every glyph increases the size of a SWF file,
 * particularly if the font is complex.  However, it is a necessary tradeoff.
 * At design time, Flash knows nothing about the capabilities of the client device,
 * therefore glyphs must always be included in the SWF file, even if the desired font
 * is already on the client machine.
 * <P>
 * To guarantee text is reproduced correctly, SWF also includes the exact position
 * of every character in a text block.  Again, this adds to the file size, but allows
 * sophisticated text layout effects (like kerning and text wrapping) without requiring
 * a complex layout engine built into the Flash player.
 * <P>
 *
 * <h4>Glyph Definitions</h4>
 * <P>
 * Glyphs are defined once in a standard coordinate
 * space called the EM square.  The same set of glyphs are
 * used for every point size of a given font.  To render a glyph
 * at different point sizes, the Flash player scales the glyph
 * from EM coordinates to point-size coordinates.
 * <P>
 * Flash fonts do not include any hinting information for improving
 * the quality of small font sizes.  However, antialiasing dramatically
 * improves the legibility of down-scaled text.  Flash text remains
 * legible down to about 12-points (viewed at 100%).  Below that, text
 * may appear fuzzy and blurred.  In any case, it is rare for Flash movies
 * to be used for large bodies of text with small point sizes.
 * <P>
 * TrueType fonts can be readily converted to SWF glyphs.  A simple
 * algorithm can replace the Quadratic B-splines (used by TrueType) with
 * Quadratic Bezier curves (used by SWF).
 * <P>
 *
 * <H4>The EM Square</H4>
 * <P>
 * The EM square is an imaginary square that
 * is used to size and align glyphs. The EM square is
 * generally large enough to completely contain all glyphs,
 * including accented glyphs.  It includes the font's ascent,
 * descent, and some extra spacing to prevent lines of text from
 * colliding.
 * <P>
 * SWF glyphs are always defined on an EM square of 1024 by 1024
 * units.  Glyphs from other sources (such as TrueType fonts) may
 * be defined on a different EM square.  To use these glyphs in SWF,
 * they should be scaled to fit an EM square of 1024.
 * <P>
 *
 * <H4>Kerning and Advance Values</H4>
 * <P>
 * Kerning defines the horizontal distance between two glyphs.
 * This distance may be smaller or larger than the width of the
 * left-hand glyph.  Some kerning pairs are more aesthetically
 * pleasing if they are moved closer together.
 * <P>
 * SWF stores kerning information as an advance value.
 * That is, the horizontal advance from one glyph to another.
 * SWF stores an advance value for every character in a text block.
 * <P>
 *
 * <H4>The DefineFont Tag</H4>
 * <P>
 * The <CODE>DefineFont</CODE> tag defines the shape outlines of
 * each glyph used in a particular font.  Only the glyphs that are
 * used by subsequent <CODE>DefineText</CODE> tags are actually defined.
 * <P>
 * The FontId uniquely identifies the font.  It can be used by subsequent
 * <CODE>DefineText</CODE> tags to select the font.
 * <P>
 * The offset table and shape table are used together.  These tables
 * have the same number of entries, and there is a one-to-one ordering match
 * between the order of the offsets, and the order of the shapes.
 * The offset table points to locations in the shape table.  Each offset entry
 * stores the difference (in bytes) between the start of the offset table and
 * the location of the corresponding shape:
 * <CODE><pre>
 * Location of ShapeRecord[n] = StartOfOffsetTable + OffsetTable[n]
 * </PRE></CODE>
 * Because the <CODE>ShapeTable</CODE> immediately follows the <CODE>OffsetTable</CODE>,
 * the number of entries in both tables can be inferred by dividing
 * the first offset by two:
 * <CODE><PRE>
 * Shape count = OffsetTable[0] / 2
 * </PRE></CODE>
 * <P>
 *
 * <H4>Mapping to Native Fonts</H4>
 * <P>
 * SWF also supports the use of native fonts. Rather than using the
 * glyph outlines in the DefineFont tag, fonts can be rendered using
 * the client machine's font engine.   Since most native font engines
 * include hinting techniques, they may produce better results at very
 * small point sizes.
 * <P>
 * The DefineFontInfo tag defines the mapping of a Flash font to a native font.
 * It includes the font name, font style - bold, italic, or plain, and the
 * encoding scheme used - ANSI, Unicode or ShiftJIS.  It also defines a mapping
 * of glyph indices to character codes.  Thus if 'a' were the first character in
 * your DefineFont tag, the DefineFontInfo tag would map index zero to the character
 * code for 'a'.
 * <P>
 *
 * <H4>DefineFont2</H4>
 * <P>
 * The DefineFont2 tag extends the functionality of DefineFont.
 * Enhancements include:
 * <UL>
 * <LI>32-bit entries in the OffsetTable, for fonts with more than 64K glyphs.
 * <li>Mapping to native fonts, by incorporating all the functionality of DefineFontInfo.
 * <LI>Ascent, descent and leading information.
 * <LI>An advance table that defines the advance for each glyph (in EM square coordinates).
 * <LI>A bounds table that defines the bounding-box of each glyph (in EM square coordinates).
 * <LI>A table of kerning pairs that defines the distance between pairs of glyphs
 * </UL>
 * <P>
 *
 * <H4>Kerning Record</H4>
 * <P>
 * A Kerning Record defines the distance between two glyphs in EM
 * square coordinates.  Certain pairs of glyphs appear more aesthetically
 * pleasing if they are moved closer together, or farther apart.
 * The FontKerningCode1 and FontKerningCode2 fields are the character codes
 * for the left and right characters.  The FontKerningAdjustment field is a
 * signed integer that defines the offset from the advance value of the
 * left-hand character.  The distance between two characters can be calculated
 * like this:
 * <CODE><PRE>
 * Distance = FontAdvanceTable[ord(FontKerningCode1)] + FontKerningAdjustment
 * </PRE></CODE>
 *
 * @author Dmitry Skavish
 */
public final class Font {

    public static final int HAS_LAYOUT    = 0x0080;
    public static final int SHIFT_JIS     = 0x0040;
    public static final int UNICODE       = 0x0020;
    public static final int ANSI          = 0x0010;
    public static final int WIDE_OFFSETS  = 0x0008;
    public static final int WIDE_CODES    = 0x0004;
    public static final int ITALIC        = 0x0002;
    public static final int BOLD          = 0x0001;

    public int flags;               // flags
    public String fontName;         // font name
    public String fontKey;          // font key in cache (constructed from name and italic and bold flags)
    public boolean cached = false;  // true if font is cached
    public byte[] fileBuffer;       // glyph buffer (usually .swt file buffer)
    public int[] glyphOffsets;      // offsets of glyphs in glyph buffer
    private BareShape[] glyphs;     // shapes of all glyphs (these shapes are parsed from glyph buffer on demand)
    public int[] codeTable;         // array of codes of all characters of the font, index in this array is index in every other array
    public int blankPos = 0;        // index of 'space' (optimization)
    public int[] advanceTable;      // advance values for coresponding characters
    public int ascent;              // ascent of the font
    public int descent;             // descent of the font
    public int leading;             // leading of the font
    // kerning tables
    public int[] kernLeftCodes;     // left characters in kerning table
    public int[] kernRightCodes;    // right characters in kerning table
    public int[] kernAdjustment;    // adjustment for left-right characters pair in kerning table
    // not used stuff
    public byte[] boundsBuffer;     // bounds buffer
    public int boundsOffset;        // offset of bounds table in bounds buffer
    public int boundsLength;        // length of bounds table in bounds buffer
    private Rectangle2D[] bounds;   // parsed bounds (parsed from bounds buffer on demand)

    public Font() {
        init( null );
    }

    /**
     * Creates empty font of given name.
     *
     * @param fontName name of the created font
     * @return created empty font
     */
    public static Font createDummyFont( String fontName ) {
        Font f = new Font();
        f.init( fontName );
        return f;
    }

    private void init( String fontName ) {
        flags = HAS_LAYOUT | ANSI;
        fontName = fontName;
        codeTable = new int[] { ' ' };
        advanceTable = new int[] { 0 };
        kernRightCodes = kernAdjustment = kernLeftCodes = new int[] { 0 };
        fileBuffer = new byte[0];
        glyphOffsets = new int[] { 0, 0 };
    }

    /**
     * Returns size of glyphs table.
     * <P>
     * Since glyph table has one additional element to hold
     * offset after the last element then number of glyphs of the font
     * is one less than the size of the glyph table.
     *
     * @return number of glyphs + 1
     */
    public int getGlyphTableSize() {
        return glyphOffsets.length;
    }

    /**
     * Returns number of glyphs in the font
     *
     * @return number of glyphs
     */
    public int getNumGlyph() {
        return glyphOffsets.length-1;
    }

    /**
     * Returns index of specified character in codetable.
     *
     * @param ch     specified character which index is to be returned
     * @return index of specified character or -1 if there is no such character
     */
    public int getIndex( int ch ) {
        try {
            int idx = ch-' '+blankPos;
            if( codeTable[idx] == ch ) return idx;
        } catch( ArrayIndexOutOfBoundsException e ) {}

        for( int i=0; i<codeTable.length; i++ ) {
            if( ch == codeTable[i] ) return i;
        }
        return -1;
    }

    /**
     * Returns font name
     *
     * @return font name
     */
    public String getFontName() {
        return fontName;
    }

    /**
     * Returns advance value for the character at specified index.
     *
     * @param idx    index of character which advance value is to be returned
     * @return advance value at specified index
     */
    public int getAdvanceValue( int idx ) {
        try {
            return advanceTable[idx];
        } catch( Exception e ) {
            return 0;
        }
    }


    /**
     * Return adjustment from kerning table for specified pair of characters
     * <p>
     * MAY BE IMPROVED BY USING BINARY SEARCH !!!!!!!!!!!!
     *
     * @param ch_left  left character
     * @param ch_right right character
     * @return adjustment value for the pair of character
     */
    public int getKerning( int ch_left, int ch_right ) {
        for( int i=0; i<kernLeftCodes.length; i++ ) {
            if( ch_left == kernLeftCodes[i] && ch_right == kernRightCodes[i] ) {
                return kernAdjustment[i];
            }
        }
        return 0;
    }

    /**
     * Returns array of shapes of all glyphs of this font
     * <P>
     * Shapes parsing can be delayed until they are requested by this call
     *
     * @return array of font glyphs
     */
    public BareShape[] getGlyphs() {
        if( glyphs == null ) {
            Parser p = new Parser();
            glyphs = new BareShape[ getNumGlyph() ];
            for( int i=0; i<glyphs.length; i++ ) {
                p.init(fileBuffer, glyphOffsets[i], fileBuffer.length);
                glyphs[i] = BareShape.parseBareShape(p);
            }
        }
        return glyphs;
    }

    /**
     * Returns array of bounds of all glyphs of this font
     *
     * @return array of glyphs bounds
     */
    public Rectangle2D[] getGlyphBounds() {
        if( bounds == null && boundsBuffer != null ) {
            Parser p = new Parser();
            bounds = new Rectangle2D[ getNumGlyph() ];
            p.init(boundsBuffer, boundsOffset, boundsBuffer.length);
            for( int i=0; i<bounds.length; i++ ) {
                bounds[i] = p.getRect();
            }
        }
        return bounds;
    }

    /**
     * Returns true if this font is large then the specified one
     * <P>
     * Font is considered large if it has more glyphs and layout
     *
     * @param f      font to be compared with
     * @return true if this font is large then the specified one
     */
    public boolean isLargeThan( Font f ) {
        if( getNumGlyph() > f.getNumGlyph() ) return true;
        if( getNumGlyph() < f.getNumGlyph() ) return false;

        if( (f.flags&Font.HAS_LAYOUT) != 0 && (flags&Font.HAS_LAYOUT) == 0 ) return false;
        if( (flags&Font.HAS_LAYOUT) != 0 && (f.flags&Font.HAS_LAYOUT) == 0 ) return true;

        return getFontSize() > f.getFontSize();
    }

    /**
     * Returns approximate size in bytes occupied by this font
     * <p>
     * Used mainly in caching algorithm.
     *
     * @return size of this font in bytes
     */
    public int getFontSize() {
        int size = fileBuffer.length;
        if( codeTable != null ) size += codeTable.length*4;
        if( advanceTable != null ) size += advanceTable.length*4;
        if( kernLeftCodes != null ) size += kernLeftCodes.length*4;
        if( kernRightCodes != null ) size += kernRightCodes.length*4;
        if( kernAdjustment != null ) size += kernAdjustment.length*4;
        if( boundsBuffer != null && boundsBuffer != fileBuffer ) size += boundsBuffer.length;
        return size;
    }

    public void printContent( PrintStream out, String indent, int id ) {
        out.println( indent+"Font: id="+id+", fontName="+fontName+", flags="+Util.w2h(flags)+", nGlyph="+(glyphOffsets.length-1) );
        /*
        out.println( indent+"  Glyphs:" );
        BareShape[] shapes = getGlyphs();
        for( int i=0; i<shapes.length; i++ ) {
            out.println( indent+"    i="+i+", code="+codeTable[i]);
            shapes[i].printContent(out, indent+"      ");
        }
        */
        out.print( indent+"  CodeTable:" );
        for( int i=0; i<codeTable.length; i++ ) {
            if( (i%10) == 0 ) {
                out.println(); out.print( indent+"    " );
            }
            if( (flags&WIDE_CODES) != 0 ) {
                out.print( Util.b2h(i)+"["+Util.toPrint( (char)codeTable[i] )+"(0x"+Util.w2h(codeTable[i])+")] " );
            } else {
                out.print( Util.b2h(i)+"["+Util.toPrint( (char)codeTable[i] )+"(0x"+Util.b2h(codeTable[i])+")] " );
            }
        }
        out.println();
        if( (flags&HAS_LAYOUT) != 0 ) {
            out.println( indent+"  hasLayout: ascent="+ascent+", descent="+descent+", leading="+leading );

            out.println( indent+"  bounds: " );
            Rectangle2D[] bounds = getGlyphBounds();
            for( int i=0; i<bounds.length; i++ ) {
                out.println( indent+"    index="+i+", rect: "+bounds[i].toString() );
            }
            out.print( indent+"  AdvanceTable:" );
            for( int i=0; i<advanceTable.length; i++ ) {
                if( (i%10) == 0 ) {
                    out.println(); out.print( indent+"    " );
                }
                out.print( Util.b2h(i)+"["+advanceTable[i]+"] " );
            }
            out.println();
            out.println( indent+"  KerningTable:" );
            for( int i=0; i<kernLeftCodes.length; i++ ) {
                int lc = kernLeftCodes[i];
                int rc = kernRightCodes[i];
                int ad = kernAdjustment[i];
                if( (flags&WIDE_CODES) != 0 ) {
                    out.println( indent+"    "+"["+Util.toPrint((char)lc)+"(0x"+Util.w2h(lc)+")] --> ["+Util.toPrint((char)rc)+"(0x"+Util.w2h(rc)+")] = "+ad );
                } else {
                    out.println( indent+"    "+"["+Util.toPrint((char)lc)+"(0x"+Util.b2h(lc)+")] --> ["+Util.toPrint((char)rc)+"(0x"+Util.b2h(rc)+")] = "+ad );
                }
            }
        }
    }

    /**
     * Copy all data from this font to the specified one
     *
     * @param font   specified font copy all the data to
     */
    public void copyTo( Font font ) {
        font.flags          = flags;
        font.fontName       = fontName;
        font.fontKey        = fontKey;
        font.fileBuffer     = fileBuffer;
        font.glyphOffsets   = glyphOffsets;
        font.glyphs         = glyphs;
        font.codeTable      = codeTable;
        font.blankPos       = blankPos;
        font.advanceTable   = advanceTable;
        font.boundsBuffer   = boundsBuffer;
        font.boundsOffset   = boundsOffset;
        font.boundsLength   = boundsLength;
        font.bounds         = bounds;
        font.ascent         = ascent;
        font.descent        = descent;
        font.leading        = leading;
        font.kernLeftCodes  = kernLeftCodes;
        font.kernRightCodes = kernRightCodes;
        font.kernAdjustment = kernAdjustment;
        /*
        font.glyphOffsets = new int[glyphOffsets.length];
        System.arraycopy(glyphOffsets, 0, font.glyphOffsets, 0, glyphOffsets.length);
        font.codeTable = new int[codeTable.length];
        System.arraycopy(codeTable, 0, font.codeTable, 0, codeTable.length);
        font.advanceTable = new int[advanceTable.length];
        System.arraycopy(advanceTable, 0, font.advanceTable, 0, advanceTable.length);
        font.kernLeftCodes = new int[kernLeftCodes.length];
        System.arraycopy(kernLeftCodes, 0, font.kernLeftCodes, 0, kernLeftCodes.length);
        font.kernRightCodes = new int[kernRightCodes.length];
        System.arraycopy(kernRightCodes, 0, font.kernRightCodes, 0, kernRightCodes.length);
        font.kernAdjustment = new int[kernAdjustment.length];
        System.arraycopy(kernAdjustment, 0, font.kernAdjustment, 0, kernAdjustment.length);*/
    }

}
