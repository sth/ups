.tr *\(**
.TH MFMODES 3 "30 September 1987"
.SH NAME
Mfmodes \- set the highlighting styles used for menu feedback
.SH SYNOPSIS
.nf
.ft B
#include <menu3.h>
Mfmodes(mdesc,fmode1,fmode2,fmode3)
int mdesc, fmode1, fmode2, fmode3;
.ft P
.fi
.SH DESCRIPTION
This function is used to set the different sorts of highlighting used
to give feedback.
There are three different circumstances under which feedback can be given
and the type of feedback is determined by the associated \fIfmode\fP
argument.
.TP .7i
.B fmode1
Determines the sort of highlighting applied to a button which has a pop-up
submenu displayed.
.TP .7i
.B fmode2
This sort of highlighting is used while the puck button is
depressed.
It is used for a field that conceals no hidden subfields and and it indicates
that some action will be taken when the button is released.
.TP .7i
.B fmode3
Determines the sort of highlighting that will be applied to a selected
field to indicate that the requested action is taking place.
When the action is completed the highlighting can be removed with
\fIMclear\fP(3)
.LP
There are three different types of highlighting available but they can
be \fIor\fP'ed together in any combination to give a total of eight different
types altogether.
The constants defining the three sorts of highlighting are defined in the
header file.
They are:
.TP
.B MH_GREY
- shades the background grey.
.TP
.B MH_BLACK
- inverts the submenu to give a reverse video image.
.TP
.B MH_BOLD
- emboldens any text by shifting it to the right one pixel and then
combining it with the original.
This mode is most useful in combination with MH_BLACK to get a readable
reverse video on some Perq screens.
.LP
The defaults, which would be overwritten by call to Mfmodes, are:
.RS
.sp
.nf
fmode1 = MH_GREY
fmode2 = MH_BLACK
fmode3 = MH_GREY
.fi
.sp
.RE
To disable highlighting set the appropriate \fIfmode\fP  to \fBMH_WHITE\fP.
.SH FILES
/usr/local/lib/libMen3.a
.SH SEE ALSO
J D Bovey \fIA tool for providing programs with menus\fP
.br
Mselect(3), Mclear(3)
.SH DIAGNOSTICS
\-1 is returned if the call is unsuccessful, otherwise 0 is returned.
The external integer variable \fImenerr\fP can be examined to find the
reason for a failure ( see \fImenus\fP(3) ).
.SH BUGS
