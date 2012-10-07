package edu.unc.cs.vrpn;

import java.io.IOException;
import java.net.InetAddress;
import java.net.NetworkInterface;
import java.net.ServerSocket;
import java.net.SocketException;
import java.util.ArrayList;
import java.util.Enumeration;
import java.util.List;
import java.util.Timer;
import java.util.TimerTask;

import jni.JniLayer;
import android.app.Activity;
import android.app.ProgressDialog;
import android.content.Context;
import android.hardware.Sensor;
import android.hardware.SensorEventListener;
import android.hardware.SensorManager;
import android.os.Bundle;
import android.os.Handler;
import android.os.PowerManager;
import android.os.PowerManager.WakeLock;
import android.util.Log;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.View;
import android.view.Window;
import android.view.View.OnTouchListener;
import android.widget.Button;
import android.widget.SeekBar;
import android.widget.TextView;

/**
 * The remote activity for the Android VRPN controller. This class is not very
 * pretty, but is the glue which holds the server together.
 * 
 * @author UNC VRPN Team
 * 
 */
public class Main extends Activity implements OnTouchListener
{
	/**
	 * The interval (in milliseconds) between calls to vrpn's mainloop()
	 * functions.  Calling these on a timer prevents the client application
	 * from complaining that it hasn't heard from the Android server.
	 */
	public static final int IDLE_INTERVAL = 1000;

	/**
	 * The Tag used by this class for logging
	 */
	static final String TAG = "VRPN";

	/**
	 * The buttons on the default on-screen interface
	 */
	private ArrayList<Button> buttons;

	/**
	 * The horizontal seek bar of the default on-screen interface
	 */
	private SeekBar seekBar1;

	/**
	 * The text area where multitouch coordinates are displayed
	 */
	private TextView multitouchReport;

	/**
	 * The text area where the device's IP address will be displayed
	 */
	private TextView ipAddressReport;

	/**
	 * The text area where the device's port number will be displayed
	 */
	private TextView portNumberReport;
	
	/**
	 * The text area where the Seek Bar data will be displayed
	 */
	private TextView seekbarReport;

	/**
	 * The text area where the Accelerometer data will be displayed
	 */
	private TextView accelerometerReport;
	
	/**
	 * The listener for the hardware accelerometer
	 */
	private SensorEventListener listener;

	/**
	 * The timer that calls mainloop to persist the connection
	 */
	private Timer idleTimer;

	/**
	 * Gets the sensor manager in use by the activity.
	 */
	private SensorManager sensorManager;

	/**
	 * The connection between Java and the C++ world of VRPN, using Android's Java Native
	 * Interface, called the JNI
	 */
	private JniLayer jniLayer;

	/**
	 * The JNI creator object
	 */
	private JniBuilder builder;

	/**
	 * Analog channel for multitouch data
	 */
	private JniBuilder.Analog multitouch;
	
	/**
	 * Analog channel for accelerometer data
	 */
	private JniBuilder.Analog accelerometer;

	
	private int multitouchAnalogNumber;

	/**
	 * This prevents the phone from sleeping while the app is running. Is acquired in the
	 * onResume event and released during onPause
	 */
	private WakeLock wakeLock;

	/**
	 * Listener for the Seek Bar data
	 */
	private SeekBar.OnSeekBarChangeListener seekBarChangeListener;
	
	private ProgressDialog waitingForConnectionDialog;

	@Override
	public void onCreate(Bundle savedInstanceState)
	{
		super.onCreate(savedInstanceState);
		requestWindowFeature(Window.FEATURE_NO_TITLE);
		super.setContentView(R.layout.main);

		TextView view = (TextView) findViewById(R.id.touchpad);
		view.setOnTouchListener(this);

		this.multitouchReport = (TextView) findViewById(R.id.onScreenLog);
		this.ipAddressReport = (TextView) findViewById(R.id.ipAddress);
		this.portNumberReport = (TextView) findViewById(R.id.portNumber);
		this.accelerometerReport = (TextView) findViewById(R.id.accelerometerReport);
		this.seekbarReport = (TextView) findViewById(R.id.textView_track);

		this.buttons = new ArrayList<Button>();
		this.buttons.add((Button) findViewById(R.id.Button0));
		this.buttons.add((Button) findViewById(R.id.Button1));

		JniBuilder.Analog slider = new JniBuilder.Analog("Slider" , 1);
		this.multitouch = new JniBuilder.Analog("Multitouch" , 2);
		this.accelerometer = new JniBuilder.Analog("Accelerometer" , 3);

		this.builder = new JniBuilder();
		this.builder.add(this.multitouch , slider , this.accelerometer);
		this.builder.add(new JniBuilder.Button("Search") , new JniBuilder.Button() , new JniBuilder.Button());

		// the JNI Layer must be instantiated before the ButtonTouchListeners are created
		this.jniLayer = builder.toJni();

		this.multitouchAnalogNumber = this.builder.getAnalogIndex(this.multitouch);
		this.seekBar1 = (SeekBar) findViewById(R.id.textView_seekBar_01);
		this.seekBarChangeListener = new VrpnSeekBarChangeListener(this.builder , slider , this);
		this.seekBar1.setOnSeekBarChangeListener(this.seekBarChangeListener);

		this.setupButtons(this.buttons);

		PowerManager pm = (PowerManager) super.getSystemService(Context.POWER_SERVICE);
		this.wakeLock = pm.newWakeLock(PowerManager.FULL_WAKE_LOCK , Main.TAG);
	}

	@Override
	public boolean onKeyDown(int keyCode, KeyEvent event)
	{
		if (keyCode == KeyEvent.KEYCODE_SEARCH)
		{
			String message = "Search button "
					+ ((event.getFlags() & KeyEvent.FLAG_LONG_PRESS) != 0 ? "long " : "") + "pressed";
			Log.i(Main.TAG , message);
			this.logText(message);
			this.jniLayer.updateButtonStatus(0 , true);
			return true;
		}

		return super.onKeyDown(keyCode , event);
	}

	@Override
	public boolean onKeyUp(int keyCode, KeyEvent event)
	{
		if (keyCode == KeyEvent.KEYCODE_SEARCH)
		{
			String message = "Search button released";
			Log.i(Main.TAG , message);
			this.logText(message);
			this.jniLayer.updateButtonStatus(0 , false);
			// finish();
			//android.os.Process.killProcess(android.os.Process.myPid());
			return true;
		}
		return super.onKeyUp(keyCode , event);
	}

	@Override
	public void onPause()
	{
		super.onPause();

		if (this.idleTimer != null)
		{
			this.idleTimer.cancel();
		}

		if (this.listener != null)
		{
			this.sensorManager.unregisterListener(this.listener);
		}

		this.sensorManager = null;

		// disconnect from the JNI-VRPN bridge, and release the WakeLock to
		// allow the phone to sleep
		this.wakeLock.release();
		jniLayer.stop();
	}

	@Override
	public void onResume()
	{
		super.onResume();
		this.setupVrpn();
	}
	
	class WaitingForConnectionThread extends Thread
	{
		Handler handler;
		
		public WaitingForConnectionThread(Handler handler)
		{
			super();
			this.handler = handler;
		}
		
		public void run()
		{
			// this is the hack to allow reconnection to the client
			// after a disconnect. the hack polls for a open port,
			// and loops until the port is open/free'd by GC.
			boolean stop = false;
			ServerSocket server = null;
			while (!stop)
			{
				try
				{
					server = new ServerSocket(jniLayer.getPort());
					Log.i(TAG, "Connection found!");
					server.close();
					stop = true;
				}
				catch(IOException e)
				{
					if (waitingForConnectionDialog != null)
					{
						
					}
					Log.v(TAG, "Connecting...");
					try {
						Thread.sleep(1000);
					} catch (InterruptedException e1) {

					}
				}
			}
			
			jniLayer.start();
			try {
				Thread.sleep(250);
			} catch (InterruptedException e) {
			
			}
			handler.post(new GotConnectionThread());
		}
	}
	
	class GotConnectionThread extends Thread {
		public void run()
		{
			if (waitingForConnectionDialog != null)
			{
				waitingForConnectionDialog.dismiss();
			}
		}
	}


	/**
	 * Creates/recreates the JniLayer and attaches the phone's sensors.
	 * The content of this method was moved from onResume()
	 */
	public void setupVrpn()
	{
		//////// This bit of code opens a new thread in case we need to wait for Dalvik to free up the port //////////
		waitingForConnectionDialog = ProgressDialog.show(Main.this , "" , "Connecting..." , true);
		Handler handler = new Handler();
		new WaitingForConnectionThread(handler).start();
		//////////////////////////////////////////////////////////////////////////////////////////////////////////////

		
		this.sensorManager = (SensorManager) this.getSystemService(SENSOR_SERVICE);
		List<Sensor> sensors = this.sensorManager.getSensorList(Sensor.TYPE_ACCELEROMETER);
		if (!sensors.isEmpty())
		{
			this.listener = new AccelerometerListener(this.builder , this.accelerometer , this);
			boolean accelerationSupported = this.sensorManager.registerListener(this.listener ,
					sensors.get(0) , SensorManager.SENSOR_DELAY_UI);
			if (!accelerationSupported)
			{
				Log.w(Main.TAG , "Accelerometer offline");
				this.multitouchReport.setText("Accelerometer offline");
			}
			else
			{
				this.multitouchReport.setText("Accelerometer online");
			}
		}
		else
		{
			Log.w(Main.TAG , "Accelerometer not found");
			this.multitouchReport.setText("Accelerometer not found");
		}
		
		Log.i(TAG , "Retrieving IP Address...");

		// load the IP Address string from
		this.ipAddressReport.setText(String.format(super.getString(R.string.ip_address) , this.getIpAddress()));

		this.portNumberReport.setText(String.format(super.getString(R.string.port_number) ,
				Integer.toString(jniLayer.getPort())));
		
		this.wakeLock.acquire();

		// setup the idle timer
		this.idleTimer = new Timer();
		this.idleTimer.schedule(new TimerTask() {
			@Override
			public void run()
			{
				sendIdle();
			}

		} , IDLE_INTERVAL, IDLE_INTERVAL);
		
		Log.i(TAG, "setupVrpn finished");
	}
	
	@Override
	public boolean onTouch(View v, MotionEvent event)
	{
		this.transmitEvent(event);
		this.setMultitouchReport(event);
		return true;
	}

	/**
	 * Calls the vrpn connection's mainLoop to send an empty message to persist the
	 * connection.
	 */
	public void sendIdle()
	{
		this.jniLayer.mainLoop();
	}

	/**
	 * Gets IP address of device. Copied from
	 * http://www.droidnova.com/get-the-ip-address-of-your-device,304.html
	 * 
	 * @return The IP address of the Droid
	 */
	private String getIpAddress()
	{
		try
		{
			for (Enumeration<NetworkInterface> en = NetworkInterface.getNetworkInterfaces(); en.hasMoreElements();)
			{
				NetworkInterface intf = en.nextElement();
				for (Enumeration<InetAddress> enumIpAddr = intf.getInetAddresses(); enumIpAddr.hasMoreElements();)
				{
					InetAddress inetAddress = enumIpAddr.nextElement();
					if (!inetAddress.isLoopbackAddress()) { return inetAddress.getHostAddress().toString(); }
				}
			}
		}
		catch(SocketException ex)
		{
			Log.e(TAG , ex.toString());
		}
		return null;
	}

	private void transmitEvent(MotionEvent event)
	{
		this.jniLayer.updateAnalogVal(this.multitouchAnalogNumber , 0 , event.getX());
		this.jniLayer.updateAnalogVal(this.multitouchAnalogNumber , 1 , event.getY());
		this.jniLayer.reportAnalogChg(this.multitouchAnalogNumber);
	}

	/**
	 * Update the text on the screen to show the x and y coordinate
	 * Multitouch data
	 * 
	 * @param event
	 *            The event which updates the multitouch report
	 */
	private void setMultitouchReport(MotionEvent event)
	{
		String x;
		float xVal = event.getX();
		if (xVal >= 0 && xVal < 10)
		{
			x = "  " + xVal;
		}
		else if (xVal >= 10 && xVal < 100)
		{
			x = " " + xVal;
		}
		else if (xVal >= 100)
		{
			x = "" + xVal;
		}
		else
		{
			x = "  0.0";
		}
		
		String y;
		float yVal = event.getY();
		if (yVal >= 0 && yVal < 10)
		{
			y = "  " + yVal;
		}
		else if (yVal >= 10 && yVal < 100)
		{
			y = " " + yVal;
		}
		else if (yVal >= 100)
		{
			y = "" + yVal;
		}
		else
		{
			y = "  0.0";
		}
		
		this.logText("x: " + x + ", y: " + y);
	}

	/**
	 * Updates text to be displayed in the multitouch textview 
	 * 
	 * @param message the message to display on the screen
	 */
	private void logText(String message)
	{
		this.multitouchReport.setText(message);
	}

	/**
	 * This method sets up and initializes all the button event listeners
	 * 
	 * @param buttons list of buttons to be initialized
	 */
	private void setupButtons(List<Button> buttons)
	{
		for (int i = 1, len = buttons.size(); i <= len; i++)
		{
			ButtonListener bl = new ButtonListener(this.builder.getButton(i) , i , this.jniLayer);
			buttons.get(i - 1).setClickable(true);
			buttons.get(i - 1).setOnTouchListener(bl);
		}
	}
	/**
	 * Updates text to be displayed in the accelerometerReport
	 * textview. 
	 * 
	 * @param message the message to display on the screen
	 */
	void logAccelerometer(String message)
	{
		this.accelerometerReport.setText(message);
	}
	/**
	 * Updates text to be displayed in the seekbarReport textview.
	 * 
	 * @param message the message to display on the screen
	 */
	void logSlider(String message)
	{
		this.seekbarReport.setText(message);
	}
}