/*
 * $Id: MP3SoundStreamBlock.java,v 1.2 2002/02/15 23:44:28 skavish Exp $
 *
 * ===========================================================================
 *
 */

package org.openlaszlo.iv.flash.api.sound;

import java.io.PrintStream;
import org.openlaszlo.iv.flash.parser.*;
import org.openlaszlo.iv.flash.util.*;
import org.openlaszlo.iv.flash.api.*;

public class MP3SoundStreamBlock extends SoundStreamBlock
{
    public int sampleCount; // MP3StreamSoundData.SampleCount

    public int delaySeek;   // MP3SoundData.DelaySeek
    public DataMarker data; // MP3SoundData.MP3Frames

    public void write( FlashOutput fob )
    {
        fob.writeTag( getTag(), 2 + 2 + data.length() );

        fob.writeWord( sampleCount );

        fob.writeWord( delaySeek );

        data.write( fob );
    }

    public boolean isConstant()
    {
        return true;
    }

    protected FlashItem copyInto( FlashItem item, ScriptCopier copier )
    {
        super.copyInto( item, copier );

        ( (MP3SoundStreamBlock) item ).sampleCount = sampleCount;
        ( (MP3SoundStreamBlock) item ).delaySeek = delaySeek;

        ( (MP3SoundStreamBlock) item ).data = data;

        return item;
    }
    public FlashItem getCopy( ScriptCopier copier )
    {
        return copyInto( new MP3SoundStreamBlock(), copier );
    }
}
