import java.io.*;
import java.util.*;

public class ButtonListenerTest
{
	public static void main( String[] args ) throws IOException
	{
		String buttonName = "Phantom@gold-cs";
		vrpn.ButtonRemote button = null;
		
		try
		{
			button = new vrpn.ButtonRemote(buttonName);
		}
		
		catch(InstantiationException e)
		{
			System.out.println("We couldn't connect to phantom " + buttonName + ".");
			System.out.println(e.getMessage());
			return;
		}
		
		ButtonRemoteListener buttonListener = new ButtonRemoteListener(button);
		
		DataInputStream in = new DataInputStream(System.in);
		
		while(true)
		{
			System.out.println("\nActions:");
			System.out.println("\tC -> check mode");
			System.out.println("\tL -> set mode to last");
			System.out.println("\tA -> set mode to all");
			System.out.println("\tD -> display updates");
			System.out.println("\tQ -> quit");
			
			String s = in.readLine();
			
			if (s.equalsIgnoreCase("q"))
			{
				System.exit(0);
			}
			
			if (s.equalsIgnoreCase("c"))
			{
				boolean last = buttonListener.getModeButtonUpdate();
				
				if (last) System.out.println("Current mode is set to last");
				else System.out.println("Current mode is set to all");
			}
			
			if (s.equalsIgnoreCase("l"))
			{
				buttonListener.setModeLastButtonUpdate();
				
				System.out.println("Mode set to last");
			}
			
			if (s.equalsIgnoreCase("a"))
			{
				buttonListener.setModeAllButtonUpdates();
				
				System.out.println("Mode set to all");
			}
			
			if (s.equalsIgnoreCase("d"))
			{
				Vector updates = buttonListener.getButtonUpdate();
				
				if (updates == null)
				{
					System.out.println("No updates received so far");
				}
				
				else
				{
					for (int i=0; i<updates.size(); i++)
					{
						vrpn.ButtonRemote.ButtonUpdate u = (vrpn.ButtonRemote.ButtonUpdate)updates.elementAt(i);
						
						System.out.println(
							"\ttime:  " + u.msg_time.getTime( ) + "  button:  " + u.button + " " +
							" state:  " + u.state
							);
					}
				}
			}
		}
	}
}
