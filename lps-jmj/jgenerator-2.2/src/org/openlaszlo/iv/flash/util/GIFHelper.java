/*
 * $Id: GIFHelper.java,v 1.2 2002/02/15 23:44:28 skavish Exp $
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
import java.util.zip.Deflater;
import org.openlaszlo.iv.flash.parser.DataMarker;

/**
 * GIF Reader utility class
 * Parse GIF code from an inputStream without using java.awt
 * (to avoid the need of a display on a server)
 *
 * Use a modified version of the GIFDecoder part the PJA Toolkit
 * Part of the implementation has been commented because we didn't need it
 * Basically everything that was dealing with GIF comments output
 * and the creation of an RGB image
 *
 * With kind permission of Emmanuel PUYBARET from eteks
 * Lots of contributions here, but the code is reliable and approved
 *
 * Following is the initial copyright notice (which for this project is Apache style)
 *
 * ****************************************************************************
 *
 * Copyright (c) 2000 Emmanuel PUYBARET / eTeks <info@eteks.com>. All Rights Reserved.
 *
 * Raster over-run errors fixed by Alan Dix (alan@hcibook.com www.hiraeth.com/alan)
 *
 * Visit eTeks web site for up-to-date versions of the original file and other
 * Java tools and tutorials : http://www.eteks.com/
 *
 * ****************************************************************************
 *
 * This file is based and translated from the following C source file :
 *
 * xvgif.c  -  GIF loading code for 'xv'.  Based strongly on...
 *
 * gif2ras.c - Converts from a Compuserve GIF (tm) image to a Sun raster image.
 *
 * Copyright (c) 1988, 1989 by Patrick J. Naughton
 *
 * Author: Patrick J. Naughton
 * naughton@wind.sun.com
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted,
 * provided that the above copyright notice appear in all copies and that
 * both that copyright notice and this permission notice appear in
 * supporting documentation.
 *
 * This file is provided AS IS with no warranties of any kind.  The author
 * shall have no liability with respect to the infringement of copyrights,
 * trade secrets or any patents by this file or any part thereof.  In no
 * event will the author be liable for any lost revenue or profits or
 * other special, indirect and consequential damages.
 *
 * ****************************************************************************
 *
 */

public class GIFHelper
{

    private byte           bytePixels [];      // image data in index model
    private int            width;              // image width
    private int            height;             // image height
    private int            bitMask;            // size of the colormap - 1;
    private int            colorMapSize;       // actual size of the colormap

    private InputStream    input;              // the GIF ByteArrayInputStream
    private byte           data[];             // the compressed (zlib) byte array


/******************************************************************************
 * The following methods are the getters needed for JGen to process
 * a new LBitmap object to insert into the Script
 * Added to the original GIFReader especially for JGen
 ******************************************************************************/

    public int getColorTableSize() {
        return bitMask;
    }

    public int getWidth() {
        return width;
    }

    public int getHeight() {
        return height;
    }

    public DataMarker getZlibData() throws IOException {
        int deflatedSize = deflate(); // compress the data
        return new DataMarker(data,0,deflatedSize);
    }

    public boolean isAlpha() {
        return (transparentIndex > -1);
    }

    /**
     * Launch the loadGIF routine
     *
     * @param buffer
     * @exception IOException
     */
    public void doRead( byte[] buffer ) throws IOException {
        doRead( new ByteArrayInputStream(buffer, 0, buffer.length) );
    }

    public void doRead( FlashBuffer fob ) throws IOException {
        doRead( fob.getInputStream() );
    }

    public void doRead( InputStream input ) throws IOException {
        this.input = input;
        try {
            loadGIF(input);
        } finally {
            try {
                input.close();
            } catch (IOException ioe) {}
        }
    }

    /**
     * Performs the ZLib compression.<br>
     *
     * <p>Everything from here is the private implementation of the reader
     * But we don't want to disturb other classes with that stuff, do we ?
     *
     * @return
     * @exception IOException
     */
    private int deflate() throws IOException {
        int retour = 0;
        int plus = 0;
        if (transparentIndex > -1)
            plus++;
        int maxpixels = bytePixels.length;
        int falsewidth = width;
        int added = 0;
        if (width % 4 > 0)
        {
            while (falsewidth % 4 > 0)
            {
                falsewidth++;
                added++;
            }
            maxpixels = falsewidth * height;
        }
        byte[] map = new byte[colorMapSize*(3+plus) + maxpixels];
        for (int i = 0; i < colorMapSize; i++)
        {
            if (transparentIndex > -1)
            {
                if (transparentIndex == i)
                {
                    map[i*(3+plus)] = 0;
                    map[(i*(3+plus))+1] = 0;
                    map[(i*(3+plus))+2] = 0;
                    map[(i*(3+plus))+3] = 0;
                }
                else
                {
                    map[i*(3+plus)] = r[i];
                    map[(i*(3+plus))+1] = g[i];
                    map[(i*(3+plus))+2] = b[i];
                    map[(i*(3+plus))+3] = (byte) 255;
                }
            }
            else
            {
                map[i*(3+plus)] = r[i];
                map[(i*(3+plus))+1] = g[i];
                map[(i*(3+plus))+2] = b[i];
            }
        }
        if (width % 4 > 0)
        {
            byte[] tempPixels = new byte[maxpixels];
            int idTemp = 0;
            int idByte = 0;
            for (int h = 0; h < height; h++)
            {
                for (int w = 0; w < width; w++)
                    tempPixels[idTemp++] = bytePixels[idByte++];
                for (int i = 0; i < added; i++)
                    tempPixels[idTemp++] = 0x00;
            }
            bytePixels = tempPixels;
        }
        System.arraycopy(bytePixels,0,map,(colorMapSize*(3+plus)),bytePixels.length);
        Deflater def = new Deflater();
        def.setInput(map);
        data = new byte[(colorMapSize*(3+plus))+bytePixels.length];
        def.finish();
        retour = def.deflate(data);
        return retour;
    }

/******************************************************************************
 * This is where the original GIFReader from PJA starts
 * A few things had been commented out because it wasn't needed
 ******************************************************************************/

    // Where the xvgif.c translation starts

    private int bitOffset = 0;                  // Bit Offset of next code
    private int XC = 0;                         // Output X coords of current pixel
    private int YC = 0;                         // Output Y coords of curent pixel
    private int pass = 0;                       // Used by output routine if interlaced pixels
    private int ptr  = 0;                       // Just an index
    private int oldYC = -1;                     // Here to remember YC

    private byte r [] = new byte [256];         // red colormap value
    private byte g [] = new byte [256];         // green colormap value
    private byte b [] = new byte [256];         // blue colormap, value
    private int transparentIndex = -1;          // index of the transparent color in the index array

  // private int            intPixels [];       // image data in RGB model
  // we don't really need this one, since we only compute the bytePixels Array

  // private String     fullInfo;              // Format: field in info box
  // private String     shortInfo;             // short format info
  // private String     comment;               // comment text

    private final static String id87 = "GIF87a";
    private final static String id89 = "GIF89a";

    private final static short EGA_PALETTE [][] = {
        {0,0,0},       {0,0,128},     {0,128,0},     {0,128,128},
        {128,0,0},     {128,0,128},   {128,128,0},   {200,200,200},
        {100,100,100}, {100,100,255}, {100,255,100}, {100,255,255},
        {255,100,100}, {255,100,255}, {255,255,100}, {255,255,255} };

    private final static byte EXTENSION     = 0x21;
    private final static byte IMAGESEP      = 0x2c;
    private final static byte TRAILER       = 0x3b;
    private final static byte INTERLACEMASK = 0x40;
    private final static byte COLORMAPMASK  = (byte)0x80;

    private void loadGIF (InputStream input) throws IOException
    {
        if (!(input instanceof BufferedInputStream))
            input = new BufferedInputStream (input);

        // Use a DataInputStream to have EOFException if file  is corrupted
        DataInputStream dataInput = new DataInputStream (input);

        // initialize variables
        bitOffset = XC = YC = pass = 0;
        boolean gotimage = false;
        boolean gif89  = false;

        byte [] idBytes = new byte [6];
        for (int i = 0; i < 6; i++)
            idBytes [i] = dataInput.readByte ();

        String id = new String (idBytes);
        if (id.equals (id87))
            gif89 = false;
        else if (id.equals (id89))
            gif89 = true;
        else
            warning (input, "not a GIF file");

        // Get variables from the GIF screen descriptor
        byte aByte = dataInput.readByte ();
        int screenWidth  = ((int)aByte & 0xFF) + 0x100 * (dataInput.readByte () & 0xFF);  // screen dimensions... not used.
        aByte = dataInput.readByte ();
        int screenHeight = ((int)aByte & 0xFF) + 0x100 * (dataInput.readByte () & 0xFF);

        aByte = dataInput.readByte ();
        boolean hasColormap = (aByte & COLORMAPMASK) != 0;

        // Bits per pixel, read from GIF header
        int bitsPerPixel = (aByte & 7) + 1;
        // Number of colors
        colorMapSize = 1 << bitsPerPixel;
        // AND mask for data size
        bitMask = colorMapSize - 1;

        int background = dataInput.readByte () & 0xFF;  // background color... not used.

        int aspect = dataInput.readByte () & 0xFF;
        if (aspect != 0)
            if (!gif89)
                warning (input, "corrupt GIF file (screen descriptor)");

        // Read in global colormap.
        if (hasColormap)
            for (int i = 0; i < colorMapSize; i++)
            {
                r [i] = dataInput.readByte ();
                g [i] = dataInput.readByte ();
                b [i] = dataInput.readByte ();
            }
        else
        {
            colorMapSize = 256;
            bitMask = 255;
            // no colormap in GIF file
            // put std EGA palette (repeated 16 times) into colormap, for lack of
            // anything better to do
            for (int i = 0; i < 256; i++)
            {
                r [i] = (byte)EGA_PALETTE [i & 15][0];
                g [i] = (byte)EGA_PALETTE [i & 15][1];
                b [i] = (byte)EGA_PALETTE [i & 15][2];
            }
        }

        for (int block = 0;
           (block = dataInput.readByte () & 0xFF) != TRAILER; )
        if (block == EXTENSION)
        {
            // possible things at this point are:
            //   an application extension block
            //   a comment extension block
            //   an (optional) graphic control extension block
            //       followed by either an image
            //     or a plaintext extension

            // parse extension blocks
            int fn, blocksize, aspnum, aspden;

            // read extension block
            fn = dataInput.readByte () & 0xFF;

            if (fn == 'R')
            {
                // GIF87 aspect extension
                int blockSize;

                blocksize = dataInput.readByte () & 0xFF;
                if (blocksize == 2)
                {
                    aspnum = dataInput.readByte ();
                    aspden = dataInput.readByte ();
                    if (aspden <= 0 || aspnum <= 0)
                    {
                        aspnum =
                        aspden = 1;
                    }
                }
                else
                    dataInput.skipBytes (blocksize);

                while ((blockSize = dataInput.readByte () & 0xFF) > 0)
                    // eat any following data subblocks
                    dataInput.skipBytes (blockSize);
            }
            else if (fn == 0xFE)
            {
                // Comment Extension
                // We don't really need them, so let's eat that too :
                int blockSize;
                while ((blockSize = dataInput.readByte () & 0xFF) > 0)
                    dataInput.skipBytes (blockSize);

                /* This part was in the original GIFReader but we don't use the GIF comments
                // to read the comments :
                for (int blockSize = 0; (blockSize = dataInput.readByte () & 0xFF) != 0; )
                {
                    byte commentBytes [] = new byte [blockSize];
                    for (int i = 0; i < blockSize; i++)
                        commentBytes [i] = dataInput.readByte ();

                    if (comment != null)
                        comment += "\n" + new String (commentBytes);
                    else
                        comment = new String (commentBytes);
                }
                */
            }
            else if (fn == 0x01)
            {
                // PlainText Extension
                int blockSize   = dataInput.readByte () & 0xFF;
                int tgLeft   = dataInput.readByte () & 0xFF;
                tgLeft   += (dataInput.readByte () & 0xFF) << 8;
                int tgTop    = dataInput.readByte () & 0xFF;
                tgTop    += (dataInput.readByte () & 0xFF) << 8;
                int tgWidth  = dataInput.readByte () & 0xFF;
                tgWidth  += (dataInput.readByte () & 0xFF) << 8;
                int tgHeight = dataInput.readByte () & 0xFF;
                tgHeight += (dataInput.readByte () & 0xFF) << 8;
                int cWidth   = dataInput.readByte () & 0xFF;
                int cHeight  = dataInput.readByte () & 0xFF;
                int fg       = dataInput.readByte () & 0xFF;
                int bg       = dataInput.readByte () & 0xFF;

                dataInput.skipBytes (blockSize - 12); // read rest of first subblock

                // read (and ignore) data sub-blocks
                while ((blockSize = dataInput.readByte () & 0xFF) != 0)
                  dataInput.skipBytes (blockSize);
            }
            else if (fn == 0xF9)
            {
                // Graphic Control Extension
                for (int blockSize = 0;
                    (blockSize = dataInput.readByte () & 0xFF) != 0; )
                // Added transparent GIF management here
                if (blockSize == 4)
                {
                    int ext1 = (dataInput.readByte () & 0xFF);
                    int ext2 = (dataInput.readByte () & 0xFF);
                    int ext3 = (dataInput.readByte () & 0xFF);
                    int ext4 = (dataInput.readByte () & 0xFF);

                    // v2.1.1 Changed condition for transparent GIFs
                    if ((ext1 & 0x01) != 0)
                        transparentIndex = ext4;
                }
                else
                    dataInput.skipBytes (blockSize);
            }
            else if (fn == 0xFF)
            {
                // Application Extension
                // read (and ignore) data sub-blocks
                for (int blockSize = 0;
                    (blockSize = dataInput.readByte () & 0xFF) != 0; )
                dataInput.skipBytes (blockSize);
            }
            else
            {
                // unknown extension
                // read (and ignore) data sub-blocks
                for (int blockSize = 0;
                    (blockSize = dataInput.readByte () & 0xFF) != 0; )
                dataInput.skipBytes (blockSize);
            }
        }
        else if (block == IMAGESEP)
        {
            if (gotimage)
            {
                // just skip over remaining images
                dataInput.skipBytes (8);   // left position
                                           // top position
                                           // width
                                           // height
                int misc = dataInput.readByte () & 0xFF;      // misc. bits

                if ((misc & 0x80) != 0)
                    // image has local colormap.  skip it
                    for (int i = 0; i < 1 << ((misc & 7) + 1);  i++)
                        dataInput.skipBytes (3);

                dataInput.skipBytes (1);       // minimum code size

                // skip image data sub-blocks
                for (int blockSize = 0;
                    (blockSize = dataInput.readByte () & 0xFF) != 0; )
                dataInput.skipBytes (blockSize);
            }
            else
            {
                readImage (dataInput, bitsPerPixel, bitMask, hasColormap, gif89);
                gotimage = true;
            }
        }
        else
        {
            // unknown block type
            // don't mention bad block if file was trunc'd, as it's all bogus
            String str =   "Unknown block type (0x" + Integer.toString (block, 16) + ")";
            warning (input, str);
            break;
        }

        if (!gotimage)
            warning (input, "no image data found in GIF file");
    }

    private void readImage (DataInputStream dataInput,
                            int             bitsPerPixel,
                            int             bitMask,
                            boolean         hasColormap,
                            boolean         gif89) throws IOException
    {
        int  npixels = 0;
        int  maxpixels = 0;

        // read in values from the image descriptor
        byte aByte = dataInput.readByte ();
        int leftOffset = (aByte & 0xFF) + 0x100 * (dataInput.readByte () & 0xFF);
        aByte = dataInput.readByte ();
        int topOffset  = (aByte & 0xFF) + 0x100 * (dataInput.readByte () & 0xFF);
        aByte = dataInput.readByte ();
        width  = (aByte & 0xFF) + 0x100 * (dataInput.readByte () & 0xFF);
        aByte = dataInput.readByte ();
        height = (aByte & 0xFF) + 0x100 * (dataInput.readByte () & 0xFF);

        int misc = dataInput.readByte ();  // miscellaneous bits (interlace, local cmap)
        boolean interlace = (misc & INTERLACEMASK) != 0;

        if ((misc & 0x80) != 0)
            for (int i = 0; i < 1 << ((misc & 7) + 1); i++)
            {
                r [i] = dataInput.readByte ();
                g [i] = dataInput.readByte ();
                b [i] = dataInput.readByte ();
            }


        if (!hasColormap && (misc & 0x80) == 0)
        {
            // no global or local colormap
        }

            // Start reading the raster data. First we get the intial code size
            // and compute decompressor constant values, based on this code size.

        // Code size, read from GIF header
        int codeSize = dataInput.readByte () & 0xFF;

        int clearCode = (1 << codeSize); // GIF clear code
        int EOFCode   = clearCode + 1;   // GIF end-of-information code
        int firstFree = clearCode + 2;   // First free code, generated per GIF spec
        int freeCode  = firstFree;       // Decompressor,next free slot in hash table

        // The GIF spec has it that the code size is the code size used to
        // compute the above values is the code size given in the file, but the
        // code size used in compression/decompression is the code size given in
        // the file plus one. (thus the ++).
        codeSize++;
        int initCodeSize = codeSize;     // Starting code size, used during Clear
        int maxCode = (1 << codeSize);   // limiting value for current code size
        int readMask = maxCode - 1;      // Code AND mask for current code size

        // UNBLOCK:
        // Read the raster data.  Here we just transpose it from the GIF array
        // to the raster array, turning it from a series of blocks into one long
        // data stream, which makes life much easier for readCode ().
        ByteArrayOutputStream bos = new ByteArrayOutputStream(10000);
        byte [] raster = null;
        for (int blockSize = 0; (blockSize = dataInput.readByte () & 0xFF) != 0; )
        {
            while (blockSize-- > 0) {
                bos.write(dataInput.readByte());
            }
        }
        raster = bos.toByteArray();

        // Allocate the 'pixels'
        maxpixels = width * height;
        bytePixels = new byte [maxpixels];
        int picptr = 0;

        // The hash table used by the decompressor
        int prefix [] = new int [4096];
        int suffix [] = new int [4096];
        // An output array used by the decompressor
        int outCode [] = new int [4097];
        int outCount = 0;   // Decompressor output 'stack count'

        int currentCode;    // Decompressor variables
        int oldCode = 0;
        int inCode;
        int finChar = 0;
        // Decompress the file, continuing until you see the GIF EOF code.
        // One obvious enhancement is to add checking for corrupt files here.
        int code = readCode (dataInput, raster, codeSize, readMask);
        while (code != EOFCode)
        {
            // Clear code sets everything back to its initial value, then reads the
            // immediately subsequent code as uncompressed data.
            if (code == clearCode)
            {
                codeSize = initCodeSize;
                maxCode = (1 << codeSize);
                readMask = maxCode - 1;
                freeCode = firstFree;
                code = readCode (dataInput, raster, codeSize, readMask);
                currentCode = oldCode = code;
                finChar = currentCode & bitMask;
                if (!interlace)
                    bytePixels [picptr++] = (byte)finChar;
                else
                    doInterlace (finChar);
                npixels++;
            }
            else
            {
                // If not a clear code, must be data: save same as currentCode and inCode

                // if we're at maxcode and didn't get a clear, stop loading
                if (freeCode >= 4096)
                    break;

                currentCode = inCode = code;

                // If greater or equal to freeCode, not in the hash table yet;
                // repeat the last character decoded
                if (currentCode >= freeCode)
                {
                    currentCode = oldCode;
                    if (outCount > 4096)
                        break;
                    outCode [outCount++] = finChar;
                }

                // Unless this code is raw data, pursue the chain pointed to by currentCode
                // through the hash table to its end; each code in the chain puts its
                // associated output code on the output queue.
                while (currentCode > bitMask)
                {
                    if (outCount > 4096)
                        break;   // corrupt file
                    outCode [outCount++] = suffix [currentCode];
                    currentCode = prefix[currentCode];
                }

                if (outCount > 4096)
                    break;

                // The last code in the chain is treated as raw data.
                finChar = currentCode & bitMask;
                outCode [outCount++] = finChar;

                // Now we put the data out to the Output routine.
                // It's been stacked LIFO, so deal with it that way...

                // safety thing:  prevent exceeding range of 'bytePixels'
                if (npixels + outCount > maxpixels)
                    outCount = maxpixels - npixels;

                npixels += outCount;
                if (!interlace)
                    for (int i = outCount - 1; i >= 0; i--)
                        bytePixels [picptr++] = (byte)outCode [i];
                else
                    for (int i = outCount - 1; i >= 0; i--)
                        doInterlace (outCode [i]);
                outCount = 0;

                // Build the hash table on-the-fly. No table is stored in the file.
                prefix [freeCode] = oldCode;
                suffix [freeCode] = finChar;
                oldCode = inCode;

                // Point to the next slot in the table.  If we exceed the current
                // maxCode value, increment the code size unless it's already 12.  If it
                // is, do nothing: the next code decompressed better be CLEAR

                freeCode++;
                if (freeCode >= maxCode)
                {
                    if (codeSize < 12)
                    {
                        codeSize++;
                        maxCode *= 2;
                        readMask = (1 << codeSize) - 1;
                    }
                }
            }

            code = readCode (dataInput, raster, codeSize, readMask);
            if (npixels >= maxpixels)
                break;
        }

        if (npixels != maxpixels)
        {
            if (!interlace)  // clear.EOBuffer
                for (int i = 0; i < maxpixels - npixels; i++)
                    bytePixels [npixels + i] = 0;
        }

        /* This was part of the original GIFReader, used to make an RGB Image
        // But we don't need the following since we only compute the bytePixels Array
        // fill in the GifImage structure

        intPixels = new int [bytePixels.length];
        for (int i = 0; i < bytePixels.length; i++)
            intPixels [i] =      transparentIndex > 0
                    && ((bytePixels [i] & 0xFF) == transparentIndex)
                    ? 0
                    :   0xFF000000
                      | ((r [bytePixels [i]] & 0xFF) << 16)
                      | ((g [bytePixels [i]] & 0xFF) << 8)
                      |  (b [bytePixels [i]] & 0xFF);
        */

        // We don't need any info or comments, really, so let's get rid of this
        /*
        fullInfo =   "GIF" +  ((gif89) ? "89" : "87")
                           + ", " + bitsPerPixel + " bit" + ((bitsPerPixel == 1) ? "" : "s") + "per pixel, "
                           + (interlace ? "" : "non-") + "interlaced.";

        shortInfo = width + "x" + height + " GIF" + ((gif89) ? "89" : "87");
        */

        // comment gets handled in main LoadGIF() block-reader
    }

  /**
   * Fetch the next code from the raster data stream.  The codes can be
   * any length from 3 to 12 bits, packed into 8-bit bytes, so we have to
   * maintain our location in the raster array as a BIT Offset.  We compute
   * the byte Offset into the raster array by dividing this by 8, pick up
   * three bytes, compute the bit Offset into our 24-bit chunk, shift to
   * bring the desired code to the bottom, then mask it off and return it.
   */
    private int readCode (DataInputStream input, byte raster [], int codeSize, int readMask) throws IOException
    {
        int byteOffset = bitOffset / 8;
        int inWordOffset = bitOffset % 8;
        // Alan Dix modification to fix raster over-run errors
        // int rawCode =   (raster [byteOffset] & 0xFF)
        //              + ((raster [byteOffset + 1] & 0xFF) << 8);
        int rawCode =   (raster [byteOffset] & 0xFF);
        if (byteOffset + 1 < raster.length)
            rawCode += ((raster [byteOffset + 1] & 0xFF) << 8);
        else if (codeSize + inWordOffset > 8)
            warning (input,   "short raster ?  raster.length = " + raster.length
                            + ", codeSize = " + codeSize
                            + ", readMask = " + readMask);
        // end of modification

        if (   codeSize >= 8 && byteOffset + 2 < raster.length)
            rawCode += (raster [byteOffset + 2] & 0xFF) << 16;
        rawCode >>= (bitOffset % 8);
        bitOffset += codeSize;
        return rawCode & readMask;
    }

    private void doInterlace (int index)
    {
        if (oldYC != YC)
        {
            ptr = YC * width;
            oldYC = YC;
        }

        if (YC < height)
            bytePixels [ptr++] = (byte)index;

        // Update the X-coordinate, and if it overflows, update the Y-coordinate
        if (++XC == width)
        {
            // deal with the interlace as described in the GIF
            // spec.  Put the decoded scan line out to the screen if we haven't gone
            // past the bottom of it
            XC = 0;

            switch (pass)
            {
                case 0:
                    YC += 8;
                    if (YC >= height)
                    {
                        pass++;
                        YC = 4;
                    }
                    break;

                case 1:
                    YC += 8;
                    if (YC >= height)
                    {
                        pass++;
                        YC = 2;
                    }
                    break;

                case 2:
                    YC += 4;
                    if (YC >= height)
                    {
                        pass++;
                        YC = 1;
                    }
                    break;

                case 3:
                    YC += 2;
                    break;

                default:
                    break;
            }
        }
    }

    private void warning (InputStream input, String  st) throws IOException
    {
        throw new IOException ("Warning ! " + input + " : " + st);
    }
}
