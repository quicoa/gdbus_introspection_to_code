# GDBus Introspection to C-code

This is a small software program that reads an xml file containing a D-Bus
interface following the D-Bus Specifications.  The software then outputs a C
source file that can be build and linked together with an application to support
a particular D-Bus interface, without the hassle of manually programming the
GDBus structures or without the potential performance loss or instabilities when
parsing the xml file every runtime.

You'll need to have the development glib-2.0 and gio-2.0 packages installed,
then you can compile the C file with a simple `make`.

After compiling, you can execute the binary with the filename to a D-Bus
introspection xml as the first parameter.  The C code that it produces is printed
to stdout and can be redirected to a file and this file can then be included in
your project to be compiled and used.  The corresponding header file containing
the prototypes of the interface get functions is not included; you'll have to
extract the prototypes yourself.

