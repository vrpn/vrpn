import vrpn.*;

public class AnalogRandomValueGenerator extends Thread
{
	vrpn.AnalogRemote analog;
	boolean running = true;
	
	
	public AnalogRandomValueGenerator(vrpn.AnalogRemote a)
	{
		analog = a;
	}
	
	
	public void run()
	{
		while (running)
		{
			try
			{
				Thread.sleep(10);
			}
			
			catch(Exception e){}
			
			int channel = (int)(Math.random()*4);
			double value = (double)(Math.random());
			
			analog.requestValueChange(channel, value);
		}
	}
	
	
	public void stopRunning()
	{
		running = false;
	}
}
