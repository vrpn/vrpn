-----------------------------------------------------------
Getting things compiled and loaded on the Android:
	Open Eclipse (after having installed everything according to the instructions that came with the user and design manual and using the notes in the section below).
	Select "File/Import..." then "Existing Projects into Workspace", then the "vrpn_android/comp523android" directory.
	I did "Project/Clean" and then "Project/Build All"; it can be set to build automatically.
	This built the comp523android.apk file in the bin/ directory.

On Android phone:
	Settings/Application Settings/Unknown sources should be checked.
	Put the Droid into PC mode (may not be needed)
	Plug in USB cable.
	Copy the comp523android.apk file to the SD card.
	Open the Files app on the Droid.
	Go find the file.
	Run it.

On remote machine:
	Run vrpn_print_devices giving it the names Analog0, Analog1, Analog2, and Button0, all @tcp:
		the IP address listed at the top of the Droid phone when the app is running
		(for example: Button0@tcp:192.168.0.17).
	Briefly press the "Search" button for button 0.  There is a 2D touch pad, a slider,
		and accelerometers for analogs (values also shown in the upper-left corner).
		There are two more buttons (1 and 2) on the phone GUI as well.
	Button0: All three buttons.
	Analog0: The touch pad.
	Analog1: The slider on the GUI.
	Analog2: The Accelerometer.

-----------------------------------------------------------
Compiling the JNI code:
	To turn the C source code into an .so library for linking into the Java code, run ndk-build in the jni subdirectory (where the Android.mk file lives).
	Note: After you do this, you need to make clean and then make the project in Eclipse; there are a bunch of errors if you don't make clean first.

	I've not yet figured out how to use javah to create the jni_layer.h file as listed in the manual, but the code compiles fine without it so I think maybe that was a leftover that is no longer needed.  I can make the .so and then build the project and then run the resulting executable, so I think we're done!

-----------------------------------------------------------
Notes made by Russ Taylor as he was working with Eric Boren
to get the code to compile on his laptop:

Follow the instructions on Developer/Android.com/sdk/installing.html.
	Get the stdout connector to help debugging using the log

You need to run in Android SDK Tools/SDK manager.
	Go ahead and let it install what it wants to install.
	Also create a new virtual device: Android 2.2

Open Eclipse and you need to teach it how to open an Android project.
	developer.android.com/sdk/installing.html tells you how to do this.
	Installing ADT Plug-In link tells one part: you add from:
		https://dl-ssl.google.com/android/eclipse/
		(address depends on version of Eclipse)
	If you did this before running SDK manager, you need to update the plug-in.

	Do Window/Preferences/Android/SDK Location and tell it where
		the Android SDK directory is.

	Configure the build path on Eclipse (in response to error message in JNILayer.java)

This lets you do File/New/Android Project.  Or File/New and see Android Project
	Create from existing source.
	Probably from code\comp523android
Or File/Open and open the .project file.

You don't need to run the javah if you just want to build the project as is.	You also don't need to follow the rest of the steps in Com

Failed to open exiting project: trying new project from source:
	File/New/Project/Android Project
		Create from existing source
		Point at the directory
		FAILED -- already a project there.

Tried to do File/Import/Existing Project
	Same problem: "Android requires .class compatibility set to 5.0."
Google says: After importing the project to your workspace, you’ve received the error. So what you need to do next is to right click on the project -> Android Tools -> Fix Project Properties. Now this alone won’t fix the problem, you need to restart Eclipse after this. After that try building the project again and it should work successfully this time (unless you have bugs in the code itself, I did). 

Problem with Override: Google says:
	
Eclipse is defaulting to Java 1.5 and you have classes implementing interface methods (which in Java 1.6 can be annotated with @Override, but in Java 1.5 can only be applied to methods overriding a superclass method). 

Go to your project/ide preferences and set the java compiler level to 1.6 and also make sure you select JRE 1.6 to execute your program from eclipse.

Now it still may show compiler compliance level at 1.6, yet you still see this problem. So now select the link "Configure Project Specific Settings..." and in there you'll see the project is set to 1.5, now change this to 1.6. You'll need to do this for all affected projects. 

(This was under Properties of the right-click menu on the compiler.)

Upper-right table icon on Eclipse, select Other, then turn on Android DDMS.

This builds comp523android.apk in the bin directory.

On Android phone:
	Settings/Application Settings/Unknown sources should be checked.
	Put the Droid into PC mode (may not be needed)
	Plug in USB cable.
	Copy the comp523android.apk file to the SD card.
	Open the Files app on the Droid.
	Go find the file.
	Run it.