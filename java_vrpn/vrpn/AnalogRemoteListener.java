/***************************************************************************
 * Use this class to store AnalogUpdates in a vector and get them when 
 * you want.  It is most useful if you don't want to worry about your code 
 * running in a multi-threaded environment and about vrpn possibly deliver-
 * ing device updates at arbitrary times.
 *
 * The Listener can be configured to buffer and return either all
 * updates or only the last (the most recent) update.  By default, it
 * keeps only the last update.
 * 
 * It is not intended that Listeners be shared among objects.  Each 
 * entity in a program that is interested in hearing about updates 
 * from some vrpn Analog device (and which wishes to use this Listener
 * mechanism) should create its own listener (even if multiple entities 
 * wish to hear from the same device).
 ***************************************************************************/

package vrpn;
import java.util.Vector;

public class AnalogRemoteListener implements AnalogRemote.AnalogChangeListener
{
	public static final int ALL_UPDATES = 0;
	public static final int LAST_UPDATE = 1;
	
	
	public AnalogRemoteListener( AnalogRemote analog )
	{
		analogUpdates = new Vector();		
		bufferMode = LAST_UPDATE;
		analog.addAnalogChangeListener(this);
	}
	
	
	/**
	 * Sets the buffering mode of the Listener to record and return only 
	 * the most recent (the last, the latest) AnalogUpdate.
	 */
	public synchronized void setModeLastAnalogUpdate()
	{
		bufferMode = LAST_UPDATE;
		if (!analogUpdates.isEmpty())
		{
			Object temp = analogUpdates.lastElement();
			analogUpdates.removeAllElements();
			analogUpdates.addElement(temp);
		}
	}
	
	
	/**
	 * Sets the buffering mode of the Listener to record and return all 
	 * AnalogUpdates, beginning at the time this mode is first enabled.
	 */
	public synchronized void setModeAllAnalogUpdates()
	{
		if( bufferMode == LAST_UPDATE )
		{
			analogUpdates.removeAllElements( );
		}
		bufferMode = ALL_UPDATES;
	}
	

	/**
	 * @return AnalogRemoteListener.ALL_UPDATES if the Listener is recording and 
	 * returning all AnalogUpdates; AnalogRemoteListener.LAST_UPDATE if only the 
	 * latest AnalogUpdate.
	 */
	public synchronized int getModeAnalogUpdate()
	{
		return bufferMode;
	}
	
	
	/**
	 * This method retreives the buffered AnalogUpdates from the Listener.
	 * If the buffering mode is LAST_UPDATE, the last update received will
	 * be returned (note that, in this mode, successive calls to getAnalogUpdate()
	 * may return the same AnalogUpdate if no new updates were received in the 
	 * interim).  If the buffering mode is ALL_UPDATES, all updates 
	 * received since the last call to getAnalogUpdate() (or since ALL_UPDATES
	 * mode was enabled) will be returned.
	 * @return A Vector containing the buffered AnalogUpdates.  The number of
	 * AnalogUpdates returned will depend on the buffering mode.  If there are
	 * no AnalogUpdates buffered, an empty Vector will be returned.
	 * @see #setModeLastAnalogUpdate
	 * @see #setModeAllAnalogUpdates
	 */
	public synchronized Vector getAnalogUpdate()
	{
		Vector v = new Vector( );
		if( analogUpdates.isEmpty() )
		{
			return v;
		}
		
		if( bufferMode == LAST_UPDATE )
		{
			v.addElement(analogUpdates.lastElement());
		}
		else if( bufferMode == ALL_UPDATES )
		{
			for( int i = 0; i < analogUpdates.size(); i++ )
			{
				v.addElement( analogUpdates.elementAt(i) );
			}
			analogUpdates.removeAllElements();
		}
		return v;
	} // end method getAnalogUpdate()

	
	/**
	 * @return The last (most recent, latest) AnalogUpdate received.  This function 
	 * returns <code>null</code> if no updates have been received.  Note that
	 * successive calls to getLastAnalogUpdate() may return the same AnalogUpdate
	 * if no updates were received in the interim.
	 */
	public synchronized AnalogRemote.AnalogUpdate getLastAnalogUpdate()
	{
		if( analogUpdates.isEmpty( ) )
			return null;
		
		return (AnalogRemote.AnalogUpdate) analogUpdates.lastElement();
	}
	
	
	/**
	 * This is the handler that the AnalogRemote instance will call to deliver updates.
	 * This method is not intended to be called by user code.
	 */
	public synchronized void analogUpdate (AnalogRemote.AnalogUpdate u, AnalogRemote analog)
	{
		if( bufferMode == LAST_UPDATE )
		{
			analogUpdates.removeAllElements();
		}
		analogUpdates.addElement(u);
	}

	
	protected Vector analogUpdates;
	protected int bufferMode;
	
} // end class AnalogRemoteListener
