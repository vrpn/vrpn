/***************************************************************************
 * Use this class to store updates in a vector and get them when you want
 *
 * How it is currently set up...
 * 
 * 1. The vector is emptied when all the updates are returned
 * 2. The vector keeps only the latest update when the mode is set to last
 * 
 * It's easy to change these settings, but if you're too lazy to do it,
 * contact Tatsuhiro Segi (segi@email.unc.edu)
 ***************************************************************************/

package vrpn;

import java.util.Vector;


public class AnalogRemoteListener implements AnalogRemote.AnalogChangeListener
{
	Vector analogUpdates;
	
	boolean returnLastAnalog;
	
	
	public AnalogRemoteListener(AnalogRemote analog)
	{
		analog.addAnalogChangeListener(this);
		
		analogUpdates = new Vector();
		
		returnLastAnalog = true;
	}
	
	
	//** Empty the vector when the mode is set to last
	public synchronized void setModeLastAnalogUpdate()
	{
		returnLastAnalog = true;
		
		if (!analogUpdates.isEmpty())
		{
			Object temp = analogUpdates.lastElement();
		
			analogUpdates.removeAllElements();
			analogUpdates.addElement(temp);
		}
	}
	
	
	public synchronized void setModeAllAnalogUpdates()
	{
		returnLastAnalog = false;
	}
	
	
	public synchronized boolean getModeAnalogUpdate()
	{
		return returnLastAnalog;
	}
	
	
	//** Empty the vector when all the updates are returned
	public synchronized Vector getAnalogUpdate()
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
	
	
	public synchronized AnalogRemote.AnalogUpdate getLastAnalogUpdate()
	{
		AnalogRemote.AnalogUpdate analog = (AnalogRemote.AnalogUpdate)(analogUpdates.lastElement());
		
		return analog;
	}
	
	
	//** Keep only the last update if the mode is set to last
	public synchronized void analogUpdate (AnalogRemote.AnalogUpdate u, AnalogRemote analog)
	{
		if (returnLastAnalog)
		{
			analogUpdates.removeAllElements();
		}
		
		analogUpdates.addElement(u);
	}
}
