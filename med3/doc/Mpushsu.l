.tr *\(**
.TH MPUSHSU 3 "30 september 1987"
.SH NAME
Mpushsu, Mpopsu \- delay screen update
.SH SYNOPSIS
.nf
.ft B
Mpushsu(wdesc)
\fIWINDOW\fP wdesc;

Mpopsu(wdesc)
\fIWINDOW\fP wdesc;
.ft R
.fi
.SH DESCRIPTION
These functions can be used to delay screen updating.
If the windowing system supports batched updating then the graphical output which
is generated after a call to \fIMpushsu\fP will be delayed until a
matching call to \fIMpopsu\fP.
Pairs of calls to these functions can be nested.
.LP
\fIMpushsu\fP and \fIMpopsu\fP are used internally by functions in the
menu library and so they should be used 
to delay output which includes the use of menus.
If \fIwn\fP is being used then \fIwdesc\fI will be a window number, otherwise
it will be the window representative for the
window enviroment being used (see \fIMdisplay\fP(3)).
.SH FILES
/usr/local/lib/libMen3.a
.SH SEE ALSO
J D Bovey \fIA tool for providing programs with menus\fP
.br
J D Bover and M T Russell \fIUsing the wn graphics library\fI
.br
Mselect(3), Mdisplay(3).
.SH DIAGNOSTICS
\-1 is returned if the call is unsuccessful, otherwise 0 is returned.
The external integer variable \fImenerr\fP can be examined to find the
reason for a failure ( see \fImenus\fP(3) ).
.SH BUGS
