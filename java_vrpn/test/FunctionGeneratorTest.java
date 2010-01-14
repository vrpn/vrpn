import vrpn.*;
import vrpn.FunctionGeneratorRemote.ChannelReply;
import vrpn.FunctionGeneratorRemote.ErrorReply;
import vrpn.FunctionGeneratorRemote.InterpreterReply;
import vrpn.FunctionGeneratorRemote.SampleRateReply;
import vrpn.FunctionGeneratorRemote.StartReply;
import vrpn.FunctionGeneratorRemote.StopReply;

public class FunctionGeneratorTest
		implements vrpn.FunctionGeneratorRemote.ChannelReplyListener,
		vrpn.FunctionGeneratorRemote.StartStopReplyListener,
		vrpn.FunctionGeneratorRemote.InterpreterReplyListener,
		vrpn.FunctionGeneratorRemote.SampleRateReplyListener,
		vrpn.FunctionGeneratorRemote.ErrorListener
{

	public void fgChannelReply( ChannelReply r, FunctionGeneratorRemote fg )
	{
		System.out.println( "FunctionGenerator channel message:  " +
							"\ttime:  " + r.msg_time.getTime( ) );
		System.out.flush();
		
	}

	public void fgStartReply( StartReply r, FunctionGeneratorRemote fg )
	{
		System.out.println( "FunctionGenerator start message:  " +
							"\ttime:  " + r.msg_time.getTime( ) );
		System.out.flush();
		
	}

	public void fgStopReply( StopReply r, FunctionGeneratorRemote fg )
	{
		System.out.println( "FunctionGenerator stop message:  " +
							"\ttime:  " + r.msg_time.getTime( ) );
		System.out.flush();
		
	}

	public void fgInterpreterReply( InterpreterReply r, FunctionGeneratorRemote fg )
	{
		System.out.println( "FunctionGenerator interpreter-description message:  " +
							"\ttime:  " + r.msg_time.getTime( ) );
		System.out.flush();
		
	}

	public void fgSampleRateReply( SampleRateReply r, FunctionGeneratorRemote fg )
	{
		System.out.println( "FunctionGenerator sample-rate message:  " +
							"\ttime:  " + r.msg_time.getTime( ) );
		System.out.flush();
		
	}

	public void fgErrorReply( ErrorReply r, FunctionGeneratorRemote fg )
	{
		System.out.println( "FunctionGenerator error message:  " +
							"\ttime:  " + r.msg_time.getTime( ) );
		System.out.flush();
		
	}

	
	public static void main( String[] args )
	{
		String functionGeneratorName = "hexa_force_server@localhost:4700";
		FunctionGeneratorRemote fg = null;
		try
		{
			System.out.println( "connecting to " + functionGeneratorName );
			fg = new FunctionGeneratorRemote( functionGeneratorName, null, null, null, null );
		}
		catch( InstantiationException e )
		{
			// do something b/c you couldn't create the function generator
			System.out.println( "We couldn't connect to function generator " + functionGeneratorName + "." );
			System.out.println( e.getMessage( ) );
			return;
		}
		
		FunctionGeneratorTest test = new FunctionGeneratorTest( );
		fg.addChannelReplyListener( test );
		fg.addStartStopReplyListener( test );
		fg.addSampleRateReplyListener( test );
		fg.addInterpreterReplyListener( test );
		fg.addErrorListener( test );
		
		for( int i = 0; i <= 9;  i++ )
			try { Thread.sleep( 100 ); }
			catch( InterruptedException e ) { }
		
		fg.requestSampleRate( 10 );
		try { Thread.sleep( 1000 ); }
		catch( InterruptedException e ) { }
		
		fg.requestStart();
		try { Thread.sleep( 1000 ); }
		catch( InterruptedException e ) { }
	
		fg.requestStop();
		try { Thread.sleep( 1000 ); }
		catch( InterruptedException e ) { }
		
		//fg.requestAllChannels();
		fg.requestChannel( -1 ); // illegal channel
		fg.requestChannel( 0 );
		fg.requestChannel( 1 );
		fg.requestChannel( 2 );
		try { Thread.sleep( 1000 ); }
		catch( InterruptedException e ) { }
		
		FunctionGeneratorRemote.Function func = new FunctionGeneratorRemote.Function_script( "blah blah blah" );
		FunctionGeneratorRemote.Channel chan = new FunctionGeneratorRemote.Channel();
		chan.function = func;
		fg.setChannel( 2, chan );
		try { Thread.sleep( 1000 ); }
		catch( InterruptedException e ) { }

		fg.setChannel( -1, chan );
		try { Thread.sleep( 1000 ); }
		catch( InterruptedException e ) { }

		func = new FunctionGeneratorRemote.Function_NULL( );
		chan.function = func;
		fg.setChannel( 2, chan );
	}

}
