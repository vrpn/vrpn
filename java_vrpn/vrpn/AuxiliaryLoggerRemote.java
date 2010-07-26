package vrpn;

import java.util.Date;
import java.util.Enumeration;
import java.util.Vector;


public class AuxiliaryLoggerRemote extends VRPNDevice implements Runnable 
{

	/////////////////////////////////
	// Public structures and interfaces

	public class LoggingReport
	{
		public Date msg_time = new Date();
		public String localInLogfileName = "";
		public String localOutLogfileName = "";
		public String remoteInLogfileName = "";
		public String remoteOutLogfileName = "";
	}
	
	public interface LoggingReportListener
	{
		public void loggingReport( LoggingReport u, AuxiliaryLoggerRemote logger );
	}
		
	// end of the public structures and interfaces
	//////////////////////////////////

	
	////////////////////////////////
	// Public methods
	//
	
	/**
	 * @param name The name of the tracker to connect to (e.g., Logger0@somehost.com)
	 * @exception java.lang.InstantiationException
	 *		If the tracker could not be created because of problems with
	 *      its native code and linking.
	 */
	public AuxiliaryLoggerRemote( String name ) 
		throws InstantiationException
	{
		super( name, null, null, null, null );
	}
	
	
	public synchronized native boolean 
		sendLoggingRequest( String localInLogfileName, String localOutLogfileName,
							String remoteInLogfileName, String remoteOutLogfileName );
	
	
	public synchronized native boolean sendLoggingStatusRequest( );

	
	public synchronized void addLoggingReportListener( LoggingReportListener listener )
	{
		listeners.addElement( listener );
	}
	
	
	/**
	 * @return true on success; false on failure
	 */
	public synchronized boolean removeLoggingReportListener( LoggingReportListener listener )
	{
		return listeners.removeElement( listener );
	}
	
	
	// end public methods
	////////////////////////
	
	
	////////////////////////
	// Protected methods
	//
	
	/**
	 * Should be called only by mainloop(), a native method which is itself
	 * synchronized.  By extension, this method is synchronized (in that, for
	 * a given LoggerRemote object, this method can only be called from one
	 * thread at a time).
	 */
	protected void handleLoggingReport( long tv_sec, long tv_usec, 
										String localIn, String localOut, 
										String remoteIn, String remoteOut )
	{
		synchronized( notifyingListenersLock )
		{
			LoggingReport r = new LoggingReport();
			r.msg_time.setTime( tv_sec * 1000 + (int) (tv_usec/1000.0) );
			r.localInLogfileName = localIn;
			r.localOutLogfileName = localOut;
			r.remoteInLogfileName = remoteIn;
			r.remoteOutLogfileName = remoteOut;
			
			// notify all listeners
			Enumeration e = listeners.elements( );
			while( e.hasMoreElements( ) )
			{
				LoggingReportListener l = (LoggingReportListener) e.nextElement( );
				l.loggingReport( r, this );
			}
		} // end synchronized block
	} // end handleLoggerReport
	
	
	protected void stoppedRunning() 
	{
		listeners.removeAllElements();
		synchronized( downInVrpnLock )
		{
			this.shutdownAuxiliaryLogger();
		}
	}
	
	
	protected native void shutdownAuxiliaryLogger();
	
	
	/**
	 * This should only be called from the method run()
	 */
	protected native void mainloop();

	
	/**
	 * Initialize the native tracker object
	 * @param name The name of the tracker and host (e.g., <code>"Tracker0@myhost.edu"</code>).
	 * @param localInLogfileName This will be ignored (as will all the *LogfileName params).
	 * @return <code>true</code> if the tracker was connected successfully, 
	 *			<code>false</code> otherwise.
	 */
	protected native boolean init( String name, String localInLogfileName,
									String localOutLogfileName, String remoteInLogfileName,
									String remoteOutLogfileName );
	// end protected methods
	///////////////////////
		
	///////////////////
	// data members
	
	protected Vector listeners = new Vector( );

	/**
	 * these notifying*ListenersLock variables are used to ensure that multiple
	 * TrackerRemote objects running in multiple threads don't call the 
	 * trackerChangeUpdate, et al, method of some single object concurrently.
	 * For example, the handleTrackerChange(...) method, which is invoked from native 
	 * code, gets a lock on the notifyingChangeListenersLock object.  Since that object
	 * is static, all other instances of TrackerRemote must wait before notifying 
	 * their listeners and completing their handleTrackerChange(...) methods.
	 * They are necessary, in part, because methods in an interface can't be declared
	 * synchronized (and the semantics of the keyword 'synchronized' aren't what's
	 * wanted here, anyway -- we want synchronization across all instances, not just a 
	 * single object).
	 */
	protected final static Object notifyingListenersLock = new Object( );
		

}
