.tr *\(**
.TH MREDRAW 3 "30 September 1987"
.SH NAME
Mredraw, Mrepair \- redraw all the menus currently displayed in a window
.SH SYNOPSIS
.nf
.ft B
Mredraw(wdesc)
\fIWINDOW\fP wdesc;

Mrepair(wdesc)
\fIWINDOW\fP wdesc;
.ft P
.fi
.SH DESCRIPTION
\fIMredraw\fP will redraw all the menus currently displayed in the window
identified by \fIwdesc\fP (see \fIMdisplay\fP(3)).
Any saved background areas will be updated  when
\fIMredraw\fP is called and so the background should be redrawn first.
\fIMredraw\fP is the best way of restoring menus after a window size change.
.LP
\fIMrepair\fP redraws the menus without updating the saved backgrounds and
so should be used to repair menus damaged by other graphical output.
.SH FILES
/usr/local/lib/libMen3.a
.SH SEE ALSO
J D Bovey \fIA tool for providing programs with menus\fP
.br
Mdisplay(3)
.SH DIAGNOSTICS
\-1 is returned if the call is unsuccessful.
The external integer variable \fImenerr\fP can be examined to find the
reason for the failure ( see \fImenus\fP(3) ).
.SH BUGS
