.tr *\(**
.TH MSTDFONT 3 "30 September 1987"
.SH NAME
Mstdfont \- return a pointer to the standard font
.SH SYNOPSIS
.nf
.ft B
#include <wn.h>
font_t *
Mstdfont()
.ft R
.fi
.SH DESCRIPTION
This function, which takes no arguments, returns a pointer to the menu
package built-in standard font.
The return value is a pointer to \fIwn\fP font structure and can be passed
directly as an argument to \fIMfonts\fP(3).
This is the default font used to display menu captions.
If other fonts are wanted they should 
be specified using \fIMfonts\fP.
.SH FILES
/usr/local/lib/libMen3.a
.SH SEE ALSO
J D Bovey \fIA tool for providing programs with menus\fP
.br
J D Bovey and M T Russell \fIUsing the wn graphics library\fI
.br
Mfonts(3), med3(1)
