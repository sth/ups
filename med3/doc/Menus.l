.tr *\(**
.TH MENUS 3 "30 September 1987"
.SH NAME
menus \- error values returned by functions in the menu library.
.SH SYNOPSIS
.nf
.ft B
#include <menu3.h>
.ft P
.fi
.SH DESCRIPTION
The mechanism used to return error values by functions in the menu library
is closely modelled on that used by UNIX system calls.
If a call to a menu function is unsuccessful the function will return \-1.
The reason for the error can then be determined from the value of the
external variable \fImenerr\fP.
.TP
1  MTABFL   \fImenu table full\fP
An attempt has been made to create a new menu descriptor when the maximum
number of menus is open already.
.TP
2  MNOTMEN   \fImenu descriptor not of an open menu\fP
The menu descriptor used did not refer to a currently open menu.
.TP
3  MBADMD  \fI menu descriptor out of range\fP
Either the menu descriptor used was negative or it was too large to
be valid.
.TP
4  MBADFIL  \fIcan't read menu from menu file\fP
The filename used as an argument to \fIMopen\fP referred to a valid file
but the file did not contain a menu.
.TP
5  MNOFIL  \fIcan't open menu file\fP
A file which was specified as containing a menu could not be opened.
.TP
6  MBADWD   \fIbad window file descriptor\fP
A window descriptor argument does not refer to a validly open window.
.TP
7  MNOTDIS   \fImenu not displayed\fP
The menu descriptor referred to a menu that was not currently displayed
on the screen and the function needed a displayed menu.
.TP
8  MBADARG   \fIbad menu function argument\fP
Some error which is not covered by any of the other error values was
discovered in a function argument.
.SH FILES
/usr/local/lib/libMen3.a
.SH SEE ALSO
J D Bovey \fIA tool for providing programs with menus\fP
.br
Mperror(3), intro(2)
.SH BUGS
