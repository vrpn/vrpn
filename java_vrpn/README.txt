Notes on this java implementation (mixed Windows and Irix/Linux notes):

- you must have sun's java sdk 1.4 installed in its standard place
	(C:\Program Files\Java\j2sdk1.4.0 in Windows).  
	Add <that>\bin to your path.

- in Visual Studio, add C:\Program Files\Java\j2sdk1.4.0\jre\lib\rt.jar
	to the project-specific classpath in the Project->"java_vrpn 
	Properties..." menu.

- Generate the native-code header files:
  - In Windows/Visual Studio, when you build the java_vrpn Java project, 
    it will generate vrpn_*Remote.h, which are used by the C++ project
  - For non-J++ windows environments, execute the make_header.sh script.
    This script will also build the .class files.

- Build the native library:
  - In Windows/Visual Studio, when you build the C++ project in java_vrpn.dsw, 
    Visual Studio will create a java_vrpn.dll 
  - In Irix or Linux, use the makefile to build libjava_vrpn.so.

  - This library needs to be in your path or executable directory.  Copy 
    java_vrpn.dll / libjava_vrpn.so into the test directory (for instance).  
    If not, you will get this error message: 
			no java_vrpn in java.library.path
			Error initializing java_vrpn.
 			-- Unable to find native library.
  you can use the -D parmeter in java to tell the path to the native library,
  e.g. java -Djava.library.path=$PWD ButtonTest

- you can only execute this code with a java 1.4.0 JVM, such as the one that
	comes with the sdk.  Old Microsoft stuff won't work.

- the resulting vrpn.jar file needs to be in your CLASSPATH or moved to
	be in your CLASSPATH.  Alternatively, you can invoke the Java
	VM with the classpath of the java_vrpn directory.

