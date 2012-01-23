import vrpn.*;

public class AnalogRandomValueGenerator extends Thread
{
	AnalogOutputRemote ao = null;
	boolean running = true;
	
	
	public AnalogRandomValueGenerator( vrpn.AnalogOutputRemote a )
	{
		ao = a;
	}
	
	
	public void run()
	{
		while (running)
		{
			int channel = (int)(Math.random()*4);
			double value = (double)(Math.random());
			
			ao.requestValueChange(channel, value);
		}
			try
			{
				Thread.sleep(10);
			}
			
			catch(Exception e){}
			
	}
	
	
	public void stopRunning()
	{
		running = false;
	}
}
