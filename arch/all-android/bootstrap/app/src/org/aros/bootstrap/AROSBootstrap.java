package org.aros.bootstrap;

import java.io.FileDescriptor;
import java.nio.ByteBuffer;

import android.app.Application;
import android.content.Intent;
import android.content.res.Configuration;
import android.graphics.Bitmap;
import android.os.Environment;
import android.os.Handler;
import android.util.Log;

final class BitmapData
{
	int	Left;
	int	Top;
	int	Width;
	int	Height;
	int BytesPerRow;
	int Address;
	ByteBuffer Buffer;
}

public class AROSBootstrap extends Application
{
	public int DisplayWidth;
	public int DisplayHeight;
	public int TitlebarSize;
	public int Orientation;
	public BitmapData Bitmap;
	public AROSActivity ui;
	private DisplayServer Server;
	public Boolean started = false;
	public Boolean NativeGraphics;
	private Handler handler;

	// Commands sent to us by AROS display driver
	static final int cmd_Nak    = -1;
	static final int cmd_Query  = 0x80000001;
	static final int cmd_Show   = 0x80000002;
	static final int cmd_Update = 0x00000003;
	static final int cmd_Scroll = 0x00000004;
	static final int cmd_Mouse  = 0x00000005;
	static final int cmd_Touch  = 0x00000006;
	static final int cmd_Key    = 0x00000007;
	static final int cmd_Flush  = 0x00000008;
	static final int cmd_Hello	= 0x80000009;
	static final int cmd_Alert  = 0x00001000;

	// Current procotol version, must match display driver
	static final int PROTOCOL_VERSION = 2;

	// Some Linux signals
	static final int SIGUSR2 = 12;
	static final int SIGTERM = 15;

	@Override
	public void onCreate()
	{
        Log.d("AROS", "Started");

        started = false;
        handler = new Handler();
	}

	@Override
	public void onLowMemory()
	{
		Log.d("AROS", "Memory panic");

		Server.ReplyCommand(cmd_Flush);
	}

	public void ColdBoot()
	{
	    // Get external storage path 
	    String arosdir = Environment.getExternalStorageDirectory().getAbsolutePath() + "/AROS";

	    Log.d("AROS", "Loading AROS, root path: " + arosdir);

	    int rc = Load(arosdir);

	    if (rc == 0)
	    	WarmBoot();
	}

	private void WarmBoot()
	{
	    FileDescriptor readfd  = new FileDescriptor();
	    FileDescriptor writefd = new FileDescriptor();

	    Log.d("AROS", "Starting AROS...");

		int rc = Kick(readfd, writefd);

		if (rc == 0)
		{
			started = true;

			// Initialize and run display server thread
	        AROSBootstrap.this.Server = new DisplayServer(AROSBootstrap.this, readfd, writefd);
	        AROSBootstrap.this.Server.start();
		}
	}

	private void Reset()
	{
		// Drop the bitmap and signal this to UI
		Bitmap = null;
		if (ui != null)
			ui.Show(0, null);
	}

    public void DisplayError(String text)
    {
    	ui.DisplayError(text);
    }

    public void HandleExit(int code)
    {
    	final int Status_WarmReboot = 0x8F;
    	final int Status_ColdReboot = 0x81;

    	Runnable cb = null;
 
    	Log.d("AROS", "AROS process exited with code " + code);

    	// Who knows in which thread context we are here...
    	// In order to get rid of problems, we post a restart call on UI thread using a handler.
    	switch (code)
    	{
    	case Status_WarmReboot:
    		cb = new WarmReboot();
    		break;
    	
    	case Status_ColdReboot:
    		cb = new ColdReboot();
    		break;

    	default:
    		System.exit(code);
    		break;
    	}

    	handler.post(cb);
    }

    public void UpdateBitmap(Bitmap dest, BitmapData src, int x, int y, int width, int height)
    {
    	if (Bitmap.Buffer != null)
    	{
    		// Fallback for poor Android 2.1 and below users...
    		// It relies on the fact that Bitmap object and AROS bitmap sizes are
    		// no way different, including "shadow pixels" (padding up to 16).
    		// This means that - sorry, but no accelerated scrolling here...
    		// Anyway, this is slow enough even without scrolling... We can't refresh a portion.
    		// Only the whole bitmap...
    		Bitmap.Buffer.rewind();
    		dest.copyPixelsFromBuffer(Bitmap.Buffer);
    	}
    	else
    	{
    		// A quick native method, utilizing Android 2.2+ native Bitmap API
        	GetBitmap(dest, src.Address, x, y, width, height, src.BytesPerRow);
    	}
    }

	private void DoCommand(int cmd, int[] params)
	{
		Class<?> ActivityClass;

		switch (cmd)
		{
		case cmd_Query:
			Log.d("AROS", "cmd_Query(" + params[0] + ")");

			Server.ReplyCommand(cmd, DisplayWidth, DisplayHeight, TitlebarSize, Orientation);
			break;

		case cmd_Show:
			Log.d("AROS", "cmd_Show(" + params[0] + ", " + params[6] + ", " + params[7] + ")");

			if (params[7] == 0)
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
				Bitmap.Address     = params[7];

				if (!HaveNativeGraphics())
					Bitmap.Buffer = MapMemory(Bitmap.Address, Bitmap.BytesPerRow * Bitmap.Height);
			}

			switch (params[6])
			{
				// These two classes have their orientations locked in AndroidManifest.xml.
				// This way we can prevent unwanted screen rotation. Since AROS currently has no rotation
				// support, AROS programs can't re-layout them on the fly because of resolution change
				// (x:y becoming y:x).
			case Configuration.ORIENTATION_PORTRAIT:
				ActivityClass = PortraitActivity.class;
				break;

			case Configuration.ORIENTATION_LANDSCAPE:
				ActivityClass = LandscapeActivity.class;
				break;

			default:
				ActivityClass = null;
				break;
			}

			if ((ui != null) && (ActivityClass != null) && (ui.getClass() != ActivityClass))
			{
				// If we want to chage the orientation, and the UI is running, we need to finish current activity first.
				// Setting 'ui' to null wull automatically trigger the part below.
				Log.d("AROS", "Orientation change needed, closing current activity");
				ui.finish();
				ui = null;
			}

			if (ui == null)
			{
				if (ActivityClass != null)
				{
					// If we have no interface, we create a new one, with requested orientation.
					// One more effect of this two-step mechanism: if AROS is hidden in the background,
					// but some program wants to open a screen, AROS will pop up.
					Intent myIntent = new Intent(this, ActivityClass);

					myIntent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
					Log.d("AROS", "Starting up activity "+ ActivityClass);
					startActivity(myIntent);

					// Activity will pick up our Bitmap when created
				}
			}
			else
			{
				// Just signal the view to update
				ui.Show(params[0], Bitmap);
			}
			Server.ReplyCommand(cmd);
			break;

		case cmd_Update:
			if (ui != null)
			{
				BitmapView view = ui.GetBitmap(params[0]);

				view.Update(Bitmap, params[1], params[2], params[3], params[4]);
			}
			// This command doesn't need a reply
			break;

		case cmd_Alert:
			Log.d("AROS", "Alert code " + params[0]);

			byte[] textBuf = GetString(params[1]);
			String text = new String(textBuf);

			ui.DisplayAlert(params[0], text);
			break;

		case cmd_Hello:
			Log.d("AROS", "Hello from client version " + params[0]);
			if (params[0] == PROTOCOL_VERSION)
			{
				Server.ReplyCommand(cmd);
				break;
			}
			// Wrong protocol version, fallthrough

		default:
			Log.d("AROS", "Unknown command " + cmd);

			Server.ReplyCommand(cmd_Nak, cmd);
			break;
		}
	}

	public void ReportMouse(int x, int y, int action)
	{
		Server.ReplyCommand(cmd_Mouse, x, y, action);
	}

	public void ReportTouch(int x, int y, int action)
	{
		Server.ReplyCommand(cmd_Touch, x, y, action);
	}

	public void ReportKey(int code, int flags)
	{
		Server.ReplyCommand(cmd_Key, code, flags);
	}

	public void Resume()
	{
		int res = Kill(SIGUSR2);

		Log.d("AROS", "Resuming process, result: " + res);

	}

	public void Quit()
	{
		Kill(SIGTERM);
		System.exit(0);
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

    class WarmReboot implements Runnable
    {
    	public void run()
    	{
    		AROSBootstrap.this.Reset();
    		AROSBootstrap.this.WarmBoot();
    	}
    }

    class ColdReboot implements Runnable
    {
    	public void run()
    	{
    		AROSBootstrap.this.Reset();
    		AROSBootstrap.this.ColdBoot();
    	}
    }

    static
    {
        System.loadLibrary("AROSBootstrap");
    }

	// libAROSBootstrap native methods
    private native int Load(String dir);
    private native int Kick(FileDescriptor rfd, FileDescriptor wfd);
    private native int Kill(int signal);
    private native ByteBuffer MapMemory(int addr, int size);
    private native byte[] GetString(int addr);
    private native int GetBitmap(Bitmap obj, int addr, int x, int y, int width, int height, int bytesPerLine);
    private native boolean HaveNativeGraphics();
}
