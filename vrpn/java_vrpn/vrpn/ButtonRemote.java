package vrpn;
import java.util.*;

public class ButtonRemote extends VRPNDevice implements Runnable
{
	
	//////////////////
	// Public structures and interfaces
	
	public class ButtonUpdate
	{
		public Date msg_time = new Date( );
		public int button = 0; //Which button.  Starts from 0
		public int state = 0; //0 for off, 1 for on
	}
	
	public interface ButtonChangeListener
	{
		public void buttonUpdate( ButtonUpdate u, ButtonRemote button );
	}

	// end of the public structures and interfaces
	///////////////////
	
	///////////////////
	// Public methods
	
	public ButtonRemote( String name, String localInLogfileName, String localOutLogfileName,
						 String remoteInLogfileName, String remoteOutLogfileName ) 
		throws InstantiationException
	{
		super( name, localInLogfileName, localOutLogfileName, remoteInLogfileName, remoteOutLogfileName );
	}
	
	public synchronized void addButtonChangeListener( ButtonChangeListener listener )
	{
		changeListeners.addElement( listener );
	}
	
	/**
	 * @return true on success; false on failure
	 */
	public synchronized boolean removeButtonChangeListener( ButtonChangeListener listener )
	{
		return changeListeners.removeElement( listener );
	}
	
	/**
	 * Stops the button device thread
	 */
	public void stoppedRunning( )
	{
		changeListeners.removeAllElements( );
		synchronized( downInVrpnLock )
		{
			this.shutdownButton( );
		}
	}

	// end public methods
	////////////////////////
	
	
	////////////////////////
	// Protected methods
	//
	
	/**
	 * Should be called only by mainloop(), a native method which is itself
	 * synchronized.  By extension, this method is synchronized (in that, for
	 * a given ButtonRemote object, this method can only be called from one
	 * thread at a time).
	 */
	protected void handleButtonChange( long tv_sec, long tv_usec, int button,
									   int state )
	{
		// putting the body of this function into a synchronized block prevents
		// other instances of ButtonRemote from calling listeners' buttonPositionUpdate
		// method concurrently.
		synchronized( notifyingChangeListenersLock )
		{
			ButtonUpdate b = new ButtonUpdate();
			b.msg_time.setTime( tv_sec * 1000 + (int) (tv_usec/1000.0) );
			b.button = button;
			b.state = state;
			
			// notify all listeners
			Enumeration e = changeListeners.elements( );
			while( e.hasMoreElements( ) )
			{
				ButtonChangeListener l = (ButtonChangeListener) e.nextElement( );
				l.buttonUpdate( b, this );
			}
		} // end synchronized( notifyingChangeListenersLock )
	} // end method handleButtonChange
	
	
	/**
	 * Initialize the native button object
	 * @param name The name of the button and host (e.g., <code>"Button0@myhost.edu"</code>).
	 * @return <code>true</code> if the button was connected successfully, 
	 *			<code>false</code> otherwise.
	 */
	protected native boolean init( String name, String localInLogfileName, 
								   String localOutLogfileName, String remoteInLogfileName,
								   String remoteOutLogfileName );

	/**
	 * This should only be called from the method finalize()
	 */
	protected native void shutdownButton( );
	
	protected native void mainloop( );
	
	// end protected methods
	///////////////////////
	
	///////////////////
	// data members
	
	protected Vector changeListeners = new Vector( );
	
	/**
	 * @see vrpn.TrackerRemote#notifyingChangeListenersLock
	 */
	protected final static Object notifyingChangeListenersLock = new Object( );

	
}


