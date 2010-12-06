
package vrpn;
import java.util.*;

public class AnalogRemote extends VRPNDevice implements Runnable
{
	
	//////////////////
	// Public structures and interfaces
	
	public static final int MAX_CHANNELS = 128;
	
	public class AnalogUpdate
	{
		public Date msg_time = new Date( );
		public double channel[];
	}
	
	public interface AnalogChangeListener
	{
		public void analogUpdate( AnalogUpdate a, AnalogRemote analog );
	}

	// end of the public structures and interfaces
	///////////////////
	
	///////////////////
	// Public methods
	
	/**
	 * @param name The name of the analog to connect to (e.g., Tracker0@localhost)
	 * @param localInLogfileName The name of a logfile to save incoming messages.  Use <code>
	 * null</code> if no such log is desired.
	 * @param localOutLogfileName The name of a logfile to save outgoing messages.  Use <code>
	 * null</code> if no such log is desired.
	 * @param remoteInLogfileName The name of a logfile in which the server <code>name</code>
	 * should save incoming messages.  Use <code>null</code> if no such log is desired.
	 * @param remoteOutLogfileName  The name of a logfile in which the server <code>name</code>
	 * should save outgoing messages.  Use <code>null</code> if no such log is desired.
	 * @exception java.lang.InstantiationException
	 *		If the analog could not be created because of problems with
	 *      its native code and linking.
	 */
	public AnalogRemote( String name, String localInLogfileName, String localOutLogfileName,
						  String remoteInLogfileName, String remoteOutLogfileName ) 
		throws InstantiationException
	{
		super( name, localInLogfileName, localOutLogfileName, remoteInLogfileName, remoteOutLogfileName );
	}
	
	
	public synchronized native int getNumActiveChannels( );
	public int getMaxActiveChannels( )
	{  return AnalogRemote.MAX_CHANNELS;  }

	public synchronized void addAnalogChangeListener( AnalogChangeListener listener )
	{
		changeListeners.addElement( listener );
	}
	
	
	public synchronized boolean removeAnalogChangeListener( AnalogChangeListener listener )
	{
		return changeListeners.removeElement( listener );
	}
	
	// end public methods
	////////////////////////
	
	
	////////////////////////
	// Protected methods
	//
	
	/**
	 * Should be called only by mainloop(), a native method which is itself
	 * synchronized.  By extension, this method is synchronized (in that, for
	 * a given AnalogRemote object, this method can only be called from one
	 * thread at a time).
	 */
	protected void handleAnalogChange( long tv_sec, long tv_usec, 
									   double[] channel )
	{
		// putting the body of this function into a synchronized block prevents
		// other instances of AnalogRemote from calling listeners' analogUpdate
		// method concurrently.
		synchronized( notifyingChangeListenersLock )
		{
			AnalogUpdate a = new AnalogUpdate();
			a.msg_time.setTime( tv_sec * 1000 + (int) (tv_usec/1000.0) );
			a.channel = (double[]) channel.clone( );
			
			// notify all listeners
			Enumeration e = changeListeners.elements( );
			while( e.hasMoreElements( ) )
			{
				AnalogChangeListener l = (AnalogChangeListener) e.nextElement( );
				l.analogUpdate( a, this );
			}
		} // end synchronized( notifyingChangeListenersLock )
	} // end method handleAnalogChange
	
	
	/**
	 * Initialize the native analog object
	 * @param name The name of the analog and host (e.g., <code>"Analog0@myhost.edu"</code>).
	 * @return <code>true</code> if the analog was connected successfully, 
	 *			<code>false</code> otherwise.
	 */
	protected native boolean init( String name, String localInLogfileName, 
								   String localOutLogfileName, String remoteInLogfileName,
								   String remoteOutLogfileName );

	/**
	 * This should only be called from the method finalize()
	 */
	protected native void shutdownAnalog( );
	
	protected native void mainloop( );
	
	/**
	 * Called when the thread is stopped/the device shuts down
	 */
	protected void stoppedRunning( )
	{
		changeListeners.removeAllElements( );
		synchronized( downInVrpnLock )
		{
			this.shutdownAnalog( );
		}
	}

	// end protected methods
	///////////////////////
	
	///////////////////
	// data members
	
	/**
	 * @see vrpn.TrackerRemote#notifyingChangeListenersLock
	 */
	protected final static Object notifyingChangeListenersLock = new Object( );
	protected Vector changeListeners = new Vector( );

	
}
