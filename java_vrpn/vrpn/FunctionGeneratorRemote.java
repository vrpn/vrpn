package vrpn;
import java.util.*;


public class FunctionGeneratorRemote extends VRPNDevice implements Runnable
{
	//////////////////
	// Public structures and interfaces
	
	public static interface Function { }
	
	public static class Function_NULL implements Function { }
	
	public static class Function_script implements Function
	{
		public String script = "";
		public Function_script( String script ) { this.script = script; }
	}
	
	public static class Channel
	{
		public Function function = new Function_NULL();
		
		public Channel( Function f )
		{
			this.function = f;
		}
		public Channel() {}
	}
	
	public class ChannelReply
	{
		public Date msg_time = new Date();
		public int channelNumber = 0;
		public Channel channel = new Channel();
	}
	
	public interface ChannelReplyListener
	{
		public void fgChannelReply( ChannelReply r, FunctionGeneratorRemote fg );
	}
	
	public class StartReply
	{
		public Date msg_time = new Date();
		public boolean isStarted = false;
	}
	
	public class StopReply
	{
		public Date msg_time = new Date();
		public boolean isStopped = false;
	}
	
	public interface StartStopReplyListener
	{
		public void fgStartReply( StartReply r, FunctionGeneratorRemote fg );
		public void fgStopReply( StopReply r, FunctionGeneratorRemote fg );
	}
	
	public class SampleRateReply
	{
		public Date msg_time = new Date();
		public double sampleRate = 0;
	}
	
	public interface SampleRateReplyListener
	{
		public void fgSampleRateReply( SampleRateReply r, FunctionGeneratorRemote fg );
	}
	
	public class InterpreterReply
	{
		public Date msg_time = new Date();
		public String description = "";
	}
	
	public interface InterpreterReplyListener
	{
		public void fgInterpreterReply( InterpreterReply r, FunctionGeneratorRemote fg );
	}
	
	public enum FGError
	{
		NO_FG_ERROR (0),
		INTERPRETER_ERROR (1),
		TAKING_TOO_LONG (2),
		INVALID_RESULT_QUANTITY (3),
		INVALID_RESULT_RANGE (4),
		
		UNKNOWN_ERROR (9999);
		
		private final int errorCode;
		private FGError( int err ) { this.errorCode = err; }
		public static FGError getErrorFromCode( int err )
		{
			for( FGError fgerror : FGError.values() )
			{
				if( err == fgerror.errorCode )
					return fgerror;
			}
			return UNKNOWN_ERROR;
		}
	}
	
	public class ErrorReply
	{
		public Date msg_time = new Date();
		public FGError error;
		public int channel;
	}
	
	public interface ErrorListener
	{
		public void fgErrorReply( ErrorReply r, FunctionGeneratorRemote fg );
	}
	
	// end of the public structures and interfaces
	//////////////////////////////////
	

	////////////////////////////////
	// Public methods
	//
		
	/**
	 * @exception java.lang.InstantiationException
	 *		If the function generator could not be created because of problems with
	 *      its native code and linking.
	 */
	public FunctionGeneratorRemote( String name, String localInLogfileName, String localOutLogfileName,
						  			String remoteInLogfileName, String remoteOutLogfileName ) 
		throws InstantiationException
	{
		super( name, localInLogfileName, localOutLogfileName, remoteInLogfileName, remoteOutLogfileName );
	}
	
	
	public boolean setChannel( int channelNumber, Channel c )
	{
		boolean retval = false;
		synchronized( downInVrpnLock )
		{
			if( c.function instanceof Function_NULL )
				retval = setChannelNULL_native( channelNumber );
			else if( c.function instanceof Function_script )
				retval = setChannelScript_native( channelNumber, ((Function_script)c.function).script );
		}
		return retval;
	}
	
	
	public boolean requestChannel( int channelNumber )
	{
		boolean retval = false;
		synchronized( downInVrpnLock )
		{
			retval = requestChannel_native( channelNumber );
		}
		return retval;
	}
	
	
	public boolean requestAllChannels( )
	{
		boolean retval = false;
		synchronized( downInVrpnLock )
		{
			retval = requestAllChannels_native( );
		}
		return retval;
	}
	
	
	public boolean requestStart( )
	{
		boolean retval = false;
		synchronized( downInVrpnLock )
		{
			retval = requestStart_native( );
		}
		return retval;
	}
	
	
	public boolean requestStop( )
	{
		boolean retval = false;
		synchronized( downInVrpnLock )
		{
			retval = requestStop_native( );
		}
		return retval;
	}
	
	
	public boolean requestSampleRate( float rate )
	{
		boolean retval = false;
		synchronized( downInVrpnLock )
		{
			retval = requestSampleRate_native( rate );
		}
		return retval;
	}
	
	
	public boolean requestInterpreterDescription( )
	{
		boolean retval = false;
		synchronized( downInVrpnLock )
		{
			retval = requestInterpreterDescription_native( );
		}
		return retval;
	}
	
	
	public synchronized void addChannelReplyListener( ChannelReplyListener listener )
	{
		channelListeners.addElement( listener );
	}


	public synchronized boolean removeChannelReplyListener( ChannelReplyListener listener )
	{
		return channelListeners.removeElement( listener );
	}
	
	
	public synchronized void addStartStopReplyListener( StartStopReplyListener listener )
	{
		startStopListeners.addElement( listener );
	}


	public synchronized boolean removeStartStopReplyListener( StartStopReplyListener listener )
	{
		return startStopListeners.removeElement( listener );
	}
	
	
	public synchronized void addSampleRateReplyListener( SampleRateReplyListener listener )
	{
		sampleRateListeners.addElement( listener );
	}


	public synchronized boolean removeSampleRateReplyListener( SampleRateReplyListener listener )
	{
		return sampleRateListeners.removeElement( listener );
	}
	
	
	public synchronized void addInterpreterReplyListener( InterpreterReplyListener listener )
	{
		interpreterListeners.addElement( listener );
	}


	public synchronized boolean removeInterpreterReplyListener( InterpreterReplyListener listener )
	{
		return interpreterListeners.removeElement( listener );
	}
	
	
	public synchronized void addErrorListener( ErrorListener listener )
	{
		errorListeners.addElement(  listener );
	}
	
	
	public synchronized boolean removeErrorListener( ErrorListener listener )
	{
		return errorListeners.removeElement( listener );
	}
	
	
	// end public methods
	////////////////////////
	
	
	
	////////////////////////
	// Protected methods
	//
	
	protected native boolean setChannelNULL_native( int channelNumber );
	protected native boolean setChannelScript_native( int channelNumber, String script );
	protected native boolean requestChannel_native( int channelNumber );
	protected native boolean requestAllChannels_native( );
	protected native boolean requestStart_native( );
	protected native boolean requestStop_native( );
	protected native boolean requestSampleRate_native( float rate );
	protected native boolean requestInterpreterDescription_native( );
	
	
	protected void stoppedRunning()
	{
		// remove all listeners
		channelListeners.removeAllElements();
		startStopListeners.removeAllElements();
		sampleRateListeners.removeAllElements();
		interpreterListeners.removeAllElements();
		errorListeners.removeAllElements();
		
		// shut down the native object
		synchronized( downInVrpnLock )
		{
			this.shutdownFunctionGenerator( );
		}
	}

	
	/**
	 * Should be called only by mainloop(), a native method which is itself
	 * synchronized.  By extension, this method is synchronized (in that, for
	 * a given FunctionGeneratorRemote object, this method can only be called from one
	 * thread at a time).
	 */
	protected void handleChannelChange_NULL( long tv_sec, long tv_usec,  int channelNumber )
	{
		// putting the body of this function into a synchronized block prevents
		// other instances of FunctionGeneratorRemote from calling listeners' fgChannelReply
		// method concurrently.
		synchronized( notifyingChannelListenersLock )
		{
			ChannelReply c = new ChannelReply();
			c.msg_time.setTime( tv_sec * 1000 + (int) (tv_usec/1000.0) );
			c.channelNumber = channelNumber;
			c.channel = new Channel();
			
			// notify all listeners
			Enumeration e = channelListeners.elements( );
			while( e.hasMoreElements( ) )
			{
				ChannelReplyListener l = (ChannelReplyListener) e.nextElement( );
				l.fgChannelReply( c, this );
			}
		} 
	} // end method handleChannelChange
	
	
	protected void handleChannelChange_Script( long tv_sec, long tv_usec,  int channelNumber,
												String script )
	{
		// putting the body of this function into a synchronized block prevents
		// other instances of FunctionGeneratorRemote from calling listeners' fgChannelReply
		// method concurrently.
		synchronized( notifyingChannelListenersLock )
		{
			ChannelReply c = new ChannelReply();
			c.msg_time.setTime( tv_sec * 1000 + (int) (tv_usec/1000.0) );
			c.channelNumber = channelNumber;
			c.channel = new Channel();
			c.channel.function = new Function_script( script );
			
			// notify all listeners
			Enumeration e = channelListeners.elements( );
			while( e.hasMoreElements( ) )
			{
				ChannelReplyListener l = (ChannelReplyListener) e.nextElement( );
				l.fgChannelReply( c, this );
			}
		} 
	} // end method handleChannelChange
	
	
	protected void handleStartReply( long tv_sec, long tv_usec, boolean isStarted )
	{
		// putting the body of this function into a synchronized block prevents
		// other instances of FunctionGeneratorRemote from calling listeners' fgStartReply
		// method concurrently.
		synchronized( notifyingStartStopListenersLock )
		{
			StartReply s = new StartReply();
			s.msg_time.setTime( tv_sec * 1000 + (int) (tv_usec/1000.0) );
			s.isStarted = isStarted;
			
			// notify all listeners
			Enumeration e = startStopListeners.elements( );
			while( e.hasMoreElements( ) )
			{
				StartStopReplyListener l = (StartStopReplyListener) e.nextElement( );
				l.fgStartReply( s, this );
			}
		} 
	}
	
	
	protected void handleStopReply( long tv_sec, long tv_usec, boolean isStopped )
	{
		// putting the body of this function into a synchronized block prevents
		// other instances of FunctionGeneratorRemote from calling listeners' fgStopReply
		// method concurrently.
		synchronized( notifyingStartStopListenersLock )
		{
			StopReply s = new StopReply();
			s.msg_time.setTime( tv_sec * 1000 + (int) (tv_usec/1000.0) );
			s.isStopped = isStopped;
			
			// notify all listeners
			Enumeration e = startStopListeners.elements( );
			while( e.hasMoreElements( ) )
			{
				StartStopReplyListener l = (StartStopReplyListener) e.nextElement( );
				l.fgStopReply( s, this );
			}
		} 
	}
	
	
	protected void handleSampleRateReply( long tv_sec, long tv_usec, double rate )
	{
		// putting the body of this function into a synchronized block prevents
		// other instances of FunctionGeneratorRemote from calling listeners' fgSampleRateReply
		// method concurrently.
		synchronized( notifyingSampleRateListenersLock )
		{
			SampleRateReply s = new SampleRateReply();
			s.msg_time.setTime( tv_sec * 1000 + (int) (tv_usec/1000.0) );
			s.sampleRate = rate;
			
			// notify all listeners
			Enumeration e = sampleRateListeners.elements( );
			while( e.hasMoreElements( ) )
			{
				SampleRateReplyListener l = (SampleRateReplyListener) e.nextElement( );
				l.fgSampleRateReply( s, this );
			}
		} 	
	}
	
	
	protected void handleInterpreterDescription( long tv_sec, long tv_usec, String desc )
	{
		// putting the body of this function into a synchronized block prevents
		// other instances of FunctionGeneratorRemote from calling listeners' forceUpdate
		// method concurrently.
		synchronized( notifyingInterpreterListenersLock )
		{
			InterpreterReply r = new InterpreterReply();
			r.msg_time.setTime( tv_sec * 1000 + (int) (tv_usec/1000.0) );
			r.description = desc;
			
			// notify all listeners
			Enumeration e = interpreterListeners.elements( );
			while( e.hasMoreElements( ) )
			{
				InterpreterReplyListener l = (InterpreterReplyListener) e.nextElement( );
				l.fgInterpreterReply( r, this );
			}
		} 
	}
	
	
	protected void handleErrorReport( long tv_sec, long tv_usec, int err, int channel )
	{
		// putting the body of this function into a synchronized block prevents
		// other instances of FunctionGeneratorRemote from calling listeners' fgErrorReply
		// method concurrently.
		synchronized( notifyingErrorListenersLock )
		{
			ErrorReply r = new ErrorReply();
			r.msg_time.setTime( tv_sec * 1000 + (int) (tv_usec/1000.0) );
			r.error = FGError.getErrorFromCode( err );
			r.channel = channel;
			
			// notify all listeners
			Enumeration e = errorListeners.elements( );
			while( e.hasMoreElements( ) )
			{
				ErrorListener l = (ErrorListener) e.nextElement( );
				l.fgErrorReply( r, this );
			}
		} 
	}

	
	/**
	 * Initialize the native function generator object
	 * @param name The name of the function generator and host (e.g., <code>"Phantom0@myhost.edu"</code>).
	 * @return <code>true</code> if the force device was connected successfully, 
	 *			<code>false</code> otherwise.
	 */
	protected native boolean init( String name, String localInLogfileName,
									String localOutLogfileName, String remoteInLogfileName,
									String remoteOutLogfileName );

	/**
	 * This should only be called from the method finalize()
	 */
	protected native void shutdownFunctionGenerator( );
	
	
	protected native void mainloop( );

	// end protected methods
	///////////////////////
	
	///////////////////
	// data members
	
	protected Vector channelListeners = new Vector();
	protected Vector startStopListeners = new Vector();
	protected Vector sampleRateListeners = new Vector();
	protected Vector interpreterListeners = new Vector();
	protected Vector errorListeners = new Vector();

	/**
	 * @see vrpn.TrackerRemote#notifyingChangeListenersLock
	 */
	protected final static Object notifyingChannelListenersLock = new Object( );
	protected final static Object notifyingStartStopListenersLock = new Object( );
	protected final static Object notifyingSampleRateListenersLock = new Object( );
	protected final static Object notifyingInterpreterListenersLock = new Object( );
	protected final static Object notifyingErrorListenersLock = new Object( );
	

}
