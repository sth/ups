.tr *\(**
.TH MRESET 3 "30 September 1987"
.SH NAME
Mreset \- reset a menu to its initial state
.SH SYNOPSIS
.nf
.ft B
Mreset(mdesc)
int mdesc;
.ft P
.fi
.SH DESCRIPTION
The menu described by \fImdesc\fP is restored to the state it was in when it
was first displayed by \fIMdisplay\fP(3).
\fIMreset\fP is normally used as an alternative to \fIMclear\fP(3) and is
called after completion of an action initiated by a menu selection.
.SH FILES
/usr/local/lib/libMen3.a
.SH SEE ALSO
J D Bovey \fIA tool for providing programs with menus\fP
.br
Mdisplay(3), Mselect(3)
.SH DIAGNOSTICS
\-1 is returned if the call is unsuccessful, otherwise 0 is returned.
The external integer variable \fImenerr\fP can be examined to find the
reason for a failure ( see \fImenus\fP(3) ).
.SH BUGS
