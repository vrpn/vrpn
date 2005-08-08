package vrpn;
import java.util.*;



public class ForceDeviceRemote extends VRPN implements Runnable
{
	
	//////////////////
	// Public structures and interfaces

	public class ForceChange
	{
		public Date msg_time = new Date();
		public double force[] = new double[3];
	}
	
	public interface ForceChangeListener
	{
		public void forceUpdate( ForceChange f, ForceDeviceRemote forceDevice );
	}
	
	
	// SCP is "Surface Contact Point"
	public class SCPChange
	{
		public Date msg_time = new Date( );
		public double pos[] = new double[3];
		public double quat[] = new double[4];
	}
		
	public interface SCPChangeListener
	{
		public void scpUpdate( SCPChange s, ForceDeviceRemote forceDevice );
	}
	
	
	public class ForceError
	{
		public Date msg_time = new Date( );
		int errorCode;
	}
	
	public interface ForceErrorListener
	{
		public void forceError( ForceError a, ForceDeviceRemote forceDevice );
	}
	
	public static final int NO_CONSTRAINT = 0;
	public static final int POINT_CONSTRAINT = 1;
	public static final int LINE_CONSTRAINT = 2;
	public static final int PLANE_CONSTRAINT = 3;
	
	// end of the public structures and interfaces
	//////////////////////////////////
	
	
	////////////////////////////////
	// Public methods
	//
	
	/**
	 * @exception java.lang.InstantiationException
	 *		If the force device could not be created because of problems with
	 *      its native code and linking.
	 */
	public ForceDeviceRemote( String name, String localInLogfileName, String localOutLogfileName,
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
			System.out.println( "Error initializing remote forece device " + name + "." );
			System.out.println( " -- Unable to find the right functions.  This may be a version problem." );
			throw new InstantiationException( e.getMessage( ) );
		}
		
		this.forceThread = new Thread( this, "vrpn ForceDeviceRemote" );
		this.forceThread.start( );

	}
	
	
	
	public native void sendSurface( );
	public native void startSurface( );
	public native void stopSurface( );

	public native void setVertex( int number, float x, float y, float z );
	public native void setNormal( int number, float x, float y, float z );
	public native void setTriangle( int number, int vertex1, int vertex2, int vertex3,
									int normal1, int normal2, int normal3 );
	public native void removeTriangle( int number );
	public native void updateTrimeshChanges( );
	public native void setTrimeshTransform( float[] transform );
	public native void clearTrimesh( );
	
	public native void useHcollide( );
	public native void useGhost( );
	
	public native boolean enableConstraint( int enable );
	public native boolean setConstraintMode( int mode );
	public native boolean setConstraintPoint( float[] point );
	public native boolean setConstraintLinePoint( float[] point );
	public native boolean setConstraintLineDirection( float[] direction );
	public native boolean setConstraintPlanePoint( float[] point );
	public native boolean setConstraintPlaneNormal( float[] normal );
	public native boolean setConstraintKSpring( float k );
	
	public native boolean sendForceField( float[] origin, float[] force, float[][] jacobian, float radius );
	public native boolean sendForceField( );
	public native boolean stopForceField( );
	
	
	public synchronized void addForceChangeListener( ForceChangeListener listener )
	{
		forceListeners.addElement( listener );
	}

	/**
	 * @return true on success; false on failure
	 */
	public synchronized boolean removeForceChangeListener( ForceChangeListener listener )
	{
		return forceListeners.removeElement( listener );
	}
	
	
	public synchronized void addSCPChangeListener( SCPChangeListener listener )
	{
		scpListeners.addElement( listener );
	}
	
	/**
	 * @return true on success; false on failure
	 */
	public synchronized boolean removeSCPChangeListener( SCPChangeListener listener )
	{
		return scpListeners.removeElement( listener );
	}
	
	
	public synchronized void addForceErrorListener( ForceErrorListener listener )
	{
		errorListeners.addElement( listener );
	}
	
	/**
	 * @return true on success; false on failure
	 */
	public synchronized boolean removeForceErrorListener( ForceErrorListener listener )
	{
		return errorListeners.removeElement( listener );
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
	
	/**
	 * @return The period, in milliseconds.
	 */
	public synchronized long getTimerPeriod( )
	{
		return mainloopPeriod;
	}
	
	
	/**
	 * Stops the force device thread
	 */
	public void stopRunning( )
	{
		keepRunning = false;
		while( forceThread.isAlive( ) )
		{
			try { forceThread.join( ); }
			catch( InterruptedException e ) { }
		}
		forceListeners.removeAllElements( );
		scpListeners.removeAllElements( );
		errorListeners.removeAllElements( );
		synchronized( downInVrpnLock )
		{
			this.shutdownForceDevice( );
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
	 * Should be called only by mainloop(), a native method which is itself
	 * synchronized.  By extension, this method is synchronized (in that, for
	 * a given ForceDeviceRemote object, this method can only be called from one
	 * thread at a time).
	 */
	protected void handleForceChange( long tv_sec, long tv_usec,  
									  double x, double y, double z )
	{
		// putting the body of this function into a synchronized block prevents
		// other instances of ForceDeviceRemote from calling listeners' forceUpdate
		// method concurrently.
		synchronized( notifyingForceListenersLock )
		{
			ForceChange f = new ForceChange();
			f.msg_time.setTime( tv_sec * 1000 + (int) (tv_usec/1000.0) );
			f.force[0] = x;  f.force[1] = y;  f.force[2] = z;
			
			// notify all listeners
			Enumeration e = forceListeners.elements( );
			while( e.hasMoreElements( ) )
			{
				ForceChangeListener l = (ForceChangeListener) e.nextElement( );
				l.forceUpdate( f, this );
			}
		} // end synchronized( notifyingForceListenersLock )
	} // end method handleForceChange
	
	
	/**
	 * @see #handleForceChange
	 */
	protected void handleSCPChange( long tv_sec, long tv_usec, 
									double x, double y, double z, 
									double quat0, double quat1, double quat2, double quat3 )
	{
		synchronized( notifyingSCPListenersLock )
		{
			SCPChange s = new SCPChange();
			s.msg_time.setTime( tv_sec * 1000 + (int) (tv_usec/1000.0) );
			s.pos[0] = x;  s.pos[1] = y;  s.pos[2] = z;
			s.quat[0] = quat0;  s.quat[1] = quat1;  s.quat[2] = quat2;  
			s.quat[3] = quat3;
			
			// notify all listeners
			Enumeration e = scpListeners.elements( );
			while( e.hasMoreElements( ) )
			{
				SCPChangeListener l = (SCPChangeListener) e.nextElement( );
				l.scpUpdate( s, this );
			}
		} // end synchronized( notifyingSCPListenersLock )
	} // end method handleSCPChange
		
	
	/**
	 * @see #handleForceChange
	 */
	protected void handleForceError( long tv_sec, long tv_usec, int errorCode )
	{
		synchronized( notifyingErrorListenersLock )
		{
			ForceError u = new ForceError();
			u.msg_time.setTime( tv_sec * 1000 + (int) (tv_usec/1000.0) );
			u.errorCode = errorCode;
			
			// notify all listeners
			Enumeration e = errorListeners.elements( );
			while( e.hasMoreElements( ) )
			{
				ForceErrorListener l = (ForceErrorListener) e.nextElement( );
				l.forceError( u, this );
			}
		} // end synchronized( notifyingAccelerationListenersLock )
	} // end method handleErrorChange
	

	/**
	 * Initialize the native force device object
	 * @param name The name of the force device and host (e.g., <code>"Phantom0@myhost.edu"</code>).
	 * @return <code>true</code> if the force device was connected successfully, 
	 *			<code>false</code> otherwise.
	 */
	protected native boolean init( String name, String localInLogfileName, 
								   String localOutLogfileName, String remoteInLogfileName,
								   String remoteOutLogfileName );

	/**
	 * This should only be called from the method finalize()
	 */
	protected native void shutdownForceDevice( );
	
	protected native void mainloop( );
	
	public void finalize( ) throws Throwable
	{
		keepRunning = false;
		while( forceThread.isAlive( ) )
		{
			try { forceThread.join( ); }
			catch( InterruptedException e ) { }
		}
		forceListeners.removeAllElements( );
		scpListeners.removeAllElements( );
		errorListeners.removeAllElements( );
		synchronized( downInVrpnLock )
		{
			this.shutdownForceDevice( );
		}
	}
	
	// end protected methods
	///////////////////////
	
	///////////////////
	// data members
	
	// this is used by the native code to store a C++ pointer to the 
	// native vrpn_ForceDeviceRemote object
	// this should be negative if the force_device is uninitialized or
	// has already been shut down
	protected int native_force_device = -1;
	
	// this is used to stop and to keep running the tracking thread
	// in an orderly fashion.
	protected boolean keepRunning = true;
	
	// the tracking thread
	Thread forceThread = null;

	// how long the thread sleeps between checking for messages
	protected long mainloopPeriod = 100; // milliseconds

	
	protected Vector forceListeners = new Vector( );
	protected Vector scpListeners = new Vector( );
	protected Vector errorListeners = new Vector( );
	
	/**
	 * @see vrpn.TrackerRemote#notifyingChangeListenersLock
	 */
	protected final static Object notifyingForceListenersLock = new Object( );
	protected final static Object notifyingSCPListenersLock = new Object( );
	protected final static Object notifyingErrorListenersLock = new Object( );
	
	
}
