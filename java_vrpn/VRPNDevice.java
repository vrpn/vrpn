package vrpn;

import java.util.Date;

public abstract class VRPNDevice extends VRPN
{
	public class NotReplayError extends Exception
	{
		public NotReplayError( String msg )
		{  super( msg );  }
	}
	
	// the arguments to this constructor are just stored for later reference.
	// 'null' is a reasonable value for any (although something else will 
	// surely break if 'name' is null).
	public VRPNDevice( String name, String localInLogfileName, String localOutLogfileName,
				 String remoteInLogfileName, String remoteOutLogfileName )
	{
		this.connectionName = name;
		this.localInLogfileName = localInLogfileName;
		this.localOutLogfileName = localOutLogfileName;
		this.remoteInLogfileName = remoteInLogfileName;
		this.remoteOutLogfileName = remoteOutLogfileName;		
	}
	
	
	public String getConnectionName( )
	{ 
		return connectionName;
	}
	
	public String getLocalInLogfileName( )
	{ 
		return localInLogfileName;
	}
	
	public String getLocalOutLogfileName( )
	{ 
		return localOutLogfileName;
	}
	
	public String getRemoteInLogfileName( )
	{ 
		return remoteInLogfileName;
	}
	
	public String getRemoteOutLogfileName( )
	{ 
		return remoteOutLogfileName;
	}
	
	/**
	 * Sets the interval between invocations of <code>mainloop()</code>, which checks
	 * for and delivers messages.
	 * @param period The period, in milliseconds, between the beginnings of two
	 * consecutive message loops.
	 */
	final public synchronized void setTimerPeriod( long period )
	{
		mainloopPeriod = period;
	}
	
	
	/**
	 * @return The period, in milliseconds.
	 */
	final public synchronized long getTimerPeriod( )
	{
		return mainloopPeriod;
	}
	
	
	final public boolean doingOkay( )
	{
		boolean retval = false;
		synchronized( downInVrpnLock )
		{
			retval = doingOkay_native( );
		}
		return retval;
	}
	
	
	final public boolean isConnected( )
	{
		boolean retval = false;
		synchronized( downInVrpnLock )
		{
			retval = isConnected_native( );
		}
		return retval;
	}
	
	
	final public boolean isLive( )
	{
		if( liveReplayValid )
			return !replay;
		boolean retval = false;
		synchronized( downInVrpnLock )
		{
			retval = isLive_native( );
		}
		replay = !retval;
		return retval;
	}
	
	
	final public boolean isReplay( )
	{
		return !isLive();
	}
	
	
	final public long getElapsedTimeSecs( )
	{
		long retval = 0;
		synchronized( downInVrpnLock )
		{
			retval = getElapsedTimeSecs_native( );
		}
		return retval;
	}
	
	/**
	 * Valid only when in replay.
	 * @param rate The fractional rate of wall-clock time.  0.0 means 
	 *		paused.  1.0 is the default and is normal speed.
	 */
	final public void setReplayRate( float rate ) throws NotReplayError
	{
		if( isLive() ) throw new NotReplayError( "a live device" );
		synchronized( downInVrpnLock )
		{
			setReplayRate_native( rate );
		}
	}
	
	
	/**
	 * Valid only when in replay.
	 * Returns to the beginning of a file.
	 */
	final public boolean reset( ) throws NotReplayError
	{
		if( isLive() ) throw new NotReplayError( "a live device" );
		boolean retval = false;
		synchronized( downInVrpnLock )
		{
			retval = reset_native();
		}
		return retval;
	}
	
	
	/**
	 * Valid only when in replay.
	 * @return true iff we are at the end of the file.  false otherwise, or if not replay.
	 */
	final public boolean eof( ) throws NotReplayError
	{
		if( isLive() ) throw new NotReplayError( "a live device" );
		boolean retval = false;
		synchronized( downInVrpnLock )
		{
			retval = eof_native( );
		}
		return retval;
	}

	/**
 	 * Valid only when in replay.
	 */
	final public boolean playToElapsedTime( long seconds ) throws NotReplayError
	{
		if( isLive() ) throw new NotReplayError( "a live device" );
		boolean retval = false;
		synchronized( downInVrpnLock )
		{
			retval = playToElapsedTime_native( seconds );
		}
		return retval;
	}
	
	
	/**
 	 * Valid only when in replay.
	 */
	final public boolean playToWallTime( Date date ) throws NotReplayError
	{
		if( isLive() ) throw new NotReplayError( "a live device" );
		boolean retval = false;
		synchronized( downInVrpnLock )
		{
			retval = playToWallTime_native( date );
		}
		return retval;
	}
	
	
	/**
 	 * Valid only when in replay.
	 */
	final public double getLengthSecs( ) throws NotReplayError
	{
		if( isLive() ) throw new NotReplayError( "a live device" );
		double retval = -1;
		synchronized( downInVrpnLock )
		{
			retval = getLengthSecs_native( );
		}
		return retval;
	}
	
	
	/**
 	 * Valid only when in replay.
	 */
	final public float getReplayRate( ) throws NotReplayError
	{
		if( isLive() ) throw new NotReplayError( "a live device" );
		float retval = -1;
		synchronized( downInVrpnLock )
		{
			retval = getReplayRate_native( );
		}
		return retval;
	}
	
	
	/**
 	 * Valid only when in replay.
 	 * @return the earliest time of a user message in a file being replayed.
	 */
	final public Date getEarliestTime( ) throws NotReplayError
	{
		if( isLive() ) throw new NotReplayError( "a live device" );
		if( !earliestTimeValid )
		{
			synchronized( downInVrpnLock )
			{
				boolean success = getEarliestTime_native( earliestTime );
				if( !success ) earliestTime.setTime( 0 );
				else earliestTimeValid = true;
			}
		}
		return earliestTime;
	}
	
	
	/**
 	 * Valid only when in replay.
 	 * @return the latest time of a user message in a file being replayed
	 */
	final public Date getLatestTime( ) throws NotReplayError
	{
		if( isLive() ) throw new NotReplayError( "a live device" );
		if( !latestTimeValid )
		{
			synchronized( downInVrpnLock )
			{
				boolean success = getLatestTime_native( latestTime );
				if( !success ) latestTime.setTime( 0 );
				else latestTimeValid = true;
			}
		}
		return latestTime;
	}
	
	
	/**
	 * Stops the device thread
	 */
	final public void stopRunning( )
	{
		keepRunning = false;
		while( deviceThread.isAlive( ) )
		{
			try { deviceThread.join( ); }
			catch( InterruptedException e ) { }
		}
		
		// let the concrete subclasses do any necessary clean-up
		stoppedRunning( );
	}

	
	/**
	 * This should <b>not</b> be called by user code.
	 */
	final public void run( )
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
	
	
	final public void finalize( ) throws Throwable
	{
		stopRunning( );
	}
	
	// end public methods
	////////////////////////
	
	
	////////////////////////
	// Protected methods
	//
	
	protected native boolean isLive_native( );
	protected native boolean doingOkay_native( );
	protected native boolean isConnected_native( );
	protected native long getElapsedTimeSecs_native( );
	protected native void setReplayRate_native( float rate );
	protected native float getReplayRate_native( );
	protected native boolean reset_native( );
	protected native boolean eof_native( );
	protected native boolean playToElapsedTime_native( long seconds );
	protected native boolean playToWallTime_native( Date d );
	protected native double getLengthSecs_native( );
	protected native boolean getEarliestTime_native( Date d );
	protected native boolean getLatestTime_native( Date d );

	/**
	 * This should only be called from the method run()
	 */
	protected abstract void mainloop( );
	
	
	/**
	 * This will be called when the specific vrpn device
	 * shuts down.
	 */
	protected abstract void stoppedRunning( );
	
	// end protected methods
	///////////////////////
	
	///////////////////
	// data members
	
	// this is used by the native code to store a C++ pointer to the 
	// native device object (e.g., the vrpn_TrackerRemote, 
	// vrpn_AnalogRemote, etc.).  this should be negative if the device 
	// is uninitialized or has already been shut down
	protected int native_device = -1;
	
	// this is used to stop and to keep running the device thread
	// in an orderly fashion.
	protected boolean keepRunning = true;

	// the device thread
	Thread deviceThread = null;

	// how long the thread sleeps between checking for messages
	protected long mainloopPeriod = 100; // milliseconds
	
	protected String connectionName = null;
	protected String localInLogfileName = null;
	protected String localOutLogfileName = null;
	protected String remoteInLogfileName = null;
	protected String remoteOutLogfileName = null;
	
	
	protected boolean liveReplayValid = false;
	protected boolean replay = false;
	protected boolean earliestTimeValid = false;
	protected Date earliestTime = new Date();
	protected boolean latestTimeValid = false;
	protected Date latestTime = new Date();
	
	// the default constructor is private so that subclasses
	// are forced to tell us the connection name and the
	// various logfile names
	private VRPNDevice( ) { }
}
