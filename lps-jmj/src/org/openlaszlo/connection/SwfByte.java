/* ****************************************************************************
 * SWFByte.java
* ****************************************************************************/

/* J_LZ_COPYRIGHT_BEGIN *******************************************************
* Copyright 2001-2004 Laszlo Systems, Inc.  All Rights Reserved.              *
* Use is subject to license terms.                                            *
* J_LZ_COPYRIGHT_END *********************************************************/

package org.openlaszlo.connection;

import org.openlaszlo.iv.flash.api.action.Actions;
import org.openlaszlo.iv.flash.api.action.Program;
import org.openlaszlo.iv.flash.util.FlashBuffer;
import org.openlaszlo.iv.flash.util.Tag;
import org.openlaszlo.xml.internal.DataCompiler;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.awt.geom.Rectangle2D;
import java.util.Vector;
import org.jdom.Element;

//**** Note: SWF files are stored little-endian (least significant bits first in order)

/** Helper class to send dribbled event information down to Flash client.
 */
public class SwfByte
{
    /** Default buffer size to set FlashBuffer */
    static private final int DEFAULT_FLASH_BUFFER_SIZE = 100;

    /** Default header length to send */
    static public final int DEFAULT_HEADER_LENGTH = 65535;

    /** Default header x-min */
    static public final int DEFAULT_HEADER_X_MIN = 0;

    /** Default header x-max */
    static public final int DEFAULT_HEADER_X_MAX = 400;

    /** Default header y-min */
    static public final int DEFAULT_HEADER_Y_MIN = 0;

    /** Default header y-max */
    static public final int DEFAULT_HEADER_Y_MAX = 400;

    /** Default header frame rate */
    static public final double DEFAULT_HEADER_FRAME_RATE = 12.0;

    /** Default header frame count */
    static public final int DEFAULT_HEADER_FRAME_COUNT = 4200;

    /** Flash buffer to send partial information */
    private FlashBuffer mBuf = null;


    private int mFlashVersion = 5;

    /** Makes sure that a buffer has been created.
     */
    private void ensureBuffer()
    {
        if (mBuf == null)
            mBuf = new FlashBuffer(DEFAULT_FLASH_BUFFER_SIZE);
    }

    /** Returns exact number of bytes written to buffer.
     * @return buffer
     */
    public byte[] getBuf()
    {
        return getExactBytes(mBuf);
    }

    /** Sets the exact length of bytes written to header at moment this method
     * is called.
     */
    public SwfByte setLength()
    {
        return this.setLength(mBuf.getPos());
    }

    /** Sets the specified length of bytes to header.
     * @param length length to set
     */
    public SwfByte setLength(int length)
    {
        mBuf.writeDWordAt(length, 4);
        return this;
    }


    /** Creates SWF header with default values. Length must be explicitly set by
     * calling on the setLength() methods. If header has been set before this,
     * it will be overridden.
     */
    public SwfByte setHeader(int version)
    {
        ensureBuffer();

        return this.setHeader(version,
                            DEFAULT_HEADER_X_MIN, DEFAULT_HEADER_X_MAX,
                            DEFAULT_HEADER_Y_MIN, DEFAULT_HEADER_Y_MAX,
                            DEFAULT_HEADER_FRAME_RATE, DEFAULT_HEADER_FRAME_COUNT);
    }



    /** Creates SWF header. Length must be explicitly set by calling on the
     * setLength() methods. If header has been set before this,
     * it will be overridden.
     * @param version version number of SWF
     * @param xMin x-min coordinates for frame size (in TWIPS)
     * @param xMax x-max coordinates for frame size (in TWIPS)
     * @param yMin x-min coordinates for frame size (in TWIPS)
     * @param yMax y-max coordinates for frame size (in TWIPS)
     * @param frameRate (not used)
     * @param frameCount total number of frames in movie
     */
    public SwfByte setHeader(int version, 
                             int xMin, int xMax, 
                             int yMin, int yMax, 
                             double frameRate, int frameCount)
    {
        ensureBuffer();

        int w = xMax - xMin;
        int h = yMax - yMin;
        Rectangle2D rect = new Rectangle2D.Double(xMin, yMin, w, h);
        return this.setHeader(version, rect, frameRate, frameCount);
    }


    /** Creates SWF header. Length must be explicitly set by calling on the
     * setLength() methods. If header has been set before this,
     * it will be overridden.
     * @param version version number of SWF
     * @param rect rectangular coordinates for frame size (in TWIPS)
     * @param frameRate (not used)
     * @param frameCount total number of frames in movie
     */
    public SwfByte setHeader(int version, Rectangle2D rect, 
                             double frameRate, int frameCount)
    {
        ensureBuffer();

        // Signature - 1 byte for each letter
        mBuf.writeByte('F');
        mBuf.writeByte('W');
        mBuf.writeByte('S');

        // Version - 1 byte
        mBuf.writeByte(version);

        // Length of file - 4 bytes (skip for now)
        mBuf.skip(4);

        // Frame size - RECT
        mBuf.write(rect);
        
        // Frame rate - 2 bytes (8.8 fixed) [made it 12.0]
        //   (make sure this fixed point value is written little-endian)
        mBuf.writeByte(0);
        mBuf.writeByte(12);

        // Frame count - 2 bytes
        mBuf.writeWord(frameCount);

        return this;
    }


    /** Write byte-code to set a variable with a value.
     * @param lVal variable to set
     * @param rVal value to set variable with
     */
    public SwfByte actionSetVariable(String lVal, String rVal)
    {
        ensureBuffer();

        Program prg = new Program();
        prg.push(lVal);
        prg.push(rVal);
        prg.setVar();
        prg.none();

        // Setting the size of prgBuf will to pos will ensure we copy the exact
        // number of bytes
        FlashBuffer prgBuf = prg.body();
        prgBuf.setSize(prgBuf.getPos());

        mBuf.writeTag(Tag.DOACTION, prgBuf.getSize());
        mBuf.writeFOB(prgBuf);
        return this;
    }


    /** Uses DataCompiler to generate action bytecode based on XML. */
    public SwfByte actionSetElement(Element el)
    {
        ensureBuffer();
        Program prg = DataCompiler.makeProgram(el, mFlashVersion);
        prg.none();
        FlashBuffer prgBuf = prg.body();
        mBuf.writeTag(Tag.DOACTION, prgBuf.getSize());
        mBuf.writeFOB(prgBuf);
        return this;
    }



    /** Write byte-code to show frame.
     */
    public SwfByte setShowFrame()
    {
        ensureBuffer();
        mBuf.writeTag(Tag.SHOWFRAME, 0);
        return this;
    }
    
    /** Returns the actual amount written to the FlashBuffer, instead of
     * returning the buffer itself. The buffer size is greater than or equal to
     * the last position.
     * @param buf flash buffer
     * @return the actual bytes being stored in the buffer
     */
    static public byte[] getExactBytes(FlashBuffer buf)
    {
        int len = buf.getPos();
        byte[] oldBuf = buf.getBuf();
        byte[] newBuf = new byte[len];
        System.arraycopy(oldBuf, 0, newBuf, 0, len);
        return newBuf;
    }
}
