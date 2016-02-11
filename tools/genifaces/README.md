Since this code is no longer part of the Mozilla build system, I'm just using straight makefiles to build things.

I've created a Windows makefile `Makefile.win` that can be invoked using `nmake -f Makefile.win`

The build requires that you have a custom version of Firefox somewhere. Download the source code for Firefox and build it for 32-bit Windows. Then create a file `dirs.mak` with the line

```
GECKOBASE= ...
```

that points to the base of your Gecko source code. You also need to make two changes to the Gecko source code.

- There's a problem getting `EnumerateInterfacesWhoseNamesStartWith()` to return all XPCOM interfaces. You can either hack up that method to return all the interfaces without filtering them, or possibly fix up `nsprpub\lib\libc\src\strstr.c` to allow an empty string to match all strings.

- XPCOM now requires you to have JavaScript support to read constants. But access to the JavaScript interfaces is blocked by default in Firefox. You have to remove MOZ_DISABLE_EXPORT_JS=1 from the Firefox build files to get the JavaScript interfaces exported so they can be used here

In order to run the code, you have to copy it to your built Firefox directory (something like `$(GECKOBASE)\obj-i686-pc-mingw32\dist\bin`), and then run it with

```
GenerateJavaInterfaces.exe -d [absolute path to output directory]
```

It then runs for a while and then crashes. But it generates some interfaces.