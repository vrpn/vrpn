/***************************************************************************
 * Use this class to store ForceChanges, ForceErrors and SCPChanges in a 
 * vector and get them when you want.  It is most useful if you don't want 
 * to worry about your code running in a multi-threaded environment and 
 * about vrpn possibly delivering device updates at arbitrary times.
 *
 * The Listener can be configured to buffer and return either all
 * updates or only the last (the most recent) update.  Force messages,
 * SCP messages and force-error messages are buffered independently.
 * By default, the Listener keeps only the last update of each.
 * 
 * It is not intended that Listeners be shared among objects.  Each 
 * entity in a program that is interested in hearing about updates 
 * from some vrpn Force device (and which wishes to use this Listener
 * mechanism) should create its own listener (even if multiple entities 
 * wish to hear from the same device).
 ***************************************************************************/

package vrpn;
import java.util.Vector;

public class ForceDeviceRemoteListener
	implements ForceDeviceRemote.ForceChangeListener,
	ForceDeviceRemote.ForceErrorListener,
	ForceDeviceRemote.SCPChangeListener
{
	public static final int ALL_UPDATES = 0;
	public static final int LAST_UPDATE = 1;
	
	
	public ForceDeviceRemoteListener(ForceDeviceRemote force)
	{
		forceUpdates = new Vector();
		errorUpdates = new Vector();
		scpUpdates = new Vector();
		
		forceBufferMode = LAST_UPDATE;
		errorBufferMode = LAST_UPDATE;
		scpBufferMode = LAST_UPDATE;
		
		force.addForceChangeListener(this);
		force.addForceErrorListener(this);
		force.addSCPChangeListener(this);
	}
	
	
	/**
	 * Sets the buffering mode of the Listener to record and return only 
	 * the most recent (the last, the latest) ForceChange.
	 */
	public synchronized void setModeLastForceUpdate()
	{
		forceBufferMode = LAST_UPDATE;
		if (!forceUpdates.isEmpty())
		{
			Object temp = forceUpdates.lastElement();
			forceUpdates.removeAllElements();
			forceUpdates.addElement(temp);
		}
	}
	
	
	/**
	 * Sets the buffering mode of the Listener to record and return only 
	 * the most recent (the last, the latest) ForceError.
	 */
	public synchronized void setModeLastForceErrorUpdate()
	{
		errorBufferMode = LAST_UPDATE;
		if (!errorUpdates.isEmpty())
		{
			Object temp = errorUpdates.lastElement();
			errorUpdates.removeAllElements();
			errorUpdates.addElement(temp);
		}
	}
	
	
	/**
	 * Sets the buffering mode of the Listener to record and return only 
	 * the most recent (the last, the latest) SCPChange.
	 */
	public synchronized void setModeLastSCPUpdate()
	{
		scpBufferMode = LAST_UPDATE;
		if (!scpUpdates.isEmpty())
		{
			Object temp = scpUpdates.lastElement();
			scpUpdates.removeAllElements();
			scpUpdates.addElement(temp);
		}
	}
	
	
	/**
	 * Sets the buffering mode of the Listener to record and return all 
	 * ForceChanges, beginning at the time this mode is first enabled.
	 */
	public synchronized void setModeAllForceUpdates()
	{
		if( forceBufferMode == LAST_UPDATE )
		{
			forceUpdates.removeAllElements( );
		}
		forceBufferMode = ALL_UPDATES;
	}
	
	
	/**
	 * Sets the buffering mode of the Listener to record and return all 
	 * ForceErrors, beginning at the time this mode is first enabled.
	 */
	public synchronized void setModeAllForceErrorUpdates()
	{
		if( errorBufferMode == LAST_UPDATE )
		{
			errorUpdates.removeAllElements( );
		}
		errorBufferMode = ALL_UPDATES;
	}
	
	
	/**
	 * Sets the buffering mode of the Listener to record and return all 
	 * SCPChanges, beginning at the time this mode is first enabled.
	 */
	public synchronized void setModeAllSCPUpdates()
	{
		if( forceBufferMode == LAST_UPDATE )
		{
			scpUpdates.removeAllElements( );
		}
		scpBufferMode = ALL_UPDATES;
	}
	
	
	/**
	 * @return ForceDeviceRemoteListener.ALL_UPDATES if the Listener is recording and 
	 * returning all ForceChanges; ForceDeviceRemoteListener.LAST_UPDATE if only the 
	 * latest ForceChanges.
	 */
	public synchronized int getModeForceUpdate()
	{
		return forceBufferMode;
	}
	
	
	/**
	 * @return ForceDeviceRemoteListener.ALL_UPDATES if the Listener is recording and 
	 * returning all ForceErrors; ForceDeviceRemoteListener.LAST_UPDATE if only the 
	 * latest ForceErrors.
	 */
	public synchronized int getModeForceErrorUpdate()
	{
		return errorBufferMode;
	}
	
	
	/**
	 * @return ForceDeviceRemoteListener.ALL_UPDATES if the Listener is recording and 
	 * returning all SCPUpdates; ForceDeviceRemoteListener.LAST_UPDATE if only the 
	 * latest SCPUpdates.
	 */
	public synchronized int getModeSCPUpdate()
	{
		return scpBufferMode;
	}
	
	
	/**
	 * This method retreives the buffered ForceChanges from the Listener.
	 * If the buffering mode is LAST_UPDATE, the last update received will
	 * be returned (note that, in this mode, successive calls to getForceUpdate()
	 * may return the same ForceChange if no new changes were received in the 
	 * interim).  If the buffering mode is ALL_UPDATES, all changes 
	 * received since the last call to getForceUpdate() (or since ALL_UPDATES
	 * mode was enabled) will be returned.
	 * @return A Vector containing the buffered ForceChanges.  The number of
	 * ForceChanges returned will depend on the buffering mode.  If there are
	 * no ForceChanges buffered, an empty Vector will be returned.
	 * @see #setModeLastForceUpdate
	 * @see #setModeAllForceUpdates
	 */
	public synchronized Vector getForceUpdate()
	{
		Vector v = new Vector( );
		if( forceUpdates.isEmpty() )
		{
			return v;
		}
		
		if( forceBufferMode == LAST_UPDATE )
		{
			v.addElement( forceUpdates.lastElement() );
		}
		else if( forceBufferMode == ALL_UPDATES )
		{
			for( int i = 0; i < forceUpdates.size(); i++ )
			{
				v.addElement( forceUpdates.elementAt(i) );
			}
			forceUpdates.removeAllElements();
		}
		return v;
	} // end method getForceUpdate()
	
	
	/**
	 * This method retreives the buffered ForceErrors from the Listener.
	 * If the buffering mode is LAST_UPDATE, the last update received will
	 * be returned (note that, in this mode, successive calls to getForceErrorUpdate()
	 * may return the same ForceError if no new changes were received in the 
	 * interim).  If the buffering mode is ALL_UPDATES, all changes 
	 * received since the last call to getForceErrorUpdate() (or since ALL_UPDATES
	 * mode was enabled) will be returned.
	 * @return A Vector containing the buffered ForceErrors.  The number of
	 * ForceErrors returned will depend on the buffering mode.  If there are
	 * no ForceErrors buffered, an empty Vector will be returned.
	 * @see #setModeLastForceErrorUpdate
	 * @see #setModeAllForceErrorUpdates
	 */
	public synchronized Vector getForceErrorUpdate()
	{
		Vector v = new Vector( );
		if( errorUpdates.isEmpty() )
		{
			return v;
		}
		
		if( errorBufferMode == LAST_UPDATE )
		{
			v.addElement( errorUpdates.lastElement() );
		}
		else if( errorBufferMode == ALL_UPDATES )
		{
			for( int i = 0; i < errorUpdates.size(); i++ )
			{
				v.addElement( errorUpdates.elementAt(i) );
			}
			errorUpdates.removeAllElements();
		}
		return v;
	} // end method getForceErrorUpdate()

	
	/**
	 * This method retreives the buffered SCPChanges from the Listener.
	 * If the buffering mode is LAST_UPDATE, the last update received will
	 * be returned (note that, in this mode, successive calls to getSCPUpdate()
	 * may return the same SCPChange if no new changes were received in the 
	 * interim).  If the buffering mode is ALL_UPDATES, all changes 
	 * received since the last call to getSCPUpdate() (or since ALL_UPDATES
	 * mode was enabled) will be returned.
	 * @return A Vector containing the buffered SCPChanges.  The number of
	 * SCPChanges returned will depend on the buffering mode.  If there are
	 * no SCPChanges buffered, an empty Vector will be returned.
	 * @see #setModeLastSCPUpdate
	 * @see #setModeAllSCPUpdates
	 */
	public synchronized Vector getSCPUpdate()
	{
		Vector v = new Vector( );
		if( scpUpdates.isEmpty() )
		{
			return v;
		}
		
		if( scpBufferMode == LAST_UPDATE )
		{
			v.addElement( scpUpdates.lastElement() );
		}
		else if( errorBufferMode == ALL_UPDATES )
		{
			for( int i = 0; i < scpUpdates.size(); i++ )
			{
				v.addElement( scpUpdates.elementAt(i) );
			}
			scpUpdates.removeAllElements();
		}
		return v;
	} // end method getForceErrorUpdate()

		
	/**
	 * @return The last (most recent, latest) ForceChange received.  This function 
	 * returns <code>null</code> if no updates have been received.  Note that
	 * successive calls to getLastForceUpdate() may return the same ForceChange
	 * if no updates were received in the interim.
	 */
	public synchronized ForceDeviceRemote.ForceChange getLastForceUpdate()
	{
		if( forceUpdates.isEmpty( ) )
			return null;
		
		return (ForceDeviceRemote.ForceChange) forceUpdates.lastElement();
	}
	
	
	/**
	 * @return The last (most recent, latest) ForceError received.  This function 
	 * returns <code>null</code> if no updates have been received.  Note that
	 * successive calls to getLastErrorUpdate() may return the same ForceError
	 * if no updates were received in the interim.
	 */
	public synchronized ForceDeviceRemote.ForceError getLastErrorUpdate()
	{
		if( errorUpdates.isEmpty( ) )
			return null;
		
		return (ForceDeviceRemote.ForceError) errorUpdates.lastElement();
	}
	
	
	/**
	 * @return The last (most recent, latest) SCPChange received.  This function 
	 * returns <code>null</code> if no updates have been received.  Note that
	 * successive calls to getLastSCPUpdate() may return the same SCPChange
	 * if no updates were received in the interim.
	 */
	public synchronized ForceDeviceRemote.SCPChange getLastSCPUpdate()
	{
		if( scpUpdates.isEmpty( ) )
			return null;
		
		return (ForceDeviceRemote.SCPChange) scpUpdates.lastElement();
	}
	
	
	/**
	 * This is the handler that the ForceDeviceRemote instance will call to deliver 
	 * force change messages.  This method is not intended to be called by user code.
	 */
	public synchronized void forceUpdate (ForceDeviceRemote.ForceChange f, ForceDeviceRemote force)
	{
		if( forceBufferMode == LAST_UPDATE )
		{
			forceUpdates.removeAllElements();
		}
			
		forceUpdates.addElement(f);
	}
	
	
	/**
	 * This is the handler that the ForceDeviceRemote instance will call to deliver 
	 * force error messages.  This method is not intended to be called by user code.
	 */
	public synchronized void forceError (ForceDeviceRemote.ForceError e, ForceDeviceRemote force)
	{
		if( errorBufferMode == LAST_UPDATE )
		{
			errorUpdates.removeAllElements();
		}
		
		errorUpdates.addElement(e);
	}
	
	
	/**
	 * This is the handler that the ForceDeviceRemote instance will call to deliver 
	 * SCP change messages.  This method is not intended to be called by user code.
	 */
	public synchronized void scpUpdate (ForceDeviceRemote.SCPChange s, ForceDeviceRemote force )
	{
		if( scpBufferMode == LAST_UPDATE )
		{
			scpUpdates.removeAllElements();
		}
		
		scpUpdates.addElement(s);
	}
	
	
	protected Vector forceUpdates;
	protected Vector errorUpdates;
	protected Vector scpUpdates;
	
	protected int forceBufferMode;
	protected int errorBufferMode;
	protected int scpBufferMode;

} // end class ForceDeviceRemoteListener
