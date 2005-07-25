
package vrpn;
import java.util.*;


public class PoserRemote extends VRPN implements Runnable
{
	
	////////////////////////////////
	// Public methods
	//
	
	/**
	 * @param name The name of the poser to connect to (e.g., Poser0@localhost)
	 * @param localInLogfileName The name of a logfile to save incoming messages.  Use <code>
	 * null</code> if no such log is desired.
	 * @param localOutLogfileName The name of a logfile to save outgoing messages.  Use <code>
	 * null</code> if no such log is desired.
	 * @param remoteInLogfileName The name of a logfile in which the server <code>name</code>
	 * should save incoming messages.  Use <code>null</code> if no such log is desired.
	 * @param remoteOutLogfileName  The name of a logfile in which the server <code>name</code>
	 * should save outgoing messages.  Use <code>null</code> if no such log is desired.
	 * @exception java.lang.InstantiationException
	 *		If the poser could not be created because of problems with
	 *      its native code and linking.
	 */
	public PoserRemote( String name, String localInLogfileName, String localOutLogfileName,
						  String remoteInLogfileName, String remoteOutLogfileName ) 
		throws InstantiationException
	{
		super( name, localInLogfileName, localOutLogfileName, remoteInLogfileName, remoteOutLogfileName );
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
			System.out.println( "Error initializing remote poser " + name + "." );
			System.out.println( " -- Unable to find the right functions.  This may be a version problem." );
			throw new InstantiationException( e.getMessage( ) );
		}
		
		this.poserThread = new Thread( this, "vrpn PoserRemote" );
		this.poserThread.start( );
	}
	
	
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
	
	
	public boolean requestPose( double[] position, double[] quaternion )
	{
		return requestPose( new Date(), position, quaternion );
	}	
		public boolean requestPose( Date t,  double[] position,  double[] quaternion )
	{
		boolean retval = false;		if( position.length != 3 || quaternion.length != 4 )			return false;		
		synchronized( downInVrpnLock )
		{			retval = requestPose_native( t.getTime(), t.getTime(), position, quaternion );		}		return retval;
	}
	
	
	public boolean requestPoseVelocity( double[] position,  double[] quaternion,  double interval )
	{
		return requestPoseVelocity( new Date(), position,  quaternion,  interval  );
	}
	
		public boolean requestPoseVelocity( Date t,  double[] position,  double[] quaternion,  double interval )	{
		boolean retval = false;
		if( position.length != 3 || quaternion.length != 4 )			return false;		
		synchronized( downInVrpnLock )
		{			retval = requestPoseVelocity_native( t.getTime(), t.getTime(), position, quaternion, interval );		}		return retval;
	}
	
	
	/**
	 * @return The period, in milliseconds.
	 */
	public synchronized long getTimerPeriod( )
	{
		return mainloopPeriod;
	}
	
	
	/**
	 * Stops the poser thread
	 */
	public void stopRunning( )
	{
		keepRunning = false;
		while( poserThread.isAlive( ) )
		{
			try { poserThread.join( ); }
			catch( InterruptedException e ) { }
		}
		synchronized( downInVrpnLock )
		{
			this.shutdownPoser( );
		}
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
	 * Initialize the native poser object
	 * @param name The name of the poser and host (e.g., <code>"Poser0@myhost.edu"</code>).
	 * @return <code>true</code> if the poser was connected successfully, 
	 *			<code>false</code> otherwise.
	 */
	protected native boolean init( String name, String localInLogfileName, 
								   String localOutLogfileName, String remoteInLogfileName,
								   String remoteOutLogfileName );

	/**
	 * This should only be called from the method finalize()
	 */
	protected native void shutdownPoser( );
	
	protected native void mainloop( );
	protected native boolean requestPose_native( long secs, long usecs, double[] position, double[] quaternion );
	protected native boolean requestPoseVelocity_native( long secs, long usecs, double[] position, double[] quaternion, double interval);
	
	public void finalize( ) throws Throwable
	{
		keepRunning = false;
		while( poserThread.isAlive( ) )
		{
			try { poserThread.join( ); }
			catch( InterruptedException e ) { }
		}
		
		synchronized( downInVrpnLock )
		{
			this.shutdownPoser( );
		}
	}
	
	// end protected methods
	///////////////////////
	
	///////////////////
	// data members
	
	// this is used by the native code to store a C++ pointer to the 
	// native vrpn_PoserRemote object
	// this should be negative if the poser is uninitialized or
	// has already been shut down
	protected int native_poser = -1;
	
	// this is used to stop and to keep running the tracking thread
	// in an orderly fashion.
	protected boolean keepRunning = true;

	// the tracking thread
	Thread poserThread = null;

	// how long the thread sleeps between checking for messages
	protected long mainloopPeriod = 100; // milliseconds
	

}
