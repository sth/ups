.tr *\(**
.TH MPLACE 3 "30 September 1987"
.SH NAME
Mplace \- change the position of a menu
.SH SYNOPSIS
.nf
.ft B
Mplace(mdesc,x,y)
int mdesc, x, y;
.ft P
.fi
.SH DESCRIPTION
\fIMplace\fP changes the position at which the menu described by \fImdesc\fP
will be displayed.
The arguments \fIx\fP and \fIy\fP should be the window-relative coordinates of
the top left hand corner of the menu.
If a menu is displayed without calling \fIMplace\fP first, it will be
positioned in the top left hand corner of the window.
.LP
The coordinates are those of the top-left-hand pixel of the menu itself
rather than being those of the outer box.
The top left hand corner of the box will be at coordinates
(\fIx\fP\ \-\ 1,\fIy\fP\ \-\ 1) which means that if the menu is displayed
at the top-left-hand corner of the window
(ie. with \fIx\fP\ =\ 0 and \fIy\fP\ =\ 0) then  its box will coincide with the
boundary of the window.
.SH FILES
/usr/local/lib/libMen3.a
.SH SEE ALSO
J D Bovey \fIA tool for providing programs with menus\fP
.br
Mopen(3), Msize(3), Mdisplay(3)
.SH DIAGNOSTICS
\-1 is returned if the call is unsuccessful, otherwise 0 is returned.
The external integer variable \fImenerr\fP can be examined to find the
reason for a failure ( see \fImenus\fP(3) ).
.SH BUGS
