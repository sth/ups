.tr *\(**
.TH MMAKE 3 "30 September 1987"
.SH NAME
Mmake \- create a simple menu with captions on successive lines
.SH SYNOPSIS
.nf
.ft B
#include <menu3.h>
Mmake(captions,rvalues,fonts,style,width)
char * captions[];
int *rvalues, *fonts;
int width, style;
.ft P
.fi
.SH DESCRIPTION
\fIMmake\fP creates a simple menu with captions on successive lines and
no submenus. 
\fICaptions\fP should be an array of null terminated
strings with the array terminated by a NULL pointer.
The other arguments are interpreted as follows:
.IP \fBrvalues\fP
If non NULL, \fIrvalues\fP should point to an array of integers which will be
used as return values when buttons from the menu are selected.
If \fIrvalues\fP is NULL then return value of a button will be the position
of the button within the menu, counting from the top, with the top button
having a return value of 1.
.IP \fBfonts\fP
If \fIfonts\fP is not NULL it should point to an array of integers in the
range 1 - 4 which which will be used to set the fonts used to display
the captions.
If \fIfonts\fP is NULL all the captions will be displayed using font 1.
The actual fonts used should be set using the \fIMfonts\fP(3) function.
.IP \fBwidth\fP
The width of the menu in pixels.
.IP \fBstyle\fP
The \fIstyle\fP argument is a combination of two flags: MM_BGLINE which,
if set, tells the menu library not to display the lines separating the
menu buttons, and MM_DRAG which tells the library to operate the menu in
\fIselect-on-drag\fP mode.
.PP
The value returned by \fIMmake\fP is a menu descriptor which can be used by
the menu interface functions in the usual way.
.SH FILES
/usr/local/lib/libMen3.a
.SH SEE ALSO
J D Bovey \fIA tool for providing programs with menus\fP
.br
Mdisplay(3), Mselect(3), med3(1), Mfonts(3) etc.
.SH DIAGNOSTICS
\-1 is returned if the call is unsuccessful.
The external integer variable \fImenerr\fP can be examined to find the
reason for the failure ( see \fImenus\fP(3) ).
.SH BUGS
At present the individual menu slots are each 19 pixels high which is
ok. for the usual fonts but would not be suitable for large fonts.
