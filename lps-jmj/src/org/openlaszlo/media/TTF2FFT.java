/******************************************************************************
 * TTF2FFT.java
 * ****************************************************************************/

/* J_LZ_COPYRIGHT_BEGIN *******************************************************
* Copyright 2001-2004 Laszlo Systems, Inc.  All Rights Reserved.              *
* Use is subject to license terms.                                            *
* J_LZ_COPYRIGHT_END *********************************************************/

package org.openlaszlo.media;

import java.io.InputStream;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.FileNotFoundException;
import java.io.File;
import java.awt.geom.Rectangle2D;

// JGenerator APIs
import org.openlaszlo.iv.flash.api.*;
import org.openlaszlo.iv.flash.api.shape.*;
import org.openlaszlo.iv.flash.api.sound.*;
import org.openlaszlo.iv.flash.api.text.Font;
import org.openlaszlo.iv.flash.api.image.*;
import org.openlaszlo.iv.flash.util.*;

import org.openlaszlo.server.LPS;

// Logger
import org.apache.log4j.*;

// Apache Batik TrueType Font Parser
import org.apache.batik.svggen.font.*;
import org.apache.batik.svggen.font.table.*;

/**
 * TrueType Font to Flash Font converter
 *
 * @author <a href="mailto:bloch@laszlosystems.com">Eric Bloch</a>
 */
public class TTF2FFT {

    /** Character code used by lfc for newline processing 
     * See LzFont.as and LzFontManager.as.  0 doesn't work for some reason.
     */
    private final static int LFC_TMARK = 160;

    /** Logger */
    private static Logger mLogger = Logger.getLogger(TTF2FFT.class);

    /** Units per EmSquare for FFTs */
    private static final int FFT_UnitsPerEm = 1024;

    /** 
     * @param input input TTF file
     * @return InputStream FFT
     */
    public static InputStream convert(File input) 
        throws TranscoderException, FileNotFoundException {

        String path = input.getPath();

        if (!input.exists()) {
            throw new FileNotFoundException(path);
        }

        // Batik should throw an exception when it can't read
        // the file (for access perms), but it doesn't.
        if (!input.canRead()) {
            throw new FileNotFoundException("Can't read: " + path);
        }

        org.apache.batik.svggen.font.Font ttf;
        ttf = org.apache.batik.svggen.font.Font.create(input.getPath());

        NameTable nameTable = ttf.getNameTable();
        String    fontName = "";
        if (nameTable == null) {
            fontName = input.getName();
            int index = fontName.indexOf(".");
            if (index > 0) {
                fontName = fontName.substring(0, index);
            }
            mLogger.warn("font missing ttf name table; made name, " + fontName + ", based on filename ");
        } else {
            fontName = nameTable.getRecord((short)1); 
        }
        HeadTable headTable = ttf.getHeadTable();
        HmtxTable hmtxTable = ttf.getHmtxTable();

        if (headTable == null) {
            // Bitmap fonts aren't required to have the head table.
            // We don't support them yet. XXX
            throw new TranscoderException(path + " missing ttf head table; this ttf font not supported");
        }

        if (hmtxTable == null) {
            throw new TranscoderException(path + " missing ttf hmtx (horiz. metrics) table; this ttf font not supported");
        }

        // FFT flags
        int     flags    = 0;

        // Is font bold, italic, or bold-italic?
        int     macStyle = headTable.getMacStyle();
        boolean isBold   = (macStyle & 0x1) != 0;
        boolean isItalic = (macStyle & 0x2) != 0;

        boolean isUnicode = false;
                                       
                                       
        if (isBold)
            flags |= org.openlaszlo.iv.flash.api.text.Font.BOLD;
        if (isItalic)
            flags |= org.openlaszlo.iv.flash.api.text.Font.ITALIC;

        // We have font metric info for the ttf
        flags |= org.openlaszlo.iv.flash.api.text.Font.HAS_LAYOUT;

        final int maxCodes = 0xffff;
        int       numCodes = 0;

        int [] codeTable  = new int[maxCodes];
        int [] indexTable = new int[maxCodes];
        int    maxCode    = 0;

        // Add Code 0 (not sure why this is needed.  Probably some lfc reason
        codeTable[0] = 0;
        indexTable[0] = 0;
        numCodes = 1;

        // 3 tries
        final int NUM_TRIES = 3;
        short [] cmapPlats = {
                Table.platformMicrosoft,
                Table.platformMacintosh,
                Table.platformMicrosoft,
        };

        short [] cmapEncodes = {
                Table.encodingUGL,
                Table.encodingRoman,
                Table.encodingUndefined,
        };

        boolean [] cmapIsUnicode = {
            true,
            false,
            false,
        };

        int tries = 0;


        CmapFormat cmapFmt = null;
        boolean hasTmark = false;
        int spaceIndex = 0;

        for (int t = 0; t < NUM_TRIES; t++) {

            cmapFmt = ttf.getCmapTable().getCmapFormat(cmapPlats[t], cmapEncodes[t]);
            // Find char codes
            if (cmapFmt != null) {
                for (int ch = 0; ch < 0xffff; ch++) {
                    int index = cmapFmt.mapCharCode(ch);

                    if (ch == 32) {
                        spaceIndex = index;
                    }
        
                    if (index != 0) {
                        if (ch == LFC_TMARK) {
                            hasTmark = true;
                        }
                        codeTable[numCodes] = ch;
                        indexTable[numCodes] = index;
                        numCodes++;
                        if (ch > maxCode) {
                            maxCode = ch;
                        }
                    }
                }
            }
            if (numCodes > 1) {
                break;
            }
            isUnicode = cmapIsUnicode[t];
        }

        if (cmapFmt == null) {
            throw new TranscoderException("Can't find a cmap table in " + path);
        }

        if (!hasTmark) {
            if (LFC_TMARK > maxCode) {
                maxCode = LFC_TMARK;
            }

            codeTable[numCodes] = LFC_TMARK;
            indexTable[numCodes] = spaceIndex;
            numCodes++;
        }

        if (isUnicode)
            flags |= org.openlaszlo.iv.flash.api.text.Font.UNICODE;
        else 
            flags |= org.openlaszlo.iv.flash.api.text.Font.ANSI;
    
        boolean useWideCodes = (maxCode > 255);
        if (useWideCodes)
            flags |= org.openlaszlo.iv.flash.api.text.Font.WIDE_CODES;

        GlyfTable glyfTable = (GlyfTable)ttf.getTable(
                org.apache.batik.svggen.font.table.Table.glyf);

        //int              numGlyphs = ttf.getNumGlyphs();
        int              numGlyphs = numCodes;
        Shape []         shapeTable  = new Shape[numGlyphs];
        Rectangle2D []   boundsTable = new Rectangle2D[numGlyphs];

        int unitsPerEm = headTable.getUnitsPerEm();
        double factor = (double)FFT_UnitsPerEm / (double)unitsPerEm;

        // Get glyph shapes, and bounds.
        for (int i = 0; i < numGlyphs; i++) {
            int          index = indexTable[i];
            int          code  = codeTable[i];
            GlyfDescript glyf  = glyfTable.getDescription(index);
            TTFGlyph     glyph = null;

            if (glyf != null) {
                glyph = new TTFGlyph(glyf);
                glyph.scale(factor);
                mLogger.debug("index: " + index + 
                              " charcode: " + code + 
                              " char: " + (char)code + 
                              " numPoints: " + glyph.getNumPoints());
            } else {
                mLogger.debug("index: " + index + 
                              " charcode: " + code + 
                              " has no glyph.");
            }


            Shape shape = new Shape();
            shape.newStyleBlock();
            convertGlyphToShape(glyph, shape);
            shapeTable[i] = shape;
            
            int x, w, y, h;

            if (glyf != null) {
                x = (int)Math.round(glyf.getXMinimum() * factor);
                y = (int)Math.round(glyf.getYMaximum() * -factor);
                w = (int)Math.round((glyf.getXMaximum() - glyf.getXMinimum()) * factor);
                h = (int)Math.round((glyf.getYMaximum() - glyf.getYMinimum()) * factor);
            } else {
                // Heuristic that hopefully works out ok for
                // missing glyfs.  First try space.  Then try index0
                glyf = glyfTable.getDescription(spaceIndex);
                if (glyf == null) {
                    glyf = glyfTable.getDescription(0);
                }
                if (glyf != null) {
                    w = (int)Math.round((glyf.getXMaximum() - glyf.getXMinimum()) * factor);
                } else {
                    w = 0;
                }
                x = y = h = 0;
            }
            boundsTable[i] = new Rectangle2D.Double(x, y, w, h);
            shape.setBounds(boundsTable[i]);
        }

        // Create a 40K buffer for generating the FFT
        FlashOutput buf = new FlashOutput( 40*1024 ); 

        // write header.
        final int TWIP = 20;

        buf.writeByte( 'F' );
        buf.writeByte( 'W' );
        buf.writeByte( 'S' );
        // write version
        buf.writeByte( 5 );
        // skip file size
        buf.skip(4);
        // write rect
        buf.write( new Rectangle2D.Double(0, 0, 5*TWIP, 5*TWIP) );         
        // write frame rate
        buf.writeWord( 10 << 8 ); 

        // Frame count
        buf.writeWord(0);

        // Remember position
        int tagPos = buf.getPos();

        // Skip definefont2 tag header
        buf.skip(6);

        // Write font id
        buf.writeWord(1);

        // Skip flags
        int flagsPos = buf.getPos();
        buf.skip(2);

        // Write font name
        buf.writeStringL(fontName);

        // Write number of glyphs
        buf.writeWord(numGlyphs);

        int [] offsetTable = new int [numGlyphs]; 

        // Write out the converted shapes into a temporary buffer
        // And remember their offsets
        FlashOutput glyphBuf = new FlashOutput(20*1024);
        for( int i=0; i < numGlyphs; i++ ) {

            offsetTable[i] = glyphBuf.getPos();

            mLogger.debug("Writing shape " + i);
            // 1 bit of line and fill
            glyphBuf.writeByte(0x11);

            ShapeRecords shapeRecords = shapeTable[i].getShapeRecords();
            shapeRecords.write(glyphBuf, 1, 1);
            // Write end of shape records
            glyphBuf.writeBits(0, 6);
            glyphBuf.flushBits();
        }

        // UseWideOffset if glyph buf + offset table + codeTable offset
        // is bigger than 16bit int
        boolean useWideOffsets = glyphBuf.getSize() + (numGlyphs+1)*2 > 0xffff;

        // Write offsets and codeTable offset
        if (useWideOffsets) {
            int offset = (numGlyphs+1)*4;
            flags |= org.openlaszlo.iv.flash.api.text.Font.WIDE_OFFSETS;
            for(int i = 0; i < numGlyphs; i++) {
                buf.writeDWord(offsetTable[i] + offset);
            }
            buf.writeDWord( glyphBuf.getSize() + offset );
        }
        else {
            int offset = (numGlyphs+1)*2;
            for(int i = 0; i < numGlyphs; i++) {
                buf.writeWord(offsetTable[i] + offset);
            }
            buf.writeWord( glyphBuf.getSize() + offset );
        }

        // Write shapes
        buf.writeFOB(glyphBuf);

        // Write out char code table. (glyph index to char code)
        for( int i=0; i<numCodes; i++ ) {
            if(useWideCodes) {
                buf.writeWord( codeTable[i] );
            } else {
                buf.writeByte( codeTable[i] );
            }
        }

        // Write ascent, descent, (external) leading
        int ascent = (int)Math.round((ttf.getAscent() * factor));
        int descent = (int)Math.round((ttf.getDescent() * -factor));
        int leading = ascent + descent - FFT_UnitsPerEm;
        mLogger.debug("Font metrics: " + ascent + " " + descent + " " +
                leading );

        buf.writeWord( ascent );
        buf.writeWord( descent );
        buf.writeWord( leading );

        // Write advance table 
        for( int i=0; i<numCodes; i++ )  {
            int index = indexTable[i];
            buf.writeWord((int)Math.round(hmtxTable.getAdvanceWidth(index) *factor));
        }

        // Write bounds table
        for( int i=0; i<numCodes; i++ ) {
            buf.write( boundsTable[i] );
        }

        // Write kerning tables 
        int nKern = 0;

        KernTable kernTable = (KernTable)ttf.getTable(Table.kern);
        // TODO: [2003-11-05 bloch] this should be passed in as an argument and taken
        // from the font definition in the LZX file
        boolean doKern = LPS.getProperty("lps.font.kerning", "false").equals("true");

        if (kernTable != null) {
            if (doKern) {
                KernSubtable kst = kernTable.getSubtable(0);
                nKern = kst.getKerningPairCount();
                mLogger.debug(nKern + " kern pairs");
                // We optimize out all 0s
                int goodKern = nKern;
                for (int i = 0; i < nKern; i++) {
                    if (kst.getKerningPair(i).getValue() == 0) {
                        goodKern--;
                    }
                }
                buf.writeWord(goodKern);
                mLogger.debug(goodKern + " non-zero kern pairs");
                for (int i = 0; i < nKern; i++) {
                    KerningPair pair = kst.getKerningPair(i);
                    if (pair.getValue() != 0) {
                        if (useWideCodes) {
                            buf.writeWord( codeTable[pair.getLeft()] );
                            buf.writeWord( codeTable[pair.getRight()] );
                        } else {
                            buf.writeByte( codeTable[pair.getLeft()] );
                            buf.writeByte( codeTable[pair.getRight()] ) ;
                        }
                        buf.writeWord( (int)Math.round(pair.getValue()*factor) );
                    }
                }
            } else {
                mLogger.warn("skipping non-empty kerning table in " + path);
            }
        } else {
            buf.writeWord( 0 );
        }

        // Write the DEFINEFONT2 tag
        int x = buf.getPos() - tagPos - 6;
        buf.writeLongTagAt(Tag.DEFINEFONT2, x, tagPos);
        // Write the flags
        buf.writeWordAt(flags, flagsPos);

        // Write the END tag
        Tag.END_TAG.write( buf );

        // Write the file size back at the beginning.
        int filesize = buf.getSize();
        buf.writeDWordAt( filesize, 4 );

        return buf.getInputStream();
    }

    /**
     * Convert TTF Glyph to Flash Shape
     * @param glyph
     * @param shape
     */
    private static void convertGlyphToShape(TTFGlyph glyph, Shape shape) {

        if (glyph == null) {
            return;
        }
        int firstIndex = 0;
        int count = 0;

        // Add each contour to the shape.
        for (int i = 0; i < glyph.getNumPoints(); i++) {
            count++;
            if (glyph.getPoint(i).endOfContour) {
                addContourToShape(shape, glyph, firstIndex, count);
                firstIndex = i + 1;
                count = 0;
            }
        }
    }

    /**
     * Add glyphs contour starting from index point and going 
     * count number of points to shape.
     * @param shape
     * @param glyph
     * @param startIndex
     * @param count
     */
    private static void addContourToShape(Shape shape, 
            TTFGlyph glyph, int startIndex, int count) {

        // If this is a single point on it's own, we can't do anything with it
        if (glyph.getPoint(startIndex).endOfContour) {
            return;
        }

        int offset = 0;

        while (offset < count) {
            Point p0  = glyph.getPoint(startIndex + offset%count);
            Point p1  = glyph.getPoint(startIndex + (offset+1)%count);

            if (offset == 0) {
                shape.movePenTo(p0.x, p0.y);
                if (startIndex == 0) {
                    StyleChangeRecord scr = new StyleChangeRecord();
                    scr.setFlags(StyleChangeRecord.FILLSTYLE1 | 
                                 StyleChangeRecord.LINESTYLE);
                    scr.setFillStyle1(1);
                    scr.setLineStyle(0);
                    shape.getShapeRecords().addStyleChangeRecord(scr);
                }
            }

            if (p0.onCurve) {
                if (p1.onCurve) {
                    shape.drawLineTo(p1.x, p1.y);
                    offset++;
                } else {
                    Point p2;
                    p2 = glyph.getPoint(startIndex + (offset+2)%count);

                    if (p2.onCurve) {
                        shape.drawCurveTo(p1.x, p1.y, p2.x, p2.y);
                    } else {
                        shape.drawCurveTo(p1.x, p1.y,
                                  midValue(p1.x, p2.x), 
                                  midValue(p1.y, p2.y));
                    }
                    offset+=2;
                } 
            } else {
                if (!p1.onCurve) {
                    shape.drawCurveTo(p0.x, p0.y,
                                      midValue(p0.x, p1.x),
                                      midValue(p0.y, p1.y));
                } else {
                    shape.drawCurveTo(p0.x, p0.y, p1.x, p1.y);
                }
                offset++;
            }
        }
    }

    /**
     * @return midpoint of (a,b)
     * @param a
     * @param b
     */
    private static int midValue(int a, int b) {
        return (a + b) / 2;
    }
}
