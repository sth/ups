.tr *\(**
.TH MDUP 3 "30 September 1987"
.SH NAME
Mdup \- duplicate a menu
.SH SYNOPSIS
.nf
.ft B
Mdup(mdesc)
int mdesc;
.ft P
.fi
.SH DESCRIPTION
\fIMdup\fP copies the menu described by \fImdesc\fP and returns a
descriptor for the duplicate menu.
The new menu will not be displayed but it will be otherwise identical to
the original menu.
.SH FILES
/usr/local/lib/libMen3.a
.SH SEE ALSO
J D Bovey \fIA tool for providing programs with menus\fP
.br
Mdisplay(3), Mopen(3)
.SH DIAGNOSTICS
\-1 is returned if the call is unsuccessful.
The external integer variable \fImenerr\fP can be examined to find the
reason for the failure ( see \fImenus\fP(3) ).
.SH BUGS
