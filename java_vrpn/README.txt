Notes on this java implementation:

- you must have sun's java sdk 1.4 installed in its standard place
	(C:\Program Files\Java\j2sdk1.4.0).  Add <that>\bin to your path.

- in Visual Studio, add C:\Program Files\Java\j2sdk1.4.0\jre\lib\rt.jar
	to the project-specific classpath in the Project->"java_vrpn 
	Properties..." menu.

- when you build the java_vrpn Java project, it will generate 
  vrpn_*Remote.h, which are used by the C++ project

- when you build the TrackerRemote C++ project, it will create a .dll that 
        needsto be in your path or executable directory.	

- you can only execute this code with a java 1.4.0 JVM, such as the one that
	comes with the sdk.  Old Microsoft stuff won't work.

- the vrpn directory in .../vrpn/java_vrpn needs to be in your CLASSPATH or
	moved to be in your CLASSPATH.  Alternatively, you can invoke the Java
	VM with the classpath of the java_vrpn directory.

