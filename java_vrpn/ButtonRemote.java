package vrpn;
import java.util.*;

public class ButtonRemote implements Runnable
{
	
	//////////////////
	// Public structures and interfaces
	
	public class ButtonUpdate
	{
		public Date msg_time = new Date( );
		public int button = 0; //Which button.  Starts from 0
		public int state = 0; //0 for off, 1 for on
	}
	
	public interface ButtonChangeListener
	{
		public void buttonUpdate( ButtonUpdate u, ButtonRemote button );
	}

	// end of the public structures and interfaces
	///////////////////
	
	///////////////////
	// Public methods
	
	public ButtonRemote( String name ) throws InstantiationException
	{
		try { this.init( name ); }
		catch( java.lang.UnsatisfiedLinkError e )
		{  
			System.out.println( "Error initializing remote button " + name + "." );
			System.out.println( " -- Unable to find the right functions.  This may be a version problem." );
			throw new InstantiationException( e.getMessage( ) );
		}
		
		this.buttonThread = new Thread( this );
		this.buttonThread.start( );
		
	}
	
	public synchronized void addButtonChangeListener( ButtonChangeListener listener )
	{
		changeListeners.addElement( listener );
	}
	
	/**
	 * @return true on success; false on failure
	 */
	public synchronized boolean removeButtonChangeListener( ButtonChangeListener listener )
	{
		return changeListeners.removeElement( listener );
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
	 * Stops the button device thread
	 */
	public void stopRunning( )
	{
		keepRunning = false;
	}

	
	/**
	 * This should <b>not</b> be called by user code.
	 */
	public void run( )
	{
		while( keepRunning )
		{
			this.mainloop( );
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
	 * a given ButtonRemote object, this method can only be called from one
	 * thread at a time).
	 */
	protected void handleButtonChange( long tv_sec, long tv_usec, int button,
									   int state )
	{
		// putting the body of this function into a synchronized block prevents
		// other instances of ButtonRemote from calling listeners' buttonPositionUpdate
		// method concurrently.
		synchronized( notifyingChangeListenersLock )
		{
			ButtonUpdate b = new ButtonUpdate();
			b.msg_time.setTime( tv_sec * 1000 + tv_usec );
			b.button = button;
			b.state = state;
			
			// notify all listeners
			Enumeration e = changeListeners.elements( );
			while( e.hasMoreElements( ) )
			{
				ButtonChangeListener l = (ButtonChangeListener) e.nextElement( );
				l.buttonUpdate( b, this );
			}
		} // end synchronized( notifyingChangeListenersLock )
	} // end method handleButtonChange
	
	
	/**
	 * Initialize the native button object
	 * @param name The name of the button and host (e.g., <code>"Button0@myhost.edu"</code>).
	 * @return <code>true</code> if the button was connected successfully, 
	 *			<code>false</code> otherwise.
	 */
	protected native boolean init( String name );

	/**
	 * This should only be called from the method finalize()
	 */
	protected native void shutdownButton( );
	
	protected synchronized native void mainloop( );
	
	protected void finalize( ) throws Throwable
	{
		keepRunning = false;
		while( buttonThread.isAlive( ) )
		{
			try { buttonThread.join( ); }
			catch( InterruptedException e ) { }
		}
		changeListeners.removeAllElements( );
		this.shutdownButton( );
	}
	
	
	// end protected methods
	///////////////////////
	
	///////////////////
	// data members
	
        // this is used by the native code to store a C++ pointer to the
        // native vrpn_ButtonRemote object
        protected int native_button = -1;

	// this is used to stop and to keep running the tracking thread
	// in an orderly fashion.
	protected boolean keepRunning = true;
	
	// the tracking thread
	Thread buttonThread = null;

	// how long the thread sleeps between checking for messages
	protected long mainloopPeriod = 100; // milliseconds
	
	protected Vector changeListeners = new Vector( );
	
	/**
	 * these notifying*ListenersLock variables are used to ensure that multiple
	 * ButtonRemote objects running in multiple threads don't call the 
	 * buttonChangeUpdate, et. al., method of some single object concurrently.
	 * For example, the handleButtonChange(...) method, which is invoked from native 
	 * code, gets a lock on the notifyingChangeListenersLock object.  Since that object
	 * is static, all other instances of ButtonRemote must wait before notifying 
	 * their listeners and completing their handleButtonChange(...) methods.
	 * They are necessary, in part, because methods in an interface can't be declared
	 * synchronized (and the semantics of the keyword 'synchronized' aren't what's
	 * wanted here, anyway -- we want synchronization across all instances, not just a 
	 * single object).
	 */
	protected final static Object notifyingChangeListenersLock = new Object( );

	// static initialization
	static 
	{
		System.loadLibrary( "ButtonRemote" );
		try { System.loadLibrary( "ButtonRemote" ); }
		catch( UnsatisfiedLinkError e )
		{
			System.out.println( e.getMessage( ) );
			System.out.println( "Error initializing remote button device." );
			System.out.println( " -- Unable to find native library." );
		}
		catch( SecurityException e )
		{
			System.out.println( e.getMessage( ) );
			System.out.println( "Security exception:  you couldn't load the native button remote dll." );
		}

	}
	
}


