/*
 * $Id: Sound.java,v 1.2 2002/02/15 23:44:28 skavish Exp $
 *
 * ===========================================================================
 *
 */

package org.openlaszlo.iv.flash.api.sound;

import org.openlaszlo.iv.flash.api.*;
import org.openlaszlo.iv.flash.parser.*;
import org.openlaszlo.iv.flash.util.*;
import java.io.PrintStream;

/**
 * Sound base class
 *
 * @author James Taylor
 *
 */

public abstract class Sound extends FlashDef
{
    public static final int COMPRESS_NONE  = 0;
    public static final int COMPRESS_ADPCM = 1;
    public static final int COMPRESS_MP3   = 2;

    public static final String [] compressions = { "None", "ADPCM", "MP3" };

    public static final int RATE_5_5 = 0;
    public static final int RATE_11  = 1;
    public static final int RATE_22  = 2;
    public static final int RATE_44  = 3;

    public static final int [] rates = { 5500, 11025, 22050, 44100 };

    public int compressFormat;
    public int rate;
    public boolean isSample16bit;
    public boolean isStereo;
    public int sampleCount;

    public int getTag()
    {
        return Tag.DEFINESOUND;
    }

    public void printContent( PrintStream out, String indent )
    {
        out.println( indent+"Sound: id="+getID()+", name='"+getName()+"'" );
    }

    public boolean isConstant()
    {
        return true;
    }

    /**
     * Return data size
     */
    public abstract int getSize();

    protected FlashItem copyInto( FlashItem item, ScriptCopier copier )
    {
        super.copyInto( item, copier );

        ( (Sound) item ).compressFormat = compressFormat;
        ( (Sound) item ).rate = rate;
        ( (Sound) item ).isSample16bit = isSample16bit;
        ( (Sound) item ).isStereo = isStereo;
        ( (Sound) item ).sampleCount = sampleCount;

        return item;
    }

    public abstract void write( FlashOutput fob );
}
