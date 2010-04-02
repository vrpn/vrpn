
package vrpn;
import java.util.*;


public class PoserRemote extends VRPNDevice implements Runnable
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
	}
	
	
	public boolean requestPose( double[] position, double[] quaternion )
	{
		return requestPose( new Date(), position, quaternion );
	}
	
	
	public boolean requestPose( Date t,  double[] position,  double[] quaternion )
	{
		boolean retval = false;
		if( position.length != 3 || quaternion.length != 4 )
			return false;
		long msecs = t.getTime( );
		long secs = msecs / 1000;
		synchronized( downInVrpnLock )
		{
			retval = requestPose_native( secs, ( msecs - secs * 1000 ) * 1000, position, quaternion );
		}
		return retval;
	}
	
	/**
	 * @param positionDelta new position = old position + positionDelta
	 * @param quaternion new orientation = quaternion * old orientation
	 * @return true if the request was successfully transmitted to the poser.
	 * false if there was some problems sending the request.
	 */
	public boolean requestPoseRelative( double[] positionDelta, double[] quaternion )
	{
		return requestPoseRelative( new Date(), positionDelta, quaternion );
	}
	
	/**
	 * 
	 * @param t the time at which this pose should be adopted
	 * @param positionDelta new position = old position + positionDelta
	 * @param quaternion new orientation = quaternion * old orientation
	 * @return true if the request was successfully transmitted to the poser.
	 * false if there was some problems sending the request.
	 */
	public boolean requestPoseRelative( Date t, double[] positionDelta, double[] quaternion )
	{
		boolean retval = false;
		if( positionDelta.length != 3 || quaternion.length != 4 )
			return false;
		long msecs = t.getTime( );
		long secs = msecs / 1000;
		synchronized( downInVrpnLock )
		{
			retval = requestPoseRelative_native( secs, ( msecs - secs * 1000 ) * 1000, positionDelta, quaternion );
		}
		return retval;
	}
	
	
	public boolean requestVelocity( double[] velocity,  double[] quaternion,  double interval )
	{
		return requestVelocity( new Date(), velocity,  quaternion,  interval  );
	}
	
	
	public boolean requestVelocity( Date t,  double[] velocity,  double[] quaternion,  double interval )
	{
		boolean retval = false;
		if( velocity.length != 3 || quaternion.length != 4 )
			return false;		
		long msecs = t.getTime( );
		long secs = msecs / 1000;
		synchronized( downInVrpnLock )
		{
			retval = requestVelocity_native( secs, ( msecs - secs * 1000 ) * 1000, velocity, quaternion, interval );
		}
		return retval;
	}
	
	/**
	 * @param velocityDelta new velocity = old velocity + velocityDelta
	 * @param quaternion new orientation = quaternion * old orientation
	 * @param intervalDelta new interval = old interval + intervalDelta
	 * @return true if the request was successfully transmitted to the poser.
	 * false if there was some problems sending the request.
	 */
	public boolean requestVelocityRelative( double[] velocityDelta, double[] quaternion, double intervalDelta )
	{
		return requestVelocityRelative( new Date(), velocityDelta, quaternion, intervalDelta );
	}
	
	
	/**
	 * @param t the time at which this velocity pose should be adopted.
	 * @param velocityDelta new velocity = old velocity + velocityDelta
	 * @param quaternion new orientation = quaternion * old orientation
	 * @param intervalDelta new interval = old interval + intervalDelta
	 * @return true if the request was successfully transmitted to the poser.
	 * false if there was some problems sending the request.
	 */
	public boolean requestVelocityRelative( Date t, double[] velocityDelta, double[] quaternion, double intervalDelta )
	{
		boolean retval = false;
		if( velocityDelta.length != 3 || quaternion.length != 4 )
			return false;
		long msecs = t.getTime( );
		long secs = msecs / 1000;
		synchronized( downInVrpnLock )
		{
			retval = requestVelocityRelative_native( secs, ( msecs - secs * 1000 ) * 1000, velocityDelta, quaternion, intervalDelta );
		}
		return retval;
	}
	
	// end public methods
	////////////////////////
	
	
	////////////////////////
	// Protected methods
	//
	
	/**
	 * Stops the poser thread
	 */
	protected void stoppedRunning( )
	{
		synchronized( downInVrpnLock )
		{
			this.shutdownPoser( );
		}
	}

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
	protected native boolean requestPoseRelative_native( long secs, long usecs, double[] position_delta, double[] quaternion );
	protected native boolean requestVelocity_native( long secs, long usecs, double[] position, double[] quaternion, double interval);
	protected native boolean requestVelocityRelative_native( long secs, long usecs, double[] velocity_delta, double[] quaternion, double interval_delta );
	
	// end protected methods
	///////////////////////
	

}
