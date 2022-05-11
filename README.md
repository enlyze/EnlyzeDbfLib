# EnlyzeDbfLib

Library for reading dBASE .dbf database files (e.g. as part of Siemens STEP 7 projects or Borland BDE-based programs)

Compatible to dBASE III PLUS and dBASE IV file formats.

**This library comes without any Visual Studio project file!**  
Just add it as a submodule to your project and add `CDbfReader.cpp` to the list of source files.
Your project also needs to depend on EnlyzeWinStringLib, libc++, and winpthreads.

We can't express these dependencies in a Visual Studio .vcxproj project file, because Visual Studio requires me to know the path to all dependencies beforehand.
However, different projects may import EnlyzeWinStringLib, libc++, and winpthreads from different paths.
For example, EnlyzeWPS-Gateway uses libc++ and winpthreads as part of EnlyzeDAWinGatewayLib while S7-Project-Explorer imports them from a different location.
