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


import vrpn.*;
import java.util.Vector;


public class ButtonRemoteListener implements vrpn.ButtonRemote.ButtonChangeListener
{
	Vector buttonUpdates;
	
	boolean returnLastButton;
	
	
	public ButtonRemoteListener(ButtonRemote button)
	{
		button.addButtonChangeListener(this);
		
		buttonUpdates = new Vector();
		
		returnLastButton = true;
	}
	
	
	//** Empty the vector when the mode is set to last
	public synchronized void setModeLastButtonUpdate()
	{
		returnLastButton = true;
		
		if (!buttonUpdates.isEmpty())
		{
			Object temp = buttonUpdates.lastElement();
		
			buttonUpdates.removeAllElements();
			buttonUpdates.addElement(temp);
		}
	}
	
	
	public synchronized void setModeAllButtonUpdates()
	{
		returnLastButton = false;
	}
	
	
	public synchronized boolean getModeButtonUpdate()
	{
		return returnLastButton;
	}
	
	
	//** Empty the vector when all the updates are returned
	public synchronized Vector getButtonUpdate()
	{
		if (buttonUpdates.isEmpty())
		{
			return null;
		}
		
		if (returnLastButton)
		{
			Vector last = new Vector();
			
			last.addElement(buttonUpdates.lastElement());
			
			return last;
		}
		
		else
		{
			Vector all = new Vector();
			
			for (int i=0; i<buttonUpdates.size(); i++)
			{
				all.addElement(buttonUpdates.elementAt(i));
			}
			
			buttonUpdates.removeAllElements();
			
			return all;
		}
	}
	
	
	public synchronized ButtonRemote.ButtonUpdate getLastButtonUpdate()
	{
		ButtonRemote.ButtonUpdate button = (ButtonRemote.ButtonUpdate)(buttonUpdates.lastElement());
		
		return button;
	}
	
	
	//** Keep only the last update if the mode is set to last
	public synchronized void buttonUpdate (ButtonRemote.ButtonUpdate u, ButtonRemote button)
	{
		if (returnLastButton)
		{
			buttonUpdates.removeAllElements();
		}
		
		buttonUpdates.addElement(u);
	}
}
