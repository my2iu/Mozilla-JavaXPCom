To build in Windows, you need a copy of the Firefox source. Build the code for win32.

Then you need to create a file `dirs.mak` with the lines

```
GECKOBASE=...
JDKDIR=...
```

that point to the directories for the JDK and for the Gecko source code. Then you can execute the makefile using

```
nmake -f Makefile.win
```

To run the code, you probably need to copy the resulting dll to your built version of Firefox (i.e. `$(GECKOBASE)\obj-i686-pc-mingw32\dist\bin`). Then, when you run your Java code, you need to set your PATH to point to the Firefox binaries/dlls, and you need to set you `java.library.path` to point there as well.