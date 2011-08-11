package org.aros.bootstrap;

import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;

import android.os.Handler;
import android.util.Log;

public class DisplayServer extends Thread
{
	private static final int cmd_Query =  1;
	private static final int cmd_Nak   = -1;

	public DisplayServer(AROSBootstrap parent)
	{
		handler = new Handler();
		pipeDir = parent.getCacheDir();
		context = parent;
	}

	public void run()
	{
		Log.d("AROSDisplay", "Display server started");

		try
		{
			File pipe = new File(pipeDir, "AROS.display");
			DataInputStream in = new DataInputStream(new FileInputStream(pipe));
			DataOutputStream out = new DataOutputStream(new FileOutputStream(pipe));

			Log.d("AROSDisplay", "Opened pipe, waiting for command...");

			int cmd = in.readInt();
			
			switch (cmd)
			{
			case cmd_Query:
				// id parameter is reserved for future, to support multiple displays
				int id = in.readInt();
				Log.d("AROSDisplay", "cmd_Query( " + id + ")");

				DisplayView d = context.GetDisplay();
				out.writeInt(cmd);
				out.writeInt(d.Width);
				out.writeInt(d.Height);

			default:
				Log.d("AROSDisplay", "Unknown command " + cmd);
				out.writeInt(cmd_Nak);
			}
		}
		catch(IOException e)
		{
			Log.d("AROSDisplay", "Pipe I/O error: " + e);
		}
	}

	private Handler handler;
	private File pipeDir;
	private AROSBootstrap context;
}
