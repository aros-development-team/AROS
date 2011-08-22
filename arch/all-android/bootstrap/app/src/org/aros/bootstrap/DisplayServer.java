package org.aros.bootstrap;

import java.io.FileDescriptor;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.IntBuffer;

import android.os.Handler;
import android.util.Log;


public class DisplayServer extends Thread
{
	private static final int BufferSize = 256;

	private Handler handler;
	private FileInputStream DisplayPipe;
	private FileOutputStream InputPipe;
	private AROSBootstrap main;
	private ByteBuffer rxbuf;
	private IntBuffer rxdata;
	private ByteBuffer txbuf;
	private IntBuffer txdata;

	public DisplayServer(AROSBootstrap parent, FileDescriptor displayfd, FileDescriptor inputfd)
	{
		main        = parent;
		handler     = new Handler();
		DisplayPipe = new FileInputStream(displayfd);
		InputPipe   = new FileOutputStream(inputfd);

		// Allocate buffers only once. This helps to speed up the server.
		rxbuf = ByteBuffer.allocate(BufferSize);
		txbuf = ByteBuffer.allocate(BufferSize);
		rxbuf.order(ByteOrder.nativeOrder());
		txbuf.order(ByteOrder.nativeOrder());
		rxdata = rxbuf.asIntBuffer();
		txdata = txbuf.asIntBuffer();
	}

	public void ReplyCommand(int cmd, int... response)
	{
		int len = response.length + 2;
 
        txdata.rewind();
        txdata.put(cmd);
        txdata.put(response.length);
        txdata.put(response);
 
        try
        {
			InputPipe.write(txbuf.array(), 0, len * 4);
		}
        catch (IOException e)
        {
        	Log.d("AROS.Server", "Error writing input pipe");
        	System.exit(0);
		}
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
		int res;
		int rawlen = len * 4;

		try
		{
			res = DisplayPipe.read(rxbuf.array(), 0, rawlen);
		}
		catch (IOException e)
		{
			res = -1;
		}
	
		if (res != rawlen)
		{
			Log.v("AROS.Server", "Error reading pipe (wanted " + len + ", got " + res + ")");
			System.exit(0);
		}

		int[] data = new int[len];

		rxdata.rewind();
		rxdata.get(data);
		return data;
	}
}
