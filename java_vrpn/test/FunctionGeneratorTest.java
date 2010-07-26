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
							"\ttime:  " + r.msg_time.getTime( ) +
							"\t#:  " + r.channelNumber + 
							"\t(with function type " + r.channel.function.getClass() + ")" );
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
							"\ttime:  " + r.msg_time.getTime( ) + "\t(" + r.description + ")" );
		System.out.flush();
		
	}

	public void fgSampleRateReply( SampleRateReply r, FunctionGeneratorRemote fg )
	{
		System.out.println( "FunctionGenerator sample-rate message:  " +
							"\ttime:  " + r.msg_time.getTime( ) + "\trate:  " + r.sampleRate );
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
		String functionGeneratorName = "hexa_force_server@iodine:4700";
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
		
		fg.requestSampleRate( 10000 );
		try { Thread.sleep( 1000 ); }
		catch( InterruptedException e ) { }

		// this is before anything has started, so nothing should happen
		fg.requestStop();
		try { Thread.sleep( 1000 ); }
		catch( InterruptedException e ) { }
	
		//fg.requestAllChannels();
		fg.requestChannel( -1 ); // invalid channel
		fg.requestChannel( 0 );
		fg.requestChannel( 1 );
		fg.requestChannel( 2 );
		try { Thread.sleep( 1000 ); }
		catch( InterruptedException e ) { }

		String theScript = "doMagnetFunction( 0.15, @magnetConstant, 1 );"
			+ "doMagnetFunction( 0.4, @magnetConstant, 2 );"
		+ "doMagnetFunction( 0.9, @magnetConstant, 3 );"
		+ "doMagnetFunction( 1.72, @magnetConstant, -1 );"
		+ "doMagnetFunction( 2, @magnetConstant, 2 );";
		//String theScript = " #$%^Q %$%^@$%QRQGQA";
		FunctionGeneratorRemote.Function func 
			= new FunctionGeneratorRemote.Function_script( theScript );
		FunctionGeneratorRemote.Channel chan = new FunctionGeneratorRemote.Channel();
		chan.function = func;
		fg.setChannel( 0, chan );
		fg.setChannel( 1, chan );
		fg.setChannel( 2, chan );
		fg.setChannel( 3, chan );
		fg.setChannel( 4, chan );
		fg.setChannel( 5, chan );
		try { Thread.sleep( 1000 ); }
		catch( InterruptedException e ) { }

		fg.setChannel( -1, chan );  // invalid channel
		try { Thread.sleep( 1000 ); }
		catch( InterruptedException e ) { }
		
		/*
		func = new FunctionGeneratorRemote.Function_NULL( );
		chan.function = func;
		fg.setChannel( 2, chan );
		*/
		
		fg.requestStart();
		try { Thread.sleep( 10000 ); }
		catch( InterruptedException e ) { }
	
		fg.requestStop();
		try { Thread.sleep( 1000 ); }
		catch( InterruptedException e ) { }
		

	}

}
