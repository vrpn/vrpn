import vrpn.*;

public class ButtonTest 
	implements vrpn.ButtonRemote.ButtonChangeListener

{
	public void buttonUpdate( ButtonRemote.ButtonUpdate u,
									   ButtonRemote button )
	{
		System.out.println( "Button message from vrpn: \n" +
							"\ttime:  " + u.msg_time.getTime( ) + "  button:  " + u.button + "\n" +
							"\tstate:  " + u.state );
	}
	
	

	public static void main( String[] args )
	{
		String buttonName = "Phantom@gold-cs.cs.unc.edu";
		ButtonRemote button = null;
		try
		{
			button = new ButtonRemote( buttonName, null, null, null, null );
		}
		catch( InstantiationException e )
		{
			// do something b/c you couldn't create the button
			System.out.println( "We couldn't connect to button " + buttonName + "." );
			System.out.println( e.getMessage( ) );
			return;
		}
		
		ButtonTest test = new ButtonTest( );
		button.addButtonChangeListener( test );
		
	}
	
	
}
