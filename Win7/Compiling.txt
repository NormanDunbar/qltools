Greetings!

If you are reading this, then I'm assuming you wish to build 'qltools' on Windows. Probably Windows 7
but what do I know - that's what I developed on anyway.

Compiling with Code::Blocks
---------------------------

There is a CodeBlocks Project file named 'qltools-gcc.cbp' which you can use to build the utility using  the free
and very useful Code::Blocks IDE. This builds with a built in 32bit version of the GCC compiler.

The output file is in one of the following locations, depending on whether you built a Debug or Release version:

    win7\bin\Debug\qltools-gcc.exe
    win7\bin\Release\qltools-gcc.exe
    

Compiling with Makefiles
------------------------

There are also a couple of makefiles in here:

Gcc Makefile
~~~~~~~~~~~~

'Makefile.gcc' - which is for the free standing gcc compiler, or, for the version of gcc that comes packaged with the CodeBlocks IDE. In order to run a make with those, you need to run:

    mingw32-make -f Makefile.gcc
    
which will create an executable named 'qltools-gcc.exe' in the win7 folder    

Embarcadero (aka Borland) Makefile
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

'Makefile.embarcadero' - which is for the free Embarcadero/Borland compiler. In order to run this one, you need to:

    make -f Makefile.embarcadero
    
which will create an executable named 'qltools-w7.exe' in the win7 folder.


Visual Studio
-------------

Don't have it, never liked it. Don't want to use it. Sorry.

Have fun.

Norman Dunbar
25th January 2019 onwards.