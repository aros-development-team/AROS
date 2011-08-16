package org.aros.bootstrap;

import java.io.FileDescriptor;
import java.nio.ByteBuffer;

import android.app.Application;
import android.content.Intent;
import android.os.Environment;
import android.util.Log;

final class DisplayData
{
	int		   Width;
	int		   Height;
	BitmapData Bitmap;
}

final class BitmapData
{
	int		   Left;
	int		   Top;
	int		   Width;
	int		   Height;
	int		   BytesPerRow;
	ByteBuffer Pixels;
}

public class AROSBootstrap extends Application
{
	public int DisplayWidth;
	public int DisplayHeight;
	public BitmapData Bitmap;
	public AROSActivity ui;
	private DisplayServer Server;
	private Boolean started;

	// Commands sent to us by AROS display driver
	static final int cmd_Nak    = -1;
	static final int cmd_Query  = 1;
	static final int cmd_Show   = 2;
	static final int cmd_Update = 3;

	@Override
	public void onCreate()
	{
        Log.d("AROS", "Started");
        started = false;
	}

	@Override
	public void onLowMemory()
	{
		Log.d("AROS", "Memory panic");
		// TODO: Here we may send a command to display driver to flush libraries
	}

	public void Boot()
	{

		// We get here after the activity is laid out and we know screen size.
		// However we need to actually run AROS only once, when the application is
		// first started. The activity may be flushed by Android OS if it's hidden,
		// in this case it will be recreated from scratch.
		if (started)
		{
			Log.d("AROS", "Already running");
			return;
		}

	    // Get external storage path 
	    String extdir = Environment.getExternalStorageDirectory().getAbsolutePath();
	    String arosdir = extdir + "/AROS";
	    FileDescriptor readfd = new FileDescriptor();
	    FileDescriptor writefd = new FileDescriptor();

	    Log.d("AROS", "Starting AROS, root path: " + arosdir);

	    int rc = Start(arosdir, readfd, writefd);

	    if (rc == 0)
	    {
	    	started = true;

	      	// Initialize and run display server thread
	        AROSBootstrap.this.Server = new DisplayServer(AROSBootstrap.this, readfd, writefd);
	        AROSBootstrap.this.Server.start();
		}
	}

	// Make sure the activity is visible
	private void Foreground()
	{
    	Intent myIntent = new Intent(this, AROSActivity.class);
    	startActivity(myIntent);
	}
	
    public void DisplayError(String text)
    {
    	Foreground();
    	ui.DisplayError(text);
    }

	private void DoCommand(int cmd, int[] params)
	{
		switch (cmd)
		{
		case cmd_Query:
			Log.d("AROS", "cmd_Query(" + params + ")");

			Server.ReplyCommand(cmd, DisplayWidth, DisplayHeight);
			break;

		case cmd_Show:
			Log.d("AROS", "cmd_Show(" + params + ")");

			if (params[6] == 0)
			{
				// Drop the displayed bitmap (if any)
				Bitmap = null;
			}
			else
			{
				// Create Java reflection of AROS bitmap
				Bitmap = new BitmapData();

				Bitmap.Left		   = params[1];
				Bitmap.Top		   = params[2];
				Bitmap.Width	   = params[3];
				Bitmap.Height	   = params[4];
				Bitmap.BytesPerRow = params[5];
				Bitmap.Pixels      = MapMemory(params[6], params[5] * params[4]);
			}

			Server.ReplyCommand(cmd);
			break;

		case cmd_Update:
			if (ui != null)
			{
				BitmapView view = ui.GetBitmap(params[0]);

				view.invalidate(params[1], params[2], params[3], params[4]);
			}
			// This command doesn't need a reply
			break;

		default:
			Log.d("AROSDisplay", "Unknown command " + cmd);

			Server.ReplyCommand(cmd_Nak, cmd);
			break;
		}
	}
	
    // This orders processing of a command from server
    class ServerCommand implements Runnable
    {
    	private int Command;
    	private int[] Parameters;

    	public ServerCommand(int cmd, int... params)
    	{
    		Command = cmd;
    		Parameters = params;
    	}

    	public void run()
    	{
    		// It's tedious to type AROSBootstrap.this. every time,
    		// so i simply forward the event to outer class.
			AROSBootstrap.this.DoCommand(Command, Parameters);
    	}
    }

    static
    {
        System.loadLibrary("AROSBootstrap");
    }
	
	// libAROSBootstrap native methods
    private native int Start(String dir, FileDescriptor rfd, FileDescriptor wfd);
    private native ByteBuffer MapMemory(long addr, long size);
}
