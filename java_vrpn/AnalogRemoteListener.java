import vrpn.*;
import java.util.Vector;


public class AnalogRemoteListener
	implements vrpn.AnalogRemote.AnalogChangeListener
{
	Vector analogUpdates;
	
	boolean returnLastAnalog;
	
	
	public AnalogRemoteListener(AnalogRemote analog)
	{
		analog.addAnalogChangeListener(this);
		
		analogUpdates = new Vector();
		
		returnLastAnalog = true;
	}
	
	
	public void setModeLastAnalogUpdate()
	{
		returnLastAnalog = true;
		
		Object temp = analogUpdates.lastElement();
		
		analogUpdates.removeAllElements();
		analogUpdates.addElement(temp);
	}
	
	
	public void setModeAllAnalogUpdates()
	{
		returnLastAnalog = false;
	}
	
	
	public boolean getModeAnalogUpdate()
	{
		return returnLastAnalog;
	}
	
	
	public Vector getAnalogUpdate()
	{
		if (analogUpdates.isEmpty())
		{
			return null;
		}
		
		if (returnLastAnalog)
		{
			Vector last = new Vector();
			
			last.addElement(analogUpdates.lastElement());
			
			return last;
		}
		
		else
		{
			Vector all = new Vector();
			
			for (int i=0; i<analogUpdates.size(); i++)
			{
				all.addElement(analogUpdates.elementAt(i));
			}
			
			analogUpdates.removeAllElements();
			
			return all;
		}
	}
	
	
	public AnalogRemote.AnalogUpdate getLastAnalogUpdate()
	{
		AnalogRemote.AnalogUpdate analog = (AnalogRemote.AnalogUpdate)(analogUpdates.lastElement());
		
		return analog;
	}
	
	
	public void analogUpdate (AnalogRemote.AnalogUpdate u, AnalogRemote analog)
	{
		if (returnLastAnalog)
		{
			analogUpdates.removeAllElements();
		}
		
		analogUpdates.addElement(u);
	}
}
