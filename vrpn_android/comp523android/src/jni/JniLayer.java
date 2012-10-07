package jni;

import android.util.Log;

/**
 * Represents the connection between the Java user-interface code and the C++ VRPN code
 * @author Eric Boren
 *
 */
public class JniLayer 
{
    static 
    {
        try 
        {
            Log.i("JNI", "Trying to load libvrpn.so");
            System.loadLibrary("vrpn");   
        }
        catch (UnsatisfiedLinkError ule) 
        {
            Log.e("JNI", "WARNING: Could not load libvrpn.so");
        }
    }
	
	private static final String TAG = "JNI Layer";
	private long cppPointer;
	private int numButtons;
	private int [] analogs;
	private boolean running = false;
	private int port;
	
	/**
	 * Instantiates a new JniLayer, for communication between this Android app and the VRPN network.
	 * Should only be called from JniBuilder.
	 * 
	 * @param analogs - an array representing the number of channels in each analog server
	 * @param numButtons - the number of buttons used by the app
	 * @param port - the port number to use for connections
	 */
	public JniLayer(int[] analogs, int numButtons, int port)
	{
		this.numButtons = numButtons;
		this.analogs = analogs;
		this.port = port;
	}
	
	/**
	 * Instantiates VRPN servers on the C++ side and opens a connection.
	 */
	public void start()
	{
		if (!this.running)
		{
			this.cppPointer = jni_layer_initialize(this.analogs, this.numButtons, this.port);
			Log.i(TAG, "Created JniLayer at address: " + this.cppPointer);
			if (this.cppPointer != 0)
			{
				this.running = true;
			}
		}
	}
	
	/**
	 * Deconstructs all objects on the C++ side and closes the connection.
	 * This is necessary any time the app loses focus.
	 */
	public void stop()
	{
		if (this.running)
		{
			jni_layer_kill(this.cppPointer);
			this.running = false;
			this.cppPointer = 0L;
		}
	}
	
	/**
	 * Is the Android server on the C++ side instantiated?
	 * @return - whether or not the VRPN Android Server is up and running
	 */
	public boolean isRunning()
	{
		return this.running;
	}
	
	/**
	 * Calls the mainloop() method on the VRPN Android Server.  This initiates
	 * the sending of data to the client, and should be called at least once 
	 * every three seconds in order to prevent the client from complaining, even
	 * if no data has changed.
	 */
	public void mainLoop()
	{
		if (this.running)
		{
			jni_layer_mainloop(cppPointer);
		}
	}
	
	/**
	 * Sets the status (up or down) of a given button.  True for button down, false for button up.
	 * @param buttonId - the button whose status to update
	 * @param state - whether to set the button's status to pressed (true) or not pressed (false)
	 */
	public void updateButtonStatus(int buttonId, boolean state)
	{
		if (this.running)
		{
			jni_layer_set_button(cppPointer, buttonId, (state == true ? 1 : 0));
			mainLoop();
		}
	}
	
	/**
	 * Sets the value for the specified channel in the specified analog server.
	 * 
	 * @param analogId - The identifier of the analog server to update
	 * @param channel - Which channel (or dimension) of the analog to pass the value
	 * @param val - The new value
	 */
	public void updateAnalogVal(int analogId, int channel, float val)
	{
		if (this.running)
		{
			jni_layer_set_analog(cppPointer, analogId, channel, val);
			mainLoop();
		}
	}
	
	
	/**
	 * Forces a data report to be sent from the given analog server, even if the value has not changed.
	 * 
	 * @param analogId - The identifier of the analog server for which to report a value
	 */
	public void reportAnalogChg(int analogId)
	{
		if (this.running)
		{
			jni_layer_report_analog_chg(cppPointer, analogId);
			mainLoop();
		}
	}
	
	/**
	 * Returns the port number which is being used.
	 * 
	 * @return port - the port which is in use
	 */
	public int getPort()
	{
		return port;
	}
	
	/*
	 * These are JNI calls to the corresponding C++ objects.
	 */
    private native long jni_layer_initialize(int[] numAnalogChannels, int numButtons, int port);
    private native void jni_layer_mainloop(long pointer);
    private native void jni_layer_set_button(long pointer, int buttonId, int state);
    private native void jni_layer_set_analog(long pointer, int analogId, int channel, float val);
    private native void jni_layer_report_analog_chg(long pointer, int analogId);
    private native void jni_layer_kill(long pointer);
}
