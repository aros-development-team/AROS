package org.aros.bootstrap;

import android.app.Activity;
import android.content.res.Configuration;
import android.os.Bundle;
import android.util.Log;

/*
 * This class is needed to trick Android OS and lock up screen orientation.
 * Android, unlike iOS, does not allow to restrict the activity to certain
 * orientations in runtime. This restriction can only be set in AndroidManifest.xml.
 * In order to work around this, we have two classes: LandscapeActivity and PortraitActivity.
 * Each of them is locked to their respective orientation in the manifest, and both of them
 * are subclasses of AROSActivity, which actually does the job.
 * Subclasses contain no code, their only purpose is to be described in the manifest.
 * The third class, this one, serves as a proxy. This activity is the first to be created,
 * its role is to detect current orientation, and select, which activity to launch next.
 * After this the application will use only one of these two activities, until restarted.
 * 
 * TODO: Perhaps it's possible to measure both screen sizes (taking Android titlebar into account)
 * Then it will be possible to teach Android display driver to select one of two screen modes
 * (portrait and landscape)
 */

public class OrientationDetector extends Activity
{
	@Override
	public void onCreate(Bundle savedInstanceState)
	{
		super.onCreate(savedInstanceState);

		AROSBootstrap app = (AROSBootstrap) getApplication();

		// We can't change orientation while we are running
		if (app.ActivityClass == null)
		{
			int o = getResources().getConfiguration().orientation;

			Log.d("AROS.UI", "Orientation set to " + o);
			app.ActivityClass = (o == Configuration.ORIENTATION_PORTRAIT) ? PortraitActivity.class : LandscapeActivity.class;
		}

    	app.Foreground();
	}
	
	@Override
	public void onPause()
	{
		/*
		 * This little trick makes the activity to completely go away when we leave it
		 * and run the main activity.
		 * Without this OrientationDetector stays laying around in paused state, and when
		 * AROS exits, it takes over and forces a restart, which we don't need.
		 */
		super.onPause();

		finish();
	}
}
