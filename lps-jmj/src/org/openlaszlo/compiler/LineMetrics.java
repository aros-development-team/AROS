/*****************************************************************************
 * LineMetrics.java
 * ****************************************************************************/

/* J_LZ_COPYRIGHT_BEGIN *******************************************************
* Copyright 2001-2004 Laszlo Systems, Inc.  All Rights Reserved.              *
* Use is subject to license terms.                                            *
* J_LZ_COPYRIGHT_END *********************************************************/

package org.openlaszlo.compiler;
import java.io.*;
import java.util.*;
import org.openlaszlo.xml.internal.XMLUtils;
import org.apache.log4j.*;


/** Utility functions for measuring HTML content, and translating it into Flash strings.
 *
 * @author <a href="mailto:hminsky@laszlosystems.com">Henry Minsky</a>
 */


/** Used to hold line widths while calculating element text metrics */
class LineMetrics {
    double maxw = 0.0;
    double w = 0.0;
    boolean verbatim = false;
    int nlines = 1;

    int last_space_pos;
    int last_newline_pos;
    double last_spacewidth = 0;

    int trailing_newlines = 0;

    /* Cases where we need to normalize whitespace across element boundaries

    foo <i>bar</i>
    foo <i> bar</i>
    <i>foo</i> bar
    <i>foo </i>bar
    <i>foo </i> bar
    <i>foo</i> <b>bar</b>
    <i>foo</i> <b> bar</b>
    <i>foo </i> <b> bar</b>
    <i>foo </i><b> bar</b>
    */
    /** Keep swallowing whitespace until the first non-whitespace character is
    encountered.
    */
    boolean trim = true;

    StringBuffer buf = new StringBuffer();

    public String toString() {
        return "LineMetrics: maxw="+maxw+" nlines="+nlines+" verbatim="+verbatim+ " str=|"+buf.toString()+"|";
    }

    /* Add a run of HTML, normalizing runs of whitespace to single
       spaces if required.  Flash's HTML text areas are sensitive to whitespace,
       so we need to present exactly the amount of whitespace that is desired.

       The trick is to figure out when to insert whitespace,
       especially as whitespace is carried across element boundaries.
       We keep one bit of state across Elements:

        trim:
          Tells the next element that it should
          discard any leading whitespace, because either this is the
          start of a line, or because we have inserted a trailing
          whitespace already.

        
    */

    void addHTML (String rawtext, String normalized_text, FontInfo fontInfo, SWFWriter generator) {
        if (rawtext.length() == 0) {
            return;
        }
        boolean leading_whitespace = Character.isWhitespace(rawtext.charAt(0));
        boolean trailing_whitespace = Character.isWhitespace(rawtext.charAt(rawtext.length()-1));
        boolean all_whitespace = (normalized_text.length() == 0);

        if (all_whitespace) {
            if (!this.trim && (leading_whitespace || trailing_whitespace)) {
                normalized_text = " ";
            } else {
                normalized_text = "";
            }
            this.trim = true;
        } else {
            if (!this.trim && leading_whitespace) {
                normalized_text = " " + normalized_text;
            }
            if (trailing_whitespace) {
                normalized_text = normalized_text + " ";
            }
            this.trim = trailing_whitespace;
        }

        addSpan(normalized_text, fontInfo, generator);
    }

    void setVerbatim (boolean val) {
        this.verbatim = val;
        resetLineWidth();
    }

    /** Add a run of text to the current text block, tracking the max width
     and accumulating the text into a buffer.  */
    void addSpan (String str, FontInfo fontInfo, SWFWriter generator) {
        if (str.length() > 0) {
            if (generator != null) {
                double sw = TextCompiler.getTextWidth(str, fontInfo, generator, this);
                w += sw;
            }
            str = XMLUtils.escapeXml(str);

            // Remember the position of the last space char on the
            // line, in case we need to trim it later.
            if (!verbatim) {
                int buflen = buf.length();
                char lastchar = str.charAt(str.length()-1);
                if (lastchar == ' ') {
                    last_space_pos = buflen + str.length() -1;
                } else {
                    last_space_pos = -1;
                }

                int newline_pos = str.lastIndexOf('\n');
                if (newline_pos >= 0) {
                    last_newline_pos = newline_pos + buflen;
                }
            }
            buf.append(str);
        }
    }

    /** Add an HTML command, does not affect textwidth */
    void addFormat (String str) {
        buf.append(str);
    }

    void addStartTag (String tagName, FontInfo fontInfo, SWFWriter generator) {
        addFormat("<" + tagName);
    }

    void endStartTag () {
        addFormat(">");
    }

    void addEndTag (String tagName) {
        addFormat("</" + tagName + ">");
    }

    /* @return the normalized text string */
    String getText () {
        return buf.toString();
    }

    /* Compute maxwidth of this line and any previous lines. */
    void endOfLine() {
        // Trim trailing whitespace at end of lines
        if (!verbatim) {
            if (last_space_pos > 0 && last_space_pos > last_newline_pos) {
                buf.deleteCharAt(last_space_pos);
                w -= last_spacewidth;
            }
        }
        maxw = Math.max(maxw, w);
        w = 0.0;
        last_space_pos = -1;
    }

    /** act as if a linebreak had occurred, for purposes of calculating max text width
     */
     void resetLineWidth() {
         maxw = Math.max(maxw, w);
         w = 0.0;         
         last_space_pos = -1;
     }

    /** End the line and add an HTML linebreak command element to the output buffer. */
    void newline() {
        nlines++;
        endOfLine();
        trim = true;
        buf.append("\n");
    }

    /** Ensures that at least two newline chars are present at end of
         buffer. */
    void paragraphBreak () {
        // Count how many trailing newlines at end of output string
        trailing_newlines = 0;
        for (int i = buf.length()-1; i >= 0; i--) {
            char c = buf.charAt(i);
            if (c == '\t' || c == ' ') continue;
            if (c == '\n') {
                trailing_newlines++;
            } else {
                break;
            }
        }
        if (trailing_newlines == 0) {
            buf.append("\n\n");
        } else if (trailing_newlines == 1) {
            buf.append("\n");
        } else {
            // don't need any more trailing newlines
        }
    }

}
