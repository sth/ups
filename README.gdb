# $Id$

See the file CHANGES for the status of this release.


Here is how to build the gdb-based version of ups on HP-UX and IRIX.
Note: the gdb-based support has not been tested recently and
probably will not work `out of the box'.  This will be fixed in a
future release.

In addition to the ups distribution tar file you will need the following:

	- a vanilla copy of the gdb version 4.16 source distribution.

	- a reasonably recent version of perl installed somewhere in
          your path.

	- on HP machines only, the GNU C compiler gcc somewhere in
	  your path.

On SunOS 4.1.3 and Solaris 2 you can build a native version of ups
which does not require gdb.  Just unpack the distribution, uncomment
the 4.1.3 lines in the top level Makefile (search for `4.1.3' in the
file), and type `make'.

The build steps for IRIX and HP-UX are slightly different as you need
to build ups with gcc under HP-UX (this is because the HP C compiler
objects to some preprocessor chicanery in ups - the ups source will be
fixed in a future release).

1) Unpack a fresh copy of the gdb 4.16 distribution, and build gdb.
   (The ups build procedure does not modify the gdb source tree, so if
   you already have a built version of gdb-4.16 you can use that.)

   On IRIX:

	gzcat gdb-4.16.tar | tar xf -
	cd gdb-4.16
	./configure
	make

   You need to build gdb with gcc on the HP, where the last step above
   is replaced by:

	make CC=gcc


2) Unpack the ups distribution compressed tar file and cd to the top
   level ups directory:

	gzip -dc ups-3.29-RGA.tar | tar xf -
	cd ups-3.29-RGA


3) Build the modified gdb tree:

	sh gdbconf/mkupsgdb -from path-to-gdb-4.16 .

   (the final `.' is a directory name - don't omit it).
   path-to-gdb-4.16 is the path to the top level directory of the gdb
   4.16 distribution.

   On the HP, you need to specify gcc:

	sh gdbconf/mkupsgdb -from path-to-gdb-4.16 -gcc .

   The mkupsgdb shell script runs various perl scripts to build a
   library `libgdb.a' based on modified versions of the gdb source
   (nearly all the modifications are the #ifdefing out of gdb
   functions not needed by ups).
   
   The result is a new directory below the top level ups directory
   containing a mixture of modified gdb source files and symbolic
   links back to the vanilla gdb source tree.  The directory name
   is formed by adding `.munged' to the last component of the vanilla
   gdb source path (thus it will normally be `gdb-4.16.munged').


4) Build ups.  The make command is currently different on each
   architecture:

        IRIX:  make SHELL=/bin/sh
       HP-UX:  make CC=gcc

   If all goes well this will build a binary called ups/ups-gdb.

The install support has not been tested yet - just copy the binary and
manual page.

Mark Russell
25th May 1995

Updated by Rod Armstrong
12th Nov 1997 
(Using fixes for gdb 4.16 posted by Luciano Lavagno <luciano@Cadence.COM> )
