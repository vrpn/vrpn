import java.io.*;
import java.util.*;
import vrpn.*;

public class AnalogListenerTest
{
	public static void main( String[] args ) throws IOException
	{
		String analogName = "tetra_force_server@gold-cs:4700";
		vrpn.AnalogRemote analog = null;
		vrpn.AnalogOutputRemote ao = null;
		
		try
		{
			analog = new vrpn.AnalogRemote( analogName, null, null, null, null );
			ao = new vrpn.AnalogOutputRemote( analogName, null, null, null, null );
		}
		
		catch( InstantiationException e )
		{
			System.out.println("We couldn't connect to analog " + analogName + ".");
			System.out.println(e.getMessage());
			return;
		}
		AnalogRandomValueGenerator thread = new AnalogRandomValueGenerator( ao );
		
		AnalogRemoteListener analogListener = new AnalogRemoteListener( analog );
		
		BufferedReader in = new BufferedReader(new InputStreamReader(System.in));
		
		while(true)
		{
			System.out.println("\nActions:");
			System.out.println("\tC -> check mode");
			System.out.println("\tL -> set mode to last");
			System.out.println("\tA -> set mode to all");
			System.out.println("\tD -> display updates");
			System.out.println("\tR -> request a value change");
			System.out.println("\tS -> start a thread to keep requesting random value changes");
			System.out.println("\tT -> stop the thread");
			System.out.println("\tQ -> quit");
			
			String s = in.readLine();
			
			if (s.equalsIgnoreCase("q"))
			{
				System.exit(0);
			}
			
			if (s.equalsIgnoreCase("c"))
			{
				int last = analogListener.getModeAnalogUpdate();
				
				if (last == AnalogRemoteListener.LAST_UPDATE) System.out.println("Current mode is set to last");
				else System.out.println("Current mode is set to all");
			}
			
			if (s.equalsIgnoreCase("l"))
			{
				analogListener.setModeLastAnalogUpdate();
				
				System.out.println("Mode set to last");
			}
			
			if (s.equalsIgnoreCase("a"))
			{
				analogListener.setModeAllAnalogUpdates();
				
				System.out.println("Mode set to all");
			}
			
			if (s.equalsIgnoreCase("d"))
			{
				Vector updates = analogListener.getAnalogUpdate();
				
				if (updates == null)
				{
					System.out.println("No updates received so far");
				}
				
				else
				{
					for (int i=0; i<updates.size(); i++)
					{
						vrpn.AnalogRemote.AnalogUpdate u = (vrpn.AnalogRemote.AnalogUpdate)updates.elementAt(i);
						
						System.out.println(
							"time:  " + u.msg_time.getTime( ) + "\n" +
							"values: channel1->" + u.channel[0] + " channel2->" + u.channel[1] + 
							" channel3->" + u.channel[2] + " channel4->" + u.channel[3]
							);
					}
				}
			}
			
			if (s.equalsIgnoreCase("r"))
			{
				System.out.println("Input value to send");
				System.out.print("(int) channel: (0-3)");
				
				int channel = Integer.parseInt(in.readLine());
				
				System.out.print("(double) value:");
				
				Double temp = new Double(in.readLine());
				double value = temp.doubleValue();
				
				if (channel<0 | channel>3)
				{
					System.out.println("invalid channel; action aborted");
				}
				
				else
				{
					ao.requestValueChange(channel, value);
					System.out.println("Request completed");
				}
			}
			
			if (s.equalsIgnoreCase("s"))
			{
				thread = new AnalogRandomValueGenerator( ao );
				thread.start();
				
				System.out.println("Thread started");
			}
			
			if (s.equalsIgnoreCase("t"))
			{
				thread.stopRunning();
				
				System.out.println("Thread stopped");
			}
		}
	}
}
