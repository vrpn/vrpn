/***************************************************************************
 * Use this class to store ButtonUpdate in a vector and get them when you 
 * want.  It is most useful if you don't want to worry about your code 
 * running in a multi-threaded environment and about vrpn possibly deliver-
 * ing device updates at arbitrary times.
 *
 * The Listener can be configured to buffer and return either all
 * updates or only the last (the most recent) update.  By default, it
 * keeps only the last update.
 * 
 * It is not intended that Listeners be shared among objects.  Each 
 * entity in a program that is interested in hearing about updates 
 * from some vrpn Button device (and which wishes to use this Listener
 * mechanism) should create its own listener (even if multiple entities 
 * wish to hear from the same device).
 ***************************************************************************/

package vrpn;
import java.util.Vector;

public class ButtonRemoteListener implements ButtonRemote.ButtonChangeListener
{
	public static final int ALL_UPDATES = 0;
	public static final int LAST_UPDATE = 1;
	
	
	public ButtonRemoteListener(ButtonRemote button)
	{
		buttonUpdates = new Vector();
		bufferMode = LAST_UPDATE;
		button.addButtonChangeListener(this);
	}
	
	
	/**
	 * Sets the buffering mode of the Listener to record and return only 
	 * the most recent (the last, the latest) ButtonUpdate.
	 */
	public synchronized void setModeLastButtonUpdate()
	{
		bufferMode = LAST_UPDATE;
		if (!buttonUpdates.isEmpty())
		{
			Object temp = buttonUpdates.lastElement();
			buttonUpdates.removeAllElements();
			buttonUpdates.addElement(temp);
		}
	}
	
	
	/**
	 * Sets the buffering mode of the Listener to record and return all 
	 * ButtonUpdates, beginning at the time this mode is first enabled.
	 */
	public synchronized void setModeAllButtonUpdates()
	{
		if( bufferMode == LAST_UPDATE )
		{
			buttonUpdates.removeAllElements( );
		}
		bufferMode = ALL_UPDATES;
	}
	
	
	/**
	 * @return ButtonRemoteListener.ALL_UPDATES if the Listener is recording and 
	 * returning all ButtonUpdates; ButtonRemoteListener.LAST_UPDATE if only the 
	 * latest ButtonUpdate.
	 */
	public synchronized int getModeButtonUpdate()
	{
		return bufferMode;
	}
	
	
	/**
	 * This method retreives the buffered ButtonUpdates from the Listener.
	 * If the buffering mode is LAST_UPDATE, the last update received will
	 * be returned (note that, in this mode, successive calls to getButtonUpdate()
	 * may return the same ButtonUpdate if no new updates were received in the 
	 * interim).  If the buffering mode is ALL_UPDATES, all updates 
	 * received since the last call to getButtonUpdate() (or since ALL_UPDATES
	 * mode was enabled) will be returned.
	 * @return A Vector containing the buffered ButtonUpdates.  The number of
	 * ButtonUpdates returned will depend on the buffering mode.  If there are
	 * no ButtonUpdates buffered, an empty Vector will be returned.
	 * @see #setModeLastButtonUpdate
	 * @see #setModeAllButtonUpdates
	 */
	public synchronized Vector getButtonUpdate()
	{
		Vector v = new Vector( );
		if( buttonUpdates.isEmpty() )
		{
			return v;
		}
		
		if( bufferMode == LAST_UPDATE )
		{
			v.addElement( buttonUpdates.lastElement() );
		}
		else if( bufferMode == ALL_UPDATES )
		{
			for( int i = 0; i < buttonUpdates.size(); i++ )
			{
				v.addElement( buttonUpdates.elementAt(i) );
			}
			buttonUpdates.removeAllElements();
		}
		return v;
	} // end method getButtonUpdate()
	
	
	/**
	 * @return The last (most recent, latest) ButtonUpdate received.  This function 
	 * returns <code>null</code> if no updates have been received.  Note that
	 * successive calls to getLastButtonUpdate() may return the same ButtonUpdate
	 * if no updates were received in the interim.
	 */
	public synchronized ButtonRemote.ButtonUpdate getLastButtonUpdate()
	{
		if( buttonUpdates.isEmpty( ) )
			return null;

		return (ButtonRemote.ButtonUpdate) buttonUpdates.lastElement();
	}
	
	
	/**
	 * This is the handler that the ButtonRemote instance will call to deliver updates.
	 * This method is not intended to be called by user code.
	 */
	public synchronized void buttonUpdate (ButtonRemote.ButtonUpdate u, ButtonRemote button)
	{
		if( bufferMode == LAST_UPDATE )
		{
			buttonUpdates.removeAllElements();
		}
		buttonUpdates.addElement(u);
	}

	
	protected Vector buttonUpdates;	
	protected int bufferMode;
	
} // end class ButtonRemoteListener
