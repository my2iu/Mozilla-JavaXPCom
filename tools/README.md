In order to use Gecko's XPCOM objects in Java, you need to have interfaces for them in Java. These tools seem to generate the interfaces for the various XPCOM objects for Java. 

`genifaces` seems to start up Firefox and uses runtime querying to find all the XPCOM objects and generate Java interfaces for them. For now, I've mostly focused on getting this version to work.

`xpidl` seems to go through Firefox XPCOM .idl files that describe the interfaces for XPCOM objects and generates Java interfaces from them. I didn't spend too much time trying to figure out how to get this code to work.