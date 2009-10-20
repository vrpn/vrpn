
package vrpn;
import java.util.*;


public class TrackerRemote extends VRPNDevice implements Runnable
{
	//////////////////
	// Public structures and interfaces

	public class TrackerUpdate
	{
		public Date msg_time = new Date();
		public int sensor = -1;
		public double pos[] = new double[3];
		public double quat[] = new double[4];
	}
	
	public interface PositionChangeListener
	{
		public void trackerPositionUpdate( TrackerUpdate u, TrackerRemote tracker );
	}
	
	
	public class VelocityUpdate
	{
		public Date msg_time = new Date( );
		public int sensor = -1;
		public double vel[] = new double[3];
		public double vel_quat[] = new double[4];
		public double vel_quat_dt = 0;
	}
		
	public interface VelocityChangeListener
	{
		public void trackerVelocityUpdate( VelocityUpdate v, TrackerRemote tracker );
	}
	
	
	public class AccelerationUpdate
	{
		public Date msg_time = new Date( );
		public int sensor = -1;
		public double acc[] = new double[3];
		public double acc_quat[] = new double[4];
		public double acc_quat_dt = 0;
	}
	
	public interface AccelerationChangeListener
	{
		public void trackerAccelerationUpdate( AccelerationUpdate a, TrackerRemote tracker );
	}
	
	// end of the public structures and interfaces
	//////////////////////////////////
	
	
	////////////////////////////////
	// Public methods
	//
	
	/**
	 * @param name The name of the tracker to connect to (e.g., Tracker0@localhost)
	 * @param localInLogfileName The name of a logfile to save incoming messages.  Use <code>
	 * null</code> if no such log is desired.
	 * @param localOutLogfileName The name of a logfile to save outgoing messages.  Use <code>
	 * null</code> if no such log is desired.
	 * @param remoteInLogfileName The name of a logfile in which the server <code>name</code>
	 * should save incoming messages.  Use <code>null</code> if no such log is desired.
	 * @param remoteOutLogfileName  The name of a logfile in which the server <code>name</code>
	 * should save outgoing messages.  Use <code>null</code> if no such log is desired.
	 * @exception java.lang.InstantiationException
	 *		If the tracker could not be created because of problems with
	 *      its native code and linking.
	 */
	public TrackerRemote( String name, String localInLogfileName, String localOutLogfileName,
						  String remoteInLogfileName, String remoteOutLogfileName ) 
		throws InstantiationException
	{
		super( name, localInLogfileName, localOutLogfileName, remoteInLogfileName, remoteOutLogfileName );
	}
	
	
	/**
	 * @return 0 on success; non-zero on failure
	 */
	public synchronized native int setUpdateRate( double samplesPerSecond );
	

	public synchronized void addPositionChangeListener( PositionChangeListener listener )
	{
		changeListeners.addElement( listener );
	}
	
	
	/**
	 * @return true on success; false on failure
	 */
	public synchronized boolean removePositionChangeListener( PositionChangeListener listener )
	{
		return changeListeners.removeElement( listener );
	}
	
	
	public synchronized void addVelocityChangeListener( VelocityChangeListener listener )
	{
		velocityListeners.addElement( listener );
	}
	
	
	/**
	 * @return true on success; false on failure
	 */
	public synchronized boolean removeVelocityChangeListener( VelocityChangeListener listener )
	{
		return velocityListeners.removeElement( listener );
	}
	
	
	public synchronized void addAccelerationChangeListener( AccelerationChangeListener listener )
	{
		accelerationListeners.addElement( listener );
	}
	
	
	/**
	 * @return true on success; false on failure
	 */
	public synchronized boolean removeAccelerationChangeListener( AccelerationChangeListener listener )
	{
		return accelerationListeners.removeElement( listener );
	}

		
	// end public methods
	////////////////////////
	
	
	////////////////////////
	// Protected methods
	//
	
	protected void stoppedRunning( )
	{
		changeListeners.removeAllElements( );
		velocityListeners.removeAllElements( );
		accelerationListeners.removeAllElements( );
		synchronized( downInVrpnLock )
		{
			this.shutdownTracker( );
		}
	}
	
	/**
	 * Should be called only by mainloop(), a native method which is itself
	 * synchronized.  By extension, this method is synchronized (in that, for
	 * a given TrackerRemote object, this method can only be called from one
	 * thread at a time).
	 */
	protected void handleTrackerChange( long tv_sec, long tv_usec, int sensor, 
										double x, double y, double z, 
										double quat0, double quat1, double quat2, double quat3 )
	{
		// putting the body of this function into a synchronized block prevents
		// other instances of TrackerRemote from calling listeners' trackerPositionUpdate
		// method concurrently.
		synchronized( notifyingChangeListenersLock )
		{
			TrackerUpdate t = new TrackerUpdate();
			t.msg_time.setTime( tv_sec * 1000 + (int) (tv_usec/1000.0) );
			t.sensor = sensor;
			t.pos[0] = x;  t.pos[1] = y;  t.pos[2] = z;
			t.quat[0] = quat0;  t.quat[1] = quat1;  t.quat[2] = quat2;  t.quat[3] = quat3;
			
			// notify all listeners
			Enumeration e = changeListeners.elements( );
			while( e.hasMoreElements( ) )
			{
				PositionChangeListener l = (PositionChangeListener) e.nextElement( );
				l.trackerPositionUpdate( t, this );
			}
		} // end synchronized( notifyingChangeListenersLock )
	} // end method handleTrackerChange
	
	
	/**
	 * @see #handleTrackerChange
	 */
	protected void handleVelocityChange( long tv_sec, long tv_usec, int sensor, 
										 double x, double y, double z, 
										 double quat0, double quat1, double quat2, double quat3,
										 double quat_dt )
	{
		synchronized( notifyingVelocityListenersLock )
		{
			VelocityUpdate v = new VelocityUpdate();
			v.msg_time.setTime( tv_sec * 1000 + (int) (tv_usec/1000.0) );
			v.sensor = sensor;
			v.vel[0] = x;  v.vel[1] = y;  v.vel[2] = z;
			v.vel_quat[0] = quat0;  v.vel_quat[1] = quat1;  v.vel_quat[2] = quat2;  
			v.vel_quat[3] = quat3;
			v.vel_quat_dt = quat_dt;
			
			// notify all listeners
			Enumeration e = velocityListeners.elements( );
			while( e.hasMoreElements( ) )
			{
				VelocityChangeListener l = (VelocityChangeListener) e.nextElement( );
				l.trackerVelocityUpdate( v, this );
			}
		} // end synchronized( notifyingVelocityListenersLock )
	} // end method handleVelocityChange
		
	
	/**
	 * @see #handleTrackerChange
	 */
	protected void handleAccelerationChange( long tv_sec, long tv_usec, int sensor, 
											 double x, double y, double z, 
											 double quat0, double quat1, double quat2, double quat3,
											 double quat_dt )
	{
		synchronized( notifyingAccelerationListenersLock )
		{
			AccelerationUpdate a = new AccelerationUpdate();
			a.msg_time.setTime( tv_sec * 1000 + (int) (tv_usec/1000.0) );
			a.sensor = sensor;
			a.acc[0] = x;  a.acc[1] = y;  a.acc[2] = z;
			a.acc_quat[0] = quat0;  a.acc_quat[1] = quat1;  a.acc_quat[2] = quat2;
			a.acc_quat[3] = quat3;
			a.acc_quat_dt = quat_dt;
			
			// notify all listeners
			Enumeration e = accelerationListeners.elements( );
			while( e.hasMoreElements( ) )
			{
				AccelerationChangeListener l = (AccelerationChangeListener) e.nextElement( );
				l.trackerAccelerationUpdate( a, this );
			}
		} // end synchronized( notifyingAccelerationListenersLock )
	} // end method handleAccelerationChange

	
	protected native void shutdownTracker( );
	
	/**
	 * This should only be called from the method run()
	 */
	protected native void mainloop( );
	

	/**
	 * Initialize the native tracker object
	 * @param name The name of the tracker and host (e.g., <code>"Tracker0@myhost.edu"</code>).
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
	
	protected Vector changeListeners = new Vector( );
	protected Vector velocityListeners = new Vector( );
	protected Vector accelerationListeners = new Vector( );

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
	protected final static Object notifyingChangeListenersLock = new Object( );
	protected final static Object notifyingVelocityListenersLock = new Object( );
	protected final static Object notifyingAccelerationListenersLock = new Object( );
		
}
