
package vrpn;
import java.util.*;

public class AnalogOutputRemote extends VRPN implements Runnable
{
	
	//////////////////
	// Public structures and interfaces
	
	public static final int MAX_CHANNELS = 128;
	
	// end of the public structures and interfaces
	///////////////////
	
	///////////////////
	// Public methods
	
	/**
	 * @exception java.lang.InstantiationException
	 *		If the analogOutput could not be created because of problems with
	 *      its native code and linking.
	 */
	public AnalogOutputRemote( String name, String localInLogfileName, String localOutLogfileName,
						  String remoteInLogfileName, String remoteOutLogfileName ) 
		throws InstantiationException
	{
		try	
		{  
			synchronized( downInVrpnLock )
			{
				this.init( name, localInLogfileName, localOutLogfileName, 
						   remoteInLogfileName, remoteOutLogfileName );  
			}
		}
		catch( java.lang.UnsatisfiedLinkError e )
		{  
			System.out.println( "Error initializing remote analogOutput device " + name + "." );
			System.out.println( " -- Unable to find the right functions.  This may be a version problem." );
			throw new InstantiationException( e.getMessage( ) );
		}
		
		this.analogOutputThread = new Thread( this );
		this.analogOutputThread.start( );

	}
	
	public synchronized native boolean requestValueChange( int channel, double value );
	
	/**
	 * Sets the interval between invocations of <code>mainloop()</code>, which checks
	 * for and delivers messages.
	 * @param period The period, in milliseconds, between the beginnings of two
	 * consecutive message loops.
	 */
	public synchronized void setTimerPeriod( long period )
	{
		mainloopPeriod = period;
	}
	
	/**
	 * @return The period, in milliseconds.
	 */
	public synchronized long getTimerPeriod( )
	{
		return mainloopPeriod;
	}
	

	/**
	 * Stops the analogOutput thread
	 */
	public void stopRunning( )
	{
		keepRunning = false;
	}

	
	/**
	 * This should <b>not</b> be called by user code.
	 */
	public void run( )
	{
		while( keepRunning )
		{
			synchronized( downInVrpnLock )
			{
				this.mainloop( );
			}
			try { Thread.sleep( mainloopPeriod ); }
			catch( InterruptedException e ) { } 
		}
	}
	
	// end public methods
	////////////////////////
	
	
	////////////////////////
	// Protected methods
	//
		
	/**
	 * Initialize the native analogOutput object
	 * @param name The name of the analogOutput and host (e.g., <code>"Analog0@myhost.edu"</code>).
	 * @return <code>true</code> if the analogOutput was connected successfully, 
	 *			<code>false</code> otherwise.
	 */
	protected native boolean init( String name, String localInLogfileName, 
								   String localOutLogfileName, String remoteInLogfileName,
								   String remoteOutLogfileName );

	/**
	 * This should only be called from the method finalize()
	 */
	protected native void shutdownAnalogOutput( );
	
	protected synchronized native void mainloop( );
	
	protected void finalize( ) throws Throwable
	{
		keepRunning = false;
		while( analogOutputThread.isAlive( ) )
		{
			try { analogOutputThread.join( ); }
			catch( InterruptedException e ) { }
		}
		changeListeners.removeAllElements( );
		synchronized( downInVrpnLock )
		{
			this.shutdownAnalogOutput( );
		}
	}
	
	
	// end protected methods
	///////////////////////
	
	///////////////////
	// data members
	
	// this is used by the native code to store a C++ pointer to the 
	// native vrpn_Analog_Output_Remote object
	protected int native_analog_output = -1;
	
	// this is used to stop and to keep running the tracking thread
	// in an orderly fashion.
	protected boolean keepRunning = true;
	
	// the analogOutput thread
	Thread analogOutputThread = null;

	// how long the thread sleeps between checking for messages
	protected long mainloopPeriod = 100; // milliseconds

	
	/**
	 * @see vrpn.TrackerRemote#notifyingChangeListenersLock
	 */
	protected final static Object notifyingChangeListenersLock = new Object( );
	protected Vector changeListeners = new Vector( );

	
}
