/***************************************************************************
 * Use this class to store Tracker-, Velocity- and AcceleraionUpdates in a 
 * vector and get them when you want.  It is most useful if you don't want 
 * to worry about your code running in a multi-threaded environment and 
 * about vrpn possibly delivering device updates at arbitrary times.
 *
 * The Listener can be configured to buffer and return either all
 * updates or only the last (the most recent) update.  Tracker, velocity
 * and acceleration messages are buffered independently.  By default, the
 * Listener keeps only the last update of each.
 * 
 * It is not intended that Listeners be shared among objects.  Each 
 * entity in a program that is interested in hearing about updates 
 * from some vrpn Tracker device (and which wishes to use this Listener
 * mechanism) should create its own listener (even if multiple entities 
 * wish to hear from the same device).
 ***************************************************************************/

package vrpn;
import java.util.Vector;

public class TrackerRemoteListener
	implements TrackerRemote.PositionChangeListener, 
	TrackerRemote.VelocityChangeListener, 
	TrackerRemote.AccelerationChangeListener
{
	public static final int ALL_UPDATES = 0;
	public static final int LAST_UPDATE = 1;
	
	
	public TrackerRemoteListener( TrackerRemote tracker )
	{
		
		trackerUpdates = new Vector();
		velocityUpdates = new Vector();
		accelerationUpdates = new Vector();
		
		trackerBufferMode = LAST_UPDATE;
		velocityBufferMode = LAST_UPDATE;
		accelerationBufferMode = LAST_UPDATE;

		tracker.addPositionChangeListener(this);
		tracker.addVelocityChangeListener(this);
		tracker.addAccelerationChangeListener(this);
	}
	
	
	/**
	 * Sets the buffering mode of the Listener to record and return only 
	 * the most recent (the last, the latest) TrackerUpdate.
	 */
	public synchronized void setModeLastTrackerUpdate()
	{
		trackerBufferMode = LAST_UPDATE;
		if( !trackerUpdates.isEmpty() )
		{
			Object temp = trackerUpdates.lastElement();
			trackerUpdates.removeAllElements();
			trackerUpdates.addElement(temp);
		}
	}
	
	
	/**
	 * Sets the buffering mode of the Listener to record and return only 
	 * the most recent (the last, the latest) VelocityUpdate.
	 */
	public synchronized void setModeLastVelocityUpdate()
	{
		velocityBufferMode = LAST_UPDATE;
		if( !velocityUpdates.isEmpty() )
		{
			Object temp = velocityUpdates.lastElement();
			velocityUpdates.removeAllElements();
			velocityUpdates.addElement(temp);
		}
	}
	
	
	/**
	 * Sets the buffering mode of the Listener to record and return only 
	 * the most recent (the last, the latest) AccelerationUpdate.
	 */
	public synchronized void setModeLastAccelerationUpdate()
	{
		accelerationBufferMode = LAST_UPDATE;
		if( !accelerationUpdates.isEmpty() )
		{
			Object temp = accelerationUpdates.lastElement();
			accelerationUpdates.removeAllElements();
			accelerationUpdates.addElement(temp);
		}
	}
	
	
	/**
	 * Sets the buffering mode of the Listener to record and return all 
	 * TrackerUpdates, beginning at the time this mode is first enabled.
	 */
	public synchronized void setModeAllTrackerUpdates()
	{
		if( trackerBufferMode == LAST_UPDATE )
		{
			trackerUpdates.removeAllElements( );
		}
		trackerBufferMode = ALL_UPDATES;
	}
	
	
	/**
	 * Sets the buffering mode of the Listener to record and return all 
	 * VelocityUpdates, beginning at the time this mode is first enabled.
	 */
	public synchronized void setModeAllVelocityUpdates()
	{
		if( velocityBufferMode == LAST_UPDATE )
		{
			velocityUpdates.removeAllElements( );
		}
		velocityBufferMode = ALL_UPDATES;
	}
	
	
	/**
	 * Sets the buffering mode of the Listener to record and return all 
	 * AccelerationUpdates, beginning at the time this mode is first enabled.
	 */
	public synchronized void setModeAllAccelerationUpdates()
	{
		if( accelerationBufferMode == LAST_UPDATE )
		{
			accelerationUpdates.removeAllElements( );
		}
		accelerationBufferMode = ALL_UPDATES;
	}
	
	
	/**
	 * @return TrackerRemoteListener.ALL_UPDATES if the Listener is recording and 
	 * returning all TrackerUpdates; TrackerRemoteListener.LAST_UPDATE if only the 
	 * latest TrackerUpdate.
	 */
	public synchronized int getModeTrackerUpdate()
	{
		return trackerBufferMode;
	}
	
	
	/**
	 * @return TrackerRemoteListener.ALL_UPDATES if the Listener is recording and 
	 * returning all VelocityUpdates; TrackerRemoteListener.LAST_UPDATE if only the 
	 * latest VelocityUpdate.
	 */
	public synchronized int getModeVelocityUpdate()
	{
		return velocityBufferMode;
	}
	
	
	/**
	 * @return TrackerRemoteListener.ALL_UPDATES if the Listener is recording and 
	 * returning all AccelerationUpdates; TrackerRemoteListener.LAST_UPDATE if only the 
	 * latest AccelerationUpdate.
	 */
	public synchronized int getModeAccelerationUpdate()
	{
		return accelerationBufferMode;
	}
	
	
	/**
	 * This method retreives the buffered TrackerUpdates from the Listener.
	 * If the buffering mode is LAST_UPDATE, the last update received will
	 * be returned (note that, in this mode, successive calls to getTrackerUpdate()
	 * may return the same TrackerUpdate if no new updates were received in the 
	 * interim).  If the buffering mode is ALL_UPDATES, all updates 
	 * received since the last call to getTrackerUpdate() (or since ALL_UPDATES
	 * mode was enabled) will be returned.
	 * @return A Vector containing the buffered TrackerUpdates.  The number of
	 * TrackerUpdates returned will depend on the buffering mode.  If there are
	 * no TrackerUpdates buffered, an empty Vector will be returned.
	 * @see #setModeLastTrackerUpdate
	 * @see #setModeAllTrackerUpdates
	 */
	public synchronized Vector getTrackerUpdate()
	{
		Vector v = new Vector( );
		if( trackerUpdates.isEmpty() )
		{
			return v;
		}
		
		if( trackerBufferMode == LAST_UPDATE )
		{
			v.addElement( trackerUpdates.lastElement() );
		}
		else if( trackerBufferMode == ALL_UPDATES )
		{
			for( int i = 0; i < trackerUpdates.size(); i++ )
			{
				v.addElement( trackerUpdates.elementAt(i) );
			}
			trackerUpdates.removeAllElements();
		}
		return v;
	} // end method getTrackerUpdate( )
	
	
	/**
	 * This method retreives the buffered VelocityUpdates from the Listener.
	 * If the buffering mode is LAST_UPDATE, the last update received will
	 * be returned (note that, in this mode, successive calls to getVelocityUpdate()
	 * may return the same VelocityUpdate if no new updates were received in the 
	 * interim).  If the buffering mode is ALL_UPDATES, all updates 
	 * received since the last call to getVelocityUpdate() (or since ALL_UPDATES
	 * mode was enabled) will be returned.
	 * @return A Vector containing the buffered VelocityUpdates.  The number of
	 * VelocityUpdates returned will depend on the buffering mode.  If there are
	 * no VelocityUpdates buffered, an empty Vector will be returned.
	 * @see #setModeLastVelocityUpdate
	 * @see #setModeAllVelocityUpdates
	 */
	public synchronized Vector getVelocityUpdate()
	{
		Vector v = new Vector( );
		if( velocityUpdates.isEmpty() )
		{
			return v;
		}
		
		if( velocityBufferMode == LAST_UPDATE )
		{
			v.addElement( velocityUpdates.lastElement() );
		}
		else if( velocityBufferMode == ALL_UPDATES )
		{
			for( int i = 0; i < velocityUpdates.size(); i++ )
			{
				v.addElement( velocityUpdates.elementAt(i) );
			}
			velocityUpdates.removeAllElements();
		}
		return v;
	} // end method getVelocityUpdate( )
	
	
	/**
	 * This method retreives the buffered AccelerationUpdates from the Listener.
	 * If the buffering mode is LAST_UPDATE, the last update received will
	 * be returned (note that, in this mode, successive calls to getAcclerationUpdate()
	 * may return the same AccelerationUpdate if no new updates were received in the 
	 * interim).  If the buffering mode is ALL_UPDATES, all updates 
	 * received since the last call to getAcclerationUpdate() (or since ALL_UPDATES
	 * mode was enabled) will be returned.
	 * @return A Vector containing the buffered AccelerationUpdates.  The number of
	 * AccelerationUpdates returned will depend on the buffering mode.  If there are
	 * no AccelerationUpdates buffered, an empty Vector will be returned.
	 * @see #setModeLastAccelerationUpdate
	 * @see #setModeAllAccelerationUpdates
	 */
	public synchronized Vector getAcclerationUpdate()
	{
		Vector v = new Vector( );
		if( accelerationUpdates.isEmpty() )
		{
			return v;
		}
		
		if( accelerationBufferMode == LAST_UPDATE )
		{
			v.addElement( accelerationUpdates.lastElement() );
		}
		else if( accelerationBufferMode == ALL_UPDATES )
		{
			for( int i = 0; i < accelerationUpdates.size(); i++ )
			{
				v.addElement( accelerationUpdates.elementAt(i) );
			}
			accelerationUpdates.removeAllElements();
		}
		return v;
	} // end method getAccelerationUpdate( )
	
	
	/**
	 * @return The last (most recent, latest) TrackerUpdate received.  This function 
	 * returns <code>null</code> if no updates have been received.  Note that
	 * successive calls to getLastTrackerUpdate() may return the same TrackerUpdate
	 * if no updates were received in the interim.
	 */
	public synchronized TrackerRemote.TrackerUpdate getLastTrackerUpdate()
	{
		if( trackerUpdates.isEmpty( ) )
			return null;
		
		return (TrackerRemote.TrackerUpdate) trackerUpdates.lastElement();
	}
	
	
	/**
	 * @return The last (most recent, latest) VelocityUpdate received.  This function 
	 * returns <code>null</code> if no updates have been received.  Note that
	 * successive calls to getLastVelocityUpdate() may return the same VelocityUpdate
	 * if no updates were received in the interim.
	 */
	public synchronized TrackerRemote.VelocityUpdate getLastVelocityUpdate()
	{
		if( velocityUpdates.isEmpty( ) )
			return null;
		
		return (TrackerRemote.VelocityUpdate) velocityUpdates.lastElement();
	}
	
	
	/**
	 * @return The last (most recent, latest) AccelerationUpdate received.  This function 
	 * returns <code>null</code> if no updates have been received.  Note that
	 * successive calls to getLastAccelerationUpdate() may return the same 
	 * AccelerationUpdate if no updates were received in the interim.
	 */
	public synchronized TrackerRemote.AccelerationUpdate getLastAccelerationUpdate()
	{
		if( accelerationUpdates.isEmpty( ) )
			return null;
		
		return (TrackerRemote.AccelerationUpdate) accelerationUpdates.lastElement();
	}
	
	
	/**
	 * This is the handler that the TrackerRemote instance will call to deliver 
	 * position updates.  This method is not intended to be called by user code.
	 */
	public synchronized void trackerPositionUpdate( TrackerRemote.TrackerUpdate u, TrackerRemote tracker )
	{
		if( trackerBufferMode == LAST_UPDATE )
		{
			trackerUpdates.removeAllElements();
		}
		trackerUpdates.addElement(u);
	}
	
	
	/**
	 * This is the handler that the TrackerRemote instance will call to deliver 
	 * velocity updates.  This method is not intended to be called by user code.
	 */
	public synchronized void trackerVelocityUpdate (TrackerRemote.VelocityUpdate v, TrackerRemote tracker)
	{
		if( velocityBufferMode == LAST_UPDATE )
		{
			velocityUpdates.removeAllElements();
		}
		velocityUpdates.addElement(v);
	}
	
	
	/**
	 * This is the handler that the TrackerRemote instance will call to deliver 
	 * acceleration updates.  This method is not intended to be called by user code.
	 */
	public synchronized void trackerAccelerationUpdate (TrackerRemote.AccelerationUpdate a, TrackerRemote tracker )
	{
		if( accelerationBufferMode == LAST_UPDATE )
		{
			accelerationUpdates.removeAllElements();
		}
		accelerationUpdates.addElement(a);
	}


	protected Vector trackerUpdates;
	protected Vector velocityUpdates;
	protected Vector accelerationUpdates;
	
	protected int trackerBufferMode;
	protected int velocityBufferMode;
	protected int accelerationBufferMode;
	
} // end class TrackerRemoteListener
