/*
 * $Id: TextLayout.java,v 1.5 2002/07/15 02:15:03 skavish Exp $
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
import java.awt.geom.*;

import org.openlaszlo.iv.flash.api.*;
import org.openlaszlo.iv.flash.api.text.*;

/**
 * This class does simple text layout in a box.
 * <P>
 *
 * @author Dmitry Skavish
 * @see org.openlaszlo.iv.flash.api.text.Text
 * @see org.openlaszlo.iv.flash.api.text.Font
 * @see org.openlaszlo.iv.flash.api.text.FontDef
 * @see org.openlaszlo.iv.flash.api.text.TextRecord
 * @see org.openlaszlo.iv.flash.api.text.TextStyleChangeRecord
 */
public final class TextLayout {

    private static final int MAX_CHARS_IN_LINE = 250;

    /**
     * One line of text.
     * <p>
     * Contains several style runs (text of different font and style)
     */
    public final class TextLine extends FlashItem {

        /**
         * Vector of TextRecords and TextStyleChangeRecords
         * <P>
         * TextStyleChangeRecords are at even positions,
         * TextRecords are at odd positions.
         */
        public IVVector records = new IVVector(4);

        private TextItem lastItem;
        private TextRecord lastRecord;

        public TextLine() {}

        /**
         * Creates new text record.
         */
        public void newRecord() {
            flushLastRecord();
            lastItem = curItem;
            lastRecord = new TextRecord( MAX_CHARS_IN_LINE );
        }

        private void flushLastRecord() {
            if( lastRecord != null && lastRecord.getSize() != 0 ) {
                int size = records.size();
                if( size%2 == 0 ) { // make sure we add stylechange only after textrecord
                    records.addElement( getStyleChange( lastItem ) );
                }
                records.addElement( lastRecord );
                lastRecord = null;
            }
        }

        /**
         * Creates text style change record from specified text item
         * and sets all the properties, like font, color and height as
         * current properties.
         *
         * @param item   item for which to create style change record
         * @return new style change record
         */
        private TextStyleChangeRecord getStyleChange( TextItem item ) {
            // create text style change record
            TextStyleChangeRecord ts = new TextStyleChangeRecord();

            if( item != null ) {
                ts.setFont( item.font );
                ts.setHeight( item.height );
                ts.setColor( item.color );
            }

            return ts;
        }

        /**
         * Adds new character to last text record.
         * <P>
         * If last text record is overflowed then creates new text record
         *
         * @param ch     character to be added
         * @param index  index of character in the font
         * @param adv    advance value of the character
         */
        public void add( char ch, int index, int adv ) {
            if( lastRecord.getSize() >= MAX_CHARS_IN_LINE ) {
                newRecord();
            }
            lastRecord.add( ch, index, adv );
        }

        /**
         * Trims this line from the end
         * <P>
         * Removes all spaces from the end
         *
         * @return width in twixels of all removed spaces
         */
        public int trimEnd() {
            flushLastRecord();

            int w = 0;

            for( int i=records.size(); --i>0; ) {
                TextRecord tr = (TextRecord) records.elementAt(i);
                w += tr.trimEnd();
                if( tr.getSize() > 0 ) break;
                records.removeElementAt(i);     // remove text record
                records.removeElementAt(--i);   // remove stylechange record
            }

            return w;
        }

        /**
         * Trims this line from the start
         * <P>
         * Removes all spaces from the start
         *
         * @return width in twixels of all removed spaces
         */
        public int trimStart() {
            flushLastRecord();

            int w = 0;

            TextStyleChangeRecord lastts = null;

            for(;records.size()>0;) {
                TextStyleChangeRecord ts = (TextStyleChangeRecord) records.elementAt(0);
                if( lastts != null ) {
                    lastts.mergeTo(ts);
                }
                lastts = ts;
                TextRecord tr = (TextRecord) records.elementAt(1);
                w += tr.trimStart();
                if( tr.getSize() > 0 ) break;
                records.removeElementAt(1);     // remove text record
                records.removeElementAt(0);     // remove stylechange record
            }

            return w;
        }

        /**
         * Mark current position in current text record.
         * <P>
         * Used for rolling back if word does not fit in the text rectangle.
         */
        public void markPosition() {
            markedPosition = lastRecord.getSize();
        }

        /**
         * Rollbacks current position to the marked one.
         */
        public void rollBack() {
            lastRecord.setSize( markedPosition );
        }

        /**
         * Does end-of-line.
         * <P>
         * Flushes current text record
         *
         * @param x      x position of this line
         * @param y      y position of this line
         */
        public void endLine( int x, int y ) {
            flushLastRecord();
            if( records.size() > 0 ) {
                TextStyleChangeRecord ts = (TextStyleChangeRecord) records.elementAt(0);
                ts.setX(x);
                ts.setY(y);
            }
        }

        /**
         * Returns maximum advance value from all the records.
         *
         * @return maximum advance value from all the records
         */
        public int getMaxAdvance() {
            int max = Integer.MIN_VALUE;
            // iterate over TextRecords only
            for( int i=1; i<records.size(); i+=2 ) {
                TextRecord tr = (TextRecord) records.elementAt(i);
                int adv = tr.getMaxAdvance();
                if( adv > max ) max = adv;
            }
            if( max == Integer.MIN_VALUE ) return 0;
            return max;
        }

        /**
         * Returns maximum character index from all the records.
         *
         * @return maximum character index from all the records
         */
        public int getMaxIndex() {
            int max = Integer.MIN_VALUE;
            // iterate over TextRecords only
            for( int i=1; i<records.size(); i+=2 ) {
                TextRecord tr = (TextRecord) records.elementAt(i);
                int idx = tr.getMaxIndex();
                if( idx > max ) max = idx;
            }
            if( max == Integer.MIN_VALUE ) return 0;
            return max;
        }

        /**
         * Returns width of this line.
         *
         * @return width if this line in twixels
         */
        public int getWidth() {
            if( records.size() == 0 ) return 0;
            TextStyleChangeRecord ts = (TextStyleChangeRecord) records.elementAt(0);
            int width = ts.getX();
            for( int i=1; i<records.size(); i+=2 ) {
                TextRecord tr = (TextRecord) records.elementAt(i);
                width += tr.getWidth();
            }
            return width;
        }

        /**
         * Return number of characters in this line
         *
         * @return number of characters in this line
         */
        public int getSize() {
            int size = 0;
            // iterate over TextRecords only
            for( int i=1; i<records.size(); i+=2 ) {
                TextRecord tr = (TextRecord) records.elementAt(i);
                size += tr.getSize();
            }
            if( lastRecord != null ) size += lastRecord.getSize();
            return size;
        }

        public void printContent( PrintStream out, String indent ) {
            out.println( indent+"TextLine: size="+records.size()+" x="+x+" y="+y );
            records.printContent(out, indent+"   ");
        }

        public void write( FlashOutput fob ) {
            records.write(fob);
        }

        protected FlashItem copyInto( FlashItem item, ScriptCopier copier ) {
            ((TextLine)item).records = records.getCopy(copier);
            // there is no need to copy lastItem and lastRecord (?)
            return item;
        }

        public FlashItem getCopy( ScriptCopier copier ) {
            return copyInto( new TextLine(), copier );
        }
    }

    private int markedPosition = 0; // used only by TextLine

    private TextItem curItem;
    private TextLine curLine;
    private boolean line_continued;
    private int line_width;
    private int line_window;
    private int x;
    private int y;
    private int max_ascent;
    private int max_descent;
    private int max_linesp;

    private int rect_width;
    private Text myText;
    private Rectangle2D bounds;
    private IVVector lines = new IVVector();

    private void reCalcHeights() {
        Font font = curItem.font;
        int height = curItem.height;
        max_ascent = (font.ascent * height) / 1024;
        max_linesp = curItem.linesp;
        max_descent = (font.descent * height) / 1024 + max_linesp;
    }

    private void endLine() {
        if( curLine != null ) {
            y += max_ascent;    // advance y to baseline
            curLine.endLine(x, y);
            y += max_descent;   // advance y to next line
            curLine = null;
        }
    }

    /**
     *
     * @param cont   true if this new line is continued from previous, this affects
     *               alignment of this line
     */
    private void newLine( boolean cont ) {
        endLine();
        reCalcHeights();
        curLine = new TextLine();
        line_continued = cont;
        lines.addElement( curLine );
        curLine.newRecord();
        line_width = 0;
        x = curItem.marginleft;
        line_window = rect_width-(curItem.marginleft+curItem.marginright);
    }

    private void newParagraph() {
        newLine( false );
        line_window -= curItem.indent;
        x += curItem.indent;
    }

    /**
     * Creates new text layout.
     *
     * @param myText text to be layed out
     * @param bounds rectangle to be used for laying the text out in
     */
    public TextLayout( Text myText, Rectangle2D bounds ) {
        this.myText = myText;
        this.bounds = bounds;
    }

    /**
     * Retrieves vector of text records layed out by this
     * text layout for the specified font.
     *
     * @param font   font of the records to be retrieved
     * @return vector of {@link org.openlaszlo.iv.flash.api.text.TextRecord}s
     */
    public IVVector getTextRecords( Font font ) {
        IVVector trs = new IVVector();
        Font lastFont = null;
        for( int i=0; i<lines.size(); i++ ) {
            TextLine line = (TextLine) lines.elementAt(i);
            for( int k=0; k<line.records.size(); k++ ) {
                Object o = line.records.elementAt(k);
                if( o instanceof TextStyleChangeRecord ) {
                    Font f = ((TextStyleChangeRecord)o).getFont();
                    if( f != null ) lastFont = f;
                } else {
                    if( lastFont == font ) trs.addElement(o);
                }
            }
        }
        return trs;
    }

    /**
     * Retrieves vector of all text records
     *
     * @return vector of {@link org.openlaszlo.iv.flash.api.text.TextRecord}s and {@link org.openlaszlo.iv.flash.api.text.TextStyleChangeRecord}s
     */
    public IVVector getAllTextRecords() {
        IVVector trs = new IVVector();
        for( int i=0; i<lines.size(); i++ ) {
            TextLine line = (TextLine) lines.elementAt(i);
            for( int k=0; k<line.records.size(); k++ ) {
                trs.addElement( line.records.elementAt(k) );
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
        for( int i=0; i<lines.size(); i++ ) {
            TextLine line = (TextLine) lines.elementAt(i);
            IVVector records = line.records;
            FontDef.changeRecordsFont(records, old_font, new_font);
        }
    }

    /**
     * Aligns current line
     */
    private void alignLine() {
        switch( curItem.align ) {
            case 0: // left
                if( curLine != null /*&& line_continued*/ ) {
                    line_width -= curLine.trimStart();
                }
                break;
            case 1: // right
                if( curLine != null ) {
                    line_width -= curLine.trimEnd();
                }
                x += line_window-line_width;
                break;
            case 2: // center
                if( curLine != null ) {
                    line_width -= curLine.trimEnd() + curLine.trimStart();
                }
                x += (line_window-line_width)/2;
                break;
            case 3: // justify
                // ........
                break;
        }
    }

    /**
     * Removes whitespace at the end of the text
     * <P>
     * Text is specified by vector of TextItems
     *
     * @param items  vector of TextItems
     * @return
     */
    private void trimEnd( IVVector items ) {
        for( int i=items.size(); --i>=0; ) {
            TextItem item = (TextItem) items.elementAt(i);
            String t = item.text;
            int j=t.length();
            for( ; --j>=0; ) {
                char ch = t.charAt(j);
                if( !Character.isWhitespace(ch) ) break;
            }
            if( j >= 0 ) {
                item.text = t.substring(0, j+1);
                return;
            } else {
                items.removeElementAt(i);
            }
        }
    }

    /**
     * Does text layout.
     */
    public void layout() {
        IVVector items = myText.getTextItems();
        trimEnd(items);
//        System.out.println( "layout of '"+((TextItem)items.elementAt(0)).text+"'" );

        rect_width = (int) bounds.getWidth();

        y = 0;

        if( items.size() > 0 ) {
            curItem = (TextItem) items.elementAt(0);

            newParagraph();

            int i=0;
            for(;;) {
                Font font = curItem.font;
                int height = curItem.height;
                int ascent = (font.ascent * height) / 1024;
                int descent = (font.descent * height) / 1024 + curItem.linesp;
                if( ascent > max_ascent ) max_ascent = ascent;
                if( descent > max_descent ) {
                    max_descent = descent;
                    max_linesp = curItem.linesp;
                }

                String text = curItem.text;
                boolean isNowWord = false;
                int word_width = 0;
                int start_word = 0;

                int text_len = text.length();
                for( int k=0; k<text_len; k++ ) {
                    char ch = text.charAt(k);
                    boolean isWord = isWord(ch);
                    if( !isNowWord && isWord  ) { // start new word
                        curLine.markPosition();
                        word_width = 0;
                        start_word = k;
                        isNowWord = true;
                    } else {
                        isNowWord = isWord;
                    }
                    if( ch == '\r' || ch == '\n' ) {
                        alignLine();
                        if( k == text_len-1 ) endLine();
                        else {
                            char ch1 = text.charAt(k+1);
                            if( ch != ch1 && (ch1 == '\r' || ch1 == '\n') ) k++;
                            if( k == text_len-1 ) endLine();
                            else newParagraph();
                        }
                    } else {
                        int idx = font.getIndex(ch);
                        int ch_adv = font.getAdvanceValue(idx);
                        if( k != text_len-1 ) ch_adv += font.getKerning(ch, text.charAt(k+1));
                        int adv = (ch_adv * height) / 1024;
                        adv += curItem.kerning;
                        line_width += adv;
                        word_width += adv;
                        if( line_width <= line_window ) {
                            curLine.add(ch,idx,adv);
                        } else {
                            if( curLine.getSize() == 0 ) { // even one character does not fit
                                // if even one character does not fit the window, then expand the window
                                line_window = line_width;
                                curLine.add(ch,idx,adv);
                                alignLine();
                                newLine( true );
                                isNowWord = false;
                            } else if( isNowWord ) {
                                if( word_width > line_window ) {
                                    // split anyway, because the word does not fit the window at all
                                    line_width -= adv;
                                    k--;
                                    alignLine();
                                    newLine( true );
                                } else {
                                    // rollback the word and send it to the next line
                                    line_width -= word_width;
                                    curLine.rollBack();
                                    k = start_word-1;
                                    alignLine();
                                    newLine( true );
                                    isNowWord = false;
                                }
                            } else {
                                line_width -= adv;
                                k--;
                                alignLine();
                                newLine( true );
                            }
                        }
                    }
                }

                i++;
                if( i >= items.size() ) break;
                curItem = (TextItem) items.elementAt(i);
                if( curLine == null ) newParagraph();
                else curLine.newRecord();
            }
            alignLine();
            endLine();

            y -= max_linesp;
        }

        // optimize text
        optimize();

        // calculate bounds
        int maxX = 0;
        for( int l=0; l<lines.size(); l++ ) {
            TextLine line = (TextLine) lines.elementAt(l);
            int max = line.getWidth();
            if( max > maxX ) maxX = max;
        }

        // check what kind of behavior is expected: MMGen or JGen and
        //        create appropriate bounds
        int bs = myText.getBoundsStyle();
        boolean isMMStyle = bs == Text.PROPERTY_CONTROLLED?
            PropertyManager.textMMStyle:
            bs == Text.MM_STYLE;
        if( isMMStyle ) {
            // set bounds to be exactly equal to the text (MMGen style)
            bounds.setFrame(bounds.getMinX(), bounds.getMinY(), maxX, y);
        } else {
            // set bounds to be equal whatever came from the template (JGen style)
            double width  = bounds.getWidth();
            double height = bounds.getHeight();
            if( maxX > width ) width = maxX;
            if( y > height ) height = y;
            bounds.setFrame(bounds.getMinX(), bounds.getMinY(), width, height);
        }

        myText.setBounds(bounds);
    }

    protected int getNGlyphBits() {
        int maxIdx = 0;
        for( int l=0; l<lines.size(); l++ ) {
            TextLine line = (TextLine) lines.elementAt(l);
            int max = line.getMaxIndex();
            if( max > maxIdx ) maxIdx = max;
        }

        return Util.getMinBitsU(maxIdx);
    }

    protected int getNAdvanceBits() {
        int maxAdv = 0;
        for( int l=0; l<lines.size(); l++ ) {
            TextLine line = (TextLine) lines.elementAt(l);
            int max = Math.abs(line.getMaxAdvance());
            if( max > maxAdv ) maxAdv = max;
        }

        return Util.getMinBitsS(maxAdv);
    }

    /**
     * Removes unneccesary data from stylechangerecords
     */
    public void optimize() {
        Font lastFont = null;
        int  lastHeight = -1;
        Color lastColor = null;

        for( int k=0; k<lines.size(); k++ ) {
            TextLine line = (TextLine) lines.elementAt(k);
            IVVector records = line.records;
            for( int i=0; i<records.size(); i+=2 ) {
                TextStyleChangeRecord ts = (TextStyleChangeRecord) records.elementAt(i);
                if( lastFont != null && lastFont == ts.getFont() ) {
                    if( lastHeight == ts.getHeight() ) {
                        ts.setFont(null);
                    } else {
                        lastHeight = ts.getHeight();
                    }
                } else {
                    lastFont = ts.getFont();
                    lastHeight = ts.getHeight();
                }
                if( lastColor != null && lastColor.equals(ts.getColor()) ) {
                    ts.setColor(null);
                } else {
                    lastColor = ts.getColor();
                }
            }
        }
    }

    public void write( FlashOutput fob ) {
        int nGlyphBits = getNGlyphBits();
        int nAdvanceBits = getNAdvanceBits();
        fob.writeByte(nGlyphBits);
        fob.writeByte(nAdvanceBits);
        //System.out.println("Layout write: nGlyphBits="+nGlyphBits+", nAdvanceBits="+nAdvanceBits);
        fob.setUserData( new int[] {nGlyphBits, nAdvanceBits} );

        for( int i=0; i<lines.size(); i++ ) {
            TextLine line = (TextLine) lines.elementAt(i);
            line.write(fob);
            //line.printContent(System.out, "Layout Write: ");
        }
        fob.writeByte(0);
    }

    public TextLayout getCopy( ScriptCopier copier ) {
        TextLayout tl = new TextLayout(myText,bounds);
        tl.lines = lines.getCopy(copier);
        return tl;
    }

    private static boolean isWord( char ch ) {
        //Text Flow by FX
        //Wrap words at whitespace and '-' only!

        if (ch == '-' )
          return false;

        return !Character.isWhitespace(ch);
    }
}
