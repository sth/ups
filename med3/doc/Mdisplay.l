.tr *\(**
.TH MDISPLAY 3 "30 September 1987"
.SH NAME
Mdisplay \- display a menu on the screen
.SH SYNOPSIS
.nf
.ft B
Mdisplay(mdesc,wdesc,save)
int mdesc, save;
\fIWINDOW\fP wdesc;
.ft R
.fi
.SH DESCRIPTION
\fIMdisplay\fP displays the menu described by \fImdesc\fP in the window
described by \fIwdesc\fP.
The type of \fIwdesc\fP depends on the window enviroment under which
is being used.
The currently supported window enviroments are:
.TP
.B Wn
\fIWdesc\fP should be a window number as returned by \fIwn_open_stdwin\fP.
.TP
.B Suntools
\fIWdesc\fP should be a retained \fIPixwin\fP.
.TP
.B ICL Perq using PNX graphics
\fIWdesc\fP should be the file descriptor of an open window.
.LP
If \fIsave\fP is non zero the screen image under the menu will be saved
so that it can be restored when the menu is removed.
.PP
Calling \fIMdisplay\fP for an already displayed menu will cause the old
display to be removed before the new one is created.
If simultaneous displays of the same menu are desired then the menu descriptor
should be duplicated using \fIMdup\fP(3).
.SH FILES
/usr/local/lib/libMen3.a
.SH SEE ALSO
J D Bovey \fIA tool for providing programs with menus\fP
.br
J D Bovey and M T Russell \fIUsing the wn graphics library\fP
.br
Mopen(3), Msize(3), Mplace(3), Mdup(3), Mremove(3), Mselect(3)
.SH DIAGNOSTICS
\-1 is returned if the call is unsuccessful, otherwise 0 is returned.
The external integer variable \fImenerr\fP can be examined to find the
reason for a failure ( see \fImenus\fP(3) ).
.SH BUGS
