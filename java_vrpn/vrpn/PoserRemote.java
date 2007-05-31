
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
	
	
	public boolean requestPoseVelocity( double[] position,  double[] quaternion,  double interval )
	{
		return requestPoseVelocity( new Date(), position,  quaternion,  interval  );
	}
	
	
	public boolean requestPoseVelocity( Date t,  double[] position,  double[] quaternion,  double interval )
	{
		boolean retval = false;
		if( position.length != 3 || quaternion.length != 4 )
			return false;
		
		synchronized( downInVrpnLock )
		{
			retval = requestPoseVelocity_native( t.getTime(), t.getTime(), position, quaternion, interval );
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
	protected native boolean requestPoseVelocity_native( long secs, long usecs, double[] position, double[] quaternion, double interval);
	
	// end protected methods
	///////////////////////
	

}
