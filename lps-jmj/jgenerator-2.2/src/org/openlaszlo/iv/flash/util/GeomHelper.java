/*
 * $Id: GeomHelper.java,v 1.4 2002/04/05 05:49:14 skavish Exp $
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

import org.openlaszlo.iv.flash.api.*;

import org.openlaszlo.iv.flash.cache.*;
import org.openlaszlo.iv.flash.url.*;

import java.awt.geom.Rectangle2D;
import java.awt.geom.Point2D;
import java.awt.geom.AffineTransform;
import java.io.*;
import java.util.*;

/**
 * Geometric helper: AffineTransform, Rectangle and related stuff.
 *
 * @author Dmitry Skavish
 */
public class GeomHelper {

    /**
     * Creates new rectangle
     *
     * @return empty rectangle (everything zero)
     */
    public static Rectangle2D newRectangle() {
        return new Rectangle2D.Double();
    }

    /**
     * Creates new rectangle
     *
     * @param x      x coordinate
     * @param y      y coordinate
     * @param w      width of the rectangle
     * @param h      height of the rectangle
     * @return new rectangle
     */
    public static Rectangle2D newRectangle( int x, int y, int w, int h ) {
        return new Rectangle2D.Double(x,y,w,h);
    }

    /**
     * Creates new rectangle
     *
     * @param x      x coordinate
     * @param y      y coordinate
     * @param w      width of the rectangle
     * @param h      height of the rectangle
     * @return new rectangle
     */
    public static Rectangle2D newRectangle( double x, double y, double w, double h ) {
        return new Rectangle2D.Double((int)x,(int)y,(int)w,(int)h);
    }

    /**
     * Transforms specified rectangle and return its bounds
     *
     * @param m      matrix which is used to transform the rectangle
     * @param src    rectangle which is transformed
     * @param dst    destination rectangle which is used to hold the bounds
     * @return destination rectangle
     */
    public static Rectangle2D calcBounds( AffineTransform m, Rectangle2D src, Rectangle2D dst ) {
        double x0 = src.getMinX();
        double y0 = src.getMinY();
        double x1 = src.getMaxX();
        double y1 = src.getMaxY();
        double[] a = new double[] {
            x0, y0,     // left,    top
            x1, y1,     // right,   bottom
            x0, y1,     // left,    bottom
            x1, y0      // right,   top
        };
        m.transform( a, 0, a, 0, 4 );
        x0 = Math.min( Math.min(a[0],a[2]), Math.min(a[4],a[6]) );
        x1 = Math.max( Math.max(a[0],a[2]), Math.max(a[4],a[6]) );
        y0 = Math.min( Math.min(a[1],a[3]), Math.min(a[5],a[7]) );
        y1 = Math.max( Math.max(a[1],a[3]), Math.max(a[5],a[7]) );
        dst.setFrame( x0, y0, x1-x0, y1-y0 );
        return dst;
    }

    /**
     * Transforms specified rectangle and return its bounds
     *
     * @param m      matrix which is used to transform the rectangle
     * @param src    rectangle which is transformed
     * @return new rectangle which is used to hold the bounds
     */
    public static Rectangle2D calcBounds( AffineTransform m, Rectangle2D src ) {
        return calcBounds( m, src, newRectangle() );
    }

    /**
     * Adds one specified rectangle to another
     *
     * @param dst    destination rectangle (can be null)
     * @param src    rectangle which is going to be added to destination one (can be null)
     * @return destination rectangle
     */
    public static Rectangle2D add( Rectangle2D dst, Rectangle2D src ) {
        if( src == null ) return dst;
        if( dst == null ) {
            dst = (Rectangle2D) src.clone();
        } else {
            dst.add( src );
        }
        return dst;
    }

    /**
     * Returns size of specified rectangle as if it were written to swf file
     *
     * @param r      specified rectangle
     * @return size of rectangle in bytes
     */
    public static int getSize( Rectangle2D r ) {
        int xmin = (int) r.getMinX();
        int xmax = (int) r.getMaxX();
        int ymin = (int) r.getMinY();
        int ymax = (int) r.getMaxY();

        int nBits = Util.getMinBitsS( Util.getMax(xmin,xmax,ymin,ymax) );
        int s = 5+nBits*4;
        return (s+7)/8;
    }

    /**
     * Concatenate two matrix without modifying them
     *
     * @param m1     left matrix
     * @param m2     right matrix
     * @return result of concatenation of two matrix (new matrix)
     */
    public static AffineTransform concatenate( AffineTransform m1, AffineTransform m2 ) {
        AffineTransform res = (AffineTransform) m1.clone();
        res.concatenate( m2 );
        return res;
    }

    private static final double c1 = 10000.0;

    /**
     * Discard 'scale' of the matrix
     * <p>
     * retrieve scale of the matrix and create new one which is descale
     * of the first and then multiply them in right order
     *
     * @param m      matrix to discard scale from, this matrix is modified
     * @return same matrix but scale is 1
     */
    public static AffineTransform deScaleMatrix( AffineTransform m ) {

        double scalex = 1.0;
        double scaley = 1.0;

        double[] r = new double[] { 0, c1, 0, 0, c1, 0 };

        m.transform( r, 0, r, 0, 3 );

        boolean mult = false;
        double sz = getDist( r, 0 );
        if( Math.abs(sz-c1) > 1e-2 ) {
            scaley = c1/sz;
            mult = true;
        }
        sz = getDist( r, 2 );
        if( Math.abs(sz-c1) > 1e-2 ) {
            scalex = c1/sz;
            mult = true;
        }

        if( mult ) {
            m.scale( scalex, scaley );
        }
        return m;
    }

    /**
     * Get values inverse to 'scale' of the matrix
     * <p>
     * retrieve inverse scale of the matrix
     *
     * @param m      matrix to get inverse scale from
     * @return array of x and y descale
     */
    public static double[] getMatrixScale( AffineTransform m ) {

        double scalex = 1.0;
        double scaley = 1.0;

        double[] r = new double[] { 0, c1, 0, 0, c1, 0 };

        m.transform( r, 0, r, 0, 3 );

        double sz = getDist( r, 0 );
        if( Math.abs(sz-c1) > 1e-2 ) {
            scaley = c1/sz;
        }
        sz = getDist( r, 2 );
        if( Math.abs(sz-c1) > 1e-2 ) {
            scalex = c1/sz;
        }
        return new double[] {scalex, scaley};
    }

    /**
     * Discard 'rotate' of the matrix
     * <p>
     * retrieve scale and transform of the matrix and create new one from them
     *
     * @param m      matrix to discard rotate from, this matrix is modified
     * @return same matrix but without rotate
     */
    public static AffineTransform deRotateMatrix( AffineTransform m ) {
        if( m.getShearX() == 0.0 && m.getShearY() == 0.0 ) return m;

        double scalex = 1.0;
        double scaley = 1.0;

        double[] r = new double[] { 0, c1, 0, 0, c1, 0 };

        m.transform( r, 0, r, 0, 3 );

        double sz = getDist( r, 0 );
        if( Math.abs(sz-c1) > 1e-2 ) {
            scaley = sz/c1;
        }
        sz = getDist( r, 2 );
        if( Math.abs(sz-c1) > 1e-2 ) {
            scalex = sz/c1;
        }

        m.setTransform( scalex, 0, 0, scaley, m.getTranslateX(), m.getTranslateY() );
        return m;
    }

    /**
     * Return distance between two points defined by the Rect
     *
     * @param r      rectangle which defines two points
     * @return distance
     */
    public static double getDist( Rectangle2D r ) {
        double dx = r.getWidth();
        double dy = r.getHeight();
        return Math.sqrt( dx*dx + dy*dy );
    }

    /**
     * Return distance between two points defined by array
     *
     * @param r      4 elements array which hold the points (x0,y0), (x1,y1)
     * @return distance between two points
     */
    public static double getDist( double[] r ) {
        return getDist( r, 0 );
    }

    /**
     * Return distance between two points defined by array
     *
     * @param r      array which hold the points (x0,y0), (x1,y1)
     * @param offset offset in the array
     * @return distance between two points
     */
    public static double getDist( double[] r, int offset ) {
        double dx = r[offset+2]-r[offset+0];
        double dy = r[offset+3]-r[offset+1];
        return Math.sqrt( dx*dx + dy*dy );
    }

    /**
     * Transforms specified rect using specified matrix and
     * returns rectangle which has width and height as of transformed one.
     *
     * @param m      specified matrix
     * @param r      specified rectangle
     * @return rectangle which has width and height as of transformed one.
     */
    public static Rectangle2D getTransformedSize( AffineTransform m, Rectangle2D r ) {
        double[] a = new double[] { 0, r.getHeight(), 0, 0, r.getWidth(), 0 };

        m.transform( a, 0, a, 0, 3 );

        return newRectangle( 0, 0, getDist(a,2), getDist(a,0) );
    }

    /**
     * Quadratic Bezier interpolation
     * <p>
     * B(t) = P0*(1-t)^2 + P1*2*t*(1-t) + P2*t^2
     *
     * @param p0     first control point (anchor)
     * @param p1     second control point (control)
     * @param p2     third control point (anchor)
     * @param t      parameter from 0 to 1
     * @return point on the curve corresponding given parameter
     */
    public static Point2D quadraticBezier( Point2D p0, Point2D p1, Point2D p2, double t ) {
        double tt  = t*t;       // t^2
        double t1  = 1-t;       // 1-t
        double tt1 = t1*t1;     // (1-t)^2
        double tt4 = 2*t*t1;    // 2*t*(1-t)

        double x = p0.getX()*tt1 + p1.getX()*tt4 + p2.getX()*tt;
        double y = p0.getY()*tt1 + p1.getY()*tt4 + p2.getY()*tt;

        return new Point2D.Double(x,y);
    }

    /**
     * Cubic Bezier interpolation
     * <p>
     * B(t) = P0*(1-t)^3 + P1*3*t*(1-t)^2 + P2*3*t^2*(1-t) + P3*t^3
     *
     * @param p0     first control point
     * @param p1     second control point
     * @param p2     third control point
     * @param p3     fourth control point
     * @param t      parameter from 0 to 1
     * @return point on the curve corresponding given parameter
     */
    public static Point2D cubicBezier( Point2D p0, Point2D p1, Point2D p2, Point2D p3, double t ) {
        double tt  = t*t;       // t^2
        double ttt = tt*t;      // t^3
        double t1  = 1-t;       // 1-t
        double tt1 = t1*t1;     // (1-t)^2
        double tt2 = tt1*t1;    // (1-t)^3
        double tt3 = 3*t*tt1;   // 3*t*(1-t)^2
        double tt4 = 3*tt*t1;   // 3*t^2*(1-t)

        double x = p0.getX()*tt2 + p1.getX()*tt3 + p2.getX()*tt4 + p3.getX()*ttt;
        double y = p0.getY()*tt2 + p1.getY()*tt3 + p2.getY()*tt4 + p3.getY()*ttt;

        return new Point2D.Double(x, y);
    }

    /**
     * Converts qubic bezier to quadratic one with some approximation
     * <P>
     * Used the following algorithm by Jens Alfke:<BR>
     * James Smith writes:
     * <P>
     * <LI>
     * <I> Can anyone show me a way to convert a Bezier cubic curve to a quadratic spline?
     *     Is this a trivial conversion?
     *     Can this conversion (if possible) be done to produce an identical curve from the Bezier to the spline?
     * </I></LI>
     * <P>
     * The answer is it's not trivial, and it will necessarily be an approximation
     * (since you're going from 3rd order to 2nd order). The usual technique is recursive subdivision:
     * <P>
     * <OL>
     * <LI>Create a quadric that tries to approximate your cubic.
     * <LI>Apply some metric to see how close an approximation it is.
     * <LI>If it's not close enough, break the cubic in half at the midpoint and recurse on each half.
     * </OL>
     * <P>
     * Call the Bezier curve ABCD, where A and D are the endpoints and B and C the control points.
     * For step (1) I found the intersection of AB and CD (call it E) and used AED as the quadric.
     * This is about your only option because the curve at A has to be parallel to AE, and at D has
     * to be parallel to ED. In step (2), I used as my metric the distance from the midpoint of the
     * quadric to the midpoint of the Bezier. The midpoint of the quadric is, if I remember correctly,
     * halfway between the midpoint of AE and the midpoint of ED. The midpoint of the Bezier involves
     * computing a few more midpoints; it's a standard construction that you should be able to find in a text.
     * <P>
     * This worked well enough but tended to produce more quadrics than was optimal. (In general there
     * were 2-3 times as many quadrics as Beziers. But the quadrics are faster to render, so it balances out.)
     * I know some people at Apple with more mathematical savvy than I have had worked on this a bit and
     * had interesting techniques where they didn't always break the Bezier in the middle, and were sometimes
     * able to merge pieces of adjacent Beziers into the same quadrics. This produced much more efficient
     * results. To my knowlege their results haven't been published anywhere; but they probably ought to be
     * now that the ship of QuickDraw GX is imminent. (If you have GX, you might look at the "cubic library",
     * which apparently does some or all of this stuff. It does describe cubics in quadric form, but I'm not
     * sure it does all the optimizations I've described in this paragraph...)
     *
     * @param p0     1-st control point of cubic bezier
     * @param p1     2-st control point of cubic bezier
     * @param p2     3-st control point of cubic bezier
     * @param p3     4-st control point of cubic bezier
     * @return array of qudratic bezier. each curve consists of three control points (Point2D)
     */
    public static Point2D[] CubicToQudratricBezier( Point2D p0, Point2D p1, Point2D p2, Point2D p3 ) {
        IVVector quadPoints = new IVVector();

        // make first quadric approximation
        Point2D q0 = p0;
        Point2D q1 = getIntersectionPoint(p0, p1, p2, p3);
        Point2D q2 = p3;

        // check and break if needed
        breakCubic( p0, p1, p2, p3, q0, q1, q2, 0.0, 1.0, quadPoints );

        Point2D[] res = new Point2D[ quadPoints.size() ];
        quadPoints.copyInto( res );
        return res;
    }

    private static void breakCubic( Point2D c0, Point2D c1, Point2D c2, Point2D c3,
                                    Point2D q0, Point2D q1, Point2D q2,
                                    double t0, double t1, IVVector result )
    {
        // compute mid point on cubic curve on specified interval
        double mp = t0+(t1-t0)*0.5;
        Point2D cubic_mid_point = cubicBezier(c0, c1, c2, c3, mp);

        // compute mid point on quatric curve on interval 0..1
        Point2D quad_mid_point = quadraticBezier(q0, q1, q2, 0.5);

        // compute distance between mid points
        double dist = quad_mid_point.distance( cubic_mid_point );

        // if distance less than one twixel, then we are done
        if( dist < 20 ) {
            result.addElement( q0 );
            result.addElement( q1 );
            result.addElement( q2 );
            return;
        }

        // compute tangent of cubic curve at mid point
        Point2D dl = derivativeOfCubicBezier(c0, c1, c2, c3, mp);

        // transfer tangent vector to cibic mid point, so they they together represent true tangent
        dl.setLocation( dl.getX()+cubic_mid_point.getX(), dl.getY()+cubic_mid_point.getY() );

        // left subdivision
        Point2D qq1 = getIntersectionPoint(q0, q1, cubic_mid_point, dl);
        breakCubic( c0, c1, c2, c3, q0, qq1, cubic_mid_point, t0, mp, result );

        // right subdivision
        qq1 = getIntersectionPoint(q2, q1, cubic_mid_point, dl);
        breakCubic( c0, c1, c2, c3, cubic_mid_point, qq1, q2, mp, t1, result );
    }

    /**
     * Computes derivative of cubic bezier at specified point
     *
     * @param p0     cubic curve parameter
     * @param p1     cubic curve parameter
     * @param p2     cubic curve parameter
     * @param p3     cubic curve parameter
     * @param t      specified point on the curve
     * @return derivative of specified curve at specified point
     */
    private static Point2D derivativeOfCubicBezier( Point2D p0, Point2D p1, Point2D p2, Point2D p3, double t ) {
        double ax = 3*p1.getX() - 3*p2.getX() - p0.getX() + p3.getX();
        double bx = 3*(p0.getX() - 2*p1.getX() + p2.getX());
        double cx = 3*(p1.getX() - p0.getX());

        double ay = 3*p1.getY() - 3*p2.getY() - p0.getY() + p3.getY();
        double by = 3*(p0.getY() - 2*p1.getY() + p2.getY());
        double cy = 3*(p1.getY() - p0.getY());

        double x = 3*ax*t*t + 2*bx*t + cx;
        double y = 3*ay*t*t + 2*by*t + cy;

        return new Point2D.Double(x, y);
    }

    /**
     * Computes intersection of two lines
     * <P>
     * Each line is specified by two points
     *
     * @param a0     first point of line A
     * @param a1     second point of line A
     * @param b0     first point of line B
     * @param b1     second point of line B
     * @return intersection point
     */
    public static Point2D getIntersectionPoint( Point2D a0, Point2D a1, Point2D b0, Point2D b1 ) {
        double dAx = a1.getX()-a0.getX();
        double dAy = a1.getY()-a0.getY();
        double dBx = b1.getX()-b0.getX();
        double dBy = b1.getY()-b0.getY();
        double Fa  = dAx*a0.getY() - dAy*a0.getX();
        double Fb  = dBx*b0.getY() - dBy*b0.getX();
        double ddd = dBy*dAx - dBx*dAy;

        double x = (Fa*dBx - Fb*dAx) / ddd;
        double y = (Fa*dBy - Fb*dAy) / ddd;

        return new Point2D.Double(x,y);
    }
}

