/******************************************************************************
 * TTFGlyph.java
 * ****************************************************************************/

/* J_LZ_COPYRIGHT_BEGIN *******************************************************
* Copyright 2001-2004 Laszlo Systems, Inc.  All Rights Reserved.              *
* Use is subject to license terms.                                            *
* J_LZ_COPYRIGHT_END *********************************************************/

package org.openlaszlo.media;

import org.apache.batik.svggen.font.table.GlyphDescription;
import org.apache.batik.svggen.font.table.GlyfDescript;
import org.apache.batik.svggen.font.Point;

// Logger
import org.apache.log4j.*;

/**
 * TrueType Glyph utility class
 */
public class TTFGlyph {

    private Point[] points;

    /**
     * Constructs a TTFGlyph from a GlyfDescription
     * @param gd a glyph description
     */
    public TTFGlyph(GlyphDescription gd) {
        int endPtIndex = 0;
        points = new Point[gd.getPointCount()];
        for (int i = 0; i < gd.getPointCount(); i++) {
            boolean endPt = gd.getEndPtOfContours(endPtIndex) == i;
            if (endPt) {
                endPtIndex++;
            }
            points[i] = new Point(
                    gd.getXCoordinate(i),
                    gd.getYCoordinate(i),
                    (gd.getFlags(i) & GlyfDescript.onCurve) != 0,
                    endPt);
        }
    }

    /**
     * @return the requested point from the glyph
     */
    public Point getPoint(int i) {
        return points[i];
    }

    /**
     * @return the number of points in the glyph
     */
    public int getNumPoints() {
        return points.length;
    }

    /**
     * Dump the glyph to the given logger's debug output
     * @param logger 
     */
    public void dump(Logger logger) {
        for (int i = 0; i < points.length; i++) {
            logger.debug( "Point x " + points[i].x + 
                           " y " + points[i].y + 
                           " " + points[i].onCurve + 
                           " " + points[i].endOfContour);
        }
    }

    /**
     * Scale the glyph and negate the Y axis.
     * @param factor scale factor
     */
    public void scale(double factor) {
        for (int i = 0; i < points.length; i++) {
            points[i].x *= factor;
            points[i].y *= -factor;
        }
    }
}
