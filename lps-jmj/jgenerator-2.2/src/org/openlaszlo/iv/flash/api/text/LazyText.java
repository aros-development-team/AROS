/*
 * $Id: LazyText.java,v 1.7 2002/07/17 05:13:37 skavish Exp $
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

import java.io.*;
import java.util.*;
import java.awt.geom.*;

import org.openlaszlo.iv.flash.parser.*;
import org.openlaszlo.iv.flash.util.*;
import org.openlaszlo.iv.flash.api.*;
import org.openlaszlo.iv.flash.context.*;

/**
 * This class represents DefineText tag.
 * <P>
 * Basically it is alredy laid out text.
 *
 * @author Dmitry Skavish
 * @see Text
 */
public final class LazyText extends FlashDef implements TextBlock {

    public static final int HAS_FONT    = 0x08;
    public static final int HAS_COLOR   = 0x04;
    public static final int HAS_YOFFSET = 0x02;
    public static final int HAS_XOFFSET = 0x01;

    public boolean withAlpha;                   // true if color with alpha
    public Rectangle2D bounds;                  // bounds of the text
    public AffineTransform matrix;              // matrix of the text
    public IVVector records = new IVVector();   // vector of TextRecord's and TextStyleChangeRecord's
    private Text text;                          // reconstructed text

    public LazyText() {}

    public LazyText( boolean withAlpha )    {
        this.withAlpha = withAlpha;
    }

    public Rectangle2D getBounds() {
        if( text != null ) return text.getBounds();
        return bounds;
    }

    public void setBounds( Rectangle2D bounds ) {
        this.bounds = bounds;
    }

    /**
     * Returns vector of all text records of this text.
     *
     * @return vector of {@link TextRecord} or/and {@link TextStyleChangeRecord}
     */
    public IVVector getAllTextRecords1() {
        if( text != null ) return text.getAllTextRecords();
        return records;
    }

    /**
     * Sets new vector of text records for this text.
     *
     * @param records  new vector of {@link TextItem} or/and {@link TextStyleChangeRecord}
     */
    public void setAllTextRecords( IVVector records ) {
        this.records = records;
    }

    /**
     * Adds new text record to this text
     *
     * @param record   new text record to be added
     */
    public void addTextRecord( TextRecord record ) {
        records.addElement(record);
    }

    /**
     * Adds new text style change record to this text
     *
     * @param record   new text style change record to be added
     */
    public void addTextStyleChangeRecord( TextStyleChangeRecord record ) {
        records.addElement(record);
    }

    public void setMatrix( AffineTransform matrix ) {
        this.matrix = matrix;
    }

    public AffineTransform getMatrix() {
        if( text != null ) return text.getMatrix();
        return matrix;
    }

    public int getTag() {
        if( withAlpha ) return Tag.DEFINETEXT2;
        return Tag.DEFINETEXT;
    }

    public static LazyText parse( Parser p, boolean withAlpha ) {
        LazyText text = new LazyText( withAlpha );
        text._parse( p );
        return text;
    }

    public void _parse( Parser p ) {
        // get id
        setID( p.getUWord() );
        // get bounds and matrix
        bounds = p.getRect();
        matrix = p.getMatrix();

        // get nBits
        int nGlyphBits = p.getUByte();
        int nAdvanceBits = p.getUByte();

        boolean is_swt = p.getFile().isTemplate();
        boolean is_const = true;
        //System.out.println("LazyText.parse: nGlyphBits="+nGlyphBits+", nAdvanceBits="+nAdvanceBits);

        // get textrecords
        Font curFont = null;
        boolean isData = false;
        for(;;) {
            int flags = p.getUByte();
            if( flags == 0 ) {
                break;
            }
            //System.out.println("LazyText.parse: flags="+Util.b2h(flags));
            //if( (flags&0x80) != 0 ) {
            if( isData || (flags&0x80) == 0 ) {
                TextRecord tr = new TextRecord( flags );    // flags contains number of glyphs
                records.addElement(tr);
                // glyph record
                p.initBits();
                for( int i=0; i<flags; i++ ) {
                    //System.out.println("LazyText.parse: i="+i);
                    int idx = p.getBits(nGlyphBits);
                    int adv = p.getSBits(nAdvanceBits);
                    char ch = (char) curFont.codeTable[idx];   // codetable has to be already setup
                    //System.out.println("LazyText.parse: idx="+idx+", adv="+adv+", ch="+ch);
                    tr.add(ch, idx, adv);
                }
                if( !is_swt && is_const ) {
                    for( int i=0; i<flags; i++ ) {
                        if( tr.getChar(i) == '{' && (i+1>=flags || tr.getChar(i+1)!='{') ) {
                            is_const = false;
                            break;
                        }
                    }
                }
                //tr.printContent(System.out, "trrecord: ");
            } else {
                // control record
                TextStyleChangeRecord ts = new TextStyleChangeRecord();
                records.addElement(ts);
                if( (flags&HAS_FONT) != 0 ) {
                    // get font definition
                    curFont = ((FontDef)p.getDef(p.getUWord())).getFont();
                    ts.setFont( curFont );
                }
                if( (flags&HAS_COLOR) != 0 ) {
                    // get color
                    ts.setColor( Color.parse(p, withAlpha) );
                }
                if( (flags&HAS_XOFFSET) != 0 ) {
                    ts.setX( p.getWord() );
                }
                if( (flags&HAS_YOFFSET) != 0 ) {
                    ts.setY( p.getWord() );
                }
                if( (flags&HAS_FONT) != 0 ) {
                    ts.setHeight( p.getUWord() );
                }
                //ts.printContent(System.out, "StyleChange: ");
            }
            isData = !isData;
        }
        setConstant(is_const);
    }

    public void write( FlashOutput fob ) {
        if( text != null ) {
            text.write(fob, this);
        } else {
            int start = fob.getPos();     // save for tag
            fob.skip( 6 );    // 6 - long tag
            fob.writeDefID( this );
            fob.write(bounds);
            fob.write(matrix);

            // update indexes from font and calculate maximum index and advance value
            int maxIdx = Integer.MIN_VALUE;
            int maxAdv = Integer.MIN_VALUE;
            Font lastFont = null;
            for( int i=0; i<records.size(); i++ ) {
                Object o = records.elementAt(i);
                if( o instanceof TextStyleChangeRecord ) {
                    Font f = ((TextStyleChangeRecord)o).getFont();
                    if( f != null ) lastFont = f;
                } else {
                    TextRecord tr = (TextRecord) o;
                    int idx = tr.getMaxIndex();
                    int adv = tr.getMaxAdvance();
                    if( idx > maxIdx ) maxIdx = idx;
                    if( adv > maxAdv ) maxAdv = adv;
                }

            }
            int nGlyphBits = Util.getMinBitsU(maxIdx);
            int nAdvanceBits = Util.getMinBitsS(maxAdv);

            fob.writeByte(nGlyphBits);
            fob.writeByte(nAdvanceBits);

            fob.setUserData( new int[] {nGlyphBits, nAdvanceBits} );
            records.write( fob );

            fob.writeByte(0);

            int size = fob.getPos()-start-6;  // 6 - long tag
            fob.writeLongTagAt( getTag(), size, start );
        }
    }

    public void printContent( PrintStream out, String indent ) {
        out.println( indent+"LazyText: id="+getID() );
        out.println( indent+"    "+bounds );
        out.println( indent+"    "+matrix );
        records.printContent( out, indent+"    " );
    }

    // ------------------------------------------------------- //
    //                 TextBlock implementation                //
    /**
     * Layouts this text.
     * <p>
     * Does nothing, because the text is already laid out.
     */
    public void layout() {
        if( text != null ) text.layout();
    }

    /**
     * Returns vector of {@link TextRecord}s from this text
     * of specified font.
     *
     * @param font   font of text records to be returned
     * @return text records of specified font
     */
    public IVVector getTextRecords( Font font ) {
        if( text != null ) return text.getTextRecords(font);
        IVVector trs = new IVVector();
        Font lastFont = null;
        for( int k=0; k<records.size(); k++ ) {
            Object o = records.elementAt(k);
            if( o instanceof TextStyleChangeRecord ) {
                Font f = ((TextStyleChangeRecord)o).getFont();
                if( f != null ) lastFont = f;
            } else {
                if( lastFont == font ) trs.addElement(o);
            }
        }
        return trs;
    }

    /**
     * Updates records' font.
     * <P>
     * Changes one specified font into another in all records.
     * In text records also updates indexes.
     *
     * @param old_font old font
     * @param new_font new font
     */
    public void changeFont( Font old_font, Font new_font ) {
        if( text != null ) {
            text.changeFont(old_font, new_font);
        } else {
            FontDef.changeRecordsFont(records, old_font, new_font);
        }
    }

    // ------------------------------------------------------- //

    public void collectDeps( DepsCollector dc ) {
        if( text != null ) {
            text.collectDeps(dc);
        } else {
            for( int i=0; i<records.size(); i++ ) {
                FlashItem t = (FlashItem) records.elementAt(i);
                if( t instanceof TextStyleChangeRecord ) {
                    TextStyleChangeRecord ts = (TextStyleChangeRecord) t;
                    if( ts.getFont() != null ) dc.addDep(ts.getFont());
                }
            }
        }
    }

    public void collectFonts( FontsCollector fc ) {
        if( text != null ) {
            text.collectFonts(fc);
        } else {
            for( int i=0; i<records.size(); i++ ) {
                FlashItem t = (FlashItem) records.elementAt(i);
                if( t instanceof TextStyleChangeRecord ) {
                    TextStyleChangeRecord ts = (TextStyleChangeRecord) t;
                    if( ts.getFont() != null ) fc.addFont( ts.getFont(), this );
                }
            }
        }
    }

    /**
     * if x == 0 && prevX was at the end then append
     * if x == 0 && prevX was NOT at the end then new item
     * if x != 0 then check if rest of x != 0 then if they are equal it's 'center'
     * @return
     */
    public Text createText() {
        Font font = null;
        Color color = null;
        int lastX = -1, lastY = 0;
        int height = 0;
        int last_rem = 0;   // remainder in last line
        char end_ch = '\0'; // last char in line

        int max_ascent = 0;
        int max_descent = 0;
        int first_not_updated = 0;

        boolean newItem = false;
        IVVector items = new IVVector();
        TextItem curItem = null;
        for( int i=0; i<records.size(); ) {
            Object o = records.elementAt(i++);
            if( !(o instanceof TextStyleChangeRecord) ) return null;
            TextStyleChangeRecord tsr = (TextStyleChangeRecord) o;
            o = records.elementAt(i++);
            if( !(o instanceof TextRecord) ) return null;
            TextRecord tr = (TextRecord) o;
            String str = new String(tr.getText(), 0, tr.getSize());

            //System.out.println("New record: '"+str+"'");
            // calculate first word size
            int first_word_size = 0;
            int k = 0;
            int max_adv = 0;
            for(; k<tr.getSize(); k++) if( tr.getChar(k) != ' ' ) break;
            for(; k<tr.getSize(); k++) {
                if( tr.getChar(k) == ' ' ) break;
                int adv = tr.getAdvance(k);
                if( adv > max_adv ) max_adv = adv;
                first_word_size += adv;
            }
            first_word_size += max_adv-1;
            //System.out.println("  first word size="+first_word_size);

            newItem = false;
            // get new font props
            if( tsr.getFont() != null ) {
                font = tsr.getFont();
                height = tsr.getHeight();
                newItem = true;
            }

            if( tsr.getColor() != null ) {
                color = tsr.getColor();
                if( !(color instanceof AlphaColor) ) {
                    color = new AlphaColor(color.getRGB());
                    ((AlphaColor)color).setAlpha(255);
                }
                newItem = true;
            }
            //System.out.println("  newItem="+newItem);

            if( newItem ) {
                if( curItem != null ) items.addElement(curItem);
                curItem = new TextItem("", font, height, color);
            }

            // check whether x and y have been changed
            int newX = tsr.getX();
            int newY = tsr.getY();
            boolean x_changed = newX == Integer.MAX_VALUE? false: newX != lastX;
            boolean y_changed = newY == Integer.MAX_VALUE? false: newY != lastY;
            //System.out.println("  x_changed="+x_changed+", newX="+newX);
            //System.out.println("  y_changed="+y_changed+", newY="+newY);

            // calculate linesp
            /*if( y_changed && i >= 2 ) {
                int delta = newY-lastY; // ascent+descent+linesp
                int linesp = delta-max_ascent-max_descent;
                for(; first_not_updated<items.size(); first_not_updated++ ) {
                    TextItem item = (TextItem) items.elementAt(first_not_updated);
                    item.linesp = linesp;
                }
                max_ascent = 0;
                max_descent = 0;
            }

            if( tsr.getFont() != null ) {
                int ascent = (font.ascent * height) / 1024;
                //int descent = (font.descent * height) / 1024 + curItem.linesp;
                int descent = (font.descent * height) / 1024;
                if( ascent > max_ascent ) max_ascent = ascent;
                if( descent > max_descent ) max_descent = descent;

            } */

            if( x_changed && y_changed ) {
                // next line
                //if( newX == 0 ) {
                    // normal new line
                    if( end_ch == ' ' && last_rem < first_word_size ) {
                        // add this text to current item
                        curItem.text += str;
                    } else {
                        // it's a newline
                        curItem.text += "\n"+str;
                    }
                //} else {
                    // aligned new line: most likely to right
                //}
            } else if( y_changed ) {
                // x was not changed, usually it's a first record
                // if it's a first record do not add newline
                if( i == 2 ) curItem.text += str;
                else {
                    curItem.text += "\n"+str;
                }
            } else if( x_changed ) {
                // y was not change, very strange -> do not properly handle
                curItem.text += str;
            } else {
                // no x nor y were changed, just add text
                curItem.text += str;
            }

            end_ch = str.length() == 0? '\0': str.charAt(str.length()-1);
            // calculate the end of this record
            if( x_changed ) lastX = newX;
            for( int j=0; j<tr.getSize(); j++ ) {
                lastX += tr.getAdvance(j);
            }
            last_rem = (int) bounds.getMaxX()-lastX;
            if( y_changed ) lastY = newY;
        }

        if( curItem != null ) items.addElement(curItem);

        Text text = new Text(withAlpha);
        text.setBounds(bounds);
        text.setGenBounds(bounds);
        text.setMatrix(matrix);
        text.setTextItems(items);
        //text.printContent(System.out, "");

        return text;
    }

    public void apply( Context context ) {
        if( isConstant() ) return;
        text = createText();
        if( text == null ) return;
        text.apply(context);
        //text.printContent(System.out, "apply: ");
    }

    protected boolean _isConstant() {
        return true;
    }

    protected FlashItem copyInto( FlashItem item, ScriptCopier copier ) {
        super.copyInto( item, copier );
        ((LazyText)item).withAlpha = withAlpha;
        ((LazyText)item).bounds = (Rectangle2D) bounds.clone();
        ((LazyText)item).matrix = (AffineTransform) matrix.clone();
        ((LazyText)item).records = records.getCopy(copier);
        ((LazyText)item).text = text==null? null: (Text) text.getCopy(copier);
        return item;
    }

    public FlashItem getCopy( ScriptCopier copier ) {
        return copyInto( new LazyText(), copier );
    }
}
