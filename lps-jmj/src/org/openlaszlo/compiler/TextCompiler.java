/*****************************************************************************
 * TextCompiler.java
 * ****************************************************************************/

/* J_LZ_COPYRIGHT_BEGIN *******************************************************
* Copyright 2001-2004 Laszlo Systems, Inc.  All Rights Reserved.              *
* Use is subject to license terms.                                            *
* J_LZ_COPYRIGHT_END *********************************************************/

package org.openlaszlo.compiler;
import org.openlaszlo.utils.ChainedException;
import java.io.*;
import java.util.*;
import org.jdom.Attribute;
import org.jdom.Comment;
import org.jdom.CDATA;
import org.jdom.Document;
import org.jdom.Element;
import org.jdom.EntityRef;
import org.jdom.Text;
import org.jdom.output.XMLOutputter;
import java.util.Iterator;
import org.apache.log4j.*;
import org.openlaszlo.iv.flash.api.text.Font;
import java.awt.geom.Rectangle2D;

/** Utility functions for measuring HTML content, and translating it into Flash strings.
 *
 * @author <a href="mailto:hminsky@laszlosystems.com">Henry Minsky</a>
 */
abstract class TextCompiler {

    private static Logger mLogger  = Logger.getLogger(TextCompiler.class);
    private static Logger mTextLogger = Logger.getLogger("lps.text");

    public static double computeTextWidth(String text, FontInfo fontInfo, SWFWriter generator)
      throws CompilationError {
        LineMetrics lm = new LineMetrics();
        return computeTextWidth(text, fontInfo, generator, lm);
    }

    /** Check if a specified font is known by the Font Manager
     *
     * @param manager a font manager
     * @param fontInfo the font spec you want to check
     *
     * This will throw an informative CompilationError if the font does not exist.
     */
     public static  void checkFontExists (SWFWriter generator, FontInfo fontInfo) {
        String fontName = fontInfo.getName();
        int    size     = fontInfo.getSize();
        int    style    = fontInfo.styleBits;

        if (!generator.checkFontExists(fontInfo)) {
            throw new CompilationError(
                "Can't find font "
                + fontName
                + " of style " 
                + fontInfo.getStyle());
        }
    }

    /**
     * Compute text width for a given font 
     *
     * @param text text stringtext string
     * @param fontInfo font info for this text
     * @return text width in pixels
     */
    public static double computeTextWidth(String text, FontInfo fontInfo, SWFWriter generator, LineMetrics lm)
        throws CompilationError {

        boolean trace = false; //mProperties.getProperty("trace.fonts", "false") == "true";

        String fontName = fontInfo.getName();
        int    size     = fontInfo.getSize();
        int    style    = fontInfo.styleBits;

        mTextLogger.debug("computeTextWidth fontName " + fontName + 
          " (style: " + fontInfo.getStyle() + 
          ", size: " + fontInfo.getSize() + 
          ") text: " + text);

        if (text.length() == 0) {
            return 0;
        }

        generator.checkFontExists(fontInfo);

        FontFamily family = generator.getFontManager().getFontFamily(fontName);
        if (family == null) {
            throw new CompilationError("Can't find font " + fontName);
        }

        Font font = family.getStyle(style);
        if (font == null) {
            throw new CompilationError(
                                       "Can't measure text because font "
                                       + FontInfo.styleBitsToString(style) + " "
                                       + fontName
                                       + " is missing.");
        }

        Rectangle2D[] bounds = family.getBounds(style);

        if (bounds == null) {
            throw new CompilationError(
                                       "Can't measure text because font "
                                       + FontInfo.styleBitsToString(style) + " "
                                       + fontName
                                       + " is missing its bounds array.");
        }

        double width = 0;
        int length = text.length();
        char c = text.charAt(0);
        int idx = font.getIndex(c);
        int nextIdx;

        double last_charwidth = 0;

        // Cope with \n \r and missing characters? XXX
        for(int i = 0; i < length; i++) {
            if (idx == -1) {
                mLogger.warn("Character \'" + c + 
                             "\' (" + (int)c + ") not available in font " + fontName +
                             " (style " +
                             fontInfo.getStyle() + ")");
                continue;
            } else {
                double adv = font.getAdvanceValue(idx);

                if (i == length - 1) {
                    double m = 0;
                    try {
                        m = bounds[idx].getMaxX(); 
                    } catch (Exception e) {
                    }
                    if (m > adv) {
                        adv = m;
                    }
                } 

                if (i == 0) {
                    try {
                        double m = bounds[idx].getMinX();
                        if (m > 0) {
                            adv += m;
                        } 
                    } catch (Exception e) {
                    }
                } 

                last_charwidth = adv;
                width += adv;

                mLogger.debug("adv " + adv);
            }

            if (i != length - 1) {
                c = text.charAt(i+1);
                nextIdx = font.getIndex(c);
                if (nextIdx != -1) {
                    double cw = font.getKerning(idx, nextIdx);
                    width += cw;
                }
                idx = nextIdx;
            }
        }
        // Width in pixels
        double w = (double)(width * fontInfo.getSize()) / 1024.0;

        // If the last character was a space, remember it's width, as we may need
        // to trim the trailing space from the HTML formatted text
        if (c == ' ') {
            lm.last_spacewidth = (double)(last_charwidth  * fontInfo.getSize()) / 1024.0;
        } 

        mTextLogger.debug("computeTextWidth: " + text + 
          " (font: " + fontInfo.getName() + 
          ", size: " + fontInfo.getSize() + 
          ", style: " + fontInfo.getStyle() +
          ") has textwidth: " + w);

        // FIXME: [2003-09-26 bloch] handle empty string case? should it be w/out slop?
        // Match this in LzNewText.as
        //
        final int SLOP = 2;

        return w + SLOP;
    }

    /** Compute the text width of a string. If there are multiple
     * lines, return the maximum line width.
     *
     * <p>
     *
     * The only multi-line strings we will ever see here will be
     * non-normalized text such as inside &ltpre;&gt; verbatim
     * regions, because in normal running HTML text, the normalization
     * will have stripped out newlines.
     *
     * <p>
     *
     * The LineMetrics holds state from possibly a previous text run
     * on the same line, telling us whether we need to prepend an
     * extra whitespace.
     */
    static double getTextWidth(String str, FontInfo fontInfo, SWFWriter generator,
                               LineMetrics lm) {

        double maxw = 0;
        int lastpos = 0;
        int nextpos = str.indexOf('\n');
        String substr;

        if (nextpos < 0) {
            return computeTextWidth(str, fontInfo, generator, lm);
        }
        while (nextpos >= 0) {
            substr = str.substring(lastpos, nextpos);
            maxw = Math.max(maxw, computeTextWidth(substr, fontInfo, generator, lm));
            lastpos = nextpos+1;
            nextpos = str.indexOf('\n', lastpos);
            lm.nlines++;
        }

        substr = str.substring(lastpos);
        maxw = Math.max(maxw, computeTextWidth(substr, fontInfo, generator, lm));
        return maxw;
    }

    /** Measure the content text allowing for "HTML" markup.
     *
     * This uses rules similar to how you would measure browser HTML text:
     *
     * <ul>
     * <li> All text is whitespace normalized, except that which occurs between &lt;pre&gt; tags
     * <li> Linebreaks occur only when &lt;br/&gt; or &lt;p/&gt; elements occur, or when a newline
     * is present inside of a &lt;pre&gt; region.
     * <li> When multiple text lines are present, the length of the longest line is returned.
     * </ul>
     */

    static LineMetrics getElementWidth(Element e, FontInfo fontInfo, SWFWriter generator) {
        LineMetrics lm = new LineMetrics();
        getElementWidth(e, fontInfo, generator, lm);
        lm.endOfLine();
        // cache the normalized HTML content
        ((ElementWithLocationInfo) e).setHTMLContent(lm.getText());
        return lm;
    }

    /** Gets the text content, with HTML normalization rules applied */
    static String getHTMLContent(Element e) {
        // check if the normalized text is cached
        if ((e instanceof ElementWithLocationInfo) &&
            ((ElementWithLocationInfo) e).getHTMLContent() != null) {
            return ((ElementWithLocationInfo) e).getHTMLContent();
        }

        LineMetrics lm = new LineMetrics();
        // Just use a dummy font info, we only care about the HTML
        // text, not string widths
        FontInfo fontInfo = new FontInfo("default", "8", "");
        getElementWidth(e, fontInfo, null, lm);
        lm.endOfLine();
        return lm.getText();
    }

    /** Return text suitable for passing to Laszlo inputtext component.
     * This means currently no HTML tags are supported except PRE
     */
    static String getInputText (Element e) {
        String text = "";
        for (Iterator iter = e.getContent().iterator();
             iter.hasNext();) {
            Object node = iter.next();
            if (node instanceof Element) {
                Element child = (Element) node;
                String tagName = child.getName();
                if (tagName.equals("p") || tagName.equals("br")) {
                    text += "\n";
                } else if (tagName.equals("pre")) {
                    text +=  child.getText();
                } else {
                    // ignore everything else
                }
            } else if ((node instanceof Text) || (node instanceof CDATA)) {
                if (node instanceof Text) {
                    text += ((Text) node).getTextNormalize();
                } else {
                    text += ((CDATA) node).getTextNormalize();
                }
            }
        }
        return text;
    }


    /** 
        Processes the text content of the element.  The element
        content may contain XHTML markup elements, which we will
        interpret as we map over the content. Normally, whitespace
        will be normalized away. However, preformat &lt;pre&gt; tags
        will cause the enclosed text to be treated as verbatim,
        meaning means that whitespace and linebreaks will be
        preserved.
    
        Supported XHTML markup is currently:
        <ul>
        <li> P, BR cause linebreaks
        <li> PRE sets verbatim (literal whitespace) mode
        <li> font face control: B, I, FONT tags modify the font
        <li> A [href] indicates a hyperlink
        </ul>

    */
    static void getElementWidth(Element e, FontInfo fontInfo, SWFWriter generator,
                                LineMetrics lm) {
        for (Iterator iter = e.getContent().iterator();
             iter.hasNext();) {
            Object node = iter.next();
            if (node instanceof Element) {
                Element child = (Element) node;
                String tagName = child.getName();

                if (tagName.equals("br")) {
                    lm.newline(); // explicit linebreak
                    getElementWidth(child, fontInfo, generator, lm);
                    if (!child.getText().equals("")) {
                        lm.newline();
                    }
                } else if (tagName.equals("p")) {
                    lm.paragraphBreak();
                    getElementWidth(child, fontInfo, generator, lm);
                    lm.paragraphBreak();
                } else if (tagName.equals("pre")) {
                    boolean prev = lm.verbatim;
                    lm.setVerbatim(true);
                    getElementWidth(child, fontInfo, generator, lm);
                    lm.setVerbatim(prev);
                } else if (ViewSchema.isHTMLElement(child)) {
                    FontInfo newInfo = new FontInfo(fontInfo);
                    if (tagName.equals("b")) {
                        newInfo.styleBits |= FontInfo.BOLD;
                    } else if (tagName.equals("i")) {
                        newInfo.styleBits |= FontInfo.ITALIC;
                    } else if (tagName.equals("font")) {
                        ViewCompiler.setFontInfo(newInfo, child);
                    }
                    lm.addStartTag(tagName, newInfo, generator);
                    // print font-related attributes:
                    // face, size, color
                    // supported Flash HTML tags: http://www.macromedia.com/support/flash/ts/documents/htmltext.htm
                    for (Iterator attrs = child.getAttributes().iterator(); attrs.hasNext(); ) {
                        Attribute attr = (Attribute) attrs.next();
                        String name = attr.getName();
                        String value = child.getAttributeValue(name);
                        // TBD: [hqm nov-15-2002] The value ought to be quoted in case it contains double quotes
                        // (but no values of currently supported HTML tags will contain double quotes)
                        lm.addFormat(" "+name+"=\""+value+"\"");
                    }
                    lm.endStartTag();
                    getElementWidth(child, newInfo, generator, lm);
                    lm.addEndTag(tagName);
                }
            } else if ((node instanceof Text) || (node instanceof CDATA)) {
                String rawtext;
                if (node instanceof Text) {
                    rawtext = ((Text) node).getText();
                } else {
                    rawtext = ((CDATA) node).getText();
                }
                if (lm.verbatim) {
                    lm.addSpan(rawtext, fontInfo, generator);
                } else {
                    // Apply HTML normalization rules to the text content.
                    if (rawtext.length() > 0) {
                        // getTextNormalize turns an all-whitespace string into an empty string
                        String normalized_text;
                        if (node instanceof Text) {
                            normalized_text = ((Text) node).getTextNormalize();
                        } else {
                            normalized_text = ((CDATA) node).getTextNormalize();
                        }
                        lm.addHTML (rawtext, normalized_text, fontInfo, generator);
                    }
                }
            } else if (node instanceof EntityRef) {
                // EntityRefs don't seem to occur in our JDOM, they were all resolved
                // to strings by the parser already
                throw new RuntimeException("encountered unexpected EntityRef node in getElementWidth()");
            } 
        }
    }
}
