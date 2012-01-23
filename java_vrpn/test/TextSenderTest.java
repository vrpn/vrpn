
import vrpn.*;

public class TextSenderTest
{
	public static void main(String[] args)
	{
		String textSenderName = "testText";
		TextSender r = null;
		try
		{
			r = new TextSender( textSenderName, null, null, null, null );
		}
		catch( InstantiationException e )
		{
			// do something b/c you couldn't create the text sender
			System.out.println( "We couldn't connect text sender to " + textSenderName + "." );
			System.out.println( e.getMessage( ) );
			return;
		}
		
		int msgCount = 1;
		while(true)
		{
			String msg = "hello from TextSenderTest (" + msgCount++ + ")";
			System.out.println( "sending:  " + msg );
			r.sendMessage( msg );
			try{ Thread.sleep( 1000 ); }
			catch( InterruptedException e ) { }
		}
	}
}
