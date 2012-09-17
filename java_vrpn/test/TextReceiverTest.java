
import vrpn.*;

public class TextReceiverTest implements TextReceiver.TextMessageListener
{
	public void receiveTextMessage( TextReceiver.TextMessage t, TextReceiver r )
	{
		System.out.println( "Text message from vrpn:  \n" +
							"\ttime:  " + t.msg_time.getTime() + 
							"\tseverity:  " + t.type + "(" + TextReceiver.getTextSeverityString( t.type ) + ")" +
							"\tlevel:  " + t.level + "\n" +
							"\tmessage:  " + t.msg
							);

	}
	
	
	public static void main(String[] args)
	{
		String textSenderName = "testText@localhost";
		TextReceiver r = null;
		try
		{
			r = new TextReceiver( textSenderName, null, null, null, null );
		}
		catch( InstantiationException e )
		{
			// do something b/c you couldn't create the text receiver
			System.out.println( "We couldn't connect text receiver to " + textSenderName + "." );
			System.out.println( e.getMessage( ) );
			return;
		}
		
		TextReceiverTest test = new TextReceiverTest( );
		r.addTextListener( test );
		
		while(true);
	}
}
