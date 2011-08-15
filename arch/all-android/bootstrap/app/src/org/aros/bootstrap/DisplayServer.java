package org.aros.bootstrap;

import java.io.FileInputStream;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.IntBuffer;

import android.os.Handler;
import android.util.Log;


public class DisplayServer extends Thread
{
	public static final int cmd_Query =  1;
	public static final int cmd_Nak   = -1;

	private Handler handler;
	private FileInputStream DisplayPipe;
	private AROSBootstrap main;
	
	public DisplayServer(AROSBootstrap parent, FileInputStream pipe)
	{
		main = parent;
		handler = new Handler();
		DisplayPipe = pipe;
	}
	
	public void run()
	{
		Log.d("AROS.Server", "Display server started");

		for(;;)
		{
			int[] cmd  = ReadData(2);
			int[] args = ReadData(cmd[1]);

			AROSBootstrap.ServerCommand cmdObj = main.new ServerCommand(cmd[0], args);
			handler.post(cmdObj);
		}
	}

	private int[] ReadData(int len)
	{
		byte[]raw = new byte[len * 4];
		
		try
		{
			DisplayPipe.read(raw);
		}
		catch (IOException e)
		{
			Log.v("AROS.Server", "Error reading pipe");
			System.exit(0);
		}

		ByteBuffer bb = ByteBuffer.wrap(raw);
		bb.order(ByteOrder.nativeOrder());
		IntBuffer ib = bb.asIntBuffer();
		int[] data = new int[len];

		ib.get(data);
		return data;
	}
}
