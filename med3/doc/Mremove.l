.tr *\(**
.TH MREMOVE 3 "30 September 1987"
.SH NAME
Mremove \- remove a displayed menu from the screen
.SH SYNOPSIS
.nf
.ft B
Mremove(mdesc;)
int mdesc;
.ft P
.fi
.SH DESCRIPTION
Remove a displayed menu from the screen.
\fIMdesc\fP should be the menu descriptor of an open menu.
If the menu was originally displayed by \fIMdisplay\fP(3) using the
\fIsave\fP option
then the area of screen that was under the menu will be restored,
otherwise it will be left blank.
.SH FILES
/usr/local/lib/libMen3.a
.SH SEE ALSO
J D Bovey \fIA tool for providing programs with menus\fP
.br
Mdisplay(3)
.SH DIAGNOSTICS
\-1 is returned if the call is unsuccessful, otherwise 0 is returned.
The external integer variable \fImenerr\fP can be examined to find the
reason for a failure ( see \fImenus\fP(3) ).
.SH BUGS
