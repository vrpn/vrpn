package vrpn;
import java.util.*;



public class ForceDeviceRemote extends VRPNDevice implements Runnable
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
	}
		
	
	public void sendSurface( )
	{
		synchronized( downInVrpnLock )
		{
			sendSurface_native( );
		}
	}
	
	
	public void startSurface( )
	{
		synchronized( downInVrpnLock )
		{
			startSurface_native( );
		}
	}
	
	
	public void stopSurface( )
	{
		synchronized( downInVrpnLock )
		{
			stopSurface_native( );
		}
	}
	

	public void setVertex( int number, float x, float y, float z )
	{
		synchronized( downInVrpnLock )
		{
			setVertex_native( number, x, y, z );
		}
	}
	

	public void setNormal( int number, float x, float y, float z )
	{
		synchronized( downInVrpnLock )
		{
			setNormal_native( number, x, y, z );
		}
	}
	

	public void setTriangle( int number, int vertex1, int vertex2, int vertex3,
									int normal1, int normal2, int normal3 )
	{
		synchronized( downInVrpnLock )
		{
			setTriangle_native( number, vertex1, vertex2, vertex3,
								normal1, normal2, normal3 );
		}
	}
	

	public void removeTriangle( int number )
	{
		synchronized( downInVrpnLock )
		{
			removeTriangle_native( number );
		}
	}
	

	public void updateTrimeshChanges( )
	{
		synchronized( downInVrpnLock )
		{
			updateTrimeshChanges_native( );
		}
	}
	

	public void setTrimeshTransform( float[] transform )
	{
		synchronized( downInVrpnLock )
		{
			setTrimeshTransform_native( transform );
		}
	}
	

	public void clearTrimesh( )
	{
		synchronized( downInVrpnLock )
		{
			clearTrimesh_native( );
		}
	}
	
	
	public void useHcollide( )
	{
		synchronized( downInVrpnLock )
		{
			useHcollide_native( );
		}
	}
	

	public void useGhost( )
	{
		synchronized( downInVrpnLock )
		{
			useGhost_native( );
		}
	}
	
	
	public boolean enableConstraint( int enable )
	{
		boolean retval = false;
		synchronized( downInVrpnLock )
		{
			retval = enableConstraint_native( enable );
		}
		return retval;
	}
	

	public boolean setConstraintMode( int mode )
	{
		boolean retval = false;
		synchronized( downInVrpnLock )
		{
			retval = setConstraintMode_native( mode );
		}
		return retval;
	}
	

	public boolean setConstraintPoint( float[] point )
	{
		boolean retval = false;
		synchronized( downInVrpnLock )
		{
			retval = setConstraintPoint_native( point );
		}
		return retval;
	}
	

	public boolean setConstraintLinePoint( float[] point )
	{
		boolean retval = false;
		synchronized( downInVrpnLock )
		{
			retval = setConstraintLinePoint_native( point );
		}
		return retval;
	}
	

	public boolean setConstraintLineDirection( float[] direction )
	{
		boolean retval = false;
		synchronized( downInVrpnLock )
		{
			retval = setConstraintLineDirection_native( direction );
		}
		return retval;
	}
	

	public boolean setConstraintPlanePoint( float[] point )
	{
		boolean retval = false;
		synchronized( downInVrpnLock )
		{
			retval = setConstraintPlanePoint_native( point );
		}
		return retval;
	}
	

	public boolean setConstraintPlaneNormal( float[] normal )
	{
		boolean retval = false;
		synchronized( downInVrpnLock )
		{
			retval = setConstraintPlaneNormal_native( normal );
		}
		return retval;
	}
	

	public boolean setConstraintKSpring( float k )
	{
		boolean retval = false;
		synchronized( downInVrpnLock )
		{
			retval = setConstraintKSpring_native( k );
		}
		return retval;
	}
	
	
	public boolean sendForceField( float[] origin, float[] force, float[][] jacobian, float radius )
	{
		boolean retval = false;
		synchronized( downInVrpnLock )
		{
			retval = sendForceField_native( origin, force, jacobian, radius );
		}
		return retval;
	}
	

	public boolean sendForceField( )
	{
		boolean retval = false;
		synchronized( downInVrpnLock )
		{
			retval = sendForceField_native( );
		}
		return retval;
	}
	

	public boolean stopForceField( )
	{
		boolean retval = false;
		synchronized( downInVrpnLock )
		{
			retval = stopForceField_native( );
		}
		return retval;
	}
	
	
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

	// end public methods
	////////////////////////
	
	
	////////////////////////
	// Protected methods
	//
	
	protected native void sendSurface_native( );
	protected native void startSurface_native( );
	protected native void stopSurface_native( );

	protected native void setVertex_native( int number, float x, float y, float z );
	protected native void setNormal_native( int number, float x, float y, float z );
	protected native void setTriangle_native( int number, int vertex1, int vertex2, int vertex3,
											  int normal1, int normal2, int normal3 );
	protected native void removeTriangle_native( int number );
	protected native void updateTrimeshChanges_native( );
	protected native void setTrimeshTransform_native( float[] transform );
	protected native void clearTrimesh_native( );
	
	protected native void useHcollide_native( );
	protected native void useGhost_native( );
	
	protected native boolean enableConstraint_native( int enable );
	protected native boolean setConstraintMode_native( int mode );
	protected native boolean setConstraintPoint_native( float[] point );
	protected native boolean setConstraintLinePoint_native( float[] point );
	protected native boolean setConstraintLineDirection_native( float[] direction );
	protected native boolean setConstraintPlanePoint_native( float[] point );
	protected native boolean setConstraintPlaneNormal_native( float[] normal );
	protected native boolean setConstraintKSpring_native( float k );
	
	protected native boolean sendForceField_native( float[] origin, float[] force, float[][] jacobian, float radius );
	protected native boolean sendForceField_native( );
	protected native boolean stopForceField_native( );
	
	
	/**
	 * Stops the force device thread
	 */
	protected void stoppedRunning( )
	{
		forceListeners.removeAllElements( );
		scpListeners.removeAllElements( );
		errorListeners.removeAllElements( );
		synchronized( downInVrpnLock )
		{
			this.shutdownForceDevice( );
		}
	}

	
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
	
	// end protected methods
	///////////////////////
	
	///////////////////
	// data members
	
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
