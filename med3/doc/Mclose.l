.tr *\(**
.TH MCLOSE 3 "30 September 1987"
.SH NAME
Mclose \- close an open menu
.SH SYNOPSIS
.nf
.ft B
Mclose(mdesc)
int mdesc;
.ft P
.fi
.SH DESCRIPTION
\fIMclose\fP removes the menu from the table of open menus.
If the menu is currently displayed on the screen it will be automatically
removed.
.SH FILES
/usr/local/lib/libMen3.a
.SH SEE ALSO
J D Bovey \fIA tool for providing programs with menus\fP
.br
Mopen(3), Mremove(3), Mdisplay(3)
.SH DIAGNOSTICS
\-1 is returned if the call is unsuccessful, otherwise 0 is returned.
The external integer variable \fImenerr\fP can be examined to find the
reason for a failure ( see \fImenus\fP(3) ).
.SH BUGS
