.tr *\(**
.TH MSIZE 3 "30 September 1987"
.SH NAME
Msize \- change the size of a menu
.SH SYNOPSIS
.nf
.ft B
Msize(mdesc,width,height)
int mdesc, width, height;
.ft P
.fi
.SH DESCRIPTION
\fIMsize\fP is used to change the size of a menu.
\fIWidth\fP and \fIheight\fP should be the new dimensions of the menu and
\fImdesc\fP should be the \fImenu descriptor\fP as returned by \fIMopen\fP(3)
etc.
If either \fIwidth\fP or \fIheight\fP is zero then that dimension will be
left unchanged.
The menu will \fBnot\fP be redisplayed automatically.
.LP
The dimensions should be those of the menu itself excluding the outer box.
The box is drawn around the outside of the menu.
.LP
The default effect of \fIMsize\fP on a free submenu is to scale it in
proportion but it will be left unscaled if the \fImed3\fP(1) editor
is used to set the \fBfixed\fP flag for the button producing the submenu.
.SH FILES
/usr/local/lib/libMen3.a
.SH SEE ALSO
J D Bovey \fIA tool for providing programs with menus\fP
.br
Mopen(3), Mdisplay(3), Mplace(3)
.SH DIAGNOSTICS
\-1 is returned if the call is unsuccessful, otherwise 0 is returned.
The external integer variable \fImenerr\fP can be examined to find the
reason for failure ( see \fImenus\fP(3) ).
.SH BUGS
