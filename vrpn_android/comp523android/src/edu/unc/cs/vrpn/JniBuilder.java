package edu.unc.cs.vrpn;

import java.util.LinkedList;
import java.util.List;

import jni.JniLayer;

/**
 * Represents an unbuilt JNI translator.
 * 
 * @author Ted
 * 
 */
public class JniBuilder
{
	/**
	 * Represents a VRPN analog input
	 * 
	 * @author Ted
	 * 
	 */
	public static class Analog
	{
		private String name;
		private int channels;

		/**
		 * Initializes a new instance of the {@link JniBuilder.Analog} class.
		 * 
		 * @param name
		 *            - The name of the new analog input
		 * @param channels
		 *            - The number of channels the new analog will have
		 * @throws IllegalArgumentException
		 *             - Thrown if {@code channels} < 0
		 */
		public Analog(String name, int channels)
		{
			this.setName(name);
			this.setChannels(channels);
		}

		/**
		 * Initializes a new instance of the {@link JniBuilder.Analog} class with an
		 * auto-generated name.
		 * 
		 * @param channels
		 *            - The number of channels the new analog will have
		 * @throws IllegalArgumentException
		 *             - Thrown if {@code channels} < 0
		 */
		public Analog(int channels)
		{
			this("Unnamed " + channels + "-dimensional analog" , channels);
		}

		public int getChannels()
		{
			return channels;
		}

		public String getName()
		{
			return name;
		}

		/**
		 * Sets the number of channels for this analog
		 * 
		 * @param channels
		 *            - The number of channels (dimensions) of the analog. This is 1 for a
		 *            slider, 2 for a 2D input, or 3 for a 3D input
		 * @throws IllegalArgumentException
		 *             - Thrown if {@code channels} < 0
		 */
		private void setChannels(int channels)
		{
			if (channels < 0)
				throw new IllegalArgumentException("Analog must have 0 or more channels: Was " + channels);

			this.channels = channels;
		}

		private void setName(String name)
		{
			this.name = name;
		}

		@Override
		public String toString()
		{
			return String.format("%s (%d channel analog)" , this.getName() , this.getChannels());
		}
	}

	/**
	 * Represents a VRPN button input 
	 * @author Ted
	 *
	 */
	public static class Button
	{
		private String name;

		// if a button is not given a name, this value is
		// incremented and assigned. Purely for logging
		private static int unnamedCount = 0;

		/**
		 * Initializes a new instance of the Button class with the given button name.
		 * @param name - the name to be given to this button.
		 */
		public Button(String name)
		{
			if (name == null || name.trim().equals(""))
				name = "Unnamed Button #" + Button.unnamedCount++;

			this.setName(name);
		}

		/**
		 * Initializes a new instance of the Button class with an auto-generated name.
		 */
		public Button()
		{
			this(null);
		}

		public String getName()
		{
			return name;
		}

		private void setName(String value)
		{
			this.name = value;
		}

		@Override
		public String toString()
		{
			return this.getName() + " (button)";
		}
	}

	private static final int DEFAULT_PORT = 3883;	
	private List<Analog> analogs = new LinkedList<Analog>();
	private List<Button> buttons = new LinkedList<Button>();
	private JniLayer jni = null;
	private int port = DEFAULT_PORT;

	public JniBuilder()
	{

	}

	public void add(Analog... analogs)
	{
		if (this.jni != null)
			throw new IllegalStateException("Cannot alter builder after JNI has been instantiated");

		for (Analog analog : analogs)
			if (analog != null)
				this.analogs.add(analog);
	}

	public void add(Button... buttons)
	{
		if (this.jni != null)
			throw new IllegalStateException("Cannot alter builder after JNI has been instantiated");

		for (Button button : buttons)
			if (button != null)
				this.buttons.add(button);
	}
	
	public Analog getAnalog(int number)
	{
		return this.analogs.get(number);
	}
	
	public int getAnalogIndex(Analog analog)
	{
		return this.analogs.indexOf(analog);
	}
	
	public int getButtonIndex(Button button)
	{
		return this.buttons.indexOf(button);
	}
	
	public Button getButton(int number)
	{
		return this.buttons.get(number);
	}
	
	public void setPort(int port)
	{
		this.port = port;
	}
	
	public int getPort()
	{
		return this.port;
	}

	/**
	 * Gets a JNI translator instance for the builder
	 * @return
	 */
	public JniLayer toJni()
	{
		if (this.jni == null)
		{
			int[] analogs = new int[this.analogs.size()];
			int numButtons = this.buttons.size();
			
			for (int i = 0; i < analogs.length; i++)
			{
				analogs[i] = this.analogs.get(i).getChannels();
			}

			this.jni = new JniLayer(analogs , numButtons, port);
		}
		
		return this.jni;
	}
	
	/**
	 * Gets whether or not the Builder is currently accepting changes (if a JNI layer has been instantiated from the builder)
	 * @return
	 */
	public boolean isLocked()
	{
		return this.jni != null;
	}

}
