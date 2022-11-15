# The ups debugger (unofficial repository)

This is an ancient debugger for C, C++ and Fortran. The original project page
is [http://ups.sourceforge.net/][homepage].

It has only seen marginal changes in the last 10 years, but still kind of
works.

This repository is a git conversion of the original CVS repository hosted
at SourceForge. It additionally contains minor modifications and ugly
hacks to make `ups` work with modern code in more cases.

## Usage

It uses your system's `libelf`, `libdwarf` and `libiberty`, so make sure you
have these libraries and also their header files installed. `ups` is then
compiled with autotools 

    sudo apt-get install libelf-dev libdwarf-dev libiberty-dev

    ./configure --enable-dwarf
    make

Make sure you compile your programs without optimizations and with
debugging information in "dwarf" format enabled, preferably dwarf version 2.

    gcc -gdwarf-2 -o test test.c
    ups/ups ./test

## More Info

For more information see the original [README](README) file, the [website][homepage],
and the various other documentation files in the repository.

 [homepage]: http://ups.sourceforge.net/
